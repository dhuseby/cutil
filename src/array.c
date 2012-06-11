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
#include "array.h"

#define DEFAULT_INITIAL_CAPACITY (16)

#ifdef UNIT_TESTING
static int fail_grow = FALSE;
#endif

/* node used in queue structure */
struct array_node_s
{
	int_t	next;	/* next node in the list */
	int_t	prev;	/* prev node in the list */
	void *	data;	/* pointer to the data */
	uint_t	dummy;
};

#define NODE_AT( a, i ) (&(a->node_buffer[i]))

/* index constants */
array_itr_t const array_itr_end_t = -1;

/* forward declaration */
static int array_grow( array_t * const array );
static int array_sanity_check( array_t const * const array );

#ifdef USE_THREADING
/*
 * This function locks the array mutex.
 */
void array_lock(array_t * const array)
{
	CHECK_PTR(array);

	/* lock the mutex */
	pthread_mutex_lock(&array->lock);
}

/*
 * This function tries to lock the mutex associated with the array.
 */
int array_try_lock(array_t * const array)
{
	CHECK_PTR_RET(array, 0);
	
	/* try to lock the mutex */
	return (pthread_mutex_trylock(&array->lock) == 0);
}

/*
 * This function unlocks the array mutex.
 */
void array_unlock(array_t * const array)
{
	CHECK_PTR(array);

	/* unlock the mutex */
	pthread_mutex_unlock(&array->lock);
}

/*
 * Returns the mutex pointer for use with condition variables.
 */
pthread_mutex_t * array_mutex(array_t * const array)
{
	CHECK_PTR_RET(array, NULL);

	/* return the mutex pointer */
	return &array->lock;
}
#endif/*USE_THREADING*/

/*
 * This function initializes the array_t structure by allocating
 * a node buffer and initializing the indexes for the data head
 * and the free list head.	It then initializes the linked lists.
 * The function pointer points to a destructor for the nodes that 
 * are going to be stored in the data structure.  If NULL is passed 
 * for the pfn parameter, then no action will be taken when the 
 * data structure is deleted.
 */
int array_initialize( array_t * const array, 
					  int_t initial_capacity,
					  delete_fn pfn )
{
	int_t i = 0;
	CHECK_PTR_RET( array, FALSE );
	CHECK_RET( initial_capacity >= 0, FALSE );

	/* zero out the memory */
	MEMSET(array, 0, sizeof(array_t));

	/* store the delete function pointer */
	array->pfn = pfn;

	/* remember the buffer size */
	array->buffer_size = 0;

	/* remember the initial capacity */
	array->initial_capacity = initial_capacity;

	/* initialize the data list */
	array->data_head = -1;

	/* initialize the free list */
	array->free_head = -1;

	/* allocate the initial node buffer only if there is an initial capacity */
	if ( initial_capacity > 0 )
	{
		if( !array_grow( array ) )
		{
			return FALSE;
		}
	}

#ifdef UNIT_TESTING
	ASSERT( array_sanity_check( array ) );
#endif

#ifdef USE_THREADING
	/* initialize the mutex */
	pthread_mutex_init(&array->lock, 0);
#endif

	return TRUE;
}

/*
 * This function allocates a new array structure and
 * initializes it.
 */
array_t * array_new( int_t initial_capacity, delete_fn pfn )
{
	array_t * array = NULL;
	CHECK_PTR_RET( initial_capacity >= 0, NULL );

	/* allocate the array structure */
	array = (array_t*)CALLOC( 1, sizeof(array_t) );

	/* initialize the array */
	if ( !array_initialize( array, initial_capacity, pfn ) )
	{
		FREE( array );
		return NULL;
	}

	/* return the new array */
	return array;
}


/*
 * This function iterates over the list of nodes passing each one to the
 * destroy function if one was provided.  If there wasn't one provided
 * then nothing will be done to clean up each node.
 */
void array_deinitialize(array_t * const array)
{
	int_t ret = 0;
	int_t i = 0;
	array_node_t* pstart = NULL;
	array_node_t* node = NULL;
	CHECK_PTR(array);
#ifdef UNIT_TESTING
	ASSERT( array_sanity_check( array ) );
#endif

	/* iterate over the list calling the destructor for each node */
	if( (array->pfn != NULL) && (array_size( array ) > 0) )
	{
		node = pstart = NODE_AT(array, array->data_head);
		do
		{
			/* if there is data, free it */
			if((node != NULL) && (node->data != NULL))
			{
				/* free up the data */
				(*(array->pfn))(node->data);

				node->data = NULL;
			}

			/* move to the next node in the list */
			node = NODE_AT( array, node->next);

		} while( node != pstart );
	}

	/* free up the node buffer */
	if ( array->node_buffer != NULL )
	{
		FREE( array->node_buffer );
	}
	array->node_buffer = NULL;

#ifdef USE_THREADING
	/* destroy the lock */
	ret = pthread_mutex_destroy(&array->lock);
#ifdef UNIT_TESTING
	ASSERT(ret == 0);
#endif
#endif

	/* invalidate the delete function */
	array->pfn = NULL;

	/* invalidate the two list heads */
	array->data_head = -1;
	array->free_head = -1;

	/* clear out the counts */
	array->num_nodes = 0;
	array->buffer_size = 0;
	
#ifdef UNIT_TESTING
	ASSERT( array_sanity_check( array ) );
#endif
}


/* this frees an allocated array structure */
void array_delete( void * arr )
{
	array_t * array = (array_t*)arr;
	CHECK_PTR(array);

	/* deinit the array */
	array_deinitialize(array);

	/* free the array */
	FREE( (void*)array );
}


/* get the size of the array in a thread-safe way */
int_t array_size( array_t const * const array )
{
	CHECK_PTR_RET(array, 0);

	return (int_t)array->num_nodes;
}


/* 
 * this function doubles the node buffer size and moves the nodes from the old
 * buffer to the new buffer, compacting them at the same time
 */
static int array_grow( array_t * const array )
{
	int_t i = 0;
	int_t old_size = 0;
	int_t new_size = 0;
	array_node_t * new_buffer = NULL;
	CHECK_PTR_RET( array, FALSE );

#ifdef UNIT_TESTING
	/* fail the grow if FAIL_GROW is true */
	if ( fail_grow == TRUE )
		return FALSE;
#endif

	/* get the old size */
	old_size = array->buffer_size;

	/* get the new size */
	if ( old_size == 0 )
	{
		new_size = ((array->initial_capacity > 0) ? array->initial_capacity : DEFAULT_INITIAL_CAPACITY);
	}
	else if ( old_size >= 256 )
	{
		new_size = (old_size + 256);
	}
	else
	{
		new_size = (old_size * 2);
	}

	/* resize the existing buffer */
	CHECK_RET( new_size > old_size, FALSE );
	CHECK_RET( new_size > 0, FALSE );
	if ( array->node_buffer != NULL )
	{
		CHECK_RET( new_size > 0, FALSE );
	}

	/* allocate a new buffer */
	new_buffer = (array_node_t*)REALLOC( array->node_buffer, (new_size * sizeof(array_node_t)) );
	CHECK_PTR_RET( new_buffer, FALSE );

	MEMSET( (void*)&(new_buffer[old_size]), 0, ((new_size - old_size) * sizeof(array_node_t)) );
	array->node_buffer = new_buffer;

	/* store the new size */
	array->buffer_size = new_size;

	/* fix up the middle free nodes */
	for(i = old_size; i < array->buffer_size; i++)
	{
		/* initialize the prev */
		array->node_buffer[i].prev = (i - 1);

		/* initialize the next */
		array->node_buffer[i].next = (i + 1);

		/* initialize the data pointer */
		array->node_buffer[i].data = NULL;
	}

	if ( old_size == 0 )
	{
		/* initialize the free_head index */
		array->free_head = 0;
	}
	else
	{
		/* link the free_head with the head of the new block of ndoes */
		array->node_buffer[array->free_head].next = old_size;
	}

	/* link the free_head node with the tail of the new block of nodes */
	array->node_buffer[array->free_head].prev = (array->buffer_size - 1);
	array->node_buffer[array->buffer_size - 1].next = array->free_head;
	
#ifdef UNIT_TESTING
	ASSERT( array_sanity_check( array ) );
#endif

	return TRUE;
}


static array_itr_t array_get_free_node( array_t * const array )
{
	array_itr_t prev = array_itr_end_t;
	array_itr_t ret = array_itr_end_t;
	array_itr_t next = array_itr_end_t;
	CHECK_PTR_RET(array, array_itr_end_t);

#ifdef UNIT_TESTING
	ASSERT( array_sanity_check( array ) );
#endif

	/* are we taking the last free node? better grow the buffer first */
	if((array->num_nodes + 1) >= array->buffer_size)
	{
		/* we need to grow the buffer */
		if(!array_grow(array))
		{
			return array_itr_end_t;
		}
	}

	/* get the index of the node to return */
	ret = array->free_head;

	/* get the index of the node previous to the free head in the list */
	prev = NODE_AT(array, ret)->prev;

	/* get the index of the node next after the free head in the list */
	next = NODE_AT(array, ret)->next;

	/* set the new free head to the next node in the free list */
	array->free_head = next;

	/* remove the old head */
	NODE_AT(array, prev)->next = next;
	NODE_AT(array, next)->prev = prev;
	NODE_AT(array, ret)->next = array_itr_end_t;
	NODE_AT(array, ret)->prev = array_itr_end_t;

#ifdef UNIT_TESTING
	ASSERT( array_sanity_check( array ) );
#endif

	/* return the node removed from the free list */
	return ret;
}


static void array_put_free_node( array_t * const array, array_itr_t const itr )
{
	array_itr_t prev = array_itr_end_t;
	CHECK_PTR(array);
	CHECK( itr != array_itr_end_t );
	CHECK( itr >= 0 );
#ifdef UNIT_TESTING
	ASSERT( array_sanity_check( array ) );
#endif

	/* clear the data pointer */
	NODE_AT( array, itr )->data = NULL;

	/* get the index of the node previous to the head */
	prev = NODE_AT( array, array->free_head )->prev;

	/* hook in the new head */
	NODE_AT( array, itr)->prev = prev;
	NODE_AT( array, prev )->next = itr;
	NODE_AT( array, itr)->next = array->free_head;
	NODE_AT( array, array->free_head )->prev = itr;

	/* calculate the new free head index */
	array->free_head = itr;
#ifdef UNIT_TESTING
	ASSERT( array_sanity_check( array ) );
#endif
}

/* get an iterator to the start of the array */
array_itr_t array_itr_begin( array_t const * const array )
{
	CHECK_PTR_RET(array, array_itr_end_t);
	CHECK_RET(array_size(array) > 0, array_itr_end_t);
#ifdef UNIT_TESTING
	ASSERT( array_sanity_check( array ) );
#endif

	return array->data_head;
}

/* get an iterator to the end of the array */
array_itr_t array_itr_end( array_t const * const array )
{
	return array_itr_end_t;
}

/* get an iterator to the last item in the array */
array_itr_t array_itr_tail( array_t const * const array )
{
	CHECK_PTR_RET(array, array_itr_end_t);
	CHECK_RET(array_size(array) > 0, array_itr_end_t);
#ifdef UNIT_TESTING
	ASSERT( array_sanity_check( array ) );
#endif
	
	/* return the index of the tail */
	return (array_itr_t)NODE_AT(array, array->data_head)->prev;
}

/* iterator based access to the array elements */
array_itr_t array_itr_next( array_t const * const array, array_itr_t const itr )
{
	CHECK_PTR_RET(array, array_itr_end_t);
	CHECK_RET(array_size(array) > 0, array_itr_end_t);
	CHECK_RET(itr != array_itr_end_t, array_itr_end_t);
#ifdef UNIT_TESTING
	ASSERT( array_sanity_check( array ) );
#endif

	return ( NODE_AT(array, itr)->next == array->data_head ) ? 
			array_itr_end_t :
			NODE_AT(array, itr)->next;
}

array_itr_t array_itr_rnext( array_t const * const array, array_itr_t const itr )
{
	CHECK_PTR_RET(array, array_itr_end_t);
	CHECK_RET(array_size(array) > 0, array_itr_end_t);
	CHECK_RET(itr != array_itr_end_t, array_itr_end_t);
#ifdef UNIT_TESTING
	ASSERT( array_sanity_check( array ) );
#endif

	return ( itr == array->data_head ) ? array_itr_end_t : NODE_AT(array, itr)->prev;
}

/* 
 * push a piece of data into the list at the iterator position
 * ahead of the piece of data already at that position.  To
 * push a piece of data at the tail so that it is the last item
 * in the array, you must use the iterator returned from the
 * array_itr_end() function.
 */
int array_push(
	array_t * const array, 
	void * const data, 
	array_itr_t const itr )
{
	array_itr_t free_itr = array_itr_end_t;

	/* we insert before the iterator, and the data nodes form a circular
	 * list, so if they are inserting at the tail, then we want to insert
	 * before the data_head node */
	array_itr_t const i = ((itr == array_itr_end(array)) ? array->data_head : itr);

	CHECK_PTR_RET( array, FALSE );
#ifdef UNIT_TESTING
	ASSERT( array_sanity_check( array ) );
#endif
	
	/* a the index of a node from the free list */
	free_itr = array_get_free_node(array);
	CHECK_RET( free_itr != array_itr_end_t, FALSE );
	
	/* store the data pointer */
	NODE_AT( array, free_itr )->data = data;
	
	/* hook the new node into the list where the iterator specifies */	
	if(array_size(array) > 0)
	{
		NODE_AT( array, free_itr )->prev = NODE_AT( array, i )->prev;
		NODE_AT(array, NODE_AT(array, free_itr)->prev)->next = free_itr;
		NODE_AT(array, free_itr)->next = i;
		NODE_AT(array, i)->prev = free_itr;
	}
	else
	{
		NODE_AT(array, free_itr)->prev = free_itr;
		NODE_AT(array, free_itr)->next = free_itr;
	}

	/* we only have to update the data_head index if they were pushing the 
	 * new node at the beginning of the array */
	if ( itr == array->data_head )
	{
		array->data_head = free_itr;
	}

	/* we added an item to the list */
	array->num_nodes++;
#ifdef UNIT_TESTING
	ASSERT( array_sanity_check( array ) );
#endif

	return TRUE;
}


/* pop the data at the location specified by the iterator */
void * array_pop(
	array_t * const array, 
	array_itr_t const itr )
{
	void * ret = NULL;
	array_itr_t new_head = array_itr_end_t;
	array_itr_t prev = array_itr_end_t;
	array_itr_t next = array_itr_end_t;

	/* if the itr is at the end, then the node we're removing is the node
	 * before the data_head node in the circular list */
	array_itr_t const i = ((itr == array_itr_end(array)) ? array_itr_tail(array) : itr);
	CHECK_PTR_RET(array, NULL);
#ifdef UNIT_TESTING
	ASSERT( array_sanity_check( array ) );
#endif
	CHECK_RET(array_size(array) > 0, NULL);
	
	/* if we're removing the node at the start of the list, then get a pointer
	 * to the node that will be the new first node */
	if(i == (array_itr_t)array->data_head)
		new_head = NODE_AT( array, array->data_head )->next;
	else
		new_head = array->data_head;

	/* get the index of the node previous to i */
	prev = NODE_AT( array, i )->prev;

	/* get the index of the node after i */
	next = NODE_AT( array, i )->next;

	/* unhook the node from the list */
	NODE_AT( array, prev )->next = NODE_AT( array, i )->next;
	NODE_AT( array, next )->prev = NODE_AT( array, i )->prev;
	NODE_AT( array, i )->prev = array_itr_end_t;
	NODE_AT( array, i )->next = array_itr_end_t;

	/* get the pointer to return */
	ret = NODE_AT( array, i )->data;

	/* put the node back on the free list */
	array_put_free_node(array, i);

	/* we removed an item from the list */
	array->num_nodes--;
	
	/* 
	 * handle the special case where the iterator pointed to the head
	 * by adjusting the head index correctly
	 */
	if(array->num_nodes == 0)
	{
		/* the list is empty so reset the head index */
		array->data_head = array_itr_end(array);
	}
	else
	{
#ifdef UNIT_TESTING
		ASSERT( new_head != array_itr_end_t );
#endif

		/* calculate the new data head index */
		array->data_head = new_head;
	}
	
#ifdef UNIT_TESTING
	ASSERT( array_sanity_check( array ) );
#endif
	return ret;
}

/* get a pointer to the data at the location specified by the iterator */
void* array_itr_get(
	array_t const * const array, 
	array_itr_t const itr )
{
	CHECK_PTR_RET(array, NULL);
	CHECK_RET(array_size(array) > 0, NULL);
	CHECK_RET(itr != array_itr_end(array), NULL);
#ifdef UNIT_TESTING
	ASSERT( array_sanity_check( array ) );
#endif

	/* return the pointer to the head's data */
	return array->node_buffer[itr].data;
}

/* clear the contents of the array */
void array_clear( array_t * const array )
{
	int_t initial_capacity = 0;
	delete_fn pfn = NULL;
	CHECK_PTR(array);
#ifdef UNIT_TESTING
	ASSERT( array_sanity_check( array ) );
#endif
	
	/* store the existing params */
	pfn = array->pfn;
	initial_capacity = array->initial_capacity;
	
	/* de-init the array and clean up all the memory except for
	 * the array struct itself */
	array_deinitialize( array );
#ifdef UNIT_TESTING
	ASSERT( array_sanity_check( array ) );
#endif
	
	/* clear out the array struct */
	memset(array, 0, sizeof(array_t));

	/* reinitialize the array with the saved params */
	array_initialize( array, initial_capacity, pfn );
}

#ifdef UNIT_TESTING
static int array_sanity_check( array_t const * const array )
{
	array_node_t * pstart = NULL;
	array_node_t * p = NULL;
	CHECK_PTR_RET( array, FALSE );

	ASSERT(array->buffer_size >= 0);
	
	if ( array->buffer_size == 0 )
	{
		ASSERT(array->node_buffer == NULL);
		ASSERT(array->data_head == -1);
		ASSERT(array->free_head == -1);
	}
	else
	{
		ASSERT( (array->num_nodes >= 0) && (array->num_nodes <= array->buffer_size) );
		ASSERT( (array->data_head >= -1) && (array->data_head <= (int_t)array->buffer_size) );
		ASSERT( (array->free_head >= -1) && (array->free_head <= (int_t)array->buffer_size) );

		/* check the free list if we have one */
		if ( array->free_head >= 0 )
		{
			p = pstart = NODE_AT( array, array->free_head );
			do {
				/* make sure next is in (0, buffer_size]  */
				ASSERT( p->next >= 0 );
				ASSERT( p->next < (int_t)array->buffer_size );
				/* make sure prev is in (0, buffer_size] */
				ASSERT( p->prev >= 0 );
				ASSERT( p->prev < (int_t)array->buffer_size );
				ASSERT( p->data == NULL );
				p = NODE_AT( array, p->next );

			} while ( p != pstart );
		}

		/* check the data list if we have one */
		if ( array->data_head >= 0 )
		{
			p = pstart = NODE_AT( array, array->free_head );
			do {
				/* make sure next is in (0, buffer_size] */
				ASSERT( p->next >= 0 );
				ASSERT( p->next < (int_t)array->buffer_size );
				/* make sure prev is in (0, buffer_size] */
				ASSERT( p->prev >= 0 );
				ASSERT( p->prev <= array->buffer_size );
				p = NODE_AT( array, p->next );

			} while ( p != pstart );
		}
	}

	return TRUE;
}

void array_force_grow( array_t * const array )
{
	CHECK_PTR( array );
	ASSERT( array_sanity_check( array ) );

	ASSERT( array_sanity_check( array ) );
	array_grow( array );
	ASSERT( array_sanity_check( array ) );

	ASSERT( array_sanity_check( array ) );
}

void array_set_fail_grow( int fail )
{
	fail_grow = fail;
}

#endif
