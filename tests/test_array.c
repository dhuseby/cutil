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

#define REPEAT (128)
#define SIZEMAX (128)
#define MULTIPLE (8)

static void test_array_newdel( void )
{
	int i;
	uint32_t size;
	array_t * arr;

	for ( i = 0; i < REPEAT; i++ )
	{
		arr = NULL;
		size = (rand() % SIZEMAX);
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

	for ( i = 0; i < REPEAT; i++ )
	{
		MEMSET( &arr, 0, sizeof(array_t) );
		size = (rand() % SIZEMAX);
		array_initialize( &arr, size, NULL );

		CU_ASSERT_EQUAL( array_size( &arr ), 0 );
		CU_ASSERT_EQUAL( arr.buffer_size, size );
		CU_ASSERT_EQUAL( arr.pfn, NULL );

		array_deinitialize( &arr );
	}
}

static void test_array_static_grow( void )
{
	int i, j;
	uint32_t size, multiple;
	array_t arr;

	for ( i = 0; i < 8; i++ )
	{
		MEMSET( &arr, 0, sizeof(array_t) );
		size = (rand() % SIZEMAX);
		array_initialize( &arr, size, NULL );

		CU_ASSERT_EQUAL( array_size( &arr ), 0 );
		CU_ASSERT_EQUAL( arr.buffer_size, size );
		CU_ASSERT_EQUAL( arr.pfn, NULL );

		for ( j = 0; j < 8; j++ )
		{
			array_force_grow( &arr );
		}

		array_deinitialize( &arr );
	}
}

static void test_array_dynamic_grow( void )
{
	int i, j;
	uint32_t size, multiple;
	array_t *arr;

	for ( i = 0; i < 8; i++ )
	{
		size = (rand() % SIZEMAX);
		arr = array_new( size, NULL );

		CU_ASSERT_PTR_NOT_NULL( arr );
		CU_ASSERT_EQUAL( array_size( arr ), 0 );
		CU_ASSERT_EQUAL( arr->buffer_size, size );
		CU_ASSERT_EQUAL( arr->pfn, NULL );

		for ( j = 0; j < 8; j++ )
		{
			array_force_grow( arr );
		}

		array_delete( arr );
		arr = NULL;
	}
}

static void test_array_empty_iterator( void )
{
	int i;
	uint32_t size;
	array_t arr;
	array_itr_t itr;

	for ( i = 0; i < REPEAT; i++ )
	{
		MEMSET( &arr, 0, sizeof(array_t) );
		size = (rand() % SIZEMAX);
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
		itr = array_itr_rprev( &arr, itr );
		CU_ASSERT_EQUAL( itr, array_itr_begin( &arr ) );
		CU_ASSERT_EQUAL( itr, array_itr_end( &arr ) );
		itr = array_itr_rnext( &arr, itr );
		CU_ASSERT_EQUAL( itr, array_itr_begin( &arr ) );
		CU_ASSERT_EQUAL( itr, array_itr_end( &arr ) );

		itr = array_itr_rend( &arr );
		CU_ASSERT_EQUAL( itr, array_itr_end( &arr ) );
		itr = array_itr_rnext( &arr, itr );
		CU_ASSERT_EQUAL( itr, array_itr_begin( &arr ) );
		CU_ASSERT_EQUAL( itr, array_itr_end( &arr ) );
		itr = array_itr_rprev( &arr, itr );
		CU_ASSERT_EQUAL( itr, array_itr_begin( &arr ) );
		CU_ASSERT_EQUAL( itr, array_itr_end( &arr ) );

		array_deinitialize( &arr );
	}
}

static void test_array_push_head_1( void )
{
	array_t arr;
	MEMSET(&arr, 0, sizeof(array_t));
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
	array_deinitialize( &arr );
}

static void test_array_push_head( void )
{
	int i, j;
	uint32_t size;
	uint32_t multiple;
	array_t arr;

	for ( i = 0; i < REPEAT; i++ )
	{
		MEMSET( &arr, 0, sizeof(array_t) );
		size = (rand() % SIZEMAX);
		multiple = (rand() % MULTIPLE);
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

static void test_array_push_tail_1( void )
{
	array_t arr;
	array_initialize( &arr, 1, NULL );
	array_push_tail( &arr, (void*)1 );
	CU_ASSERT_EQUAL( array_size( &arr ), 1 );
	array_push_tail( &arr, (void*)2 );
	CU_ASSERT_EQUAL( array_size( &arr ), 2 );
	array_push_tail( &arr, (void*)3 );
	CU_ASSERT_EQUAL( array_size( &arr ), 3 );
	array_push_tail( &arr, (void*)4 );
	CU_ASSERT_EQUAL( array_size( &arr ), 4 );
	array_push_tail( &arr, (void*)5 );
	CU_ASSERT_EQUAL( array_size( &arr ), 5 );
	array_deinitialize( &arr );
}

static void test_array_push_tail( void )
{
	int i, j;
	uint32_t size;
	uint32_t multiple;
	array_t arr;
	array_itr_t itr;

	for ( i = 0; i < REPEAT; i++ )
	{
		MEMSET( &arr, 0, sizeof(array_t) );
		size = (rand() % SIZEMAX);
		multiple = (rand() % MULTIPLE);
		array_initialize( &arr, size, NULL );

		/* push integers to the tail */
		for ( j = 0; j < (size * multiple); j++ )
		{
			array_push_tail( &arr, (void*)j );
		}

		/* walk the array to make sure the values are what we expect */
		itr = array_itr_begin( &arr );
		j = 0;
		for ( ; itr != array_itr_end( &arr ); itr = array_itr_next( &arr, itr ) )
		{
			CU_ASSERT_EQUAL( (int)array_itr_get( &arr, itr ), j );
			j++;
		}

		CU_ASSERT_EQUAL( array_size( &arr ), (size * multiple) );
		CU_ASSERT_EQUAL( arr.pfn, NULL );

		array_deinitialize( &arr );
	}
}

static void test_array_push_dynamic( void )
{
	int i, j;
	void * p;
	void ** t;
	uint32_t size;
	uint32_t multiple;
	array_t arr;
	array_itr_t itr;

	for ( i = 0; i < REPEAT; i++ )
	{
		MEMSET( &arr, 0, sizeof(array_t) );
		size = (rand() % SIZEMAX);
		multiple = (rand() % MULTIPLE);
		array_initialize( &arr, size, FREE );

		for ( j = 0; j < (size * multiple); j++ )
		{
			p = CALLOC( (rand() % SIZEMAX) + 1, sizeof(uint8_t) );
			array_push_tail( &arr, p );
			CU_ASSERT_EQUAL( array_size( &arr ), (j + 1) );
		}

		CU_ASSERT_EQUAL( array_size( &arr ), (size * multiple) );
		CU_ASSERT_EQUAL( arr.pfn, FREE );

		array_deinitialize( &arr );
	}
}

static void test_array_push_zero_initial_size( void )
{
	array_t arr;
	array_initialize( &arr, 0, NULL );
	CU_ASSERT_EQUAL( array_size( &arr ), 0 );
	CU_ASSERT_EQUAL( arr.pfn, NULL );
	array_push_tail( &arr, (void*)0 );
	CU_ASSERT_EQUAL( array_size( &arr ), 1 );
	array_deinitialize( &arr );
}

static void test_array_pop_head_static( void )
{
	int i, j;
	uint32_t size;
	uint32_t multiple;
	array_t arr;
	array_itr_t itr;

	size = (rand() % SIZEMAX);
	array_initialize( &arr, size, NULL );
	multiple = (rand() % MULTIPLE);
	for( i = 0; i < (size * multiple); i++ )
	{
		array_push_tail( &arr, (void*)i );
		CU_ASSERT_EQUAL( array_size( &arr ), (i + 1) );
	}

	CU_ASSERT_EQUAL( array_size( &arr ), (size * multiple) );

	/* walk the array to make sure the values are what we expect */
	itr = array_itr_begin( &arr );
	i = 0;
	for ( ; itr != array_itr_end( &arr ); itr = array_itr_next( &arr, itr ) )
	{
		CU_ASSERT_EQUAL( (int)array_itr_get( &arr, itr ), i );
		i++;
	}

	itr = array_itr_rbegin( &arr );
	i = ((size * multiple) - 1);
	for ( ; itr != array_itr_rend( &arr ); itr = array_itr_rnext( &arr, itr ) )
	{
		CU_ASSERT_EQUAL( (int)array_itr_get( &arr, itr ), i );
		i--;
	}

	for ( i = 0; i < (size * multiple); i++ )
	{
		j = (int)array_pop_head( &arr );
		CU_ASSERT_EQUAL( j, i );
	}

	CU_ASSERT_EQUAL( array_size( &arr ), 0 );
	array_deinitialize( &arr );
}

static void test_array_pop_tail_static( void )
{
	int i, j;
	uint32_t size;
	uint32_t multiple;
	array_t arr;
	array_itr_t itr;

	size = (rand() % SIZEMAX);
	array_initialize( &arr, size, NULL );
	multiple = (rand() % MULTIPLE);
	for( i = 0; i < (size * multiple); i++ )
	{
		array_push_head( &arr, (void*)i );
		CU_ASSERT_EQUAL( array_size( &arr ), (i + 1) );
		CU_ASSERT_EQUAL( (int)array_get_head(&arr), i );
		if ( (int)array_get_head(&arr) != i )
		{
			printf( "%d == %d (size: %u, bufsize: %u\n", (int)array_get_head(&arr), i, size, arr.buffer_size );
		}

		if ( i > 0 )
		{
			/* make sure the previous item in the array is what we expect */
			itr = array_itr_begin( &arr );
			itr = array_itr_next( &arr, itr );
			/*CU_ASSERT_EQUAL( (int)array_itr_get(&arr, itr), (i - 1) );*/
			if ( (i - 1) != (int)array_itr_get(&arr, itr) )
			{
				itr = array_itr_begin( &arr );
				itr = array_itr_next( &arr, itr );
				j = (int)array_itr_get( &arr, itr );
			}
		}
	}
	
	CU_ASSERT_EQUAL( array_size( &arr ), (size * multiple) );
	/* walk the array to make sure the values are what we expect */
	itr = array_itr_begin( &arr );
	i = 0;
	for ( ; itr != array_itr_end( &arr ); itr = array_itr_next( &arr, itr ) )
	{
		printf("%d\n", (int)array_itr_get( &arr, itr ) );
		CU_ASSERT_EQUAL( (int)array_itr_get( &arr, itr ), i );
		i++;
	}

	itr = array_itr_rbegin( &arr );
	i = ((size * multiple) - 1);
	for ( ; itr != array_itr_rend( &arr ); itr = array_itr_rnext( &arr, itr ) )
	{
		CU_ASSERT_EQUAL( (int)array_itr_get( &arr, itr ), i );
		i--;
	}

	for ( i = ((size * multiple) - 1); i >= 0; i-- )
	{
		j = (int)array_pop_tail( &arr );
		CU_ASSERT_EQUAL( j, i );
	}

	CU_ASSERT_EQUAL( array_size( &arr ), 0 );
	array_deinitialize( &arr );
}

static void test_array_clear( void )
{
	int i;
	uint32_t size;
	uint32_t multiple;
	array_t arr;

	size = (rand() % SIZEMAX);
	array_initialize( &arr, size, NULL );
	multiple = (rand() % MULTIPLE);
	for( i = 0; i < (size * multiple); i++ )
	{
		array_push_head( &arr, (void*)i );
		CU_ASSERT_EQUAL( array_size( &arr ), (i + 1) );
	}

	CU_ASSERT_EQUAL( array_size( &arr ), (size * multiple) );

	array_clear( &arr );

	CU_ASSERT_EQUAL( array_size( &arr ), 0 );
	array_deinitialize( &arr );
}

static void test_array_new_fail( void )
{
	int i;
	uint32_t size;
	array_t * arr = NULL;

	/* turn on the flag that forces grows to fail */
	array_set_fail_grow( TRUE );

	size = (rand() % SIZEMAX);
	arr = array_new( size, NULL );

	CU_ASSERT_PTR_NULL( arr );
	CU_ASSERT_EQUAL( array_size( arr ), 0 );
	
	array_set_fail_grow( FALSE );
}

static void test_array_init_fail( void )
{
	int i;
	uint32_t size;
	array_t arr;

	/* turn on the flag that forces grows to fail */
	array_set_fail_grow( TRUE );

	MEMSET( &arr, 0, sizeof(array_t) );
	size = (rand() % SIZEMAX);
	array_initialize( &arr, size, NULL );

	CU_ASSERT_EQUAL( array_size( &arr ), 0 );

	array_set_fail_grow( FALSE );
}

static void test_array_push_fail( void )
{
	int ret;
	array_t arr;
	MEMSET(&arr, 0, sizeof(array_t));
	array_initialize( &arr, 0, NULL );

	/* turn on the flag that forces grows to fail */
	array_set_fail_grow( TRUE );

	ret = array_push_head( &arr, (void*)1 )
	CU_ASSERT_EQUAL( ret, FALSE );
	CU_ASSERT_EQUAL( array_size( &arr ), 0 );
	
	array_deinitialize( &arr );
	
	array_set_fail_grow( FALSE );
}


/*TODO:
 *	- middle itr push test
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
	CHECK_PTR_RET( CU_add_test( pSuite, "grow of static array", test_array_static_grow), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "grow of dynamic array", test_array_dynamic_grow), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "empty array itr tests", test_array_empty_iterator), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "push one head tests", test_array_push_head_1), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "push head tests", test_array_push_head), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "push one tail tests", test_array_push_tail_1), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "push tail tests", test_array_push_tail), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "push dynamic memory", test_array_push_dynamic), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "push to zero initial size", test_array_push_zero_initial_size), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "pop head static", test_array_pop_head_static), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "pop tail static", test_array_pop_tail_static), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "array clear", test_array_clear), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "array new fail", test_array_new_fail), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "array init fail", test_array_init_fail), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "array push fail", test_array_push_fail), NULL );
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

