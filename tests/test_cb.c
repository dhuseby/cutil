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

/*
 * CALLBACKS
 */

static int cb1 = 0;
static int cb2 = 0;

static void callback1(void * ctx)
{
  cb1++;
}

static void callback2(void * ctx)
{
  cb2++;
}

static void callback3(void * ctx, int i1, int i2)
{
  cb1 = i1;
  cb2 = i2;
}

static void callback4(int * ctx)
{
  (*ctx)++;
}

static void cb1_wrap(void * ctx, va_list args)
{
  callback1(ctx);
}
static void cb2_wrap(void * ctx, va_list args)
{
  callback2(ctx);
}

static void cb3_wrap(void * ctx, va_list args)
{
  int i1 = va_arg(args, int);
  int i2 = va_arg(args, int);
  callback3(ctx, i1, i2);
}

static void cb4_wrap(void * ctx, va_list args)
{
  callback4((int*)ctx);
}

CALLBACK_0(callback1, void*);
CALLBACK_0(callback2, void*);
CALLBACK_2(callback3, void*, int, int);
CALLBACK_0(callback4, int*);

/*
 * TESTS
 */

extern void test_cb_private_functions(void);

static void test_cb_newdel(void)
{
	int i;
	uint32_t size;
	cb_t * cb;

	for (i = 0; i < REPEAT; i++)
	{
		cb = cb_new();

		CU_ASSERT_PTR_NOT_NULL_FATAL(cb);

		cb_delete(cb);
	}
}

static void test_cb_fail_alloc(void)
{
	fail_alloc = TRUE;
	CU_ASSERT_PTR_NULL(cb_new());
	fail_alloc = FALSE;
}

static void test_cb_fail_ht_new(void)
{
	fake_ht_init = TRUE;
	fake_ht_init_ret = FALSE;
	CU_ASSERT_PTR_NULL(cb_new());
	fake_ht_init = FALSE;
}

static void test_cb_add(void)
{
  cb_t * cb = cb_new();
  CU_ASSERT_FALSE(cb_add(NULL, NULL, NULL, NULL));
  CU_ASSERT_FALSE(cb_add(cb, NULL, NULL, NULL));
  CU_ASSERT_FALSE(cb_add(cb, "foo", NULL, NULL));
  CU_ASSERT_TRUE(cb_add(cb, "foo", NULL, &cb1_wrap));
  /* can't add it twice */
  CU_ASSERT_FALSE(cb_add(cb, "foo", NULL, &cb1_wrap));
  /* can add another callback with the same name */
  CU_ASSERT_TRUE(cb_add(cb, "foo", NULL, &cb2_wrap));
  cb_delete(cb);
}

static void test_cb_add_macros(void)
{
  cb_t * cb = cb_new();
  CU_ASSERT_FALSE(ADD_CALLBACK(NULL, NULL, callback1, NULL));
  CU_ASSERT_FALSE(ADD_CALLBACK(cb, NULL, callback1, NULL));
  CU_ASSERT_TRUE(ADD_CALLBACK(cb, "foo", callback1, NULL));
  /* can't add it twice */
  CU_ASSERT_FALSE(ADD_CALLBACK(cb, "foo", callback1, NULL));
  cb_delete(cb);
}

static void test_cb_call(void)
{
  cb_t * cb = cb_new();
  cb1 = 0;
  cb2 = 0;
  CU_ASSERT_FALSE(cb_call(cb, "foo", NULL));
  CU_ASSERT_TRUE((cb1 == 0) && (cb2 == 0));
  CU_ASSERT_TRUE(cb_add(cb, "foo", NULL, &cb1_wrap));
  CU_ASSERT_TRUE(cb_call(cb, "foo"));
  CU_ASSERT_TRUE((cb1 == 1) && (cb2 == 0));
  CU_ASSERT_TRUE(cb_add(cb, "bar", NULL, &cb2_wrap));
  CU_ASSERT_TRUE(cb_call(cb, "foo"));
  CU_ASSERT_TRUE((cb1 == 2) && (cb2 == 0));
  CU_ASSERT_TRUE(cb_call(cb, "bar"));
  CU_ASSERT_TRUE((cb1 == 2) && (cb2 == 1));
  cb_delete(cb);
}

static void test_cb_call_chaining(void)
{
  cb_t * cb = cb_new();
  cb1 = 0;
  cb2 = 0;
  CU_ASSERT_FALSE(cb_call(cb, "foo", NULL));
  CU_ASSERT_TRUE((cb1 == 0) && (cb2 == 0));
  CU_ASSERT_TRUE(cb_add(cb, "foo", NULL, &cb1_wrap));
  CU_ASSERT_TRUE(cb_call(cb, "foo"));
  CU_ASSERT_TRUE((cb1 == 1) && (cb2 == 0));
  CU_ASSERT_TRUE(cb_add(cb, "foo", NULL, &cb2_wrap));
  CU_ASSERT_TRUE(cb_call(cb, "foo"));
  CU_ASSERT_TRUE((cb1 == 2) && (cb2 == 1));
  cb_delete(cb);
}

static void test_cb_call_macros(void)
{
  cb_t * cb = cb_new();
  cb1 = 0;
  cb2 = 0;
  CU_ASSERT_FALSE(cb_call(cb, "foo", NULL));
  CU_ASSERT_TRUE((cb1 == 0) && (cb2 == 0));
  CU_ASSERT_TRUE(ADD_CALLBACK(cb, "foo", callback1, NULL));
  CU_ASSERT_TRUE(cb_call(cb, "foo"));
  CU_ASSERT_TRUE((cb1 == 1) && (cb2 == 0));
  CU_ASSERT_TRUE(ADD_CALLBACK(cb, "bar", callback2, NULL));
  CU_ASSERT_TRUE(cb_call(cb, "foo"));
  CU_ASSERT_TRUE((cb1 == 2) && (cb2 == 0));
  CU_ASSERT_TRUE(cb_call(cb, "bar"));
  CU_ASSERT_TRUE((cb1 == 2) && (cb2 == 1));
  cb_delete(cb);
}

static void test_cb_call_chaining_macros(void)
{
  cb_t * cb = cb_new();
  cb1 = 0;
  cb2 = 0;
  CU_ASSERT_FALSE(cb_call(cb, "foo", NULL));
  CU_ASSERT_TRUE((cb1 == 0) && (cb2 == 0));
  CU_ASSERT_TRUE(ADD_CALLBACK(cb, "foo", callback1, NULL));
  CU_ASSERT_TRUE(cb_call(cb, "foo"));
  CU_ASSERT_TRUE((cb1 == 1) && (cb2 == 0));
  CU_ASSERT_TRUE(ADD_CALLBACK(cb, "foo", callback2, NULL));
  CU_ASSERT_TRUE(cb_call(cb, "foo"));
  CU_ASSERT_TRUE((cb1 == 2) && (cb2 == 1));
  cb_delete(cb);
}

static void test_cb_remove(void)
{
  cb_t * cb = cb_new();
  cb1 = 0;
  cb2 = 0;
  CU_ASSERT_FALSE(cb_call(cb, "foo", NULL));
  CU_ASSERT_TRUE((cb1 == 0) && (cb2 == 0));
  CU_ASSERT_TRUE(cb_add(cb, "foo", NULL, &cb1_wrap));
  CU_ASSERT_TRUE(cb_call(cb, "foo"));
  CU_ASSERT_TRUE((cb1 == 1) && (cb2 == 0));
  CU_ASSERT_TRUE(cb_add(cb, "foo", NULL, &cb2_wrap));
  CU_ASSERT_TRUE(cb_call(cb, "foo"));
  CU_ASSERT_TRUE((cb1 == 2) && (cb2 == 1));
  CU_ASSERT_TRUE(cb_remove(cb, "foo", NULL, &cb1_wrap));
  CU_ASSERT_TRUE(cb_call(cb, "foo"));
  CU_ASSERT_TRUE((cb1 == 2) && (cb2 == 2));
  cb_delete(cb);
}

static void test_cb_remove_macros(void)
{
  cb_t * cb = cb_new();
  cb1 = 0;
  cb2 = 0;
  CU_ASSERT_FALSE(cb_call(cb, "foo", NULL));
  CU_ASSERT_TRUE((cb1 == 0) && (cb2 == 0));
  CU_ASSERT_TRUE(ADD_CALLBACK(cb, "foo", callback1, NULL));
  CU_ASSERT_TRUE(cb_call(cb, "foo"));
  CU_ASSERT_TRUE((cb1 == 1) && (cb2 == 0));
  CU_ASSERT_TRUE(ADD_CALLBACK(cb, "bar", callback2, NULL));
  CU_ASSERT_TRUE(cb_call(cb, "foo"));
  CU_ASSERT_TRUE((cb1 == 2) && (cb2 == 0));
  CU_ASSERT_TRUE(cb_call(cb, "bar"));
  CU_ASSERT_TRUE((cb1 == 2) && (cb2 == 1));
  CU_ASSERT_TRUE(REMOVE_CALLBACK(cb, "bar", callback2, NULL));
  CU_ASSERT_FALSE(cb_call(cb, "bar"));
  CU_ASSERT_TRUE((cb1 == 2) && (cb2 == 1));
  cb_delete(cb);
}

static void test_cb_args(void)
{
  cb_t * cb = cb_new();
  cb1 = 0;
  cb2 = 0;
  CU_ASSERT_FALSE(cb_call(cb, "baz", 1, 1));
  CU_ASSERT_TRUE((cb1 == 0) && (cb2 == 0));
  CU_ASSERT_TRUE(cb_add(cb, "baz", NULL, &cb3_wrap));
  CU_ASSERT_TRUE(cb_call(cb, "baz", 1, 1));
  CU_ASSERT_TRUE((cb1 == 1) && (cb2 == 1));
  CU_ASSERT_TRUE(cb_call(cb, "baz", 2, 2));
  CU_ASSERT_TRUE((cb1 == 2) && (cb2 == 2));
  cb_delete(cb);
}

static void test_cb_args_macros(void)
{
  cb_t * cb = cb_new();
  cb1 = 0;
  cb2 = 0;
  CU_ASSERT_FALSE(cb_call(cb, "baz", 1, 1));
  CU_ASSERT_TRUE((cb1 == 0) && (cb2 == 0));
  CU_ASSERT_TRUE(ADD_CALLBACK(cb, "baz", callback3, NULL));
  CU_ASSERT_TRUE(cb_call(cb, "baz", 1, 1));
  CU_ASSERT_TRUE((cb1 == 1) && (cb2 == 1));
  CU_ASSERT_TRUE(cb_call(cb, "baz", 1, 2));
  CU_ASSERT_TRUE((cb1 == 1) && (cb2 == 2));
  cb_delete(cb);
}

static void test_cb_context(void)
{
  cb_t * cb = cb_new();
  cb1 = 0;
  CU_ASSERT_FALSE(cb_call(cb, "qux"));
  CU_ASSERT_TRUE(cb_add(cb, "qux", &cb1, &cb4_wrap));
  CU_ASSERT_TRUE(cb_call(cb, "qux"));
  CU_ASSERT_TRUE(cb1 == 1);
  CU_ASSERT_TRUE(cb_call(cb, "qux"));
  CU_ASSERT_TRUE(cb1 == 2);
  cb_delete(cb);
}

static void test_cb_context_macros(void)
{
  cb_t * cb = cb_new();
  cb1 = 0;
  CU_ASSERT_FALSE(cb_call(cb, "qux"));
  CU_ASSERT_TRUE(ADD_CALLBACK(cb, "qux", callback4, &cb1));
  CU_ASSERT_TRUE(cb_call(cb, "qux"));
  CU_ASSERT_TRUE(cb1 == 1);
  CU_ASSERT_TRUE(cb_call(cb, "qux"));
  CU_ASSERT_TRUE(cb1 == 2);
  cb_delete(cb);
}

static int init_cb_suite(void)
{
	srand(0xDEADBEEF);
	reset_test_flags();
	return 0;
}

static int deinit_cb_suite(void)
{
	reset_test_flags();
	return 0;
}

static CU_pSuite add_cb_tests(CU_pSuite pSuite)
{
	ADD_TEST("new/delete of cb", test_cb_newdel);
  ADD_TEST("cb new fail alloc", test_cb_fail_alloc);
  ADD_TEST("cb new fail ht new", test_cb_fail_ht_new);
  ADD_TEST("cb test add", test_cb_add);
  ADD_TEST("cb test add macros", test_cb_add_macros);
  ADD_TEST("cb test call", test_cb_call);
  ADD_TEST("cb test call chaining", test_cb_call_chaining);
  ADD_TEST("cb test call macros", test_cb_call_macros);
  ADD_TEST("cb test call chaining macros", test_cb_call_chaining_macros);
  ADD_TEST("cb test remove", test_cb_remove);
  ADD_TEST("cb test remove_macros", test_cb_remove_macros);
  ADD_TEST("cb test args", test_cb_args);
  ADD_TEST("cb test args macros", test_cb_args_macros);
  ADD_TEST("cb test context", test_cb_context);
  ADD_TEST("cb test context macros", test_cb_context_macros);
	ADD_TEST("cb private functions", test_cb_private_functions);

	return pSuite;
}

CU_pSuite add_cb_test_suite()
{
	CU_pSuite pSuite = NULL;

	/* add the suite to the registry */
	pSuite = CU_add_suite("Callback Tests", init_cb_suite, deinit_cb_suite);
	CHECK_PTR_RET(pSuite, NULL);

	/* add in cb specific tests */
	CHECK_PTR_RET(add_cb_tests(pSuite), NULL);

	return pSuite;
}

