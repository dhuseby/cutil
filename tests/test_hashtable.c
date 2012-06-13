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

#define REPEAT (128)
#define SIZEMAX (128)
#define MULTIPLE (8)

static void test_hashtable_newdel_default_fns( void )
{
	int i;
	uint32_t size;
	ht_t * ht;

	for ( i = 0; i < REPEAT; i++ )
	{
		ht = NULL;
		size = (rand() % SIZEMAX);
		ht = ht_new( size, NULL, NULL, NULL, NULL );

		CU_ASSERT_PTR_NOT_NULL( ht );
		CU_ASSERT_EQUAL( ht_size( ht ), 0 );
		CU_ASSERT_EQUAL( ht->initial_capacity, size );
		CU_ASSERT_NOT_EQUAL( ht->khfn, NULL );
		CU_ASSERT_NOT_EQUAL( ht->kefn, NULL );
		CU_ASSERT_EQUAL( ht->kdfn, NULL );
		CU_ASSERT_EQUAL( ht->vdfn, NULL );

		CU_ASSERT_EQUAL( ht_load( ht ), 0.0f );
		CU_ASSERT_EQUAL( ht_get_resize_load_factor( ht ), 0.65f );

		ht_delete( ht );
	}
}

static void test_hashtable_initdeinit_default_fns( void )
{
	int i;
	uint32_t size;
	ht_t ht;

	for ( i = 0; i < REPEAT; i++ )
	{
		MEMSET( &ht, 0, sizeof(ht_t) );
		size = (rand() % SIZEMAX);
		ht_initialize( &ht, size, NULL, NULL, NULL, NULL );

		CU_ASSERT_EQUAL( ht_size( &ht ), 0 );
		CU_ASSERT_EQUAL( ht.initial_capacity, size );
		CU_ASSERT_NOT_EQUAL( ht.khfn, NULL );
		CU_ASSERT_NOT_EQUAL( ht.kefn, NULL );
		CU_ASSERT_EQUAL( ht.kdfn, NULL );
		CU_ASSERT_EQUAL( ht.vdfn, NULL );

		CU_ASSERT_EQUAL( ht_load( &ht ), 0.0f );
		CU_ASSERT_EQUAL( ht_get_resize_load_factor( &ht ), 0.65f );

		ht_deinitialize( &ht );
	}
}

static uint_t key_hash( void const * const key )
{
	uint_t const hash = (uint_t)key;
	return ((hash == 0) ? hash + 1 : hash);
}

static int key_eq( void const * const l, void const * const r )
{
	return ((uint_t)l == (uint_t)r);
}

static void test_hashtable_newdel_custom_key_fns( void )
{
	int i;
	uint32_t size;
	ht_t * ht;

	for ( i = 0; i < REPEAT; i++ )
	{
		ht = NULL;
		size = (rand() % SIZEMAX);
		ht = ht_new( size, &key_hash, NULL, &key_eq, NULL );

		CU_ASSERT_PTR_NOT_NULL( ht );
		CU_ASSERT_EQUAL( ht_size( ht ), 0 );
		CU_ASSERT_EQUAL( ht->initial_capacity, size );
		CU_ASSERT_EQUAL( ht->khfn, &key_hash );
		CU_ASSERT_EQUAL( ht->kefn, &key_eq );
		CU_ASSERT_EQUAL( ht->kdfn, NULL );
		CU_ASSERT_EQUAL( ht->vdfn, NULL );

		ht_delete( ht );
	}
}

static void test_hashtable_static_grow( void )
{
	int i, j;
	uint_t k;
	uint32_t size, multiple;
	ht_t ht;

	for ( i = 0; i < 8; i++ )
	{
		MEMSET( &ht, 0, sizeof(ht_t) );
		size = (rand() % SIZEMAX);
		ht_initialize( &ht, size, NULL, NULL, NULL, NULL );

		CU_ASSERT_EQUAL( ht_size( &ht ), 0 );
		CU_ASSERT_EQUAL( ht.initial_capacity, size );
		CU_ASSERT_NOT_EQUAL( ht.khfn, NULL );
		CU_ASSERT_NOT_EQUAL( ht.kefn, NULL );
		CU_ASSERT_EQUAL( ht.kdfn, NULL );
		CU_ASSERT_EQUAL( ht.vdfn, NULL );

		for ( j = 0; j < 8; j++ )
		{
			k = ht.prime_index;
			ht_force_grow( &ht );
			CU_ASSERT_EQUAL( k + 1, ht.prime_index );
		}

		ht_deinitialize( &ht );
	}
}

static void test_hashtable_dynamic_grow( void )
{
	int i, j;
	uint_t k;
	uint32_t size, multiple;
	ht_t * ht;

	for ( i = 0; i < 8; i++ )
	{
		ht = NULL;
		size = (rand() % SIZEMAX);
		ht = ht_new( size, NULL, NULL, NULL, NULL );

		CU_ASSERT_EQUAL( ht_size( ht ), 0 );
		CU_ASSERT_EQUAL( ht->initial_capacity, size );
		CU_ASSERT_NOT_EQUAL( ht->khfn, NULL );
		CU_ASSERT_NOT_EQUAL( ht->kefn, NULL );
		CU_ASSERT_EQUAL( ht->kdfn, NULL );
		CU_ASSERT_EQUAL( ht->vdfn, NULL );

		for ( j = 0; j < 8; j++ )
		{
			k = ht->prime_index;
			ht_force_grow( ht );
			CU_ASSERT_EQUAL( k + 1, ht->prime_index );
		}

		ht_delete( (void*)ht );
	}
}

static void test_hashtable_add_static( void )
{
	int i, j, k;
	uint32_t size, multiple;
	ht_t ht;

	/*
	for ( i = 0; i < REPEAT; i++ )
	{
	*/
		MEMSET( &ht, 0, sizeof(ht_t) );
		size = (rand() % SIZEMAX);
		multiple = (rand() % MULTIPLE);
		ht_initialize( &ht, size, NULL, NULL, NULL, NULL );

		for ( j = 0; j < (size * multiple); j++ )
		{
			if ( ht_add( &ht, (void*)j, (void*)j ) == FALSE )
			{
				printf("failed to add %d\n", j);
			}

			printf("%d == %d\n", ht_size( &ht ), (size * multiple));
			CU_ASSERT_EQUAL( ht_size( &ht ), (size * multiple) );

			k = (int)ht_find( &ht, (void*)j );
			if ( k != j )
			{
				printf("failed to find %d\n", j );
			}
		}

		printf("%d == %d\n", ht_size( &ht ), (size * multiple));
		CU_ASSERT_EQUAL( ht_size( &ht ), (size * multiple) );

		ht_deinitialize( &ht );
	/*}*/
}

static void test_hashtable_find_static( void )
{
	int i, j, k;
	uint32_t size, multiple;
	ht_t ht;

	for ( i = 0; i < REPEAT; i++ )
	{
		MEMSET( &ht, 0, sizeof(ht_t) );
		size = (rand() % SIZEMAX);
		multiple = (rand() % MULTIPLE);
		ht_initialize( &ht, size, NULL, NULL, NULL, NULL );

		for ( j = 0; j < (size * multiple); j++ )
		{
			ht_add( &ht, (void*)j, (void*)j );
		}

		CU_ASSERT_EQUAL( ht_size( &ht ), (size * multiple) );

		for ( j = 0; j < (size * multiple); j++ )
		{
			k = (int)ht_find( &ht, (void*)j );
			CU_ASSERT_EQUAL( k, j );
		}

		ht_deinitialize( &ht );
	}
}


static int init_hashtable_suite( void )
{
	srand(0xDEADBEEF);
	return 0;
}

static int deinit_hashtable_suite( void )
{
	return 0;
}

static CU_pSuite add_hashtable_tests( CU_pSuite pSuite )
{
	CHECK_PTR_RET( CU_add_test( pSuite, "new/delete of heap hashtable with default fns", test_hashtable_newdel_default_fns), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "init/deinit of stack hashtable with default fns", test_hashtable_initdeinit_default_fns), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "new/delete of hashtable with custom key fns", test_hashtable_newdel_custom_key_fns), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "grow of stack hashtable", test_hashtable_static_grow), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "grow of heap hashtable", test_hashtable_dynamic_grow), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "hashtable add static values", test_hashtable_add_static), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "hashtable find static values", test_hashtable_find_static), NULL );
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

