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

#include "test_macros.h"
#include "test_flags.h"

#define REPEAT (1)
#define SIZEMAX (128)
#define MULTIPLE (8)

extern evt_loop_t * el;
extern void test_child_private_functions( void );

static int exit_fn( child_process_t * const cp, int rpid, int rstatus, void * user_data )
{
	evt_stop( el, TRUE );
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
		child = child_process_new( "./child.sh", args, env, &ops, el, TRUE, NULL );

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
		fake_pipe = TRUE;
		fake_pipe_ret = -1;
		child = NULL;
		child = child_process_new( "./child.sh", args, env, &ops, el, TRUE, NULL );
		fake_pipe = FALSE;
		fake_pipe_ret = 0;
		
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
		fake_pipe = TRUE;
		fake_pipe_ret = -1;
		child = NULL;
		child = child_process_new( "./child.sh", args, env, &ops, el, TRUE, NULL );
		fake_pipe = FALSE;
		fake_pipe_ret = 0;
		
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
		fake_fork = TRUE;
		fake_fork_ret = -1;
		child = NULL;
		child = child_process_new( "./child.sh", args, env, &ops, el, TRUE, NULL );
		fake_fork = FALSE;
		fake_fork_ret = 0;

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
	child = child_process_new( "./child.sh", args, env, &ops, el, TRUE, NULL );

	/* run the event loop */
	evt_run( el );

	CU_ASSERT_PTR_NOT_NULL( child );

	child_process_delete( child, TRUE );
}

static buf[16];
static pid_t child_pid = -1;

static int exit_pid_fn( child_process_t * const cp, int rpid, int rstatus, void * user_data )
{
	evt_stop( el, FALSE );
	return TRUE;
}

/* this will get called whenever there is data to be read */
static int32_t read_pid_fn( child_process_t * const cp, size_t nread, void * user_data )
{
	MEMSET( buf, 0, 16 );
	child_process_read( cp, (uint8_t * const)buf, (nread < 16) ? nread : 16 );
	child_pid = (pid_t)atoi(C(buf));
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
	child_ops_t ops = { &exit_pid_fn, &read_pid_fn, &write_pid_fn };

	for ( i = 0; i < REPEAT; i++ )
	{
		child_pid = -1;
		cpid = -1;
		child = NULL;
		child = child_process_new( "./child_pid.sh", args, env, &ops, el, TRUE, NULL );

		CU_ASSERT_PTR_NOT_NULL( child );

		cpid = child_process_get_pid( child );

		CU_ASSERT_NOT_EQUAL( cpid, 0 );

		/* run the event loop */
		evt_run( el );

		CU_ASSERT_EQUAL( child_pid, cpid );

		child_process_delete( child, TRUE );
	}
}

void test_child_process_write( void )
{
	CU_ASSERT_FALSE( child_process_write( NULL, NULL, 0 ) );
}

void test_child_process_writev( void )
{
	CU_ASSERT_FALSE( child_process_writev( NULL, NULL, 0 ) );
}

void test_child_process_flush( void )
{
	CU_ASSERT_FALSE( child_process_flush( NULL ) );
}


static int init_child_suite( void )
{
	srand(0xDEADBEEF);
	reset_test_flags();
	return 0;
}

static int deinit_child_suite( void )
{
	reset_test_flags();
	return 0;
}

static CU_pSuite add_child_tests( CU_pSuite pSuite )
{
	ADD_TEST( "new/delete of child process", test_child_newdel);
	ADD_TEST( "new/delete of child process fail first pipe", test_child_newdel_fail_first_pipe);
	ADD_TEST( "new/delete of child process fail second pipe", test_child_newdel_fail_second_pipe);
	ADD_TEST( "new/delete of child process fail fork", test_child_newdel_fail_fork);
	ADD_TEST( "wait on child process", test_child_wait);
	ADD_TEST( "test read from child process", test_child_read);
	ADD_TEST( "test child process write", test_child_process_write );
	ADD_TEST( "test child process writev", test_child_process_writev );
	ADD_TEST( "test child process flush", test_child_process_flush );

	ADD_TEST( "child process private functions", test_child_private_functions );
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

