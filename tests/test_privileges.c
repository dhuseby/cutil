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
	priv_state_t orig;
	CU_ASSERT_FALSE( drop_privileges( FALSE, NULL ) );
	CU_ASSERT_FALSE( restore_privileges( NULL ) );
	
	MEMSET( &orig, 0, sizeof( priv_state_t ) );
	CU_ASSERT_TRUE( drop_privileges( FALSE, &orig ) );
	CU_ASSERT_TRUE( restore_privileges( &orig ) );
}

static void test_privileges_permanent_drop( void )
{
	CU_ASSERT_TRUE( drop_privileges( TRUE, NULL ) );
}

static void test_privileges_drop_failures( void )
{
	priv_state_t orig;
	MEMSET( &orig, 0, sizeof( priv_state_t ) );

	fake_getgid = TRUE;
	CU_ASSERT_FALSE( drop_privileges( FALSE, &orig ) );
	fake_getgid = FALSE;

	fake_getegid = TRUE;
	CU_ASSERT_FALSE( drop_privileges( FALSE, &orig ) );
	fake_getegid = FALSE;

	fake_getuid = TRUE;
	CU_ASSERT_FALSE( drop_privileges( FALSE, &orig ) );
	fake_getuid = FALSE;

	fake_geteuid = TRUE;
	CU_ASSERT_FALSE( drop_privileges( FALSE, &orig ) );
	fake_geteuid = FALSE;

	fake_getgroups = TRUE;
	CU_ASSERT_FALSE( drop_privileges( FALSE, &orig ) );
	fake_getgroups = FALSE;

	fake_setgroups = TRUE;
	fake_geteuid = TRUE;
	fake_geteuid_ret = 0; /* make it look like we have root privileges */
	CU_ASSERT_FALSE( drop_privileges( FALSE, &orig ) );
	fake_geteuid = FALSE;
	fake_geteuid_ret = -1;
	fake_setgroups = FALSE;

	fake_getgid = TRUE;
	fake_getgid_ret = 20;
	fake_getegid = TRUE;
	fake_getegid_ret = 30;
	CU_ASSERT_FALSE( drop_privileges( FALSE, &orig ) );
	fake_getgid = FALSE;
	fake_getgid_ret = -1;
	fake_getegid = FALSE;
	fake_getegid_ret = -1;

	fake_getgid = TRUE;
	fake_getgid_ret = 20;
	fake_getegid = TRUE;
	fake_getegid_ret = 30;
	CU_ASSERT_FALSE( drop_privileges( TRUE, NULL ) );
	fake_getgid = FALSE;
	fake_getgid_ret = -1;
	fake_getegid = FALSE;
	fake_getegid_ret = -1;

	fake_getgid = TRUE;
	fake_getgid_ret = 20;
	fake_getegid = TRUE;
	fake_getegid_ret = 30;
	fake_setregid = TRUE;
	CU_ASSERT_FALSE( drop_privileges( FALSE, &orig ) );
	fake_setregid = FALSE;
	fake_getgid = FALSE;
	fake_getgid_ret = -1;
	fake_getegid = FALSE;
	fake_getegid_ret = -1;

	fake_getgid = TRUE;
	fake_getgid_ret = 20;
	fake_getegid = TRUE;
	fake_getegid_ret = 30;
	fake_setregid = TRUE;
	fake_setregid_ret = 0;
	CU_ASSERT_FALSE( drop_privileges( FALSE, &orig ) );
	fake_setregid = FALSE;
	fake_setregid_ret = -1;
	fake_getgid = FALSE;
	fake_getgid_ret = -1;
	fake_getegid = FALSE;
	fake_getegid_ret = -1;


	fake_getuid = TRUE;
	fake_getuid_ret = 20;
	fake_geteuid = TRUE;
	fake_geteuid_ret = 30;
	CU_ASSERT_FALSE( drop_privileges( FALSE, &orig ) );
	fake_getuid = FALSE;
	fake_getuid_ret = -1;
	fake_geteuid = FALSE;
	fake_geteuid_ret = -1;

	fake_getuid = TRUE;
	fake_getuid_ret = 20;
	fake_geteuid = TRUE;
	fake_geteuid_ret = 30;
	CU_ASSERT_FALSE( drop_privileges( TRUE, NULL ) );
	fake_getuid = FALSE;
	fake_getuid_ret = -1;
	fake_geteuid = FALSE;
	fake_geteuid_ret = -1;

	fake_getuid = TRUE;
	fake_getuid_ret = 20;
	fake_geteuid = TRUE;
	fake_geteuid_ret = 30;
	fake_setregid = TRUE;
	CU_ASSERT_FALSE( drop_privileges( FALSE, &orig ) );
	fake_setregid = FALSE;
	fake_getuid = FALSE;
	fake_getuid_ret = -1;
	fake_geteuid = FALSE;
	fake_geteuid_ret = -1;


	fake_getgid = TRUE;
	fake_getgid_ret = 20;
	fake_getegid = TRUE;
	fake_getegid_ret = 20;
	CU_ASSERT_TRUE( drop_privileges( TRUE, NULL ) );
	CU_ASSERT_TRUE( drop_privileges( FALSE, &orig ) );
	fake_getgid = FALSE;
	fake_getgid_ret = -1;
	fake_getegid = FALSE;
	fake_getegid_ret = -1;

	fake_getgid = TRUE;
	fake_getgid_ret = 30;
	fake_getegid = TRUE;
	fake_getegid_ret = 20;
	fake_setregid = TRUE;
	fake_setregid_ret = 0;
	CU_ASSERT_FALSE( drop_privileges( TRUE, NULL ) );
	CU_ASSERT_FALSE( drop_privileges( FALSE, &orig ) );
	fake_setregid = FALSE;
	fake_setregid_ret = -1;
	fake_getgid = FALSE;
	fake_getgid_ret = -1;
	fake_getegid = FALSE;
	fake_getegid_ret = -1;

	fake_getgid = TRUE;
	fake_getgid_ret = 30;
	fake_getegid = TRUE;
	fake_getegid_ret = 20;
	fake_setregid = TRUE;
	fake_setregid_ret = 0;
	fake_setegid = TRUE;
	CU_ASSERT_FALSE( drop_privileges( TRUE, NULL ) );
	CU_ASSERT_FALSE( drop_privileges( FALSE, &orig ) );
	fake_setegid = FALSE;
	fake_setregid = FALSE;
	fake_setregid_ret = -1;
	fake_getgid = FALSE;
	fake_getgid_ret = -1;
	fake_getegid = FALSE;
	fake_getegid_ret = -1;

	fake_getgid = TRUE;
	fake_getgid_ret = 30;
	fake_getegid = TRUE;
	fake_getegid_ret = 20;
	fake_setregid = TRUE;
	fake_setregid_ret = 0;
	fake_setegid = TRUE;
	fake_setegid_ret = 0;
	CU_ASSERT_FALSE( drop_privileges( TRUE, NULL ) );
	CU_ASSERT_FALSE( drop_privileges( FALSE, &orig ) );
	fake_setegid = FALSE;
	fake_setegid_ret = -1;
	fake_setregid = FALSE;
	fake_setregid_ret = -1;
	fake_getgid = FALSE;
	fake_getgid_ret = -1;
	fake_getegid = FALSE;
	fake_getegid_ret = -1;


	fake_getuid = TRUE;
	fake_getuid_ret = 20;
	fake_geteuid = TRUE;
	fake_geteuid_ret = 20;
	CU_ASSERT_TRUE( drop_privileges( TRUE, NULL ) );
	CU_ASSERT_TRUE( drop_privileges( FALSE, &orig ) );
	fake_getuid = FALSE;
	fake_getuid_ret = -1;
	fake_geteuid = FALSE;
	fake_geteuid_ret = -1;

	fake_getuid = TRUE;
	fake_getuid_ret = 30;
	fake_geteuid = TRUE;
	fake_geteuid_ret = 20;
	fake_setreuid = TRUE;
	fake_setreuid_ret = 0;
	CU_ASSERT_FALSE( drop_privileges( TRUE, NULL ) );
	CU_ASSERT_FALSE( drop_privileges( FALSE, &orig ) );
	fake_setreuid = FALSE;
	fake_setreuid_ret = -1;
	fake_getuid = FALSE;
	fake_getuid_ret = -1;
	fake_geteuid = FALSE;
	fake_geteuid_ret = -1;

	fake_getuid = TRUE;
	fake_getuid_ret = 30;
	fake_geteuid = TRUE;
	fake_geteuid_ret = 20;
	fake_setreuid = TRUE;
	fake_setreuid_ret = 0;
	fake_seteuid = TRUE;
	CU_ASSERT_FALSE( drop_privileges( TRUE, NULL ) );
	CU_ASSERT_FALSE( drop_privileges( FALSE, &orig ) );
	fake_seteuid = FALSE;
	fake_setreuid = FALSE;
	fake_setreuid_ret = -1;
	fake_getuid = FALSE;
	fake_getuid_ret = -1;
	fake_geteuid = FALSE;
	fake_geteuid_ret = -1;

	fake_getuid = TRUE;
	fake_getuid_ret = 30;
	fake_geteuid = TRUE;
	fake_geteuid_ret = 20;
	fake_setreuid = TRUE;
	fake_setreuid_ret = 0;
	fake_seteuid = TRUE;
	fake_seteuid_ret = 0;
	CU_ASSERT_FALSE( drop_privileges( TRUE, NULL ) );
	CU_ASSERT_FALSE( drop_privileges( FALSE, &orig ) );
	fake_seteuid = FALSE;
	fake_seteuid_ret = -1;
	fake_setreuid = FALSE;
	fake_setreuid_ret = -1;
	fake_getuid = FALSE;
	fake_getuid_ret = -1;
	fake_geteuid = FALSE;
	fake_geteuid_ret = -1;
}

static void test_privileges_restore_failures( void )
{
	priv_state_t orig, tmp;
	MEMSET( &tmp, 0, sizeof( priv_state_t ) );
	MEMSET( &orig, 0, sizeof( priv_state_t ) );

	fake_geteuid = TRUE;
	CU_ASSERT_FALSE( restore_privileges( &orig ) );
	fake_geteuid = FALSE;

	CU_ASSERT_TRUE( drop_privileges( FALSE, &orig ) );
	
	fake_geteuid = TRUE;
	fake_geteuid_ret = 0;
	MEMCPY( &tmp, &orig, sizeof( priv_state_t ) );
	CU_ASSERT_FALSE( restore_privileges( &tmp ) );
	fake_geteuid = FALSE;
	fake_geteuid_ret = -1;

	fake_geteuid = TRUE;
	fake_geteuid_ret = 0;
	fake_seteuid = TRUE;
	MEMCPY( &tmp, &orig, sizeof( priv_state_t ) );
	CU_ASSERT_FALSE( restore_privileges( &tmp ) );
	fake_seteuid = FALSE;
	fake_geteuid = FALSE;
	fake_geteuid_ret = -1;

	fake_geteuid = TRUE;
	fake_geteuid_ret = 0;
	fake_seteuid = TRUE;
	fake_seteuid_ret = 0;
	MEMCPY( &tmp, &orig, sizeof( priv_state_t ) );
	CU_ASSERT_FALSE( restore_privileges( &tmp ) );
	fake_seteuid = FALSE;
	fake_seteuid_ret = -1;
	fake_geteuid = FALSE;
	fake_geteuid_ret = -1;

	fake_getegid = TRUE;
	fake_getegid_ret = 0;
	MEMCPY( &tmp, &orig, sizeof( priv_state_t ) );
	CU_ASSERT_FALSE( restore_privileges( &tmp ) );
	fake_getegid = FALSE;
	fake_getegid_ret = -1;

	fake_getegid = TRUE;
	fake_getegid_ret = 0;
	fake_setegid = TRUE;
	MEMCPY( &tmp, &orig, sizeof( priv_state_t ) );
	CU_ASSERT_FALSE( restore_privileges( &tmp ) );
	fake_setegid = FALSE;
	fake_getegid = FALSE;
	fake_getegid_ret = -1;

	fake_getegid = TRUE;
	fake_getegid_ret = 0;
	fake_setegid = TRUE;
	fake_setegid_ret = 0;
	MEMCPY( &tmp, &orig, sizeof( priv_state_t ) );
	CU_ASSERT_FALSE( restore_privileges( &tmp ) );
	fake_setegid = FALSE;
	fake_setegid_ret = -1;
	fake_getegid = FALSE;
	fake_getegid_ret = -1;
}

static void test_privileges_drop_root( void )
{
	priv_state_t orig;
	MEMSET( &orig, 0, sizeof( priv_state_t ) );

	fake_geteuid = TRUE;
	fake_geteuid_ret = 0; /* make it look like we have root privileges */
	CU_ASSERT_FALSE( drop_privileges( FALSE, &orig ) );

	fake_setgroups = TRUE;
	fake_setgroups_ret = 0;
	CU_ASSERT_FALSE( drop_privileges( FALSE, &orig ) );
	fake_setgroups = FALSE;
	fake_setgroups_ret = -1;
	fake_geteuid = FALSE;
	fake_geteuid_ret = -1;
}

static void test_privileges_restore_root( void )
{
	priv_state_t orig;
	MEMSET( &orig, 0, sizeof( priv_state_t ) );

	CU_ASSERT_TRUE( drop_privileges( FALSE, &orig ) );

	orig.uid = 0;

	fake_geteuid = TRUE;
	fake_geteuid_ret = 0;
	CU_ASSERT_FALSE( restore_privileges( &orig ) );
	fake_geteuid = FALSE;
	fake_geteuid_ret = -1;

	fake_geteuid = TRUE;
	fake_geteuid_ret = 0;
	fake_setgroups = TRUE;
	fake_setgroups_ret = 0;
	CU_ASSERT_TRUE( restore_privileges( &orig ) );
	fake_setgroups = TRUE;
	fake_setgroups_ret = -1;
	fake_geteuid = FALSE;
	fake_geteuid_ret = -1;
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
	ADD_TEST( "privileges drop failures", test_privileges_drop_failures );
	ADD_TEST( "privileges restore failures", test_privileges_restore_failures );
	ADD_TEST( "privileges drop root", test_privileges_drop_root );
	ADD_TEST( "privileges restore root", test_privileges_restore_root );
	
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

