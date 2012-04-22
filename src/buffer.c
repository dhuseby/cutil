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

#include "debug.h"
#include "macros.h"
#include "buffer.h"

struct buffer_s
{
	void * p;
	size_t size;
};

buffer_t * buffer_new( void * p, size_t size )
{
	buffer_t * b = CALLOC( 0, sizeof(buffer_t) );
	CHECK_PTR_RET( b, NULL );

	b->p = p;
	b->size = size;

	return b;
}

void buffer_delete( void * b )
{
	buffer_t * buf = (buffer_t*)b;
	CHECK_PTR( buf );

	FREE( buf );
}

