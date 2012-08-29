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
	int single_alloc;
	size_t len;
	void * p;
};

buffer_t * buffer_new( void * p, size_t len )
{
	buffer_t * b = CALLOC( 1, sizeof(buffer_t) );
	CHECK_PTR_RET( b, NULL );

	if ( p == NULL )
	{
		/* allocating a new buffer, so allocate the buffer struct and
		 * the data area just after it */
		b = CALLOC( 1, sizeof(buffer_t) + (len * sizeof(uint8_t)) );
		CHECK_PTR_RET( b, NULL );
		b->p = (void*)b + sizeof(buffer_t);
		b->single_alloc = TRUE;
	}
	else
	{
		b = CALLOC( 1, sizeof(buffer_t) );
		CHECK_PTR_RET( b, NULL );
		b->p = p;
	}

	b->len = len;

	return b;
}

void buffer_delete( void * b )
{
	buffer_t * buf = (buffer_t*)b;
	CHECK_PTR( buf );

	if ( !buf->single_alloc )
	{
		FREE( buf->p );
	}

	FREE( buf );
}

void * buffer_dref( buffer_t * const b )
{
	CHECK_PTR_RET( b, NULL );
	CHECK_RET( b->len > 0, NULL );
	return b->p;
}

int buffer_append( buffer_t * const b, void * p, size_t len )
{
	CHECK_PTR_RET( b, FALSE );
	CHECK_PTR_RET( p, FALSE );
	CHECK_RET( len > 0, FALSE );

	/* make the buffer bigger */
	b->p = REALLOC( b->p, b->len + len );
	CHECK_PTR_RET( b->p, FALSE );

	/* copy the data over */
	MEMCPY( (b->p + b->len), p, len );

	return TRUE;
}

/* initialize a zero copy array of iovec structs from an array of buffers */
int buffer_iovec( buffer_t * const b, int nbufs, struct iovec ** iovs )
{
	int i, j;
	int niovs = 0;
	CHECK_PTR_RET( b, 0 );
	CHECK_PTR_RET( iovs, 0 );

	/* count the number of valid buffers */
	for ( i = 0; i < nbufs; i++ )
	{
		if ( (b[i].p != NULL) && (b[i].len > 0) )
			niovs++;
	}

	(*iovs) = CALLOC( niovs, sizeof(struct iovec) );
	CHECK_PTR_RET( (*iovs), 0 );

	for ( i = 0, j = 0; i < nbufs; i++ )
	{
		if ( (b[i].p != NULL) && (b[i].len > 0) )
		{
			(*iovs)[j].iov_base = b[i].p;
			(*iovs)[j].iov_len = b[i].len;
			j++;
		}
	}

	ASSERT( j == niovs );

	return niovs;
}


