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

#ifndef __LIST_H__
#define __LIST_H__

#include <stdint.h>
#include "macros.h"

/* define the delete function ponter type */
typedef void (*list_delete_fn)(void*);

/* defines the list iterator type */
typedef int_t list_itr_t;

/* internal node struct */
typedef struct list_item_s list_item_t;

/* dynamic list struct */
typedef struct list_s
{
	list_delete_fn	dfn;					/* destruction function for each node */
	uint_t			size;					/* total number of allocated slots in list */
	uint_t			count;					/* number of items in the list */
	list_itr_t		used_head;				/* head node of the used circular list */
	list_itr_t		free_head;				/* head node of the free circular list */
	list_item_t*	items;					/* array of list items */

} list_t;

/* heap allocated list */
list_t * list_new( uint_t const initial_capacity, list_delete_fn dfn );
void list_delete( void * l );

/* stack allocated list */
int list_initialize( list_t * const list, uint_t const initial_capacity, list_delete_fn dfn );
int list_deinitialize( list_t * const list );

/* gets the number of items in the list */
uint_t list_count( list_t const * const list );

/* grow the list to be at least this size */
int list_reserve( list_t * const list, uint const amount );

/* clear the entire list */
int list_clear( list_t * const list );

/* functions for getting iterators */
list_itr_t list_itr_begin( list_t const * const list );
list_itr_t list_itr_end( list_t const * const list );
#define list_itr_head(x) list_itr_begin(x)
list_itr_t list_itr_tail( list_t const * const list );
#define list_itr_rbegin(x) list_itr_tail(x)
#define list_itr_rend(x) list_itr_end(x)

/* iterator manipulation functions */
list_itr_t list_itr_next( list_t const * const list, list_itr_t const itr);
list_itr_t list_itr_rnext( list_t const * const list, list_itr_t const itr );
#define list_itr_prev(a,i) list_itr_rnext(a,i)
#define list_itr_rprev(a,i) list_itr_next(a,i)

/* O(1) functions for adding items to the list */
int list_push( list_t * const list, void * const data, list_itr_t const itr );
#define list_push_head(list, data) list_push(list, data, list_itr_head(list))
#define list_push_tail(list, data) list_push(list, data, list_itr_end(list))

/* O(1) functions for removing items from the list */
list_itr_t list_pop( list_t * const list, list_itr_t const itr );
#define list_pop_head(list) list_pop(list, list_itr_head(list))
#define list_pop_tail(list) list_pop(list, list_itr_end(list))

/* functions for getting the data pointer from the list */
void* list_itr_get( list_t const * const list, list_itr_t const itr );
#define list_get_head(list) list_itr_get(list, list_itr_head(list))
#define list_get_tail(list) list_itr_get(list, list_itr_tail(list))

#if defined(UNIT_TESTING)
void test_list_private_functions( void );
#endif

#endif/*__LIST_H__*/
 
