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
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

#include <CUnit/Basic.h>

#include <cutil/debug.h>
#include <cutil/macros.h>
#include <cutil/cb.h>
#include <cutil/events.h>

#include "test_macros.h"
#include "test_flags.h"

#define REPEAT (128)
#define SIZEMAX (128)
#define MULTIPLE (8)

extern evt_loop_t * el;
extern void test_events_private_functions(void);

static void test_evt_new_failure(void)
{
  fake_ev_default_loop = TRUE;
  CU_ASSERT_PTR_NULL(evt_new());
  fake_ev_default_loop = FALSE;
}

static void test_evt_delete_null(void)
{
  evt_delete(NULL);
}

static void test_new_signal_event(void)
{
  evt_t * evt = NULL;
  fail_alloc = TRUE;
  CU_ASSERT_PTR_NULL(evt_new_signal_event(NULL, SIGUSR1));
  fail_alloc = FALSE;

  fake_event_init = TRUE;
  fake_event_init_ret = FALSE;
  CU_ASSERT_PTR_NULL(evt_new_signal_event(NULL, SIGUSR1));
  fake_event_init_ret = TRUE;
  evt = evt_new_signal_event(NULL, SIGUSR1);
  fake_event_init_ret = FALSE;
  fake_event_init = FALSE;
  evt_delete_event(evt);
}

static void test_new_child_event(void)
{
  evt_t * evt = NULL;
  fail_alloc = TRUE;
  CU_ASSERT_PTR_NULL(evt_new_child_event(NULL, (int)getpid(), FALSE));
  fail_alloc = FALSE;

  fake_event_init = TRUE;
  fake_event_init_ret = FALSE;
  CU_ASSERT_PTR_NULL(evt_new_child_event(NULL, (int)getpid(), FALSE));
  fake_event_init_ret = TRUE;
  evt = evt_new_child_event(NULL, (int)getpid(), FALSE);
  fake_event_init_ret = FALSE;
  fake_event_init = FALSE;
  evt_delete_event(evt);
}

static void test_new_io_event(void)
{
  evt_t * evt = NULL;
  fail_alloc = TRUE;
  CU_ASSERT_PTR_NULL(evt_new_io_event(NULL, STDOUT_FILENO, EV_WRITE));
  fail_alloc = FALSE;

  fake_event_init = TRUE;
  fake_event_init_ret = FALSE;
  CU_ASSERT_PTR_NULL(evt_new_io_event(NULL, STDOUT_FILENO, EV_WRITE));
  fake_event_init_ret = TRUE;
  evt = evt_new_io_event(NULL, STDOUT_FILENO, EV_WRITE);
  fake_event_init_ret = FALSE;
  fake_event_init = FALSE;
  evt_delete_event(evt);
}

static void test_delete_event(void)
{
  evt_t * evt = NULL;
  evt_delete_event(NULL);
  evt = evt_new_signal_event(NULL, SIGUSR1);
  CU_ASSERT_PTR_NOT_NULL_FATAL(evt);
  evt_delete_event(evt);
}

static int_t test_flag = FALSE;

static void test_signal_cb(int_t * p, evt_t * evt, int signum)
{
  (*p) = TRUE;
  debug_signals_dump("start event 4 (cb)");
  evt_stop_event(evt);
  evt_stop(el, FALSE);
  debug_signals_dump("start event 5 (cb)");
}

SIGNAL_CB(test_signal_cb, int_t*);

static void test_start_event(void)
{
  evt_t * evt = NULL;
  cb_t * cb = NULL;
  struct itimerval timer_value;
  MEMSET(&timer_value, 0, sizeof(struct itimerval));

  debug_signals_dump("start event 0");

  CU_ASSERT_EQUAL(evt_start_event(NULL, NULL), EVT_BADPTR);
  CU_ASSERT_EQUAL(evt_start_event(NULL, el), EVT_BADPTR);

  cb = cb_new();
  ADD_SIGNAL_CB(cb, test_signal_cb, &test_flag);
  evt = evt_new_signal_event(cb, SIGALRM);

  test_flag = FALSE;
  debug_signals_dump("start event 1");
  CU_ASSERT_EQUAL(evt_start_event(evt, el), EVT_OK);
  debug_signals_dump("start event 2");

  /* we'll receive a SIGALRM in 1 second */
  timer_value.it_value.tv_usec = 100000;
  timer_value.it_interval.tv_usec = 100000;
  CU_ASSERT_EQUAL(setitimer(ITIMER_REAL, &timer_value, NULL), 0);

  debug_signals_dump("start event 3");
  evt_run(el);
  debug_signals_dump("start event 6");

  CU_ASSERT_TRUE(test_flag);
  test_flag = FALSE;
  MEMSET(&timer_value, 0, sizeof(struct itimerval));
  CU_ASSERT_EQUAL(setitimer(ITIMER_REAL, &timer_value, NULL), 0);
  debug_signals_dump("start event 7");
  CU_ASSERT_FALSE(test_flag);

  evt_delete_event(evt);
  cb_delete(cb);
}

static int_t test_flag_a = 0;
static int_t test_flag_b = 0;

static void test_signal_1_cb(int_t * p, evt_t * evt, int signum)
{
  (*p)++;
  evt_stop_event(evt);
}

static void test_signal_2_cb(int_t * p, evt_t * evt, int signum)
{
  struct itimerval timer_value;

  (*p)++;

  if(*p < 2)
  {
    /* set another alarm */
    MEMSET(&timer_value, 0, sizeof(struct itimerval));
    timer_value.it_value.tv_usec = 100000;
    timer_value.it_interval.tv_usec = 100000;
    CU_ASSERT_EQUAL(setitimer(ITIMER_REAL, &timer_value, NULL), 0);
  }
  else
  {
    /* otherwise we can stop the event loop */
    evt_stop(el, FALSE);
  }
}

SIGNAL_CB(test_signal_1_cb, int_t*);
SIGNAL_CB(test_signal_2_cb, int_t*);

static void test_stop_event(void)
{
  evt_t * evt1 = NULL;
  evt_t * evt2 = NULL;
  cb_t * cb1 = NULL;
  cb_t * cb2 = NULL;
  struct itimerval timer_value;
  MEMSET(&timer_value, 0, sizeof(struct itimerval));

  cb1 = cb_new();
  ADD_SIGNAL_CB(cb1, test_signal_1_cb, &test_flag_a);
  evt1 = evt_new_signal_event(cb1, SIGALRM);
  cb2 = cb_new();
  ADD_SIGNAL_CB(cb2, test_signal_2_cb, &test_flag_b);
  evt2 = evt_new_signal_event(cb2, SIGALRM);

  test_flag_a = 0;
  test_flag_b = 0;
  /* start the event */
  CU_ASSERT_EQUAL(evt_start_event(evt1, el), EVT_OK);
  CU_ASSERT_EQUAL(evt_start_event(evt2, el), EVT_OK);

  /* set an alarm */
  timer_value.it_value.tv_usec = 100000;
  timer_value.it_interval.tv_usec = 100000;
  CU_ASSERT_EQUAL(setitimer(ITIMER_REAL, &timer_value, NULL), 0);

  evt_run(el);

  CU_ASSERT_EQUAL(test_flag_a, 1);
  CU_ASSERT_EQUAL(test_flag_b, 2);

  evt_delete_event(evt1);
  evt_delete_event(evt2);
  cb_delete(cb1);
  cb_delete(cb2);
}

#if 0
static void test_evt_run_null(void)
{
  CU_ASSERT_EQUAL(evt_run(NULL), EVT_BADPTR);
}

static void test_evt_stop_null(void)
{
  CU_ASSERT_EQUAL(evt_stop(NULL, FALSE), EVT_BADPTR);
}
#endif

static int init_events_suite(void)
{
  srand(0xDEADBEEF);
  reset_test_flags();
  return 0;
}

static int deinit_events_suite(void)
{
  reset_test_flags();
  return 0;
}

static CU_pSuite add_events_tests(CU_pSuite pSuite)
{
  ADD_TEST("evt new failure", test_evt_new_failure);
  ADD_TEST("evt delete null", test_evt_delete_null);
  ADD_TEST("test new signal event", test_new_signal_event);
  ADD_TEST("test new child event", test_new_child_event);
  ADD_TEST("test new io event", test_new_io_event);
  ADD_TEST("test delete event", test_delete_event);
  ADD_TEST("test start event", test_start_event);
  ADD_TEST("test stop event", test_stop_event);
#if 0
  ADD_TEST("test evt run null", test_evt_run_null);
  ADD_TEST("test evt stop null", test_evt_stop_null);
#endif
  ADD_TEST("events private functions", test_events_private_functions);
  return pSuite;
}

CU_pSuite add_events_test_suite()
{
  CU_pSuite pSuite = NULL;

  /* add the suite to the registry */
  pSuite = CU_add_suite("Events Tests", init_events_suite, deinit_events_suite);
  CHECK_PTR_RET(pSuite, NULL);

  /* add in specific tests */
  CHECK_PTR_RET(add_events_tests(pSuite), NULL);

  return pSuite;
}

