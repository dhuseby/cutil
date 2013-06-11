/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with main.c; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "debug.h"
#include "macros.h"
#include "daemon.h"
#include "log.h"

pid_t pid, sid;

/* 
 * this function handles daemonizing the process, it follows the
 * process outlined in the linux daemon writing howto
 */
void daemonize( int8_t const * const root_dir )
{
    /* forking off the parent processes */
    pid = fork();
    if(pid < 0)
    {
        /* something failed so just close */
        exit(EXIT_FAILURE);
    }
    
    if(pid > 0)
    {
        /* close the parent process */
        exit(EXIT_SUCCESS);
    }
    
    /* we are now a parent-less child process */
    
    /* change file mode mask (umask) */
    umask(0);
    
    /* create a unique session id (SID) */
    sid = setsid();
    if(sid < 0)
    {
        /* log the failure and exit */
        exit(EXIT_FAILURE);
    }
    
    /* change the current working directory to a safe place */
    if ( root_dir == NULL )
    {
        if ( chdir( "/" ) < 0 )
            exit( EXIT_FAILURE );
    }
    else
    {
        if ( chdir( root_dir ) < 0 )
            exit( EXIT_FAILURE );
    }
    
    /* close standard file descriptors */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);

    /* NOTE: don't close STDERR because we hook that up to the logging
     * facility to make logging brain-dead simple.
     * close(STDERR_FILENO);
     */
}

int create_pid_file( int8_t const * const fpath )
{
    FILE * fpid = NULL;
    CHECK_PTR_RET( fpath, FALSE );

    fpid = fopen( fpath, "w+" );

    CHECK_PTR_RET( fpid, FALSE );

    /* write the process ID to the file */
    fprintf( fpid, "%d", getpid() );

    /* close the file and return */
    fclose( fpid );
    return TRUE;
}

int create_start_file( int8_t const * const fpath )
{
    time_t t;
    FILE * fstart = NULL;
    CHECK_PTR_RET( fpath, FALSE );

    fstart = fopen( fpath, "w+" );

    CHECK_PTR_RET( fstart, FALSE );

    /* write the process ID to the file */
    t = time( NULL );
    fprintf( fstart, "%s", asctime( localtime( &t ) ) );

    /* close the file and return */
    fclose( fstart );
    return TRUE;
}

