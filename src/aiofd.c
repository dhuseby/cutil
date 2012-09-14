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
#include <errno.h>
#include <sys/ioctl.h>

#include "debug.h"
#include "macros.h"
#include "list.h"
#include "events.h"
#include "aiofd.h"

#if defined(UNIT_TESTING)
#include "test_flags.h"
extern evt_loop_t * el;
#endif

typedef struct aiofd_write_s
{
	void * data;
	size_t size;
	int iov;
	size_t nleft;

} aiofd_write_t;


static evt_ret_t aiofd_write_fn( evt_loop_t * const el,
								 evt_t * const evt,
								 evt_params_t * const params,
								 void * user_data )
{
	int keep_evt_on = TRUE;
	int errval = 0;
	ssize_t written = 0;
	aiofd_t * aiofd = (aiofd_t*)user_data;

	CHECK_PTR_RET( el, EVT_BADPTR );
	CHECK_PTR_RET( evt, EVT_BADPTR );
	CHECK_PTR_RET( params, EVT_BADPTR );
	CHECK_PTR_RET( aiofd, EVT_BADPTR );

	DEBUG( "write event\n" );

	while ( list_count( &(aiofd->wbuf) ) > 0 )
	{
		/* we must have data to write */
		aiofd_write_t * wb = list_get_head( &(aiofd->wbuf) );

		if ( ! wb )
		{
			list_pop_head( &(aiofd->wbuf) );
			return EVT_BADPTR;
		}

		if ( wb->iov )
		{
			written = WRITEV( aiofd->wfd, (struct iovec *)wb->data, (int)wb->size );
		}
		else
		{
			written = WRITE( aiofd->wfd, wb->data, wb->size );
		}

		/* try to write the data to the socket */
		if ( written < 0 )
		{
			if ( (ERRNO == EAGAIN) || (ERRNO == EWOULDBLOCK) )
			{
				DEBUG( "write would block...waiting for next write event\n" );
				break;
			}
			else
			{
				DEBUG( "write error: %d\n", ERRNO );
				if ( aiofd->ops.error_fn != NULL )
				{
					DEBUG( "calling error callback\n" );
					(*(aiofd->ops.error_fn))( aiofd, ERRNO, aiofd->user_data );
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
				list_pop_head( &(aiofd->wbuf) );

				/* call the write complete callback to let client know that a particular
				 * buffer has been written to the fd. */
				if ( aiofd->ops.write_fn != NULL )
				{
					DEBUG( "calling write complete callback\n" );
					keep_evt_on = (*(aiofd->ops.write_fn))( aiofd, wb->data, aiofd->user_data );
				}

				/* free it */
				FREE( wb );
			}
		}
	}

	/* call the write complete callback with NULL buffer to signal completion */
	if ( aiofd->ops.write_fn != NULL )
	{
		DEBUG( "calling write complete callback with null buffer\n" );
		keep_evt_on = (*(aiofd->ops.write_fn))( aiofd, NULL, aiofd->user_data );
	}
	
	if ( ! keep_evt_on )
	{
		/* stop the write event processing */
		evt_stop_event_handler( aiofd->el, &(aiofd->wevt) );
	}
	
	return EVT_OK;
}


static evt_ret_t aiofd_read_fn( evt_loop_t * const el,
								evt_t * const evt,
								evt_params_t * const params,
								void * user_data )
{
	int keep_going = TRUE;
	size_t nread = 0;
	aiofd_t * aiofd = (aiofd_t*)user_data;

	DEBUG( "read event\n" );

	/* get how much data is available to read */
	if ( (IOCTL( aiofd->rfd, FIONREAD, &nread ) < 0) && (aiofd->listen == FALSE) )
	{
		if ( aiofd->ops.error_fn != NULL )
		{
			DEBUG( "calling error callback\n" );
			(*(aiofd->ops.error_fn))( aiofd, ERRNO, aiofd->user_data );
		}
		return EVT_OK;
	}

	/* callback to tell client that there is data to read */
	if ( aiofd->ops.read_fn != NULL )
	{
		DEBUG( "calling read callback (nread = %d)\n", nread );
		keep_going  = (*(aiofd->ops.read_fn))( aiofd, nread, aiofd->user_data );
		DEBUG( "keep_going = %s\n", keep_going ? "TRUE" : "FALSE" );
	}

	/* we were told to stop the read event */
	if ( keep_going == FALSE )
	{
		DEBUG( "stopping read event\n" );
		/* stop the read event processing */
		evt_stop_event_handler( aiofd->el, &(aiofd->revt) );
	}

	return EVT_OK;
}


aiofd_t * aiofd_new( int const write_fd,
					 int const read_fd,
					 aiofd_ops_t * const ops,
					 evt_loop_t * const el,
					 void * user_data )
{
	aiofd_t * aiofd = NULL;
	
	/* allocate the the aiofd struct */
	aiofd = (aiofd_t*)CALLOC( 1, sizeof(aiofd_t) );
	CHECK_PTR_RET( aiofd, NULL );

	/* initlialize the aiofd */
	if ( !aiofd_initialize( aiofd, write_fd, read_fd, ops, el, user_data ) )
	{
		FREE( aiofd );
		return NULL;
	}

	return aiofd;
}

void aiofd_delete( void * aio )
{
	aiofd_t * aiofd = (aiofd_t*)aio;
	CHECK_PTR( aiofd );

	aiofd_deinitialize( aiofd );

	FREE( (void*)aiofd );
}

int aiofd_initialize( aiofd_t * const aiofd, 
					  int const write_fd,
					  int const read_fd,
					  aiofd_ops_t * const ops,
					  evt_loop_t * const el,
					  void * user_data )
{
	evt_params_t params;
#if defined(UNIT_TESTING)
	CHECK_RET( !fake_aiofd_initialize, fake_aiofd_initialize_ret );
#endif
	CHECK_PTR_RET( aiofd, FALSE );
	CHECK_PTR_RET( ops, FALSE );
	CHECK_PTR_RET( el, FALSE );
	CHECK_RET( write_fd >= 0, FALSE );
	CHECK_RET( read_fd >= 0, FALSE );

	MEMSET( (void*)aiofd, 0, sizeof(aiofd_t) );

	/* store the file descriptors */
	aiofd->wfd = write_fd;
	aiofd->rfd = read_fd;

	/* initialize the write buffer */
	CHECK_RET( list_initialize( &(aiofd->wbuf), 8, FREE ), FALSE );

	/* set up params for fd write event */
	params.io_params.fd = aiofd->wfd;
	params.io_params.types = EVT_IO_WRITE;

	/* hook up the fd write event */
	if ( !evt_initialize_event_handler( &(aiofd->wevt), 
										EVT_IO, 
										&params, 
										aiofd_write_fn, 
										(void *)aiofd ) )
	{
		list_deinitialize( &(aiofd->wbuf) );
		return FALSE;
	}

	/* set up params for socket read event */
	params.io_params.fd = aiofd->rfd;
	params.io_params.types = EVT_IO_READ;

	/* hook up the fd read event */
	if ( !evt_initialize_event_handler( &(aiofd->revt), 
										EVT_IO, 
										&params, 
										aiofd_read_fn, 
										(void *)aiofd ) )
	{
		list_deinitialize( &(aiofd->wbuf) );
		return FALSE;
	}

	/* store the event loop pointer */
	aiofd->el = el;
	
	/* store the user_data pointer */
	aiofd->user_data = user_data;

	/* copy the ops into place */
	MEMCPY( (void*)&(aiofd->ops), ops, sizeof(aiofd_ops_t) );

	return TRUE;
}

void aiofd_deinitialize( aiofd_t * const aiofd )
{
	/* deinitialize the write event */
	evt_deinitialize_event_handler( &(aiofd->wevt) );

	/* deinitialize the read event */
	evt_deinitialize_event_handler( &(aiofd->revt) );

	/* clean up the list of write buffers */
	list_deinitialize( &(aiofd->wbuf) );
}

int aiofd_enable_write_evt( aiofd_t * const aiofd,
							int enable )
{
	CHECK_RET( aiofd, FALSE );

	if ( enable )
	{
		evt_start_event_handler( aiofd->el, &(aiofd->wevt) );
	}
	else
	{
		evt_stop_event_handler( aiofd->el, &(aiofd->wevt) );
	}
	return TRUE;
}

int aiofd_enable_read_evt( aiofd_t * const aiofd,
						   int enable )
{
#if defined(UNIT_TESTING)
	CHECK_RET( !fake_aiofd_enable_read_evt, fake_aiofd_enable_read_evt_ret );
#endif
	CHECK_RET( aiofd, FALSE );

	if ( enable )
	{
		DEBUG("starting read event\n");
		evt_start_event_handler( aiofd->el, &(aiofd->revt) );
	}
	else
	{
		DEBUG("stopping read event\n");
		evt_stop_event_handler( aiofd->el, &(aiofd->revt) );
	}
	return TRUE;
}

int32_t aiofd_read( aiofd_t * const aiofd,
					uint8_t * const buffer,
					int32_t const n )
{
	ssize_t res = 0;
#if defined(UNIT_TESTING)
	CHECK_RET( !fake_aiofd_read, fake_aiofd_read_ret );
#endif
	CHECK_PTR_RET(aiofd, 0);
	
	/* if they pass a NULL buffer pointer return -1 */
	CHECK_PTR_RET(buffer, 0);

	CHECK_RET(n > 0, 0);

	res = READ( aiofd->rfd, buffer, n );
	switch ( (int)res )
	{
		case 0:
			errno = EPIPE;
		case -1:
			if ( aiofd->ops.error_fn != NULL )
			{
				(*(aiofd->ops.error_fn))( aiofd, ERRNO, aiofd->user_data );
			}
			return 0;
		default:
			return (int32_t)res;
	}
}


/* queue up data to write to the fd */
static int aiofd_write_common( aiofd_t* const aiofd, 
							   void * buffer,
							   size_t cnt,
							   size_t total,
							   int iov )
{
	int32_t asize = 0;
	ssize_t res = 0;
	aiofd_write_t * wb = NULL;
	
	CHECK_PTR_RET(aiofd, 0);
	CHECK_PTR_RET(buffer, 0);
	CHECK_RET(cnt > 0, 0);

	wb = CALLOC( 1, sizeof(aiofd_write_t) );
	if ( wb == NULL )
	{
		WARN( "failed to allocate write buffer struct\n" );
		return FALSE;
	}

	/* store the values */
	wb->data = buffer;
	wb->size = cnt;
	wb->iov = iov;
	wb->nleft = total;

	/* queue the write */
	list_push_tail( &(aiofd->wbuf), wb );

	/* just in case it isn't started, start the write event processing so
	 * the queued data will get written */
	evt_start_event_handler( aiofd->el, &(aiofd->wevt) );

	return TRUE;
}

int aiofd_write( aiofd_t * const aiofd, 
				 uint8_t const * const buffer, 
				 size_t const n )
{
#if defined(UNIT_TESTING)
	CHECK_RET( !fake_aiofd_write, fake_aiofd_write_ret );
#endif
	return aiofd_write_common( aiofd, (void*)buffer, n, n, FALSE );
}

int aiofd_writev( aiofd_t * const aiofd,
				  struct iovec * iov,
				  size_t iovcnt )
{
	int i;
	size_t total = 0;

#if defined(UNIT_TESTING)
	CHECK_RET( !fake_aiofd_writev, fake_aiofd_writev_ret );
#endif

	/* calculate how many bytes are in the iovec */
	for ( i = 0; i < iovcnt; i++ )
	{
		total += iov[i].iov_len;
	}

	return aiofd_write_common( aiofd, (void*)iov, iovcnt, total, TRUE );
}

int aiofd_flush( aiofd_t * const aiofd )
{
	CHECK_PTR_RET(aiofd, FALSE);
	
	fsync( aiofd->wfd );
	fsync( aiofd->rfd );
	
	return TRUE;
}

int aiofd_set_listen( aiofd_t * const aiofd, int listen )
{
	CHECK_PTR_RET( aiofd, FALSE );

	aiofd->listen = listen;

	return TRUE;
}

int aiofd_get_listen( aiofd_t * const aiofd )
{
	CHECK_PTR_RET( aiofd, FALSE );
	return aiofd->listen;
}

#if defined(UNIT_TESTING)

#include <CUnit/Basic.h>

typedef struct cb_count_s
{
	int r, w, e;
} cb_count_t;

static cb_count_t cb_counts;

static int read_callback_fn( aiofd_t * const aiofd, size_t nread, void * user_data )
{
	((cb_count_t*)user_data)->r++;
	return TRUE;
}

static int write_callback_fn( aiofd_t * const aiofd, uint8_t const * const buffer, void * user_data )
{
	((cb_count_t*)user_data)->w++;
	return TRUE;
}

static int error_callback_fn( aiofd_t * const aiofd, int err, void * user_data )
{
	((cb_count_t*)user_data)->e++;
	return TRUE;
}

static void test_aiofd_write_fn( void )
{
	evt_t evt;
	evt_params_t params;
	aiofd_t aiofd;
	aiofd_write_t * wb = NULL;
	int8_t const * const buf = "foo";
	int const size = 4;
	struct iovec iov;

	MEMSET( &evt, 0, sizeof( evt_t ) );
	MEMSET( &params, 0, sizeof( evt_params_t ) );
	MEMSET( &aiofd, 0, sizeof( aiofd_t ) );
	MEMSET( &iov, 0, sizeof( struct iovec) );
	MEMSET( &cb_counts, 0, sizeof( cb_count_t ) );

	CU_ASSERT_EQUAL( aiofd_write_fn( NULL, NULL, NULL, NULL ), EVT_BADPTR );
	CU_ASSERT_EQUAL( aiofd_write_fn( el, NULL, NULL, NULL ), EVT_BADPTR );
	CU_ASSERT_EQUAL( aiofd_write_fn( el, &evt, NULL, NULL ), EVT_BADPTR );
	CU_ASSERT_EQUAL( aiofd_write_fn( el, &evt, &params, NULL ), EVT_BADPTR );

	/* nothing in the write buffer, no write callback, should fall through to OK */
	list_initialize( &(aiofd.wbuf), 0, NULL );
	CU_ASSERT_EQUAL( aiofd_write_fn( el, &evt, &params, &aiofd ), EVT_OK );

	/* set user data to the callback counter */
	aiofd.user_data = &cb_counts;

	aiofd.ops.write_fn = &write_callback_fn;
	MEMSET( &cb_counts, 0, sizeof( cb_count_t ) );

	/* w callback should be called once, queue should be empty */
	CU_ASSERT_EQUAL( aiofd_write_fn( el, &evt, &params, &aiofd ), EVT_OK );
	CU_ASSERT_EQUAL( cb_counts.r, 0 );
	CU_ASSERT_EQUAL( cb_counts.w, 1 );
	CU_ASSERT_EQUAL( cb_counts.e, 0 );
	CU_ASSERT_EQUAL( list_count( &(aiofd.wbuf) ), 0 );

	/* push a NULL pointer into the write queue and confirm that it recovers gracefully */	
	list_push_tail( &(aiofd.wbuf), NULL );
	MEMSET( &cb_counts, 0, sizeof( cb_count_t ) );

	CU_ASSERT_EQUAL( aiofd_write_fn( el, &evt, &params, &aiofd ), EVT_BADPTR );
	CU_ASSERT_EQUAL( cb_counts.r, 0 );
	CU_ASSERT_EQUAL( cb_counts.w, 0 );
	CU_ASSERT_EQUAL( cb_counts.e, 0 );
	CU_ASSERT_EQUAL( list_count( &(aiofd.wbuf) ), 0 );

	/* now queue up a non-iovec pending write */
	wb = CALLOC( 1, sizeof( aiofd_write_t ) );
	wb->data = (void*)buf;
	wb->size = size;
	wb->iov = FALSE;
	wb->nleft = size;
	list_push_tail( &(aiofd.wbuf), wb );
	MEMSET( &cb_counts, 0, sizeof( cb_count_t ) );
	fake_write = TRUE;
	fake_write_ret = 2;

	/* results in 2 calls to write callback and an empty wbuf queue */
	CU_ASSERT_EQUAL( aiofd_write_fn( el, &evt, &params, &aiofd ), EVT_OK );
	CU_ASSERT_EQUAL( cb_counts.r, 0 );
	CU_ASSERT_EQUAL( cb_counts.w, 2 ); /* 1 for buf write complete, 1 for queue empty */
	CU_ASSERT_EQUAL( cb_counts.e, 0 );
	CU_ASSERT_EQUAL( list_count( &(aiofd.wbuf) ), 0 ); /* queue should be empty */

	/* remove write callback pointer */
	wb = CALLOC( 1, sizeof( aiofd_write_t ) );
	wb->data = (void*)buf;
	wb->size = size;
	wb->iov = FALSE;
	wb->nleft = size;
	list_push_tail( &(aiofd.wbuf), wb );
	aiofd.ops.write_fn = NULL;
	MEMSET( &cb_counts, 0, sizeof( cb_count_t ) );
	fake_write_ret = 4;

	/* results in no callback, queue should be empty */
	CU_ASSERT_EQUAL( aiofd_write_fn( el, &evt, &params, &aiofd ), EVT_OK );
	CU_ASSERT_EQUAL( cb_counts.r, 0 );
	CU_ASSERT_EQUAL( cb_counts.w, 0 ); /* NULL callback pointer */
	CU_ASSERT_EQUAL( cb_counts.e, 0 );
	CU_ASSERT_EQUAL( list_count( &(aiofd.wbuf) ), 0 ); /* queue should be empty */

	/* done with write, moving to writev */
	fake_write = FALSE;
	fake_write_ret = -1;
	fake_writev = TRUE;
	fake_writev_ret = 2;

	/* queue up a writev */
	iov.iov_base = (void*)buf;
	iov.iov_len = size;
	wb = CALLOC( 1, sizeof( aiofd_write_t ) );
	wb->data = (void*)&iov;
	wb->size = 1;
	wb->iov = TRUE;
	wb->nleft = size;
	list_push_tail( &(aiofd.wbuf), wb );
	aiofd.ops.write_fn = &write_callback_fn;
	MEMSET( &cb_counts, 0, sizeof( cb_count_t ) );

	/* results in 2 calls to write callback and an empty wbuf queue */
	CU_ASSERT_EQUAL( aiofd_write_fn( el, &evt, &params, &aiofd ), EVT_OK );
	CU_ASSERT_EQUAL( cb_counts.r, 0 );
	CU_ASSERT_EQUAL( cb_counts.w, 2 ); /* NULL callback pointer */
	CU_ASSERT_EQUAL( cb_counts.e, 0 );
	CU_ASSERT_EQUAL( list_count( &(aiofd.wbuf) ), 0 ); /* queue should be empty */


	wb = CALLOC( 1, sizeof( aiofd_write_t ) );
	wb->data = (void*)&iov;
	wb->size = 1;
	wb->iov = TRUE;
	wb->nleft = size;
	list_push_tail( &(aiofd.wbuf), wb );
	aiofd.ops.write_fn = NULL;
	MEMSET( &cb_counts, 0, sizeof( cb_count_t ) );
	fake_writev_ret = 4;

	/* results in no callback, queue should be empty */
	CU_ASSERT_EQUAL( aiofd_write_fn( el, &evt, &params, &aiofd ), EVT_OK );
	CU_ASSERT_EQUAL( cb_counts.r, 0 );
	CU_ASSERT_EQUAL( cb_counts.w, 0 ); /* NULL callback pointer */
	CU_ASSERT_EQUAL( cb_counts.e, 0 );
	CU_ASSERT_EQUAL( list_count( &(aiofd.wbuf) ), 0 ); /* queue should be empty */

	wb = CALLOC( 1, sizeof( aiofd_write_t ) );
	wb->data = (void*)&iov;
	wb->size = 1;
	wb->iov = TRUE;
	wb->nleft = size;
	list_push_tail( &(aiofd.wbuf), wb );
	aiofd.ops.write_fn = &write_callback_fn;
	MEMSET( &cb_counts, 0, sizeof( cb_count_t ) );
	fake_writev_ret = -1;
	fake_errno = TRUE;
	fake_errno_value = EAGAIN;

	/* results in 1 callback, queue should not be empty */
	CU_ASSERT_EQUAL( aiofd_write_fn( el, &evt, &params, &aiofd ), EVT_OK );
	CU_ASSERT_EQUAL( cb_counts.r, 0 );
	CU_ASSERT_EQUAL( cb_counts.w, 1 ); /* NULL callback pointer */
	CU_ASSERT_EQUAL( cb_counts.e, 0 );
	CU_ASSERT_EQUAL( list_count( &(aiofd.wbuf) ), 1 ); /* queue should be empty */

	aiofd.ops.write_fn = NULL;
	MEMSET( &cb_counts, 0, sizeof( cb_count_t ) );
	fake_errno_value = EWOULDBLOCK;

	/* results in no callback, queue should not be empty */
	CU_ASSERT_EQUAL( aiofd_write_fn( el, &evt, &params, &aiofd ), EVT_OK );
	CU_ASSERT_EQUAL( cb_counts.r, 0 );
	CU_ASSERT_EQUAL( cb_counts.w, 0 ); /* NULL callback pointer */
	CU_ASSERT_EQUAL( cb_counts.e, 0 );
	CU_ASSERT_EQUAL( list_count( &(aiofd.wbuf) ), 1 ); /* queue should be empty */

	MEMSET( &cb_counts, 0, sizeof( cb_count_t ) );
	fake_errno_value = EBADF;

	/* results in no callback, queue should not be empty */
	CU_ASSERT_EQUAL( aiofd_write_fn( el, &evt, &params, &aiofd ), EVT_OK );
	CU_ASSERT_EQUAL( cb_counts.r, 0 );
	CU_ASSERT_EQUAL( cb_counts.w, 0 ); /* NULL callback pointer */
	CU_ASSERT_EQUAL( cb_counts.e, 0 );
	CU_ASSERT_EQUAL( list_count( &(aiofd.wbuf) ), 1 ); /* queue should be empty */

	MEMSET( &cb_counts, 0, sizeof( cb_count_t ) );
	aiofd.ops.write_fn = &write_callback_fn;
	fake_errno_value = EBADF;

	/* results in 1 write callback, queue should not be empty */
	CU_ASSERT_EQUAL( aiofd_write_fn( el, &evt, &params, &aiofd ), EVT_OK );
	CU_ASSERT_EQUAL( cb_counts.r, 0 );
	CU_ASSERT_EQUAL( cb_counts.w, 1 ); /* NULL callback pointer */
	CU_ASSERT_EQUAL( cb_counts.e, 0 );
	CU_ASSERT_EQUAL( list_count( &(aiofd.wbuf) ), 1 ); /* queue should be empty */

	MEMSET( &cb_counts, 0, sizeof( cb_count_t ) );
	aiofd.ops.error_fn = &error_callback_fn;
	fake_errno_value = EBADF;

	/* results in 1 write and 1 error callback, queue should not be empty */
	CU_ASSERT_EQUAL( aiofd_write_fn( el, &evt, &params, &aiofd ), EVT_OK );
	CU_ASSERT_EQUAL( cb_counts.r, 0 );
	CU_ASSERT_EQUAL( cb_counts.w, 1 ); /* NULL callback pointer */
	CU_ASSERT_EQUAL( cb_counts.e, 1 );
	CU_ASSERT_EQUAL( list_count( &(aiofd.wbuf) ), 1 ); /* queue should be empty */

	FREE( wb );

	fake_errno = FALSE;
	fake_errno_value = -1;
	fake_writev = FALSE;
	fake_writev_ret = -1;
}

void test_aiofd_private_functions( void )
{
	test_aiofd_write_fn();
}

#endif


