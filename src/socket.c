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
#include "array.h"
#include "events.h"
#include "socket.h"

struct socket_s
{
	socket_type_t	type;			/* type of socket */
	int32_t			connected;		/* is the socket connected? */
	int32_t			bound;			/* is the socket bound? */
	int8_t*			host;			/* host name */
	uint16_t		port;			/* port number */
	IPv4			addr;			/* IPv4 struct from host string */
	void *			user_data;		/* passed to ops callbacks */
	socket_ops_t	ops;			/* socket callbacks */
	aiofd_t			aiofd;			/* the fd management state */
};

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
	socklen_t len = sizeof(errval);
	socket_t * s = (socket_t*)user_data;
	CHECK_PTR_RET( aiofd, FALSE );
	CHECK_PTR_RET( s, FALSE );

	if ( s->connected )
	{
		ASSERT( s->aiofd.rfd != -1 );

		if ( buffer == NULL )
		{
			if ( array_size( &(s->aiofd.wbuf) ) == 0 )
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
		if ( getsockopt( s->aiofd.wfd, SOL_SOCKET, SO_ERROR, &errval, &len ) < 0 )
		{
			WARN( "failed to get socket option while checking connect\n" );
			if ( s->ops.error_fn != NULL )
			{
				DEBUG( "calling socket error callback\n" );
				(*(s->ops.error_fn))( s, errno, s->user_data );
			}
			
			/* stop write event processing */
			return FALSE;
		}

		ASSERT( len == sizeof(errval) );
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
			aiofd_enable_read_evt( &(s->aiofd), TRUE );

			if ( array_size( &(s->aiofd.wbuf) ) == 0 )
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
				WARN( "failed to accept incoming connection!\n" );
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
			if ( (fd = socket( PF_INET, SOCK_STREAM, IPPROTO_TCP )) < 0 )
			{
				WARN("failed to open socket\n");
				return FALSE;
			}
			else
			{
				DEBUG("created socket\n");
			}

			/* turn off TCP naggle algorithm */
			flags = 1;
			if ( setsockopt( fd, IPPROTO_TCP, TCP_NODELAY, (char*)&flags, sizeof(flags) ) )
			{
				WARN( "failed to turn on TCP no delay\n" );
				return FALSE;
			}
			else
			{
				DEBUG("turned on TCP no delay\n");
			}

			break;
	
		case SOCKET_UNIX:

			/* try to open a socket */
			if ( (fd = socket( PF_UNIX, SOCK_STREAM, 0 )) < 0 )
			{
				WARN("failed to open socket\n");
				return FALSE;
			}
			else
			{
				DEBUG("created socket\n");
			}

			break;
	}

	/* set the socket to non blocking mode */
	flags = fcntl( fd, F_GETFL );
	if( fcntl( fd, F_SETFL, (flags | O_NONBLOCK) ) < 0 )
	{
		WARN("failed to set socket to non-blocking\n");
	}
	else
	{
		DEBUG("socket is now non-blocking\n");
	}

	/* initialize the aiofd to manage the socket */
	if ( aiofd_initialize( &(s->aiofd), fd, fd, &aiofd_ops, el, (void*)s ) == FALSE )
	{
		WARN( "failed to initialzie the aiofd\n" );
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
	/* shut down the aiofd */
	aiofd_deinitialize( &(s->aiofd) );

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
	CHECK_PTR_RET( s, FALSE );
	CHECK_RET( s->aiofd.rfd >= 0, FALSE );
	return s->connected;
}

socket_ret_t socket_connect( socket_t* const s, 
							 int8_t const * const hostname, 
							 uint16_t const port )
{
	int err = 0;
	int len = 0;
	socket_ret_t ret = SOCKET_OK;
	struct sockaddr_in in_addr;
	struct sockaddr_un un_addr;
	
	CHECK_PTR_RET(s, SOCKET_BADPARAM);
	CHECK_RET((hostname != NULL) || (s->host != NULL), SOCKET_BADHOSTNAME);
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

			/* initialize the socket address struct */
			MEMSET( &in_addr, 0, sizeof(struct sockaddr_in) );
			in_addr.sin_family = AF_INET;
			in_addr.sin_addr.s_addr = s->addr.s_addr;
			in_addr.sin_port = htons(s->port);

			/* try to make the connection */
			if ( (err = connect(s->aiofd.rfd, (struct sockaddr*)&in_addr, sizeof(struct sockaddr))) < 0 )
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
		
		case SOCKET_UNIX:
			
			/* start the socket write event processing so we can catch connection */
			aiofd_enable_write_evt( &(s->aiofd), TRUE );

			/* initialize the socket address struct */
			MEMSET( &un_addr, 0, sizeof(struct sockaddr_un) );
			un_addr.sun_family = AF_UNIX;
			strncpy( un_addr.sun_path, s->host, 100 );

			/* calculate the length of the address struct */
			len = strlen( un_addr.sun_path ) + sizeof( un_addr.sun_family );

			/* try to make the connection */
			if ( (err = connect(s->aiofd.rfd, (struct sockaddr*)&un_addr, len)) < 0 )
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
	}

	return SOCKET_OK;
}


/* check to see if bound */
int socket_is_bound( socket_t* const s )
{
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
			if ( setsockopt( s->aiofd.rfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0 )
			{
				WARN( "failed to set the socket to reuse addr\n" );
			}

			/* initialize the socket address struct */
			MEMSET( &in_addr, 0, sizeof(struct sockaddr_in) );
			in_addr.sin_family = AF_INET;
			in_addr.sin_addr.s_addr = s->addr.s_addr;
			in_addr.sin_port = htons(s->port);

			len = sizeof( in_addr );

			if ( bind( s->aiofd.rfd, (struct sockaddr*)&in_addr, len) < 0 )
			{
				DEBUG( "failed to bind (errno: %d)\n", errno );
				return SOCKET_ERROR;
			}

			/* flag the socket as bound */
			s->bound = TRUE;

			break;

		case SOCKET_UNIX:
			
			/* initialize the socket address struct */
			MEMSET( &un_addr, 0, sizeof(struct sockaddr_un) );
			un_addr.sun_family = AF_UNIX;
			strncpy( un_addr.sun_path, s->host, 100 );

			/* try to delete the socket inode */
			unlink( un_addr.sun_path );

			/* calculate the length of the address struct */
			len = strlen( un_addr.sun_path ) + sizeof( un_addr.sun_family );

			if ( bind( s->aiofd.rfd, (struct sockaddr*)&un_addr, len) < 0 )
			{
				DEBUG( "failed to bind (errno: %d)\n", errno );
				return SOCKET_ERROR;
			}

			/* flag the socket as bound */
			s->bound = TRUE;

			break;
	}

	/* start the socket read even processing to catch incoming connections */
	aiofd_enable_read_evt( &(s->aiofd), TRUE );

	return SOCKET_OK;
}

socket_ret_t socket_listen( socket_t * const s, int const backlog )
{
	CHECK_PTR_RET( s, SOCKET_BADPARAM );
	CHECK_RET( socket_is_bound( s ), SOCKET_BOUND );
	CHECK_RET( !socket_is_connected( s ), SOCKET_CONNECTED );

	/* now begin listening for incoming connections */
	if ( listen( s->aiofd.rfd, backlog ) < 0 )
	{
		DEBUG( "failed to listen (errno: %d)\n", errno );
		return SOCKET_ERROR;
	}

	/* set the listen flag so that it doesn't error on 0 size read callbacks */
	aiofd_set_listen( &(s->aiofd), TRUE );

	return SOCKET_OK;
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
	CHECK_RET( socket_is_bound( s ), NULL );

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
			if ( (fd = accept( s->aiofd.rfd, (struct sockaddr *)&in_addr, &len )) < 0 )
			{
				WARN("failed to accept incoming connection\n");
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
			if ( setsockopt( fd, IPPROTO_TCP, TCP_NODELAY, (char*)&flags, sizeof(flags) ) )
			{
				WARN( "failed to turn on TCP no delay\n" );
				socket_delete( (void*)client );
				return NULL;
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
			if ( (fd = accept( s->aiofd.rfd, (struct sockaddr *)&un_addr, &len )) < 0 )
			{
				WARN("failed to open socket\n");
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
	flags = fcntl( fd, F_GETFL );
	if( fcntl( fd, F_SETFL, (flags | O_NONBLOCK) ) < 0 )
	{
		WARN("failed to set socket to non-blocking\n");
	}
	else
	{
		DEBUG("socket is now non-blocking\n");
	}

	/* initialize the aiofd to manage the socket */
	aiofd_initialize( &(client->aiofd), fd, fd, &aiofd_ops, el, (void*)client );
	
	DEBUG( "socket connected\n" );
	client->connected = TRUE;

	if ( client->ops.connect_fn != NULL )
	{
		DEBUG( "calling socket connect callback\n" );
		/* call the connect callback */
		(*(client->ops.connect_fn))( client, client->user_data );
	}

	/* we're connected to start read event */
	aiofd_enable_read_evt( &(client->aiofd), TRUE );

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
	return aiofd_write( &(s->aiofd), buffer, n );
}

socket_ret_t socket_writev( socket_t * const s,
							struct iovec * iov,
							size_t iovcnt )
{
	return aiofd_writev( &(s->aiofd), iov, iovcnt );
}

/* flush the socket output */
socket_ret_t socket_flush( socket_t* const s )
{
	CHECK_PTR_RET(s, SOCKET_BADPARAM);
	return aiofd_flush( &(s->aiofd) );
}

