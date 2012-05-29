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

#include "debug.h"
#include "macros.h"
#include "array.h"
#include "events.h"
#include "privileges.h"
#include "sanitize.h"
#include "aiofd.h"
#include "child.h"

struct child_process_s
{
	pid_t			pid;			/* the pid of the child process */
	child_ops_t		ops;			/* child proces callbacks */
	aiofd_t			aiofd;			/* the fd management state */
	void *			user_data;		/* passed to ops callbacks */
};

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
		if ( array_size( &(child->aiofd.wbuf) ) == 0 )
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
		WARN( "child process closed pipe\n" );

		/* return FALSE to turn off the read event processing */
		return FALSE;
	}

	if ( child->ops.read_fn != NULL )
	{
		DEBUG( "calling child process read callback\n");
		(*(child->ops.read_fn))( child, nread, child->user_data );
	}

	return TRUE;
}


static pid_t safe_fork( void )
{
	pid_t childpid;

	if ( (childpid = fork()) == -1 )
		return -1;

	/* test for parent process */
	if ( childpid != 0 )
		return childpid;

	/* this is the child process */
	sanitize_files();
	drop_privileges( TRUE );
}


static int child_process_initialize( child_process_t * const child,
									 int8_t const * const path,
									 int8_t const * const argv[],
									 int8_t const * const environ[],
									 child_ops_t const * const ops,
									 evt_loop_t * const el,
									 void * user_data )
{
	int stdin_pipe[2];
	int stdout_pipe[2];
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
	if ( pipe(stdin_pipe) == -1 )
	{
		return FALSE;
	}
	if ( pipe(stdout_pipe) == -1 )
	{
		close( stdin_pipe[1] );
		close( stdin_pipe[0] );
		return FALSE;
	}

	/* fork the child process */
	child->pid = safe_fork();
	if ( child->pid == -1 )
	{
		close( stdout_pipe[1] );
		close( stdout_pipe[0] );
		close( stdin_pipe[1] );
		close( stdin_pipe[0] );
		return FALSE;
	}

	if ( !child->pid )
	{
		/* child process */

		/* close read side of stdout pipe */
		close( stdout_pipe[0] );

		/* close write side of stdin pipe */
		close( stdin_pipe[1] );

		/* set our stdin to be the read side of the stdin pipe */
		if ( stdin_pipe[0] != STDIN_FILENO )
		{
			dup2( stdin_pipe[0], STDIN_FILENO );
			close( stdin_pipe[0] );
		}

		/* set our stdout to be the write side of the stdout pipe */
		if ( stdout_pipe[1] != STDOUT_FILENO )
		{
			dup2( stdout_pipe[1], STDOUT_FILENO );
			close( stdout_pipe[1] );
		}

		/* now replace the process image */
		execve( path, (char ** const)argv, (char ** const)environ );
		exit( 127 );
	}

	/* parent process */
	
	/* close the read side of the child's stdin pipe */
	close( stdin_pipe[0] );

	/* close the write side of the child's stdout pipe */
	close( stdout_pipe[1] );

	/* initialize the aiofd to monitor the parent side of the pipes */
	aiofd_initialize( &(child->aiofd), stdin_pipe[1], stdout_pipe[0], &aiofd_ops, el, (void*)child );

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

	child_process_initialize( child, path, argv, environ, ops, el, user_data );

	return child;
}

static void child_process_deinitialize( child_process_t * const child, int wait )
{
	int status;
	pid_t pid;
	CHECK_PTR( child );

	/* shut down the aiofd */
	aiofd_deinitialize( &(child->aiofd) );

	/* wait if the client wants to */
	if ( wait && (child->pid != -1) )
	{
		do
		{
			pid = waitpid( child->pid, &status, 0 );
		} while ( (pid == -1) && (errno == EINTR) );
	}

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


