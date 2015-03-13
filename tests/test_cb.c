/* Copyright (c) 2012-2015 David Huseby
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
#include <cutil/cb.h>

#include "test_macros.h"
#include "test_flags.h"

#define REPEAT (128)
#define SIZEMAX (128)
#define MULTIPLE (8)

extern void test_cb_private_functions( void );

static void test_cb_newdel( void )
{
	int i;
	uint32_t size;
	cb_t * cb;

	for ( i = 0; i < REPEAT; i++ )
	{
		cb = cb_new();

		CU_ASSERT_PTR_NOT_NULL_FATAL( cb );

		cb_delete( cb );
	}
}

static void test_cb_fail_alloc( void )
{
	fail_alloc = TRUE;
	CU_ASSERT_PTR_NULL( cb_new() );
	fail_alloc = FALSE;
}

static void test_cb_fail_ht_new( void )
{
	fake_ht_init = TRUE;
	fake_ht_init_ret = FALSE;
	CU_ASSERT_PTR_NULL( cb_new() );
	fake_ht_init = FALSE;
}

static int cb1 = 0;
static int cb2 = 0;

static void callback1(uint8_t const * const name, void * state, void * param)
{
  cb1++;
}

static void callback2(uint8_t const * const name, void * state, void * param)
{
  cb2++;
}

static void test_cb_add( void )
{
  cb_t * cb = cb_new();
  CU_ASSERT_FALSE( cb_add( NULL, NULL, NULL, NULL ) );
  CU_ASSERT_FALSE( cb_add( cb, NULL, NULL, NULL ) );
  CU_ASSERT_FALSE( cb_add( cb, T("foo"), NULL, NULL ) );
  CU_ASSERT_TRUE( cb_add( cb, T("foo"), NULL, &callback1 ) );
  /* can't add it twice */
  CU_ASSERT_FALSE( cb_add( cb, T("foo"), NULL, &callback1 ) );
  /* can add another callback with the same name */
  CU_ASSERT_TRUE( cb_add( cb, T("foo"), NULL, &callback2 ) );
  cb_delete(cb);
}

static void test_cb_call( void )
{
  cb_t * cb = cb_new();
  cb1 = 0;
  cb2 = 0;
  CU_ASSERT_FALSE( cb_call( cb, "foo", NULL ) );
  CU_ASSERT_TRUE( (cb1 == 0) && (cb2 == 0) );
  CU_ASSERT_TRUE( cb_add( cb, T("foo"), NULL, &callback1 ) );
  CU_ASSERT_TRUE( cb_call( cb, "foo", NULL ) );
  CU_ASSERT_TRUE( (cb1 == 1) && (cb2 == 0) );
  CU_ASSERT_TRUE( cb_add( cb, T("foo"), NULL, &callback2 ) );
  CU_ASSERT_TRUE( cb_call( cb, "foo", NULL ) );
  CU_ASSERT_TRUE( (cb1 == 2) && (cb2 == 1) );
  cb_delete(cb);
}

static void test_cb_remove( void )
{
  cb_t * cb = cb_new();
  cb1 = 0;
  cb2 = 0;
  CU_ASSERT_FALSE( cb_call( cb, "foo", NULL ) );
  CU_ASSERT_TRUE( (cb1 == 0) && (cb2 == 0) );
  CU_ASSERT_TRUE( cb_add( cb, T("foo"), NULL, &callback1 ) );
  CU_ASSERT_TRUE( cb_call( cb, "foo", NULL ) );
  CU_ASSERT_TRUE( (cb1 == 1) && (cb2 == 0) );
  CU_ASSERT_TRUE( cb_add( cb, T("foo"), NULL, &callback2 ) );
  CU_ASSERT_TRUE( cb_call( cb, "foo", NULL ) );
  CU_ASSERT_TRUE( (cb1 == 2) && (cb2 == 1) );
  CU_ASSERT_TRUE( cb_remove( cb, T("foo"), NULL, &callback1 ) );
  CU_ASSERT_TRUE( cb_call( cb, "foo", NULL ) );
  CU_ASSERT_TRUE( (cb1 == 2) && (cb2 == 2) );
  cb_delete(cb);
}

static int init_cb_suite( void )
{
	srand(0xDEADBEEF);
	reset_test_flags();
	return 0;
}

static int deinit_cb_suite( void )
{
	reset_test_flags();
	return 0;
}

static CU_pSuite add_cb_tests( CU_pSuite pSuite )
{
	ADD_TEST( "new/delete of cb", test_cb_newdel );
  ADD_TEST( "cb new fail alloc", test_cb_fail_alloc );
  ADD_TEST( "cb new fail ht new", test_cb_fail_ht_new );
  ADD_TEST( "cb test add", test_cb_add );
  ADD_TEST( "cb test call", test_cb_call );
  ADD_TEST( "cb test remove", test_cb_remove );
	ADD_TEST( "cb private functions", test_cb_private_functions );

	return pSuite;
}

CU_pSuite add_cb_test_suite()
{
	CU_pSuite pSuite = NULL;

	/* add the suite to the registry */
	pSuite = CU_add_suite("Callback Tests", init_cb_suite, deinit_cb_suite);
	CHECK_PTR_RET( pSuite, NULL );

	/* add in cb specific tests */
	CHECK_PTR_RET( add_cb_tests( pSuite ), NULL );

	return pSuite;
}

