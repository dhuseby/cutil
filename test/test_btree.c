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

#include "test_btree.h"

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

		if ( size == 0 )
		{
			CU_ASSERT_PTR_NULL( bt );
		}
		else
		{
			CU_ASSERT_PTR_NOT_NULL( bt );
			bt_delete( (void*)bt );
		}
	}
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

