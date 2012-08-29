/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
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

#ifndef __BUFFER_H__
#define __BUFFER_H__

#include <stdint.h>
#include <sys/uio.h>

/* opaque handle for the buffer */
typedef struct iovec buffer_t;

buffer_t * buffer_new( void * p, size_t len );
void buffer_delete( void * p );
int buffer_initialize( buffer_t * const b, void * p, size_t len );
void buffer_deinitialize( buffer_t * const b );

/* grow a buffer */
int buffer_append( buffer_t * const b, void * p, size_t len );

#endif/*__BUFFER_H__*/

