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

static void test_array_newdel( void )
{
	int i;
	uint32_t size;
	array_t * arr;

	for ( i = 0; i < 1024; i++ )
	{
		arr = NULL;
		size = (rand() % 1024);
		arr = array_new( size, NULL );

		CU_ASSERT_PTR_NOT_NULL( arr );
		CU_ASSERT_EQUAL( array_size( arr ), 0 );
		CU_ASSERT_EQUAL( arr->buffer_size, size );
		CU_ASSERT_EQUAL( arr->pfn, NULL );

		array_delete( arr );
	}
}

static void test_array_initdeinit( void )
{
	int i;
	uint32_t size;
	array_t arr;

	for ( i = 0; i < 1024; i++ )
	{
		MEMSET( &arr, 0, sizeof(array_t) );
		size = (rand() % 1024);
		array_initialize( &arr, size, NULL );

		CU_ASSERT_EQUAL( array_size( &arr ), 0 );
		CU_ASSERT_EQUAL( arr.buffer_size, size );
		CU_ASSERT_EQUAL( arr.pfn, NULL );

		array_deinitialize( &arr );
	}
}

static void test_array_empty_iterator( void )
{
	int i;
	uint32_t size;
	array_t arr;
	array_itr_t itr;

	for ( i = 0; i < 1024; i++ )
	{
		MEMSET( &arr, 0, sizeof(array_t) );
		size = (rand() % 1024);
		array_initialize( &arr, size, NULL );

		itr = array_itr_begin( &arr );
		CU_ASSERT_EQUAL( itr, array_itr_end( &arr ) );
		itr = array_itr_end( &arr );
		CU_ASSERT_EQUAL( itr, array_itr_begin( &arr ) );

		itr = array_itr_head( &arr );
		CU_ASSERT_EQUAL( itr, array_itr_tail( &arr ) );
		itr = array_itr_tail( &arr );
		CU_ASSERT_EQUAL( itr, array_itr_head( &arr ) );

		itr = array_itr_rbegin( &arr );
		CU_ASSERT_EQUAL( itr, array_itr_rend( &arr ) );
		itr = array_itr_rend( &arr );
		CU_ASSERT_EQUAL( itr, array_itr_rbegin( &arr ) );

		itr = array_itr_begin( &arr );
		CU_ASSERT_EQUAL( itr, array_itr_end( &arr ) );
		itr = array_itr_next( &arr, itr );
		CU_ASSERT_EQUAL( itr, array_itr_begin( &arr ) );
		CU_ASSERT_EQUAL( itr, array_itr_end( &arr ) );
		itr = array_itr_prev( &arr, itr );
		CU_ASSERT_EQUAL( itr, array_itr_begin( &arr ) );
		CU_ASSERT_EQUAL( itr, array_itr_end( &arr ) );

		itr = array_itr_end( &arr );
		CU_ASSERT_EQUAL( itr, array_itr_end( &arr ) );
		itr = array_itr_prev( &arr, itr );
		CU_ASSERT_EQUAL( itr, array_itr_begin( &arr ) );
		CU_ASSERT_EQUAL( itr, array_itr_end( &arr ) );
		itr = array_itr_next( &arr, itr );
		CU_ASSERT_EQUAL( itr, array_itr_begin( &arr ) );
		CU_ASSERT_EQUAL( itr, array_itr_end( &arr ) );

		itr = array_itr_rbegin( &arr );
		CU_ASSERT_EQUAL( itr, array_itr_end( &arr ) );
		itr = array_itr_prev( &arr, itr );
		CU_ASSERT_EQUAL( itr, array_itr_begin( &arr ) );
		CU_ASSERT_EQUAL( itr, array_itr_end( &arr ) );
		itr = array_itr_next( &arr, itr );
		CU_ASSERT_EQUAL( itr, array_itr_begin( &arr ) );
		CU_ASSERT_EQUAL( itr, array_itr_end( &arr ) );

		itr = array_itr_rend( &arr );
		CU_ASSERT_EQUAL( itr, array_itr_end( &arr ) );
		itr = array_itr_next( &arr, itr );
		CU_ASSERT_EQUAL( itr, array_itr_begin( &arr ) );
		CU_ASSERT_EQUAL( itr, array_itr_end( &arr ) );
		itr = array_itr_prev( &arr, itr );
		CU_ASSERT_EQUAL( itr, array_itr_begin( &arr ) );
		CU_ASSERT_EQUAL( itr, array_itr_end( &arr ) );

		array_deinitialize( &arr );
	}
}

static void test_array_push_head_1( void )
{
	array_t arr;
	array_initialize( &arr, 1, NULL );
	array_push_head( &arr, (void*)1 );
	CU_ASSERT_EQUAL( array_size( &arr ), 1 );
	array_push_head( &arr, (void*)2 );
	CU_ASSERT_EQUAL( array_size( &arr ), 2 );
	array_push_head( &arr, (void*)3 );
	CU_ASSERT_EQUAL( array_size( &arr ), 3 );
	array_push_head( &arr, (void*)4 );
	CU_ASSERT_EQUAL( array_size( &arr ), 4 );
	array_push_head( &arr, (void*)5 );
	CU_ASSERT_EQUAL( array_size( &arr ), 5 );
}

static void test_array_push_head( void )
{
	int i, j;
	uint32_t size;
	uint32_t multiple;
	array_t arr;

	for ( i = 0; i < 1024; i++ )
	{
		MEMSET( &arr, 0, sizeof(array_t) );
		size = (rand() % 1024);
		multiple = (rand() % 8);
		array_initialize( &arr, size, NULL );

		for ( j = 0; j < (size * multiple); j++ )
		{
			array_push_head( &arr, (void*)j );
		}

		CU_ASSERT_EQUAL( array_size( &arr ), (size * multiple) );
		CU_ASSERT_EQUAL( arr.pfn, NULL );

		array_deinitialize( &arr );
	}
}

/*TODO:
 *	- non-empty array iterator test
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
	CHECK_PTR_RET( CU_add_test( pSuite, "empty array itr tests", test_array_empty_iterator), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "push one head tests", test_array_push_head_1), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "push head tests", test_array_push_head), NULL );
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

