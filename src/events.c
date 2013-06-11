/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */

#define EV_STANDALONE   1       /* we don't use autoconf */
#define EV_MULTIPLICITY 1       /* must pass event loop pointer to all fucntions */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "debug.h"
#include "macros.h"
#include "events.h"

#if defined(UNIT_TESTING)
#include "test_flags.h"
extern evt_loop_t * el;
#endif

/* libev signal event callback */
static void evt_signal_callback( struct ev_loop * loop,
                                 struct ev_signal * w,
                                 int revents )
{
    CHECK_PTR( loop );
    CHECK_PTR( w );
    CHECK( revents == EV_SIGNAL );
    DEBUG("received signal event\n");

    /* get the pointer to our own struct */
    evt_t * evt = (evt_t*)w;

    CHECK( evt->el == loop );

    if ( evt->callback != NULL )
    {
        /* initialize the params with signal event data */
        evt_params_t params;
        params.signal_params.signum = evt->ev.sig.signum;

        /* call the callback */
        DEBUG("calling signal callback\n");
        (*(evt->callback))( (evt_loop_t*)loop, evt, &params, evt->user_data );
    }
}

/* libev child event callback */
static void evt_child_callback( struct ev_loop * loop,
                                struct ev_child * w,
                                int revents )
{
    CHECK_PTR( loop );
    CHECK_PTR( w );
    CHECK( revents == EV_CHILD );
    DEBUG("received child event\n");

    /* get the pointer to our own struct */
    evt_t * evt = (evt_t*)w;

    CHECK( evt->el == loop );

    if ( evt->callback != NULL )
    {
        /* initialize the params with signal event data */
        evt_params_t params;
        params.child_params.pid = evt->ev.child.pid;
        params.child_params.rpid = evt->ev.child.rpid;
        params.child_params.rstatus = evt->ev.child.rstatus;

        /* call the callback */
        DEBUG("calling child callback\n");
        (*(evt->callback))( (evt_loop_t*)loop, evt, &params, evt->user_data );
    }
}

/* livev io event callback */
static void evt_io_callback( struct ev_loop * loop,
                             struct ev_io * w,
                             int revents )
{
    CHECK_PTR( loop );
    CHECK_PTR( w );
    CHECK( (revents & EV_READ) || (revents & EV_WRITE) );
    DEBUG("receive io event\n");

    /* get the pointer to our own struct */
    evt_t * evt = (evt_t*)w;

    CHECK( evt->el == loop );

    if ( evt->callback != NULL )
    {
        /* initialize the params with io event data */
        evt_params_t params;
        params.io_params.fd = evt->ev.io.fd;
        params.io_params.types = (evt_io_type_t)evt->ev.io.events;

        /* call the callback */
        DEBUG("calling io callback\n");
        (*(evt->callback))( (evt_loop_t*)loop, evt, &params, evt->user_data );
    }
}

static void evt_log_backend( evt_loop_t * const el )
{
    unsigned int flags = 0;

    CHECK_PTR( el );

    /* get the flag specifying which backend is being used */
    flags = ev_backend( (struct ev_loop*)el );

    if ( flags & EVBACKEND_SELECT )
        DEBUG( "using SELECT backend\n" );
    if ( flags & EVBACKEND_POLL )
        DEBUG( "using POLL backend\n" );
    if ( flags & EVBACKEND_EPOLL )
        DEBUG( "using EPOLL backend\n" );
    if ( flags & EVBACKEND_KQUEUE )
        DEBUG( "using KQUEUE backend\n" );
    if ( flags & EVBACKEND_DEVPOLL )
        DEBUG( "using DEVPOLL backend\n" );
    if ( flags & EVBACKEND_PORT )
        DEBUG( "using PORT backend\n" );
}

evt_loop_t* evt_new( void )
{
    evt_loop_t* el = NULL;
    
    /* initialize the event loop, auto-selecting backend */
    el = (evt_loop_t*)EV_DEFAULT_LOOP( EVFLAG_AUTO | EVFLAG_NOENV );
    CHECK_PTR_RET_MSG( el, NULL, "failed to start event loop\n" );

    // log which backend was chosen
    evt_log_backend( el );
    
    return el;
}

void evt_delete(void * e)
{
    evt_loop_t* el = (evt_loop_t*)e;
    
    CHECK_PTR( el );

    /* clean up the default event loop */
    ev_loop_destroy( EV_DEFAULT_UC );
}


int evt_initialize_event_handler( evt_t * const evt,
                                  evt_type_t const t,
                                  evt_params_t * const params,
                                  evt_fn callback,
                                  void * user_data )
{
#if defined(UNIT_TESTING)
    if ( fake_event_handler_init )
    {
        if ( fake_event_handler_init_count == 0 )
            return fake_event_handler_init_ret;
        fake_event_handler_init_count--;
    }
#endif
    CHECK_PTR_RET( evt, FALSE );
    CHECK_RET( VALID_EVENT_TYPE( t ), FALSE );
    CHECK_PTR_RET( params, FALSE );

    MEMSET( (void*)evt, 0, sizeof(evt_t) );

    /* initialize the event handler struct */
    evt->evt_type = t;
    MEMCPY( &(evt->evt_params), params, sizeof( evt_params_t ) );
    evt->callback = callback;
    evt->user_data = user_data;
    evt->el = NULL;

    switch ( t )
    {
        case EVT_SIGNAL:
        {
            /* initialize a libev signal event */
            ev_signal_init( (struct ev_signal*)&(evt->ev.sig), 
                            evt_signal_callback, 
                            evt->evt_params.signal_params.signum );
            return TRUE;
        }
        case EVT_CHILD:
        {
            /* initialize a libev child event */
            ev_child_init( (struct ev_child*)&(evt->ev.child),
                           evt_child_callback,
                           evt->evt_params.child_params.pid,
                           evt->evt_params.child_params.trace );
            return TRUE;
        }
        case EVT_IO:
        {
            /* initialize a libev io event */
            ev_io_init( (struct ev_io*)&(evt->ev.io),
                        evt_io_callback,
                        evt->evt_params.io_params.fd,
                        evt->evt_params.io_params.types );
            return TRUE;
        }
    }

    return FALSE;
}


evt_t * evt_new_event_handler( evt_type_t const t,
                               evt_params_t * const params,
                               evt_fn callback,
                               void * user_data )
{
    evt_t * evt = NULL;

    evt = CALLOC( 1, sizeof(evt_t) );
    CHECK_PTR_RET( evt, NULL );

    /* initialize it */
    if ( !evt_initialize_event_handler( evt, t, params, callback, user_data ) )
    {
        FREE( evt );
        return NULL;
    }

    return evt;
}

void evt_deinitialize_event_handler( evt_t * const evt )
{
    /* stop the event handler if needed */
    if ( evt->el != NULL )
    {
        evt_stop_event_handler( evt->el, evt );
    }
}

void evt_delete_event_handler( void * e )
{
    evt_t * evt = (evt_t*)e;

    CHECK_PTR( evt );

    evt_deinitialize_event_handler( evt );

    FREE( evt );
};

evt_ret_t evt_start_event_handler( evt_loop_t * const el,
                                   evt_t * const evt )
{
    UNIT_TEST_RET( event_start_handler );

    CHECK_PTR_RET_MSG( el, EVT_BADPTR, "bad event loop pointer\n" );
    CHECK_PTR_RET_MSG( evt, EVT_BADPTR, "bad event pointer\n" );
    CHECK_RET( VALID_EVENT_TYPE( evt->evt_type ), EVT_BADPARAM );

    /* store the pointer to the loop we're hooking into */
    evt->el = el;

    switch ( evt->evt_type )
    {
        case EVT_SIGNAL:
        {
            /* start the libev signal event */
            DEBUG("starting signal event\n");
            ev_signal_start( (struct ev_loop*)el, (struct ev_signal*)&(evt->ev.sig) );
            break;
        }
        case EVT_CHILD:
        {
            /* start the libevn child event */
            DEBUG("staring child event\n");
            ev_child_start( (struct ev_loop*)el, (struct ev_child*)&(evt->ev.child) );
            break;
        }
        case EVT_IO:
        {
            /* start the libev io event */
            DEBUG("starting io event\n");
            ev_io_start( (struct ev_loop*)el, (struct ev_io*)&(evt->ev.io) );
            break;
        }
    }

    return EVT_OK;
}

evt_ret_t evt_stop_event_handler( evt_loop_t * const el,
                                  evt_t * const evt )
{
    UNIT_TEST_RET( event_stop_handler );

    CHECK_PTR_RET( el, EVT_BADPTR );
    CHECK_PTR_RET( evt, EVT_BADPTR );

    switch ( evt->evt_type )
    {
        case EVT_SIGNAL:
        {
            /* stop the libev signal event */
            ev_signal_stop( (struct ev_loop*)el, (struct ev_signal*)evt );
            break;
        }
        case EVT_CHILD:
        {
            /* stop the libev child event */
            ev_child_stop( (struct ev_loop*)el, (struct ev_child*)evt );
            break;
        }
        case EVT_IO:
        {
            /* stop the libev io event */
            ev_io_stop( (struct ev_loop*)el, (struct ev_io*)evt );
            break;
        }
    }

    /* clear our pointer to the event loop */
    evt->el = NULL;

    return EVT_OK;
}


evt_ret_t evt_run( evt_loop_t * const el )
{
    CHECK_PTR_RET( el, EVT_BADPTR );

    /* start the libev event loop */
    ev_run( (struct ev_loop*)el, 0 );

    return EVT_OK;
}

evt_ret_t evt_stop( evt_loop_t * const el, int once )
{
    CHECK_PTR_RET( el, EVT_BADPTR );

    /* stop the libev event loop */
    if ( once )
        ev_break( (struct ev_loop*)el, EVBREAK_ONE );
    else
        ev_break( (struct ev_loop*)el, EVBREAK_ALL );

    return EVT_OK;
}

#if defined(UNIT_TESTING)

#include <CUnit/Basic.h>

static int test_flag = FALSE;

evt_ret_t test_evt_callback( evt_loop_t * const el, evt_t * const evt, evt_params_t * const params, void * user_data )
{
    *((int*)user_data) = TRUE;
}

void test_evt_signal_callback( void )
{
    evt_t evt;
    MEMSET( &evt, 0, sizeof(evt_t) );

    evt_signal_callback( NULL, NULL, 0 );
    evt_signal_callback( (struct ev_loop*)el, NULL, 0 );
    evt_signal_callback( (struct ev_loop*)el, (struct ev_signal*)&evt, 0 );
    evt_signal_callback( (struct ev_loop*)el, (struct ev_signal*)&evt, EV_SIGNAL );

    evt.el = el;
    evt_signal_callback( (struct ev_loop*)el, (struct ev_signal*)&evt, EV_SIGNAL );
    
    evt.callback = &test_evt_callback;
    evt.user_data = &test_flag;
    test_flag = FALSE;
    evt_signal_callback( (struct ev_loop*)el, (struct ev_signal*)&evt, EV_SIGNAL );
    CU_ASSERT_TRUE( test_flag );
    test_flag = FALSE;
}

void test_evt_child_callback( void )
{
    evt_t evt;
    MEMSET( &evt, 0, sizeof(evt_t) );

    evt_child_callback( NULL, NULL, 0 );
    evt_child_callback( (struct ev_loop*)el, NULL, 0 );
    evt_child_callback( (struct ev_loop*)el, (struct ev_child*)&evt, 0 );
    evt_child_callback( (struct ev_loop*)el, (struct ev_child*)&evt, EV_CHILD );

    evt.el = el;
    evt_child_callback( (struct ev_loop*)el, (struct ev_child*)&evt, EV_CHILD );
    
    evt.callback = &test_evt_callback;
    evt.user_data = &test_flag;
    test_flag = FALSE;
    evt_child_callback( (struct ev_loop*)el, (struct ev_child*)&evt, EV_CHILD );
    CU_ASSERT_TRUE( test_flag );
    test_flag = FALSE;
}

void test_evt_io_callback( void )
{
    evt_t evt;
    MEMSET( &evt, 0, sizeof(evt_t) );

    evt_io_callback( NULL, NULL, 0 );
    evt_io_callback( (struct ev_loop*)el, NULL, 0 );
    evt_io_callback( (struct ev_loop*)el, (struct ev_io*)&evt, 0 );
    evt_io_callback( (struct ev_loop*)el, (struct ev_io*)&evt, EV_READ );

    evt.el = el;
    evt_io_callback( (struct ev_loop*)el, (struct ev_io*)&evt, EV_READ );
    
    evt.callback = &test_evt_callback;
    evt.user_data = &test_flag;
    test_flag = FALSE;
    evt_io_callback( (struct ev_loop*)el, (struct ev_io*)&evt, EV_READ );
    CU_ASSERT_TRUE( test_flag );
    test_flag = FALSE;
}

void test_evt_log_backend( void )
{
    evt_log_backend( NULL );
}


void test_events_private_functions( void )
{
    test_evt_signal_callback();
    test_evt_child_callback();
    test_evt_io_callback();
    test_evt_log_backend();
}

#endif

