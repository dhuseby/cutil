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

#define SUITE(x) extern CU_pSuite add_##x##_test_suite(); CU_pSuite x##_test_suite;
#define ADD_SUITE(x) x##_test_suite = add_##x##_test_suite();

/* switches for forcing varicus things to fail */
int fail_alloc = FALSE;
int fail_list_grow = FALSE;

/*SUITE( aiofd );
SUITE( bitset );
SUITE( btree );
SUITE( buffer );
SUITE( child );
SUITE( hashtable );*/
SUITE( list );
/*SUITE( pair );
SUITE( socket );
SUITE( sanitize );*/

int main()
{
	/* initialize the CUnit test registry */
	if ( CUE_SUCCESS != CU_initialize_registry() )
		return CU_get_error();

	/* add each suite of tests */
	/*ADD_SUITE( aiofd );
	ADD_SUITE( bitset );
	ADD_SUITE( btree );
	ADD_SUITE( buffer );
	ADD_SUITE( child );
	ADD_SUITE( hashtable );*/
	ADD_SUITE( list );
	/*ADD_SUITE( pair );
	ADD_SUITE( socket );
	ADD_SUITE( sanitize );*/

	/* run all tests using the CUnit Basic interface */
	CU_basic_set_mode( CU_BRM_VERBOSE );
	CU_basic_run_tests();

	/* clean up */
	CU_cleanup_registry();

	return CU_get_error();
}

