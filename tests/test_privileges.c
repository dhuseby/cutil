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
#include <cutil/privileges.h>

#include "test_macros.h"
#include "test_flags.h"

#define REPEAT (128)
#define SIZEMAX (128)
#define MULTIPLE (8)

extern void test_privileges_private_functions( void );

static void test_privileges_temp_drop( void )
{
	drop_privileges( FALSE );
	restore_privileges();
}

static void test_privileges_permanent_drop( void )
{
	drop_privileges( TRUE );
}


static int init_privileges_suite( void )
{
	srand(0xDEADBEEF);
	reset_test_flags();
	return 0;
}

static int deinit_privileges_suite( void )
{
	reset_test_flags();
	return 0;
}

static CU_pSuite add_privileges_tests( CU_pSuite pSuite )
{
	ADD_TEST( "privileges temporary drop", test_privileges_temp_drop );
	ADD_TEST( "privileges permanent drop", test_privileges_permanent_drop );
	ADD_TEST( "privileges private functions", test_privileges_private_functions );
	return pSuite;
}

CU_pSuite add_privileges_test_suite()
{
	CU_pSuite pSuite = NULL;

	/* add the suite to the registry */
	pSuite = CU_add_suite("Privileges Tests", init_privileges_suite, deinit_privileges_suite);
	CHECK_PTR_RET( pSuite, NULL );

	/* add in hashtable specific tests */
	CHECK_PTR_RET( add_privileges_tests( pSuite ), NULL );

	return pSuite;
}

