/* Copyright (c) 2012-2015 David Huseby
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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

extern ht_itr_t fake_ht_find_ret;
extern void test_hashtable_private_functions( void );

static uint_t hash_fn( void const * const key )
{
	return (uint_t)key;
}

static int_t match_fn( void const * const l, void const * const r )
{
	return (int_t)((uint_t)l == (uint_t)r);
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
	fail_alloc = TRUE;
	CU_ASSERT_PTR_NULL( ht_new( rand(), &hash_fn, &match_fn, NULL ) );
	fail_alloc = FALSE;
}

static void test_hashtable_fail_init( void )
{
	fake_ht_init = TRUE;
	fake_ht_init_ret = FALSE;
	CU_ASSERT_PTR_NULL( ht_new( rand(), &hash_fn, &match_fn, NULL ) );
	fake_ht_init = FALSE;
}

static void test_hashtable_delete_null( void )
{
	ht_delete( NULL );
}

static void test_hashtable_init_prereqs( void )
{
	ht_t ht;
	MEMSET( &ht, 0, sizeof(ht_t) );
	CU_ASSERT_FALSE( ht_initialize( NULL, 0, NULL, NULL, NULL ) );
	MEMSET( &ht, 0, sizeof(ht_t) );
	CU_ASSERT_FALSE( ht_initialize( &ht, 0, NULL, NULL, NULL ) );
	MEMSET( &ht, 0, sizeof(ht_t) );
	CU_ASSERT_FALSE( ht_initialize( &ht, 0, &hash_fn, NULL, NULL ) );
	MEMSET( &ht, 0, sizeof(ht_t) );
	CU_ASSERT_TRUE( ht_initialize( &ht, 0, &hash_fn, &match_fn, NULL ) );
	CU_ASSERT_TRUE( ht_deinitialize( &ht ) );

	fake_ht_grow = TRUE;
	fake_ht_grow_ret = FALSE;
	CU_ASSERT_FALSE( ht_initialize( &ht, 0, &hash_fn, &match_fn, NULL ) );
	fake_ht_grow = FALSE;
}

static void test_hashtable_deinit_prereqs( void )
{
	ht_t ht;
	MEMSET( &ht, 0, sizeof(ht_t) );

	CU_ASSERT_FALSE( ht_deinitialize( NULL ) );
	fake_ht_deinit = TRUE;
	fake_ht_deinit_ret = FALSE;
	CU_ASSERT_FALSE( ht_deinitialize( &ht ) );
	fake_ht_deinit = FALSE;
}

static void test_hashtable_count( void )
{
	ht_t ht;
	MEMSET( &ht, 0, sizeof(ht_t) );
	CU_ASSERT_EQUAL( ht_count( NULL ), 0 );
	CU_ASSERT_EQUAL( ht_count( &ht ), 0 );
}

static void test_hashtable_insert( void )
{
	int_t i;
	ht_t ht;
	MEMSET( &ht, 0, sizeof(ht_t) );
	CU_ASSERT_TRUE( ht_initialize( &ht, 5, &hash_fn, &match_fn, NULL ) );

	CU_ASSERT_FALSE( ht_insert( NULL, NULL ) );
	CU_ASSERT_FALSE( ht_insert( &ht, NULL ) );

	CU_ASSERT_TRUE( ht_insert( &ht, (void*)0x4 ) );
	CU_ASSERT_EQUAL( ht.count, 1 );
	CU_ASSERT_FALSE( ht_insert( &ht, (void*)0x4) );
	CU_ASSERT_EQUAL( ht.count, 1 );

	CU_ASSERT_TRUE( ht_clear( &ht ) );
	CU_ASSERT_EQUAL( ht.count, 0 );

	for ( i = 1; i < 100; i++ )
	{
		CU_ASSERT_TRUE( ht_insert( &ht, (void*)i ) );
		CU_ASSERT_EQUAL( ht.count, i );
	}

	CU_ASSERT_TRUE( ht_clear( &ht ) );
	CU_ASSERT_EQUAL( ht.count, 0 );

	fake_ht_find = TRUE;
	fake_ht_find_ret.idx = 0;
	fake_ht_find_ret.itr = -1;
	CU_ASSERT_FALSE( ht_insert( &ht, (void*)0x4 ) );
	fake_ht_find_ret.idx = -1;
	fake_ht_find_ret.itr = 0;
	CU_ASSERT_FALSE( ht_insert( &ht, (void*)0x5 ) );
	fake_ht_find_ret.itr = -1;
	fake_ht_find = FALSE;

	CU_ASSERT_TRUE( ht_clear( &ht ) );
	CU_ASSERT_EQUAL( ht.count, 0 );

	fake_ht_grow = TRUE;
	fake_ht_grow_ret = FALSE;
	for ( i = 1; i < 100; i++ )
	{
		if ( (ht.count / ht.size) > ht.limit )
		{
			CU_ASSERT_FALSE( ht_insert( &ht, (void*)i ) );
		}
		else
		{
			CU_ASSERT_TRUE( ht_insert( &ht, (void*)i ) );
		}
	}
	fake_ht_grow = FALSE;

	CU_ASSERT_TRUE( ht_clear( &ht ) );
	CU_ASSERT_EQUAL( ht.count, 0 );

	fake_list_push = TRUE;
	fake_list_push_ret = FALSE;
	CU_ASSERT_FALSE( ht_insert( &ht, (void*)0x4 ) );
	fake_list_push = FALSE;

	CU_ASSERT_TRUE( ht_deinitialize( &ht ) );
}

static void test_hashtable_clear( void )
{
	ht_t ht;
	MEMSET( &ht, 0, sizeof(ht_t) );

	CU_ASSERT_FALSE( ht_clear( NULL ) );

	fake_ht_deinit = TRUE;
	fake_ht_deinit_ret = FALSE;
	CU_ASSERT_FALSE( ht_clear( &ht ) );
	fake_ht_deinit = FALSE;

	fake_ht_init = TRUE;
	fake_ht_init_ret = FALSE;
	CU_ASSERT_FALSE( ht_clear( &ht ) );
	fake_ht_init = FALSE;
}

static void test_hashtable_find( void )
{
	ht_t ht;
	ht_itr_t end = { .idx = -1, .itr = -1 };
	MEMSET( &ht, 0, sizeof(ht_t) );
	CU_ASSERT_TRUE( ITR_EQ( ht_find( NULL, NULL ), end ) );
	CU_ASSERT_TRUE( ITR_EQ( ht_find( &ht, NULL ), end ) );
}

static void test_hashtable_remove( void )
{
	ht_t ht;
	ht_itr_t itr = { .idx = -1, .itr = -1 };
	MEMSET( &ht, 0, sizeof(ht_t) );

	CU_ASSERT_FALSE( ht_remove( NULL, itr ) );
	CU_ASSERT_FALSE( ht_remove( &ht, itr ) );
	itr.idx = 0;
	CU_ASSERT_FALSE( ht_remove( &ht, itr ) );
	itr.itr = 0;
	CU_ASSERT_FALSE( ht_remove( &ht, itr ) );
	itr.idx = -1;
	CU_ASSERT_FALSE( ht_remove( &ht, itr ) );
	itr.idx = 200;
	CU_ASSERT_FALSE( ht_remove( &ht, itr ) );

	CU_ASSERT_TRUE( ht_initialize( &ht, 5, &hash_fn, &match_fn, NULL ) );

	itr.idx = 0;
	itr.idx = 0;
	CU_ASSERT_FALSE( ht_remove( &ht, itr ) );

	CU_ASSERT_TRUE( ht_insert( &ht, (void*)0x4 ) );
	CU_ASSERT_EQUAL( ht.count, 1 );
	itr.idx = -1;
	CU_ASSERT_FALSE( ht_remove( &ht, itr ) );

	itr = ht_find( &ht, (void*)0x4 );	
	CU_ASSERT_TRUE( ht_remove( &ht, itr ) );
	CU_ASSERT_EQUAL( ht.count, 0 );

	CU_ASSERT_TRUE( ht_insert( &ht, (void*)0x4 ) );
	CU_ASSERT_EQUAL( ht.count, 1 );
	itr = ht_find( &ht, (void*)0x4 );	
	fake_list_get = TRUE;
	fake_list_get_ret = NULL;
	CU_ASSERT_FALSE( ht_remove( &ht, itr ) );
	fake_list_get = FALSE;
	CU_ASSERT_EQUAL( ht.count, 1 );
	
	CU_ASSERT_TRUE( ht_deinitialize( &ht ) );
}

static void test_hashtable_get( void )
{
	ht_t ht;
	ht_itr_t itr = { .idx = -1, .itr = -1 };
	MEMSET( &ht, 0, sizeof(ht_t) );

	CU_ASSERT_PTR_NULL( ht_get( NULL, itr ) );
	CU_ASSERT_PTR_NULL( ht_get( &ht, itr ) );
	itr.idx = 0;
	CU_ASSERT_PTR_NULL( ht_get( &ht, itr ) );
	itr.itr = 0;
	CU_ASSERT_PTR_NULL( ht_get( &ht, itr ) );
	itr.idx = -1;
	CU_ASSERT_PTR_NULL( ht_get( &ht, itr ) );
	itr.idx = 200;
	CU_ASSERT_PTR_NULL( ht_get( &ht, itr ) );

	CU_ASSERT_TRUE( ht_initialize( &ht, 5, &hash_fn, &match_fn, NULL ) );
	CU_ASSERT_TRUE( ht_insert( &ht, (void*)0x4 ) );
	CU_ASSERT_EQUAL( ht.count, 1 );
	itr = ht_find( &ht, (void*)0x4 );	
	CU_ASSERT_PTR_NOT_NULL( ht_get( &ht, itr ) );
	
	CU_ASSERT_TRUE( ht_deinitialize( &ht ) );
}

static void test_hashtable_empty_iterator( void )
{
	int i;
	uint32_t size;
	ht_t ht;
	ht_itr_t itr;

	reset_test_flags();

	for ( i = 0; i < REPEAT; i++ )
	{
		MEMSET( &ht, 0, sizeof(ht_t) );
		size = (rand() % SIZEMAX);
		CU_ASSERT_TRUE( ht_initialize( &ht, size, &hash_fn, &match_fn, NULL ) );

		itr = ht_itr_begin( &ht );
		CU_ASSERT_TRUE( ITR_EQ( itr, ht_itr_end( &ht ) ) );
		itr = ht_itr_end( &ht );
		CU_ASSERT_TRUE( ITR_EQ( itr, ht_itr_begin( &ht ) ) );

		itr = ht_itr_rbegin( &ht );
		CU_ASSERT_TRUE( ITR_EQ( itr, ht_itr_rend( &ht ) ) );
		itr = ht_itr_rend( &ht );
		CU_ASSERT_TRUE( ITR_EQ( itr, ht_itr_rbegin( &ht ) ) );

		itr = ht_itr_begin( &ht );
		CU_ASSERT_TRUE( ITR_EQ( itr, ht_itr_end( &ht ) ) );
		itr = ht_itr_next( &ht, itr );
		CU_ASSERT_TRUE( ITR_EQ( itr, ht_itr_begin( &ht ) ) );
		CU_ASSERT_TRUE( ITR_EQ( itr, ht_itr_end( &ht ) ) );
		itr = ht_itr_prev( &ht, itr );
		CU_ASSERT_TRUE( ITR_EQ( itr, ht_itr_begin( &ht ) ) );
		CU_ASSERT_TRUE( ITR_EQ( itr, ht_itr_end( &ht ) ) );

		itr = ht_itr_end( &ht );
		CU_ASSERT_TRUE( ITR_EQ( itr, ht_itr_end( &ht ) ) );
		itr = ht_itr_prev( &ht, itr );
		CU_ASSERT_TRUE( ITR_EQ( itr, ht_itr_begin( &ht ) ) );
		CU_ASSERT_TRUE( ITR_EQ( itr, ht_itr_end( &ht ) ) );
		itr = ht_itr_next( &ht, itr );
		CU_ASSERT_TRUE( ITR_EQ( itr, ht_itr_begin( &ht ) ) );
		CU_ASSERT_TRUE( ITR_EQ( itr, ht_itr_end( &ht ) ) );

		itr = ht_itr_rbegin( &ht );
		CU_ASSERT_TRUE( ITR_EQ( itr, ht_itr_end( &ht ) ) );
		itr = ht_itr_rprev( &ht, itr );
		CU_ASSERT_TRUE( ITR_EQ( itr, ht_itr_begin( &ht ) ) );
		CU_ASSERT_TRUE( ITR_EQ( itr, ht_itr_end( &ht ) ) );
		itr = ht_itr_rnext( &ht, itr );
		CU_ASSERT_TRUE( ITR_EQ( itr, ht_itr_begin( &ht ) ) );
		CU_ASSERT_TRUE( ITR_EQ( itr, ht_itr_end( &ht ) ) );

		itr = ht_itr_rend( &ht );
		CU_ASSERT_TRUE( ITR_EQ( itr, ht_itr_end( &ht ) ) );
		itr = ht_itr_rnext( &ht, itr );
		CU_ASSERT_TRUE( ITR_EQ( itr, ht_itr_begin( &ht ) ) );
		CU_ASSERT_TRUE( ITR_EQ( itr, ht_itr_end( &ht ) ) );
		itr = ht_itr_rprev( &ht, itr );
		CU_ASSERT_TRUE( ITR_EQ( itr, ht_itr_begin( &ht ) ) );
		CU_ASSERT_TRUE( ITR_EQ( itr, ht_itr_end( &ht ) ) );

		CU_ASSERT_TRUE( ht_deinitialize( &ht ) );
	}
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
	ADD_TEST( "hashtable new fail alloc", test_hashtable_fail_alloc );
	ADD_TEST( "hashtable new fail init", test_hashtable_fail_init );
	ADD_TEST( "hashtable delete null", test_hashtable_delete_null );
	ADD_TEST( "hashtable init pre-reqs", test_hashtable_init_prereqs );
	ADD_TEST( "hashtable deinit pre-reqs", test_hashtable_deinit_prereqs );
	ADD_TEST( "hashtable count", test_hashtable_count );
	ADD_TEST( "hashtable insert", test_hashtable_insert );
	ADD_TEST( "hashtable clear", test_hashtable_clear );
	ADD_TEST( "hashtable find", test_hashtable_find );
	ADD_TEST( "hashtable remove", test_hashtable_remove );
	ADD_TEST( "hashtable get", test_hashtable_get );
	ADD_TEST( "empty hashtable iterator", test_hashtable_empty_iterator );

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

