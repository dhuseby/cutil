/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
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

#ifndef __BTREE_H__
#define __BTREE_H__

#include <stdint.h>
#include "macros.h"

/* the  iterator type */
typedef void * bt_itr_t;

/* the binary tree opaque handle */
typedef struct bt_s bt_t;

/* define the key compare than function
 * must return -1 if l < r
 * must return 0 if l == r
 * must return 1 if l > r
 * state is optional and used for faster compares */
typedef int (*bt_key_cmp_fn)( void * l, void * r);

/* define the value delete function type */
typedef void (*bt_delete_fn)(void * value);

/* dynamically allocates and initializes a binary tree */
/* NOTE: If NULL is passed in for the key_cmp_fn function, the pointer to the 
 * key will be cast to an int32 and used as the compare value.  If NULL is passed 
 * for the delete functions, then the key/values will not be deleted when 
 * bt_delete is called. */
bt_t* bt_new(
    uint_t initial_capacity, 
    bt_key_cmp_fn kcfn,
    bt_delete_fn vdfn, 
    bt_delete_fn kdfn);

/* deinitializes and frees a binary tree allocated with bt_new() */
void bt_delete(void * bt);

/* returns the number of key/value pairs stored in the btree */
uint_t bt_size(bt_t * const btree);

/* adds a key/value pair to the btree */
int bt_add(
    bt_t * const btree, 
    void * const key, 
    void * const value);

/* find a value by it's key */
void * bt_find(bt_t * const btree, void * const key);

/* remove the value associated with the key from the btree */
void * bt_remove(bt_t * const btree, void * const key);

/* print tree */
void bt_print( bt_t * const btree );

/* In-order, forward, iterator based access to the btree */
bt_itr_t bt_itr_begin( bt_t const * const btree );
bt_itr_t bt_itr_next(
    bt_t const * const btree, 
    bt_itr_t const itr );
bt_itr_t bt_itr_end( bt_t const * const btree );

/* in-order, reverse, iterator based access to the btree */
bt_itr_t bt_itr_rbegin( bt_t const * const btree );
bt_itr_t bt_itr_rnext(
    bt_t const * const btree,
    bt_itr_t const itr );
bt_itr_t bt_itr_rend( bt_t const * const btree );

void* bt_itr_get(
    bt_t const * const btree, 
    bt_itr_t const itr);

void* bt_itr_get_key(
    bt_t const * const btree,
    bt_itr_t const itr);

#endif
