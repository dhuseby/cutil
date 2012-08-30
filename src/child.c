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
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/uio.h>

#if defined (__APPLE__) && defined (UNIT_TESTING)
#include <vproc.h>					/* to get vproc_transaction_begin() workaround */
#endif

#include "debug.h"
#include "macros.h"
#include "events.h"
#include "list.h"
#include "privileges.h"
#include "sanitize.h"
#include "aiofd.h"
#include "child.h"

#if defined(UNIT_TESTING)
static int fail_fork = FALSE;
static int good_pipes = -1;
#endif

struct child_process_s
{
	pid_t			pid;			/* the pid of the child process */
	child_ops_t		ops;			/* child proces callbacks */
	aiofd_t			aiofd;			/* the fd management state */
	int				exited;			/* have we received the SIGCHLD signal? */
	evt_t			sigchld;		/* the SIGCHILD signal event handler */
	void *			user_data;		/* passed to ops callbacks */
};

static evt_ret_t sigchld_cb( evt_loop_t * const el,
							 evt_t * const evt,
							 evt_params_t * const params,
							 void * user_data )
{
	child_process_t * child = (child_process_t*)user_data;
	CHECK_PTR_RET( child, EVT_BAD_PTR );
	CHECK_RET( (child->pid == params->child_params.pid), EVT_ERROR );
	
	DEBUG( "received SIGCHLD\n" );

	/* execute the exit_fn callback */
	if ( child->ops.exit_fn != NULL )
	{
		DEBUG( "calling child process exit callback\n" );
		(*(child->ops.exit_fn))( child, 
								 params->child_params.rpid, 
								 params->child_params.rstatus,
								 child->user_data );
	}

	/* record that the child process has exited */
	child->exited = TRUE;

	return EVT_OK;
}


static int child_aiofd_write_fn( aiofd_t * const aiofd,
								 uint8_t const * const buffer,
								 void * user_data )
{
	child_process_t * child = (child_process_t*)user_data;
	CHECK_PTR_RET( aiofd, FALSE );
	CHECK_PTR_RET( child, FALSE );

	ASSERT( child->aiofd.rfd != -1 );

	if ( buffer == NULL )
	{
		if ( list_count( &(child->aiofd.wbuf) ) == 0 )
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
		if ( child->ops.write_fn != NULL )
		{
			DEBUG( "calling child process write complete callback\n" );
			(*(child->ops.write_fn))( child, buffer, child->user_data );
		}

		return TRUE;
	}

	return TRUE;
}


static int child_aiofd_error_fn( aiofd_t * const aiofd,
								  int err,
								  void * user_data )
{
	child_process_t * child = (child_process_t*)user_data;
	CHECK_PTR_RET( aiofd, FALSE );
	CHECK_PTR_RET( child, FALSE );

	return TRUE;
}


static int child_aiofd_read_fn( aiofd_t * const aiofd,
								size_t const nread,
								void * user_data )
{
	child_process_t * child = (child_process_t*)user_data;
	CHECK_PTR_RET( aiofd, FALSE );
	CHECK_PTR_RET( child, FALSE );

	if ( nread == 0 )
	{
		/* return FALSE to turn off the read event processing */
		return FALSE;
	}

	if ( child->ops.read_fn != NULL )
	{
		(*(child->ops.read_fn))( child, nread, child->user_data );
	}

	return TRUE;
}


static pid_t safe_fork( int keepfds[], int nfds )
{
	pid_t childpid;

#if defined(UNIT_TESTING)
	if ( fail_fork == TRUE )
		childpid = -1;
	else
		childpid = fork();
#else
	childpid = fork();
#endif

	/* this is a workaround to a bug in Mac OS X < 10.7 */
#if defined (__APPLE__) && defined (UNIT_TESTING)
	vproc_transaction_begin(0);
#endif

	/* test for error */
	if ( childpid == -1 )
		return -1;

	/* test for parent process */
	if ( childpid != 0 )
		return childpid; /* PARENT PROCESS RETURNS */

	/* CHILD PROCESS  */

	/* clean up open file descriptors */
	sanitize_files( keepfds, nfds );

	/* remove root privileges and any unnecessary group privileges permanently */
	drop_privileges( TRUE );

	ASSERT( childpid == 0 );

	/* this returns 0 */
	return childpid;
}

static int safe_pipe( int pipefd[2] )
{
#if defined(UNIT_TESTING)
	if ( good_pipes != -1)
	{
		if ( good_pipes == 0 )
			return -1;
		good_pipes--;
	}
#endif
	return pipe(pipefd);
}

#define PIPE_READ_FD 0
#define PIPE_WRITE_FD 1

static int child_process_initialize( child_process_t * const child,
									 int8_t const * const path,
									 int8_t const * const argv[],
									 int8_t const * const environ[],
									 child_ops_t const * const ops,
									 evt_loop_t * const el,
									 void * user_data )
{
	/* 
	 * (parent) -> p2c_pipe[PIPE_WRITE_FD] -> kernel -> p2c_pipe[PIPE_READ_FD] -> (child)
	 * (parent) <- c2p_pipe[PIPE_READ_FD] <- kernel <- c2p_pipe[PIPE_WRITE_FD] <- (child)
	 *
	 */
	int p2c_pipe[2];
	int c2p_pipe[2];
	int keepfds[4];
	evt_params_t chld_params;
	static aiofd_ops_t aiofd_ops =
	{
		&child_aiofd_read_fn,
		&child_aiofd_write_fn,
		&child_aiofd_error_fn
	};

	CHECK_PTR_RET( child, FALSE );
	CHECK_PTR_RET( path, FALSE );
	CHECK_PTR_RET( argv, FALSE );
	CHECK_PTR_RET( environ, FALSE );
	CHECK_PTR_RET( ops, FALSE );

	MEMSET( (void*)child, 0, sizeof(child_process_t) );

	/* store the user context */
	child->user_data = user_data;

	/* copy the ops into place */
	MEMCPY( (void*)&(child->ops), ops, sizeof(child_ops_t) );

	/* set up the pipes */
	if ( safe_pipe(c2p_pipe) == -1 )
	{
		return FALSE;
	}
	if ( safe_pipe(p2c_pipe) == -1 )
	{
		close( c2p_pipe[PIPE_WRITE_FD] );
		close( c2p_pipe[PIPE_READ_FD] );
		return FALSE;
	}

	/* fork the child process */
	keepfds[0] = p2c_pipe[PIPE_READ_FD];
	keepfds[1] = p2c_pipe[PIPE_WRITE_FD];
	keepfds[2] = c2p_pipe[PIPE_READ_FD];
	keepfds[3] = c2p_pipe[PIPE_WRITE_FD];
	child->pid = safe_fork( keepfds, 4 );
	if ( child->pid == -1 )
	{
		close( p2c_pipe[PIPE_WRITE_FD] );
		close( p2c_pipe[PIPE_READ_FD] );
		close( c2p_pipe[PIPE_WRITE_FD] );
		close( c2p_pipe[PIPE_READ_FD] );
		return FALSE;
	}

	if ( child->pid == 0 )
	{
		/* CHILD PROCESS */

		/* close the child's STDIN and STDOUT file descriptors */
		close( STDIN_FILENO );
		close( STDOUT_FILENO );

		/* close the read side of the child-to-parent pipe */
		close( c2p_pipe[PIPE_READ_FD] );

		/* close the write side of the parent-to-child pipe */
		close( p2c_pipe[PIPE_WRITE_FD] );

		/* make our STDIN be the read side of the parent-to-child pipe */
		dup2( p2c_pipe[PIPE_READ_FD], STDIN_FILENO );

		/* close the pipe fd which doesn't close the pipe, it just leaves the
		 * only copy of the fd bound to STDIN */
		close( p2c_pipe[PIPE_READ_FD] );

		/* make our STDOUT be the write side of the child-to-parent pipe */
		dup2( c2p_pipe[PIPE_WRITE_FD], STDOUT_FILENO );

		/* close the pipe fd which doesn't close the pipe, it just leaves the
		 * only copy of the fd bound to STDOUT */
		close( c2p_pipe[PIPE_WRITE_FD] );

		/* now replace the process image */
		execve( path, (char ** const)argv, (char ** const)environ );
		exit( 127 );
	}

	/* PARENT PROCESS */

	/* close the read side of the parent-to-child pipe */
	close( p2c_pipe[PIPE_READ_FD] );

	/* close the write side of the child-to-parent pipe */
	close( p2c_pipe[PIPE_WRITE_FD] );

	/* initialize our SIGCHLD handler */
	MEMSET( &chld_params, 0, sizeof(evt_params_t) );
	chld_params.child_params.pid = child->pid;
	chld_params.child_params.trace = FALSE;
	evt_initialize_event_handler( &(child->sigchld), EVT_CHILD, 
								  &chld_params, &sigchld_cb, (void*)child );

	/* start the handler */
	evt_start_event_handler( el, &(child->sigchld) );

	/* initialize the aiofd to monitor the parent side of the pipes */
	aiofd_initialize( &(child->aiofd), p2c_pipe[PIPE_WRITE_FD], c2p_pipe[PIPE_READ_FD], &aiofd_ops, el, (void*)child );

	/* start listening for incoming data from the child */
	aiofd_enable_read_evt( &(child->aiofd), TRUE );

	return TRUE;
}

child_process_t * child_process_new( int8_t const * const path,
									 int8_t const * const argv[],
									 int8_t const * const environ[],
									 child_ops_t const * const ops,
									 evt_loop_t * const el,
									 void * user_data )
{
	child_process_t * child = NULL;

	CHECK_PTR_RET( path, NULL );
	CHECK_PTR_RET( argv, NULL );
	CHECK_PTR_RET( environ, NULL );
	CHECK_PTR_RET( ops, NULL );

	/* allocate a child struct */
	child = (child_process_t*)CALLOC( 1, sizeof(child_process_t) );
	CHECK_PTR_RET_MSG( child, NULL, "failed to allocate child_process_t\n" );

	if ( child_process_initialize( child, path, argv, environ, ops, el, user_data ) == FALSE )
	{
		FREE( child );
		return NULL;
	}
	
	return child;
}

static void child_process_deinitialize( child_process_t * const child, int wait )
{
	CHECK_PTR( child );

	/* shut down the aiofd */
	aiofd_deinitialize( &(child->aiofd) );

	/* wait if the client wants to */
	while ( wait && (child->exited == FALSE) )
	{
		sleep( 1 );
	}

	/* shut down the child event handler */
	evt_deinitialize_event_handler( &(child->sigchld) );

	/* close the file descriptors */
	if ( child->aiofd.rfd >= 0 )
		close( child->aiofd.rfd );
	if ( child->aiofd.wfd >= 0 )
		close( child->aiofd.wfd );
}

void child_process_delete( void * cp, int wait )
{
	child_process_t * child = (child_process_t *)cp;
	CHECK_PTR( child );

	/* clean everything up */
	child_process_deinitialize( child, wait );

	FREE( (void*)cp );
}

pid_t child_process_get_pid( child_process_t * const cp )
{
	CHECK_PTR_RET( cp, 0 );
	return cp->pid;
}

int32_t child_process_read( child_process_t * const cp, 
							uint8_t * const buffer, 
							int32_t const n )
{
	CHECK_PTR_RET( cp, FALSE );
	CHECK_PTR_RET(buffer, 0);
	CHECK_RET(n > 0, 0);
	return aiofd_read( &(cp->aiofd), buffer, n );
}

int child_process_write( child_process_t * const cp, 
						 uint8_t const * const buffer, 
					 	 size_t const n )
{
	CHECK_PTR_RET( cp, FALSE );
	return aiofd_write( &(cp->aiofd), buffer, n );
}

int child_process_writev( child_process_t * const cp,
						  struct iovec * iov,
						  size_t iovcnt )
{
	CHECK_PTR_RET( cp, FALSE );
	return aiofd_writev( &(cp->aiofd), iov, iovcnt );
}

int child_process_flush( child_process_t * const cp )
{
	CHECK_PTR_RET( cp, FALSE );
	return aiofd_flush( &(cp->aiofd) );
}

#if defined(UNIT_TESTING)
void child_process_set_fail_fork( int fail )
{
	fail_fork = fail;
}
void child_process_set_num_good_pipes( int ngood )
{
	good_pipes = ngood;
}
#endif


