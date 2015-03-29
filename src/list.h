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

#ifndef LIST_H
#define LIST_H

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
  list_delete_fn  dfn;            /* destruction function for each node */
  uint_t          size;           /* total number of allocated slots in list */
  uint_t          count;          /* number of items in the list */
  list_itr_t      used_head;      /* head node of the used circular list */
  list_itr_t      free_head;      /* head node of the free circular list */
  list_item_t*    items;          /* array of list items */
} list_t;

/* heap allocated list */
list_t * list_new(uint_t initial_capacity, list_delete_fn dfn);
void list_delete(void * l);

/* stack allocated list */
int_t list_init(list_t * list, uint_t initial_capacity, list_delete_fn dfn);
int_t list_deinit(list_t * list);

/* gets the number of items in the list */
uint_t list_count(list_t const * list);

/* grow the list to be at least this size */
int_t list_reserve(list_t * list, uint amount);

/* clear the entire list */
int_t list_clear(list_t * list);

/* functions for getting iterators */
list_itr_t list_itr_begin(list_t const * list);
list_itr_t list_itr_end(list_t const * list);
#define list_itr_head(x) list_itr_begin(x)
list_itr_t list_itr_tail(list_t const * list);
#define list_itr_rbegin(x) list_itr_tail(x)
#define list_itr_rend(x) list_itr_end(x)

/* iterator manipulation functions */
list_itr_t list_itr_next(list_t const * list, list_itr_t itr);
list_itr_t list_itr_rnext(list_t const * list, list_itr_t itr);
#define list_itr_prev(a,i) list_itr_rnext(a,i)
#define list_itr_rprev(a,i) list_itr_next(a,i)

/* O(1) functions for adding items to the list */
int_t list_push(list_t * list, void * data, list_itr_t itr);
#define list_push_head(list, data) list_push(list, data, list_itr_head(list))
#define list_push_tail(list, data) list_push(list, data, list_itr_end(list))

/* O(1) functions for removing items from the list */
list_itr_t list_pop(list_t * list, list_itr_t itr);
#define list_pop_head(list) list_pop(list, list_itr_head(list))
#define list_pop_tail(list) list_pop(list, list_itr_end(list))

/* functions for getting the data pointer from the list */
void* list_get(list_t const * list, list_itr_t itr);
#define list_get_head(list) list_get(list, list_itr_head(list))
#define list_get_tail(list) list_get(list, list_itr_tail(list))

#endif /*LIST_H*/
