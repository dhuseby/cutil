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
#if 0
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

#include "test_hashtable.h"

#define REPEAT (128)
#define SIZEMAX (128)
#define MULTIPLE (8)

static void test_hashtable_newdel( void )
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

		ht_delete( ht );
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
	CHECK_PTR_RET( CU_add_test( pSuite, "new/delete of hashtable", test_hashtable_newdel), NULL );
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
#endif

