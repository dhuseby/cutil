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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "debug.h"
#include "macros.h"
#include "list.h"

#if defined(UNIT_TESTING)
#include "test_flags.h"
#endif

/* node used in queue structure */
struct list_item_s
{
	int_t	next;	/* next node in the list */
	int_t	prev;	/* prev node in the list */
	int_t	used;	/* bool to mark used nodes */
	void *	data;	/* pointer to the data */
};

#define ITEM_AT( items, index ) (&(items[index]))

/* index constants */
list_itr_t const list_itr_end_t = -1;

/* forward declaration of private functions */
static list_itr_t remove_item( list_item_t * const items, list_itr_t const itr );
static list_itr_t insert_item( list_item_t * const items, 
							   list_itr_t const itr, 
							   list_itr_t const item );
static int list_grow( list_t * const list, uint_t amount );


/********** PUBLIC **********/

list_t * list_new( uint_t const initial_capacity, list_delete_fn dfn )
{
	list_t * list = CALLOC( 1, sizeof(list_t) );
	CHECK_PTR_RET( list, NULL );
	if ( !list_initialize( list, initial_capacity, dfn ) )
	{
		FREE( list );
		list = NULL;
	}
	return list;
}

void list_delete( void * l )
{
	CHECK_PTR( l );
	list_deinitialize( (list_t*)l );
	FREE( l );
}

int list_initialize( list_t * const list, uint_t const initial_capacity, list_delete_fn dfn )
{
	uint_t i;
	CHECK_PTR_RET( list, FALSE );
#if defined(UNIT_TESTING)
	CHECK_RET( !fake_list_init, fake_list_init_ret );
#endif

	/* intialize the members */
	list->dfn = (dfn ? dfn : NULL );
	list->size = 0;
	list->count = 0;
	list->used_head = list_itr_end_t;
	list->free_head = list_itr_end_t;
	list->items = NULL;

	/* grow the list array if needed */
	CHECK_RET( list_grow( list, initial_capacity ), FALSE );

	return TRUE;
}

int list_deinitialize( list_t * const list )
{
	list_itr_t itr, end;
#if defined(UNIT_TESTING)
	CHECK_RET( !fake_list_deinit, fake_list_deinit_ret );
#endif
	CHECK_PTR_RET( list, FALSE );

	/* empty lists need no work */
	if ( list->size == 0 )
		return TRUE;

	if ( list->dfn )
	{
		end = list_itr_end( list );
		for( itr = list_itr_begin( list ); itr != end; itr = list_itr_next( list, itr ) )
		{
			/* call the delete function on the data at the node */
			(*(list->dfn))( ITEM_AT( list->items, itr )->data );
		}
	}

	/* free the items array */
	FREE( list->items );

	return TRUE;
}

uint_t list_count( list_t const * const list )
{
	CHECK_PTR_RET( list, 0 );
	return list->count;
}

int list_reserve( list_t * const list, uint const amount )
{
	CHECK_PTR_RET( list, FALSE );

	if ( amount > list->size )
	{
		return list_grow( list, (amount - list->size) );
	}

	return TRUE;
}

int list_clear( list_t * const list )
{
	list_delete_fn dfn = NULL;
	CHECK_PTR_RET( list, FALSE );

	/* remember the delete function pointer */
	dfn = list->dfn;

	/* deinit, then init the list */
	CHECK_RET( list_deinitialize( list ), FALSE );
	CHECK_RET( list_initialize( list, 0, dfn ), FALSE );

	return TRUE;
}

list_itr_t list_itr_begin( list_t const * const list )
{
	CHECK_PTR_RET( list, list_itr_end_t );
	CHECK_RET( list->count, list_itr_end_t );
	return list->used_head;
}

list_itr_t list_itr_end( list_t const * const list )
{
	return list_itr_end_t;
}

list_itr_t list_itr_tail( list_t const * const list )
{
	CHECK_PTR_RET( list, list_itr_end_t );
	CHECK_RET( list->count, list_itr_end_t );

	/* the list is circular so just return the head's prev item index */
	return ITEM_AT( list->items, list->used_head )->prev;
}

list_itr_t list_itr_next( list_t const * const list, list_itr_t const itr )
{
	CHECK_PTR_RET( list, list_itr_end_t );
	CHECK_RET( itr != list_itr_end_t, list_itr_end_t );

	/* if the next item in the list isn't the head, return its index.
	 * otherwise we're at the end of the list, return the end itr */
	return ( ITEM_AT( list->items, itr )->next != list->used_head ) ?
			 ITEM_AT( list->items, itr )->next :
			 list_itr_end_t;
}

list_itr_t list_itr_rnext( list_t const * const list, list_itr_t const itr )
{
	CHECK_PTR_RET( list, list_itr_end_t );
	CHECK_RET( itr != list_itr_end_t, list_itr_end_t );

	/* if the iterator isn't the head, return the previous item index.
	 * otherwise we're at the reverse end of the list, return the end itr */
	return ( itr != list->used_head ) ?
			 ITEM_AT( list->items, itr )->prev :
			 list_itr_end_t;
}

int list_push( list_t * const list, void * const data, list_itr_t const itr )
{
	list_itr_t item = list_itr_end_t;
#if defined(UNIT_TESTING)
	CHECK_RET( !fake_list_push, fake_list_push_ret );
#endif
	CHECK_PTR_RET( list, FALSE );

	/* do we need to resize to accomodate this node? */
	if ( list->count == list->size )
		CHECK_RET( list_grow( list, 1 ), FALSE );

	/* get an item from the free list */
	item = list->free_head;
	list->free_head = remove_item( list->items, list->free_head );

	/* store the data pointer over */
	ITEM_AT( list->items, item )->data = data;
	ITEM_AT( list->items, item )->used = TRUE;

	/* insert the item into the used list */
	list->used_head = insert_item( list->items, list->used_head, item );

	/* update the count */
	list->count++;

	return TRUE;
}

list_itr_t list_pop( list_t * const list, list_itr_t const itr )
{
	/* itr can be either a valid index or list_itr_end_t.  if it is list_itr_end_t
	 * they are asking to pop the tail item of the list.  the return value from
	 * this function is the index of the item after the popped item or
	 * list_itr_end_t if the list is empty or we popped the tail. */
	list_itr_t item = list_itr_end_t;
	list_itr_t next = list_itr_end_t;

	CHECK_PTR_RET( list, list_itr_end_t );
	CHECK_RET( list->size, list_itr_end_t );
	CHECK_RET( ((itr == list_itr_end_t) || ((itr >= 0) && (itr < list->size))), list_itr_end_t );

	/* if they pass in list_itr_end_t, they want to remove the tail item */
	item = (itr == list_itr_end_t) ? list_itr_tail( list ) : itr;

	/* make sure their iterator references an item in the used list */
	CHECK_RET( ITEM_AT( list->items, item )->used, list_itr_end_t );

	/* remove the item from the used list */
	next = remove_item( list->items, item );

	/* if we removed the head, update the used_head iterator */
	if ( item == list->used_head )
		list->used_head = next;

	/* reset the item and put it onto the free list */
	ITEM_AT( list->items, item )->data = NULL;
	ITEM_AT( list->items, item )->used = FALSE;
	list->free_head = insert_item( list->items, list->free_head, item );

	/* update the count */
	list->count--;

	/* if we popped the tail, then we need to return list_itr_end_t, otherwise
	 * we return the iterator of the next item in the list */
	return (itr == list_itr_end_t) ? list_itr_end_t : next;
}

void * list_get( list_t const * const list, list_itr_t const itr )
{
#if defined(UNIT_TESTING)
	CHECK_RET( !fake_list_get, fake_list_get_ret );
#endif
	CHECK_PTR_RET( list, NULL );
	CHECK_RET( itr != list_itr_end_t, NULL );
	CHECK_RET( list->size, NULL );							/* empty list? */
	CHECK_RET( ((itr >= 0) && (itr < list->size)), NULL );	/* valid index? */
	CHECK_RET( ITEM_AT( list->items, itr )->used, NULL );	/* in used list? */

	return ITEM_AT( list->items, itr )->data;
}


/********** PRIVATE **********/

/* removes the item at index "itr" and returns the iterator of the
 * item after the item that was removed. */
static list_itr_t remove_item( list_item_t * const items, list_itr_t const itr )
{
	list_itr_t next = list_itr_end_t;
	CHECK_PTR_RET( items, list_itr_end_t );
	CHECK_RET( itr != list_itr_end_t, list_itr_end_t );

	/* remove the item from the list if there is more than 1 item */
	if ( (ITEM_AT( items, itr )->next != itr) &&
		 (ITEM_AT( items, itr )->prev != itr) )
	{
		next = ITEM_AT( items, itr )->next;
		ITEM_AT( items, ITEM_AT( items, itr )->prev )->next = ITEM_AT( items, itr )->next;
		ITEM_AT( items, ITEM_AT( items, itr )->next )->prev = ITEM_AT( items, itr )->prev;
	}

	/* clear out the item's next/prev links */
	ITEM_AT( items, itr )->next = list_itr_end_t;
	ITEM_AT( items, itr )->prev = list_itr_end_t;

	return next;
}

/* inserts the item at index "item" before the item at "itr" */
static list_itr_t insert_item( list_item_t * const items, 
							   list_itr_t const itr, 
							   list_itr_t const item )
{
	CHECK_PTR_RET( items, list_itr_end_t );
	CHECK_RET( item != list_itr_end_t, list_itr_end_t );

	/* if itr is list_itr_end_t, the list is empty, so make the
	 * item the only node in the list */
	if ( itr == list_itr_end_t )
	{
		ITEM_AT( items, item )->prev = item;
		ITEM_AT( items, item )->next = item;
		return item;
	}

	ITEM_AT( items, item )->next = itr;
	ITEM_AT( items, ITEM_AT( items, itr )->prev )->next = item;
	ITEM_AT( items, item )->prev = ITEM_AT( items, itr )->prev;
	ITEM_AT( items, itr )->prev = item;

	return itr;
}

static int list_grow( list_t * const list, uint_t amount )
{
	uint_t i = 0;
	uint_t new_size = 0;
	list_item_t * items = NULL;
	list_item_t * old = NULL;
	list_itr_t itr = list_itr_end_t;
	list_itr_t end = list_itr_end_t;
	list_itr_t free_head = list_itr_end_t;
	list_itr_t used_head = list_itr_end_t;
	list_itr_t free_item = list_itr_end_t;

	CHECK_PTR_RET( list, FALSE );
	CHECK_RET( amount, TRUE ); /* do nothing if grow amount is 0 */

#if defined(UNIT_TESTING)
	CHECK_RET( !fake_list_grow, fake_list_grow_ret );
#endif

	/* figure out how big the new item array should be */
	if ( list->size )
	{
		new_size = list->size;
		do
		{
			/* double the size until we have enough room */
			new_size <<= 1;

		} while( new_size < (list->size + amount) );
	}
	else
		new_size = amount;

	/* try to allocate a new item array */
	items = CALLOC( new_size, sizeof( list_item_t ) );
	CHECK_PTR_RET( items, FALSE );

	/* initialize the new array as a free list */
	for ( i = 0; i < new_size; i++ )
	{
		free_head = insert_item( items, free_head, i );
	}

	/* move items from old array to new array */
	end = list_itr_end( list );
	for ( itr = list_itr_begin( list ); itr != end; itr = list_itr_next( list, itr ) )
	{
		/* get an item from the free list */
		free_item = free_head;
		free_head = remove_item( items, free_head );

		/* copy the data pointer over */
		ITEM_AT( items, free_item )->data = ITEM_AT( list->items, itr )->data;
		ITEM_AT( items, free_item )->used = ITEM_AT( list->items, itr )->used;

		/* insert the item into the data list */
		used_head = insert_item( items, used_head, free_item );
	}

	/* if we get here, we can update the list struct */
	old = list->items;
	list->items = items;
	list->size = new_size;
	list->used_head = used_head;
	list->free_head = free_head;

	/* free up the old items array */
	FREE( old );

	return TRUE;
}

#if defined(UNIT_TESTING)

#include <CUnit/Basic.h>

void test_list_private_functions( void )
{
	int i;
	list_item_t items[4];
	list_itr_t head = list_itr_end_t;
	MEMSET( items, 0, 4 * sizeof(list_item_t) );

	/* REMOVE_ITEM TESTS */

	/* test remove_item pre-reqs */
	CU_ASSERT_EQUAL( remove_item( NULL, list_itr_end_t ), list_itr_end_t );
	CU_ASSERT_EQUAL( remove_item( items, list_itr_end_t ), list_itr_end_t );

	/* add some items */
	for( i = 0; i < 4; i++ )
	{
		head = insert_item( items, head, i );
	}

	/* remove items from the back */
	CU_ASSERT_EQUAL( remove_item( items, 3 ), 0 );
	CU_ASSERT_EQUAL( remove_item( items, 2 ), 0 );
	CU_ASSERT_EQUAL( remove_item( items, 1 ), 0 );
	CU_ASSERT_EQUAL( remove_item( items, 0 ), list_itr_end_t );

	/* add some items */
	for( i = 0; i < 4; i++ )
	{
		head = insert_item( items, head, i );
	}

	/* remove items from the front */
	CU_ASSERT_EQUAL( remove_item( items, 0 ), 1 );
	CU_ASSERT_EQUAL( remove_item( items, 1 ), 2 );
	CU_ASSERT_EQUAL( remove_item( items, 2 ), 3 );
	CU_ASSERT_EQUAL( remove_item( items, 3 ), list_itr_end_t );

	/* INSERT_ITEM TESTS */

	/* test insert_item pre-reqs */
	CU_ASSERT_EQUAL( insert_item( NULL, list_itr_end_t, list_itr_end_t ), list_itr_end_t );
	CU_ASSERT_EQUAL( insert_item( items, list_itr_end_t, list_itr_end_t ), list_itr_end_t );

	/* LIST_GROW TESTS */
	CU_ASSERT_FALSE( list_grow( NULL, 0 ) );
}

#endif

