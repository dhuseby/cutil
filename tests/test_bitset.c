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

#include <CUnit/Basic.h>

#include <cutil/debug.h>
#include <cutil/macros.h>
#include <cutil/bitset.h>

void test_bitset_newdel( void )
{
	int i;
	size_t size;
	bitset_t * bset;

	for ( i = 0; i < 1024; i++ )
	{
		bset = NULL;
		size = (rand() % 1024);
		bset = bset_new( size );

		if ( size == 0 )
		{
			CU_ASSERT_PTR_NULL( bset );
		}
		else
		{
			CU_ASSERT_PTR_NOT_NULL( bset );
			CU_ASSERT_PTR_NOT_NULL( bset->bits );
			CU_ASSERT_EQUAL( bset->num_bits, size );
			bset_delete( (void*)bset );
		}
	}
}

void test_bitset_initdeinit( void )
{
	int i;
	size_t size;
	bitset_t bset;

	for ( i = 0; i < 1024; i++ )
	{
		MEMSET( &bset, 0, sizeof(bitset_t) );
		size = (rand() % 1024);
		bset_initialize( &bset, size );

		if ( size > 0 )
		{
			CU_ASSERT_PTR_NOT_NULL( bset.bits );
		}
		CU_ASSERT_EQUAL( bset.num_bits, size );

		bset_deinitialize( &bset );
	}
}

void test_bitset_zerosize( void )
{
	bitset_t bset1;
	bitset_t* bset2;

	bset_initialize( &bset1, 0 );
	CU_ASSERT_PTR_NULL( bset1.bits );
	CU_ASSERT_EQUAL( bset1.num_bits, 0 );

	bset2 = bset_new( 0 );
	CU_ASSERT_PTR_NULL( bset2 );
}

void test_bitset_maxsize( void )
{
	bitset_t bset1;
	bitset_t* bset2;
	size_t max_size = (size_t)-1;

	bset_initialize( &bset1, max_size );
	CU_ASSERT_PTR_NOT_NULL( bset1.bits );
	CU_ASSERT_EQUAL( bset1.num_bits, max_size );
	bset_deinitialize( &bset1 );
	CU_ASSERT_PTR_NULL( bset1.bits );
	CU_ASSERT_EQUAL( bset1.num_bits, 0 );

	bset2 = bset_new( max_size );
	CU_ASSERT_PTR_NOT_NULL( bset2 );
	CU_ASSERT_PTR_NOT_NULL( bset2->bits );
	CU_ASSERT_EQUAL( bset2->num_bits, max_size );
	bset_delete( (void*)bset2 );
}

void test_bitset_setall( void )
{
	bitset_t bset;
	size_t i, size;

	size = (rand() % 65535);
	MEMSET( &bset, 0, sizeof(bitset_t) );
	bset_initialize( &bset, size );

	CU_ASSERT_PTR_NOT_NULL( bset.bits );
	CU_ASSERT_EQUAL( bset.num_bits, size );

	bset_set_all( &bset );

	for ( i = 0; i < size; i++ )
	{
		CU_ASSERT_EQUAL( bset_test( &bset, i ), TRUE );
	}

	bset_deinitialize( &bset );
	CU_ASSERT_PTR_NULL( bset.bits );
	CU_ASSERT_EQUAL( bset.num_bits, 0 );
}

void test_bitset_clearall( void )
{
	bitset_t bset;
	size_t i, nflips, size;

	size = (rand() % 65535);
	MEMSET( &bset, 0, sizeof(bitset_t) );
	bset_initialize( &bset, size );

	CU_ASSERT_PTR_NOT_NULL( bset.bits );
	CU_ASSERT_EQUAL( bset.num_bits, size );

	/* figure out a random number of bits to flip */
	nflips = (rand() % size);

	/* flip random bits */
	for ( i = 0; i < nflips; i++ )
	{
		bset_set( &bset, (size_t)(rand() % size) );
	}

	/* clear all of them */
	bset_clear_all( &bset );

	/* check */
	for ( i = 0; i < size; i++ )
	{
		CU_ASSERT_EQUAL( bset_test( &bset, i ), FALSE );
	}

	bset_deinitialize( &bset );
	CU_ASSERT_PTR_NULL( bset.bits );
	CU_ASSERT_EQUAL( bset.num_bits, 0 );
}

void test_bitset_patternbitflips( void )
{
	size_t i;
	size_t size;
	bitset_t bset;

	size = (rand() % 65535);
	MEMSET( &bset, 0, sizeof(bitset_t) );
	bset_initialize( &bset, size );

	CU_ASSERT_PTR_NOT_NULL( bset.bits );
	CU_ASSERT_EQUAL( bset.num_bits, size );

	/* flip all of the odd bits */
	for ( i = 0; i < size; i++ )
	{
		if ( i & 1 )
		{
			bset_set( &bset, i );
		}
	}

	/* check to see if odd bits are flipped */
	for ( i = 0; i < size; i++ )
	{
		if ( i & 1 )
		{
			CU_ASSERT_EQUAL( bset_test( &bset, i ), TRUE );
		}
		else
		{
			CU_ASSERT_EQUAL( bset_test( &bset, i ), FALSE );
		}
	}

	/* clear all bits */
	bset_clear_all( &bset );

	/* flip all of the odd bits */
	for ( i = 0; i < size; i++ )
	{
		if ( !(i & 1) )
		{
			bset_set( &bset, i );
		}
	}

	/* check to see if odd bits are flipped */
	for ( i = 0; i < size; i++ )
	{
		if ( !(i & 1) )
		{
			CU_ASSERT_EQUAL( bset_test( &bset, i ), TRUE );
		}
		else
		{
			CU_ASSERT_EQUAL( bset_test( &bset, i ), FALSE );
		}
	}

	bset_deinitialize( &bset );
}

void test_bitset_randombitflips( void )
{
	int test;
	bitset_t bset;
	size_t i, j, nflips, size;
	size_t * idxs;

	size = (rand() % 65535);
	MEMSET( &bset, 0, sizeof(bitset_t) );
	bset_initialize( &bset, size );

	CU_ASSERT_PTR_NOT_NULL( bset.bits );
	CU_ASSERT_EQUAL( bset.num_bits, size );

	/* figure out a random number of bits to flip */
	nflips = (rand() % size);

	/* allocate an array to remember the indexes */
	idxs = CALLOC( nflips, sizeof(size_t) );
	CU_ASSERT_PTR_NOT_NULL( idxs );

	/* flip random bits */
	for ( i = 0; i < nflips; i++ )
	{
		idxs[i] = (rand() % size);
		bset_set( &bset, idxs[i] );
	}

	/* check */
	for ( i = 0; i < size; i++ )
	{
		test = FALSE;

		/* do a slow linear search of the random index list */
		for ( j = 0; j < nflips; j++ )
		{
			if (idxs[j] == i)
			{
				test = TRUE;
				break;
			}
		}

		/* does the bit match what we expect? */
		CU_ASSERT_EQUAL( bset_test( &bset, i ), test );
	}

	/* release the memory for the index list */
	FREE( idxs );

	bset_deinitialize( &bset );
	CU_ASSERT_PTR_NULL( bset.bits );
	CU_ASSERT_EQUAL( bset.num_bits, 0 );
}

static int init_bitset_suite( void )
{
	srand(0xDEADBEEF);
	return 0;
}

static int deinit_bitset_suite( void )
{
	return 0;
}

static CU_pSuite add_bitset_tests( CU_pSuite pSuite )
{
	CHECK_PTR_RET( CU_add_test( pSuite, "new/delete of bitset", test_bitset_newdel), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "init/deinit of bitset", test_bitset_initdeinit), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "zero size bitset", test_bitset_zerosize), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "max size bitset", test_bitset_maxsize), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "bitset setall test", test_bitset_setall), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "bitset clearall test", test_bitset_clearall), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "bitset pattern test", test_bitset_patternbitflips), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "bitset random test", test_bitset_randombitflips), NULL );
	
	return pSuite;
}

CU_pSuite add_bitset_test_suite()
{
	CU_pSuite pSuite = NULL;

	/* add the suite to the registry */
	pSuite = CU_add_suite("Bitset Tests", init_bitset_suite, deinit_bitset_suite);
	CHECK_PTR_RET( pSuite, NULL );

	/* add in bitset specific tests */
	CHECK_PTR_RET( add_bitset_tests( pSuite ), NULL );

	return pSuite;
}

