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

#ifndef EVENTS_H
#define EVENTS_H

#include <ev.h>

typedef enum evt_ret_e
{
  /* non-errors */
  EVT_OK = 1,

  /* errors */
  EVT_BADPTR = -1,
  EVT_ERROR = -2,
  EVT_BADPARAM = -3
} evt_ret_t;

typedef enum evt_type_e
{
  EVT_SIGNAL,
  EVT_CHILD,
  EVT_IO
} evt_type_t;

#define EVT_TYPE_FIRST (EVT_SIGNAL)
#define EVT_TYPE_LAST (EVT_IO)
#define EVT_TYPE_COUNT (EVT_TYPE_LAST - EVT_TYPE_FIRST + 1)
#define VALID_EVENT_TYPE(t) ((t >= EVT_TYPE_FIRST) && (t <= EVT_TYPE_LAST))

extern uint8_t const * const evt_type_cb[EVT_TYPE_COUNT];
#define EVT_CB_NAME(x) (VALID_EVENT_TYPE(x) ? evt_type_cb[x] : NULL)

typedef enum evt_io_type_e
{
  EVT_IO_READ =  EV_READ,
  EVT_IO_WRITE = EV_WRITE
} evt_io_type_t;

typedef struct ev_loop evt_loop_t;
typedef struct evt_s evt_t;

/*
 * EVENT LOOP INTERFACE
 */

/* allocate and initialize the events system */
evt_loop_t* evt_new(void);

/* deinitialize and free the events system */
void evt_delete(void * e);

/* runs the event loop */
evt_ret_t evt_run(evt_loop_t * el);

/* stops the event loop */
evt_ret_t evt_stop(evt_loop_t * el, int_t once);

/*
 * EVENTS INTERFACE
 *
 * When an event is created, you can pass in a cb_t to receive callbacks for
 * your event.  Here is an example for signal events:
 *
 *   typedef struct foo_s {
 *   } foo_t;
 *
 *   static void signal_cb(foo_t * f, evt_t * evt, int signum)
 *   {
 *   }
 *
 *   SIGNAL_CB(signal_cb, foo_t*);
 *
 * Then later in your code, hook up the callback and create the event
 *
 *   evt_t * evt = NULL;
 *   cb_t * cb = cb_new();
 *   foo_t * f = calloc(1, sizeof(foo_t));
 *   ADD_SIGNAL_CB(cb, signal_cb, f);
 *   evt = evt_new_signal_event(cb, SIGUSR1);
 *
 * Signal callbacks must have a signature with the first parameter being
 * the user defined context parameter, the second parameter is always a
 * pointer to the event that the callback is associated with, and the
 * third prameters is the number of the signal that just fired.
 *
 * Child callbacks have a signature with the first parameter being the user
 * defined context parameter, the second parameter is the event pointer, the
 * third parameter is the pid of the process being watched, the fourth
 * parameter is the pid of the process that caused the event to fire, and the
 * fifth parameter is the status word of the process that can be interpreted
 * using the macros from sys/wait.h.
 *
 *   typedef struct foo_s {
 *   } foo_t;
 *
 *   static void child_cb(foo_t * f, evt_t * evt, int pid, int rpid, int rstatus)
 *   {
 *   }
 *
 *   CHILD_CB(child_cb, foo_t*);
 *
 * IO callbacks have a signature with the first parameter being the user
 * defined context parameter, the second paraeter is the event pointer, the
 * third parameter is the file descriptor that changed, and the fourth
 * parameter is event type flags or'd together.
 *
 *   typedef struct foo_s {
 *   } foo_t;
 *
 *   static void io_cb(foo_t *f, evt_t * evt, int fd, evt_io_type_t flags)
 *   {
 *   }
 *
 *   IO_CB(io_cb, foo_t*);
 */

/* helper macros for declaring and adding event callbacks */
#define SIGNAL_CB(fn,ctx) CB_2(fn,ctx,evt_t*,int)
#define CHILD_CB(fn,ctx) CB_4(fn,ctx,evt_t*,int,int,int)
#define IO_CB(fn,ctx) CB_3(fn,ctx,evt_t*,int,evt_io_type_t)

#define ADD_SIGNAL_CB(cb,fn,ctx) ADD_CB(cb,EVT_CB_NAME(EVT_SIGNAL),fn,ctx)
#define ADD_CHILD_CB(cb,fn,ctx) ADD_CB(cb,EVT_CB_NAME(EVT_CHILD),fn,ctx)
#define ADD_IO_CB(cb,fn,ctx) ADD_CB(cb,EVT_CB_NAME(EVT_IO),fn,ctx)

/* create a new event */
#define evt_new_signal_event(cb, signum) evt_new_event(cb,EVT_SIGNAL,signum)
#define evt_new_child_event(cb, pid, trace) evt_new_event(cb,EVT_CHILD,pid,trace)
#define evt_new_io_event(cb, fd, flags) evt_new_event(cb,EVT_IO,fd,flags)
evt_t * evt_new_event(cb_t * cb, evt_type_t t, ...);

/* delete function for events */
void evt_delete_event(void * e);

/* start the event on the supplied event loop */
evt_ret_t evt_start_event(evt_t * evt, evt_loop_t * el);

/* stop the event */
evt_ret_t evt_stop_event(evt_t * evt);

/*
 * DEBUG
 */
void debug_signals_dump(uint8_t const * const prefix);

#endif /*EVENTS_H*/
