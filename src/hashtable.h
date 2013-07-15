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

#ifndef __HASHTABLE_H__
#define __HASHTABLE_H__

#include <stdint.h>
#include "macros.h"
#include "list.h"

/* iterator type */
typedef struct ht_itr_s
{
    int_t               idx;                /* index of the list */
    list_itr_t          itr;                /* list iterator */
} ht_itr_t;

#define ITR_EQ( i, j ) ((i.idx == j.idx) && (i.itr == j.itr))

/* hash function prototype */
typedef uint_t (*ht_hash_fn)(void const * const key);

/* match function prototype
 * it must return 1 if the data matches, 0 otherwise */
typedef int_t (*ht_match_fn)(void const * const l, void const * const r);

/* the delete function prototype */
typedef void (*ht_delete_fn)(void * value);

/* the hash table structure */
typedef struct ht_s
{
    ht_hash_fn          hfn;                /* hash function */
    ht_match_fn         mfn;                /* match function */
    ht_delete_fn        dfn;                /* key delete function */
    uint_t              initial;            /* initial capacity */
    float               limit;              /* load limit that will trigger resize */
    uint_t              count;              /* number of items in the hashtable */
    uint_t              size;               /* the size of the list array */
    list_t*             lists;              /* pointer to list array */
} ht_t;

/* heap allocated hash table */
ht_t* ht_new( uint_t const initial_capacity, ht_hash_fn hfn, 
              ht_match_fn mfn, ht_delete_fn dfn );
void ht_delete( void * ht );

/* stack allocated hash table */
int_t ht_initialize( ht_t * const htable, uint_t const initial_capacity, 
                   ht_hash_fn hfn, ht_match_fn mfn, ht_delete_fn dfn );
int_t ht_deinitialize( ht_t * const htable );

/* returns the number of items stored in the hashtable */
uint_t ht_count( ht_t * const htable );

/* inserts the given data into the hash table */
int_t ht_insert( ht_t * const htable, void * const data);

/* clears all data from the hash table */
int_t ht_clear( ht_t * const htable );

/* finds the corresponding data in the hash table */
ht_itr_t ht_find( ht_t const * const htable, void * const data );

/* remove the key/value at the specified iterator position */
int_t ht_remove( ht_t * const htable, ht_itr_t const itr );

/* get the data at the given iterator position */
void* ht_get( ht_t const * const htable, ht_itr_t const itr );

/* iterator based access to the hashtable */
ht_itr_t ht_itr_begin( ht_t const * const htable );
ht_itr_t ht_itr_end( ht_t const * const htable );
ht_itr_t ht_itr_rbegin( ht_t const * const htable );
#define ht_itr_rend(x) ht_itr_end(x)
ht_itr_t ht_itr_next( ht_t const * const htable, ht_itr_t const itr );
ht_itr_t ht_itr_rnext( ht_t const * const htable, ht_itr_t const itr );
#define ht_itr_prev(h,i) ht_itr_rnext(h,i)
#define ht_itr_rprev(h,i) ht_itr_next(h,i)


#endif

