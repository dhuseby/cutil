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
#include <cutil/pair.h>

#include "test_macros.h"

extern int fail_alloc;

int8_t const * const first = "first";
int8_t const * const second = "second";

void test_pair_newdel( void )
{
	int i;
	pair_t * pair;

	for ( i = 0; i < 1024; i++ )
	{
		pair = pair_new( (void*)first, (void*)second );

		CU_ASSERT_PTR_NOT_NULL( pair );
		CU_ASSERT_PTR_NOT_NULL( pair_first( pair ) );
		CU_ASSERT_PTR_NOT_NULL( pair_second( pair ) );
		CU_ASSERT_EQUAL( pair_first( pair ), first );
		CU_ASSERT_EQUAL( pair_second( pair ), second );

		pair_delete( (void*)pair );
	}
}

void test_pair_nulls( void )
{
	int i;
	pair_t * pair;

	for ( i = 0; i < 1024; i++ )
	{
		pair = pair_new( NULL, NULL );

		CU_ASSERT_PTR_NOT_NULL( pair );
		CU_ASSERT_PTR_NULL( pair_first( pair ) );
		CU_ASSERT_PTR_NULL( pair_second( pair ) );
		CU_ASSERT_EQUAL( pair_first( pair ), NULL );
		CU_ASSERT_EQUAL( pair_second( pair ), NULL );

		pair_delete( (void*)pair );
	}
}

void test_pair_fail_alloc( void )
{
	fail_alloc = TRUE;
	CU_ASSERT_PTR_NULL( pair_new( NULL, NULL ) );
	fail_alloc = FALSE;
}

void test_pair_delete_null( void )
{
	pair_delete(NULL);
}

void test_pair_accessor_prereqs( void )
{
	CU_ASSERT_PTR_NULL( pair_first( NULL ) );
	CU_ASSERT_PTR_NULL( pair_second( NULL ) );
}

static int init_pair_suite( void )
{
	srand(0xDEADBEEF);
	return 0;
}

static int deinit_pair_suite( void )
{
	return 0;
}

static CU_pSuite add_pair_tests( CU_pSuite pSuite )
{
	ADD_TEST( "new/delete of pair",		test_pair_newdel );
	ADD_TEST( "pair with nulls test",	test_pair_nulls );
	ADD_TEST( "pair fail alloc",		test_pair_fail_alloc );
	ADD_TEST( "pair delete null",		test_pair_delete_null );
	ADD_TEST( "pair accessor pre-reqs", test_pair_accessor_prereqs );
	
	return pSuite;
}

CU_pSuite add_pair_test_suite()
{
	CU_pSuite pSuite = NULL;

	/* add the suite to the registry */
	pSuite = CU_add_suite("Pair Tests", init_pair_suite, deinit_pair_suite);
	CHECK_PTR_RET( pSuite, NULL );

	/* add in pair specific tests */
	CHECK_PTR_RET( add_pair_tests( pSuite ), NULL );

	return pSuite;
}

