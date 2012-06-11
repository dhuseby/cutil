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

#include <stdint.h>

#include <CUnit/Basic.h>

#include <cutil/debug.h>
#include <cutil/macros.h>
#include <cutil/buffer.h>

#include "test_buffer.h"

int8_t const * const buf = "blah";
size_t const size = 5;

void test_buffer_newdel( void )
{
	int i;
	buffer_t * b;

	for ( i = 0; i < 1024; i++ )
	{
		b = buffer_new( (void*)buf, size, TRUE );

		CU_ASSERT_PTR_NOT_NULL( b );

		buffer_delete( (void*)b );
	}
}

static int init_buffer_suite( void )
{
	srand(0xDEADBEEF);
	return 0;
}

static int deinit_buffer_suite( void )
{
	return 0;
}

static CU_pSuite add_buffer_tests( CU_pSuite pSuite )
{
	CHECK_PTR_RET( CU_add_test( pSuite, "new/delete of buffer", test_buffer_newdel), NULL );
	
	return pSuite;
}

CU_pSuite add_buffer_test_suite()
{
	CU_pSuite pSuite = NULL;

	/* add the suite to the registry */
	pSuite = CU_add_suite("Buffer Tests", init_buffer_suite, deinit_buffer_suite);
	CHECK_PTR_RET( pSuite, NULL );

	/* add in buffer specific tests */
	CHECK_PTR_RET( add_buffer_tests( pSuite ), NULL );

	return pSuite;
}

