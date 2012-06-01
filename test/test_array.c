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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include <CUnit/Basic.h>

#include <cutil/debug.h>
#include <cutil/macros.h>
#include <cutil/array.h>

#include "test_array.h"

void test_array_newdel( void )
{
	int i;
	uint32_t size;
	array_t * arr;

	for ( i = 0; i < 1024; i++ )
	{
		arr = NULL;
		size = (rand() % 1024);
		arr = array_new( size, FREE );

		CU_ASSERT_PTR_NOT_NULL( arr );
		CU_ASSERT_EQUAL( array_size( arr ), 0 );
		CU_ASSERT_EQUAL( arr->buffer_size, size );
		CU_ASSERT_EQUAL( arr->pfn, FREE );

		array_delete( arr );
	}
}

void test_array_initdeinit( void )
{
	int i;
	uint32_t size;
	array_t arr;

	for ( i = 0; i < 1024; i++ )
	{
		MEMSET( &arr, 0, sizeof(array_t) );
		size = (rand() % 1024);
		array_initialize( &arr, size, FREE );

		CU_ASSERT_EQUAL( array_size( &arr ), 0 );
		CU_ASSERT_EQUAL( arr.buffer_size, size );
		CU_ASSERT_EQUAL( arr.pfn, FREE );

		array_deinitialize( &arr );
	}
}

/*TODO:
 *	- empty array iterator test
 *	- non-empty array iterator test
 *	- head push test
 *	- tail push test
 *	- middle itr push test
 *	- head pop test
 *	- tal pop test
 *	- middle itr pop test
 *	- head get test
 *	- tail get test
 *	- middle itr get test
 *	- clear of empty array
 *	- clear of non-empty array
 *	- threading tests
 */

static int init_array_suite( void )
{
	srand(0xDEADBEEF);
	return 0;
}

static int deinit_array_suite( void )
{
	return 0;
}

static CU_pSuite add_array_tests( CU_pSuite pSuite )
{
	CHECK_PTR_RET( CU_add_test( pSuite, "new/delete of array", test_array_newdel), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "init/deinit of array", test_array_initdeinit), NULL );
	return pSuite;
}

CU_pSuite add_array_test_suite()
{
	CU_pSuite pSuite = NULL;

	/* add the suite to the registry */
	pSuite = CU_add_suite("Array Tests", init_array_suite, deinit_array_suite);
	CHECK_PTR_RET( pSuite, NULL );

	/* add in array specific tests */
	CHECK_PTR_RET( add_array_tests( pSuite ), NULL );

	return pSuite;
}

