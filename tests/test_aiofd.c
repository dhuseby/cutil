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
#include <cutil/aiofd.h>

#include "test_macros.h"
#include "test_flags.h"

#define REPEAT (128)
#define SIZEMAX (128)
#define MULTIPLE (8)

extern evt_loop_t * el;
extern void test_aiofd_private_functions(void);

static int read_fn( aiofd_t * const aiofd, size_t nread, void * user_data )
{
	return FALSE;
}

static int write_fn( aiofd_t * const aiofd, uint8_t const * const buffer, void * user_data )
{
	return FALSE;
}

static int error_fn( aiofd_t * const aiofd, int err, void * user_data )
{
	return TRUE;
}

static void test_aiofd_newdel( void )
{
	int i;
	aiofd_t * aiofd;
	aiofd_ops_t ops = { &read_fn, &write_fn, &error_fn };

	/* make sure there is an event loop */
	CU_ASSERT_PTR_NOT_NULL( el );

	for ( i = 0; i < REPEAT; i++ )
	{
		aiofd = NULL;
		aiofd = aiofd_new( fileno(stdout), fileno(stdin), &ops, el, NULL );

		CU_ASSERT_PTR_NOT_NULL_FATAL( aiofd );
		CU_ASSERT_EQUAL( aiofd->ops.read_fn, read_fn );
		CU_ASSERT_EQUAL( aiofd->ops.write_fn, write_fn );
		CU_ASSERT_EQUAL( aiofd->ops.error_fn, error_fn );

		aiofd_delete( aiofd );
	}
}

static void test_aiofd_initdeinit( void )
{
	int i;
	aiofd_t aiofd;
	aiofd_ops_t ops = { &read_fn, &write_fn, &error_fn };

	/* make sure there is an event loop */
	CU_ASSERT_PTR_NOT_NULL( el );

	for ( i = 0; i < REPEAT; i++ )
	{
		CU_ASSERT_TRUE( aiofd_initialize( &aiofd, fileno(stdout), fileno(stdin), &ops, el, NULL ) );

		CU_ASSERT_EQUAL( aiofd.ops.read_fn, read_fn );
		CU_ASSERT_EQUAL( aiofd.ops.write_fn, write_fn );
		CU_ASSERT_EQUAL( aiofd.ops.error_fn, error_fn );

		aiofd_deinitialize( &aiofd );
	}
}

static void test_aiofd_new_fail_init( void )
{
	int i;
	aiofd_t * aiofd;
	aiofd_ops_t ops = { &read_fn, &write_fn, &error_fn };

	/* make sure there is an event loop */
	CU_ASSERT_PTR_NOT_NULL( el );

	fake_aiofd_initialize = TRUE;
	fake_aiofd_initialize_ret = FALSE;
	for ( i = 0; i < REPEAT; i++ )
	{
		aiofd = NULL;
		aiofd = aiofd_new( fileno(stdout), fileno(stdin), &ops, el, NULL );

		CU_ASSERT_PTR_NULL( aiofd );
	}
	fake_aiofd_initialize = FALSE;
}

static void test_aiofd_init_fail_evt_init( void )
{
	int i;
	aiofd_t aiofd;
	aiofd_ops_t ops = { &read_fn, &write_fn, &error_fn };

	/* make sure there is an event loop */
	CU_ASSERT_PTR_NOT_NULL( el );

	fake_event_handler_init = TRUE;
	fake_event_handler_init_ret = FALSE;
	for ( i = 0; i < REPEAT; i++ )
	{
		fake_event_handler_init_count = 1;
		CU_ASSERT_FALSE( aiofd_initialize( &aiofd, fileno(stdout), fileno(stdin), &ops, el, NULL ) );
	}
	fake_event_handler_init_count = 0;
	for ( i = 0; i < REPEAT; i++ )
	{
		CU_ASSERT_FALSE( aiofd_initialize( &aiofd, fileno(stdout), fileno(stdin), &ops, el, NULL ) );
	}
	fake_event_handler_init = FALSE;
}

static void test_aiofd_start_stop_write( void )
{
	int i;
	aiofd_t aiofd;
	aiofd_ops_t ops = { &read_fn, &write_fn, &error_fn };

	/* make sure there is an event loop */
	CU_ASSERT_PTR_NOT_NULL( el );

	CU_ASSERT_TRUE( aiofd_initialize( &aiofd, fileno(stdout), fileno(stdin), &ops, el, NULL ) );
	CU_ASSERT_TRUE( aiofd_enable_write_evt( &aiofd, TRUE ) );
	CU_ASSERT_TRUE( aiofd_enable_write_evt( &aiofd, FALSE ) );
	aiofd_deinitialize( &aiofd );
}

static void test_aiofd_start_stop_read( void )
{
	int i;
	aiofd_t aiofd;
	aiofd_ops_t ops = { &read_fn, &write_fn, &error_fn };

	/* make sure there is an event loop */
	CU_ASSERT_PTR_NOT_NULL( el );

	CU_ASSERT_TRUE( aiofd_initialize( &aiofd, fileno(stdout), fileno(stdin), &ops, el, NULL ) );
	CU_ASSERT_TRUE( aiofd_enable_read_evt( &aiofd, TRUE ) );
	CU_ASSERT_TRUE( aiofd_enable_read_evt( &aiofd, FALSE ) );
	aiofd_deinitialize( &aiofd );
}

static int init_aiofd_suite( void )
{
	srand(0xDEADBEEF);
	reset_test_flags();
	return 0;
}

static int deinit_aiofd_suite( void )
{
	reset_test_flags();
	return 0;
}

static CU_pSuite add_aiofd_tests( CU_pSuite pSuite )
{
	ADD_TEST( "new/delete of aiofd", test_aiofd_newdel );
	ADD_TEST( "init/deinit of aiofd", test_aiofd_initdeinit );
	ADD_TEST( "fail init of aiofd", test_aiofd_new_fail_init );
	ADD_TEST( "fail aiofd evt init", test_aiofd_init_fail_evt_init );
	ADD_TEST( "aiofd write start/stop", test_aiofd_start_stop_write );
	ADD_TEST( "aiofd read start/stop", test_aiofd_start_stop_read );

	ADD_TEST( "test aiofd private functions", test_aiofd_private_functions );
	return pSuite;
}

CU_pSuite add_aiofd_test_suite()
{
	CU_pSuite pSuite = NULL;

	/* add the suite to the registry */
	pSuite = CU_add_suite("Async I/O fd Tests", init_aiofd_suite, deinit_aiofd_suite);
	CHECK_PTR_RET( pSuite, NULL );

	/* add in aiofd specific tests */
	CHECK_PTR_RET( add_aiofd_tests( pSuite ), NULL );

	return pSuite;
}

