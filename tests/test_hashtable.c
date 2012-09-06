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
#include <cutil/hashtable.h>

#include "test_macros.h"
#include "test_flags.h"

#define REPEAT (128)
#define SIZEMAX (128)
#define MULTIPLE (8)

extern void test_hashtable_private_functions( void );

static uint_t hash_fn( void const * const key )
{
	uint_t const hash = (uint_t)key;
	return ((hash == 0) ? hash + 1 : hash);
}

static int match_fn( void const * const l, void const * const r )
{
	return ((uint_t)l == (uint_t)r);
}

static void test_hashtable_newdel( void )
{
	int i;
	uint32_t size;
	ht_t * ht;

	for ( i = 0; i < REPEAT; i++ )
	{
		ht = NULL;
		size = (rand() % SIZEMAX);
		ht = ht_new( size, &hash_fn, &match_fn, NULL );

		CU_ASSERT_PTR_NOT_NULL_FATAL( ht );
		CU_ASSERT_EQUAL( ht->hfn, &hash_fn );
		CU_ASSERT_EQUAL( ht->mfn, &match_fn );
		CU_ASSERT_EQUAL( ht->dfn, NULL );
		CU_ASSERT_EQUAL( ht->initial, size );
		CU_ASSERT_EQUAL( ht->count, 0 );
		CU_ASSERT_NOT_EQUAL( ht->size, 0 );
		CU_ASSERT_PTR_NOT_NULL( ht->lists );

		ht_delete( ht );
	}
}

static void test_hashtable_initdeinit( void )
{
	int i;
	uint32_t size;
	ht_t ht;

	for ( i = 0; i < REPEAT; i++ )
	{
		MEMSET( &ht, 0, sizeof(ht_t) );
		size = (rand() % SIZEMAX);
		CU_ASSERT_TRUE( ht_initialize( &ht, size, &hash_fn, &match_fn, NULL ) );

		CU_ASSERT_EQUAL( ht.hfn, &hash_fn );
		CU_ASSERT_EQUAL( ht.mfn, &match_fn );
		CU_ASSERT_EQUAL( ht.dfn, NULL );
		CU_ASSERT_EQUAL( ht.initial, size );
		CU_ASSERT_EQUAL( ht.count, 0 );
		CU_ASSERT_NOT_EQUAL( ht.size, 0 );
		CU_ASSERT_PTR_NOT_NULL( ht.lists );

		ht_deinitialize( &ht );
	}
}

static void test_hashtable_fail_alloc( void )
{
	ht_t * ht;
	fail_alloc = TRUE;
	CU_ASSERT_PTR_NULL( ht_new( rand(), &hash_fn, &match_fn, NULL ) );
	fail_alloc = FALSE;
}

static int init_hashtable_suite( void )
{
	srand(0xDEADBEEF);
	reset_test_flags();
	return 0;
}

static int deinit_hashtable_suite( void )
{
	reset_test_flags();
	return 0;
}

static CU_pSuite add_hashtable_tests( CU_pSuite pSuite )
{
	ADD_TEST( "new/delete of hashtable", test_hashtable_newdel );
	ADD_TEST( "init/deinit of hashtable", test_hashtable_initdeinit );
	ADD_TEST( "hashtable new/del fail alloc", test_hashtable_fail_alloc );

	ADD_TEST( "hashtable private functions", test_hashtable_private_functions );
	return pSuite;
}

CU_pSuite add_hashtable_test_suite()
{
	CU_pSuite pSuite = NULL;

	/* add the suite to the registry */
	pSuite = CU_add_suite("Hashtable Tests", init_hashtable_suite, deinit_hashtable_suite);
	CHECK_PTR_RET( pSuite, NULL );

	/* add in hashtable specific tests */
	CHECK_PTR_RET( add_hashtable_tests( pSuite ), NULL );

	return pSuite;
}

