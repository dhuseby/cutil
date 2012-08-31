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
#include <cutil/list.h>

#define REPEAT (128)
#define SIZEMAX (128)
#define MULTIPLE (8)

/* the fail switches */
extern int fail_alloc;
extern int fail_list_grow;


static void test_list_newdel( void )
{
	int i;
	uint32_t size;
	list_t * list;

	for ( i = 0; i < REPEAT; i++ )
	{
		list = NULL;
		size = (rand() % SIZEMAX);
		list = list_new( size, NULL );

		CU_ASSERT_PTR_NOT_NULL( list );
		CU_ASSERT_EQUAL( list_count( list ), 0 );
		CU_ASSERT_EQUAL( list->size, size );
		CU_ASSERT_EQUAL( list->dfn, NULL );

		list_delete( list );
	}
}

static void test_list_initdeinit( void )
{
	int i;
	uint32_t size;
	list_t list;

	for ( i = 0; i < REPEAT; i++ )
	{
		MEMSET( &list, 0, sizeof(list_t) );
		size = (rand() % SIZEMAX);
		list_initialize( &list, size, NULL );

		CU_ASSERT_EQUAL( list_count( &list ), 0 );
		CU_ASSERT_EQUAL( list.size, size );
		CU_ASSERT_EQUAL( list.dfn, NULL );

		list_deinitialize( &list );
	}
}

static void test_list_static_grow( void )
{
	int i, j;
	uint32_t size, multiple;
	list_t list;

	for ( i = 0; i < 8; i++ )
	{
		MEMSET( &list, 0, sizeof(list_t) );
		size = (rand() % SIZEMAX);
		list_initialize( &list, size, NULL );

		CU_ASSERT_EQUAL( list_count( &list ), 0 );
		CU_ASSERT_EQUAL( list.size, size );
		CU_ASSERT_EQUAL( list.dfn, NULL );

		for ( j = 0; j < 8; j++ )
		{
			list_reserve( &list, (j * size) );
			CU_ASSERT_TRUE( list.size >= ((j * size > size) ? (j * size) : size) );
		}

		list_deinitialize( &list );
	}
}

static void test_list_dynamic_grow( void )
{
	int i, j;
	uint32_t size, multiple;
	list_t *list;

	for ( i = 0; i < 8; i++ )
	{
		size = (rand() % SIZEMAX);
		list = list_new( size, NULL );

		CU_ASSERT_PTR_NOT_NULL( list );
		CU_ASSERT_EQUAL( list_count( list ), 0 );
		CU_ASSERT_EQUAL( list->size, size );
		CU_ASSERT_EQUAL( list->dfn, NULL );

		for ( j = 0; j < 8; j++ )
		{
			list_reserve( list, j * size );
			CU_ASSERT_TRUE( list->size >= ((j * size > size) ? (j * size) : size) );
		}

		list_delete( list );
		list = NULL;
	}
}

static void test_list_empty_iterator( void )
{
	int i;
	uint32_t size;
	list_t list;
	list_itr_t itr;

	for ( i = 0; i < REPEAT; i++ )
	{
		MEMSET( &list, 0, sizeof(list_t) );
		size = (rand() % SIZEMAX);
		list_initialize( &list, size, NULL );

		itr = list_itr_begin( &list );
		CU_ASSERT_EQUAL( itr, list_itr_end( &list ) );
		itr = list_itr_end( &list );
		CU_ASSERT_EQUAL( itr, list_itr_begin( &list ) );

		itr = list_itr_head( &list );
		CU_ASSERT_EQUAL( itr, list_itr_tail( &list ) );
		itr = list_itr_tail( &list );
		CU_ASSERT_EQUAL( itr, list_itr_head( &list ) );

		itr = list_itr_rbegin( &list );
		CU_ASSERT_EQUAL( itr, list_itr_rend( &list ) );
		itr = list_itr_rend( &list );
		CU_ASSERT_EQUAL( itr, list_itr_rbegin( &list ) );

		itr = list_itr_begin( &list );
		CU_ASSERT_EQUAL( itr, list_itr_end( &list ) );
		itr = list_itr_next( &list, itr );
		CU_ASSERT_EQUAL( itr, list_itr_begin( &list ) );
		CU_ASSERT_EQUAL( itr, list_itr_end( &list ) );
		itr = list_itr_prev( &list, itr );
		CU_ASSERT_EQUAL( itr, list_itr_begin( &list ) );
		CU_ASSERT_EQUAL( itr, list_itr_end( &list ) );

		itr = list_itr_end( &list );
		CU_ASSERT_EQUAL( itr, list_itr_end( &list ) );
		itr = list_itr_prev( &list, itr );
		CU_ASSERT_EQUAL( itr, list_itr_begin( &list ) );
		CU_ASSERT_EQUAL( itr, list_itr_end( &list ) );
		itr = list_itr_next( &list, itr );
		CU_ASSERT_EQUAL( itr, list_itr_begin( &list ) );
		CU_ASSERT_EQUAL( itr, list_itr_end( &list ) );

		itr = list_itr_rbegin( &list );
		CU_ASSERT_EQUAL( itr, list_itr_end( &list ) );
		itr = list_itr_rprev( &list, itr );
		CU_ASSERT_EQUAL( itr, list_itr_begin( &list ) );
		CU_ASSERT_EQUAL( itr, list_itr_end( &list ) );
		itr = list_itr_rnext( &list, itr );
		CU_ASSERT_EQUAL( itr, list_itr_begin( &list ) );
		CU_ASSERT_EQUAL( itr, list_itr_end( &list ) );

		itr = list_itr_rend( &list );
		CU_ASSERT_EQUAL( itr, list_itr_end( &list ) );
		itr = list_itr_rnext( &list, itr );
		CU_ASSERT_EQUAL( itr, list_itr_begin( &list ) );
		CU_ASSERT_EQUAL( itr, list_itr_end( &list ) );
		itr = list_itr_rprev( &list, itr );
		CU_ASSERT_EQUAL( itr, list_itr_begin( &list ) );
		CU_ASSERT_EQUAL( itr, list_itr_end( &list ) );

		list_deinitialize( &list );
	}
}

static void test_list_push_head_1( void )
{
	list_t list;
	MEMSET(&list, 0, sizeof(list_t));
	list_initialize( &list, 1, NULL );
	list_push_head( &list, (void*)1 );
	CU_ASSERT_EQUAL( list_count( &list ), 1 );
	list_push_head( &list, (void*)2 );
	CU_ASSERT_EQUAL( list_count( &list ), 2 );
	list_push_head( &list, (void*)3 );
	CU_ASSERT_EQUAL( list_count( &list ), 3 );
	list_push_head( &list, (void*)4 );
	CU_ASSERT_EQUAL( list_count( &list ), 4 );
	list_push_head( &list, (void*)5 );
	CU_ASSERT_EQUAL( list_count( &list ), 5 );
	list_deinitialize( &list );
}

static void test_list_push_head( void )
{
	int i;
	int_t j;
	uint32_t size;
	uint32_t multiple;
	list_t list;

	for ( i = 0; i < REPEAT; i++ )
	{
		MEMSET( &list, 0, sizeof(list_t) );
		size = (rand() % SIZEMAX);
		multiple = (rand() % MULTIPLE);
		list_initialize( &list, size, NULL );

		for ( j = 0; j < (size * multiple); j++ )
		{
			list_push_head( &list, (void*)j );
		}

		CU_ASSERT_EQUAL( list_count( &list ), (size * multiple) );
		CU_ASSERT_EQUAL( list.dfn, NULL );

		list_deinitialize( &list );
	}
}

static void test_list_push_tail_1( void )
{
	list_t list;
	list_initialize( &list, 1, NULL );
	list_push_tail( &list, (void*)1 );
	CU_ASSERT_EQUAL( list_count( &list ), 1 );
	list_push_tail( &list, (void*)2 );
	CU_ASSERT_EQUAL( list_count( &list ), 2 );
	list_push_tail( &list, (void*)3 );
	CU_ASSERT_EQUAL( list_count( &list ), 3 );
	list_push_tail( &list, (void*)4 );
	CU_ASSERT_EQUAL( list_count( &list ), 4 );
	list_push_tail( &list, (void*)5 );
	CU_ASSERT_EQUAL( list_count( &list ), 5 );
	list_deinitialize( &list );
}

static void test_list_push_tail_small( void )
{
	int_t j;
	list_t list;
	list_itr_t end;
	list_itr_t itr;

	MEMSET( &list, 0, sizeof(list_t) );
	list_initialize( &list, 4, NULL );

	/* push integers to the tail */
	for ( j = 0; j < 8; j++ )
	{
		list_push_tail( &list, (void*)j );
	}

	/* walk the list to make sure the values are what we expect */
	end = list_itr_end( &list );
	j = 0;
	for ( itr = list_itr_begin( &list ); itr != end; itr = list_itr_next( &list, itr ) )
	{
		if ( (int_t)list_itr_get( &list, itr ) != j )
		{
			NOTICE("[%d] %d != %d\n", (int_t)itr, (int_t)list_itr_get( &list, itr ), j );
		}
		CU_ASSERT_EQUAL( (int_t)list_itr_get( &list, itr ), j );
		j++;
	}

	CU_ASSERT_EQUAL( list_count( &list ), 8 );
	CU_ASSERT_EQUAL( list.dfn, NULL );

	list_deinitialize( &list );
}

static void test_list_push_tail( void )
{
	int i;
	int_t j;
	uint32_t size;
	uint32_t multiple;
	list_t list;
	list_itr_t end;
	list_itr_t itr;

	for ( i = 0; i < REPEAT; i++ )
	{
		MEMSET( &list, 0, sizeof(list_t) );
		size = (rand() % SIZEMAX);
		multiple = (rand() % MULTIPLE);
		list_initialize( &list, size, NULL );

		/* push integers to the tail */
		for ( j = 0; j < (size * multiple); j++ )
		{
			list_push_tail( &list, (void*)j );
		}

		/* walk the list to make sure the values are what we expect */
		end = list_itr_end( &list );
		j = 0;
		for ( itr = list_itr_begin( &list ); itr != end; itr = list_itr_next( &list, itr ) )
		{
			if ( (int_t)list_itr_get( &list, itr ) != j )
			{
				NOTICE("[%d] %d != %d\n", (int_t)itr, (int_t)list_itr_get( &list, itr ), j );
			}
			CU_ASSERT_EQUAL( (int_t)list_itr_get( &list, itr ), j );
			j++;
		}

		CU_ASSERT_EQUAL( list_count( &list ), (size * multiple) );
		CU_ASSERT_EQUAL( list.dfn, NULL );

		list_deinitialize( &list );
	}
}

static void test_list_push_dynamic( void )
{
	int i, j;
	void * p;
	void ** t;
	uint32_t size;
	uint32_t multiple;
	list_t list;
	list_itr_t itr;

	for ( i = 0; i < REPEAT; i++ )
	{
		MEMSET( &list, 0, sizeof(list_t) );
		size = (rand() % SIZEMAX);
		multiple = (rand() % MULTIPLE);
		list_initialize( &list, size, FREE );

		for ( j = 0; j < (size * multiple); j++ )
		{
			p = CALLOC( (rand() % SIZEMAX) + 1, sizeof(uint8_t) );
			list_push_tail( &list, p );
			CU_ASSERT_EQUAL( list_count( &list ), (j + 1) );
		}

		CU_ASSERT_EQUAL( list_count( &list ), (size * multiple) );
		CU_ASSERT_EQUAL( list.dfn, FREE );

		list_deinitialize( &list );
	}
}

static void test_list_push_zero_initial_size( void )
{
	list_t list;
	list_initialize( &list, 0, NULL );
	CU_ASSERT_EQUAL( list_count( &list ), 0 );
	CU_ASSERT_EQUAL( list.dfn, NULL );
	list_push_tail( &list, (void*)0 );
	CU_ASSERT_EQUAL( list_count( &list ), 1 );
	list_deinitialize( &list );
}

static void test_list_pop_head_static( void )
{
	int_t i, j;
	uint32_t size;
	uint32_t multiple;
	list_t list;
	list_itr_t itr;

	size = (rand() % SIZEMAX);
	list_initialize( &list, size, NULL );
	multiple = (rand() % MULTIPLE);
	for( i = 0; i < (size * multiple); i++ )
	{
		list_push_tail( &list, (void*)i );
		CU_ASSERT_EQUAL( list_count( &list ), (i + 1) );
	}

	CU_ASSERT_EQUAL( list_count( &list ), (size * multiple) );

	/* walk the list to make sure the values are what we expect */
	itr = list_itr_begin( &list );
	i = 0;
	for ( ; itr != list_itr_end( &list ); itr = list_itr_next( &list, itr ) )
	{
		CU_ASSERT_EQUAL( (int_t)list_itr_get( &list, itr ), i );
		i++;
	}

	itr = list_itr_rbegin( &list );
	i = ((size * multiple) - 1);
	for ( ; itr != list_itr_rend( &list ); itr = list_itr_rnext( &list, itr ) )
	{
		CU_ASSERT_EQUAL( (int_t)list_itr_get( &list, itr ), i );
		i--;
	}

	for ( i = 0; i < (size * multiple); i++ )
	{
		j = (int_t)list_get_head( &list );
		list_pop_head( &list );
		CU_ASSERT_EQUAL( j, i );
	}

	CU_ASSERT_EQUAL( list_count( &list ), 0 );
	list_deinitialize( &list );
}

static void test_list_pop_tail_static( void )
{
	int_t i, j;
	uint32_t size;
	uint32_t multiple;
	list_t list;
	list_itr_t itr;

	size = (rand() % SIZEMAX);
	list_initialize( &list, size, NULL );
	multiple = (rand() % MULTIPLE);
	for( i = 0; i < (size * multiple); i++ )
	{
		list_push_head( &list, (void*)i );
		CU_ASSERT_EQUAL( list_count( &list ), (i + 1) );
		CU_ASSERT_EQUAL( (int_t)list_get_head(&list), i );
		if ( i > 0 )
		{
			/* make sure the previous item in the list is what we expect */
			itr = list_itr_begin( &list );
			itr = list_itr_next( &list, itr );
			/*CU_ASSERT_EQUAL( (int)list_itr_get(&list, itr), (i - 1) );*/
			if ( (i - 1) != (int_t)list_itr_get(&list, itr) )
			{
				itr = list_itr_begin( &list );
				itr = list_itr_next( &list, itr );
				j = (int_t)list_itr_get( &list, itr );
			}
		}
	}
	
	CU_ASSERT_EQUAL( list_count( &list ), (size * multiple) );
	/* walk the list to make sure the values are what we expect */
	itr = list_itr_begin( &list );
	i = ((size * multiple) - 1);
	for ( ; itr != list_itr_end( &list ); itr = list_itr_next( &list, itr ) )
	{
		CU_ASSERT_EQUAL( (int_t)list_itr_get( &list, itr ), i );
		i--;
	}

	itr = list_itr_rbegin( &list );
	i = 0;
	for ( ; itr != list_itr_rend( &list ); itr = list_itr_rnext( &list, itr ) )
	{
		CU_ASSERT_EQUAL( (int_t)list_itr_get( &list, itr ), i );
		i++;
	}

	for ( i = 0; i < (size * multiple); i++ )
	{
		j = (int_t)list_get_tail( &list );
		list_pop_tail( &list );
		CU_ASSERT_EQUAL( j, i );
	}

	CU_ASSERT_EQUAL( list_count( &list ), 0 );
	list_deinitialize( &list );
}

static void test_list_clear( void )
{
	int_t i;
	uint32_t size;
	uint32_t multiple;
	list_t list;

	size = (rand() % SIZEMAX);
	list_initialize( &list, size, NULL );
	multiple = (rand() % MULTIPLE);
	for( i = 0; i < (size * multiple); i++ )
	{
		list_push_head( &list, (void*)i );
		CU_ASSERT_EQUAL( list_count( &list ), (i + 1) );
	}

	CU_ASSERT_EQUAL( list_count( &list ), (size * multiple) );

	list_clear( &list );

	CU_ASSERT_EQUAL( list_count( &list ), 0 );
	list_deinitialize( &list );
}

static void test_list_clear_empty( void )
{
	int i;
	uint32_t size;
	list_t list;

	for ( i = 0; i < REPEAT; i++ )
	{
		MEMSET( &list, 0, sizeof(list_t) );
		size = (rand() % SIZEMAX);
		list_initialize( &list, size, NULL );

		CU_ASSERT_EQUAL( list_count( &list ), 0 );
		CU_ASSERT_EQUAL( list.size, size );
		CU_ASSERT_EQUAL( list.dfn, NULL );

		list_clear( &list );

		list_deinitialize( &list );
	}
}

static void test_list_new_fail( void )
{
	int i;
	uint32_t size;
	list_t * list = NULL;

	/* turn on the flag that forces grows to fail */
	fail_list_grow = TRUE;

	size = (rand() % SIZEMAX);
	list = list_new( size, NULL );

	CU_ASSERT_PTR_NULL( list );
	CU_ASSERT_EQUAL( list_count( list ), 0 );
	
	fail_list_grow = FALSE;
}

static void test_list_init_fail( void )
{
	int i;
	uint32_t size;
	list_t list;

	/* turn on the flag that forces grows to fail */
	fail_list_grow = TRUE;

	MEMSET( &list, 0, sizeof(list_t) );
	size = (rand() % SIZEMAX);
	list_initialize( &list, size, NULL );

	CU_ASSERT_EQUAL( list_count( &list ), 0 );

	fail_list_grow = FALSE;
}

static void test_list_push_fail( void )
{
	int ret;
	list_t list;
	MEMSET(&list, 0, sizeof(list_t));
	list_initialize( &list, 0, NULL );

	/* turn on the flag that forces grows to fail */
	fail_list_grow = TRUE;

	ret = list_push_head( &list, (void*)1 )
	CU_ASSERT_EQUAL( ret, FALSE );
	CU_ASSERT_EQUAL( list_count( &list ), 0 );
	
	list_deinitialize( &list );
	
	fail_list_grow = FALSE;
}

static void test_list_push_middle( void )
{
	int_t i, j;
	uint32_t size;
	uint32_t multiple;
	list_t list;
	list_itr_t itr;

	for ( i = 0; i < REPEAT; i++ )
	{
		MEMSET( &list, 0, sizeof(list_t) );
		size = (rand() % SIZEMAX);
		multiple = (rand() % MULTIPLE);
		list_initialize( &list, size, NULL );

		for ( j = 0; j < (size * multiple); j++ )
		{
			list_push_head( &list, (void*)j );
		}

		itr = list_itr_begin( &list );
		for ( j = 0; j < (size * multiple); j++ )
		{
			if ( j & 0x1 )
			{
				list_push( &list, (void*)j, itr );
			}
			itr = list_itr_next( &list, itr );
		}

		CU_ASSERT_EQUAL( list_count( &list ), (((size*multiple) & ~0x1) / 2) + (size * multiple) );
		CU_ASSERT_EQUAL( list.dfn, NULL );

		list_deinitialize( &list );
	}
}

static void test_list_pop_middle( void )
{
	int_t i, j;
	uint32_t size;
	uint32_t multiple;
	list_t list;
	list_itr_t itr;

	for ( i = 0; i < REPEAT; i++ )
	{
		MEMSET( &list, 0, sizeof(list_t) );
		size = (rand() % SIZEMAX);
		multiple = (rand() % MULTIPLE);
		list_initialize( &list, size, NULL );

		for ( j = 0; j < (size * multiple); j++ )
		{
			list_push_head( &list, (void*)j );
		}

		itr = list_itr_begin( &list );
		for ( j = 0; j < (size * multiple); j++ )
		{
			if ( j & 0x1 )
			{
				list_push( &list, (void*)j, itr );
			}
			itr = list_itr_next( &list, itr );
		}

		itr = list_itr_begin( &list );
		for ( j = 0; j < (size * multiple); j++ )
		{
			if ( j & 0x1 )
			{
				itr = list_pop( &list, itr );
			}
			itr = list_itr_next( &list, itr );
		}

		CU_ASSERT_EQUAL( list_count( &list ), (size * multiple) );
		CU_ASSERT_EQUAL( list.dfn, NULL );

		list_deinitialize( &list );
	}
}

static void test_list_get_middle( void )
{
	int_t i, j, k;
	uint32_t size;
	uint32_t multiple;
	list_t list;
	list_itr_t itr;

	for ( i = 0; i < REPEAT; i++ )
	{
		MEMSET( &list, 0, sizeof(list_t) );
		size = (rand() % SIZEMAX);
		multiple = (rand() % MULTIPLE);
		list_initialize( &list, size, NULL );

		for ( j = 0; j < (size * multiple); j++ )
		{
			list_push_tail( &list, (void*)j );
		}

		itr = list_itr_begin( &list );
		for ( j = 0; j < (size * multiple); j++ )
		{
			if ( j & 0x1 )
			{
				k = (int_t)list_itr_get( &list, itr );
				CU_ASSERT_EQUAL( j, k );
			}
			itr = list_itr_next( &list, itr );
		}

		CU_ASSERT_EQUAL( list_count( &list ), (size * multiple) );
		CU_ASSERT_EQUAL( list.dfn, NULL );

		list_deinitialize( &list );
	}
}

/*TODO:
 *	- threading tests
 */

static int init_list_suite( void )
{
	srand(0xDEADBEEF);
	return 0;
}

static int deinit_list_suite( void )
{
	return 0;
}

static CU_pSuite add_list_tests( CU_pSuite pSuite )
{
	CHECK_PTR_RET( CU_add_test( pSuite, "new/delete of list", test_list_newdel), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "init/deinit of list", test_list_initdeinit), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "grow of static list", test_list_static_grow), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "grow of dynamic list", test_list_dynamic_grow), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "empty list itr tests", test_list_empty_iterator), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "push one head tests", test_list_push_head_1), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "push head tests", test_list_push_head), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "push one tail tests", test_list_push_tail_1), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "push tail small", test_list_push_tail_small), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "push tail tests", test_list_push_tail), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "push dynamic memory", test_list_push_dynamic), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "push to zero initial size", test_list_push_zero_initial_size), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "pop head static", test_list_pop_head_static), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "pop tail static", test_list_pop_tail_static), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "list clear", test_list_clear), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "list clear empty", test_list_clear_empty), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "list new fail", test_list_new_fail), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "list init fail", test_list_init_fail), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "list push fail", test_list_push_fail), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "list push middle", test_list_push_middle), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "list pop middle", test_list_pop_middle), NULL );
	CHECK_PTR_RET( CU_add_test( pSuite, "list get middle", test_list_get_middle), NULL );
	
	return pSuite;
}

CU_pSuite add_list_test_suite()
{
	CU_pSuite pSuite = NULL;

	/* add the suite to the registry */
	pSuite = CU_add_suite("Array Tests", init_list_suite, deinit_list_suite);
	CHECK_PTR_RET( pSuite, NULL );

	/* add in list specific tests */
	CHECK_PTR_RET( add_list_tests( pSuite ), NULL );

	return pSuite;
}

