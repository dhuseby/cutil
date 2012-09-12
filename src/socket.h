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
	SOCKET_BOUND		= -8,
	SOCKET_CONNECT_FAIL = -9

} socket_ret_t;

typedef enum socket_type_e
{
    SOCKET_TCP,
	SOCKET_UNIX,

	SOCKET_LAST,
	SOCKET_FIRST = SOCKET_TCP,
	SOCKET_COUNT = (SOCKET_LAST - SOCKET_FIRST),

	SOCKET_UNKNOWN = -1
	
} socket_type_t;

#define VALID_SOCKET_TYPE( t ) ((t >= SOCKET_FIRST) && (t < SOCKET_LAST))

typedef struct in_addr IPv4;
typedef struct in6_addr IPv6;
typedef struct socket_s socket_t;

typedef struct socket_ops_s 
{
	socket_ret_t (*connect_fn)( socket_t * const s, void * user_data );
	socket_ret_t (*disconnect_fn)( socket_t * const s, void * user_data );
	socket_ret_t (*error_fn)( socket_t * const s, int err, void * user_data );
	int32_t (*read_fn)( socket_t * const s, size_t nread, void * user_data );
	int32_t (*write_fn)( socket_t * const s, uint8_t const * const buffer, void * user_data );

} socket_ops_t;

/* create/destroy a socket */
socket_t* socket_new( socket_type_t const type,
					  socket_ops_t * const ops,
					  evt_loop_t * const el,
					  void * user_data );

void socket_delete( void * s );

/* check to see if connected */
int socket_is_connected( socket_t * const s );

/* connect a socket as a client*/
socket_ret_t socket_connect( socket_t * const s, 
							 int8_t const * const host, 
							 uint16_t const port );

/* check to see if bound */
int socket_is_bound( socket_t * const s );

/* bind a socket to a specified IP/port or inode */
socket_ret_t socket_bind( socket_t * const s,
						  int8_t const * const host,
						  uint16_t const port );

/* listen for incoming connections */
socket_ret_t socket_listen( socket_t * const s,
							int const backlog );
int socket_is_listening( socket_t * const s );

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
int32_t socket_read( socket_t * const s, 
					 uint8_t * const buffer, 
					 int32_t const n );

/* write data to the socket */
socket_ret_t socket_write( socket_t * const s, 
						   uint8_t const * const buffer, 
						   size_t const n );

/* write iovec to the socket */
socket_ret_t socket_writev( socket_t * const s,
							struct iovec * iov,
							size_t iovcnt );

/* flush the socket output */
socket_ret_t socket_flush( socket_t* const s );

#endif/*__SOCKET_H__*/
