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

typedef struct child_process_s child_process_t;

typedef struct child_ops_s 
{
	int (*exit_fn)( child_process_t * const cp, void * user_data );
	int32_t (*read_fn)( child_process_t * const cp, size_t nread, void * user_data );
	int32_t (*write_fn)( child_process_t * const cp, uint8_t const * const buffer, void * user_data );

} child_ops_t;

/* create/destroy a child process */
child_process_t * child_process_new( int8_t const * const path,
									 int8_t const * const argv[],
									 int8_t const * const environ[],
									 child_ops_t const * const ops,
									 evt_loop_t * const el,
									 void * user_data );
void child_process_delete( void * cp, int wait );

/* read data from the child process */
int32_t child_process_read( child_process_t * const cp, 
							uint8_t * const buffer, 
							int32_t const n );

/* write data to the child process */
int child_process_write( child_process_t * const cp, 
						 uint8_t const * const buffer, 
					 	 size_t const n );

/* write iovec to the child process */
int child_process_writev( child_process_t * const cp,
						  struct iovec * iov,
						  size_t iovcnt );

/* flush the child process output */
int child_process_flush( child_process_t * const cp );

#endif/*__CHILD_H__*/

