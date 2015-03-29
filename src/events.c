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

#define EV_STANDALONE   1       /* we don't use autoconf */
#define EV_MULTIPLICITY 1       /* must pass event loop pointer to all fns */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "debug.h"
#include "macros.h"
#include "cb.h"
#include "events.h"

#if defined(UNIT_TESTING)
#include "test_flags.h"
extern evt_loop_t * el;
#endif

typedef union ev_data_u
{
  struct ev_signal    sig;
  struct ev_child     child;
  struct ev_io        io;
} ev_data_t;

typedef struct signal_params_s
{
  int signum;
  sigset_t oldset;        /* old mask value for specified signal */
  struct sigaction oldact;/* old action value for specified signal */
} signal_params_t;

typedef struct child_params_s
{
  int pid;    /* pid to watch */
  int trace;  /* 0 == only signal upon term, 1 == also signal when stopped/continued */
  int rpid;   /* pid of process causing change */
  int rstatus;/* status word of process, use macros from sys/wait.h, waitpid */

  sigset_t oldset;        /* old mask value for specified signal */
  struct sigaction oldact;/* old action value for specified signal */
} child_params_t;

typedef struct io_params_s
{
  int fd;
  evt_io_type_t types;
} io_params_t;

struct evt_s
{
  ev_data_t       ev;         /* MUST BE FIRST */

  evt_type_t      type;
  signal_params_t signal_params;
  child_params_t  child_params;
  io_params_t     io_params;
  cb_t            *cb;        /* callbacks handler */

  evt_loop_t *    el;         /* the event loop associated with */
};

static int_t evt_init_event(evt_t * evt, cb_t *cb, evt_type_t t, va_list args);
static void evt_deinit_event(evt_t * evt);
static void evt_signal_cb(struct ev_loop * loop, struct ev_signal * w, int revents);
static void evt_child_cb(struct ev_loop * loop, struct ev_child * w, int revents);
static void evt_io_cb(struct ev_loop * loop, struct ev_io * w, int revents);
static void evt_cb(evt_t * evt);
static void evt_log_backend(evt_loop_t * el);
#ifdef DEBUG_ON
static size_t get_signals_debug_string(sigset_t const * sigs, uint8_t ** p);
#endif

uint8_t const * const evt_type_cb[EVT_TYPE_COUNT] =
{
  UT("evt-signal"),
  UT("evt-child"),
  UT("evt-io")
};

/* helper macro for calling callbacks */
#define EVT(cb, ...) cb_call(cb, EVT_CB_NAME(evt->type), __VA_ARGS__)

/*
 * EVENT LOOP
 */

evt_loop_t* evt_new(void)
{
  evt_loop_t* el = NULL;

  /* initialize the event loop, auto-selecting backend */
  el = (evt_loop_t*)EV_DEFAULT_LOOP(EVFLAG_AUTO | EVFLAG_NOENV);
  CHECK_PTR_RET_MSG(el, NULL, "failed to start event loop\n");

  /* log which backend was chosen */
  evt_log_backend(el);

  return el;
}

void evt_delete(void * e)
{
  evt_loop_t* el = (evt_loop_t*)e;

  CHECK_PTR(el);

  /* clean up the default event loop */
  ev_loop_destroy(EV_DEFAULT_UC);
}

evt_ret_t evt_run(evt_loop_t * el)
{
  CHECK_PTR_RET(el, EVT_BADPTR);

  /* start the libev event loop */
  ev_run(el, 0);

  return EVT_OK;
}

evt_ret_t evt_stop(evt_loop_t * el, int_t once)
{
  CHECK_PTR_RET(el, EVT_BADPTR);

  /* stop the libev event loop */
  if (once)
    ev_break((struct ev_loop*)el, EVBREAK_ONE);
  else
    ev_break((struct ev_loop*)el, EVBREAK_ALL);

  return EVT_OK;
}

/*
 * EVENTS
 */

evt_t * evt_new_event(cb_t * cb, evt_type_t t, ...)
{
  int_t ret = 0;
  evt_t * evt = NULL;
  va_list args;

  CHECK_RET(VALID_EVENT_TYPE(t), FALSE);

  evt = CALLOC(1, sizeof(evt_t));
  CHECK_PTR_RET(evt, NULL);

  /* initialize it */
  va_start(args, t);
  ret = evt_init_event(evt, cb, t, args);
  va_end(args);

  if(ret)
    return evt;

  FREE(evt);
  return NULL;
}


void evt_delete_event(void * e)
{
  evt_t * evt = (evt_t*)e;
  CHECK_PTR(evt);
  evt_deinit_event(evt);
  FREE(evt);
};

evt_ret_t evt_start_event(evt_t * evt, evt_loop_t * el)
{
  UNIT_TEST_RET(event_start);

  CHECK_PTR_RET_MSG(el, EVT_BADPTR, "bad event loop pointer\n");
  CHECK_PTR_RET_MSG(evt, EVT_BADPTR, "bad event pointer\n");
  CHECK_RET(VALID_EVENT_TYPE(evt->type), EVT_BADPARAM);

  /* store the pointer to the loop we're hooking into */
  evt->el = el;

  switch (evt->type)
  {
    case EVT_SIGNAL:
    {
      /* start the libev signal event */
      DEBUG("starting signal event\n");
      ev_signal_start((struct ev_loop*)el, (struct ev_signal*)evt);
      break;
    }
    case EVT_CHILD:
    {
      /* start the libevn child event */
      DEBUG("staring child event\n");
      ev_child_start((struct ev_loop*)el, (struct ev_child*)evt);
      break;
    }
    case EVT_IO:
    {
      /* start the libev io event */
      DEBUG("starting io event\n");
      ev_io_start((struct ev_loop*)el, (struct ev_io*)evt);
      break;
    }
  }

  return EVT_OK;
}

evt_ret_t evt_stop_event(evt_t * evt)
{
  UNIT_TEST_RET(event_stop);

  CHECK_PTR_RET(evt, EVT_BADPTR);
  CHECK_PTR_RET(evt->el, EVT_BADPTR);

  switch (evt->type)
  {
    case EVT_SIGNAL:
    {
      /* stop the libev signal event */
      DEBUG("stopping signal event\n");
      ev_signal_stop((struct ev_loop*)(evt->el), (struct ev_signal*)evt);
      break;
    }
    case EVT_CHILD:
    {
      /* stop the libev child event */
      DEBUG("stopping child event\n");
      ev_child_stop((struct ev_loop*)(evt->el), (struct ev_child*)evt);
      break;
    }
    case EVT_IO:
    {
      /* stop the libev io event */
      ev_io_stop((struct ev_loop*)(evt->el), (struct ev_io*)evt);
      break;
    }
  }

  /* clear our pointer to the event loop */
  evt->el = NULL;

  return EVT_OK;
}

#ifdef DEBUG_ON
void debug_signals_dump(uint8_t const * const prefix)
{
  sigset_t s;
  uint8_t * p;

  sigemptyset(&s);
  sigprocmask(SIG_BLOCK, NULL, &s);
  if (get_signals_debug_string(&s, &p))
    LOG("%s Blocked Signals:\n%s", prefix, p);

  sigemptyset(&s);
  sigprocmask(SIG_UNBLOCK, NULL, &s);
  if (get_signals_debug_string(&s, &p))
    LOG("%s Unblocked Signals:\n%s", prefix, p);

  sigemptyset(&s);
  sigpending(&s);
  if (get_signals_debug_string(&s, &p))
    LOG("%s Pending Signals:\n%s", prefix, p);
}
#else
void debug_signals_dump(uint8_t const * const prefix) {;}
#endif

/*
 * PRIVATE FUNCTIONS
 */

static int_t evt_init_event(evt_t * evt, cb_t *cb, evt_type_t t, va_list args)
{
  UNIT_TEST_N_RET(event_init);

  CHECK_PTR_RET(evt, FALSE);

  /* store the callbacks and type */
  evt->cb = cb;
  evt->type = t;

  switch (evt->type)
  {
    case EVT_SIGNAL:
    {
      /* store the signal number */
      evt->signal_params.signum = va_arg(args, int);

      /* get the current signal mask value */
      sigemptyset(&(evt->signal_params.oldset));
      sigprocmask(SIG_BLOCK, NULL, &(evt->signal_params.oldset));

      /* get the current sigaction */
      MEMSET(&(evt->signal_params.oldact), 0, sizeof(struct sigaction));
      sigaction(evt->signal_params.signum, NULL,
                &(evt->signal_params.oldact));

      /* initialize a libev signal event */
      ev_signal_init((struct ev_signal*)evt, &evt_signal_cb,
                     evt->signal_params.signum);
      return TRUE;
    }
    case EVT_CHILD:
    {
      /* store the child params */
      evt->child_params.pid = va_arg(args, int);
      evt->child_params.trace = va_arg(args, int);

      /* get the current signal mask value */
      sigemptyset(&(evt->child_params.oldset));
      sigprocmask(0, NULL, &(evt->child_params.oldset));

      /* get the current sigaction */
      MEMSET(&(evt->child_params.oldact), 0, sizeof(struct sigaction));
      sigaction(SIGCHLD, NULL, &(evt->signal_params.oldact));

      /* initialize a libev child event */
      ev_child_init((struct ev_child*)evt, &evt_child_cb, evt->child_params.pid,
                    evt->child_params.trace);
      return TRUE;
    }
    case EVT_IO:
    {
      /* store the io params */
      evt->io_params.fd = va_arg(args, int);
      evt->io_params.types = va_arg(args, evt_io_type_t);

      /* initialize a libev io event */
      ev_io_init((struct ev_io*)evt, &evt_io_cb, evt->io_params.fd,
                 evt->io_params.types);
      return TRUE;
    }
  }

  return FALSE;
}

static void evt_deinit_event(evt_t *evt)
{
  sigset_t mask;

  /* stop the event handler if needed */
  if (evt->el != NULL)
  {
    evt_stop_event(evt);
  }

  CHECK(VALID_EVENT_TYPE(evt->type));

  switch(evt->type)
  {
    case EVT_SIGNAL:

      sigemptyset(&mask);

      /* if the oldset had the signal asserted, it was blocked before
       * so add it to our new mask... */
      if (sigismember(&(evt->signal_params.oldset), evt->signal_params.signum))
      {
        sigaddset(&mask, evt->signal_params.signum);
      }

      /*...and now we can block it again */
      sigprocmask(SIG_BLOCK, &mask, NULL);

      /* now restore the old sigaction */
      sigaction(evt->signal_params.signum, &(evt->signal_params.oldact), NULL);

      break;

    case EVT_CHILD:
      sigemptyset(&mask);

      /* if SIGCHLD was asserted in the old set... */
      if (sigismember(&(evt->child_params.oldset), SIGCHLD))
      {
          /* ...we add it to our new mask... */
          sigaddset(&mask, SIGCHLD);
      }

      /*...and now we can block it again */
      sigprocmask(SIG_BLOCK, &mask, NULL);

      /* now restore the old SIGCHLD sigaction */
      sigaction(SIGCHLD, &(evt->signal_params.oldact), NULL);
      break;

    case EVT_IO:
        break;
  }
}

/* libev callbacks */
static void evt_signal_cb(struct ev_loop * l, struct ev_signal * w, int r)
{
  evt_t * evt = (evt_t*)w;
  CHECK_PTR(l);
  CHECK_PTR(evt);
  CHECK(evt->el == l);
  CHECK(r == EV_SIGNAL);
  evt_cb(evt);
}

static void evt_child_cb(struct ev_loop * l, struct ev_child * w, int r)
{
  evt_t * evt = (evt_t*)w;
  CHECK_PTR(l);
  CHECK_PTR(evt);
  CHECK(evt->el == l);
  CHECK(r == EV_CHILD);
  evt_cb(evt);
}

static void evt_io_cb(struct ev_loop * l, struct ev_io * w, int r)
{
  evt_t * evt = (evt_t*)w;
  CHECK_PTR(l);
  CHECK_PTR(evt);
  CHECK(evt->el == l);
  CHECK((r & EV_READ) || (r & EV_WRITE));
  evt_cb(evt);
}

static void evt_cb(evt_t * evt)
{
  struct ev_signal *sig;
  struct ev_child *child;
  struct ev_io *io;
  CHECK_PTR(evt);
  CHECK(VALID_EVENT_TYPE(evt->type));

  switch(evt->type)
  {
    case EVT_SIGNAL:
      DEBUG("calling signal callback\n");
      sig = (struct ev_signal*)&evt;
      EVT(evt->cb, evt, sig->signum);
      break;

    case EVT_CHILD:
      DEBUG("calling child callback\n");
      child = (struct ev_child*)&evt;
      EVT(evt->cb, evt, child->pid, child->rpid, child->rstatus);
      break;

    case EVT_IO:
      DEBUG("calling io callback\n");
      io = (struct ev_io*)&evt;
      EVT(evt->cb, evt, io->fd, (evt_io_type_t)io->events);
      break;
  }
}

static void evt_log_backend(evt_loop_t * el)
{
  unsigned int flags = 0;

  CHECK_PTR(el);

  /* get the flag specifying which backend is being used */
  flags = ev_backend((struct ev_loop*)el);

  if (flags & EVBACKEND_SELECT)
    DEBUG("using SELECT backend\n");
  if (flags & EVBACKEND_POLL)
    DEBUG("using POLL backend\n");
  if (flags & EVBACKEND_EPOLL)
    DEBUG("using EPOLL backend\n");
  if (flags & EVBACKEND_KQUEUE)
    DEBUG("using KQUEUE backend\n");
  if (flags & EVBACKEND_DEVPOLL)
    DEBUG("using DEVPOLL backend\n");
  if (flags & EVBACKEND_PORT)
    DEBUG("using PORT backend\n");
}

#ifdef DEBUG_ON
static size_t get_signals_debug_string(sigset_t const * sigs, uint8_t ** out)
{
  static uint8_t buf[4096];
  uint8_t * p;
  int i;

  MEMSET(buf, 0, 4096);
  p = &buf[0];
  for (i = 1; (i < EV_NSIG) && (p < &buf[4095]); ++i)
  {
    if (sigismember(sigs, i))
    {
      p += sprintf(buf, "\t%s\n", strsignal(i));
    }
  }

  (*out) = &buf[0];

  return (size_t)(p - &buf[0]);
}
#endif

#if defined(UNIT_TESTING)

#include <CUnit/Basic.h>

static int_t test_flag = FALSE;

static void test_signal_cb(void * p, evt_t * evt, int signum)
{
  test_flag = TRUE;
}

static void test_child_cb(void * p, evt_t * evt, int pid, int rpid, int rstatus)
{
  test_flag = TRUE;
}

static void test_io_cb(void * p, evt_t * evt, int fd, evt_io_type_t events)
{
  test_flag = TRUE;
}

CB_2(test_signal_cb, void *, evt_t *, int);
CB_4(test_child_cb, void *, evt_t *, int, int, int);
CB_3(test_io_cb, void *, evt_t *, int, evt_io_type_t);

static void test_evt_signal_cb(void)
{
  evt_t evt;
  MEMSET(&evt, 0, sizeof(evt_t));

  evt_signal_cb(NULL, NULL, 0);
  evt_signal_cb((struct ev_loop*)el, NULL, 0);
  evt_signal_cb((struct ev_loop*)el, (struct ev_signal*)&evt, 0);
  evt_signal_cb((struct ev_loop*)el, (struct ev_signal*)&evt, EV_SIGNAL);

  evt.el = el;
  evt_signal_cb((struct ev_loop*)el, (struct ev_signal*)&evt, EV_SIGNAL);

  evt.type = EVT_SIGNAL;
  evt.cb = cb_new();
  ADD_CB(evt.cb, EVT_CB_NAME(evt.type), test_signal_cb, NULL);

  test_flag = FALSE;
  evt_signal_cb((struct ev_loop*)el, (struct ev_signal*)&evt, EV_SIGNAL);
  CU_ASSERT_TRUE(test_flag);
  test_flag = FALSE;

  cb_delete(evt.cb);
}

static void test_evt_child_cb(void)
{
  evt_t evt;
  MEMSET(&evt, 0, sizeof(evt_t));

  evt_child_cb(NULL, NULL, 0);
  evt_child_cb((struct ev_loop*)el, NULL, 0);
  evt_child_cb((struct ev_loop*)el, (struct ev_child*)&evt, 0);
  evt_child_cb((struct ev_loop*)el, (struct ev_child*)&evt, EV_CHILD);

  evt.el = el;
  evt_child_cb((struct ev_loop*)el, (struct ev_child*)&evt, EV_CHILD);

  evt.type = EVT_CHILD;
  evt.cb = cb_new();
  ADD_CB(evt.cb, EVT_CB_NAME(evt.type), test_child_cb, NULL);
  test_flag = FALSE;
  evt_child_cb((struct ev_loop*)el, (struct ev_child*)&evt, EV_CHILD);
  CU_ASSERT_TRUE(test_flag);
  test_flag = FALSE;

  cb_delete(evt.cb);
}

static void test_evt_io_cb(void)
{
  evt_t evt;
  MEMSET(&evt, 0, sizeof(evt_t));

  evt_io_cb(NULL, NULL, 0);
  evt_io_cb((struct ev_loop*)el, NULL, 0);
  evt_io_cb((struct ev_loop*)el, (struct ev_io*)&evt, 0);
  evt_io_cb((struct ev_loop*)el, (struct ev_io*)&evt, EV_READ);

  evt.el = el;
  evt_io_cb((struct ev_loop*)el, (struct ev_io*)&evt, EV_READ);

  evt.type = EVT_IO;
  evt.cb = cb_new();
  ADD_CB(evt.cb, EVT_CB_NAME(evt.type), test_io_cb, NULL);
  test_flag = FALSE;
  evt_io_cb((struct ev_loop*)el, (struct ev_io*)&evt, EV_READ);
  CU_ASSERT_TRUE(test_flag);
  test_flag = FALSE;

  cb_delete(evt.cb);
}

static void test_evt_log_backend(void)
{
  evt_log_backend(NULL);
}

void test_events_private_functions(void)
{
  test_evt_signal_cb();
  test_evt_child_cb();
  test_evt_io_cb();
  test_evt_log_backend();
}

#endif

