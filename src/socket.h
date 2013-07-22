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

#ifndef __SOCKET_H__
#define __SOCKET_H__

#include "aiofd.h"
#include "events.h"
#include "list.h"

typedef enum socket_ret_e
{
    /* non-error return value */
    SOCKET_OK           = 1,
    SOCKET_INPUT        = 2,
    SOCKET_OUTPUT       = 3,
    
    /* errors */
    SOCKET_ERROR        = -1,
    SOCKET_BADPARAM     = -2,
    SOCKET_BADHOSTNAME  = -3,
    SOCKET_INVALIDPORT  = -4,
    SOCKET_TIMEOUT      = -5,
    SOCKET_POLLERR      = -6,
    SOCKET_CONNECTED    = -7,
    SOCKET_BOUND        = -8,
    SOCKET_OPEN_FAIL    = -9,
    SOCKET_CONNECT_FAIL = -10,
    SOCKET_BIND_FAIL    = -11,
    SOCKET_OPENED       = -12

} socket_ret_t;

typedef enum socket_type_e
{
    SOCKET_TCP,
    SOCKET_UDP,
    SOCKET_UNIX,

    SOCKET_LAST,
    SOCKET_FIRST = SOCKET_TCP,
    SOCKET_COUNT = (SOCKET_LAST - SOCKET_FIRST),

    SOCKET_UNKNOWN = -1
    
} socket_type_t;

#define VALID_SOCKET_TYPE( t ) ((t >= SOCKET_FIRST) && (t < SOCKET_LAST))
#define HOSTNAME_BUFFER_LEN (128)
#define PORT_BUFFER_LEN (8)

typedef struct socket_s socket_t;
typedef struct sockaddr_storage sockaddr_t;

/* NOTE: The connect_fn callback is used for connection based sockets (e.g.
 * TCP and Unix sockets).  For sockets that are bound and listening, it gets 
 * called for each new incoming connection.  The new socket that is created 
 * from the accept call also receives a connect_fn callback.  Sockets that are 
 * connecting, the connect_fn callback gets called when the connect is complete.
 *
 * For UDP sockets, the connect_fn callback never gets called.
 *
 * Same goes with the disconnect_fn callback, UDP sockets never receive them.
 */

typedef struct socket_ops_s 
{
    socket_ret_t (*connect_fn)( socket_t * const s, void * user_data );
    socket_ret_t (*disconnect_fn)( socket_t * const s, void * user_data );
    socket_ret_t (*error_fn)( socket_t * const s, int err, void * user_data );
    ssize_t (*read_fn)( socket_t * const s, size_t const nread, void * user_data );
    ssize_t (*write_fn)( socket_t * const s, uint8_t const * const buffer, void * user_data );

} socket_ops_t;

/* create/destroy a socket */
/* NOTE: ai_flags and ai_family have the same meaning as ai_flags
 * and ai_family in the struct addrinfo used by getaddrinfo() */
socket_t* socket_new( socket_type_t const type,
                      uint8_t const * const host, 
                      uint8_t const * const port,
                      int const ai_flags,
                      int const ai_family,
                      socket_ops_t * const ops,
                      evt_loop_t * const el,
                      void * user_data );

void socket_delete( void * s );

/* used to convert sockaddr_t to the correct sockaddr_in struct */
void * socket_in_addr( sockaddr_t * const addr );

/* used to get the string representation of an addr. */
int_t socket_get_addr_string( sockaddr_t const * const addr,
                              uint8_t * const buf, 
                              size_t const len );

/* used to get the addr and addrlen for the socket */
int_t socket_get_addr( socket_t const * const s,
                       sockaddr_t * const addr,
                       socklen_t * const len );

/* check to see if connected */
int_t socket_is_connected( socket_t const * const s );

/* connect a socket as a client*/
socket_ret_t socket_connect( socket_t * const s );

/* check to see if bound */
int_t socket_is_bound( socket_t const * const s );

/* bind a socket to a specified IP/port or inode */
socket_ret_t socket_bind( socket_t * const s );

/* check to see if listening */
int_t socket_is_listening( socket_t const * const s );

/* listen for incoming connections */
socket_ret_t socket_listen( socket_t * const s,
                            int const backlog );

/* accept an incoming connection */
socket_t* socket_accept( socket_t * const s,
                         socket_ops_t * const ops,
                         evt_loop_t * const el,
                         void * user_data );

/* disconnect all sockets */
socket_ret_t socket_disconnect( socket_t * const s );

/* get the socket type */
socket_type_t socket_get_type( socket_t * const s );

/* read data from the socket */
ssize_t socket_read( socket_t * const s, 
                     uint8_t * const buffer, 
                     size_t const n );

/* read data from the socket to iovec (scatter input) */
ssize_t socket_readv( socket_t * const s,
                      struct iovec * const iov,
                      size_t const iovcnt );

/* read data from the socket and get the addr too */
ssize_t socket_read_from( socket_t * const s,
                          uint8_t * const buffer,
                          size_t const n,
                          sockaddr_t * const addr,
                          socklen_t * const addrlen );

/* scatter input and get addr too */
ssize_t socket_readv_from( socket_t * const s,
                           struct iovec * const iov,
                           size_t const iovcnt,
                           sockaddr_t * const addr,
                           socklen_t * const addrlen );

/* write data to the socket */
socket_ret_t socket_write( socket_t * const s, 
                           uint8_t const * const buffer, 
                           size_t const n );

/* write iovec to the socket (gather output) */
socket_ret_t socket_writev( socket_t * const s,
                            struct iovec const * const iov,
                            size_t const iovcnt );

/* write data to the socket destined for the given addr */
socket_ret_t socket_write_to( socket_t * const s,
                              uint8_t const * const buffer,
                              size_t const n,
                              sockaddr_t const * const addr,
                              socklen_t const addrlen );

/* gather output destined for the given addr */
socket_ret_t socket_writev_to( socket_t * const s,
                               struct iovec const * const iov,
                               size_t const iovcnt,
                               sockaddr_t const * const addr,
                               socklen_t const addrlen );

/* flush the socket output */
socket_ret_t socket_flush( socket_t* const s );

#endif/*__SOCKET_H__*/
