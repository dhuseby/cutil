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

/*
 * struct iovec {
 *     void *iov_base;
 *     size_t iov_len;
 * }
 */


buffer_t * buffer_new( void * p, size_t len )
{
	buffer_t * b = NULL;

	/* allcoate the buffer struct */
	b = CALLOC( 1, sizeof(buffer_t) );
	CHECK_PTR_RET( b, NULL );

	if ( !buffer_initialize( b, p, len ) )
	{
		FREE( b );
		return NULL;
	}

	return b;
}

void buffer_delete( void * b )
{
	buffer_t * buf = (buffer_t*)b;
	CHECK_PTR( buf );
	buffer_deinitialize( buf );
	FREE( buf );
}

int buffer_initialize( buffer_t * const b, void * p, size_t len )
{
	CHECK_PTR_RET( b, FALSE );

	if ( p == NULL )
	{
		/* allocating a new buffer, so allocate the buffer struct and
		 * the data area just after it */
		b->iov_base = CALLOC( len, sizeof(uint8_t) );
		CHECK_PTR_RET( b->iov_base, FALSE );
	}
	else
	{
		/* take ownership */
		b->iov_base = p;
	}

	b->iov_len = len;

	return TRUE;
}

void buffer_deinitialize( buffer_t * const b )
{
	CHECK_PTR( b );
	FREE( b->iov_base );
	b->iov_base = NULL;
	b->iov_len = 0;
}

int buffer_append( buffer_t * const b, void * p, size_t len )
{
	CHECK_PTR_RET( b, FALSE );

	/* make the buffer bigger */
	b->iov_base = REALLOC( b->iov_base, b->iov_len + len );
	CHECK_PTR_RET( b->iov_base, FALSE );

	if ( p != NULL )
	{
		/* copy the additional data into the buffer */
		MEMCPY( (b->iov_base + b->iov_len), p, len );
	}
	else
	{
		/* zero out the new part of the buffer */
		MEMSET( (b->iov_base + b->iov_len), 0, len );
	}

	/* update the buffer length */
	b->iov_len += len;

	return TRUE;
}


