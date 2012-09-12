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
#include <signal.h>

#include <CUnit/Basic.h>

#include <cutil/debug.h>
#include <cutil/macros.h>
#include <cutil/events.h>

#include "test_macros.h"
#include "test_flags.h"

#define REPEAT (128)
#define SIZEMAX (128)
#define MULTIPLE (8)

extern evt_loop_t * el;
extern void test_events_private_functions( void );

static void test_evt_new_failure( void )
{
	fake_ev_default_loop = TRUE;
	CU_ASSERT_PTR_NULL( evt_new() );
	fake_ev_default_loop = FALSE;
}

static void test_evt_delete_null( void )
{
	evt_delete( NULL );
}

static void test_initialize_event_handler( void )
{
	evt_t evt;
	evt_params_t params;
	MEMSET( &evt, 0, sizeof(evt_t) );
	MEMSET( &params, 0, sizeof(evt_params_t) );

	CU_ASSERT_FALSE( evt_initialize_event_handler( NULL, (evt_type_t)50, NULL, NULL, NULL ) );
	CU_ASSERT_FALSE( evt_initialize_event_handler( &evt, (evt_type_t)50, NULL, NULL, NULL ) );
	CU_ASSERT_FALSE( evt_initialize_event_handler( &evt, EVT_SIGNAL, NULL, NULL, NULL ) );

	MEMSET( &evt, 0, sizeof(evt_t) );
	CU_ASSERT_TRUE( evt_initialize_event_handler( &evt, EVT_SIGNAL, &params, NULL, NULL ) );
	CU_ASSERT_PTR_NULL( evt.callback );
	CU_ASSERT_PTR_NULL( evt.user_data );
	CU_ASSERT_PTR_NULL( evt.el );
	CU_ASSERT_EQUAL( evt.evt_type, EVT_SIGNAL );

	MEMSET( &evt, 0, sizeof(evt_t) );
	CU_ASSERT_TRUE( evt_initialize_event_handler( &evt, EVT_CHILD, &params, NULL, NULL ) );
	CU_ASSERT_PTR_NULL( evt.callback );
	CU_ASSERT_PTR_NULL( evt.user_data );
	CU_ASSERT_PTR_NULL( evt.el );
	CU_ASSERT_EQUAL( evt.evt_type, EVT_CHILD );

	MEMSET( &evt, 0, sizeof(evt_t) );
	CU_ASSERT_TRUE( evt_initialize_event_handler( &evt, EVT_IO, &params, NULL, NULL ) );
	CU_ASSERT_PTR_NULL( evt.callback );
	CU_ASSERT_PTR_NULL( evt.user_data );
	CU_ASSERT_PTR_NULL( evt.el );
	CU_ASSERT_EQUAL( evt.evt_type, EVT_IO );
}

static void test_new_event_handler( void )
{
	evt_t * evt = NULL;
	fail_alloc = TRUE;
	CU_ASSERT_PTR_NULL( evt_new_event_handler( EVT_SIGNAL, NULL, NULL, NULL ) );
	fail_alloc = FALSE;

	fake_event_handler_init = TRUE;
	fake_event_handler_init_ret = FALSE;
	CU_ASSERT_PTR_NULL( evt_new_event_handler( EVT_SIGNAL, NULL, NULL, NULL ) );
	fake_event_handler_init_ret = TRUE;
	evt = evt_new_event_handler( EVT_SIGNAL, NULL, NULL, NULL );
	fake_event_handler_init_ret = FALSE;
	fake_event_handler_init = FALSE;
	evt_delete_event_handler( evt );
}

static void test_delete_event_handler( void )
{
	evt_t * evt = NULL;
	evt_delete_event_handler( NULL );
	evt = CALLOC( 1, sizeof(evt_t) );
	evt_delete_event_handler( evt );
}


static int test_flag = FALSE;

static evt_ret_t test_event_callback( evt_loop_t * const el, evt_t * const evt,
									  evt_params_t * const params, void * user_data )
{
	*((int*)user_data) = TRUE;
	evt_stop_event_handler( el, evt );
	evt_stop( el, FALSE );
}

static void test_start_event_handler( void )
{
	evt_t evt;
	evt_params_t params;
	struct itimerval timer_value;
	MEMSET( &evt, 0, sizeof( evt_t ) );
	MEMSET( &params, 0, sizeof( evt_params_t ) );
	MEMSET( &timer_value, 0, sizeof( struct itimerval ) );

	CU_ASSERT_EQUAL( evt_start_event_handler( NULL, NULL ), EVT_BADPTR );
	CU_ASSERT_EQUAL( evt_start_event_handler( el, NULL ), EVT_BADPTR );
	evt.evt_type = (evt_type_t)50;
	CU_ASSERT_EQUAL( evt_start_event_handler( el, &evt ), EVT_BADPARAM );

	MEMSET( &evt, 0, sizeof( evt_t ) );
	params.signal_params.signum = SIGALRM;
	test_flag = FALSE;
	CU_ASSERT_TRUE( evt_initialize_event_handler( &evt, EVT_SIGNAL, &params, &test_event_callback, &test_flag ) );
	CU_ASSERT_EQUAL( evt_start_event_handler( el, &evt), EVT_OK );
	timer_value.it_value.tv_sec = 1;
	timer_value.it_interval.tv_sec = 1;
	CU_ASSERT_EQUAL( setitimer( ITIMER_REAL, &timer_value, NULL ), 0 );
	evt_run( el );
	CU_ASSERT_TRUE( test_flag );
	test_flag = FALSE;
	MEMSET( &timer_value, 0, sizeof( struct itimerval ) );
	CU_ASSERT_EQUAL( setitimer( ITIMER_REAL, &timer_value, NULL ), 0 );
}

static void test_stop_event_handler( void )
{
	CU_ASSERT_EQUAL( evt_stop_event_handler( NULL, NULL ), EVT_BADPTR );
	CU_ASSERT_EQUAL( evt_stop_event_handler( el, NULL ), EVT_BADPTR );
}

static void test_evt_run_null( void )
{
	CU_ASSERT_EQUAL( evt_run( NULL ), EVT_BADPTR );
}

static void test_evt_stop_null( void )
{
	CU_ASSERT_EQUAL( evt_stop( NULL, FALSE ), EVT_BADPTR );
}

static int init_events_suite( void )
{
	srand(0xDEADBEEF);
	reset_test_flags();
	return 0;
}

static int deinit_events_suite( void )
{
	reset_test_flags();
	return 0;
}

static CU_pSuite add_events_tests( CU_pSuite pSuite )
{
	ADD_TEST( "evt new failure", test_evt_new_failure );
	ADD_TEST( "evt delete null", test_evt_delete_null );
	ADD_TEST( "test init event handler", test_initialize_event_handler );
	ADD_TEST( "test new event handler", test_new_event_handler );
	ADD_TEST( "test delete event handler", test_delete_event_handler );
	ADD_TEST( "test start event handler", test_start_event_handler );
	ADD_TEST( "test stop event handler", test_stop_event_handler );
	ADD_TEST( "test evt run null", test_evt_run_null );
	ADD_TEST( "test evt stop null", test_evt_stop_null );

	ADD_TEST( "events private functions", test_events_private_functions );
	return pSuite;
}

CU_pSuite add_events_test_suite()
{
	CU_pSuite pSuite = NULL;

	/* add the suite to the registry */
	pSuite = CU_add_suite("Events Tests", init_events_suite, deinit_events_suite);
	CHECK_PTR_RET( pSuite, NULL );

	/* add in specific tests */
	CHECK_PTR_RET( add_events_tests( pSuite ), NULL );

	return pSuite;
}

