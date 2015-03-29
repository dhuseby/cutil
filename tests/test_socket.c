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
#include <sys/socket.h>
#include <netdb.h>

#include <CUnit/Basic.h>

#include <cutil/debug.h>
#include <cutil/macros.h>
#include <cutil/socket.h>

#include "test_macros.h"
#include "test_flags.h"

#define REPEAT (128)
#define SIZEMAX (128)
#define MULTIPLE (8)

extern evt_loop_t * el;
extern void test_socket_private_functions( void );
static cb_t *cb;
static int conn_evts;
static int disc_evts;
static int err_evts;
static int read_evts;
static int write_evts;

static void conn_evt_cb(int *i, socket_t *s, socket_ret_t *ret)
{
}

static void disc_evt_cb(int *i, socket_t *s)
{
}

static void err_evt_cb(int *i, socket_t *s, int eno)
{
}

static void read_evt_cb(int *i, socket_t *s, size_t n)
{
}

static void write_evt_cb(int *i, socket_t *s, void *buf, size_t n)
{
}

S_CONNECT_EVT_CB(conn_evt_cb, int*);
S_DISCONNECT_EVT_CB(disc_evt_cb, int*);
S_ERROR_EVT_CB(err_evt_cb, int*);
S_READ_EVT_CB(read_evt_cb, int*);
S_WRITE_EVT_CB(write_evt_cb, int*);


static void test_socket_newdel( void )
{
  int i;
  socket_t * s;
  socket_type_t type;

  for ( i = 0; i < REPEAT; i++ )
  {
    s = NULL;
    /* randomly select from SOCKET_TCP, SOCKET_UDP, and SOCKET_UNIX */
    type = SOCKET_FIRST + (rand() % SOCKET_COUNT);

    switch( type )
    {
      case SOCKET_TCP:
      case SOCKET_UDP:
        s = socket_new( type, NULL, "80", AI_PASSIVE, AF_UNSPEC, el );
        break;
      case SOCKET_UNIX:
        s = socket_new( type, "/tmp/blah", NULL, 0, 0, el );
        break;
    }

    CU_ASSERT_PTR_NOT_NULL( s );
    CU_ASSERT_EQUAL( socket_get_type( s ), type );
    CU_ASSERT_EQUAL( socket_is_connected( s ), FALSE );
    CU_ASSERT_EQUAL( socket_is_bound( s ), FALSE );

    socket_delete( s );
  }
#if 0
  /* clean up after ourselves */
  unlink( "/tmp/blah" );
#endif
}

#if 0
static void test_socket_bad_hostname( void )
{
  socket_t * s;

  s = socket_new( SOCKET_TCP, "invalid.hostname", "80", 0, AF_UNSPEC, el );

  CU_ASSERT_PTR_NULL( s );
}


typedef struct sock_state_s
{
    int_t connected;
    int_t error;
} sock_state_t;

static void connect_tests_connect_cb( uint8_t const * const name, void * state, void * param)
{
  socket_event_t *svt = (socket_event_t*)state;
  sock_state_t *s = (sock_state_t*)param;

  CU_ASSERT_EQUAL( s->error, FALSE );
  s->connected = TRUE;

  evt_stop( el, FALSE );
  svt->ret = SOCKET_OK;
}

static void connect_tests_error_cb( uint8_t const * const name, void * state, void * param)
{
  socket_event_t *svt = (socket_event_t*)state;
  sock_state_t *s = (sock_state_t*)param;

  CU_ASSERT_EQUAL( s->connected, FALSE );
  s->error = TRUE;

  evt_stop( el, FALSE );
  svt->ret = SOCKET_OK;
}

static void test_tcp_socket_failed_connection( void )
{
    sock_state_t state = { FALSE, FALSE };
    socket_t * s;

    s = socket_new( SOCKET_TCP, "localhost", "5559", 0, AF_INET, el );

    CU_ASSERT_PTR_NOT_NULL( s );
    CU_ASSERT_EQUAL( socket_get_type( s ), SOCKET_TCP );
    CU_ASSERT_EQUAL( socket_is_connected( s ), FALSE );
    CU_ASSERT_EQUAL( socket_is_bound( s ), FALSE );

    /* hook up callbacks */
    CU_ASSERT_EQUAL( socket_add_cb( s, "socket-connect", (void*)&state, &connect_tests_connect_cb ), SOCKET_OK );
    CU_ASSERT_EQUAL( socket_add_cb( s, "socket-error", (void*)&state, &connect_tests_error_cb ), SOCKET_OK );

    /* connect to the socket */
    CU_ASSERT_EQUAL( socket_connect( s ), SOCKET_OK );

    /* run the event loop */
    evt_run( el );

    CU_ASSERT_EQUAL( state.error, TRUE );
    CU_ASSERT_EQUAL( state.connected, FALSE );

    socket_delete( s );
}




static int t_sdone = FALSE;
static int t_cdone = FALSE;
static int t_sclose = FALSE;

static socket_ret_t t_server_connect_fn( socket_t * const s, void * user_data )
{
    DEBUG("server socket connect callback\n");
    return SOCKET_OK;
}

static socket_ret_t t_server_disconnect_fn( socket_t * const s, void * user_data )
{
    t_sdone = TRUE;

    DEBUG("server socket disconnect callback\n");
    if ( t_sdone && t_cdone )
    {
        evt_stop( el, FALSE );
    }

    return SOCKET_OK;
}

static socket_ret_t t_server_error_fn( socket_t * const s, int err, void * user_data )
{
    return SOCKET_OK;
}

static ssize_t t_server_read_fn( socket_t * const s, size_t const nread, void * user_data )
{
    uint8_t ping[6];
    uint8_t const * const pong = UT("PONG!");

    DEBUG("server socket read callback\n");

    CU_ASSERT_EQUAL( nread, 6 );

    socket_read( s, UT(ping), 6 );

    CU_ASSERT_EQUAL( strcmp( C(ping), "PING!" ), 0 );
    DEBUG("TCP server received %s\n", ping);

    DEBUG("TCP server writing PONG!");
    socket_write( s, pong, 6 );

    /* tell the server to disconnect after the next write completes */
    t_sclose = TRUE;

    return 6;
}

static ssize_t t_server_write_fn( socket_t * const s, uint8_t const * const buffer, void * user_data )
{
    socket_flush( s );
    DEBUG("server socket write completion callback\n");

    if ( t_sclose == TRUE )
    {
        socket_disconnect( s );
    }

    return 0;
}

/* gets called when an incoming connection happens on the listening socket */
static socket_ret_t t_incoming_fn( socket_t * const s, void * user_data )
{
    socket_t ** server = (socket_t **)user_data;
    socket_ops_t sops =
    {
        &t_server_connect_fn,
        &t_server_disconnect_fn,
        &t_server_error_fn,
        &t_server_read_fn,
        &t_server_write_fn
    };
    CHECK_RET( socket_get_type( s ) == SOCKET_TCP, SOCKET_ERROR );
    CHECK_RET( socket_is_bound( s ), SOCKET_ERROR );

    DEBUG("listen socket incoming callback...calling socket_accept %p\n", (void*)s);

    (*server) = socket_accept( s, &sops, el, NULL );
    DEBUG("server socket %p\n", (void*)(*server));

    CU_ASSERT_PTR_NOT_NULL( (*server) );
    CHECK_PTR_RET( (*server), SOCKET_ERROR );

    return SOCKET_OK;
}

static socket_ret_t t_client_connect_fn( socket_t * const s, void * user_data )
{
    uint8_t const * const ping = UT("PING!");

    DEBUG("client socket connect callback, server sending PING!\n");
    socket_write( s, ping, 6 );

    return SOCKET_OK;
}

static socket_ret_t t_client_disconnect_fn( socket_t * const s, void * user_data )
{
    t_cdone = TRUE;

    DEBUG("client socket disconnect callback\n");
    if ( t_sdone && t_cdone )
    {
        evt_stop( el, FALSE );
    }

    return SOCKET_OK;
}

static socket_ret_t t_client_error_fn( socket_t * const s, int err, void * user_data )
{
    return SOCKET_OK;
}

static ssize_t t_client_read_fn( socket_t * const s, size_t const nread, void * user_data )
{
    uint8_t pong[6];

    DEBUG("client socket read callback\n");
    CU_ASSERT_EQUAL( nread, 6 );

    socket_read( s, UT(pong), 6 );

    CU_ASSERT_EQUAL( strcmp( C(pong), "PONG!" ), 0 );

    DEBUG("TCP client received %s\n", pong);

    socket_disconnect( s );

    return 6;
}

static ssize_t t_client_write_fn( socket_t * const s, uint8_t const * const buffer, void * user_data )
{
    DEBUG("client socket write completion callback\n");
    return 0;
}

static void test_tcp_socket( void )
{
    socket_t * lsock;
    socket_t * ssock;
    socket_t * csock;

    socket_ops_t lops = { &t_incoming_fn, NULL, NULL, NULL, NULL };
    socket_ops_t cops =
    {
        &t_client_connect_fn,
        &t_client_disconnect_fn,
        &t_client_error_fn,
        &t_client_read_fn,
        &t_client_write_fn
    };

    /* create the listening socket */
    lsock = socket_new( SOCKET_TCP, NULL, "12121", AI_PASSIVE, AF_UNSPEC, &lops, el, (void*)&ssock );
    CU_ASSERT_PTR_NOT_NULL_FATAL( lsock );

    /* bind it */
    CU_ASSERT_EQUAL( socket_bind( lsock ), SOCKET_OK );
    CU_ASSERT_TRUE( socket_is_bound( lsock ) );

    /* set it to listen */
    DEBUG("listening socket %p\n", (void*)lsock);
    CU_ASSERT_EQUAL( socket_listen( lsock, 5 ), SOCKET_OK );
    CU_ASSERT_TRUE( socket_is_listening( lsock ) );

    /* create the client socket */
    csock = socket_new( SOCKET_TCP, "127.0.0.1", "12121", 0, AF_INET, &cops, el, NULL );
    CU_ASSERT_PTR_NOT_NULL( csock );
    DEBUG("client socket %p\n", (void*)csock);

    /* connect to the socket */
    socket_connect( csock );

    /* run the event loop */
    evt_run( el );

    socket_delete( lsock );
    socket_delete( ssock );
    socket_delete( csock );
}

static int_t u_sdone = FALSE;
static int_t u_cdone = FALSE;
static int_t u_sexit = FALSE;
static int_t u_cexit = FALSE;
static int_t u_cconnected = FALSE;
static int_t u_sconnected = FALSE;

static socket_ret_t u_server_error_fn( socket_t * const s, int err, void * user_data )
{
    DEBUG("server error callback\n");
    return SOCKET_OK;
}

static ssize_t u_server_read_fn( socket_t * const s, size_t const nread, void * user_data )
{
    static uint8_t buf[1024];
    uint8_t ping[6];
    uint8_t const * const pong = UT("PONG!");
    sockaddr_t addr;
    socklen_t addrlen = sizeof(sockaddr_t);

    DEBUG("server read event! (nread: %u)\n", (unsigned int)nread);

    CU_ASSERT_EQUAL( nread, 6 );

    MEMSET( &addr, 0, sizeof(sockaddr_t) );
    socket_read_from( s, UT(ping), 6, &addr, &addrlen );

    CU_ASSERT_EQUAL( strcmp( C(ping), "PING!" ), 0 );

    MEMSET( buf, 0, 1024);
    socket_get_addr_string( &addr, buf, 1024 );
    DEBUG("received %s from: %s\n", ping, buf );

    /* the server side has written the response */
    u_sdone = TRUE;

    DEBUG("server writing %s to: %s\n", pong, buf );
    socket_write_to( s, pong, 6, &addr, addrlen );

    return 6;
}

static ssize_t u_server_write_fn( socket_t * const s, uint8_t const * const buffer, void * user_data )
{
    if ( buffer == NULL )
    {
        if ( u_sconnected == FALSE )
        {
            DEBUG("server socket open write callback\n");
            u_sconnected = TRUE;
        }
        else
        {
            DEBUG("server all writes complete callback\n");
        }
    }
    else
    {
        DEBUG("server normal write completion\n");
        socket_flush( s );
        if ( u_sdone == TRUE )
            u_sexit = TRUE;
    }

    return 0;
}

static socket_ret_t u_client_error_fn( socket_t * const s, int err, void * user_data )
{
    return SOCKET_OK;
}

static ssize_t u_client_read_fn( socket_t * const s, size_t const nread, void * user_data )
{
    static uint8_t buf[1024];
    sockaddr_t addr;
    socklen_t addrlen = sizeof(sockaddr_t);
    uint8_t pong[6];

    DEBUG("client read callback\n");
    CU_ASSERT_EQUAL( nread, 6 );

    MEMSET( &addr, 0, sizeof(sockaddr_t) );
    socket_read_from( s, UT(pong), 6, &addr, &addrlen );

    CU_ASSERT_EQUAL( strcmp( C(pong), "PONG!" ), 0 );

    MEMSET( buf, 0, 1024);
    socket_get_addr_string( &addr, buf, 1024 );
    DEBUG("received %s from: %s\n", pong, buf );

    if ( u_sexit && u_cexit )
    {
        DEBUG("exiting the event loop\n");
        evt_stop( el, FALSE );
    }

    return 6;
}

static ssize_t u_client_write_fn( socket_t * const s, uint8_t const * const buffer, void * user_data )
{
    static uint8_t buf[1024];
    sockaddr_t addr;
    socklen_t addrlen = 0;
    uint8_t const * const ping = UT("PING!");


    if ( buffer == NULL )
    {
        if ( u_cconnected == FALSE )
        {
            DEBUG("client socket open write callback\n");

            MEMSET( &addr, 0, sizeof( sockaddr_t ) );
            socket_get_addr( s, &addr, &addrlen );

            MEMSET( buf, 0, 1024);
            socket_get_addr_string( &addr, buf, 1024 );

            /* this is the initial write event for a newly created UDP socket */
            DEBUG("client sending PING! to %s\n", buf);
            socket_write_to( s, ping, 6, &addr, addrlen );
            u_cconnected = TRUE;
            u_cdone = TRUE;
        }
        else
        {
            DEBUG("client all writes complete callback\n");
        }
    }
    else
    {
        DEBUG("client normal write complete\n");
        if ( u_cdone == TRUE )
            u_cexit = TRUE;
    }
    return 0;
}

static void test_udp_socket( void )
{
    socket_t * ssock;
    socket_t * csock;

    socket_ops_t sops =
    {
        NULL,
        NULL,
        &u_server_error_fn,
        &u_server_read_fn,
        &u_server_write_fn
    };
    socket_ops_t cops =
    {
        NULL,
        NULL,
        &u_client_error_fn,
        &u_client_read_fn,
        &u_client_write_fn
    };

    /* create the listening socket */
    ssock = socket_new( SOCKET_UDP, NULL, "12122", AI_PASSIVE, AF_INET, &sops, el, NULL );
    DEBUG( "server socket %p\n", (void*)ssock );
    CU_ASSERT_PTR_NOT_NULL_FATAL( ssock );

    /* bind it */
    CU_ASSERT_EQUAL( socket_bind( ssock ), SOCKET_OK );
    CU_ASSERT_TRUE( socket_is_bound( ssock ) );

    /* create the client socket */
    csock = socket_new( SOCKET_UDP, "127.0.0.1", "12122", 0, AF_INET, &cops, el, NULL );
    DEBUG( "client socket %p\n", (void*)csock );
    CU_ASSERT_PTR_NOT_NULL( csock );

    /* run the event loop */
    DEBUG("running event loop\n");
    evt_run( el );

    socket_delete( ssock );
    socket_delete( csock );
}



static int x_sdone = FALSE;
static int x_cdone = FALSE;
static int x_sclose = FALSE;

static socket_ret_t x_server_connect_fn( socket_t * const s, void * user_data )
{
    return SOCKET_OK;
}

static socket_ret_t x_server_disconnect_fn( socket_t * const s, void * user_data )
{
    x_sdone = TRUE;

    if ( x_sdone && x_cdone )
    {
        evt_stop( el, FALSE );
    }

    return SOCKET_OK;
}

static socket_ret_t x_server_error_fn( socket_t * const s, int err, void * user_data )
{
    return SOCKET_OK;
}

static ssize_t x_server_read_fn( socket_t * const s, size_t const nread, void * user_data )
{
    uint8_t * ping[6];
    uint8_t const * const pong = UT("PONG!");

    CU_ASSERT_EQUAL( nread, 6 );

    CU_ASSERT_EQUAL( socket_read( s, UT(ping), 6 ), 6 );

    CU_ASSERT_EQUAL( strcmp( C(ping), "PING!" ), 0 );

    CU_ASSERT_EQUAL( socket_write( s, pong, 6 ), SOCKET_OK );

    /* signal to the server that it should disconnect after the next write completes */
    x_sclose = TRUE;

    return 6;
}

static ssize_t x_server_write_fn( socket_t * const s, uint8_t const * const buffer, void * user_data )
{
    if ( x_sclose == TRUE )
    {
        CU_ASSERT_EQUAL( socket_disconnect( s ), SOCKET_OK );
    }

    return 0;
}

/* gets called when an incoming connection happens on the listening socket */
static socket_ret_t x_incoming_fn( socket_t * const s, void * user_data )
{
    socket_t ** server = (socket_t **)user_data;
    socket_ops_t sops =
    {
        &x_server_connect_fn,
        &x_server_disconnect_fn,
        &x_server_error_fn,
        &x_server_read_fn,
        &x_server_write_fn
    };
    CHECK_RET( socket_get_type( s ) == SOCKET_UNIX, SOCKET_ERROR );
    CHECK_RET( socket_is_bound( s ), SOCKET_ERROR );

    (*server) = socket_accept( s, &sops, el, NULL );
    CHECK_PTR_RET( (*server), SOCKET_ERROR );

    return SOCKET_OK;
}

static socket_ret_t x_client_connect_fn( socket_t * const s, void * user_data )
{
    uint8_t const * const ping = UT("PING!");

    CU_ASSERT_EQUAL( socket_write( s, ping, 6 ), SOCKET_OK );

    return SOCKET_OK;
}

static socket_ret_t x_client_disconnect_fn( socket_t * const s, void * user_data )
{
    x_cdone = TRUE;

    if ( x_sdone && x_cdone )
    {
        evt_stop( el, FALSE );
    }

    return SOCKET_OK;
}

static socket_ret_t x_client_error_fn( socket_t * const s, int err, void * user_data )
{
    return SOCKET_OK;
}

static ssize_t x_client_read_fn( socket_t * const s, size_t const nread, void * user_data )
{
    uint8_t * pong[6];

    CU_ASSERT_EQUAL( nread, 6 );

    CU_ASSERT_EQUAL( socket_read( s, UT(pong), 6 ), 6 );

    CU_ASSERT_EQUAL( strcmp( C(pong), "PONG!" ), 0 );

    CU_ASSERT_EQUAL( socket_disconnect( s ), SOCKET_OK );

    return 6;
}

static ssize_t x_client_write_fn( socket_t * const s, uint8_t const * const buffer, void * user_data )
{
    return 0;
}

static void test_unix_socket( void )
{
    socket_t * lsock;
    socket_t * ssock;
    socket_t * csock;

    socket_ops_t lops = { &x_incoming_fn, NULL, NULL, NULL, NULL };
    socket_ops_t cops =
    {
        &x_client_connect_fn,
        &x_client_disconnect_fn,
        &x_client_error_fn,
        &x_client_read_fn,
        &x_client_write_fn
    };

    /* create the listening socket */
    lsock = socket_new( SOCKET_UNIX, "/tmp/blah", NULL, 0, 0, &lops, el, (void*)&ssock );
    CU_ASSERT_PTR_NOT_NULL( lsock );

    /* bind it */
    CU_ASSERT_EQUAL( socket_bind( lsock ), SOCKET_OK );
    CU_ASSERT_TRUE( socket_is_bound( lsock ) );

    /* set it to listen */
    CU_ASSERT_EQUAL( socket_listen( lsock, 5 ), SOCKET_OK );

    /* create the client socket */
    csock = socket_new( SOCKET_UNIX, "/tmp/blah", NULL, 0, 0, &cops, el, NULL );
    CU_ASSERT_PTR_NOT_NULL( csock );

    /* connect to the socket */
    CU_ASSERT_EQUAL( socket_connect( csock ), SOCKET_OK );

    /* run the event loop */
    evt_run( el );

    socket_delete( lsock );
    socket_delete( ssock );
    socket_delete( csock );

    /* clean up after ourselves */
    unlink( "/tmp/blah" );
}

static void test_socket_delete_null( void )
{
    socket_delete( NULL );
}

static void test_socket_new_fail_alloc( void )
{
    socket_ops_t ops =
    {
        &connect_fn,
        &disconnect_fn,
        &error_fn,
        &read_fn,
        &write_fn
    };

    fail_alloc = TRUE;
    CU_ASSERT_PTR_NULL( socket_new( SOCKET_TCP, NULL, "80", AI_PASSIVE, AF_UNSPEC, &ops, el, NULL ) );
    fail_alloc = FALSE;
}

static void test_socket_new_fail_init( void )
{
    socket_ops_t ops =
    {
        &connect_fn,
        &disconnect_fn,
        &error_fn,
        &read_fn,
        &write_fn
    };

    fail_socket_initialize = TRUE;
    CU_ASSERT_PTR_NULL( socket_new( SOCKET_TCP, NULL, "80", AI_PASSIVE, AF_UNSPEC, &ops, el, NULL ) );
    fail_socket_initialize = FALSE;
}

static void test_socket_write( void )
{
    CU_ASSERT_EQUAL( socket_write( NULL, NULL, 0 ), SOCKET_BADPARAM );
}

static void test_socket_writev( void )
{
    CU_ASSERT_EQUAL( socket_writev( NULL, NULL, 0 ), SOCKET_BADPARAM );
}

static void test_socket_get_type( void )
{
    socket_t * s = NULL;
    socket_ops_t ops =
    {
        &connect_fn,
        &disconnect_fn,
        &error_fn,
        &read_fn,
        &write_fn
    };
    s = socket_new( SOCKET_TCP, NULL, "80", AI_PASSIVE, AF_UNSPEC, &ops, el, NULL );
    CU_ASSERT_PTR_NOT_NULL( s );
    CU_ASSERT_EQUAL( socket_get_type( NULL ), SOCKET_UNKNOWN );
    CU_ASSERT_EQUAL( socket_get_type( s ), SOCKET_TCP );
    socket_delete( s );
}

static void test_socket_disconnect( void )
{
    CU_ASSERT_EQUAL( socket_disconnect( NULL ), SOCKET_BADPARAM );
}

static void test_socket_flush( void )
{
    CU_ASSERT_EQUAL( socket_flush( NULL ), SOCKET_BADPARAM );
}
#endif

static int init_socket_suite( void )
{
  srand(0xDEADBEEF);
  reset_test_flags();
  cb = cb_new();
  S_ADD_CONNECT_EVT_CB(cb, conn_evt_cb, &conn_evts);
  S_ADD_DISCONNECT_EVT_CB(cb, disc_evt_cb, &disc_evts);
  S_ADD_ERROR_EVT_CB(cb, err_evt_cb, &err_evts);
  S_ADD_READ_EVT_CB(cb, read_evt_cb, &read_evts);
  S_ADD_WRITE_EVT_CB(cb, write_evt_cb, &write_evts);
  return 0;
}

static int deinit_socket_suite( void )
{
  cb_delete(cb);
  reset_test_flags();
  return 0;
}

static CU_pSuite add_socket_tests( CU_pSuite pSuite )
{
    ADD_TEST( "new/delete of socket", test_socket_newdel );
#if 0
    ADD_TEST( "udp socket ping/pong", test_udp_socket );
    ADD_TEST( "tcp socket ping/pong", test_tcp_socket );
    ADD_TEST( "unix socket ping/pong", test_unix_socket );
    ADD_TEST( "socket bad hostname", test_socket_bad_hostname );
    ADD_TEST( "tcp failed connection", test_tcp_socket_failed_connection );
    ADD_TEST( "socket delete null", test_socket_delete_null );
    ADD_TEST( "socket new fail alloc", test_socket_new_fail_alloc );
    ADD_TEST( "socket new fail init", test_socket_new_fail_init );
    ADD_TEST( "socket write", test_socket_write );
    ADD_TEST( "socket writev", test_socket_writev );
    ADD_TEST( "socket get type", test_socket_get_type );
    ADD_TEST( "socket disconnect", test_socket_disconnect );
    ADD_TEST( "socket flush", test_socket_flush );
#endif
    ADD_TEST( "socket private functions", test_socket_private_functions );
    return pSuite;
}

CU_pSuite add_socket_test_suite()
{
    CU_pSuite pSuite = NULL;

    /* add the suite to the registry */
    pSuite = CU_add_suite("Socket Tests", init_socket_suite, deinit_socket_suite);
    CHECK_PTR_RET( pSuite, NULL );

    /* add in hashtable specific tests */
    CHECK_PTR_RET( add_socket_tests( pSuite ), NULL );

    return pSuite;
}

