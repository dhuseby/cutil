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

int8_t const * const buf = "blah";
size_t const size = 5;

void test_buffer_newdel( void )
{
	int i, size;
	buffer_t * b;

	for ( i = 0; i < 1024; i++ )
	{
		size = rand() % 1024;
		b = buffer_new( NULL, (size_t)size );

		CU_ASSERT_PTR_NOT_NULL( b );
		CU_ASSERT_PTR_NOT_NULL( b->iov_base );
		CU_ASSERT_EQUAL( b->iov_len, (size_t)size );

		buffer_delete( (void*)b );
	}
}

void test_buffer_newdel_pwned( void )
{
	int i, size;
	void * p;
	buffer_t * b;

	for ( i = 0; i < 1024; i++ )
	{
		size = rand() % 1024;
		p = CALLOC( size, sizeof(uint8_t) );
		b = buffer_new( p, (size_t)size );

		CU_ASSERT_PTR_NOT_NULL( b );
		CU_ASSERT_PTR_NOT_NULL( b->iov_base );
		CU_ASSERT_EQUAL( b->iov_base, p );
		CU_ASSERT_EQUAL( b->iov_len, (size_t)size );

		buffer_delete( (void*)b );
	}
}

void test_buffer_initdeinit( void )
{
	int i, size;
	buffer_t b;

	for ( i = 0; i < 1024; i++ )
	{
		size = rand() % 1024;
		buffer_initialize( &b, NULL, (size_t)size );

		CU_ASSERT_PTR_NOT_NULL( b.iov_base );
		CU_ASSERT_EQUAL( b.iov_len, (size_t)size );

		buffer_deinitialize( &b );
	}
}

void test_buffer_initdeinit_pwned( void )
{
	int i, size;
	void * p;
	buffer_t b;

	for ( i = 0; i < 1024; i++ )
	{
		size = rand() % 1024;
		p = CALLOC( size, sizeof(uint8_t) );
		buffer_initialize( &b, p, (size_t)size );

		CU_ASSERT_PTR_NOT_NULL( b.iov_base );
		CU_ASSERT_EQUAL( b.iov_base, p );
		CU_ASSERT_EQUAL( b.iov_len, (size_t)size );

		buffer_deinitialize( &b );
	}
}

void test_buffer_append( void )
{
	int i, size1, size2;
	buffer_t * b;

	for ( i = 0; i < 1024; i++ )
	{
		size1 = rand() % 1024;
		size2 = rand() % 1024;
		b = buffer_new( NULL, (size_t)size1 );

		CU_ASSERT_PTR_NOT_NULL( b );
		CU_ASSERT_PTR_NOT_NULL( b->iov_base );
		CU_ASSERT_EQUAL( b->iov_len, (size_t)size1 );

		buffer_append( b, NULL, size2 );
		CU_ASSERT_PTR_NOT_NULL( b->iov_base );
		CU_ASSERT_EQUAL( b->iov_len, (size_t)(size1 + size2) );

		buffer_delete( (void*)b );
	}
}

void test_buffer_append_pwned( void )
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
		CU_ASSERT_PTR_NOT_NULL( b->iov_base );
		CU_ASSERT_EQUAL( b->iov_len, (size_t)size1 );

		buffer_append( b, p, size2 );
		CU_ASSERT_PTR_NOT_NULL( b->iov_base );
		CU_ASSERT_EQUAL( b->iov_len, (size_t)(size1 + size2) );

		buffer_delete( (void*)b );
	}
}

static int init_buffer_suite( void )
{
	srand(0xDEADBEEF);
	return 0;
}

static int deinit_buffer_suite( void )
{
	return 0;
}

static CU_pSuite add_buffer_tests( CU_pSuite pSuite )
{
	CHECK_PTR_RET( CU_add_test( pSuite, "new/delete of buffer", test_buffer_newdel ), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "new/delete of buffer with prev allocation", test_buffer_newdel_pwned ), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "init/deinit of buffer", test_buffer_initdeinit ), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "init/deinit of buffer with prev allocation", test_buffer_initdeinit_pwned ), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "buffer append", test_buffer_append ), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "buffer append with prev allocation", test_buffer_append_pwned ), NULL );
	
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

