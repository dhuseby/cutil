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

#define EV_STANDALONE	1		/* we don't use autoconf */
#define EV_MULTIPLICITY 1		/* must pass event loop pointer to all fucntions */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "debug.h"
#include "macros.h"
#include "events.h"

/* libev signal event callback */
static void evt_signal_callback( struct ev_loop * loop,
								 struct ev_signal * w,
								 int revents )
{
	DEBUG("received signal event\n");

	/* get the pointer to our own struct */
	evt_t * evt = (evt_t*)w;

	ASSERT( evt->el == loop );
	ASSERT( revents == EV_SIGNAL );

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
	DEBUG("received child event\n");

	/* get the pointer to our own struct */
	evt_t * evt = (evt_t*)w;

	ASSERT( evt->el == loop );
	ASSERT( revents == EV_CHILD );

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
	DEBUG("receive io event\n");

	/* get the pointer to our own struct */
	evt_t * evt = (evt_t*)w;

	ASSERT( evt->el == loop );
	ASSERT( (revents & EV_READ) || (revents & EV_WRITE) );

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
	el = (evt_loop_t*)ev_default_loop( EVFLAG_AUTO | EVFLAG_NOENV );
	if ( el == NULL )
	{
		WARN("failed to start event loop\n");
		return NULL;
	}

	// log which backend was chosen
	evt_log_backend( el );
	
	return el;
}

void evt_delete(void * e)
{
	evt_loop_t* el = (evt_loop_t*)e;
	
	CHECK_PTR( el );

	/* clean up the default event loop */
	ev_default_destroy();
}


int evt_initialize_event_handler( evt_t * const evt,
								  evt_type_t const t,
								  evt_params_t * const params,
								  evt_fn callback,
								  void * user_data )
{
	CHECK_PTR_RET( evt, FALSE );
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

	evt = MALLOC( sizeof(evt_t) );
	if ( evt == NULL )
	{
		WARN( "failed to allocate event handler struct\n" );
		return NULL;
	}

	/* initialize it */
	evt_initialize_event_handler( evt, t, params, callback, user_data );

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
	CHECK_PTR_RET_MSG( el, EVT_BAD_PTR, "bad event loop pointer\n" );
	CHECK_PTR_RET_MSG( evt, EVT_BAD_PTR, "bad event pointer\n" );

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
	CHECK_PTR_RET( el, EVT_BAD_PTR );
	CHECK_PTR_RET( evt, EVT_BAD_PTR );

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
	CHECK_PTR_RET( el, EVT_BAD_PTR );

	/* start the libev event loop */
	ev_run( (struct ev_loop*)el, 0 );

	return EVT_OK;
}

evt_ret_t evt_stop( evt_loop_t * const el )
{
	CHECK_PTR_RET( el, EVT_BAD_PTR );

	/* stop the libev event loop */
	ev_break( (struct ev_loop*)el, EVUNLOOP_ALL );

	return EVT_OK;
}


