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

		if ( ht_load( ht ) != 0.0f )
		{
			printf("(size: %d)  %f != %f\n", size, ht_load( ht ), 0.0f);
		}

		CU_ASSERT_EQUAL( ht_load( ht ), 0.0f );
		CU_ASSERT_EQUAL( ht_get_resize_load_factor( ht ), ht->load_factor );

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
		CU_ASSERT_EQUAL( ht_get_resize_load_factor( &ht ), ht.load_factor );

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

static void test_hashtable_compact( void )
{
	int i, j, k;
	ht_itr_t itr;
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
			CU_ASSERT_EQUAL( ht_add( &ht, (void*)j, (void*)j ), TRUE );
			CU_ASSERT_EQUAL( ht_size( &ht ), (j + 1) );

			itr = ht_find( &ht, (void*)j );
			CU_ASSERT_NOT_EQUAL( itr, ht_itr_end( &ht ) );

			k = (int)ht_itr_get( &ht, itr, NULL );
			CU_ASSERT_EQUAL( k, j );
		}

		ht_compact( &ht );

		CU_ASSERT_EQUAL( ht_size( &ht ), (size * multiple) );

		ht_deinitialize( &ht );
	}
}

static void test_hashtable_change_load_factor( void )
{
	int i, j, k;
	ht_itr_t itr;
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
			CU_ASSERT_EQUAL( ht_add( &ht, (void*)j, (void*)j ), TRUE );
			CU_ASSERT_EQUAL( ht_size( &ht ), (j + 1) );

			itr = ht_find( &ht, (void*)j );
			CU_ASSERT_NOT_EQUAL( itr, ht_itr_end( &ht ) );

			k = (int)ht_itr_get( &ht, itr, NULL );
			CU_ASSERT_EQUAL( k, j );
		}

		ht_set_resize_load_factor( &ht, 0.25f );

		CU_ASSERT_EQUAL( ht_size( &ht ), (size * multiple) );

		ht_deinitialize( &ht );
	}
}

static void test_hashtable_clear( void )
{
	int i, j, k;
	ht_itr_t itr;
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
			CU_ASSERT_EQUAL( ht_add( &ht, (void*)j, (void*)j ), TRUE );
			CU_ASSERT_EQUAL( ht_size( &ht ), (j + 1) );

			itr = ht_find( &ht, (void*)j );
			CU_ASSERT_NOT_EQUAL( itr, ht_itr_end( &ht ) );

			k = (int)ht_itr_get( &ht, itr, NULL );
			CU_ASSERT_EQUAL( k, j );
		}

		CU_ASSERT_EQUAL( ht_size( &ht ), (size * multiple) );
		ht_clear( &ht );
		CU_ASSERT_EQUAL( ht_size( &ht ), 0 );

		ht_deinitialize( &ht );
	}
}

static void test_hashtable_forward_itr( void )
{
	int i, j, k, v;
	ht_itr_t itr;
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
			CU_ASSERT_EQUAL( ht_add( &ht, (void*)j, (void*)j ), TRUE );
			CU_ASSERT_EQUAL( ht_size( &ht ), (j + 1) );
		}

		itr = ht_itr_begin( &ht );
		for ( ; itr != ht_itr_end( &ht ); itr = ht_itr_next( &ht, itr ) )
		{
			k = (int)ht_itr_get( &ht, itr, (void**)&v );
			CU_ASSERT_EQUAL( k, v );
		}

		CU_ASSERT_EQUAL( ht_size( &ht ), (size * multiple) );

		ht_deinitialize( &ht );
	}
}

static void test_hashtable_reverse_itr( void )
{
	int i, j, k, v;
	ht_itr_t itr;
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
			CU_ASSERT_EQUAL( ht_add( &ht, (void*)j, (void*)j ), TRUE );
			CU_ASSERT_EQUAL( ht_size( &ht ), (j + 1) );
		}

		itr = ht_itr_rbegin( &ht );
		for ( ; itr != ht_itr_rend( &ht ); itr = ht_itr_rnext( &ht, itr ) )
		{
			k = (int)ht_itr_get( &ht, itr, (void**)&v );
			CU_ASSERT_EQUAL( k, v );
		}

		CU_ASSERT_EQUAL( ht_size( &ht ), (size * multiple) );

		ht_deinitialize( &ht );
	}
}


static void test_hashtable_clear_empty( void )
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
		CU_ASSERT_EQUAL( ht_get_resize_load_factor( &ht ), ht.load_factor );

		ht_clear( &ht );
		CU_ASSERT_EQUAL( ht_size( &ht ), 0 );

		ht_deinitialize( &ht );
	}
}

static void test_hashtable_remove( void )
{
	int i, j, k;
	ht_itr_t itr;
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
			CU_ASSERT_EQUAL( ht_add( &ht, (void*)j, (void*)j ), TRUE );
			CU_ASSERT_EQUAL( ht_size( &ht ), (j + 1) );

			itr = ht_find( &ht, (void*)j );
			CU_ASSERT_NOT_EQUAL( itr, ht_itr_end( &ht ) );

			k = (int)ht_itr_get( &ht, itr, NULL );
			CU_ASSERT_EQUAL( k, j );
		}

		CU_ASSERT_EQUAL( ht_size( &ht ), (size * multiple) );

		itr = ht_itr_begin( &ht );
		for ( j = 0; j < (size * multiple); j++ )
		{
			if ( j & 0x1 )
			{
				CU_ASSERT_EQUAL( ht_remove( &ht, itr ), TRUE );
			}

			itr = ht_itr_next( &ht, itr );
		}

		CU_ASSERT_EQUAL( ht_size( &ht ), (size * multiple) / 2 );

		ht_deinitialize( &ht );
	}
}

static void test_hashtable_add_static( void )
{
	int i, j, k;
	ht_itr_t itr;
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
			CU_ASSERT_EQUAL( ht_add( &ht, (void*)j, (void*)j ), TRUE );
			CU_ASSERT_EQUAL( ht_size( &ht ), (j + 1) );

			itr = ht_find( &ht, (void*)j );
			CU_ASSERT_NOT_EQUAL( itr, ht_itr_end( &ht ) );

			k = (int)ht_itr_get( &ht, itr, NULL );
			CU_ASSERT_EQUAL( k, j );
		}

		CU_ASSERT_EQUAL( ht_size( &ht ), (size * multiple) );

		ht_deinitialize( &ht );
	}
}

static void test_hashtable_find_static( void )
{
	int i, j, k;
	ht_itr_t itr;
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
			CU_ASSERT_EQUAL( ht_add( &ht, (void*)j, (void*)j ), TRUE );
			CU_ASSERT_EQUAL( ht_size( &ht ), (j + 1) );
		}

		CU_ASSERT_EQUAL( ht_size( &ht ), (size * multiple) );

		for ( j = 0; j < (size * multiple); j++ )
		{
			itr = ht_find( &ht, (void*)j );
			CU_ASSERT_NOT_EQUAL( itr, ht_itr_end( &ht ) );

			k = (int)ht_itr_get( &ht, itr, NULL );
			CU_ASSERT_EQUAL( k, j );
		}

		ht_deinitialize( &ht );
	}
}

static uint_t dynamic_key_hash(void const * const key)
{
	return *((uint_t*)key);
}

static int dynamic_key_eq(void const * const l, void const * const r)
{
	return ( *((uint_t*)l) == *((uint_t*)r) );
}



static void test_hashtable_add_dynamic( void )
{
	int i, j;
	int * k;
	int * v;;
	ht_itr_t itr;
	uint32_t size, multiple;
	ht_t ht;

	for ( i = 0; i < REPEAT; i++ )
	{
		MEMSET( &ht, 0, sizeof(ht_t) );
		size = (rand() % SIZEMAX);
		multiple = (rand() % MULTIPLE);
		ht_initialize( &ht, size, &dynamic_key_hash, FREE, &dynamic_key_eq, FREE );

		for ( j = 0; j < (size * multiple); j++ )
		{
			k = (int*)CALLOC( 1, sizeof(int) );
			(*k) = j;
			v = (int*)CALLOC( 1, sizeof(int) );
			(*v) = j;

			CU_ASSERT_EQUAL( ht_add( &ht, (void*)k, (void*)v ), TRUE );
			CU_ASSERT_EQUAL( ht_size( &ht ), (j + 1) );

			itr = ht_find( &ht, (void*)k );
			CU_ASSERT_NOT_EQUAL( itr, ht_itr_end( &ht ) );

			v = (int*)ht_itr_get( &ht, itr, (void**)&k );
			CU_ASSERT_EQUAL( (*k), j );
			CU_ASSERT_EQUAL( (*v), j );
		}

		CU_ASSERT_EQUAL( ht_size( &ht ), (size * multiple) );

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
	CHECK_PTR_RET( CU_add_test( pSuite, "hashtable compact", test_hashtable_compact), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "hashtable change load factor", test_hashtable_change_load_factor), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "hashtable clear", test_hashtable_clear), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "hashtable clear empty", test_hashtable_clear_empty), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "hashtable forward iterator", test_hashtable_forward_itr), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "hashtable reverse iterator", test_hashtable_reverse_itr), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "hashtable remove", test_hashtable_remove), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "hashtable add static values", test_hashtable_add_static), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "hashtable find static values", test_hashtable_find_static), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "hashtable add dynamic values", test_hashtable_add_dynamic), NULL );
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

