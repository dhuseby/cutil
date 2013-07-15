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

static int_t read_evt_fn( aiofd_t * const aiofd, size_t nread, void * user_data )
{
    return FALSE;
}

static int_t write_evt_fn( aiofd_t * const aiofd, uint8_t const * const buffer, void * user_data )
{
    return FALSE;
}

static int_t error_evt_fn( aiofd_t * const aiofd, int err, void * user_data )
{
    return FALSE;
}

static ssize_t read_fn(int fd, void *buf, size_t count, void * user_data)
{
    return -1;
}

static ssize_t write_fn(int fd, const void *buf, size_t count, void * user_data)
{
    return -1;
}

static ssize_t readv_fn(int fd, struct iovec *iov, int iovcnt, void * user_data)
{
    return -1;
}

static ssize_t writev_fn(int fd, const struct iovec *iov, int iovcnt, void * user_data)
{
    return -1;
}

static void test_aiofd_newdel( void )
{
	int i;
	aiofd_t * aiofd;
	aiofd_ops_t ops = { &read_evt_fn, &write_evt_fn, &error_evt_fn, NULL, NULL, NULL, NULL };

	/* make sure there is an event loop */
	CU_ASSERT_PTR_NOT_NULL( el );

	for ( i = 0; i < REPEAT; i++ )
	{
		aiofd = NULL;
		aiofd = aiofd_new( fileno(stdout), fileno(stdin), &ops, el, NULL );

		CU_ASSERT_PTR_NOT_NULL_FATAL( aiofd );
		CU_ASSERT_EQUAL( aiofd->ops.read_evt_fn, read_evt_fn );
		CU_ASSERT_EQUAL( aiofd->ops.write_evt_fn, write_evt_fn );
		CU_ASSERT_EQUAL( aiofd->ops.error_evt_fn, error_evt_fn );

		aiofd_delete( aiofd );
	}
}

static void test_aiofd_initdeinit( void )
{
	int i;
	aiofd_t aiofd;
	aiofd_ops_t ops = { &read_evt_fn, &write_evt_fn, &error_evt_fn, NULL, NULL, NULL, NULL };

	/* make sure there is an event loop */
	CU_ASSERT_PTR_NOT_NULL( el );

	for ( i = 0; i < REPEAT; i++ )
	{
		CU_ASSERT_TRUE( aiofd_initialize( &aiofd, fileno(stdout), fileno(stdin), &ops, el, NULL ) );

		CU_ASSERT_EQUAL( aiofd.ops.read_evt_fn, read_evt_fn );
		CU_ASSERT_EQUAL( aiofd.ops.write_evt_fn, write_evt_fn );
		CU_ASSERT_EQUAL( aiofd.ops.error_evt_fn, error_evt_fn );

		aiofd_deinitialize( &aiofd );
	}
}

static void test_aiofd_new_fail_alloc( void )
{
	int i;
	aiofd_ops_t ops = { &read_evt_fn, &write_evt_fn, &error_evt_fn, NULL, NULL, NULL, NULL };

	/* make sure there is an event loop */
	CU_ASSERT_PTR_NOT_NULL( el );

	fail_alloc = TRUE;
	for ( i = 0; i < REPEAT; i++ )
	{
		CU_ASSERT_PTR_NULL( aiofd_new( fileno(stdout), fileno(stdin), &ops, el, NULL ) )
	}
	fail_alloc = FALSE;
}

static void test_aiofd_new_fail_init( void )
{
	int i;
	aiofd_t * aiofd;
	aiofd_ops_t ops = { &read_evt_fn, &write_evt_fn, &error_evt_fn, NULL, NULL, NULL, NULL };

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
	aiofd_ops_t ops = { &read_evt_fn, &write_evt_fn, &error_evt_fn, NULL, NULL, NULL, NULL };

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
	aiofd_ops_t ops = { &read_evt_fn, &write_evt_fn, &error_evt_fn, NULL, NULL, NULL, NULL };

	/* make sure there is an event loop */
	CU_ASSERT_PTR_NOT_NULL( el );

	CU_ASSERT_TRUE( aiofd_initialize( &aiofd, fileno(stdout), fileno(stdin), &ops, el, NULL ) );
	CU_ASSERT_FALSE( aiofd_enable_write_evt( NULL, TRUE ) );
	CU_ASSERT_FALSE( aiofd_enable_write_evt( NULL, FALSE ) );
	CU_ASSERT_TRUE( aiofd_enable_write_evt( &aiofd, TRUE ) );
	CU_ASSERT_TRUE( aiofd_enable_write_evt( &aiofd, FALSE ) );
	fake_event_stop_handler = TRUE;
	fake_event_stop_handler_ret = FALSE;
	CU_ASSERT_FALSE( aiofd_enable_write_evt( &aiofd, FALSE ) );
	fake_event_stop_handler = FALSE;
	aiofd_deinitialize( &aiofd );
}

static void test_aiofd_start_stop_read( void )
{
	int i;
	aiofd_t aiofd;
	aiofd_ops_t ops = { &read_evt_fn, &write_evt_fn, &error_evt_fn, NULL, NULL, NULL, NULL };

	/* make sure there is an event loop */
	CU_ASSERT_PTR_NOT_NULL( el );

	CU_ASSERT_TRUE( aiofd_initialize( &aiofd, fileno(stdout), fileno(stdin), &ops, el, NULL ) );
	CU_ASSERT_FALSE( aiofd_enable_read_evt( NULL, TRUE ) );
	CU_ASSERT_FALSE( aiofd_enable_read_evt( NULL, FALSE ) );
	CU_ASSERT_TRUE( aiofd_enable_read_evt( &aiofd, TRUE ) );
	CU_ASSERT_TRUE( aiofd_enable_read_evt( &aiofd, FALSE ) );
	fake_event_stop_handler = TRUE;
	fake_event_stop_handler_ret = FALSE;
	CU_ASSERT_FALSE( aiofd_enable_read_evt( &aiofd, FALSE ) );
	fake_event_stop_handler = FALSE;
	aiofd_deinitialize( &aiofd );
}

static void test_aiofd_delete_null( void )
{
	aiofd_delete( NULL );
}

static void test_aiofd_init_prereqs( void )
{
	aiofd_t aiofd;
	aiofd_ops_t ops = { &read_evt_fn, &write_evt_fn, &error_evt_fn, NULL, NULL, NULL, NULL };
	MEMSET( &aiofd, 0, sizeof( aiofd_t ) );

	CU_ASSERT_FALSE( aiofd_initialize( NULL, -1, -1, NULL, NULL, NULL ) );
	CU_ASSERT_FALSE( aiofd_initialize( &aiofd, -1, -1, NULL, NULL, NULL ) );
	CU_ASSERT_FALSE( aiofd_initialize( &aiofd, STDOUT_FILENO, -1, NULL, NULL, NULL ) );
	CU_ASSERT_FALSE( aiofd_initialize( &aiofd, STDOUT_FILENO, STDIN_FILENO, NULL, NULL, NULL ) );
	CU_ASSERT_FALSE( aiofd_initialize( &aiofd, STDOUT_FILENO, STDIN_FILENO, &ops, NULL, NULL ) );
	CU_ASSERT_TRUE( aiofd_initialize( &aiofd, STDOUT_FILENO, STDIN_FILENO, &ops, el, NULL ) );
	aiofd_deinitialize( &aiofd );
}

static void test_aiofd_init_fail_list_init( void )
{
	aiofd_t aiofd;
	aiofd_ops_t ops = { &read_evt_fn, &write_evt_fn, &error_evt_fn, NULL, NULL, NULL, NULL };
	MEMSET( &aiofd, 0, sizeof( aiofd_t ) );

	fake_list_init = TRUE;
	fake_list_init_ret = FALSE;
	CU_ASSERT_FALSE( aiofd_initialize( &aiofd, STDOUT_FILENO, STDIN_FILENO, &ops, el, NULL ) );
	fake_list_init = FALSE;
}

static void test_aiofd_writev( void )
{
	aiofd_t aiofd;
	int8_t * buf = "foo";
	int const size = 4;
	struct iovec iov;

	MEMSET( &aiofd, 0, sizeof( aiofd_t ) );
	MEMSET( &iov, 0, sizeof( struct iovec) );

	iov.iov_base = (void*)buf;
	iov.iov_len = size;

	fake_aiofd_write_common = TRUE;
	fake_aiofd_write_common_ret = FALSE;
	CU_ASSERT_FALSE( aiofd_writev( &aiofd, &iov, 1 ) );
	fake_aiofd_write_common = FALSE;
}

static void test_aiofd_read( void )
{
	aiofd_t aiofd;
	aiofd_ops_t ops = { &read_evt_fn, &write_evt_fn, &error_evt_fn, NULL, NULL, NULL, NULL };
	uint8_t buf[10];
	MEMSET( &aiofd, 0, sizeof( aiofd_t ) );

	CU_ASSERT_EQUAL( aiofd_read( NULL, NULL, 0 ), -1 );
	CU_ASSERT_EQUAL( aiofd_read( &aiofd, NULL, 0 ), -1 );
	CU_ASSERT_EQUAL( aiofd_read( &aiofd, buf, 0 ), -1 );

    /* sets up low level read/write/readv/writev function pointers to interal ones */
    CU_ASSERT_TRUE( aiofd_initialize( &aiofd, STDOUT_FILENO, STDIN_FILENO, &ops, el, NULL ) );
	fake_read = TRUE;
	fake_read_ret = 10;
	CU_ASSERT_EQUAL( aiofd_read( &aiofd, buf, 10 ), 10 );
	fake_read = TRUE;
	fake_read_ret = 0;
	CU_ASSERT_EQUAL( aiofd_read( &aiofd, buf, 10 ), -1 );
	CU_ASSERT_EQUAL( errno, EPIPE );
	fake_read = TRUE;
	fake_read_ret = -1;
	aiofd.ops.error_evt_fn = &error_evt_fn;
	CU_ASSERT_EQUAL( aiofd_read( &aiofd, buf, 10 ), -1 );
	fake_errno = TRUE;
	fake_errno_value = EPIPE;
	CU_ASSERT_EQUAL( aiofd_read( &aiofd, buf, 10 ), -1 );
	fake_errno_value = 0;
	fake_errno = FALSE;
	fake_read = FALSE;
}

static void test_aiofd_flush_null( void )
{
	CU_ASSERT_FALSE( aiofd_flush( NULL ) );
}

static void test_aiofd_set_listen_null( void )
{
	CU_ASSERT_FALSE( aiofd_set_listen( NULL, TRUE ) );
}

static void test_aiofd_get_listen_null( void )
{
	CU_ASSERT_FALSE( aiofd_get_listen( NULL ) );
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
	ADD_TEST( "new of aiofd fail alloc", test_aiofd_new_fail_alloc );
	ADD_TEST( "fail init of aiofd", test_aiofd_new_fail_init );
	ADD_TEST( "fail aiofd evt init", test_aiofd_init_fail_evt_init );
	ADD_TEST( "aiofd write start/stop", test_aiofd_start_stop_write );
	ADD_TEST( "aiofd read start/stop", test_aiofd_start_stop_read );
	ADD_TEST( "aiofd delete NULL", test_aiofd_delete_null );
	ADD_TEST( "aiofd init prereqs", test_aiofd_init_prereqs );
	ADD_TEST( "aiofd init fail list init", test_aiofd_init_fail_list_init );
	ADD_TEST( "aiofd writev", test_aiofd_writev );
	ADD_TEST( "aiofd read", test_aiofd_read );
	ADD_TEST( "aiofd flush null", test_aiofd_flush_null );
	ADD_TEST( "aiofd set listen null", test_aiofd_set_listen_null );
	ADD_TEST( "aiofd get listen null", test_aiofd_get_listen_null );

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

