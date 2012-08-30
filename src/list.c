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

#ifdef UNIT_TESTING
extern int fail_list_grow;
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

list_t * list_new( uint_t const initial_capacity, delete_fn dfn )
{
	list_t * list = CALLOC( 1, sizeof(list_t) );
	CHECK_PTR_RET( list, NULL );
	CHECK_RET( list_initialize( list, initial_capacity, dfn ), NULL );
	return list;
}

void list_delete( void * l )
{
	CHECK_PTR( l );
	list_deinitialize( (list_t*)l );
	FREE( l );
}

int list_initialize( list_t * const list, uint_t const initial_capacity, delete_fn dfn )
{
	uint_t i;
	CHECK_PTR_RET( list, FALSE );

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

void list_deinitialize( list_t * const list )
{
	list_itr_t itr, end;
	CHECK_PTR( list );
	CHECK( list->size > 0 );

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
}

uint_t list_count( list_t const * const list )
{
	CHECK_PTR_RET( list, 0 );
	return list->count;
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
	CHECK_RET( ((itr >= 0) && (itr < list->size)), list_itr_end_t );

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

	/* if we popped the tail, then we need to return list_itr_end_t, otherwise
	 * we return the iterator of the next item in the list */
	return (itr == list_itr_end_t) ? list_itr_end_t : next;
}

void * list_itr_get( list_t const * const list, list_itr_t const itr )
{
	CHECK_PTR_RET( list, NULL );
	CHECK_RET( itr != list_itr_end_t, NULL );
	CHECK_RET( ((itr >= 0) && (itr < list->size)), NULL );	/* valid index? */
	CHECK_RET( ITEM_AT( list->items, itr )->used, NULL );	/* in used list? */

	return ITEM_AT( list->items, itr )->data;
}

void list_clear( list_t * const list )
{
	delete_fn dfn = NULL;
	CHECK_PTR(list);

	/* remember the delete function pointer */
	dfn = list->dfn;

	/* deinit, then init the list */
	list_deinitialize( list );
	list_initialize( list, 0, dfn );
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
	list_itr_t cur = itr;
	CHECK_PTR_RET( items, list_itr_end_t );
	CHECK_RET( item != list_itr_end_t, list_itr_end_t );

	/* check for an empty list */
	if ( cur == list_itr_end_t )
		cur = item;

	ITEM_AT( items, item )->next = cur;
	ITEM_AT( items, ITEM_AT( items, cur )->prev )->next = item;
	ITEM_AT( items, item )->prev = ITEM_AT( items, cur )->prev;
	ITEM_AT( items, cur )->prev = item;

	return cur;
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
	/* fail the grow if FAIL_GROW is true */
	if ( fail_list_grow == TRUE )
		return FALSE;
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

