/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
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

#include <debug.h>
#include <macros.h>
#include <array.h>

#include "test_array.h"

static int init_array_suite( void )
{
	return 0;
}

static int deinit_array_suite( void )
{
	return 0;
}

static CU_pSuite add_array_tests( CU_pSuite pSuite )
{
	return pSuite;
}

CU_pSuite add_array_test_suite()
{
	CU_pSuite pSuite = NULL;

	/* add the suite to the registry */
	pSuite = CU_add_suite("Array Tests", init_array_suite, deinit_array_suite);
	CHECK_PTR_RET( pSuite, NULL );

	/* add in array specific tests */
	CHECK_PTR_RET( add_array_tests( pSuite ), NULL );

	return pSuite;
}

