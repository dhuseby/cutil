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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#ifdef USE_THREADING
#include <pthread.h>
#endif

#include "debug.h"
#include "macros.h"
#include "btree.h"

typedef struct node_s
{
	void * key;					/* key */
	void * val;					/* value */
	struct node_s * parent;		/* parent pointer */
	struct node_s * left;		/* left child */
	struct node_s * right;		/* right child */
	struct node_s * next;		/* free/used list pointer */
} node_t;

/* the binary tree structure */
struct bt_s
{
	/* callbacks */
	bt_key_cmp_fn		kcfn;		/* key compare function */
	bt_delete_fn		kdfn;		/* key delete function */
	bt_delete_fn		vdfn;		/* value delete function */

	/* memory management */
	node_t**			node_list;	/* memory for the nodes */
	node_t*				free_list;	/* list of free nodes */
	uint_t				num_lists;	/* number of blocks allocated */
	uint_t				list_size;	/* equal to initial capacity, size blocks in node_list */

	/* binary tree */
	node_t*				tree;		/* pointer to btree root */
	uint_t				size;		/* number of nodes in the tree */

#ifdef USE_THREADING
	pthread_mutex_t		lock;		/* btree lock */
#endif
};


/* the default key compare function */
static int default_key_cmp( void * l, void * r )
{
	if ((uint_t)l < (uint_t)r)
		return -1;
	else if ((uint_t)l == (uint_t)r)
		return 0;
	return 1;
}


#ifdef USE_THREADING
void bt_lock(bt_t * const btree)
{
	CHECK_PTR(btree);

	/* lock the mutex */
	pthread_mutex_lock(&btree->lock);
}


int bt_try_lock(bt_t * const btree)
{
	CHECK_PTR_RET(btree, -1);

	/* try to lock the mutex */
	return pthread_mutex_trylock(&btree->lock);
}


void bt_unlock(bt_t * const btree)
{
	CHECK_PTR(btree);

	/* lock the mutex */
	pthread_mutex_unlock(&btree->lock);
}


pthread_mutex_t * bt_get_mutex(bt_t * const btree)
{
	CHECK_PTR_RET(btree, NULL);
	
	/* return the mutex pointer */
	return &btree->lock;	
}
#endif

static void bt_add_more_nodes( bt_t * const btree )
{
	int_t i = 0;
	int_t j = 0;
	CHECK_PTR( btree );

	/* update the number of lists */
	i = btree->num_lists;
	(btree->num_lists)++;

	/* resize the list of pointer */
	btree->node_list = (node_t**)REALLOC(btree->node_list, btree->num_lists * sizeof(node_t*));

	/* allocate the new block of nodes */
	WARN("heap allocating %d nodes\n", btree->list_size);
	btree->node_list[i] = (node_t*)CALLOC(btree->list_size, sizeof(node_t));

	btree->free_list = &(btree->node_list[i][0]);
	for ( j = 0; j < (btree->list_size - 1); ++j )
	{
		btree->node_list[i][j].next = &(btree->node_list[i][j+1]);
	}
	btree->node_list[i][btree->list_size - 1].next = NULL;
}

static void bt_put_node( node_t ** const nlist, node_t * const node )
{
	CHECK_PTR( nlist );
	CHECK_PTR( node );

	/* put the node at the head of the list */
	node->next = (*nlist);
	(*nlist) = node;
}

static node_t * bt_get_node( node_t ** const nlist )
{
	node_t * p;
	CHECK_PTR_RET( nlist, NULL );
	CHECK_PTR_RET( *nlist, NULL );

	p = (*nlist);
	(*nlist) = p->next;
	return p;
}


/* initializes abtree */
static void bt_initialize
(
	bt_t * const btree, 
	uint_t initial_capacity, 
	bt_key_cmp_fn kcfn,
	bt_delete_fn vdfn,
	bt_delete_fn kdfn
)
{
	int_t i = 0;
	CHECK_PTR(btree);

	/* initialize the key compare function */
	if(kcfn != NULL)
		btree->kcfn = kcfn;
	else
		btree->kcfn = default_key_cmp;

	/* initialize the key delete function */
	btree->kdfn = kdfn;
	
	/* initialize the value delete function */
	btree->vdfn = vdfn;

	/* initialize memory management */
	btree->num_lists = 0;
	btree->list_size = initial_capacity;
	btree->node_list = NULL;
	btree->free_list = NULL;

	/* allocate the initial set of nodes */
	bt_add_more_nodes( btree );

	/* set up the binary tree */
	btree->tree = NULL;
	btree->size = 0;

#ifdef USE_THREADING
	/* initialize the mutex */
	pthread_mutex_init(&btree->lock, 0);
#endif
}


/* dynamically allocates and initializes a btree */
/* NOTE: If NULL is passed in for the bt_key_cmp_fn function, the default
 * key compare function will be used */
bt_t* bt_new
(
	uint_t initial_capacity, 
	bt_key_cmp_fn kcfn, 
	bt_delete_fn vdfn, 
	bt_delete_fn kdfn
)
{
	bt_t* btree = NULL;

	/* allocate a btree struct */
	WARN("heap allocating bt struct\n");
	btree = (bt_t*)CALLOC(1, sizeof(bt_t));
	CHECK_PTR_RET(btree, NULL);

	/* initialize the btree */
	bt_initialize(btree, initial_capacity, kcfn, vdfn, kdfn);

	/* return the new btree */
	return btree;
}


/* deinitialize a btree. */
static void bt_deinitialize(bt_t * const btree)
{
    int_t ret = 0;
	int_t i = 0;
	int_t j = 0;
	node_t * p = NULL;
	CHECK_PTR(btree);

	for ( i = 0; i < btree->num_lists; ++i )
	{
		for ( j = 0; j < btree->list_size; ++j )
		{
			p = &(btree->node_list[i][j]);

			if ( (btree->kdfn != NULL) && (p->key != NULL) )
				(*(btree->kdfn))(p->key);

			if ( (btree->vdfn != NULL) && (p->val != NULL) )
				(*(btree->vdfn))(p->val);
		}

		/* free the node block */
		FREE( btree->node_list[i] );
	}

	/* free the list of node block */
	FREE( btree->node_list );

	btree->free_list = NULL;
	btree->tree = NULL;
	btree->size = 0;
	btree->num_lists = 0;

#ifdef USE_THREADING
	/* destroy the lock */
	ret = pthread_mutex_destroy(&btree->lock);
	ASSERT(ret == 0);
#endif
}


/* deinitializes and frees a btree allocated with bt_new() */
void bt_delete(bt_t * const btree)
{
	CHECK_PTR(btree);

	/* de-init the btree if needed */
	bt_deinitialize(btree);

	/* free the btree */
	FREE(btree);
}


/* returns the number of key/value pairs stored in the btree */
uint_t bt_size(bt_t * const btree)
{
	CHECK_PTR_RET(btree, 0);

	return btree->size;
}


static void bt_insert_node( bt_t * const btree,
							void * const key,
							void * const value )
{
	node_t ** p;
	node_t * n;
	node_t * parent;
	CHECK_PTR( btree );
	CHECK_PTR( key );
	CHECK_PTR( value );
	CHECK_PTR( btree->kcfn );

	/* start at the root */
	p = &btree->tree;

	while ( (*p) != NULL )
	{
		if ( (*(btree->kcfn))(key, (*p)->key ) < 0 )
		{
			parent = (*p);
			p = &((*p)->left);
			continue;
		}
		if ( (*(btree->kcfn))(key, (*p)->key ) > 0 )
		{
			parent = (*p);
			p = &((*p)->right);
			continue;
		}
		
		/* keys are the same */
		if ( (btree->vdfn != NULL) && ((*p)->val != NULL) )
		{
			/* delete the old value */
			(*(btree->vdfn))((*p)->val);
		}

		/* store the value */
		(*p)->val = value;

		return;
	}

	/* get a node from the free list */
	n = bt_get_node( &(btree->free_list) );

	/* initialize it */
	n->parent = parent;
	n->left = NULL;
	n->right = NULL;
	n->key = key;
	n->val = key;

	/* add it to the list */
	(*p) = n;

	(btree->size)++;
}


/* adds a key/value pair to the btree. */
/* NOTE: if the number of values stored in the table will exceed a load 
 * factor of 65% then the btree grows to make more room. */
int bt_add( bt_t * const btree, 
			void * const key,
			void * const value )
{
	int_t i = 0;
	CHECK_PTR_RET(btree, 0);
	CHECK_PTR_RET(key, 0);
	CHECK_PTR_RET(value, 0);

	/* are we out of free nodes? */
	if ( btree->free_list == NULL )
	{
		bt_add_more_nodes( btree );
	}

	/* add it to the btree */
	bt_insert_node( btree, key, value );

	/* success */
	return 1;
}


static node_t * bt_find_node( bt_t * const btree, void * const key )
{
	node_t * p;
	CHECK_PTR_RET(btree, NULL);
	CHECK_PTR_RET(key, NULL);
	CHECK_PTR_RET( btree->kcfn, NULL );

	p = btree->tree;

	while( p != NULL )
	{
		if ( (*(btree->kcfn))( key, p->key ) < 0 )
		{
			p = p->left;
			continue;
		}
		if ( (*(btree->kcfn))( key, p->key ) > 0 )
		{
			p = p->right;
			continue;
		}
		break;
	}
	return p;
}


/* find a value by it's key. */
void * bt_find(bt_t * const btree, void * const key )
{
	node_t * p = NULL;
	CHECK_PTR_RET(btree, NULL);
	CHECK_PTR_RET(key, NULL);

	p = bt_find_node( btree, key );
	if ( p != NULL )
		return p->val;

	return NULL;
}


/* remove the value associated with the key from the btree */
void * bt_remove(bt_t * const btree, void * const key )
{
	void * val = NULL;
	node_t * p = NULL;
	CHECK_PTR_RET(btree, NULL);
	CHECK_PTR_RET(key, NULL);

	/* look up the node */
	p = bt_find_node( btree, key );

	/* lazy delete it's value, but leave it in the tree
	 * to save time...*/
	if ( p != NULL )
	{
		/* get the value pointer to pass back */
		val = p->val;
		p->val = NULL;
	}

	return val;
}

static const bt_itr_t itr_begin = { 0, 0 };
static const bt_itr_t itr_end = { -1, -1 };


bt_itr_t bt_itr_begin(bt_t const * const btree)
{
	CHECK_PTR_RET(btree, itr_end);

	return itr_begin;
}


bt_itr_t bt_itr_next(bt_t const * const btree, bt_itr_t const itr)
{
	bt_itr_t itmp = itr;
	CHECK_PTR_RET(btree, itr_end);

	for(;;) 
	{
		itmp.j++;

		if ( itmp.j == btree->list_size )
		{
			itmp.i++;
			itmp.j = 0;
		}

		if ( itmp.i == btree->num_lists )
			break;

		if ( btree->node_list[itmp.i][itmp.j].val != NULL )
			return itmp;
	}

	return itr_end;
}

bt_itr_t bt_itr_end(bt_t const * const btree)
{
	return itr_end;
}

void* bt_itr_get(bt_t const * const btree, bt_itr_t const itr)
{
	CHECK_PTR_RET(btree, NULL);
	if ( (itr.i == -1) && (itr.j == -1) )
		return NULL;

	return btree->node_list[itr.i][itr.j].val;
}

