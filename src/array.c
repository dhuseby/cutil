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

/* node used in queue structure */
struct array_node_s
{
	struct array_node_s *	next;					/* next link */
	struct array_node_s *	prev;					/* prev link */
	void *					data;					/* pointer to the data */
	uint_t				dummy;
};

/* index constants */
array_itr_t const array_itr_end_t = -1;

/* forward declaration */
static int array_grow(array_t * const array);

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
void array_initialize( array_t * const array, 
					   uint_t initial_capacity,
					   delete_fn pfn )
{
	uint_t i = 0;
	CHECK_PTR(array);

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
	array->free_head = 0;

	/* allocate the initial node buffer only if there is an initial capacity */
	if ( initial_capacity > 0 )
	{
		if( !array_grow( array ) )
		{
			WARN("failed to grow the array\n");
		}
	}

#ifdef USE_THREADING
	/* initialize the mutex */
	pthread_mutex_init(&array->lock, 0);
#endif
}

/*
 * This function allocates a new array structure and
 * initializes it.
 */
array_t * array_new( uint_t initial_capacity, delete_fn pfn )
{
	array_t * array = NULL;

	/* allocate the array structure */
	array = (array_t*)CALLOC( 1, sizeof(array_t) );

	/* initialize the array */
	array_initialize( array, initial_capacity, pfn );

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
	uint_t i = 0;
	array_node_t* node = NULL;
	CHECK_PTR(array);

	/* iterate over the list calling the destructor for each node */
	if((array->pfn != NULL) && (array->node_buffer != NULL))
	{
		node = array->node_buffer;
		for(i = 0; i < array->num_nodes; i++)
		{
			/* if there is data, free it */
			if((node != NULL) && (node->data != NULL))
			{
				/* free up the data */
				(*(array->pfn))(node->data);

				node->data = NULL;
			}

			/* move to the next node in the list */
			node++;
		}
	}

	/* free up the node buffer */
	FREE( array->node_buffer );
	array->node_buffer = NULL;

#ifdef USE_THREADING
	/* destroy the lock */
	ret = pthread_mutex_destroy(&array->lock);
	ASSERT(ret == 0);
#endif

	/* invalidate the delete function */
	array->pfn = NULL;

	/* invalidate the two list heads */
	array->data_head = -1;
	array->free_head = -1;

	/* clear out the counts */
	array->num_nodes = 0;
	array->buffer_size = 0;
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
int_t array_size(array_t const * const array)
{
	CHECK_PTR_RET(array, -1);

	return (int_t)array->num_nodes;
}


/* 
 * this function doubles the node buffer size and moves the nodes from the old
 * buffer to the new buffer, compacting them at the same time
 */
static int array_grow(array_t * const array)
{
	uint_t i = 0;
	uint_t old_size = 0;
	uint_t new_size = 0;
	array_node_t * new_buffer = NULL;
	CHECK_PTR_RET(array, 0);

	/* get the old size */
	old_size = array->buffer_size;

	/* get the new size */
	if ( old_size == 0 )
	{
		new_size = array->initial_capacity;
	}
	else if ( old_size >= 256 )
	{
		new_size = (old_size + 256);
	}
	else
	{
		new_size = (old_size << 1);
	}

	/* resize the existing buffer */
	ASSERT( new_size > array->buffer_size );
	new_buffer = (array_node_t*)CALLOC( new_size, sizeof(array_node_t) );
	MEMCPY( (void*)new_buffer, (void*)array->node_buffer, old_size * sizeof(array_node_t) );
	FREE( array->node_buffer );
	array->node_buffer = new_buffer;

	/* zero out the new memory */
	MEMSET( (void*)&(array->node_buffer[old_size]), 0, ((new_size - old_size) * sizeof(array_node_t)) );

	/* store the new size */
	array->buffer_size = new_size;

	/* make the new free list head's prev point at the new free list's tail node */
	array->node_buffer[array->free_head].prev = &array->node_buffer[array->buffer_size - 1];

	/* make the new free list head's next point at the first node in the new memory */
	if ( old_size > 0 )
		array->node_buffer[array->free_head].next = &array->node_buffer[old_size];

	/* make the new free list tail's next point at the new free list's head node */
	array->node_buffer[array->buffer_size - 1].next = &array->node_buffer[array->free_head];
	
	/* fix up the middle free nodes */
	for(i = old_size; i < array->buffer_size; i++)
	{
		/* initialize the prev pointer if needed */
		if(array->node_buffer[i].prev == NULL)
			array->node_buffer[i].prev = &array->node_buffer[i - 1];

		/* initialize the next pointer if needed */
		if(array->node_buffer[i].next == NULL)
			array->node_buffer[i].next = &array->node_buffer[i + 1];

		/* initialize the data pointer */
		array->node_buffer[i].data = NULL;
	}

	return TRUE;
}


static array_node_t* array_get_free_node(array_t * const array)
{
	array_node_t* node = NULL;
	array_node_t* new_free_head = NULL;
	CHECK_PTR_RET(array, NULL);

	/* are we taking the last free node? better grow the buffer first */
	if((array->num_nodes + 1) >= array->buffer_size)
	{
		/* we need to grow the buffer */
		if(!array_grow(array))
		{
			WARN("failed to grow the array!");
			return NULL;
		}
	}

	/* remove the node at the head of the free list */
	node = &array->node_buffer[array->free_head];

	/* get the pointer to the new free head */
	new_free_head = node->next;

	/* remove the old head */
	node->prev->next = node->next;
	node->next->prev = node->prev;
	node->next = NULL;
	node->prev = NULL;

	/* calculate the new free head index */
	array->free_head = (int_t)((uint_t)new_free_head - (uint_t)array->node_buffer) / sizeof(array_node_t);

	/* return the old free head */
	return node;
}


static void array_put_free_node(
	array_t * const array, 
	array_node_t* const node)
{
	array_node_t* old_free_head = NULL;
	CHECK_PTR(array);

	/* clear the data pointer */
	node->data = NULL;

	/* get the old free head */
	old_free_head = &array->node_buffer[array->free_head];

	/* hook in the new head */
	node->prev = old_free_head->prev;
	node->prev->next = node;
	node->next = old_free_head;
	node->next->prev = node;

	/* calculate the new free head index */
	array->free_head = (int_t)((uint_t)node - (uint_t)array->node_buffer) / sizeof(array_node_t);
}

/* get an iterator to the start of the array */
array_itr_t array_itr_begin(array_t const * const array)
{
	CHECK_PTR_RET(array, array_itr_end_t);
	CHECK_RET(array_size(array) > 0, array_itr_end_t);

	return array->data_head;
}

/* get an iterator to the end of the array */
array_itr_t array_itr_end(array_t const * const array)
{
	return array_itr_end_t;
}

/* get an iterator to the last item in the array */
array_itr_t array_itr_tail(array_t const * const array)
{
	array_node_t* tail = NULL;
	
	CHECK_PTR_RET(array, array_itr_end_t);
	CHECK_RET(array_size(array) > 0, array_itr_end_t);
	
	/* get a pointer to the tail */
	tail = array->node_buffer[array->data_head].prev;
	
	/* calculate return the iterator pointed to the tail item */
	return (array_itr_t)(((uint_t)tail - (uint_t)array->node_buffer) / sizeof(array_node_t));
}

/* iterator based access to the array elements */
array_itr_t array_itr_next(
	array_t const * const array, 
	array_itr_t const itr)
{
	array_node_t* node = NULL;
	
	CHECK_PTR_RET(array, array_itr_end_t);
	CHECK_RET(array_size(array) > 0, array_itr_end_t);
	CHECK_RET(itr != array_itr_end_t, array_itr_end_t);

	/* get a pointer to the node the iterator points to */
	node = array->node_buffer[itr].next;

	/* check that we got a valid node pointer */
	CHECK_RET(node != NULL, array_itr_end_t);

	/* check to see if we've gone full circle */
	if(node == &array->node_buffer[array->data_head])
	{
		/* yep so return -1 */
		return array_itr_end_t;
	}

	/* return the index of the node */
	return (array_itr_t)(((uint_t)node - (uint_t)array->node_buffer) / sizeof(array_node_t));
}

array_itr_t array_itr_rnext(
	array_t const * const array, 
	array_itr_t const itr)
{
	array_node_t* node = NULL;
	
	CHECK_PTR_RET(array, array_itr_end_t);
	CHECK_RET(array_size(array) > 0, array_itr_end_t);
	CHECK_RET(itr != array_itr_end_t, array_itr_end_t);

	/* get a pointer to the node the iterator points to */
	node = &(array->node_buffer[itr]);

	/* check that we got a valid node pointer */
	CHECK_RET(node != NULL, array_itr_end_t);

	/* check to see if we've gone full circle */
	if(node == &array->node_buffer[array->data_head])
	{
		/* yep so return -1 */
		return array_itr_end_t;
	}

	/* now get the previous node pointer */
	node = array->node_buffer[itr].prev;

	/* check that we got a valid node pointer */
	CHECK_RET(node != NULL, array_itr_end_t);

	/* return the index of the node */
	return (array_itr_t)(((uint_t)node - (uint_t)array->node_buffer) / sizeof(array_node_t));
}

/* 
 * push a piece of data into the list at the iterator position
 * ahead of the piece of data already at that position.  To
 * push a piece of data at the tail so that it is the last item
 * in the array, you must use the iterator returned from the
 * array_itr_end() function.
 */
void array_push(
	array_t * const array, 
	void * const data, 
	array_itr_t const itr)
{
	array_node_t * node = NULL;

	/* we insert before the iterator, and the data nodes form a circular
	 * list, so if they are inserting at the tail, then we want to insert
	 * before the data_head node */
	array_itr_t const i = ((itr == array_itr_end(array)) ? array->data_head : itr);

	CHECK_PTR(array);
	
	/* a node from the free list */
	node = array_get_free_node(array);
	ASSERT(node != NULL);
	
	/* store the data pointer */
	node->data = data;
	
	/* hook the new node into the list where the iterator specifies */	
	if(array_size(array) > 0)
	{
		node->prev = array->node_buffer[i].prev;
		node->prev->next = node;
		node->next = &array->node_buffer[i];
		node->next->prev = node;
	}
	else
	{
		node->prev = node;
		node->next = node;
	}

	/* we only have to update the data_head index if they were pushing the 
	 * new node at the beginning of the array */
	if ( itr == array->data_head )
	{
		array->data_head = (int_t)((uint_t)node - (uint_t)array->node_buffer) / sizeof(array_node_t);
	}

	/* we added an item to the list */
	array->num_nodes++;
}


/* pop the data at the location specified by the iterator */
void * array_pop(
	array_t * const array, 
	array_itr_t const itr)
{
	void * ret = NULL;
	array_node_t * new_head = NULL;
	array_node_t * node = NULL;

	/* if the itr is at the end, then the node we're removing is the node
	 * before the data_head node in the circular list */
	array_itr_t const i = ((itr == array_itr_end(array)) ? array_itr_tail(array) : itr);
	CHECK_PTR_RET(array, NULL);
	CHECK_RET(array_size(array) > 0, NULL);
	
	/* get a pointer to the node */
	node = &array->node_buffer[i];
	
	/* if we're removing the node at the start of the list, then get a pointer
	 * to the node that will be the new first node */
	if(i == (array_itr_t)array->data_head)
		new_head = node->next;

	/* unhook the node from the list */
	node->prev->next = node->next;
	node->next->prev = node->prev;
	node->prev = NULL;
	node->next = NULL;

	/* get the pointer to return */
	ret = node->data;

	/* put the node back on the free list */
	array_put_free_node(array, node);

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
		/* calculate the new data head index */
		ASSERT( new_head != NULL );
		array->data_head = (int_t)((uint_t)new_head - (uint_t)array->node_buffer) / sizeof(array_node_t);
	}
	
	return ret;
}

/* get a pointer to the data at the location specified by the iterator */
void* array_itr_get(
	array_t const * const array, 
	array_itr_t const itr)
{
	CHECK_PTR_RET(array, NULL);
	CHECK_RET(array_size(array) > 0, NULL);
	CHECK_RET(itr != array_itr_end(array), NULL);

	/* return the pointer to the head's data */
	return array->node_buffer[itr].data;
}

/* clear the contents of the array */
void array_clear(array_t * const array)
{
	uint_t initial_capacity = 0;
	delete_fn pfn = NULL;
	CHECK_PTR(array);
	
	/* store the existing params */
	pfn = array->pfn;
	initial_capacity = array->initial_capacity;
	
	/* de-init the array and clean up all the memory except for
	 * the array struct itself */
	array_deinitialize( array );
	
	/* clear out the array struct */
	memset(array, 0, sizeof(array_t));

	/* reinitialize the array with the saved params */
	array_initialize( array, initial_capacity, pfn );
}

