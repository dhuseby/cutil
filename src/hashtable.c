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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "debug.h"
#include "macros.h"
#include "list.h"
#include "hashtable.h"

#if defined(UNIT_TESTING)
#include "test_flags.h"
ht_itr_t fake_ht_find_ret;
#endif

#define LIST_AT( lists, index )  (&(lists[index]))
#define ITEM_AT( lists, itr ) (list_get(LIST_AT(itr.list), itr.itr))

/* index constants */
ht_itr_t const ht_itr_end_t = { -1, -1 };

/* load factor is how full the table will get before a resize is triggered */
float const default_load_limit = 3.0f;

/* this list of primes are the table size used by the hashtable.  each prime
 * is roughly double the size of the previous prime to get amortized resize
 * operations. */
uint_t const NUM_PRIMES = 30;
uint_t const PRIMES[] =
{
    3, 7, 13, 29, 53, 97, 193, 389, 769, 1543, 3079,
    6151, 12289, 24593, 49157,
    98317, 196613, 393241, 786433,
    1572869, 3145739, 6291469,
    12582917, 251658343, 50331653,
    100663319, 201326611,
    402563189, 805306457,
    1610612741
};

/* forward declarations of private functions */
static uint_t ht_get_new_size( uint_t const count, float const limit );
static int_t ht_grow( ht_t * const htable );

/* heap allocate the hashtable */
ht_t* ht_new( uint_t const initial_capacity, ht_hash_fn hfn,
              ht_match_fn mfn, ht_delete_fn dfn )
{
    ht_t* htable = NULL;

    /* allocate a hashtable struct */
    htable = (ht_t*)CALLOC(1, sizeof(ht_t));
    CHECK_PTR_RET(htable, NULL);

    /* initialize the hashtable */
    if ( !ht_initialize(htable, initial_capacity, hfn, mfn, dfn) )
    {
        FREE( htable );
        return NULL;
    }

    /* return the new hashtable */
    return htable;
}

/* deinitializes and frees a hashtable allocated with ht_new() */
void ht_delete(void * ht)
{
    ht_t * htable = (ht_t*)ht;
    CHECK_PTR(htable);

    /* de-init the hashtable if needed */
    ht_deinitialize(htable);

    /* free the hashtable */
    FREE((void*)htable);
}

/* initializes a hashtable */
int_t ht_initialize( ht_t * const htable, uint_t const initial_capacity,
                   ht_hash_fn hfn, ht_match_fn mfn, ht_delete_fn dfn )
{
    uint_t i = 0;

    UNIT_TEST_RET( ht_init );

    CHECK_PTR_RET( htable, FALSE );
    CHECK_PTR_RET( hfn, FALSE );
    CHECK_PTR_RET( mfn, FALSE );

    /* zero out the memory */
    MEMSET( htable, 0, sizeof(ht_t) );

    /* initialize the members */
    htable->hfn = hfn;
    htable->mfn = mfn;
    htable->dfn = dfn;
    htable->initial = initial_capacity;
    htable->limit = default_load_limit;

    CHECK_RET( ht_grow( htable ), FALSE );

    return TRUE;
}

/* deinitialize a hashtable. */
int_t ht_deinitialize( ht_t * const htable )
{
    uint_t i;

    UNIT_TEST_RET( ht_deinit );

    CHECK_PTR_RET( htable, FALSE );

    /* free up all of the memory in the lists */
    for( i = 0; i < htable->size; i++ )
    {
        list_deinitialize( LIST_AT( htable->lists, i ) );
    }

    /* free up the lists */
    FREE( htable->lists );

    return TRUE;
}

uint_t ht_count( ht_t * const htable )
{
    CHECK_PTR_RET( htable, 0 );
    return htable->count;
}

int_t ht_insert( ht_t * const htable, void * const data )
{
    uint_t index;
    CHECK_PTR_RET( htable, FALSE );
    CHECK_PTR_RET( data, FALSE );

    /* make sure the item isn't already in the list */
    CHECK_RET( ITR_EQ(ht_find( htable, data ), ht_itr_end_t), FALSE );

    /* does the table need to grow? */
    if ( (htable->count / htable->size) > htable->limit )
        CHECK_RET( ht_grow( htable ), FALSE );

    /* hash the data and figure out which list to add it to */
    index = (*(htable->hfn))(data) % htable->size;

    /* add the data to the appropriate list */
    CHECK_RET( list_push_tail( LIST_AT( htable->lists, index ), data ), FALSE );

    /* update the count */
    htable->count++;

    return TRUE;
}

int_t ht_clear( ht_t * const htable )
{
    ht_t tmp;
    CHECK_PTR_RET( htable, FALSE );

    /* make a copy of the htable members */
    MEMCPY( &tmp, htable, sizeof(ht_t) );
    tmp.lists = NULL;

    /* deinit, then init the table */
    CHECK_RET( ht_deinitialize( htable ), FALSE );
    CHECK_RET( ht_initialize( htable, tmp.initial, tmp.hfn, tmp.mfn, tmp.dfn ), FALSE );

    return TRUE;
}

ht_itr_t ht_find( ht_t const * const htable, void * const data )
{
    uint_t index;
    ht_itr_t ret;
    list_itr_t itr, end;

    UNIT_TEST_RET( ht_find );

    CHECK_PTR_RET( htable, ht_itr_end_t );
    CHECK_PTR_RET( data, ht_itr_end_t );

    /* get the list index */
    index = (*(htable->hfn))(data) % htable->size;

    end = list_itr_end( LIST_AT( htable->lists, index ) );
    itr = list_itr_begin( LIST_AT( htable->lists, index ) );
    for( ; itr != end; itr = list_itr_next( LIST_AT( htable->lists, index ), itr ) )
    {
        if ( (*(htable->mfn))( data, list_get(LIST_AT( htable->lists, index ), itr) ) )
        {
            ret.idx = index;
            ret.itr = itr;
            return ret;
        }
    }

    return ht_itr_end_t;
}

int_t ht_remove( ht_t * const htable, ht_itr_t const itr )
{
    CHECK_PTR_RET( htable, FALSE );
    CHECK_RET( !ITR_EQ( itr, ht_itr_end_t ), FALSE );
    CHECK_RET( ((itr.idx >= 0) && (itr.idx < htable->size)), FALSE );
    CHECK_RET( htable->count > 0, FALSE );
    CHECK_PTR_RET( list_get( LIST_AT( htable->lists, itr.idx ), itr.itr ), FALSE );

    /* remove the item from the list */
    list_pop( LIST_AT( htable->lists, itr.idx ), itr.itr );

    /* update the count */
    htable->count--;

    return TRUE;
}

void * ht_get( ht_t const * const htable, ht_itr_t const itr )
{
    CHECK_PTR_RET( htable, FALSE );
    CHECK_RET( !ITR_EQ( itr, ht_itr_end_t ), FALSE );
    CHECK_RET( ((itr.idx >= 0) && (itr.idx < htable->size)), FALSE );

    return list_get( LIST_AT( htable->lists, itr.idx ), itr.itr );
}

ht_itr_t ht_itr_begin( ht_t const * const htable )
{
    int_t i = 0;
    CHECK_PTR_RET( htable, ht_itr_end_t );
    CHECK_RET( htable->size > 0, ht_itr_end_t );
    CHECK_RET( htable->count > 0, ht_itr_end_t );

    /* find the first list that isn't empty */
    while( (i < htable->size) && (list_count( LIST_AT( htable->lists, i ) ) == 0) )
        i++;

    /* if we didn't find a non-empty list, return end itr */
    CHECK_RET( i < htable->size, ht_itr_end_t );

    /* return an iterator to the beginning of that list */
    return (ht_itr_t){ .idx = i, .itr = list_itr_begin( LIST_AT( htable->lists, i ) ) };
}

ht_itr_t ht_itr_end( ht_t const * const htable )
{
    return ht_itr_end_t;
}

ht_itr_t ht_itr_rbegin( ht_t const * const htable )
{
    int_t i = 0;
    CHECK_PTR_RET( htable, ht_itr_end_t );
    CHECK_RET( htable->size > 0, ht_itr_end_t );
    CHECK_RET( htable->count > 0, ht_itr_end_t );
    i = htable->size;

    /* scan from the end to the beginging */
    do
    {
        i--;
    } while( (i >= 0) && (list_count( LIST_AT( htable->lists, i ) ) == 0) );

    /* check to see if we found a list */
    CHECK_RET( i >= 0, ht_itr_end_t );

    /* return an iterator to the end of the the list we found */
    return (ht_itr_t){ .idx = i, .itr = list_itr_rbegin( LIST_AT( htable->lists, i ) ) };
}

ht_itr_t ht_itr_next( ht_t const * const htable, ht_itr_t const itr )
{
    ht_itr_t ret = itr;
    CHECK_PTR_RET( htable, ht_itr_end_t );

    /* advance the iterator */
    ret.itr = list_itr_next( LIST_AT( htable->lists, ret.idx ), ret.itr );

    if ( ret.itr == list_itr_end( LIST_AT( htable->lists, ret.idx ) ) )
    {
        /* we need to scan for the next non-empty list */
        do
        {
            ret.idx++;
        } while( (ret.idx < htable->size) &&
                 (list_count( LIST_AT( htable->lists, ret.idx ) ) == 0) );

        /* if we didn't find a non-empty list, return end itr */
        CHECK_RET( ret.idx < htable->size, ht_itr_end_t );

        /* re-initialize the list iterator */
        ret.itr = list_itr_begin( LIST_AT( htable->lists, ret.idx ) );
    }

    return ret;
}

ht_itr_t ht_itr_rnext( ht_t const * const htable, ht_itr_t const itr )
{
    ht_itr_t ret = itr;
    CHECK_PTR_RET( htable, ht_itr_end_t );

    /* advance the iterator */
    ret.itr = list_itr_rnext( LIST_AT( htable->lists, ret.idx ), ret.itr );

    if ( ret.itr == list_itr_end( LIST_AT( htable->lists, ret.idx ) ) )
    {
        /* we need to scan for the next non-empty list */
        do
        {
            ret.idx--;
        } while( (ret.idx >= 0) && (list_count( LIST_AT( htable->lists, ret.idx ) ) == 0) );

        /* check if we found a list */
        CHECK_RET( ret.idx >= 0, ht_itr_end_t );

        /* re-initialize the list iterator */
        ret.itr = list_itr_rbegin( LIST_AT( htable->lists, ret.idx ) );
    }

    return ret;
}



/********** PRIVATE **********/

static uint_t ht_get_new_size( uint_t const count, float const limit )
{
    uint_t index = 0;

    /* find the first prime that results in a load less than the limit */
    while( ( index < (NUM_PRIMES - 1) ) && ( ((float)count / (float)PRIMES[index]) > limit ) )
    {
        DEBUG( "%f / %f = %f > %f\n", (float)count, (float)PRIMES[index], ((float)count / (float)PRIMES[index]), limit );
        index++;
    }

    DEBUG( "%f / %f = %f > %f\n", (float)count, (float)PRIMES[index], ((float)count / (float)PRIMES[index]), limit );

    return PRIMES[index];
}

static int_t ht_grow( ht_t * const htable )
{
    uint_t i, count, new_size, old_size;
    list_t *new_lists, *old_lists;

    UNIT_TEST_RET( ht_grow );

    CHECK_PTR_RET( htable, FALSE );

    /* if it's empty, then use the initial capacity */
    count = htable->count ? htable->count : htable->initial;

    /* figure out the new size from the count */
    new_size = ht_get_new_size( count, htable->limit );

    /* remember some stuff */
    old_size = htable->size;
    old_lists = htable->lists;

    /* allocate the new list array */
    new_lists = CALLOC( new_size, sizeof(list_t) );
    CHECK_PTR_RET( new_lists, FALSE );

    /* initialize the lists */
    for ( i = 0; i < new_size; i++ )
    {
        if ( !list_initialize( LIST_AT( new_lists, i ), 0, htable->dfn ) )
        {
            FREE( new_lists );
            return FALSE;
        }
    }

    /* set up the hash table with the empty lists */
    htable->count = 0;
    htable->size = new_size;
    htable->lists = new_lists;

    /* hash the old data into the new table */
    for ( i = 0; i < old_size; i++ )
    {
        while( list_count( LIST_AT( old_lists, i ) ) )
        {
            /* hash the data item into the new table */
            ht_insert( htable, list_get_head( LIST_AT( old_lists, i ) ) );

            /* remove the data item from the old list */
            list_pop_head( LIST_AT( old_lists, i ) );
        }

        /* free up the list memory */
        list_deinitialize( LIST_AT( old_lists, i ) );
    }

    /* free up the old lists array */
    FREE( old_lists );

    return TRUE;
}


#ifdef UNIT_TESTING

#include <CUnit/Basic.h>

void test_hashtable_private_functions( void )
{
    ht_t ht;
    MEMSET( &ht, 0, sizeof(ht_t) );

    /* ht_get_new_size */
    CU_ASSERT_EQUAL( ht_get_new_size( (uint_t)-1, 0.1 ), 1610612741 );

    /* ht_grow */
    CU_ASSERT_FALSE( ht_grow( NULL ) );
    fail_alloc = TRUE;
    CU_ASSERT_FALSE( ht_grow( &ht ) );
    fail_alloc = FALSE;
    fake_list_init = TRUE;
    fake_list_init_ret = FALSE;
    CU_ASSERT_FALSE( ht_grow( &ht ) );
    fake_list_init = FALSE;
}

#endif

