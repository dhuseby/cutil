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

#ifndef __CHILD_H__
#define __CHILD_H__

#include <sys/uio.h>
#include "events.h"

#define NO_ARGS(x) { x, NULL }
#define EMPTY_ENV { NULL }

typedef struct child_process_s child_process_t;

typedef struct child_ops_s 
{
    int_t (*exit_evt_fn)( child_process_t * const cp, int rpid, int rstatus, void * user_data );
    int_t (*read_evt_fn)( child_process_t * const cp, size_t nread, void * user_data );
    int_t (*write_evt_fn)( child_process_t * const cp, uint8_t const * const buffer, void * user_data );
    int_t (*error_evt_fn)( child_process_t * const cp, int err, void * user_data );

} child_ops_t;

/* create/destroy a child process */
child_process_t * child_process_new( uint8_t const * const path,
                                     uint8_t const * const argv[],
                                     uint8_t const * const environ[],
                                     child_ops_t const * const ops,
                                     evt_loop_t * const el,
                                     int_t const drop_privileges,
                                     void * user_data );
void child_process_delete( void * cp, int wait );

/* get the child process id */
pid_t child_process_get_pid( child_process_t * const cp );

/* read data from the child process */
ssize_t child_process_read( child_process_t * const cp, 
                            uint8_t * const buffer, 
                            size_t const n );

/* write data to the child process */
int_t child_process_write( child_process_t * const cp, 
                           uint8_t const * const buffer, 
                           size_t const n );

/* read from child process into data into iovec (scatter input) */
ssize_t child_process_readv( child_process_t * const cp,
                             struct iovec * iov,
                             size_t iovcnt );

/* write iovec to the child process (gather output) */
int_t child_process_writev( child_process_t * const cp,
                            struct iovec * iov,
                            size_t iovcnt );

/* flush the child process output */
int_t child_process_flush( child_process_t * const cp );

#endif/*__CHILD_H__*/

