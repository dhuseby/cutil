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
#include <cutil/socket.h>

#define REPEAT (128)
#define SIZEMAX (128)
#define MULTIPLE (8)

static evt_loop_t * el = NULL;

static socket_ret_t connect_fn( socket_t * const s, void * user_data )
{
	return SOCKET_OK;
}

static socket_ret_t disconnect_fn( socket_t * const s, void * user_data )
{
	return SOCKET_OK;
}

static socket_ret_t error_fn( socket_t * const s, int err, void * user_data )
{
	return SOCKET_OK;
}

static int32_t read_fn( socket_t * const s, size_t nread, void * user_data )
{
	return 0;
}

static int32_t write_fn( socket_t * const s, uint8_t const * const buffer, void * user_data )
{
	return 0;
}

static void test_socket_newdel( void )
{
	int i;
	socket_t * s;
	socket_type_t type;
	socket_ops_t ops =
	{
		&connect_fn,
		&disconnect_fn,
		&error_fn,
		&read_fn,
		&write_fn
	};

	for ( i = 0; i < REPEAT; i++ )
	{
		s = NULL;
		type = ((rand() & 1) ? SOCKET_TCP : SOCKET_UNIX);
		s = socket_new( type, &ops, el, NULL );

		CU_ASSERT_PTR_NOT_NULL( s );
		CU_ASSERT_EQUAL( socket_get_type( s ), type );
		CU_ASSERT_EQUAL( socket_is_connected( s ), FALSE );
		CU_ASSERT_EQUAL( socket_is_bound( s ), FALSE );

		socket_delete( s );
	}
}

static void test_socket_bad_hostname( void )
{
	socket_t * s;
	socket_ops_t ops =
	{
		&connect_fn,
		&disconnect_fn,
		&error_fn,
		&read_fn,
		&write_fn
	};

	s = socket_new( SOCKET_TCP, &ops, el, NULL );

	CU_ASSERT_PTR_NOT_NULL( s );
	CU_ASSERT_EQUAL( socket_get_type( s ), SOCKET_TCP );
	CU_ASSERT_EQUAL( socket_is_connected( s ), FALSE );
	CU_ASSERT_EQUAL( socket_is_bound( s ), FALSE );
	
	/* connect to the socket */
	CU_ASSERT_EQUAL( socket_connect( s, "invalid.hostname", 80 ), SOCKET_BADHOSTNAME );

	socket_delete( s );
}


typedef struct sock_state_s
{
	int connected;
	int error;
} sock_state_t;

static socket_ret_t connect_tests_connect_fn( socket_t * const s, void * user_data )
{
	sock_state_t * state = (sock_state_t*)user_data;

	CU_ASSERT_EQUAL( state->error, FALSE );
	state->connected = TRUE;

	evt_stop( el, FALSE );
	
	return SOCKET_OK;
}

static socket_ret_t connect_tests_error_fn( socket_t * const s, int err, void * user_data )
{
	sock_state_t * state = (sock_state_t*)user_data;

	CU_ASSERT_EQUAL( state->connected, FALSE );
	state->error = TRUE;

	evt_stop( el, FALSE );

	return SOCKET_OK;
}

static void test_tcp_socket_failed_connection( void )
{
	sock_state_t state = { FALSE, FALSE };
	socket_t * s;
	socket_ops_t ops =
	{
		&connect_tests_connect_fn,
		&disconnect_fn,
		&connect_tests_error_fn,
		&read_fn,
		&write_fn
	};

	s = socket_new( SOCKET_TCP, &ops, el, (void*)&state );

	CU_ASSERT_PTR_NOT_NULL( s );
	CU_ASSERT_EQUAL( socket_get_type( s ), SOCKET_TCP );
	CU_ASSERT_EQUAL( socket_is_connected( s ), FALSE );
	CU_ASSERT_EQUAL( socket_is_bound( s ), FALSE );
	
	/* connect to the socket */
	CU_ASSERT_EQUAL( socket_connect( s, "localhost", 5559 ), SOCKET_OK );

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
	return SOCKET_OK;
}

static socket_ret_t t_server_disconnect_fn( socket_t * const s, void * user_data )
{
	t_sdone = TRUE;

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

static int32_t t_server_read_fn( socket_t * const s, size_t nread, void * user_data )
{
	uint8_t ping[6];
	uint8_t const * const pong = UT("PONG!");

	CU_ASSERT_EQUAL( nread, 6 );

	NOTICE("server receiving data\n");
	socket_read( s, UT(ping), 6 );

	CU_ASSERT_EQUAL( strcmp( C(ping), "PING!" ), 0 );
	NOTICE("server received %s\n", ping );

	socket_write( s, pong, 6 );

	/* tell the server to disconnect after the next write completes */
	t_sclose = TRUE;

	return 6;
}

static int32_t t_server_write_fn( socket_t * const s, uint8_t const * const buffer, void * user_data )
{
	socket_flush( s );

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

	NOTICE("accepting incoming connection\n");

	(*server) = socket_accept( s, &sops, el, NULL );
	NOTICE("accepted!\n");
	CU_ASSERT_PTR_NOT_NULL( (*server) );
	CHECK_PTR_RET( (*server), SOCKET_ERROR );

	return SOCKET_OK;
}

static socket_ret_t t_client_connect_fn( socket_t * const s, void * user_data )
{
	uint8_t const * const ping = UT("PING!");

	NOTICE("sending PING! from client\n");
	socket_write( s, ping, 6 );

	return SOCKET_OK;
}

static socket_ret_t t_client_disconnect_fn( socket_t * const s, void * user_data )
{
	t_cdone = TRUE;

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

static int32_t t_client_read_fn( socket_t * const s, size_t nread, void * user_data )
{
	uint8_t pong[6];

	CU_ASSERT_EQUAL( nread, 6 );

	NOTICE("client receiving data\n");
	socket_read( s, UT(pong), 6 );

	CU_ASSERT_EQUAL( strcmp( C(pong), "PONG!" ), 0 );
	NOTICE("client received %s\n", pong);

	socket_disconnect( s );

	return 6;
}

static int32_t t_client_write_fn( socket_t * const s, uint8_t const * const buffer, void * user_data )
{
	return 0;
}

static void test_tcp_socket( void )
{
	socket_t * lsock;
	socket_t * ssock;
	socket_t * csock;

	socket_ops_t lops = { &t_incoming_fn, NULL, NULL, NULL, NULL };
	socket_ops_t cops = { &t_client_connect_fn, &t_client_disconnect_fn, &t_client_error_fn, &t_client_read_fn, &t_client_write_fn };

	/* create the listening socket */
	lsock = socket_new( SOCKET_TCP, &lops, el, (void*)&ssock );
	CU_ASSERT_PTR_NOT_NULL_FATAL( lsock );
	
	/* bind it */
	CU_ASSERT_EQUAL( socket_bind( lsock, "127.0.0.1", 12121 ), SOCKET_OK );

	/* set it to listen */
	CU_ASSERT_EQUAL( socket_listen( lsock, 5 ), SOCKET_OK );

	/* create the client socket */
	csock = socket_new( SOCKET_TCP, &cops, el, NULL );
	CU_ASSERT_PTR_NOT_NULL( lsock );

	/* connect to the socket */
	socket_connect( csock, "127.0.0.1", 12121 );

	/* run the event loop */
	evt_run( el );

	socket_delete( lsock );
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

static int32_t x_server_read_fn( socket_t * const s, size_t nread, void * user_data )
{
	uint8_t * ping[6];
	uint8_t const * const pong = UT("PONG!");

	CU_ASSERT_EQUAL( nread, 6 );

	socket_read( s, UT(ping), 6 );

	CU_ASSERT_EQUAL( strcmp( C(ping), "PING!" ), 0 );

	socket_write( s, pong, 6 );

	/* signal to the server that it should disconnect after the next write completes */
	x_sclose = TRUE;

	return 6;
}

static int32_t x_server_write_fn( socket_t * const s, uint8_t const * const buffer, void * user_data )
{
	if ( x_sclose == TRUE )
	{
		socket_disconnect( s );
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

	socket_write( s, ping, 6 );

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

static int32_t x_client_read_fn( socket_t * const s, size_t nread, void * user_data )
{
	uint8_t * pong[6];

	CU_ASSERT_EQUAL( nread, 6 );

	socket_read( s, UT(pong), 6 );

	CU_ASSERT_EQUAL( strcmp( C(pong), "PONG!" ), 0 );

	socket_disconnect( s );

	return 6;
}

static int32_t x_client_write_fn( socket_t * const s, uint8_t const * const buffer, void * user_data )
{
	return 0;
}

static void test_unix_socket( void )
{
	socket_t * lsock;
	socket_t * ssock;
	socket_t * csock;

	socket_ops_t lops = { &x_incoming_fn, NULL, NULL, NULL, NULL };
	socket_ops_t cops = { &x_client_connect_fn, &x_client_disconnect_fn, &x_client_error_fn, &x_client_read_fn, &x_client_write_fn };

	/* create the listening socket */
	lsock = socket_new( SOCKET_UNIX, &lops, el, (void*)&ssock );
	
	/* bind it */
	socket_bind( lsock, "/tmp/blah", 0 );

	/* set it to listen */
	socket_listen( lsock, 5 );

	/* create the client socket */
	csock = socket_new( SOCKET_UNIX, &cops, el, NULL );

	/* connect to the socket */
	socket_connect( csock, "/tmp/blah", 0 );

	/* run the event loop */
	evt_run( el );

	socket_delete( lsock );
	socket_delete( ssock );
	socket_delete( csock );

	/* clean up after ourselves */
	unlink( "/tmp/blah" );
}


static int init_socket_suite( void )
{
	srand(0xDEADBEEF);

	/* set up the event loop */
	el = evt_new();

	return 0;
}

static int deinit_socket_suite( void )
{
	/* take down the event loop */
	evt_delete( el );
	el = NULL;

	return 0;
}

static CU_pSuite add_socket_tests( CU_pSuite pSuite )
{
	CHECK_PTR_RET( CU_add_test( pSuite, "new/delete of socket", test_socket_newdel), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "tcp socket ping/pong", test_tcp_socket), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "unix socket ping/pong", test_unix_socket), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "socket bad hostname", test_socket_bad_hostname), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "tcp failed connection", test_tcp_socket_failed_connection), NULL );
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

