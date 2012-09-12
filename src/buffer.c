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

#if defined(UNIT_TESTING)
#include "test_flags.h"
int fail_alloc_bak = FALSE;
#endif

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
#if defined(UNIT_TESTING)
	CHECK_RET( !fail_buffer_init, FALSE );
#endif
	CHECK_PTR_RET( b, FALSE );

	if ( p == NULL )
	{
#if defined(UNIT_TESTING)
		if ( fail_buffer_init_alloc )
		{
			fail_alloc_bak = fail_alloc;
			fail_alloc = TRUE;
		}
#endif
		/* allocating a new buffer, so allocate the buffer struct and
		 * the data area just after it */
		b->iov_base = CALLOC( len, sizeof(uint8_t) );

#if defined(UNIT_TESTING)
		if ( fail_buffer_init_alloc )
		{
			fail_alloc = fail_alloc_bak;
		}
#endif

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

int buffer_deinitialize( buffer_t * const b )
{
#if defined(UNIT_TESTING)
	CHECK_RET( !fail_buffer_deinit, FALSE );
#endif
	CHECK_PTR_RET( b, FALSE );
	FREE( b->iov_base );
	b->iov_base = NULL;
	b->iov_len = 0;
	return TRUE;
}

void* buffer_append( buffer_t * const b, void * p, size_t len )
{
	void * new_memory = NULL;
	CHECK_PTR_RET( b, NULL );
	CHECK_RET( len > 0, NULL );

	/* make the buffer bigger */
	new_memory = REALLOC( b->iov_base, b->iov_len + len );
	CHECK_PTR_RET( new_memory, NULL );

	/* realloc succeeded so overwrite the old pointer */	
	b->iov_base = new_memory;

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

	/* return the pointer to the first byte of the new part of memory so that
	 * it makes it easy to read data into it */
	return (b->iov_base + b->iov_len);
}

#if defined(UNIT_TESTING)

#include <CUnit/Basic.h>

void test_buffer_private_functions( void )
{
}

#endif

