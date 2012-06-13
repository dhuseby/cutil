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
#include <cutil/btree.h>

void test_btree_newdel( void )
{
	int i;
	size_t size;
	bt_t * bt;

	for ( i = 0; i < 1024; i++ )
	{
		bt = NULL;
		size = (rand() % 1024);
		bt = bt_new( size, NULL, FREE, FREE );

		CU_ASSERT_PTR_NOT_NULL( bt );
		bt_delete( (void*)bt );
	}
}

static int int_less( void * l, void * r )
{
	int_t li = (int_t)l;
	int_t ri = (int_t)r;

	if ( li < ri )
		return -1;
	else if ( li > ri )
		return 1;
	return 0;
}

static int pint_less( void * l, void * r )
{
	int_t li = *((int_t*)l);
	int_t ri = *((int_t*)r);

	if ( li < ri )
		return -1;
	else if ( li > ri )
		return 1;
	return 0;
}

void test_btree_iterator( void )
{
	int_t i;
	int_t cur, prev;
	bt_t * bt;
	bt_itr_t itr;

	bt = bt_new( 9, int_less, NULL, NULL );
	CU_ASSERT_PTR_NOT_NULL( bt );

	for ( i = 1; i < 10; i++ )
	{
		CU_ASSERT_EQUAL( bt_add( bt, (void*)i, (void*)i ), TRUE );
	}

	prev = 0;
	itr = bt_itr_begin( bt );
	for ( ; itr != bt_itr_end( bt ); itr = bt_itr_next( bt, itr ) )
	{
		/* get the value at the current iterator position */
		cur = (int_t)bt_itr_get( bt, itr );
		CU_ASSERT( cur == (prev + 1) );
		prev = cur;
	}

	CU_ASSERT_EQUAL( itr, bt_itr_end( bt ) );

	prev = 10;
	itr = bt_itr_rbegin( bt );
	for ( ; itr != bt_itr_rend( bt ); itr = bt_itr_rnext( bt, itr ) )
	{
		/* get the value at the current iterator position */
		cur = (int_t)bt_itr_get( bt, itr );
		CU_ASSERT( cur == (prev - 1) );
		prev = cur;
	}

	/* remove 5 */
	CU_ASSERT_EQUAL( 5, (int_t)bt_remove( bt, (void*)5 ) );
	CU_ASSERT_PTR_NULL( bt_find( bt, (void*)5 ) );

	prev = 0;
	itr = bt_itr_begin( bt );
	for ( ; itr != bt_itr_end( bt ); itr = bt_itr_next( bt, itr ) )
	{
		/* get the value at the current iterator position */
		cur = (int_t)bt_itr_get( bt, itr );
		if ( cur == 6 )
		{
			CU_ASSERT( cur == (prev + 2) ); /* special case for cur == 6, prev == 4 */
		}
		else
		{
			CU_ASSERT( cur == (prev + 1) );
		}
		prev = cur;
	}

	bt_delete( (void*)bt );
}

void test_btree_random( void )
{
	int_t i = 0;
	int_t v = 0;
	int_t cur, prev;
	bt_t * bt;
	bt_itr_t itr;
	size_t size = (rand() % 1024);

	bt = bt_new( 10, int_less, NULL, NULL );
	for ( i = 0; i < size; i++ )
	{
		v = rand();
		CU_ASSERT_EQUAL( bt_add( bt, (void*)v, (void*)v ), TRUE );
	}

	CU_ASSERT_EQUAL( bt_size( bt ), size );

	itr = bt_itr_begin( bt );
	prev = (int_t)bt_itr_get( bt, itr ) - 1;
	for ( ; itr != bt_itr_end( bt ); itr = bt_itr_next( bt, itr ) )
	{
		cur = (int_t)bt_itr_get( bt, itr );
		CU_ASSERT( cur > prev );
		prev = cur;
	}
	CU_ASSERT_EQUAL( itr, bt_itr_end( bt ) );

	bt_delete( (void*)bt );
}

void test_btree_random_default( void )
{
	int_t i = 0;
	int_t v = 0;
	int_t cur, prev;
	bt_t * bt;
	bt_itr_t itr;
	size_t size = (rand() % 1024);

	bt = bt_new( 10, NULL, NULL, NULL );
	for ( i = 0; i < size; i++ )
	{
		v = rand();
		CU_ASSERT_EQUAL( bt_add( bt, (void*)v, (void*)v ), TRUE );
	}

	CU_ASSERT_EQUAL( bt_size( bt ), size );

	itr = bt_itr_begin( bt );
	prev = (int_t)bt_itr_get( bt, itr ) - 1;
	for ( ; itr != bt_itr_end( bt ); itr = bt_itr_next( bt, itr ) )
	{
		cur = (int_t)bt_itr_get( bt, itr );
		CU_ASSERT( cur > prev );
		prev = cur;
	}
	CU_ASSERT_EQUAL( itr, bt_itr_end( bt ) );

	bt_delete( (void*)bt );
}

void test_btree_random_duplicate( void )
{
	int_t i = 0;
	int_t v = 0;
	int_t cur, prev;
	bt_t * bt;
	bt_itr_t itr;
	size_t size = (rand() % 1024);

	bt = bt_new( 10, NULL, NULL, NULL );
	for ( i = 0; i < size; i++ )
	{
		v = rand();
		CU_ASSERT_EQUAL( bt_add( bt, (void*)v, (void*)v ), TRUE );
		CU_ASSERT_EQUAL( bt_add( bt, (void*)v, (void*)v ), FALSE );
	}

	CU_ASSERT_EQUAL( bt_size( bt ), size );

	itr = bt_itr_begin( bt );
	prev = (int_t)bt_itr_get( bt, itr ) - 1;
	for ( ; itr != bt_itr_end( bt ); itr = bt_itr_next( bt, itr ) )
	{
		cur = (int_t)bt_itr_get( bt, itr );
		CU_ASSERT( cur > prev );
		prev = cur;
	}
	CU_ASSERT_EQUAL( itr, bt_itr_end( bt ) );

	bt_delete( (void*)bt );
}

void test_btree_random_dynamic( void )
{
	int_t i = 0;
	int_t* v = NULL;
	int_t* k = NULL;
	int_t cur;
	int_t prev = -1;
	bt_t * bt;
	bt_itr_t itr;
	size_t size = (rand() % 1024);

	bt = bt_new( 10, pint_less, FREE, FREE );
	for ( i = 0; i < size; i++ )
	{
		v = CALLOC( 1, sizeof(int_t) );
		CU_ASSERT_PTR_NOT_NULL( v );
		k = CALLOC( 1, sizeof(int_t) );
		CU_ASSERT_PTR_NOT_NULL( k );

		(*k) = rand();
		(*v) = rand();
		CU_ASSERT_EQUAL( bt_add( bt, (void*)k, (void*)v ), TRUE );
		CU_ASSERT_EQUAL( bt_add( bt, (void*)k, (void*)v ), FALSE );
	}

	CU_ASSERT_EQUAL( bt_size( bt ), size );

	itr = bt_itr_begin( bt );
	for ( itr = bt_itr_begin( bt ); itr != bt_itr_end( bt ); itr = bt_itr_next( bt, itr ) )
	{
		if ( prev == -1 )
		{
			prev = *((int_t*)bt_itr_get_key( bt, itr ));
		}
		else
		{
			prev = cur;
			cur = *((int_t*)bt_itr_get_key( bt, itr ));
			CU_ASSERT( cur > prev );
		}
	}
	CU_ASSERT_EQUAL( itr, bt_itr_end( bt ) );

	bt_delete( (void*)bt );
}



static int init_btree_suite( void )
{
	srand(0xDEADBEEF);
	return 0;
}

static int deinit_btree_suite( void )
{
	return 0;
}

static CU_pSuite add_btree_tests( CU_pSuite pSuite )
{
	CHECK_PTR_RET( CU_add_test( pSuite, "new/delete of btree", test_btree_newdel), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "iteration of btree", test_btree_iterator), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "iteration of random btree", test_btree_random), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "iteration of random btree using default compare", test_btree_random_default), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "iteration of random btree add duplicates", test_btree_random_duplicate), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "random btree with dynamically allocated keys and values", test_btree_random_dynamic), NULL );
	
	return pSuite;
}

CU_pSuite add_btree_test_suite()
{
	CU_pSuite pSuite = NULL;

	/* add the suite to the registry */
	pSuite = CU_add_suite("BTree Tests", init_btree_suite, deinit_btree_suite);
	CHECK_PTR_RET( pSuite, NULL );

	/* add in btree specific tests */
	CHECK_PTR_RET( add_btree_tests( pSuite ), NULL );

	return pSuite;
}

