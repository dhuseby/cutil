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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>

#include "debug.h"
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
    if( chdir( ((root_dir == NULL) ? "/" : root_dir) ) < 0)
    {
        /* log the failure and exit */
        exit(EXIT_FAILURE);
    }
    
    /* close standard file descriptors */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

