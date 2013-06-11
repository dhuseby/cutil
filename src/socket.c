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

struct socket_s
{
    socket_type_t   type;           /* type of socket */
    int32_t         connected;      /* is the socket connected? */
    int32_t         bound;          /* is the socket bound? */
    int8_t*         host;           /* host name */
    uint16_t        port;           /* port number */
    IPv4            addr;           /* IPv4 struct from host string */
    void *          user_data;      /* passed to ops callbacks */
    socket_ops_t    ops;            /* socket callbacks */
    aiofd_t         aiofd;          /* the fd management state */
};

static int socket_get_error( socket_t * const s, int * errval )
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

static int socket_do_tcp_bind( socket_t * const s )
{
    int len = 0;
    struct sockaddr_in in_addr;

    UNIT_TEST_RET( socket_bind );

    CHECK_PTR_RET( s, FALSE );
    CHECK_RET( s->type == SOCKET_TCP, FALSE );
    CHECK_RET( s->port > 0, FALSE );

    /* initialize the socket address struct */
    MEMSET( &in_addr, 0, sizeof(struct sockaddr_in) );
    in_addr.sin_family = AF_INET;
    in_addr.sin_addr.s_addr = s->addr.s_addr;
    in_addr.sin_port = htons(s->port);

    len = sizeof( in_addr );

    if ( BIND( s->aiofd.rfd, (struct sockaddr*)&in_addr, len) < 0 )
    {
        DEBUG( "failed to bind (errno: %d)\n", errno );
        return FALSE;
    }

    return TRUE;
}

static int socket_do_unix_bind( socket_t * const s )
{
    int len = 0;
    struct sockaddr_un un_addr;

    UNIT_TEST_RET( socket_bind );

    CHECK_PTR_RET( s, FALSE );
    CHECK_RET( s->type == SOCKET_UNIX, FALSE );
    CHECK_PTR_RET( s->host, FALSE );

    /* initialize the socket address struct */
    MEMSET( &un_addr, 0, sizeof(struct sockaddr_un) );
    un_addr.sun_family = AF_UNIX;
    strncpy( un_addr.sun_path, s->host, 100 );

    /* try to delete the socket inode */
    unlink( un_addr.sun_path );

    /* calculate the length of the address struct */
    len = strlen( un_addr.sun_path ) + sizeof( un_addr.sun_family );

    if ( BIND( s->aiofd.rfd, (struct sockaddr*)&un_addr, len) < 0 )
    {
        DEBUG( "failed to bind (errno: %d)\n", errno );
        return FALSE;
    }

    return TRUE;
}


static int socket_do_tcp_connect( socket_t * const s )
{
    int len = 0;
    struct sockaddr_in in_addr;

    UNIT_TEST_RET( socket_connect );
    
    CHECK_PTR_RET( s, FALSE );
    CHECK_RET( s->type == SOCKET_TCP, FALSE );
    CHECK_RET( s->port > 0, FALSE );

    /* initialize the socket address struct */
    MEMSET( &in_addr, 0, sizeof(struct sockaddr_in) );
    in_addr.sin_family = AF_INET;
    in_addr.sin_addr.s_addr = s->addr.s_addr;
    in_addr.sin_port = htons(s->port);
    
    len = sizeof( in_addr );

    /* try to make the connection */
    if ( CONNECT(s->aiofd.rfd, (struct sockaddr*)&in_addr, len) < 0 )
    {
        if ( ERRNO == EINPROGRESS )
        {
            DEBUG( "connection in progress\n" );
        }
        else
        {
            DEBUG("failed to initiate connect to the server\n");
            return FALSE;
        }
    }

    return TRUE;
}

static int socket_do_unix_connect( socket_t * const s )
{
    int len = 0;
    struct sockaddr_un un_addr;

    UNIT_TEST_RET( socket_connect );

    CHECK_PTR_RET( s, FALSE );
    CHECK_RET( s->type == SOCKET_UNIX, FALSE );
    CHECK_PTR_RET( s->host, FALSE );

    /* initialize the socket address struct */
    MEMSET( &un_addr, 0, sizeof(struct sockaddr_un) );
    un_addr.sun_family = AF_UNIX;
    strncpy( un_addr.sun_path, s->host, 100 );

    /* calculate the length of the address struct */
    len = strlen( un_addr.sun_path ) + sizeof( un_addr.sun_family );

    /* try to make the connection */
    if ( CONNECT(s->aiofd.rfd, (struct sockaddr*)&un_addr, len) < 0 )
    {
        if ( ERRNO == EINPROGRESS )
        {
            DEBUG( "connection in progress\n" );
        }
        else
        {
            DEBUG("failed to initiate connect to the server\n");
            return FALSE;
        }
    }

    return TRUE;
}

static socket_ret_t socket_lookup_host( socket_t * const s, 
                                        int8_t const * const hostname)
{
    struct hostent *host;

    UNIT_TEST_RET( socket_lookup_host );
    
    CHECK_PTR_RET(s, SOCKET_BADPARAM);
    CHECK_RET((hostname != NULL) || (s->host != NULL), SOCKET_BADHOSTNAME);
    
    /* store the hostname if needed */
    if( hostname != NULL )
    {
        /* free the existing host if any */
        FREE(s->host);
        
        /* copy the hostname into the server struct */
        s->host = T(strdup((char const * const)hostname));
    }

    if ( s->type != SOCKET_UNIX )
    {
        /* try to look up the host name */
        if( (host = gethostbyname((char const * const)hostname)) == NULL )
        {
            DEBUG("look up of %s failed\n", hostname);
            return SOCKET_BADHOSTNAME;
        }
        
        /* store the IP address */
        s->addr.s_addr = *((uint32_t*)host->h_addr_list[0]);
    }
    
    return SOCKET_OK;
}


static int socket_aiofd_write_fn( aiofd_t * const aiofd,
                                  uint8_t const * const buffer,
                                  void * user_data )
{
    int errval = 0;
    socket_t * s = (socket_t*)user_data;
    CHECK_PTR_RET( aiofd, FALSE );
    CHECK_PTR_RET( s, FALSE );

    if ( s->connected )
    {
        if ( buffer == NULL )
        {
            if ( list_count( &(s->aiofd.wbuf) ) == 0 )
            {
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
            DEBUG( "failed to get socket option while checking connect\n" );
            if ( s->ops.error_fn != NULL )
            {
                DEBUG( "calling socket error callback\n" );
                (*(s->ops.error_fn))( s, errno, s->user_data );
            }
            
            /* stop write event processing */
            return FALSE;
        }

        if ( errval == 0 )
        {
            DEBUG( "socket connected\n" );
            s->connected = TRUE;

            if ( s->ops.connect_fn != NULL )
            {
                DEBUG( "calling socket connect callback\n" );
                /* call the connect callback */
                (*(s->ops.connect_fn))( s, s->user_data );
            }

            /* we're connected to start read event */
            /* TODO: check for error and handle it properly */
            aiofd_enable_read_evt( &(s->aiofd), TRUE );

            if ( list_count( &(s->aiofd.wbuf) ) == 0 )
            {
                /* stop the write event processing until we have data to write */
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
                DEBUG( "calling socket error callback\n" );
                (*(s->ops.error_fn))( s, errno, s->user_data );
            }

            /* stop write event processing */
            return FALSE;
        }
    }

    return TRUE;
}


static int socket_aiofd_error_fn( aiofd_t * const aiofd,
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


static int socket_aiofd_read_fn( aiofd_t * const aiofd,
                                 size_t const nread,
                                 void * user_data )
{
    socket_t * s = (socket_t*)user_data;
    CHECK_PTR_RET( aiofd, FALSE );
    CHECK_PTR_RET( s, FALSE );

    if ( socket_is_bound( s ) )
    {
        /* this is a socket accepting incoming connections */
        if ( s->ops.connect_fn != NULL )
        {
            DEBUG( "calling socket connect callback for incoming connection\n" );
            if ( (*(s->ops.connect_fn))( s, s->user_data ) != SOCKET_OK )
            {
                DEBUG( "failed to accept incoming connection!\n" );
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
            return FALSE;
        }

        /* this is a normal connection socket, so pass the read event along */
        if ( s->ops.read_fn != NULL )
        {
            DEBUG( "calling socket read callback\n");
            (*(s->ops.read_fn))( s, nread, s->user_data );
        }
    }

    return TRUE;
}

static int socket_initialize( socket_t * const s,
                        socket_type_t const type, 
                        socket_ops_t * const ops,
                        evt_loop_t * const el,
                        void * user_data )
{
    int fd;
    int32_t flags;
    static aiofd_ops_t aiofd_ops =
    {
        &socket_aiofd_read_fn,
        &socket_aiofd_write_fn,
        &socket_aiofd_error_fn
    };

    UNIT_TEST_FAIL( socket_initialize );

    CHECK_PTR_RET( s, FALSE );
    CHECK_RET( VALID_SOCKET_TYPE( type ), FALSE );
    CHECK_PTR_RET( ops, FALSE );
    CHECK_PTR_RET( el, FALSE );

    MEMSET( (void*)s, 0, sizeof(socket_t) );

    /* store the type */
    s->type = type;

    /* store the user_data pointer */
    s->user_data = user_data;

    /* copy the ops into place */
    MEMCPY( (void*)&(s->ops), ops, sizeof(socket_ops_t) );

    /* do the connect based on the type */
    switch(s->type)
    {
        case SOCKET_TCP:

            /* try to open a socket */
            if ( (fd = SOCKET( PF_INET, SOCK_STREAM, IPPROTO_TCP )) < 0 )
            {
                DEBUG("failed to open socket\n");
                return FALSE;
            }
            else
            {
                DEBUG("created socket\n");
            }

            /* turn off TCP naggle algorithm */
            flags = 1;
            if ( SETSOCKOPT( fd, IPPROTO_TCP, TCP_NODELAY, (char*)&flags, sizeof(flags) ) )
            {
                DEBUG( "failed to turn on TCP no delay\n" );
                return FALSE;
            }
            else
            {
                DEBUG("turned on TCP no delay\n");
            }

            break;
    
        case SOCKET_UNIX:

            /* try to open a socket */
            if ( (fd = SOCKET( PF_UNIX, SOCK_STREAM, 0 )) < 0 )
            {
                DEBUG("failed to open socket\n");
                return FALSE;
            }
            else
            {
                DEBUG("created socket\n");
            }

            break;
    }

    /* set the socket to non blocking mode */
    flags = FCNTL( fd, F_GETFL );
    if( FCNTL( fd, F_SETFL, (flags | O_NONBLOCK) ) < 0 )
    {
        DEBUG("failed to set socket to non-blocking\n");
    }
    else
    {
        DEBUG("socket is now non-blocking\n");
    }

    /* initialize the aiofd to manage the socket */
    if ( aiofd_initialize( &(s->aiofd), fd, fd, &aiofd_ops, el, (void*)s ) == FALSE )
    {
        DEBUG( "failed to initialzie the aiofd\n" );
        return FALSE;
    }

    return TRUE;
}

socket_t* socket_new( socket_type_t const type, 
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
    if ( socket_initialize( s, type, ops, el, user_data ) == FALSE )
    {
        socket_delete( s );
        return NULL;
    }

    return s;
}

static void socket_deinitialize( socket_t * const s )
{
    /* close the socket */
    if ( s->aiofd.rfd >= 0 )
        socket_disconnect( s );

    /* clean up the host name */
    if ( s->host != NULL )
        FREE ( s->host );
}

void socket_delete( void * s )
{
    socket_t * sock = (socket_t*)s;
    CHECK_PTR( s );

    socket_deinitialize( sock );

    FREE( sock );
}

/* check to see if connected */
int socket_is_connected( socket_t* const s )
{
    UNIT_TEST_RET( socket_connected );

    CHECK_PTR_RET( s, FALSE );
    CHECK_RET( s->aiofd.rfd >= 0, FALSE );
    return s->connected;
}

socket_ret_t socket_connect( socket_t* const s, 
                             int8_t const * const hostname, 
                             uint16_t const port )
{
    int len = 0;
    socket_ret_t ret = SOCKET_OK;
    struct sockaddr_in in_addr;
    struct sockaddr_un un_addr;
    
    CHECK_PTR_RET(s, SOCKET_BADPARAM);
    CHECK_RET((hostname != NULL) || (s->host != NULL), SOCKET_BADHOSTNAME);
    CHECK_RET( VALID_SOCKET_TYPE( s->type ), SOCKET_ERROR );
    CHECK_RET(((s->type != SOCKET_UNIX) && ((port > 0) || (s->port > 0))) || (s->type == SOCKET_UNIX), SOCKET_INVALIDPORT);
    CHECK_RET( !socket_is_connected( s ), SOCKET_CONNECTED );
    
    /* store port number if provided */
    if( port > 0 )
        s->port = port;
    
    /* look up and store the hostname if provided */
    if( hostname != NULL )
    {
        if ( (ret = socket_lookup_host(s, hostname)) != SOCKET_OK )
            return ret;
    }

    /* do the connect based on the type */
    switch(s->type)
    {
        case SOCKET_TCP:

            /* start the socket write event processing so we can catch connection */
            aiofd_enable_write_evt( &(s->aiofd), TRUE );

            /* try to make the TCP connection */
            CHECK_RET( socket_do_tcp_connect( s ), SOCKET_CONNECT_FAIL );

            break;
        
        case SOCKET_UNIX:
            
            /* start the socket write event processing so we can catch connection */
            aiofd_enable_write_evt( &(s->aiofd), TRUE );

            /* try to make the UNIX connection */
            CHECK_RET( socket_do_unix_connect( s ), SOCKET_CONNECT_FAIL );
        
            break;
    }

    return SOCKET_OK;
}


/* check to see if bound */
int socket_is_bound( socket_t* const s )
{
    UNIT_TEST_RET( socket_bound );

    CHECK_PTR_RET(s, FALSE);
    CHECK_RET( s->aiofd.rfd >= 0, FALSE );
    return s->bound;
}

socket_ret_t socket_bind( socket_t * const s,
                          int8_t const * const host,
                          uint16_t const port )
{
    int len;
    int on = 1;
    socket_ret_t ret;
    struct sockaddr_in in_addr;
    struct sockaddr_un un_addr;

    CHECK_PTR_RET( s, SOCKET_BADPARAM );
    CHECK_RET( !socket_is_bound( s ), SOCKET_BOUND );
    CHECK_RET( VALID_SOCKET_TYPE( s->type ), SOCKET_ERROR );

    /* store port number if provided */
    if( port > 0 )
        s->port = port;
    
    /* look up and store the hostname if provided */
    if( host != NULL )
    {
        if ( (ret = socket_lookup_host(s, host)) != SOCKET_OK )
            return ret;
    }

    switch ( s->type )
    {
        case SOCKET_TCP:
            if ( SETSOCKOPT( s->aiofd.rfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0 )
            {
                DEBUG( "failed to set the socket to reuse addr\n" );
            }

            /* try to bind the tcp socket */
            CHECK_RET( socket_do_tcp_bind( s ), SOCKET_ERROR );

            /* flag the socket as bound */
            s->bound = TRUE;

            break;

        case SOCKET_UNIX:
        
            /* try to bind the unix socket */
            CHECK_RET( socket_do_unix_bind( s ), SOCKET_ERROR );

            /* flag the socket as bound */
            s->bound = TRUE;

            break;
    }

    /* start the socket read even processing to catch incoming connections */
    CHECK_RET( aiofd_enable_read_evt( &(s->aiofd), TRUE ), SOCKET_ERROR );

    return SOCKET_OK;
}

socket_ret_t socket_listen( socket_t * const s, int const backlog )
{
    CHECK_PTR_RET( s, SOCKET_BADPARAM );
    CHECK_RET( socket_is_bound( s ), SOCKET_BOUND );
    CHECK_RET( !socket_is_connected( s ), SOCKET_CONNECTED );

    /* now begin listening for incoming connections */
    if ( LISTEN( s->aiofd.rfd, backlog ) < 0 )
    {
        DEBUG( "failed to listen (errno: %d)\n", errno );
        return SOCKET_ERROR;
    }

    /* set the listen flag so that it doesn't error on 0 size read callbacks */
    /* TODO: check for errors and handle them properly */
    aiofd_set_listen( &(s->aiofd), TRUE );

    return SOCKET_OK;
}

int socket_is_listening( socket_t * const s )
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
    int len;
    int32_t flags;
    socket_t * client;
    struct sockaddr_in in_addr;
    struct sockaddr_un un_addr;
    static aiofd_ops_t aiofd_ops =
    {
        &socket_aiofd_read_fn,
        &socket_aiofd_write_fn,
        &socket_aiofd_error_fn
    };

    CHECK_PTR_RET( s, NULL );
    CHECK_PTR_RET( ops, NULL );
    CHECK_PTR_RET( el, NULL );
    CHECK_RET( socket_is_bound( s ), NULL );
    CHECK_RET( VALID_SOCKET_TYPE( s->type ), NULL );

    client = CALLOC( 1, sizeof( socket_t ) );

    CHECK_PTR_RET_MSG( client, NULL, "failed to allocate new socket struct\n" );

    /* store the type */
    client->type = s->type;

    /* store the user_data pointer */
    client->user_data = user_data;

    /* copy the ops into place */
    MEMCPY( (void*)&(client->ops), ops, sizeof(socket_ops_t) );

    /* do the connect based on the type */
    switch(s->type)
    {
        case SOCKET_TCP:

            MEMSET( (void*)&in_addr, 0, sizeof( struct sockaddr_in ) );

            len = sizeof(in_addr);

            /* try to open a socket */
            if ( (fd = ACCEPT( s->aiofd.rfd, (struct sockaddr *)&in_addr, &len )) < 0 )
            {
                DEBUG("failed to accept incoming connection\n");
                socket_delete( (void*)client );
                return NULL;
            }
            else
            {
                DEBUG("accepted socket\n");
            }

            /* store the connection information */
            client->addr.s_addr = in_addr.sin_addr.s_addr;
            client->port = ntohs( in_addr.sin_port );

            /* turn off TCP naggle algorithm */
            flags = 1;
            if ( SETSOCKOPT( fd, IPPROTO_TCP, TCP_NODELAY, (char*)&flags, sizeof(flags) ) < 0 )
            {
                DEBUG( "failed to turn on TCP no delay\n" );
            }
            else
            {
                DEBUG("turned on TCP no delay\n");
            }

            break;
    
        case SOCKET_UNIX:

            MEMSET( (void*)&un_addr, 0, sizeof( struct sockaddr_un ) );

            len = sizeof(un_addr);

            /* try to open a socket */
            if ( (fd = ACCEPT( s->aiofd.rfd, (struct sockaddr *)&un_addr, &len )) < 0 )
            {
                DEBUG("failed to open socket\n");
                socket_delete( (void*)client );
                return NULL;
            }
            else
            {
                DEBUG("accepted socket\n");
            }

            /* store the connection information */
            client->host = T(strdup((char const * const)un_addr.sun_path));

            break;
    }

    /* set the socket to non blocking mode */
    flags = FCNTL( fd, F_GETFL );
    if( FCNTL( fd, F_SETFL, (flags | O_NONBLOCK) ) < 0 )
    {
        DEBUG("failed to set socket to non-blocking\n");
    }
    else
    {
        DEBUG("socket is now non-blocking\n");
    }

    /* initialize the aiofd to manage the socket */
    if ( !aiofd_initialize( &(client->aiofd), fd, fd, &aiofd_ops, el, (void*)client ) )
    {
        DEBUG("failed to initialize the aiofd for the socket\n");
        socket_delete( (void*)client );
        return NULL;
    }
    else
    {
        DEBUG("aiofd initialized\n");
    }
    
    DEBUG( "socket connected\n" );
    client->connected = TRUE;

    if ( client->ops.connect_fn != NULL )
    {
        DEBUG( "calling socket connect callback\n" );
        /* call the connect callback */
        (*(client->ops.connect_fn))( client, client->user_data );
    }

    /* we're connected to start read event */
    if ( !aiofd_enable_read_evt( &(client->aiofd), TRUE ) )
    {
        DEBUG( "failed to enable read event\n" );
        socket_delete( (void*)client );
        return NULL;
    }

    return client;
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

    if ( s->ops.disconnect_fn != NULL )
    {
        /* call the disconnect callback */
        (*(s->ops.disconnect_fn))( s, s->user_data );
    }

    return SOCKET_OK;
}

socket_type_t socket_get_type( socket_t * const s )
{
    CHECK_PTR_RET( s, SOCKET_UNKNOWN );
    return s->type;
}

int32_t socket_read( socket_t* const s, 
                     uint8_t * const buffer, 
                     int32_t const n )
{
    CHECK_PTR_RET(s, 0);
    CHECK_PTR_RET(buffer, 0);
    CHECK_RET(n > 0, 0);

    return aiofd_read( &(s->aiofd), buffer, n );
}

socket_ret_t socket_write( socket_t * const s,
                           uint8_t const * const buffer,
                           size_t const n )
{
    CHECK_PTR_RET( s, SOCKET_BADPARAM );
    return ( aiofd_write( &(s->aiofd), buffer, n ) ? SOCKET_OK : SOCKET_ERROR );
}

socket_ret_t socket_writev( socket_t * const s,
                            struct iovec * iov,
                            size_t iovcnt )
{
    CHECK_PTR_RET( s, SOCKET_BADPARAM );
    return ( aiofd_writev( &(s->aiofd), iov, iovcnt ) ? SOCKET_OK : SOCKET_ERROR );
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
    *((int*)user_data) = TRUE;
    return test_error_fn_ret;
}

static socket_ret_t test_connect_fn( socket_t * const s, void * user_data )
{
    *((int*)user_data) = TRUE;
    return test_connect_fn_ret;
}

void test_socket_private_functions( void )
{
    int test_flag;
    int8_t * const host = T( strdup( "foo.com") );
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

    /* test socket_do_tcp_connect */
    fake_connect = TRUE;
    fake_connect_ret = 0;
    CU_ASSERT_FALSE( socket_do_tcp_connect( NULL ) );
    s.type = SOCKET_UNIX;
    CU_ASSERT_FALSE( socket_do_tcp_connect( &s ) );
    s.type = SOCKET_TCP;
    CU_ASSERT_FALSE( socket_do_tcp_connect( &s ) );
    s.port = 1024;
    CU_ASSERT_TRUE( socket_do_tcp_connect( &s ) );
    fake_connect_ret = -1;
    CU_ASSERT_FALSE( socket_do_tcp_connect( &s ) );
    fake_errno = TRUE;
    fake_errno_value = EINPROGRESS;
    CU_ASSERT_TRUE( socket_do_tcp_connect( &s ) );
    fake_errno_value = EACCES;
    CU_ASSERT_FALSE( socket_do_tcp_connect( &s ) );

    reset_test_flags();

    /* test socket_do_unix_connect */
    fake_connect = TRUE;
    fake_connect_ret = 0;
    CU_ASSERT_FALSE( socket_do_unix_connect( NULL ) );
    s.type = SOCKET_TCP;
    CU_ASSERT_FALSE( socket_do_unix_connect( &s ) );
    s.type = SOCKET_UNIX;
    CU_ASSERT_FALSE( socket_do_unix_connect( &s ) );
    s.host = host;
    CU_ASSERT_TRUE( socket_do_unix_connect( &s ) );
    fake_connect_ret = -1;
    CU_ASSERT_FALSE( socket_do_unix_connect( &s ) );
    fake_errno = TRUE;
    fake_errno_value = EINPROGRESS;
    CU_ASSERT_TRUE( socket_do_unix_connect( &s ) );
    fake_errno_value = EACCES;
    CU_ASSERT_FALSE( socket_do_unix_connect( &s ) );
    
    reset_test_flags();

    /* test socket_lookup_host */
    MEMSET( &s, 0, sizeof( socket_t ) );
    CU_ASSERT_EQUAL( socket_lookup_host( NULL, NULL ), SOCKET_BADPARAM );
    CU_ASSERT_EQUAL( socket_lookup_host( &s, NULL ), SOCKET_BADHOSTNAME );
    CU_ASSERT_EQUAL( socket_lookup_host( &s, "foo.com" ), SOCKET_OK ); /* strdup's the hostname */
    FREE( s.host );
    s.type = SOCKET_UNIX;
    s.host = host;
    CU_ASSERT_EQUAL( socket_lookup_host( &s, NULL ), SOCKET_OK );
    
    reset_test_flags();

    /* test socket_aiofd_write_fn */
    CU_ASSERT_FALSE( socket_aiofd_write_fn( NULL, NULL, NULL ) );
    CU_ASSERT_FALSE( socket_aiofd_write_fn( &(s.aiofd), NULL, NULL ) );
    s.connected = TRUE;
    CU_ASSERT_TRUE( list_initialize( &(s.aiofd.wbuf), 2, NULL ) );
    CU_ASSERT_TRUE( list_push_tail( &(s.aiofd.wbuf), (void*)host ) );
    CU_ASSERT_TRUE( socket_aiofd_write_fn( &(s.aiofd), NULL, (void*)(&s) ) );
    CU_ASSERT_TRUE( socket_aiofd_write_fn( &(s.aiofd), (uint8_t const * const)&host, (void*)(&s) ) );
    s.connected = FALSE;
    s.aiofd.rfd = STDIN_FILENO;
    CU_ASSERT_FALSE( socket_aiofd_write_fn( &(s.aiofd), (uint8_t const * const)&host, (void*)(&s) ) );
    s.ops.error_fn = &test_error_fn;
    test_flag = FALSE;
    s.user_data = &test_flag;
    CU_ASSERT_FALSE( socket_aiofd_write_fn( &(s.aiofd), (uint8_t const * const)&host, (void*)(&s) ) );
    CU_ASSERT_TRUE( test_flag );
    fake_socket_getsockopt = TRUE;
    fake_socket_errval = 0;
    fake_socket_get_error_ret = TRUE;
    list_clear( &(s.aiofd.wbuf) );
    CU_ASSERT_FALSE( socket_aiofd_write_fn( &(s.aiofd), (uint8_t const * const)&host, (void*)(&s) ) );
    fake_socket_errval = -1;
    s.ops.error_fn = NULL;
    s.connected = FALSE;
    CU_ASSERT_FALSE( socket_aiofd_write_fn( &(s.aiofd), (uint8_t const * const)&host, (void*)(&s) ) );
    fake_socket_errval = 0;
    fake_socket_getsockopt = FALSE;

    reset_test_flags();

    /* test socket_aiofd_error_fn */
    CU_ASSERT_FALSE( socket_aiofd_error_fn( NULL, 0, NULL ) );
    CU_ASSERT_FALSE( socket_aiofd_error_fn( &(s.aiofd), 0, NULL ) );
    CU_ASSERT_TRUE( socket_aiofd_error_fn( &(s.aiofd), 0, (void*)(&s) ) );
    test_flag = FALSE;
    s.ops.error_fn = &test_error_fn;
    CU_ASSERT_TRUE( socket_aiofd_error_fn( &(s.aiofd), 0, (void*)(&s) ) );
    CU_ASSERT_TRUE( test_flag );

    reset_test_flags();

    /* test socket_aiofd_read_fn */
    CU_ASSERT_FALSE( socket_aiofd_read_fn( NULL, 0, NULL ) );
    CU_ASSERT_FALSE( socket_aiofd_read_fn( &(s.aiofd), 0, NULL ) );
    s.aiofd.rfd = 0;
    CU_ASSERT_FALSE( socket_aiofd_read_fn( &(s.aiofd), 0, (void*)(&s) ) );
    CU_ASSERT_TRUE( socket_aiofd_read_fn( &(s.aiofd), 1, (void*)(&s) ) );
    s.bound = TRUE;
    test_flag = FALSE;
    CU_ASSERT_TRUE( socket_aiofd_read_fn( &(s.aiofd), 0, (void*)(&s) ) );
    CU_ASSERT_FALSE( test_flag );
    s.ops.connect_fn = &test_connect_fn;
    test_connect_fn_ret = SOCKET_ERROR;
    test_flag = FALSE;
    CU_ASSERT_TRUE( socket_aiofd_read_fn( &(s.aiofd), 0, (void*)(&s) ) );
    CU_ASSERT_TRUE( test_flag );

    reset_test_flags();

    /* test socket_initialize */
    fake_socket = TRUE;
    fake_setsockopt = TRUE;
    fake_fcntl = TRUE;
    fake_aiofd_initialize = TRUE;
    MEMSET( &s, 0, sizeof( socket_t ) );
    MEMSET( &ops, 0, sizeof( socket_ops_t ) );
    CU_ASSERT_FALSE( socket_initialize( NULL, SOCKET_UNKNOWN, NULL, NULL, NULL ) );
    CU_ASSERT_FALSE( socket_initialize( &s, SOCKET_UNKNOWN, NULL, NULL, NULL ) );
    CU_ASSERT_FALSE( socket_initialize( &s, SOCKET_LAST, NULL, NULL, NULL ) );
    CU_ASSERT_FALSE( socket_initialize( &s, SOCKET_TCP, NULL, NULL, NULL ) );
    CU_ASSERT_FALSE( socket_initialize( &s, SOCKET_TCP, &ops, NULL, NULL ) );
    fake_socket_ret = -1;
    CU_ASSERT_FALSE( socket_initialize( &s, SOCKET_TCP, &ops, el, NULL ) );
    fake_socket_ret = 0;
    fake_setsockopt_ret = -1;
    CU_ASSERT_FALSE( socket_initialize( &s, SOCKET_TCP, &ops, el, NULL ) );
    fake_setsockopt_ret = 0;
    fake_fcntl_ret = -1;
    CU_ASSERT_FALSE( socket_initialize( &s, SOCKET_TCP, &ops, el, NULL ) );
    fake_fcntl_ret = 0;
    fake_aiofd_initialize_ret = FALSE;
    CU_ASSERT_FALSE( socket_initialize( &s, SOCKET_TCP, &ops, el, NULL ) );
    fake_aiofd_initialize_ret = TRUE;
    CU_ASSERT_TRUE( socket_initialize( &s, SOCKET_TCP, &ops, el, NULL ) );
    MEMSET( &s, 0, sizeof( socket_t ) );
    MEMSET( &ops, 0, sizeof( socket_ops_t ) );
    fake_socket_ret = -1;
    CU_ASSERT_FALSE( socket_initialize( &s, SOCKET_UNIX, &ops, el, NULL ) );
    fake_socket_ret = 0;
    CU_ASSERT_TRUE( socket_initialize( &s, SOCKET_UNIX, &ops, el, NULL ) );
    fake_socket = FALSE;
    fake_setsockopt = FALSE;
    fake_fcntl = FALSE;
    fake_aiofd_initialize = FALSE;
    
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
    MEMSET( &s, 0, sizeof(socket_t) );
    CU_ASSERT_EQUAL( socket_connect( NULL, NULL, 0 ), SOCKET_BADPARAM );
    CU_ASSERT_EQUAL( socket_connect( &s, NULL, 0 ), SOCKET_BADHOSTNAME );
    CU_ASSERT_EQUAL( socket_connect( &s, host, 0 ), SOCKET_INVALIDPORT );
    s.type = SOCKET_LAST;
    CU_ASSERT_EQUAL( socket_connect( &s, host, 0 ), SOCKET_ERROR );
    s.type = SOCKET_UNKNOWN;
    CU_ASSERT_EQUAL( socket_connect( &s, host, 0 ), SOCKET_ERROR );
    s.type = SOCKET_TCP;
    s.host = host;
    CU_ASSERT_EQUAL( socket_connect( &s, NULL, 0 ), SOCKET_INVALIDPORT );
    fake_socket_connected = TRUE;
    fake_socket_connected_ret = TRUE;
    CU_ASSERT_EQUAL( socket_connect( &s, NULL, 1024 ), SOCKET_CONNECTED );
    s.port = 1024;
    CU_ASSERT_EQUAL( socket_connect( &s, NULL, 0 ), SOCKET_CONNECTED );
    s.type = SOCKET_UNIX;
    CU_ASSERT_EQUAL( socket_connect( &s, NULL, 0 ), SOCKET_CONNECTED );
    s.type = SOCKET_TCP;
    fake_socket_connected_ret = FALSE;
    CU_ASSERT_EQUAL( socket_connect( &s, NULL, 0 ), SOCKET_CONNECT_FAIL );
    fake_socket_connect = TRUE;
    fake_socket_connect_ret = TRUE;
    CU_ASSERT_EQUAL( socket_connect( &s, NULL, 0 ), SOCKET_OK );
    fake_socket_connect_ret = FALSE;
    CU_ASSERT_EQUAL( socket_connect( &s, NULL, 0 ), SOCKET_CONNECT_FAIL );
    s.type = SOCKET_UNIX;
    fake_socket_connect_ret = TRUE;
    CU_ASSERT_EQUAL( socket_connect( &s, NULL, 0 ), SOCKET_OK );
    fake_socket_connect_ret = FALSE;
    CU_ASSERT_EQUAL( socket_connect( &s, NULL, 0 ), SOCKET_CONNECT_FAIL );
    
    reset_test_flags();

    /* test socket_bind */
    fake_setsockopt = TRUE;
    fake_aiofd_enable_read_evt = TRUE;
    fake_socket_bound = TRUE;
    fake_socket_lookup_host = TRUE;
    fake_socket_bind = TRUE;
    MEMSET( &s, 0, sizeof( socket_t ) );
    CU_ASSERT_EQUAL( socket_bind( NULL, NULL, 0 ), SOCKET_BADPARAM );
    fake_socket_bound_ret = TRUE;
    CU_ASSERT_EQUAL( socket_bind( &s, NULL, 0 ), SOCKET_BOUND );
    fake_socket_bound_ret = FALSE;
    s.type = SOCKET_LAST;
    CU_ASSERT_EQUAL( socket_bind( &s, host, 0 ), SOCKET_ERROR );
    s.type = SOCKET_UNKNOWN;
    CU_ASSERT_EQUAL( socket_bind( &s, host, 0 ), SOCKET_ERROR );
    s.type = SOCKET_TCP;
    fake_socket_lookup_host_ret = SOCKET_ERROR;
    CU_ASSERT_EQUAL( socket_bind( &s, host, 0 ), SOCKET_ERROR );
    CU_ASSERT_EQUAL( socket_bind( &s, NULL, 0 ), SOCKET_ERROR );
    s.type = SOCKET_TCP;
    fake_socket_bind_ret = FALSE;
    fake_setsockopt_ret = 0;
    CU_ASSERT_EQUAL( socket_bind( &s, NULL, 0 ), SOCKET_ERROR );
    fake_setsockopt_ret = 1;
    CU_ASSERT_EQUAL( socket_bind( &s, NULL, 0 ), SOCKET_ERROR );
    fake_socket_bind_ret = TRUE;
    fake_aiofd_enable_read_evt_ret = FALSE;
    CU_ASSERT_EQUAL( socket_bind( &s, NULL, 0 ), SOCKET_ERROR );
    fake_aiofd_enable_read_evt_ret = TRUE;
    CU_ASSERT_EQUAL( socket_bind( &s, NULL, 0 ), SOCKET_OK );
    s.type = SOCKET_UNIX;
    fake_socket_bind_ret = FALSE;
    CU_ASSERT_EQUAL( socket_bind( &s, NULL, 0 ), SOCKET_ERROR );
    fake_socket_bind_ret = TRUE;
    fake_aiofd_enable_read_evt_ret = FALSE;
    CU_ASSERT_EQUAL( socket_bind( &s, NULL, 0 ), SOCKET_ERROR );
    fake_aiofd_enable_read_evt_ret = TRUE;
    CU_ASSERT_EQUAL( socket_bind( &s, NULL, 0 ), SOCKET_OK );

    reset_test_flags();

    /* test socket_do_tcp_bind */
    MEMSET( &s, 0, sizeof(socket_t) );
    CU_ASSERT_FALSE( socket_do_tcp_bind( NULL ) );
    s.type = SOCKET_UNIX;
    CU_ASSERT_FALSE( socket_do_tcp_bind( &s ) );
    s.type = SOCKET_TCP;
    CU_ASSERT_FALSE( socket_do_tcp_bind( &s ) );
    s.port = 1024;
    fake_bind = TRUE;
    fake_bind_ret = 0;
    CU_ASSERT_TRUE( socket_do_tcp_bind( &s ) );
    fake_bind_ret = -1;
    CU_ASSERT_FALSE( socket_do_tcp_bind( &s ) );
    fake_bind = FALSE;
    fake_bind_ret = 0;

    reset_test_flags();

    /* test socket_do_unix_bind */
    MEMSET( &s, 0, sizeof(socket_t) );
    CU_ASSERT_FALSE( socket_do_unix_bind( NULL ) );
    s.type = SOCKET_TCP;
    CU_ASSERT_FALSE( socket_do_unix_bind( &s ) );
    s.type = SOCKET_UNIX;
    CU_ASSERT_FALSE( socket_do_unix_bind( &s ) );
    s.host = host;
    fake_bind = TRUE;
    fake_bind_ret = 0;
    CU_ASSERT_TRUE( socket_do_unix_bind( &s ) );
    fake_bind_ret = -1;
    CU_ASSERT_FALSE( socket_do_unix_bind( &s ) );
    fake_bind = FALSE;
    fake_bind_ret = 0;

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
    CU_ASSERT_EQUAL( socket_listen( &s, 0 ), SOCKET_OK );
    fake_listen_ret = -1;
    CU_ASSERT_EQUAL( socket_listen( &s, 0 ), SOCKET_ERROR );

    reset_test_flags();

    /* test socket_read */
    fake_aiofd_read = TRUE;
    MEMSET( &s, 0, sizeof(socket_t) );
    CU_ASSERT_EQUAL( socket_read( NULL, NULL, 0 ), 0 );
    CU_ASSERT_EQUAL( socket_read( &s, NULL, 0 ), 0 );
    CU_ASSERT_EQUAL( socket_read( &s, buf, 0 ), 0 );
    fake_aiofd_read_ret = 0;
    CU_ASSERT_EQUAL( socket_read( &s, buf, 64 ), 0 );
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


