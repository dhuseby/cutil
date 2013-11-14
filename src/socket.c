/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <poll.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <netinet/tcp.h>

#include "debug.h"
#include "macros.h"
#include "events.h"
#include "list.h"
#include "socket.h"

#if defined(UNIT_TESTING)
#include "test_flags.h"
extern evt_loop_t * el;
#endif

typedef struct addrinfo addrinfo_t;

typedef struct write_dst_s
{
    sockaddr_t addr;
    socklen_t addrlen;

} write_dst_t;

struct socket_s
{
    socket_type_t   type;           /* type of socket */
    int_t           connected;      /* is the socket connected? */
    int_t           bound;          /* is the socket bound? */
    uint8_t *       host;           /* host name */
    uint8_t *       port;           /* port number as string */
    socket_ops_t    ops;            /* socket callbacks */
    aiofd_t         aiofd;          /* the fd management state */
    sockaddr_t      addr;           /* place to stash the sockaddr_storage data */
    socklen_t       addrlen;        /* length of addr */
    sockaddr_t *    readaddr;       /* place to put addr for UDP read */
    socklen_t *     readaddrlen;    /* place to put addr len for UDP read */
    void *          user_data;      /* passed to ops callbacks */
};

/* forward declarations of callback functions */
static int_t socket_aiofd_read_evt_fn( aiofd_t * const aiofd,
                                       size_t const nread,
                                       void * user_data );
static int_t socket_aiofd_write_evt_fn( aiofd_t * const aiofd,
                                        uint8_t const * const buffer,
                                        void * user_data,
                                        void * per_write_data );
static int_t socket_aiofd_error_evt_fn( aiofd_t * const aiofd,
                                        int err,
                                        void * user_data );

static ssize_t socket_udp_read_fn( int fd, 
                                   void * const buf, 
                                   size_t const count, 
                                   void * user_data );
static ssize_t socket_udp_readv_fn( int fd, 
                                    struct iovec * const iov, 
                                    size_t const iovcnt, 
                                    void * user_data );
static ssize_t socket_udp_write_fn( int fd, 
                                    void const * const buf, 
                                    size_t const count, 
                                    void * user_data,
                                    void * per_write_data );
static ssize_t socket_udp_writev_fn( int fd, 
                                     struct iovec const * const iov, 
                                     size_t const iovcnt, 
                                     void * user_data,
                                     void * per_write_data );

static uint8_t * socket_get_addr_flags_string( int flags )
{
    static uint8_t buf[4096];
    uint8_t * p;
    MEMSET( buf, 0, 4096 );
    p = &buf[0];

    if ( flags & AI_NUMERICHOST )
    {
        p += sprintf( p, "AI_NUMERICHOST," );
    }
    if ( flags & AI_PASSIVE )
    {
        p += sprintf( p, "AI_PASSIVE," );
    }
    if ( flags & AI_CANONNAME )
    {
        p += sprintf( p, "AI_CANONNAME," );
    }
    if ( flags & AI_ADDRCONFIG )
    {
        p += sprintf( p, "AI_ADDRCONFIG," );
    }
    if ( flags & AI_V4MAPPED )
    {
        p += sprintf( p, "AI_V4MAPPED," );
    }
    if ( flags & AI_ALL )
    {
        p += sprintf( p, "AI_ALL," );
    }
#if 0
    if ( flags & AI_IDN )
    {
        p += sprintf( p, "AI_IDN" );
    }
    if ( flags & AI_CANONIDN )
    {
        p += sprintf( p, "AI_CANONIDN" );
    }
    if ( flags & AI_IDN_ALLOW_UNASSIGNED )
    {
        p += sprintf( p, "AI_IDN_ALLOW_UNASSIGNED" );
    }
    if ( flags & AI_IDN_USE_STD3_ASCII_RULES )
    {
        p += sprintf( p, "AI_IDN_USE_STD3_ASCII_RULES" );
    }
#endif
    return &buf[0];
}

static uint8_t * socket_get_addrinfo_string( addrinfo_t const * const info )
{
    static uint8_t buf[4096];
    MEMSET( buf, 0, 4096 );

    CHECK_PTR_RET( info, buf );

    sprintf( buf, "Family: %s, Type: %s, Proto: %s, Flags: %s",
             (info->ai_family == AF_INET ? "AF_INET" : (info->ai_family == AF_INET6 ? "AF_INET6" : (info->ai_family == AF_UNSPEC ? "AF_UNSPEC" : "UNKNOWN"))),
             (info->ai_socktype == SOCK_STREAM ? "SOCK_STREAM" : (info->ai_socktype == SOCK_DGRAM ? "SOCK_DGRAM" : "UNKNOWN")),
             (info->ai_protocol == IPPROTO_TCP ? "IPPROTO_TCP" : (info->ai_protocol == IPPROTO_UDP ? "IPPROTO_UDP" : "UNKNOWN")),
             socket_get_addr_flags_string( info->ai_flags ) );
    return &buf[0];
}

void * socket_in_addr( sockaddr_t * const addr )
{
    CHECK_PTR_RET( addr, NULL );
    if ( addr->ss_family == AF_INET )
    {
        return &(((struct sockaddr_in*)addr)->sin_addr);
    }
    else
    {
        return &(((struct sockaddr_in6*)addr)->sin6_addr);
    }
}

static inline in_port_t socket_in_port( sockaddr_t * const addr )
{
    CHECK_PTR_RET( addr, 0 );
    if ( addr->ss_family == AF_INET )
    {
        return ((struct sockaddr_in*)addr)->sin_port;
    }
    else
    {
        return ((struct sockaddr_in6*)addr)->sin6_port;
    }
}

static inline int_t socket_validate_port( uint8_t const * const port )
{
    uint8_t * endp = NULL;
    uint32_t pnum = 0;

    CHECK_PTR_RET( port, FALSE );

    /* convert port string to number */
    pnum = STRTOL( port, (char**)&endp, 10 );

    /* make sure we parsed something */
    CHECK_RET( endp > port, FALSE );

    /* make sure there wasn't any garbage */
    CHECK_RET( *endp == '\0', FALSE );

    /* make sure the port is somewhere in the valid range */
    CHECK_RET( ((pnum >= 0) && (pnum <= 65535)), FALSE );

    return TRUE;
}

static int_t socket_get_error( socket_t * const s, int * errval )
{
    socklen_t len = sizeof(int);
    CHECK_PTR_RET( s, FALSE );
    CHECK_PTR_RET( errval, FALSE );
#if defined(UNIT_TESTING)
    if ( fake_socket_getsockopt ) 
    {
        *errval = fake_socket_errval;
        return fake_socket_get_error_ret;
    }
#endif

    CHECK_RET( getsockopt( s->aiofd.wfd, SOL_SOCKET, SO_ERROR, errval, &len ) < 0, TRUE );
    return FALSE;
}

static int_t socket_open_tcp_socket( socket_t * const s,
                                     evt_loop_t * const el,
                                     int const ai_flags,
                                     int const ai_family )
{
    struct addrinfo hints, *info, *p;
    int n, fd = 0, on = 1;
    int_t success = FALSE;
    int32_t flags;
    static aiofd_ops_t aiofd_ops =
    {
        &socket_aiofd_read_evt_fn,
        &socket_aiofd_write_evt_fn,
        &socket_aiofd_error_evt_fn,
        NULL, NULL, NULL, NULL
    };

    UNIT_TEST_RET( socket_create_tcp_socket );
    
    CHECK_PTR_RET( s, FALSE );
    CHECK_RET( s->type == SOCKET_TCP, FALSE );
    CHECK_RET( s->port != NULL, FALSE );

    /* zero out the hints */
    MEMSET( &hints, 0, sizeof(struct addrinfo) );

    hints.ai_family = ai_family;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = ai_flags;
    hints.ai_protocol = IPPROTO_TCP;

    if ( (n = GETADDRINFO( s->host, s->port, &hints, &info ) ) != 0 )
    {
        DEBUG( "GETADDRINFO: %s\n", gai_strerror(n) );
        return FALSE;
    }

    for( p = info; (p != NULL) && (success != TRUE); p = p->ai_next )
    {
        /* try to open a socket */
        if ( (fd = SOCKET( info->ai_family, info->ai_socktype, info->ai_protocol )) < 0 )
        {
            DEBUG( "failed to open TCP socket\n" );
            continue;
        }

        DEBUG( "created TCP socket [%s]\n", socket_get_addrinfo_string(info) );

        /* turn off TCP naggle algorithm */
        flags = 1;
        if ( SETSOCKOPT( fd, info->ai_protocol, TCP_NODELAY, &on, sizeof(on) ) < 0 )
        {
            DEBUG( "failed to turn on TCP no delay\n" );
            close(fd);
            continue;
        }
        
        DEBUG( "turned on TCP no delay\n" );

        /* set the socket to non blocking mode */
        flags = FCNTL( fd, F_GETFL );
        if( FCNTL( fd, F_SETFL, (flags | O_NONBLOCK) ) < 0 )
        {
            DEBUG( "failed to set TCP socket to non-blocking\n" );
            close(fd);
            continue;
        }
        
        DEBUG( "TCP socket is now non-blocking\n" );

        /* initialize the aiofd to manage the socket */
        if ( aiofd_initialize( &(s->aiofd), fd, fd, &aiofd_ops, el, (void*)s ) == FALSE )
        {
            DEBUG( "failed to initialzie the aiofd\n" );
            close(fd);
            continue;
        }
        
        DEBUG( "aiofd initialized\n" );

        /* store the address info */
        MEMSET( &(s->addr), 0, sizeof(sockaddr_t) );
        MEMCPY( &(s->addr), p->ai_addr, p->ai_addrlen );
        s->addrlen = p->ai_addrlen;

        /* if we get here, we've got a good socket */
        success = TRUE;
    }

    /* release the addr info data */
    freeaddrinfo( info );

    return success;
}

static int_t socket_open_udp_socket( socket_t * const s,
                                     evt_loop_t * const el,
                                     int const ai_flags,
                                     int const ai_family )
{
    struct addrinfo hints, *info, *p;
    int n, fd = 0, on = 1;
    int_t success = FALSE;
    int32_t flags;
    static aiofd_ops_t aiofd_ops =
    {
        &socket_aiofd_read_evt_fn,
        &socket_aiofd_write_evt_fn,
        &socket_aiofd_error_evt_fn,
        &socket_udp_read_fn,
        &socket_udp_write_fn,
        &socket_udp_readv_fn,
        &socket_udp_writev_fn
    };

    UNIT_TEST_RET( socket_create_udp_socket );
    
    CHECK_PTR_RET( s, FALSE );
    CHECK_RET( s->type == SOCKET_UDP, FALSE );
    CHECK_RET( s->port != NULL, FALSE );

    /* zero out the hints */
    MEMSET( &hints, 0, sizeof(struct addrinfo) );

    hints.ai_family = ai_family;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = ai_flags;
    hints.ai_protocol = IPPROTO_UDP;

    if ( (n = GETADDRINFO( s->host, s->port, &hints, &info ) ) != 0 )
    {
        DEBUG( "GETADDRINFO: %s\n", gai_strerror(n) );
        return FALSE;
    }

    for( p = info; (p != NULL) && (success != TRUE); p = p->ai_next )
    {
        /* try to open a socket */
        if ( (fd = SOCKET( info->ai_family, info->ai_socktype, info->ai_protocol )) < 0 )
        {
            LOG( "failed to open UDP socket\n" );
            continue;
        }
        
        DEBUG( "created UDP socket [%s]\n", socket_get_addrinfo_string(info) );

        /* set the socket to non blocking mode */
        flags = FCNTL( fd, F_GETFL );
        if( FCNTL( fd, F_SETFL, (flags | O_NONBLOCK) ) < 0 )
        {
            DEBUG( "failed to set UDP socket to non-blocking\n" );
            close(fd);
            continue;
        }
        
        DEBUG( "UDP socket is now non-blocking\n" );

        /* initialize the aiofd to manage the socket */
        if ( aiofd_initialize( &(s->aiofd), fd, fd, &aiofd_ops, el, (void*)s ) == FALSE )
        {
            DEBUG( "failed to initialzie the aiofd\n" );
            close(fd);
            continue;
        }
        
        DEBUG( "aiofd initialized\n" );

        /* store the address info */
        MEMSET( &(s->addr), 0, sizeof(sockaddr_t) );
        MEMCPY( &(s->addr), p->ai_addr, p->ai_addrlen );
        s->addrlen = p->ai_addrlen;

        /* if we get here, we've got a good socket */
        success = TRUE;
    }

    /* release the addr info data */
    freeaddrinfo( info );

    if ( success )
    {
        DEBUG("UDP socket events enabled: %p\n", (void*)s);
        /* start the socket read event processing... */
        aiofd_enable_read_evt( &(s->aiofd), TRUE );
        /* start the socket write event processing... */
        aiofd_enable_write_evt( &(s->aiofd), TRUE );
    }

    return success;
}

static int_t socket_open_unix_socket( socket_t * const s,
                                      evt_loop_t * const el,
                                      int const ai_flags,
                                      int const ai_family )
{
    int fd = 0;
    int32_t flags;
    static aiofd_ops_t aiofd_ops =
    {
        &socket_aiofd_read_evt_fn,
        &socket_aiofd_write_evt_fn,
        &socket_aiofd_error_evt_fn,
        NULL, NULL, NULL, NULL
    };

    UNIT_TEST_RET( socket_create_unix_socket );
    
    CHECK_PTR_RET( s, FALSE );
    CHECK_RET( s->type == SOCKET_UNIX, FALSE );
    CHECK_RET( s->host != NULL, FALSE );
    CHECK_RET( s->port == NULL, FALSE );

    /* try to open a socket */
    if ( (fd = SOCKET( PF_UNIX, SOCK_STREAM, 0 )) < 0 )
    {
        DEBUG("failed to open UNIX socket\n");
        return FALSE;
    }
    
    DEBUG("created UNIX socket\n");

    /* set the socket to non blocking mode */
    flags = FCNTL( fd, F_GETFL );
    if ( FCNTL( fd, F_SETFL, (flags | O_NONBLOCK) ) < 0 )
    {
        DEBUG( "failed to set UNIX socket to non-blocking\n" );
        close(fd);
        return FALSE;
    }
    
    DEBUG( "UNIX socket is now non-blocking\n" );

    /* initialize the aiofd to manage the socket */
    if ( aiofd_initialize( &(s->aiofd), fd, fd, &aiofd_ops, el, (void*)s ) == FALSE )
    {
        DEBUG( "failed to initialzie the aiofd\n" );
        close(fd);
        return FALSE;
    }
    
    DEBUG( "aiofd initialized\n" );

    /* initialize the socket address struct */
    MEMSET( &(s->addr), 0, sizeof(sockaddr_t) );
    s->addr.ss_family = AF_UNIX;
   
    /* copy the path from the socket to the sockaddr_t */
    strncpy( ((struct sockaddr_un*)&(s->addr))->sun_path, s->host, 107 );

    /* calculate the length of the address struct */
    s->addrlen = sizeof(struct sockaddr_un);

    return TRUE;
}

static int_t socket_open_socket( socket_t * const s, 
                                 evt_loop_t * const el, 
                                 int const ai_flags,
                                 int const ai_family )
{
    CHECK_PTR_RET( s, FALSE );
    CHECK_PTR_RET( el, FALSE );
    CHECK_RET( VALID_SOCKET_TYPE( s->type ), FALSE );

    switch ( s->type )
    {
        case SOCKET_TCP:
            return socket_open_tcp_socket( s, el, ai_flags, ai_family );
        case SOCKET_UDP:
            return socket_open_udp_socket( s, el, ai_flags, ai_family );
        case SOCKET_UNIX:
            return socket_open_unix_socket( s, el, ai_flags, ai_family );
    }

    return FALSE;
}


static int_t socket_aiofd_read_evt_fn( aiofd_t * const aiofd,
                                       size_t const nread,
                                       void * user_data )
{
    socket_t * s = (socket_t*)user_data;
    CHECK_PTR_RET( aiofd, FALSE );
    CHECK_PTR_RET( s, FALSE );

    DEBUG("read callback for socket %p\n", (void*)s );

    if ( s->type == SOCKET_UDP )
    {
        /* this is a normal connection socket, so pass the read event along */
        if ( s->ops.read_fn != NULL )
        {
            DEBUG( "calling socket read callback for socket %p\n", (void*)s );
            (*(s->ops.read_fn))( s, nread, s->user_data );
        }
    }
    else
    {
        if ( socket_is_bound( s ) && socket_is_listening( s ) )
        {
            /* this is a socket accepting incoming connections */
            if ( s->ops.connect_fn != NULL )
            {
                DEBUG( "calling connect callback for incoming connection %p\n", (void*)s );
                if ( (*(s->ops.connect_fn))( s, s->user_data ) != SOCKET_OK )
                {
                    DEBUG( "failed to accept incoming connection!\n" );
                    return FALSE;
                }
            }
        }
        else
        {
            if ( nread == 0 )
            {
                /* the other side disconnected, so follow suit */
                socket_disconnect( s );

                /* return FALSE to turn off the read event processing */
                DEBUG("stopping read events for socket %p\n", (void*)s );
                return FALSE;
            }

            /* this is a normal connection socket, so pass the read event along */
            if ( s->ops.read_fn != NULL )
            {
                DEBUG( "calling socket read callback for socket %p\n", (void*)s );
                (*(s->ops.read_fn))( s, nread, s->user_data );
            }
        }
    }

    return TRUE;
}

static int_t socket_aiofd_write_evt_fn( aiofd_t * const aiofd,
                                        uint8_t const * const buffer,
                                        void * user_data,
                                        void * per_write_data )
{
    int errval = 0;
    socket_t * s = (socket_t*)user_data;
    write_dst_t * wd = (write_dst_t*)per_write_data;
    CHECK_PTR_RET( aiofd, FALSE );
    CHECK_PTR_RET( s, FALSE );

    DEBUG("write callback for socket %p\n", (void*)s);

    if ( s->type == SOCKET_UDP )
    {
        /* free up the struct that stored the UDP write destination */
        FREE( per_write_data );

        /* call the write complete callback to let client know that a particular
         * buffer has been written to the socket. */
        if ( s->ops.write_fn != NULL )
        {
            DEBUG("calling write complete callback for socket %p\n", (void*)s );

            DEBUG( "calling socket write complete callback\n" );
            (*(s->ops.write_fn))( s, buffer, s->user_data );
        }

        if ( (buffer == NULL) && (list_count( &(s->aiofd.wbuf) ) == 0) )
        {
            DEBUG("stopping write events for socket %p\n", (void*)s );
            /* stop the write event processing until we have data to write */
            return FALSE;
        }
    }
    else /* SOCKET_TCP || SOCKET_UNIX */
    {
        if ( socket_is_connected( s ) )
        {
            if ( buffer == NULL )
            {
                if ( list_count( &(s->aiofd.wbuf) ) == 0 )
                {
                    DEBUG("stopping write events for socket %p\n", (void*)s );
                    /* stop the write event processing until we have data to write */
                    return FALSE;
                }

                /* this will keep the write event processing going */
                return TRUE;
            }
            else
            {
                /* call the write complete callback to let client know that a particular
                 * buffer has been written to the socket. */
                if ( s->ops.write_fn != NULL )
                {
                    DEBUG("calling write complete callback for socket %p\n", (void*)s );

                    DEBUG( "calling socket write complete callback\n" );
                    (*(s->ops.write_fn))( s, buffer, s->user_data );
                }

                return TRUE;
            }
        }
        else if ( s->aiofd.rfd != -1 )
        {
            /* we're connecting the socket and this write event callback is to signal
             * that the connect has completed either successfully or failed */
            
            /* check to see if the connect() succeeded */
            if ( !socket_get_error( s, &errval ) )
            {
                DEBUG( "failed to get socket option while checking connect %p\n", (void*)s );
                if ( s->ops.error_fn != NULL )
                {
                    DEBUG( "calling socket error callback\n" );
                    (*(s->ops.error_fn))( s, errno, s->user_data );
                }
                
                /* stop write event processing */
                DEBUG("stopping write events for socket %p\n", (void*)s );
                return FALSE;
            }

            if ( errval == 0 )
            {
                DEBUG( "socket connected\n" );
                s->connected = TRUE;

                if ( s->ops.connect_fn != NULL )
                {
                    DEBUG("calling connect callback for socket %p\n", (void*)s );
                    DEBUG( "calling socket connect callback\n" );
                    /* call the connect callback */
                    (*(s->ops.connect_fn))( s, s->user_data );
                }

                /* we're connected to start read event */
                DEBUG("enabling read event for socket %p\n", (void*)s );
                aiofd_enable_read_evt( &(s->aiofd), TRUE );

                if ( list_count( &(s->aiofd.wbuf) ) == 0 )
                {
                    /* stop the write event processing until we have data to write */
                    DEBUG("stopping write events for socket %p\n", (void*)s );
                    return FALSE;
                }

                /* this will keep the write event processing going */
                return TRUE;
            }
            else
            {
                DEBUG( "socket connect failed\n" );
                if ( s->ops.error_fn != NULL )
                {
                    DEBUG( "calling socket error callback %p\n", (void*)s );
                    (*(s->ops.error_fn))( s, errno, s->user_data );
                }

                /* stop write event processing */
                DEBUG("stopping write events for socket %p\n", (void*)s );
                return FALSE;
            }
        }
    }

    return TRUE;
}

static int_t socket_aiofd_error_evt_fn( aiofd_t * const aiofd,
                                        int err,
                                        void * user_data )
{
    socket_t * s = (socket_t*)user_data;
    CHECK_PTR_RET( aiofd, FALSE );
    CHECK_PTR_RET( s, FALSE );

    if ( s->ops.error_fn != NULL )
    {
        DEBUG( "calling socket error callback\n" );
        (*(s->ops.error_fn))( s, errno, s->user_data );
    }

    return TRUE;
}

static ssize_t socket_udp_read_fn( int fd, void * const buf, size_t const count, void * user_data )
{
    socket_t * s = (socket_t*)user_data;
    CHECK_PTR_RET( s, -1 );

    /* NOTE: we can use blocking calls here because these are usually
     * only called in response to a read callback from the aiofd when
     * there is data to be read from the socket so this won't block. */

    if ( socket_is_connected( s ) )
    {
        return RECV( fd, buf, count, 0 );
    }

    return RECVFROM( fd, buf, count, 0, (struct sockaddr *)s->readaddr, s->readaddrlen );
}

static ssize_t socket_udp_readv_fn( int fd, struct iovec * const iov, size_t const iovcnt, void * user_data )
{
    ssize_t ret = 0;
    struct msghdr msg;
    socket_t * s = (socket_t*)user_data;
    CHECK_PTR_RET( s, -1 );

    /* if the UDP socket is connected, we can use readv to do scatter input */
    if ( socket_is_connected( s ) )
    {
        return READV( fd, iov, iovcnt );
    }

    /* otherwise, use recvmsg to do scatter input and get the source addr */

    /* clear the msghdr */
    MEMSET( &msg, 0, sizeof(struct msghdr) );

    /* set it up so that the data is read into the iovec passed in */
    msg.msg_iov = iov;
    msg.msg_iovlen = iovcnt;

    /* set it so that the sender address is stored in the socket */
    msg.msg_name = (void*)s->readaddr;
    msg.msg_namelen = (s->readaddrlen != NULL ? *(s->readaddrlen) : 0);

    /* receive the data */
    ret = RECVMSG( fd, &msg, 0 );

    if ( s->readaddrlen != NULL )
    {
        *(s->readaddrlen) = msg.msg_namelen;
    }

    return ret;
}

static ssize_t socket_udp_write_fn( int fd, void const * const buf, size_t const count, void * user_data, void * per_write_data )
{
    socket_t * s = (socket_t*)user_data;
    write_dst_t * wd = (write_dst_t*)per_write_data;
    CHECK_PTR_RET( s, -1 );

    if ( socket_is_connected( s ) )
    {
        return SEND( fd, buf, count, 0 );
    }

    /* write destination struct pointer must not be null */
    CHECK_PTR_RET( wd, -1 );

    return SENDTO( fd, buf, count, 0, (struct sockaddr const *)&(wd->addr), wd->addrlen );
}

static ssize_t socket_udp_writev_fn( int fd, struct iovec const * const iov, size_t const iovcnt, void * user_data, void * per_write_data )
{
    struct msghdr msg;
    socket_t * s = (socket_t*)user_data;
    write_dst_t * wd = (write_dst_t*)per_write_data;
    CHECK_PTR_RET( s, -1 );

    if( socket_is_connected( s ) )
    {
        return WRITEV( fd, iov, iovcnt );
    }

    /* write destination struct pointer must not be null */
    CHECK_PTR_RET( wd, -1 );

    /* clear the msghdr */
    MEMSET( &msg, 0, sizeof(struct msghdr) );

    /* set it up so that the data is read into the iovec passed in */
    msg.msg_iov = (struct iovec *)iov;
    msg.msg_iovlen = iovcnt;

    /* set it so that the sender address is stored in the socket */
    msg.msg_name = (void*)&(wd->addr);
    msg.msg_namelen = wd->addrlen;

    /* receive the data */
    return SENDMSG( fd, &msg, 0 );
}

static int socket_initialize( socket_t * const s,
                              socket_type_t const type,
                              uint8_t const * const host,
                              uint8_t const * const port,
                              socket_ops_t * const ops,
                              void * user_data )
{
    UNIT_TEST_FAIL( socket_initialize );

    CHECK_PTR_RET( s, FALSE );
    CHECK_RET( VALID_SOCKET_TYPE( type ), FALSE );
    CHECK_PTR_RET( ops, FALSE );

    MEMSET( (void*)s, 0, sizeof(socket_t) );

    /* store the type */
    s->type = type;

    /* store port number if provided */
    if ( port != NULL )
    {
        /* copy the port num into the socket struct */
        s->port = STRDUP( port );
    }
    
    /* look up and store the hostname if provided */
    if( host != NULL )
    {
        /* copy the hostname into the server struct */
        s->host = STRDUP( host );
    }

    /* copy the ops into place */
    MEMCPY( (void*)&(s->ops), ops, sizeof(socket_ops_t) );

    /* store the user_data pointer */
    s->user_data = user_data;

    return TRUE;
}

socket_t* socket_new( socket_type_t const type, 
                      uint8_t const * const host,
                      uint8_t const * const port,
                      int const ai_flags,
                      int const ai_family,
                      socket_ops_t * const ops,
                      evt_loop_t * const el,
                      void * user_data )
{
    socket_ret_t ret = SOCKET_OK;
    socket_t* s = NULL;
    
    /* allocate the socket struct */
    s = CALLOC( 1, sizeof(socket_t) );
    CHECK_PTR_RET(s, NULL);

    /* initlialize the socket */
    CHECK_GOTO( socket_initialize( s, type, host, port, ops, user_data ), socket_new_fail );
    
    /* open the socket and initialize everything */
    CHECK_GOTO( socket_open_socket( s, el, ai_flags, ai_family ), socket_new_fail );

    return s;

socket_new_fail:
    DEBUG( "socket_new failure: %s\n", check_err_str_ );
    socket_delete( s );
    return NULL;
}

static void socket_deinitialize( socket_t * const s )
{
    /* close the socket */
    if ( s->aiofd.rfd >= 0 )
        socket_disconnect( s );

    /* clean up the host name */
    if ( s->host != NULL )
        FREE( s->host );

    /* clean up the port number */
    if ( s->port != NULL )
        FREE( s->port );
}

void socket_delete( void * s )
{
    socket_t * sock = (socket_t*)s;
    CHECK_PTR( sock );

    socket_deinitialize( sock );

    FREE( sock );
}

int_t socket_get_addr_string( sockaddr_t const * const addr,
                              uint8_t * const buf, 
                              size_t const len )
{
    static uint8_t tmp[1024];
    static uint16_t port;
    static struct in_addr const * v4addr;
    static struct in6_addr const * v6addr;
    CHECK_PTR_RET( addr, FALSE );
    CHECK_PTR_RET( buf, FALSE );
    CHECK_RET( len > 0, FALSE );

    if ( addr->ss_family == AF_INET )
    {
        port = ((struct sockaddr_in const * const)addr)->sin_port;
        v4addr = &((struct sockaddr_in const * const)addr)->sin_addr;
        snprintf( buf, len, "AF_INET %s:%hu", 
                  inet_ntop( addr->ss_family, (void const *)v4addr, tmp, 1024 ),
                  ntohs( port ) );
    }
    else
    {
        port = ((struct sockaddr_in6 const * const)addr)->sin6_port;
        v6addr = &((struct sockaddr_in6 const * const)addr)->sin6_addr;
        snprintf( buf, len, "AF_INET6 %s:%hu", 
              inet_ntop( addr->ss_family, (void const *)v6addr, tmp, 1024 ),
              ntohs( port ) );
    }

    /* make sure it is null terminated */
    buf[len-1] = '\0';

    return TRUE;
}

int_t socket_get_addr( socket_t const * const s,
                       sockaddr_t * const addr,
                       socklen_t * const len )
{
    CHECK_PTR_RET( s, FALSE );
    CHECK_PTR_RET( addr, FALSE );
    CHECK_PTR_RET( len, FALSE );

    MEMCPY( addr, &(s->addr), sizeof(sockaddr_t) );
    *(len) = s->addrlen;
    return TRUE;
}


/* check to see if connected */
int_t socket_is_connected( socket_t const * const s )
{
    UNIT_TEST_RET( socket_connected );

    CHECK_PTR_RET( s, FALSE );
    CHECK_RET( s->aiofd.rfd >= 0, FALSE );
    return s->connected;
}

socket_ret_t socket_connect( socket_t * const s )
{
    CHECK_PTR_RET( s, SOCKET_BADPARAM );
    CHECK_RET( !socket_is_connected( s ), SOCKET_CONNECTED );
    CHECK_RET( VALID_SOCKET_TYPE( s->type ), SOCKET_ERROR );

    /* try to make the connection */
    if ( CONNECT( s->aiofd.rfd, (struct sockaddr*)&(s->addr), s->addrlen) < 0 )
    {
        if ( ERRNO != EINPROGRESS )
        {
            DEBUG("failed to initiate connect to the server\n");
            return SOCKET_CONNECT_FAIL;
        }
        DEBUG( "connection in progress\n" );
    }

    if ( s->type != SOCKET_UDP )
    {
        /* start the socket write event processing to catch successful connect */
        aiofd_enable_write_evt( &(s->aiofd), TRUE );
    }

    return SOCKET_OK;
}


/* check to see if bound */
int_t socket_is_bound( socket_t const * const s )
{
    UNIT_TEST_RET( socket_bound );

    CHECK_PTR_RET(s, FALSE);
    CHECK_RET( s->aiofd.rfd >= 0, FALSE );
    return s->bound;
}


socket_ret_t socket_bind( socket_t * const s )
{
    int on = 1, status = 0;
    struct stat st;
    CHECK_PTR_RET( s, SOCKET_BADPARAM );
    CHECK_RET( !socket_is_bound( s ), SOCKET_BOUND );
    CHECK_RET( VALID_SOCKET_TYPE( s->type ), SOCKET_ERROR );

    switch ( s->type )
    {
        case SOCKET_UDP:
        case SOCKET_TCP:
            /* turn on address reuse */
            if ( SETSOCKOPT( s->aiofd.rfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) ) < 0 )
            {
                DEBUG( "failed to set the IP socket to reuse addr\n" );
                close( s->aiofd.rfd );
                return SOCKET_ERROR;
            }
            
            DEBUG( "turned on IP socket address reuse\n" );

            break;

        case SOCKET_UNIX:
            /* do not unlink without checking first */
            MEMSET( &st, 0, sizeof( struct stat ) );
            if ( (status = STAT( s->host, &st )) == 0 )
            {
                /* we will only unlink a socket node, all other node types
                 * are an error */
                if ( (st.st_mode & S_IFMT) != S_IFSOCK )
                {
                    DEBUG( "can't create unix socket, existing node is not a socket\n" );
                    return SOCKET_OPEN_FAIL;
                }

                /* unlink the socket node that is already there */
                if ( UNLINK( s->host ) != 0 )
                {
                    LOG( "failed to unlink existing socket node\n" );
                    return SOCKET_OPEN_FAIL;
                }
            }
            else
            {
                /* if stat doesn't return 0, only ENOENT is valid since
                 * it indicates that there isn't a node at that path */
                if ( ERRNO != ENOENT )
                {
                    DEBUG( "can't create unix socket, can't stat node\n" );
                    return SOCKET_OPEN_FAIL;
                }
            }

            break;
    }

    /* bind the socket */
    if ( BIND( s->aiofd.rfd, (struct sockaddr const *)&(s->addr), s->addrlen ) < 0 )
    {
        DEBUG( "failed to bind IP socket\n" );
        aiofd_deinitialize( &(s->aiofd) );
        close(s->aiofd.rfd);
        return SOCKET_ERROR;
    }
    
    DEBUG( "socket is bound %p fd: %d\n", (void*)s, s->aiofd.rfd );

    /* flag the socket as bound */
    s->bound = TRUE;

    if ( s->type == SOCKET_UDP )
    {
    }

    return SOCKET_OK;
}

socket_ret_t socket_listen( socket_t * const s, int const backlog )
{
    static uint8_t buf[1024];
    CHECK_PTR_RET( s, SOCKET_BADPARAM );
    CHECK_RET( socket_is_bound( s ), SOCKET_BOUND );
    CHECK_RET( !socket_is_connected( s ), SOCKET_CONNECTED );

    /* don't call listen on UDP sockets */
    CHECK_RET( s->type != SOCKET_UDP, SOCKET_ERROR );

    /* start the socket read event processing to catch incoming connections */
    CHECK_RET( aiofd_enable_read_evt( &(s->aiofd), TRUE ), SOCKET_ERROR );

    /* now begin listening for incoming connections */
    if ( LISTEN( s->aiofd.rfd, backlog ) < 0 )
    {
        DEBUG( "failed to listen (errno: %d)\n", errno );
        return SOCKET_ERROR;
    }

    MEMSET( buf, 0, 1024 );
    socket_get_addr_string( &(s->addr), buf, 1024 );
    DEBUG( "socket is listening %p fd: %d -- %s\n", (void*)s, s->aiofd.rfd, buf );

    /* set the listen flag so that it doesn't error on 0 size read callbacks */
    aiofd_set_listen( &(s->aiofd), TRUE );

    return SOCKET_OK;
}

int_t socket_is_listening( socket_t const * const s )
{
    CHECK_PTR_RET( s, FALSE );
    return aiofd_get_listen( &(s->aiofd) );
}

socket_t * socket_accept( socket_t * const s,
                          socket_ops_t * const ops,
                          evt_loop_t * const el,
                          void * user_data )
{
    int fd;
    socklen_t len = 0;
    int32_t flags;
    socket_t * client;
    struct sockaddr_in addr;
    struct sockaddr_in6 addr6;
    struct sockaddr_un un_addr;
    static aiofd_ops_t aiofd_ops =
    {
        &socket_aiofd_read_evt_fn,
        &socket_aiofd_write_evt_fn,
        &socket_aiofd_error_evt_fn,
        NULL, NULL, NULL, NULL
    };

    CHECK_PTR_RET( s, NULL );
    CHECK_PTR_RET( ops, NULL );
    CHECK_PTR_RET( el, NULL );
    CHECK_RET( socket_is_bound( s ), NULL );
    CHECK_RET( VALID_SOCKET_TYPE( s->type ), NULL );

    /* don't call accept on UDP sockets */
    CHECK_RET( s->type != SOCKET_UDP, NULL );

    /* create a new socket for the incoming connection */
    client = CALLOC( 1, sizeof(socket_t) );
    CHECK_PTR_RET_MSG( client, NULL, "failed to allocate new socket struct\n" );

    /* initlialize the socket */
    CHECK_GOTO( socket_initialize( client, s->type, NULL, NULL, ops, user_data ), socket_accept_fail );
    s->addrlen = sizeof(s->addr);

    /* accept the incoming connection */
    CHECK_GOTO( (fd = ACCEPT( s->aiofd.rfd, (struct sockaddr *)&(s->addr), &(s->addrlen) ) ) >= 0, socket_accept_fail );
    
    /* do the connect based on the type */
    switch(s->type)
    {
        case SOCKET_UDP:
            break;
        case SOCKET_TCP:

            /* turn off TCP naggle algorithm */
            flags = 1;
            CHECK_GOTO(SETSOCKOPT( fd, IPPROTO_TCP, TCP_NODELAY, (char*)&flags, sizeof(flags) ) == 0, socket_accept_fail );
            DEBUG("turned on TCP no delay\n");

            /* fill in the host string */
            FREE( client->host );
            client->host = CALLOC( HOSTNAME_BUFFER_LEN, sizeof(uint8_t) );
            CHECK_PTR_GOTO( client->host, socket_accept_fail );
            inet_ntop( client->addr.ss_family,
                       socket_in_addr( &client->addr ),
                       client->host,
                       HOSTNAME_BUFFER_LEN );

            /* fill in the port string */
            FREE( client->port );
            client->port = CALLOC( PORT_BUFFER_LEN, sizeof(uint8_t) );
            CHECK_PTR_GOTO( client->port, socket_accept_fail );
            snprintf( client->port, PORT_BUFFER_LEN, "%hu", socket_in_port( &client->addr ) );

            break;
    
        case SOCKET_UNIX:

            /* store the connection information */
            client->host = STRDUP( ((struct sockaddr_un*)&(client->addr))->sun_path );

            /* make sure the copy succeeded */
            CHECK_PTR_GOTO( client->host, socket_accept_fail );

            break;
    }

    /* set the socket to non blocking mode */
    flags = FCNTL( fd, F_GETFL );
    CHECK_GOTO( FCNTL( fd, F_SETFL, (flags | O_NONBLOCK) ) == 0, socket_accept_fail );
    DEBUG("socket is now non-blocking\n");

    /* initialize the aiofd to manage the socket */
    CHECK_GOTO( aiofd_initialize( &(client->aiofd), fd, fd, &aiofd_ops, el, (void*)client ), socket_accept_fail );
    
    DEBUG("aiofd initialized\n");
    
    DEBUG( "socket connected\n" );
    client->connected = TRUE;

    if ( client->ops.connect_fn != NULL )
    {
        DEBUG( "calling socket connect callback\n" );
        /* call the connect callback */
        (*(client->ops.connect_fn))( client, client->user_data );
    }

    /* we're connected so start read event */
    CHECK_GOTO( aiofd_enable_read_evt( &(client->aiofd), TRUE ), socket_accept_fail );

    return client;

socket_accept_fail:
    DEBUG( "socket_accept failure: %s\n", check_err_str_ );
    DEBUG( "ERRNO is: %d -- %s\n", ERRNO, strerror(ERRNO));
    socket_delete( (void*)client );
    return NULL;
}

socket_ret_t socket_disconnect( socket_t* const s )
{
    CHECK_PTR_RET(s, SOCKET_BADPARAM);

    /* shutdown the aiofd */
    aiofd_deinitialize( &(s->aiofd) );

    if ( s->aiofd.rfd > 0 )
    {
        /* shutdown the socket */
        shutdown( s->aiofd.rfd, SHUT_RDWR );
        close( s->aiofd.rfd );
        s->aiofd.rfd = -1;
        s->aiofd.wfd = -1;
    }

    /* we're now disconnected */
    s->connected = FALSE;

    if ( s->type != SOCKET_UDP )
    {
        if ( s->ops.disconnect_fn != NULL )
        {
            /* call the disconnect callback */
            (*(s->ops.disconnect_fn))( s, s->user_data );
        }
    }

    return SOCKET_OK;
}

socket_type_t socket_get_type( socket_t * const s )
{
    CHECK_PTR_RET( s, SOCKET_UNKNOWN );
    return s->type;
}

ssize_t socket_read( socket_t* const s, 
                     uint8_t * const buffer, 
                     size_t const n )
{
    CHECK_PTR_RET( s, -1 );
    CHECK_PTR_RET( buffer, -1 );
    CHECK_RET( n > 0, -1 );

    /* can't use this with UDP sockets that aren't connected */
    if ( (s->type == SOCKET_UDP) && !socket_is_bound( s ) )
        return SOCKET_ERROR;

    return aiofd_read( &(s->aiofd), buffer, n );
}

ssize_t socket_readv( socket_t * const s,
                      struct iovec * const iov,
                      size_t const iovcnt )
{
    CHECK_PTR_RET( s, -1 );
    CHECK_PTR_RET( iov, -1 );
    CHECK_RET( iovcnt > 0, -1 );

    /* can't use this with UDP sockets that aren't connected */
    if ( (s->type == SOCKET_UDP) && !socket_is_bound( s ) )
        return SOCKET_ERROR;

    return aiofd_readv( &(s->aiofd), iov, iovcnt );
}

ssize_t socket_read_from( socket_t* const s, 
                          uint8_t * const buffer, 
                          size_t const n,
                          sockaddr_t * const addr,
                          socklen_t * const addrlen )
{
    CHECK_PTR_RET( s, -1 );
    CHECK_PTR_RET( buffer, -1 );
    CHECK_RET( n > 0, -1 );

    /* initialize the place to put the source addr */
    s->readaddr = addr;
    s->readaddrlen = addrlen;

    return aiofd_read( &(s->aiofd), buffer, n );
}

ssize_t socket_readv_from( socket_t * const s,
                           struct iovec * const iov,
                           size_t const iovcnt,
                           sockaddr_t * const addr,
                           socklen_t * const addrlen )
{
    CHECK_PTR_RET( s, -1 );
    CHECK_PTR_RET( iov, -1 );
    CHECK_RET( iovcnt > 0, -1 );

    /* initialize the place to put the source addr */
    s->readaddr = addr;
    s->readaddrlen = addrlen;

    return aiofd_readv( &(s->aiofd), iov, iovcnt );
}

socket_ret_t socket_write( socket_t * const s,
                           uint8_t const * const buffer,
                           size_t const n )
{
    CHECK_PTR_RET( s, SOCKET_BADPARAM );

    /* can't use this with UDP sockets that aren't connected */
    if ( (s->type == SOCKET_UDP) && !socket_is_connected( s ) )
        return SOCKET_ERROR;

    return ( aiofd_write( &(s->aiofd), buffer, n, NULL ) ? SOCKET_OK : SOCKET_ERROR );
}

socket_ret_t socket_writev( socket_t * const s,
                            struct iovec const * const iov,
                            size_t const iovcnt )
{
    CHECK_PTR_RET( s, SOCKET_BADPARAM );

    /* can't use this with UDP sockets that aren't connected */
    if ( (s->type == SOCKET_UDP) && !socket_is_connected( s ) )
        return SOCKET_ERROR;

    return ( aiofd_writev( &(s->aiofd), iov, iovcnt, NULL ) ? SOCKET_OK : SOCKET_ERROR );
}

socket_ret_t socket_write_to( socket_t * const s,
                              uint8_t const * const buffer,
                              size_t const n,
                              sockaddr_t const * const addr,
                              socklen_t const addrlen )
{
    write_dst_t * wd = NULL;
    CHECK_PTR_RET( s, SOCKET_BADPARAM );

    /* we'll get the pointer to this memory back in the write callback */
    wd = CALLOC( 1, sizeof(write_dst_t) );
    if ( wd == NULL )
    {
        DEBUG("failed to calloc write destination struct\n");
        return SOCKET_ERROR;
    }

    /* copy the destination information */
    MEMCPY( &(wd->addr), addr, sizeof(sockaddr_t) );
    wd->addrlen = addrlen;

    return ( aiofd_write( &(s->aiofd), buffer, n, (void*)wd ) ? SOCKET_OK : SOCKET_ERROR );
}

socket_ret_t socket_writev_to( socket_t * const s,
                               struct iovec const * const iov,
                               size_t const iovcnt,
                               sockaddr_t const * const addr,
                               socklen_t const addrlen )
{
    write_dst_t * wd = NULL;
    CHECK_PTR_RET( s, SOCKET_BADPARAM );

    /* we'll get the pointer to this memory back in the write callback */
    wd = CALLOC( 1, sizeof(write_dst_t) );
    if ( wd == NULL )
    {
        DEBUG("failed to calloc write destination struct\n");
        return SOCKET_ERROR;
    }

    /* copy the destination information */
    MEMCPY( &(wd->addr), addr, sizeof(sockaddr_t) );
    wd->addrlen = addrlen;

    return ( aiofd_writev( &(s->aiofd), iov, iovcnt, (void*)wd ) ? SOCKET_OK : SOCKET_ERROR );
}

/* flush the socket output */
socket_ret_t socket_flush( socket_t* const s )
{
    CHECK_PTR_RET(s, SOCKET_BADPARAM);
    return aiofd_flush( &(s->aiofd) );
}

#if defined(UNIT_TESTING)

#include <CUnit/Basic.h>

socket_ret_t test_error_fn_ret = SOCKET_OK;
socket_ret_t test_connect_fn_ret = SOCKET_OK;

static socket_ret_t test_error_fn( socket_t * const s, int err, void * user_data )
{
    *((int_t*)user_data) = TRUE;
    return test_error_fn_ret;
}

static socket_ret_t test_connect_fn( socket_t * const s, void * user_data )
{
    *((int_t*)user_data) = TRUE;
    return test_connect_fn_ret;
}

void test_socket_private_functions( void )
{
    int test_flag;
    uint8_t * const host = UT( STRDUP( "foo.com" ) );
    uint8_t buf[64];
    socket_t s, *p;
    socket_ops_t ops;

    reset_test_flags();

    MEMSET( &s, 0, sizeof(socket_t) );
    MEMSET( &ops, 0, sizeof(socket_ops_t) );

    /* test socket_get_errror */
    CU_ASSERT_FALSE( socket_get_error( NULL, NULL ) );
    CU_ASSERT_FALSE( socket_get_error( &s, NULL ) );
    fake_socket_getsockopt = TRUE;
    fake_socket_errval = TRUE;
    fake_socket_get_error_ret = TRUE;
    test_flag = FALSE;
    CU_ASSERT_TRUE( socket_get_error( &s, &test_flag ) );
    CU_ASSERT_TRUE( test_flag );
    
    reset_test_flags();

    /* test socket_aiofd_write_evt_fn */
    CU_ASSERT_FALSE( socket_aiofd_write_evt_fn( NULL, NULL, NULL, NULL ) );
    CU_ASSERT_FALSE( socket_aiofd_write_evt_fn( &(s.aiofd), NULL, NULL, NULL ) );
    s.connected = TRUE;
    CU_ASSERT_TRUE( list_initialize( &(s.aiofd.wbuf), 2, NULL ) );
    CU_ASSERT_TRUE( list_push_tail( &(s.aiofd.wbuf), (void*)host ) );
    CU_ASSERT_TRUE( socket_aiofd_write_evt_fn( &(s.aiofd), NULL, (void*)(&s), NULL ) );
    CU_ASSERT_TRUE( socket_aiofd_write_evt_fn( &(s.aiofd), (uint8_t const * const)&host, (void*)(&s), NULL ) );
    s.connected = FALSE;
    s.aiofd.rfd = STDIN_FILENO;
    CU_ASSERT_FALSE( socket_aiofd_write_evt_fn( &(s.aiofd), (uint8_t const * const)&host, (void*)(&s), NULL ) );
    s.ops.error_fn = &test_error_fn;
    test_flag = FALSE;
    s.user_data = &test_flag;
    CU_ASSERT_FALSE( socket_aiofd_write_evt_fn( &(s.aiofd), (uint8_t const * const)&host, (void*)(&s), NULL ) );
    CU_ASSERT_TRUE( test_flag );
    fake_socket_getsockopt = TRUE;
    fake_socket_errval = 0;
    fake_socket_get_error_ret = TRUE;
    list_clear( &(s.aiofd.wbuf) );
    CU_ASSERT_FALSE( socket_aiofd_write_evt_fn( &(s.aiofd), (uint8_t const * const)&host, (void*)(&s), NULL ) );
    fake_socket_errval = -1;
    s.ops.error_fn = NULL;
    s.connected = FALSE;
    CU_ASSERT_FALSE( socket_aiofd_write_evt_fn( &(s.aiofd), (uint8_t const * const)&host, (void*)(&s), NULL ) );
    fake_socket_errval = 0;
    fake_socket_getsockopt = FALSE;

    reset_test_flags();

    /* test socket_aiofd_error_evt_fn */
    CU_ASSERT_FALSE( socket_aiofd_error_evt_fn( NULL, 0, NULL ) );
    CU_ASSERT_FALSE( socket_aiofd_error_evt_fn( &(s.aiofd), 0, NULL ) );
    CU_ASSERT_TRUE( socket_aiofd_error_evt_fn( &(s.aiofd), 0, (void*)(&s) ) );
    test_flag = FALSE;
    s.ops.error_fn = &test_error_fn;
    CU_ASSERT_TRUE( socket_aiofd_error_evt_fn( &(s.aiofd), 0, (void*)(&s) ) );
    CU_ASSERT_TRUE( test_flag );

    reset_test_flags();

    /* test socket_aiofd_read_evt_fn */
    CU_ASSERT_FALSE( socket_aiofd_read_evt_fn( NULL, 0, NULL ) );
    CU_ASSERT_FALSE( socket_aiofd_read_evt_fn( &(s.aiofd), 0, NULL ) );
    s.aiofd.rfd = 0;
    CU_ASSERT_FALSE( socket_aiofd_read_evt_fn( &(s.aiofd), 0, (void*)(&s) ) );
    CU_ASSERT_TRUE( socket_aiofd_read_evt_fn( &(s.aiofd), 1, (void*)(&s) ) );

    /* call socket_aiofd_read_evt_fn as if we're bound and listening but
     * don't have a connect callback set so the test_flag should not get 
     * changed to TRUE */
    s.bound = TRUE;
    s.aiofd.listen = TRUE;
    test_flag = FALSE;
    CU_ASSERT_TRUE( socket_aiofd_read_evt_fn( &(s.aiofd), 0, (void*)(&s) ) );
    CU_ASSERT_FALSE( test_flag );

    /* now we'll set up a connect callback and have it return an error */
    s.ops.connect_fn = &test_connect_fn;
    test_connect_fn_ret = SOCKET_ERROR;
    test_flag = FALSE;
    CU_ASSERT_FALSE( socket_aiofd_read_evt_fn( &(s.aiofd), 0, (void*)(&s) ) );
    CU_ASSERT_TRUE( test_flag );

    reset_test_flags();

    /* test socket_initialize */
    fake_socket = TRUE;
    fake_setsockopt = TRUE;
    fake_fcntl = TRUE;
    fake_aiofd_initialize = TRUE;
    MEMSET( &s, 0, sizeof( socket_t ) );
    MEMSET( &ops, 0, sizeof( socket_ops_t ) );
    CU_ASSERT_FALSE( socket_initialize( NULL, SOCKET_UNKNOWN, NULL, NULL, NULL, NULL ) );
    CU_ASSERT_FALSE( socket_initialize( &s, SOCKET_UNKNOWN, NULL, NULL, NULL, NULL ) );
    CU_ASSERT_FALSE( socket_initialize( &s, SOCKET_LAST, NULL, NULL, NULL, NULL ) );
    CU_ASSERT_FALSE( socket_initialize( &s, SOCKET_TCP, NULL, NULL, NULL, NULL ) );
    CU_ASSERT_TRUE( socket_initialize( &s, SOCKET_TCP, NULL, NULL, &ops, NULL ) );
   
    reset_test_flags();

    /* test socket_is_connected */
    MEMSET( &s, 0, sizeof(socket_t) );
    CU_ASSERT_FALSE( socket_is_connected( NULL ) );
    s.aiofd.rfd = -1;
    CU_ASSERT_FALSE( socket_is_connected( &s ) );
    s.aiofd.rfd = STDIN_FILENO;
    CU_ASSERT_FALSE( socket_is_connected( &s ) );
    s.connected = TRUE;
    CU_ASSERT_TRUE( socket_is_connected( &s ) );

    reset_test_flags();

    /* test socket_is_bound */
    MEMSET( &s, 0, sizeof(socket_t) );
    CU_ASSERT_FALSE( socket_is_bound( NULL ) );
    s.aiofd.rfd = -1;
    CU_ASSERT_FALSE( socket_is_bound( &s ) );
    s.aiofd.rfd = STDIN_FILENO;
    CU_ASSERT_FALSE( socket_is_bound( &s ) );
    s.bound = TRUE;
    CU_ASSERT_TRUE( socket_is_bound( &s ) );

    reset_test_flags();

    /* test socket_connect */
#if 0
    MEMSET( &s, 0, sizeof(socket_t) );
    CU_ASSERT_EQUAL( socket_connect( NULL, NULL, NULL, NULL ), SOCKET_BADPARAM );
    CU_ASSERT_EQUAL( socket_connect( &s, NULL, NULL, NULL ), SOCKET_BADHOSTNAME );
    CU_ASSERT_EQUAL( socket_connect( &s, "foo.com", NULL, NULL ), SOCKET_INVALIDPORT );
    s.type = SOCKET_LAST;
    CU_ASSERT_EQUAL( socket_connect( &s, "foo.com", "0", el ), SOCKET_ERROR );
    s.type = SOCKET_UNKNOWN;
    CU_ASSERT_EQUAL( socket_connect( &s, "foo.com", NULL, el ), SOCKET_ERROR );
    s.type = SOCKET_TCP;
    s.host = STRDUP("foo.com");
    CU_ASSERT_EQUAL( socket_connect( &s, NULL, NULL, el ), SOCKET_INVALIDPORT );
    fake_socket_connected = TRUE;
    fake_socket_connected_ret = TRUE;
    CU_ASSERT_EQUAL( socket_connect( &s, NULL, "1024", el ), SOCKET_CONNECTED );
    FREE( s.port );
    s.port = STRDUP( "1024" );
    CU_ASSERT_EQUAL( socket_connect( &s, NULL, NULL, el ), SOCKET_CONNECTED );
    FREE( s.port );
    s.port = NULL;
    s.type = SOCKET_UNIX;
    CU_ASSERT_EQUAL( socket_connect( &s, NULL, NULL, el ), SOCKET_CONNECTED );
    s.type = SOCKET_TCP;
    fake_socket_connected_ret = FALSE;
    CU_ASSERT_EQUAL( socket_connect( &s, NULL, "1024", el ), SOCKET_OK );
    fake_socket_connect = TRUE;
    fake_socket_connect_ret = TRUE;
    CU_ASSERT_EQUAL( socket_connect( &s, NULL, NULL, el ), SOCKET_OK );
    fake_socket_connect_ret = FALSE;
    CU_ASSERT_EQUAL( socket_connect( &s, NULL, NULL, el ), SOCKET_OK );
    s.type = SOCKET_UNIX;
    fake_socket_connect_ret = TRUE;
    CU_ASSERT_EQUAL( socket_connect( &s, NULL, NULL, el ), SOCKET_OPEN_FAIL );
    fake_socket_connect_ret = FALSE;
    CU_ASSERT_EQUAL( socket_connect( &s, NULL, NULL, el ), SOCKET_OPEN_FAIL );
    FREE( s.host );
    s.host = NULL;
    FREE( s.port );
    s.port = NULL;

    reset_test_flags();

    /* test socket_bind */
    fake_setsockopt = TRUE;
    fake_aiofd_enable_read_evt = TRUE;
    fake_socket_bound = TRUE;
    fake_socket_bind = TRUE;
    MEMSET( &s, 0, sizeof( socket_t ) );
    CU_ASSERT_EQUAL( socket_bind( NULL, NULL, NULL, NULL ), SOCKET_BADPARAM );
    fake_socket_bound_ret = TRUE;
    CU_ASSERT_EQUAL( socket_bind( &s, NULL, NULL, NULL ), SOCKET_BOUND );
    fake_socket_bound_ret = FALSE;
    s.type = SOCKET_LAST;
    CU_ASSERT_EQUAL( socket_bind( &s, "foo.com", "0", el ), SOCKET_ERROR );
    s.type = SOCKET_UNKNOWN;
    CU_ASSERT_EQUAL( socket_bind( &s, "foo.com", NULL, el ), SOCKET_ERROR );
    s.type = SOCKET_TCP;
    s.host = STRDUP("foo.com");
    CU_ASSERT_EQUAL( socket_bind( &s, NULL, NULL, el ), SOCKET_INVALIDPORT );
    CU_ASSERT_EQUAL( socket_bind( &s, NULL, "12354", el ), SOCKET_OPEN_FAIL );
    s.type = SOCKET_TCP;
    fake_socket_bind_ret = FALSE;
    fake_setsockopt_ret = 0;
    CU_ASSERT_EQUAL( socket_bind( &s, NULL, NULL, el ), SOCKET_ERROR );
    fake_setsockopt_ret = 1;
    CU_ASSERT_EQUAL( socket_bind( &s, NULL, NULL, el ), SOCKET_ERROR );
    fake_socket_bind_ret = TRUE;
    fake_aiofd_enable_read_evt_ret = FALSE;
    CU_ASSERT_EQUAL( socket_bind( &s, NULL, NULL, el ), SOCKET_ERROR );
    fake_aiofd_enable_read_evt_ret = TRUE;
    CU_ASSERT_EQUAL( socket_bind( &s, NULL, NULL, el ), SOCKET_ERROR );
    s.type = SOCKET_UNIX;
    fake_socket_bind_ret = FALSE;
    CU_ASSERT_EQUAL( socket_bind( &s, NULL, NULL, el ), SOCKET_OPEN_FAIL );
    fake_socket_bind_ret = TRUE;
    fake_aiofd_enable_read_evt_ret = FALSE;
    CU_ASSERT_EQUAL( socket_bind( &s, NULL, NULL, el ), SOCKET_OPEN_FAIL );
    fake_aiofd_enable_read_evt_ret = TRUE;
    CU_ASSERT_EQUAL( socket_bind( &s, NULL, NULL, el ), SOCKET_OPEN_FAIL );
    FREE( s.port );
    s.port = NULL;
#endif
    reset_test_flags();

    /* test_socket_listen */
    MEMSET( &s, 0, sizeof( socket_t ) );
    CU_ASSERT_EQUAL( socket_listen( NULL, 0 ), SOCKET_BADPARAM );
    fake_listen = TRUE;
    fake_listen_ret = -1;
    fake_socket_bound = TRUE;
    fake_socket_bound_ret = FALSE;
    CU_ASSERT_EQUAL( socket_listen( &s, 0 ), SOCKET_BOUND );
    fake_socket_bound_ret = TRUE;
    fake_socket_connected = TRUE;
    fake_socket_connected_ret = TRUE;
    CU_ASSERT_EQUAL( socket_listen( &s, 0 ), SOCKET_CONNECTED );
    fake_socket_bound_ret = FALSE;
    CU_ASSERT_EQUAL( socket_listen( &s, 0 ), SOCKET_BOUND );
    fake_socket_bound_ret = TRUE;
    fake_socket_connected_ret = FALSE;
    fake_listen_ret = 0;
    CU_ASSERT_EQUAL( socket_listen( &s, 0 ), SOCKET_ERROR );
    fake_listen_ret = -1;
    CU_ASSERT_EQUAL( socket_listen( &s, 0 ), SOCKET_ERROR );

    reset_test_flags();

    /* test socket_read */
    fake_aiofd_read = TRUE;
    MEMSET( &s, 0, sizeof(socket_t) );
    CU_ASSERT_EQUAL( socket_read( NULL, NULL, 0 ), -1 );
    CU_ASSERT_EQUAL( socket_read( &s, NULL, 0 ), -1 );
    CU_ASSERT_EQUAL( socket_read( &s, buf, 0 ), -1 );
    fake_aiofd_read_ret = -1;
    CU_ASSERT_EQUAL( socket_read( &s, buf, 64 ), -1 );
    fake_aiofd_read_ret = 64;
    CU_ASSERT_EQUAL( socket_read( &s, buf, 64 ), 64 );
    fake_aiofd_read = FALSE;

    reset_test_flags();

    /* test socket_write */
    fake_aiofd_write = TRUE;
    MEMSET( &s, 0, sizeof(socket_t) );
    CU_ASSERT_EQUAL( socket_write( NULL, NULL, 0 ), SOCKET_BADPARAM );
    fake_aiofd_write_ret = FALSE;
    CU_ASSERT_EQUAL( socket_write( &s, NULL, 0 ), SOCKET_ERROR );
    fake_aiofd_write_ret = TRUE;
    CU_ASSERT_EQUAL( socket_write( &s, NULL, 0 ), SOCKET_OK );
    fake_aiofd_write = FALSE;

    reset_test_flags();

    /* test socket_writev */
    fake_aiofd_writev = TRUE;
    MEMSET( &s, 0, sizeof(socket_t) );
    CU_ASSERT_EQUAL( socket_writev( NULL, NULL, 0 ), SOCKET_BADPARAM );
    fake_aiofd_writev_ret = FALSE;
    CU_ASSERT_EQUAL( socket_writev( &s, NULL, 0 ), SOCKET_ERROR );
    fake_aiofd_writev_ret = TRUE;
    CU_ASSERT_EQUAL( socket_writev( &s, NULL, 0 ), SOCKET_OK );
    fake_aiofd_writev = FALSE;

    reset_test_flags();

    /* test socket_accept */
    fake_accept = TRUE;
    fake_socket_bound = TRUE;
    fake_setsockopt = TRUE;
    fake_fcntl = TRUE;
    fake_aiofd_initialize = TRUE;
    fake_aiofd_enable_read_evt = TRUE;
    MEMSET( &s, 0, sizeof(socket_t) );
    MEMSET( &ops, 0, sizeof(socket_ops_t) );
    CU_ASSERT_PTR_NULL( socket_accept( NULL, NULL, NULL, NULL ) );
    CU_ASSERT_PTR_NULL( socket_accept( &s, NULL, NULL, NULL ) );
    CU_ASSERT_PTR_NULL( socket_accept( &s, &ops, NULL, NULL ) );
    CU_ASSERT_PTR_NULL( socket_accept( &s, &ops, el, NULL ) );
    fake_socket_bound_ret = FALSE;
    CU_ASSERT_PTR_NULL( socket_accept( &s, &ops, el, NULL ) );
    fake_socket_bound_ret = TRUE;
    s.type = SOCKET_LAST;
    CU_ASSERT_PTR_NULL( socket_accept( &s, &ops, el, NULL ) );
    s.type = SOCKET_UNKNOWN;
    CU_ASSERT_PTR_NULL( socket_accept( &s, &ops, el, NULL ) );
    s.type = SOCKET_TCP;
    fail_alloc = TRUE;
    CU_ASSERT_PTR_NULL( socket_accept( &s, &ops, el, NULL ) );
    fail_alloc = FALSE;
    s.type = SOCKET_UNIX;
    s.host = host;
    fake_aiofd_initialize_ret = FALSE;
    fake_fcntl_ret = -1;
    fake_accept_ret = -1;
    CU_ASSERT_PTR_NULL( socket_accept( &s, &ops, el, NULL ) );
    fake_accept_ret = 0;
    CU_ASSERT_PTR_NULL( socket_accept( &s, &ops, el, NULL ) );
    s.type = SOCKET_TCP;
    fake_accept_ret = -1;
    CU_ASSERT_PTR_NULL( socket_accept( &s, &ops, el, NULL ) );
    fake_accept_ret = 0;
    fake_setsockopt_ret = -1;
    fake_fcntl_ret = -1;
    fake_aiofd_initialize_ret = FALSE;
    CU_ASSERT_PTR_NULL( socket_accept( &s, &ops, el, NULL ) );
    fake_fcntl_ret = 0;
    fake_aiofd_initialize_ret = FALSE;
    CU_ASSERT_PTR_NULL( socket_accept( &s, &ops, el, NULL ) );
    fake_setsockopt_ret = 0;
    fake_fcntl_ret = -1;
    fake_aiofd_initialize_ret = FALSE;
    CU_ASSERT_PTR_NULL( socket_accept( &s, &ops, el, NULL ) );
    fake_fcntl_ret = 0;
    fake_aiofd_initialize_ret = FALSE;
    CU_ASSERT_PTR_NULL( socket_accept( &s, &ops, el, NULL ) );
    fake_aiofd_initialize_ret = TRUE;
    fake_aiofd_enable_read_evt_ret = FALSE;
    CU_ASSERT_PTR_NULL( socket_accept( &s, &ops, el, NULL ) );
    test_flag = FALSE;
    ops.connect_fn = &test_connect_fn;
    CU_ASSERT_PTR_NULL( socket_accept( &s, &ops, el, &test_flag ) );
    CU_ASSERT_TRUE( test_flag );
    MEMSET( &ops, 0, sizeof(socket_ops_t) );
    fake_aiofd_enable_read_evt_ret = TRUE;
    p = socket_accept( &s, &ops, el, NULL );
    CU_ASSERT_PTR_NOT_NULL( p );
    FREE( p );

    reset_test_flags();
    FREE( host );
}

#endif


