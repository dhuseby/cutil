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

#include "test_macros.h"

#define REPEAT (128)
#define SIZEMAX (128)
#define MULTIPLE (8)

/* the fail switches */
extern int fail_alloc;
extern int fail_list_grow;
extern int fail_list_init;
extern int fail_list_deinit;

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
		CU_ASSERT_TRUE( list_initialize( &list, size, NULL ) );

		CU_ASSERT_EQUAL( list_count( &list ), 0 );
		CU_ASSERT_EQUAL( list.size, size );
		CU_ASSERT_EQUAL( list.dfn, NULL );

		CU_ASSERT_TRUE( list_deinitialize( &list ) );
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
		CU_ASSERT_TRUE( list_initialize( &list, size, NULL ) );

		CU_ASSERT_EQUAL( list_count( &list ), 0 );
		CU_ASSERT_EQUAL( list.size, size );
		CU_ASSERT_EQUAL( list.dfn, NULL );

		for ( j = 0; j < 8; j++ )
		{
			CU_ASSERT_TRUE( list_reserve( &list, (j * size) ) );
			CU_ASSERT_TRUE( list.size >= ((j * size > size) ? (j * size) : size) );
		}

		CU_ASSERT_TRUE( list_deinitialize( &list ) );
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
			CU_ASSERT_TRUE( list_reserve( list, j * size ) );
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
		CU_ASSERT_TRUE( list_initialize( &list, size, NULL ) );

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

		CU_ASSERT_TRUE( list_deinitialize( &list ) );
	}
}

static void test_list_push_head_1( void )
{
	list_t list;
	MEMSET(&list, 0, sizeof(list_t));
	CU_ASSERT_TRUE( list_initialize( &list, 1, NULL ) );
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
	CU_ASSERT_TRUE( list_deinitialize( &list ) );
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
	list_itr_t itr, end;

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
	end = list_itr_end( &list );
	i = 0;
	for ( itr = list_itr_begin( &list ); itr != end; itr = list_itr_next( &list, itr ) )
	{
		CU_ASSERT_EQUAL( (int_t)list_itr_get( &list, itr ), i );
		i++;
	}

	/* walk the list backwards to make sure the values are we expect */
	end = list_itr_rend( &list );
	i = ((size * multiple) - 1);
	for ( itr = list_itr_rbegin( &list ); itr != end; itr = list_itr_rnext( &list, itr ) )
	{
		CU_ASSERT_EQUAL( (int_t)list_itr_get( &list, itr ), i );
		i--;
	}

	/* now pop head enough times to empty the list */
	for ( i = 0; i < (size * multiple); i++ )
	{
		j = (int_t)list_get_head( &list );
		list_pop_head( &list );
		CU_ASSERT_EQUAL( j, i );
	}

	if ( list_count( &list ) != 0 )
	{
		NOTICE( "%d\n", list_count( &list ) );
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

static void test_list_new_grow_fail( void )
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

static void test_list_init_grow_fail( void )
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

static void test_list_new_alloc_fail( void )
{
	int i;
	uint32_t size;
	list_t * list = NULL;

	/* turn on the flag that forces heap allocations to fail */
	fail_alloc = TRUE;

	size = (rand() % SIZEMAX);
	list = list_new( size, NULL );

	CU_ASSERT_PTR_NULL( list );
	CU_ASSERT_EQUAL( list_count( list ), 0 );
	
	fail_alloc = FALSE;
}

static void test_list_init_alloc_fail( void )
{
	int i;
	uint32_t size;
	list_t list;

	/* turn on the flag that forces heap allocations to fail */
	fail_alloc = TRUE;

	MEMSET( &list, 0, sizeof(list_t) );
	size = (rand() % SIZEMAX);
	list_initialize( &list, size, NULL );

	CU_ASSERT_EQUAL( list_count( &list ), 0 );

	fail_alloc = FALSE;
}

static void test_list_push_fail( void )
{
	int ret;
	list_t list;
	MEMSET(&list, 0, sizeof(list_t));
	list_initialize( &list, 0, NULL );

	/* turn on the flag that forces grows to fail */
	fail_list_grow = TRUE;

	ret = list_push_head( &list, (void*)1 );
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

		if ( list_count( &list ) != (size * multiple) )
		{
			NOTICE( "%d != %d\n", list_count( &list ), (size * multiple) );
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

static void test_list_delete_null( void )
{
	list_delete( NULL );
}

static void test_list_init_null( void )
{
	CU_ASSERT_FALSE( list_initialize( NULL, rand(), NULL ) );
}

static void test_list_deinit_null( void )
{
	CU_ASSERT_FALSE( list_deinitialize( NULL ) );
}

static void test_list_reserve_null( void )
{
	CU_ASSERT_FALSE( list_reserve( NULL, rand() ) );
}

static void test_list_clear_null( void )
{
	CU_ASSERT_FALSE( list_clear( NULL ) );
}

static void test_list_clear_dep_fails( void )
{
	list_t list;
	MEMSET( &list, 0, sizeof(list_t) );
	
	fail_list_init = TRUE;
	CU_ASSERT_FALSE( list_clear( &list ) );
	fail_list_init = FALSE;

	fail_list_deinit = TRUE;
	CU_ASSERT_FALSE( list_clear( &list ) );
	fail_list_deinit = FALSE;
}

static void test_list_begin_null( void )
{
	CU_ASSERT_EQUAL( list_itr_begin( NULL ), -1 );
}

static void test_list_end_null( void )
{
	CU_ASSERT_EQUAL( list_itr_end( NULL ), -1 );
}

static void test_list_tail_null( void )
{
	CU_ASSERT_EQUAL( list_itr_tail( NULL ), -1 );
}

static void test_list_next_null( void )
{
	list_t list;
	CU_ASSERT_EQUAL( list_itr_next( NULL, 0 ), -1 );
	CU_ASSERT_EQUAL( list_itr_next( &list, -1 ), -1 );
}

static void test_list_rnext_null( void )
{
	list_t list;
	CU_ASSERT_EQUAL( list_itr_rnext( NULL, 0 ), -1 );
	CU_ASSERT_EQUAL( list_itr_rnext( &list, -1 ), -1 );
}

static void test_list_push_null( void )
{
	CU_ASSERT_FALSE( list_push( NULL, NULL, 0 ) );
}

static void test_list_pop_prereqs( void )
{
	list_t list;
	MEMSET( &list, 0, sizeof(list_t) );

	/* pass in NULL for the list */
	CU_ASSERT_EQUAL( list_pop( NULL, 0 ), -1 );

	/* initialize the list and ask to pop 0. it's an empty
	 * list so the size check will fail */
	CU_ASSERT_TRUE( list_initialize( &list, 0, NULL ) );
	CU_ASSERT_EQUAL( list_pop( &list, 0 ), -1 );

	/* push an item on the list so it isn't empty */
	CU_ASSERT_TRUE( list_push( &list, (void*)1, -1 ) );
	CU_ASSERT_EQUAL( list_count( &list ), 1 );

	/* pop at illegal indexes */
	CU_ASSERT_EQUAL( list_pop( &list, -2 ), -1 );
	CU_ASSERT_EQUAL( list_pop( &list, 5 ), -1 );
	CU_ASSERT_EQUAL( list_pop( &list, -1 ), -1 );

	/* make the size at least 10 */
	CU_ASSERT_TRUE( list_reserve( &list, 10 ) );

	/* try to pop and item from the free list */
	CU_ASSERT_EQUAL( list_pop( &list, 7 ), -1 );
}

static void test_list_get_prereqs( void )
{
	list_t list;
	MEMSET( &list, 0, sizeof(list_t) );

	/* pass in NULL for the list */
	CU_ASSERT_PTR_NULL( list_itr_get( NULL, -1 ) );

	/* pass int -1 */
	CU_ASSERT_PTR_NULL( list_itr_get( &list, -1 ) );

	/* init the list with zero size */
	CU_ASSERT_TRUE( list_initialize( &list, 0, NULL ) );

	/* pass in not -1, this will trigger the size check */
	CU_ASSERT_PTR_NULL( list_itr_get( &list, 0 ) );

	/* make the size at least 4 */
	CU_ASSERT_TRUE( list_reserve( &list, 4 ) );

	/* pass in bogus iterators */
	CU_ASSERT_PTR_NULL( list_itr_get( &list, -2 ) );
	CU_ASSERT_PTR_NULL( list_itr_get( &list, 5 ) );

	/* try to get an item from the free list */
	CU_ASSERT_PTR_NULL( list_itr_get( &list, 3 ) );
}


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
	ADD_TEST("new/delete of list",			test_list_newdel);
	ADD_TEST("init/deinit of list",			test_list_initdeinit);
	ADD_TEST("grow of static list",			test_list_static_grow);
	ADD_TEST("grow of dynamic list",		test_list_dynamic_grow);
	ADD_TEST("empty list itr tests",		test_list_empty_iterator);
	ADD_TEST("push one head tests",			test_list_push_head_1);
	ADD_TEST("push head tests",				test_list_push_head);
	ADD_TEST("push one tail tests",			test_list_push_tail_1);
	ADD_TEST("push tail small",				test_list_push_tail_small);
	ADD_TEST("push tail tests",				test_list_push_tail);
	ADD_TEST("push dynamic memory",			test_list_push_dynamic);
	ADD_TEST("push to zero initial size",	test_list_push_zero_initial_size);
	ADD_TEST("pop head static",				test_list_pop_head_static);
	ADD_TEST("pop tail static",				test_list_pop_tail_static);
	ADD_TEST("list clear",					test_list_clear);
	ADD_TEST("list clear empty",			test_list_clear_empty);
	ADD_TEST("list new grow fail",			test_list_new_grow_fail);
	ADD_TEST("list init grow fail",			test_list_init_grow_fail);
	ADD_TEST("list new alloc fail",			test_list_new_alloc_fail);
	ADD_TEST("list init alloc fail",		test_list_init_alloc_fail);
	ADD_TEST("list push fail",				test_list_push_fail);
	ADD_TEST("list push middle",			test_list_push_middle);
	ADD_TEST("list pop middle",				test_list_pop_middle);
	ADD_TEST("list get middle",				test_list_get_middle);
	ADD_TEST("list delete null",			test_list_delete_null);
	ADD_TEST("list init null",				test_list_init_null);
	ADD_TEST("list deinit null",			test_list_deinit_null);
	ADD_TEST("list reserve null",			test_list_reserve_null);
	ADD_TEST("list clear null",				test_list_clear_null);
	ADD_TEST("list clear init/deinit fail", test_list_clear_dep_fails);
	ADD_TEST("list begin null",				test_list_begin_null);
	ADD_TEST("list end null",				test_list_end_null);
	ADD_TEST("list tail null",				test_list_tail_null);
	ADD_TEST("list next null",				test_list_next_null);
	ADD_TEST("list rnext null",				test_list_rnext_null);
	ADD_TEST("list push null",				test_list_push_null);
	ADD_TEST("list pop pre-reqs",			test_list_pop_prereqs);
	ADD_TEST("list get pre-reqs",			test_list_get_prereqs);
	ADD_TEST("list private functions",		test_list_private_functions);

	return pSuite;
}

CU_pSuite add_list_test_suite()
{
	CU_pSuite pSuite = NULL;

	/* add the suite to the registry */
	pSuite = CU_add_suite("List Tests", init_list_suite, deinit_list_suite);
	CHECK_PTR_RET( pSuite, NULL );

	/* add in list specific tests */
	CHECK_PTR_RET( add_list_tests( pSuite ), NULL );

	return pSuite;
}

