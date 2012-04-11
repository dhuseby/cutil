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
#include <netinet/tcp.h>

#include "debug.h"
#include "macros.h"
#include "array.h"
#include "events.h"
#include "socket.h"

typedef struct socket_write_s
{
	void * data;
	size_t size;
	int iov;
	size_t nleft;

} socket_write_t;

static socket_ret_t socket_lookup_host( socket_t * const s, 
										int8_t const * const hostname)
{
	struct hostent *host;
	
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
	
	/* try to look up the host name */
	if( (host = gethostbyname((char const * const)hostname)) == NULL )
	{
		WARN("look up of %s failed\n", hostname);
		return SOCKET_BADHOSTNAME;
	}
	
	/* store the IP address */
	s->addr.s_addr = *((uint32_t*)host->h_addr_list[0]);
	
	return SOCKET_OK;
}

static evt_ret_t socket_write_fn( evt_loop_t * const el,
								  evt_t * const evt,
								  evt_params_t * const params,
								  void * user_data )
{
	int errval = 0;
	ssize_t written = 0;
	socklen_t len = sizeof(errval);
	socket_t * s = (socket_t*)user_data;

	DEBUG( "write event\n" );

	if ( s->connected )
	{
		while ( array_size( &(s->wbuf) ) > 0 )
		{
			/* we must have data to write */
			socket_write_t * wb = array_get_head( &(s->wbuf) );

			if ( wb->iov )
			{
				written = writev( s->socket, (struct iovec *)wb->data, (int)wb->size );
			}
			else
			{
				written = write( s->socket, wb->data, wb->size );
			}

			/* try to write the data to the socket */
			if ( written < 0 )
			{
				if ( (errno == EAGAIN) || (errno == EWOULDBLOCK) )
				{
					DEBUG( "write would block...waiting for next write event\n" );
					break;
				}
				else
				{
					WARN( "write error: %d\n", errno );
					if ( s->ops.error_fn != NULL )
					{
						DEBUG( "calling error callback\n" );
						(*(s->ops.error_fn))( s, errno, s->user_data );
					}
					return EVT_OK;
				}
			}
			else
			{
				/* decrement how many bytes are left to write */
				wb->nleft -= written;

				/* check to see if everything has been written */
				if ( wb->nleft <= 0 )
				{
					/* remove the write buffer from the queue */
					array_pop_head( &(s->wbuf) );

					/* call the write complete callback to let client know that a particular
					 * buffer has been written to the socket. */
					if ( s->ops.write_fn != NULL )
					{
						DEBUG( "calling write complete callback\n" );
						(*(s->ops.write_fn))( s, wb->data, s->user_data );
					}

					/* free it */
					FREE( wb );
				}
			}
		}

		/* no more data to write, stop the write event processing */
		evt_stop_event_handler( s->el, &(s->wevt) );
		return EVT_OK;
	}

	/* if we get here then the socket is completing a connect() */

	/* check to see if the connect() succeeded */
	if ( getsockopt( s->socket, SOL_SOCKET, SO_ERROR, &errval, &len ) < 0 )
	{
		WARN( "failed to get socket option while checking connect\n" );
		if ( s->ops.error_fn != NULL )
		{
			DEBUG( "calling error callback\n" );
			(*(s->ops.error_fn))( s, errno, s->user_data );
		}
		return EVT_ERROR;
	}

	ASSERT( len == sizeof(errval) );
	if ( errval == 0 )
	{
		DEBUG( "socket connected\n" );
		s->connected = TRUE;

		if ( s->ops.connect_fn != NULL )
		{
			DEBUG( "calling connect callback\n" );
			/* call the connect callback */
			(*(s->ops.connect_fn))( s, s->user_data );
		}

		/* we're connected to start read event */
		evt_start_event_handler( s->el, &(s->revt) );

		if ( array_size( &(s->wbuf) ) == 0 )
		{
			/* stop the write event processing until we have data to write */
			evt_stop_event_handler( s->el, &(s->wevt) );
		}

		return EVT_OK;
	}
	else
	{
		DEBUG( "socket connect failed\n" );
		if ( s->ops.error_fn != NULL )
		{
			DEBUG( "calling error callback\n" );
			(*(s->ops.error_fn))( s, errno, s->user_data );
		}

		/* stop write event */
		evt_stop_event_handler( s->el, &(s->wevt) );

		return EVT_ERROR;
	}

	return EVT_OK;
}

static evt_ret_t socket_read_fn( evt_loop_t * const el,
								 evt_t * const evt,
								 evt_params_t * const params,
								 void * user_data )
{
	size_t nread = 0;
	int errval = 0;
	socklen_t len = sizeof(errval);
	socket_t * s = (socket_t*)user_data;

	if ( s->connected )
	{
		DEBUG( "read event\n" );

		/* get how much data is available to read */
		if ( ioctl(s->socket, FIONREAD, &nread) < 0 )
		{
			if ( s->ops.error_fn != NULL )
			{
				DEBUG( "calling error callback\n" );
				(*(s->ops.error_fn))( s, errno, s->user_data );
			}
			return EVT_OK;
		}

		if ( nread == 0 )
		{
			/* the other side disconnected so follow suit */
			socket_disconnect( s );
			return EVT_OK;
		}

		/* callback to tell client that there is data to read */
		if ( s->ops.read_fn != NULL )
		{
			DEBUG( "calling read callback\n" );
			(*(s->ops.read_fn))( s, nread, s->user_data );
		}
	}

	return EVT_OK;
}

void socket_initialize( socket_t * const s,
						socket_type_t const type, 
					    socket_ops_t * const ops,
					    evt_loop_t * const el,
					    void * user_data )
{
	int32_t flags;

	MEMSET( (void*)s, 0, sizeof(socket_t) );

	/* store the type */
	s->type = type;

	/* store the user_data pointer */
	s->user_data = user_data;

	/* store the event loop pointer */
	s->el = el;
	
	/* copy the ops into place */
	MEMCPY( (void*)&(s->ops), ops, sizeof(socket_ops_t) );

	/* do the connect based on the type */
	switch(s->type)
	{
		case SOCKET_TCP:

			/* try to open a socket */
			if ( (s->socket = socket( PF_INET, SOCK_STREAM, IPPROTO_TCP )) < 0 )
			{
				WARN("failed to open socket\n");
				socket_delete( (void*)s );
			}
			else
			{
				DEBUG("created socket\n");
			}

			/* turn off TCP naggle algorithm */
			flags = 1;
			if ( setsockopt( s->socket, IPPROTO_TCP, TCP_NODELAY, (char*)&flags, sizeof(flags) ) )
			{
				WARN( "failed to turn on TCP no delay\n" );
				socket_delete( (void*)s );
			}
			else
			{
				DEBUG("turned on TCP no delay\n");
			}

			/* set the socket to non blocking mode */
			flags = fcntl( s->socket, F_GETFL );
			if( fcntl( s->socket, F_SETFL, (flags | O_NONBLOCK) ) < 0 )
			{
				WARN("failed to set socket to non-blocking\n");
			}
			else
			{
				DEBUG("socket is now non-blocking\n");
			}

			break;
	
		case SOCKET_UDP:
			WARN("UDP sockets not implemented\n");
			break;
		
		case SOCKET_SCTP:
			WARN("SCTP sockets not implemented\n");
			break;
	}

	/* initialize the write buffer */
	array_initialize( &(s->wbuf), FREE );

	/* set up params for socket write event */
	evt_params_t params;
	params.io_params.fd = s->socket;
	params.io_params.types = EVT_IO_WRITE;

	/* hook up the socket event */
	evt_initialize_event_handler( &(s->wevt), 
								  s->el, 
								  EVT_IO, 
								  &params, 
								  socket_write_fn, 
								  (void *)s );

	/* set up params for socket read event */
	params.io_params.types = EVT_IO_READ;

	/* hook up the socket event */
	evt_initialize_event_handler( &(s->revt), 
								  s->el, 
								  EVT_IO, 
								  &params, 
								  socket_read_fn, 
								  (void *)s );
}

socket_t* socket_new( socket_type_t const type, 
					  socket_ops_t * const ops,
					  evt_loop_t * const el,
					  void * user_data )
{
	socket_ret_t ret = SOCKET_OK;
	socket_t* s = NULL;
	
	/* allocate the socket struct */
	s = MALLOC( sizeof(socket_t) );
	CHECK_PTR_RET(s, NULL);

	/* initlialize the socket */
	socket_initialize( s, type, ops, el, user_data );

	return s;
}

void socket_deinitialize( socket_t * const s )
{
	/* stop write event */
	evt_stop_event_handler( s->el, &(s->wevt) );
	
	/* stop read event */
	evt_stop_event_handler( s->el, &(s->revt) );

	/* close the socket */
	if ( s->socket > 0 )
		socket_disconnect( s );

	/* clean up the array of write buffers */
	array_deinitialize( &(s->wbuf) );

	/* clean up the host name */
	if ( s->host != NULL )
		FREE ( s->host );
}

void socket_delete( void * s )
{
	socket_t * sock = (socket_t*)s;
	CHECK_PTR( s );

	socket_deinitialize( sock );
}

/* check to see if connected */
int socket_is_connected( socket_t* const s )
{
	CHECK_PTR_RET(s, 0);
	CHECK_RET( s->socket > 0, 0 );
	return s->connected;
}

socket_ret_t socket_connect( socket_t* const s, 
							 int8_t const * const hostname, 
							 uint16_t const port )
{
	int err = 0;
	socket_ret_t ret = SOCKET_OK;
	struct sockaddr_in addr;
	
	CHECK_PTR_RET(s, SOCKET_BADPARAM);
	CHECK_RET((hostname != NULL) || (s->host != NULL), SOCKET_BADHOSTNAME);
	CHECK_RET((port > 0) || (s->port > 0), SOCKET_INVALIDPORT);
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
			evt_start_event_handler( s->el, &(s->wevt) );

			/* initialize the socket address struct */
			MEMSET( &addr, 0, sizeof(struct sockaddr_in) );
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = s->addr.s_addr;
			addr.sin_port = htons(s->port);

			/* try to make the connection */
			if ( (err = connect(s->socket, (struct sockaddr*)&addr, sizeof(struct sockaddr))) < 0 )
			{
				if ( errno == EINPROGRESS )
				{
					DEBUG( "connection in progress\n" );
				}
				else
				{
					DEBUG("failed to initiate connect to the server\n");
					return SOCKET_ERROR;
				}
			}
		
			break;
		
		case SOCKET_UDP:
			WARN("UDP sockets not implemented\n");
			break;
		
		case SOCKET_SCTP:
			WARN("SCTP sockets not implemented\n");
			break;
	}

	return SOCKET_OK;
}

socket_ret_t socket_disconnect( socket_t* const s )
{
	CHECK_PTR_RET(s, SOCKET_BADPARAM);

	/* stop the socket write event processing */
	evt_stop_event_handler( s->el, &(s->wevt) );

	/* stop the socket read event processing */
	evt_stop_event_handler( s->el, &(s->revt) );

	/* clear the write buffer array */
	array_clear( &(s->wbuf) );

	if ( s->socket > 0 )
	{
		/* shutdown the socket */
		shutdown( s->socket, SHUT_RDWR );
		close( s->socket );
		s->socket = 0;
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

int32_t socket_read( socket_t* const s, 
					 uint8_t * const buffer, 
					 int32_t const n )
{
	ssize_t res = 0;

	CHECK_PTR_RET(s, 0);
	
	/* if they pass a NULL buffer pointer return -1 */
	CHECK_PTR_RET(buffer, 0);

	CHECK_RET(n > 0, 0);

	res = read( s->socket, buffer, n );
	switch ( (int)res )
	{
		case 0:
			errno = EPIPE;
		case -1:
			if ( s->ops.error_fn != NULL )
			{
				(*(s->ops.error_fn))( s, errno, s->user_data );
			}
			return 0;
		default:
			return (int32_t)res;
	}
}

/* write data to the socket */
static socket_ret_t socket_write_common( socket_t* const s, 
										 void * buffer,
										 size_t cnt,
										 size_t total,
										 int iov )
{
	int_t asize = 0;
	ssize_t res = 0;
	socket_write_t * wb = NULL;
	
	CHECK_PTR_RET(s, 0);
	CHECK_PTR_RET(buffer, 0);
	CHECK_RET(s->socket > 2, 0);
	CHECK_RET(cnt > 0, 0);

	wb = CALLOC( 1, sizeof(socket_write_t) );
	if ( wb == NULL )
	{
		WARN( "failed to allocate write buffer struct\n" );
		return SOCKET_ERROR;
	}

	/* store the values */
	wb->data = buffer;
	wb->size = cnt;
	wb->iov = iov;
	wb->nleft = total;

	/* queue the write */
	array_push_tail( &(s->wbuf), wb );

	/* just in case it isn't started, start the socket write event processing so
	 * the queued data will get written */
	evt_start_event_handler( s->el, &(s->wevt) );

	return SOCKET_OK;
}

socket_ret_t socket_write( socket_t * const s,
						   uint8_t const * const buffer,
						   size_t const n )
{
	return socket_write_common( s, (void*)buffer, n, n, FALSE );
}

socket_ret_t socket_writev( socket_t * const s,
							struct iovec * iov,
							size_t iovcnt )
{
	int i;
	size_t total = 0;

	/* calculate how many bytes are in the iovec */
	for ( i = 0; i < iovcnt; i++ )
	{
		total += iov[i].iov_len;
	}

	return socket_write_common( s, (void*)iov, iovcnt, total, TRUE );
}

/* flush the socket output */
socket_ret_t socket_flush( socket_t* const s )
{
	CHECK_PTR_RET(s, SOCKET_BADPARAM);
	CHECK_RET(s->socket > 0, SOCKET_ERROR);
	
	fsync( s->socket );
	
	return SOCKET_OK;
}

