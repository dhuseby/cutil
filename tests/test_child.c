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
#include <cutil/child.h>

#define REPEAT (128)
#define SIZEMAX (128)
#define MULTIPLE (8)

static evt_loop_t * el = NULL;

static int exit_fn( child_process_t * const cp, int rpid, int rstatus, void * user_data )
{
	evt_stop( el );
	return TRUE;
}

static int32_t read_fn( child_process_t * const cp, size_t nread, void * user_data )
{
	return 0;
}

static int32_t write_fn( child_process_t * const cp, uint8_t const * const buffer, void * user_data )
{
	return 0;
}

static void test_child_newdel( void )
{
	int i;
	uint32_t size;
	int8_t const * const args[] = { "./child.sh", NULL };
	int8_t const * const env[] = { NULL };
	child_process_t * child;
	child_ops_t ops = { &exit_fn, &read_fn, &write_fn };

	for ( i = 0; i < REPEAT; i++ )
	{
		child = NULL;
		child = child_process_new( "./child.sh", args, env, &ops, el, NULL );

		/* run the event loop */
		evt_run( el );

		CU_ASSERT_PTR_NOT_NULL( child );

		child_process_delete( child, TRUE );
	}
}

static void test_child_newdel_fail_first_pipe( void )
{
	int i;
	uint32_t size;
	int8_t const * const args[] = { "./child.sh", NULL };
	int8_t const * const env[] = { NULL };
	child_process_t * child;
	child_ops_t ops = { &exit_fn, &read_fn, &write_fn };

	for ( i = 0; i < REPEAT; i++ )
	{
		child_process_set_num_good_pipes( 0 );
		child = NULL;
		child = child_process_new( "./child.sh", args, env, &ops, el, NULL );
		child_process_set_num_good_pipes( -1 );
		
		CU_ASSERT_PTR_NULL( child );
	}
}

static void test_child_newdel_fail_second_pipe( void )
{
	int i;
	uint32_t size;
	int8_t const * const args[] = { "./child.sh", NULL };
	int8_t const * const env[] = { NULL };
	child_process_t * child;
	child_ops_t ops = { &exit_fn, &read_fn, &write_fn };

	for ( i = 0; i < REPEAT; i++ )
	{
		child_process_set_num_good_pipes( 1 );
		child = NULL;
		child = child_process_new( "./child.sh", args, env, &ops, el, NULL );
		child_process_set_num_good_pipes( -1 );
		
		CU_ASSERT_PTR_NULL( child );
	}
}

static void test_child_newdel_fail_fork( void )
{
	int i;
	uint32_t size;
	int8_t const * const args[] = { "./child.sh", NULL };
	int8_t const * const env[] = { NULL };
	child_process_t * child;
	child_ops_t ops = { &exit_fn, &read_fn, &write_fn };

	for ( i = 0; i < REPEAT; i++ )
	{
		child_process_set_fail_fork( TRUE );
		child = NULL;
		child = child_process_new( "./child.sh", args, env, &ops, el, NULL );
		child_process_set_fail_fork( FALSE );

		CU_ASSERT_PTR_NULL( child );
	}
}

static void test_child_wait( void )
{
	int i;
	uint32_t size;
	int8_t const * const args[] = { "./child_sleep.sh", NULL };
	int8_t const * const env[] = { NULL };
	child_process_t * child;
	child_ops_t ops = { &exit_fn, &read_fn, &write_fn };

	child = NULL;
	child = child_process_new( "./child_sleep.sh", args, env, &ops, el, NULL );

	/* run the event loop */
	evt_run( el );

	CU_ASSERT_PTR_NOT_NULL( child );

	child_process_delete( child, TRUE );
}

static pid_t child_pid = -1;

static int32_t read_pid_fn( child_process_t * const cp, size_t nread, void * user_data )
{
	NOTICE("read_fn callback\n");
	child_process_read( cp, (uint8_t * const)&child_pid, sizeof(pid_t) );
	return 0;
}

static int32_t write_pid_fn( child_process_t * const cp, uint8_t const * const buffer, void * user_data )
{
	return 0;
}

static void test_child_read( void )
{
	int i;
	uint32_t size;
	pid_t cpid;
	int8_t const * const args[] = { "./child_pid.sh", NULL };
	int8_t const * const env[] = { NULL };
	child_process_t * child;
	child_ops_t ops = { &exit_fn, &read_pid_fn, &write_pid_fn };

	for ( i = 0; i < REPEAT; i++ )
	{
		child_pid = -1;
		cpid = -1;
		child = NULL;
		child = child_process_new( "./child_pid.sh", args, env, &ops, el, NULL );

		cpid = child_process_get_pid( child );

		/* run the event loop */
		evt_run( el );

		CU_ASSERT_PTR_NOT_NULL( child );
		CU_ASSERT_EQUAL( child_pid, cpid );

		child_process_delete( child, TRUE );
	}
}


static int init_child_suite( void )
{
	srand(0xDEADBEEF);

	/* set up the event loop */
	el = evt_new();

	return 0;
}

static int deinit_child_suite( void )
{
	/* take down the event loop */
	evt_delete( el );
	el = NULL;

	return 0;
}

static CU_pSuite add_child_tests( CU_pSuite pSuite )
{
	CHECK_PTR_RET( CU_add_test( pSuite, "new/delete of child process", test_child_newdel), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "new/delete of child process fail first pipe", test_child_newdel_fail_first_pipe), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "new/delete of child process fail second pipe", test_child_newdel_fail_second_pipe), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "new/delete of child process fail fork", test_child_newdel_fail_fork), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "wait on child process", test_child_wait), NULL );
	/*CHECK_PTR_RET( CU_add_test( pSuite, "test read from child process", test_child_read), NULL );*/
	return pSuite;
}

CU_pSuite add_child_test_suite()
{
	CU_pSuite pSuite = NULL;

	/* add the suite to the registry */
	pSuite = CU_add_suite("Child Process Tests", init_child_suite, deinit_child_suite);
	CHECK_PTR_RET( pSuite, NULL );

	/* add in child specific tests */
	CHECK_PTR_RET( add_child_tests( pSuite ), NULL );

	return pSuite;
}

