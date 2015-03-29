/* Copyright (c) 2012-2015 David Huseby
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stdint.h>
#include "macros.h"
#include "list.h"

/* iterator type */
typedef struct ht_itr_s
{
  int_t               idx;          /* index of the list */
  list_itr_t          itr;          /* list iterator */
} ht_itr_t;

#define ITR_EQ(i, j) ((i.idx == j.idx) && (i.itr == j.itr))

/* hash function prototype */
typedef uint_t (*ht_hash_fn)(void const * key);

/* match function prototype
 * it must return 1 if the data matches, 0 otherwise */
typedef int_t (*ht_match_fn)(void const * l, void const * r);

/* the delete function prototype */
typedef void (*ht_delete_fn)(void * value);

/* the hash table structure */
typedef struct ht_s
{
  ht_hash_fn          hfn;          /* hash function */
  ht_match_fn         mfn;          /* match function */
  ht_delete_fn        dfn;          /* key delete function */
  uint_t              initial;      /* initial capacity */
  float               limit;        /* load limit that will trigger resize */
  uint_t              count;        /* number of items in the hashtable */
  uint_t              size;         /* the size of the list array */
  list_t*             lists;        /* pointer to list array */
} ht_t;

/* heap allocated hash table */
ht_t* ht_new(uint_t initial_capacity, ht_hash_fn hfn,
              ht_match_fn mfn, ht_delete_fn dfn);
void ht_delete(void * ht);

/* stack allocated hash table */
int_t ht_init(ht_t * htable, uint_t initial_capacity,
              ht_hash_fn hfn, ht_match_fn mfn, ht_delete_fn dfn);
int_t ht_deinit(ht_t * htable);

/* returns the number of items stored in the hashtable */
uint_t ht_count(ht_t * htable);

/* inserts the given data into the hash table */
int_t ht_insert(ht_t * htable, void * data);

/* clears all data from the hash table */
int_t ht_clear(ht_t * htable);

/* finds the corresponding data in the hash table */
ht_itr_t ht_find(ht_t const * htable, void * data);

/* remove the key/value at the specified iterator position */
int_t ht_remove(ht_t * htable, ht_itr_t itr);

/* get the data at the given iterator position */
void* ht_get(ht_t const * htable, ht_itr_t itr);

/* iterator based access to the hashtable */
ht_itr_t ht_itr_begin(ht_t const * htable);
ht_itr_t ht_itr_end(ht_t const * htable);
ht_itr_t ht_itr_rbegin(ht_t const * htable);
#define ht_itr_rend(x) ht_itr_end(x)
ht_itr_t ht_itr_next(ht_t const * htable, ht_itr_t itr);
ht_itr_t ht_itr_rnext(ht_t const * htable, ht_itr_t itr);
#define ht_itr_prev(h,i) ht_itr_rnext(h,i)
#define ht_itr_rprev(h,i) ht_itr_next(h,i)

#endif /*HASHTABLE_H*/
