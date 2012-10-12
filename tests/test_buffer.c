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

#include <stdint.h>

#include <CUnit/Basic.h>

#include <cutil/debug.h>
#include <cutil/macros.h>
#include <cutil/buffer.h>

#include "test_macros.h"
#include "test_flags.h"

extern void test_buffer_private_functions( void );

int8_t const * const buf = "blah";
size_t const size = 5;

static void test_buffer_newdel( void )
{
	int i, size;
	buffer_t * b;

	for ( i = 0; i < 1024; i++ )
	{
		size = rand() % 1024;
		b = buffer_new( NULL, (size_t)size );

		CU_ASSERT_PTR_NOT_NULL( b );
		if ( size > 0 )
		{
			CU_ASSERT_PTR_NOT_NULL( b->iov_base );
		}
		else
		{
			CU_ASSERT_PTR_NULL( b->iov_base );
		}
		CU_ASSERT_EQUAL( b->iov_len, (size_t)size );

		buffer_delete( (void*)b );
	}
}

static void test_buffer_newdel_pwned( void )
{
	int i, size;
	void * p;
	buffer_t * b;

	for ( i = 0; i < 1024; i++ )
	{
		size = (rand() % 1024) + 1;
		p = CALLOC( size, sizeof(uint8_t) );
		b = buffer_new( p, (size_t)size );

		CU_ASSERT_PTR_NOT_NULL( b );
		CU_ASSERT_PTR_NOT_NULL( b->iov_base );
		CU_ASSERT_EQUAL( b->iov_base, p );
		CU_ASSERT_EQUAL( b->iov_len, (size_t)size );

		buffer_delete( (void*)b );
	}
}

static void test_buffer_initdeinit( void )
{
	int i, size;
	buffer_t b;

	for ( i = 0; i < 1024; i++ )
	{
		size = rand() % 1024;
		CU_ASSERT_TRUE( buffer_initialize( &b, NULL, (size_t)size ) );

		if ( size > 0 )
		{
			CU_ASSERT_PTR_NOT_NULL( b.iov_base );
		}
		else
		{
			CU_ASSERT_PTR_NULL( b.iov_base );
		}
		CU_ASSERT_EQUAL( b.iov_len, (size_t)size );

		CU_ASSERT_TRUE( buffer_deinitialize( &b ) );
	}
}

static void test_buffer_initdeinit_pwned( void )
{
	int i, size;
	void * p;
	buffer_t b;

	for ( i = 0; i < 1024; i++ )
	{
		size = (rand() % 1024) + 1;
		p = CALLOC( size, sizeof(uint8_t) );
		CU_ASSERT_TRUE( buffer_initialize( &b, p, (size_t)size ) );

		CU_ASSERT_PTR_NOT_NULL( b.iov_base );
		CU_ASSERT_EQUAL( b.iov_base, p );
		CU_ASSERT_EQUAL( b.iov_len, (size_t)size );

		CU_ASSERT_TRUE( buffer_deinitialize( &b ) );
	}
}

static void test_buffer_append( void )
{
	int i, size1, size2;
	buffer_t * b;

	for ( i = 0; i < 1024; i++ )
	{
		size1 = rand() % 1024;
		size2 = rand() % 1024;
		b = buffer_new( NULL, (size_t)size1 );

		CU_ASSERT_PTR_NOT_NULL( b );
		if ( size1 > 0 )
		{
			CU_ASSERT_PTR_NOT_NULL( b->iov_base );
		}
		else
		{
			CU_ASSERT_PTR_NULL( b->iov_base );
		}
		CU_ASSERT_EQUAL( b->iov_len, (size_t)size1 );

		CU_ASSERT_PTR_NOT_NULL( buffer_append( b, NULL, size2 ) );
		CU_ASSERT_PTR_NOT_NULL( b->iov_base );
		CU_ASSERT_EQUAL( b->iov_len, (size_t)(size1 + size2) );

		buffer_delete( (void*)b );
	}
}

static void test_buffer_append_pwned( void )
{
	int i, size1, size2;
	void * p;
	buffer_t * b;

	for ( i = 0; i < 1024; i++ )
	{
		size1 = 1 + rand() % 1023;
		size2 = 1 + rand() % 1023;
		p = CALLOC( size2, sizeof(uint8_t) );
		b = buffer_new( NULL, (size_t)size1 );

		CU_ASSERT_PTR_NOT_NULL( b );
		if ( size1 > 0 )
		{
			CU_ASSERT_PTR_NOT_NULL( b->iov_base );
		}
		else
		{
			CU_ASSERT_PTR_NULL( b->iov_base );
		}
		CU_ASSERT_EQUAL( b->iov_len, (size_t)size1 );

		CU_ASSERT_PTR_NOT_NULL( buffer_append( b, p, size2 ) );
		FREE( p );
		CU_ASSERT_PTR_NOT_NULL( b->iov_base );
		CU_ASSERT_EQUAL( b->iov_len, (size_t)(size1 + size2) );

		buffer_delete( (void*)b );
	}
}

static void test_buffer_delete_null( void )
{
	buffer_delete( NULL );
}

static void test_buffer_new_fail_alloc( void )
{
	fail_alloc = TRUE;
	CU_ASSERT_PTR_NULL( buffer_new( NULL, 10 ) );
	fail_alloc = FALSE;
}

static void test_buffer_new_fail_init( void )
{
	fail_buffer_init = TRUE;
	CU_ASSERT_PTR_NULL( buffer_new( NULL, 10 ) );
	fail_buffer_init = FALSE;
}

static void test_buffer_init_null( void )
{
	CU_ASSERT_FALSE( buffer_initialize( NULL, NULL, 0 ) );
}

static void test_buffer_init_fail_alloc( void )
{
	buffer_t b;
	fail_buffer_init_alloc = TRUE;
	CU_ASSERT_FALSE( buffer_initialize( &b, NULL, 10 ) );
	fail_buffer_init_alloc = FALSE;
}

static void test_buffer_deinit_null( void )
{
	CU_ASSERT_FALSE( buffer_deinitialize( NULL ) );
}

static void test_buffer_deinit_fail( void )
{
	buffer_t b;
	fail_buffer_deinit = TRUE;
	CU_ASSERT_FALSE( buffer_deinitialize( &b ) );
	fail_buffer_deinit = FALSE;
}

static void test_buffer_append_prereqs( void )
{
	buffer_t b;
	uint8_t c;
	MEMSET( &b, 0, sizeof(buffer_t) );

	CU_ASSERT_PTR_NULL( buffer_append( NULL, NULL, 0 ) );
	CU_ASSERT_PTR_NULL( buffer_append( &b, NULL, 0 ) );
	CU_ASSERT_PTR_NULL( buffer_append( &b, (void*)&c, 0 ) );

	fail_alloc = TRUE;
	CU_ASSERT_PTR_NULL( buffer_append( &b, (void*)&c, 1) );
	fail_alloc = FALSE;
	CU_ASSERT_PTR_NULL( ((struct iovec)b).iov_base );
	CU_ASSERT_EQUAL( ((struct iovec)b).iov_len, 0 );

	/* this is the equivilent of an init with 1 byte */
	CU_ASSERT_PTR_NOT_NULL( buffer_append( &b, (void*)&c, 1 ) );
	CU_ASSERT_PTR_NOT_NULL( ((struct iovec)b).iov_base );
	CU_ASSERT_EQUAL( ((struct iovec)b).iov_len, 1 );
	CU_ASSERT_TRUE( buffer_deinitialize( &b ) );
}

static int init_buffer_suite( void )
{
	srand(0xDEADBEEF);
	reset_test_flags();
	return 0;
}

static int deinit_buffer_suite( void )
{
	reset_test_flags();
	return 0;
}

static CU_pSuite add_buffer_tests( CU_pSuite pSuite )
{
	ADD_TEST( "new/delete of buffer", test_buffer_newdel );
	ADD_TEST( "new/delete of buffer with prev allocation", test_buffer_newdel_pwned );
	ADD_TEST( "init/deinit of buffer", test_buffer_initdeinit );
	ADD_TEST( "init/deinit of buffer with prev allocation", test_buffer_initdeinit_pwned );
	ADD_TEST( "buffer append", test_buffer_append );
	ADD_TEST( "buffer append with prev allocation", test_buffer_append_pwned );
	ADD_TEST( "buffer delete null", test_buffer_delete_null );
	ADD_TEST( "buffer new failed alloc", test_buffer_new_fail_alloc );
	ADD_TEST( "buffer new failed init", test_buffer_new_fail_init );
	ADD_TEST( "buffer init null", test_buffer_init_null );
	ADD_TEST( "buffer init fail alloc", test_buffer_init_fail_alloc );
	ADD_TEST( "buffer deinit null", test_buffer_deinit_null );
	ADD_TEST( "buffer deinit fail", test_buffer_deinit_fail );
	ADD_TEST( "buffer append pre-reqs", test_buffer_append_prereqs );
	ADD_TEST( "buffer private functions", test_buffer_private_functions );
	
	return pSuite;
}

CU_pSuite add_buffer_test_suite()
{
	CU_pSuite pSuite = NULL;

	/* add the suite to the registry */
	pSuite = CU_add_suite("Buffer Tests", init_buffer_suite, deinit_buffer_suite);
	CHECK_PTR_RET( pSuite, NULL );

	/* add in buffer specific tests */
	CHECK_PTR_RET( add_buffer_tests( pSuite ), NULL );

	return pSuite;
}

