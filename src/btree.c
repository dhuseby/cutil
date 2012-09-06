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
#include <inttypes.h>
#ifdef USE_THREADING
#include <pthread.h>
#endif

#include "debug.h"
#include "macros.h"
#include "btree.h"

#if defined(UNIT_TESTING)
#include "test_flags.h"
#endif

#define DEFAULT_INITIAL_CAPACITY (16)
#define min(x, y) ((x < y) ? x : y)
#define max(x, y) ((x > y) ? x : y)


typedef struct node_s
{
	void * key;					/* key */
	void * val;					/* value */
	int32_t balance;			/* balance factor */
	struct node_s * parent;		/* parent pointer */
	struct node_s * left;		/* left child */
	struct node_s * right;		/* right child */
	struct node_s * next;		/* traversal threading pointer/free list pointer */
	struct node_s * prev;		/* traversal threading pointer */
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
	node_t **p = NULL;
	CHECK_PTR( btree );
	CHECK_MSG( (btree->free_list == NULL), "adding more nodes when free list isn't empty\n" );

	/* update the number of lists */
	i = btree->num_lists;
	(btree->num_lists)++;

	/* resize the list of pointer */
	p = (node_t**)REALLOC(btree->node_list, btree->num_lists * sizeof(node_t*));
	CHECK_PTR_MSG( p, "realloc failed\n" );

	/* store the new buffer */
	btree->node_list = p;

	/* allocate the new block of nodes */
	btree->node_list[i] = (node_t*)CALLOC(btree->list_size, sizeof(node_t));
	CHECK_PTR_MSG( btree->node_list[i], "block allocation failed\n" );

	/* link up the new nodes into a free list */
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

	/* take a node from the head of the list */
	p = (*nlist);
	(*nlist) = p->next;
	p->next = NULL;
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
	btree->list_size = ( (initial_capacity > 0) ? initial_capacity : DEFAULT_INITIAL_CAPACITY );
	btree->node_list = NULL;
	btree->free_list = NULL;

	/* allocate the initial set of nodes */
	bt_add_more_nodes( btree );
	CHECK_PTR_MSG( btree->free_list, "failed to allocate more nodes\n" );

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
void bt_delete(void * bt)
{
	bt_t * btree = (bt_t*)bt;
	CHECK_PTR(btree);

	/* de-init the btree if needed */
	bt_deinitialize(btree);

	/* free the btree */
	FREE((void*)btree);
}


/* returns the number of key/value pairs stored in the btree */
uint_t bt_size(bt_t * const btree)
{
	CHECK_PTR_RET(btree, 0);

	return btree->size;
}

static int bt_is_left_child( node_t * p, node_t * n )
{
	CHECK_PTR_RET( p, FALSE );
	CHECK_PTR_RET( n, FALSE );
	
	if ( p->left == n )
		return TRUE;
	
	return FALSE;
}

static node_t * bt_rotate_left( node_t * const p )
{
	/* if p's balance is ++ and n's balance is + we have this situation:
	 *
	 *       R
	 *       |
	 *       p++
	 *      / \
	 *     a   n+
	 *        / \
	 *       b   c
	 *
	 * where height(c) > height(b). we rotate to the left to get this:
	 *
	 *       R
	 *       |
	 *       n
	 *      / \
	 *     p   c
	 *    / \
	 *   a   b
	 *
	 * after the rotation n and p will have balance 0.
	 */
	int left;
	node_t * R = p->parent;
	node_t *n, *b;
	CHECK_PTR_RET( p, p );
	CHECK_PTR_RET( p->right, p ); /* p must have a right sub-tree */
	CHECK_RET( (p->balance == 2), p );
	CHECK_RET( (p->right->balance == 1), p );

	/* is p the left child of R? */
	if ( R != NULL )
		left = bt_is_left_child( R, p );

	/* initialize the pointers */
	n = p->right;
	b = n->left;

	/* move b to be p's right child */
	p->right = b;
	if ( b != NULL )
		b->parent = p;

	/* move p to be n's left child */
	n->left = p;
	p->parent = n;

	/* if needed, make n a child of R */
	if ( R != NULL )
	{
		if ( left )
			R->left = n;
		else
			R->right = n;
		n->parent = R;
	}
	else
		n->parent = NULL;

	/* update balance factors */
	n->balance = 0;
	p->balance = 0;

	return n;
}

static node_t * bt_rotate_right( node_t * const p )
{
	/* if p's balance is -- and n's balance is - we have this situation:
	 *
	 *       R
	 *       |
	 *       p--
	 *      / \
	 *     n-  c
	 *    / \
	 *   a   b
	 *
	 * where height(a) > height(b). we rotate to the right to get this:
	 *
	 *		 R
	 *		 |
	 *       n
	 *      / \
	 *     a   p
	 *        / \
	 *       b   c
	 *
	 * after the rotation n and p will have balance 0.
	 */

	int left;
	node_t * R = p->parent;
	node_t *n, *b;
	CHECK_PTR_RET( p, p );
	CHECK_PTR_RET( p->left, p ); /* p must have a left sub-tree */
	CHECK_RET( (p->balance == -2), p );
	CHECK_RET( (p->left->balance == -1), p );

	/* is p the left child of R? */
	if ( R != NULL )
		left = bt_is_left_child( R, p );

	/* initialize the pointers */
	n = p->left;
	b = n->right;

	/* move b to be p's left child */
	p->left = b;
	if ( b != NULL )
		b->parent = p;

	/* move p to be n's right child */
	n->right = p;
	p->parent = n;

	/* if needed, make n a child of R */
	if ( R != NULL )
	{
		if ( left )
			R->left = n;
		else
			R->right = n;
		n->parent = R;
	}
	else
		n->parent = NULL;

	/* update balance factors */
	n->balance = 0;
	p->balance = 0;

	return n;
}

static node_t * bt_rotate_left_right( node_t * const p )
{
	/* if p's balance is -- and n's balance is +, g's balance can be -, 0, or +,
	 * and we have this situation:
	 *
	 *       R
	 *       |
	 *       p--
	 *      / \
	 *     n+  d
	 *    / \
	 *   a   g?
	 *      / \
	 *     b   c
	 *
	 * first we do a left rotation to get this:
	 *
	 *       R
	 *       |
	 *       p--
	 *      / \
	 *     g?  d
	 *    / \
	 *   n+  c
	 *  / \
	 * a   b
	 *
	 * then we do a right rotation to get this:
	 *
	 *       R
	 *       |
	 *       g
	 *     /   \
	 *    n     p
	 *   / \   / \
	 *  a   b c   d
	 *
	 *  and g will have a balance of 0 and the balance values for n and p depend
	 *  on the starting balance of g.  the balance values follow the following
	 *  table:
	 *
	 *  |  g  |  n  |  p  |
	 *  +=====+=====+=====+
	 *  |  0  |  0  |  0  |
	 *  |  -  |  0  |  +  |
	 *  |  +  |  -  |  0  |
	 */
	int left;
	node_t * R = p->parent;
	node_t *g, *n, *b, *c;
	CHECK_PTR_RET( p, p );
	CHECK_PTR_RET( p->left, p ); /* p must have a left sub-tree */
	CHECK_PTR_RET( p->left->right, p ); /* p must also have a left-right sub-tree */
	CHECK_RET( (p->balance == -2), p );
	CHECK_RET( (p->left->balance == 1), p );

	/* is p the left child of R? */
	if ( R != NULL )
		left = bt_is_left_child( R, p );

	/* initialize the pointers */
	n = p->left;
	g = n->right;
	b = g->left;
	c = g->right;

	/* move b to be n's right child */
	n->right = b;
	if ( b != NULL )
		b->parent = n;

	/* move c to be p's left child */
	p->left = c;
	if ( c != NULL )
		c->parent = p;

	/* move n to be g's left child */
	g->left = n;
	n->parent = g;

	/* move p to be g's right child */
	g->right = p;
	p->parent = g;

	/* if needed, make g a child of R */
	if ( R != NULL )
	{
		if ( left )
			R->left = g;
		else
			R->right = g;
		g->parent = R;
	}
	else
		g->parent = NULL;

	/* update balance factors */
	if ( g->balance == 0 )
		n->balance = p->balance = 0;
	else if ( g->balance == -1 )
	{
		n->balance = 0;
		p->balance = 1;
	}
	else /* g->balance == 1 */
	{
		n->balance = -1;
		p->balance = 0;
	}
	g->balance = 0;

	return g;
}

static node_t * bt_rotate_right_left( node_t * const p )
{
	/* if p's balance is ++ and n's balance is -, g's balance can be -, 0, or +,
	 * and we have this situation:
	 *
	 *       R
	 *       |
	 *       p++
	 *      / \
	 *     a   n-
	 *        / \
	 *       g?  d
	 *      / \
	 *     b   c
	 *
	 * first we do a right rotation to get this:
	 *
	 *       R
	 *       |
	 *       p--
	 *      / \
	 *     a   g?
	 *        / \
	 *       b   n+
	 *          / \
	 *         c   d
	 *
	 * then we do a left rotation to get this:
	 *
	 *       R
	 *       |
	 *       g
	 *     /   \
	 *    p     n
	 *   / \   / \
	 *  a   b c   d
	 *
	 *  and g will have a balance of 0 and the balance values for n and p depend
	 *  on the starting balance of g.  the balance values follow the following
	 *  table:
	 *
	 *  |  g  |  n  |  p  |
	 *  +=====+=====+=====+
	 *  |  0  |  0  |  0  |
	 *  |  -  |  +  |  0  |
	 *  |  +  |  0  |  -  |
	 */
	int left;
	node_t * R = p->parent;
	node_t *g, *n, *b, *c;
	CHECK_PTR_RET( p, p );
	CHECK_PTR_RET( p->right, p ); /* p must have a right sub-tree */
	CHECK_PTR_RET( p->right->left, p ); /* p must also have a right-left sub-tree */
	CHECK_RET( (p->balance == 2), p );
	CHECK_RET( (p->right->balance == -1), p );

	/* is p the left child of R? */
	if ( R != NULL )
		left = bt_is_left_child( R, p );

	/* initialize pointers */
	n = p->right;
	g = n->left;
	b = g->left;
	c = g->right;

	/* move b to be p's right child */
	p->right = b;
	if ( b != NULL )
		b->parent = p;

	/* move c to be n's left child */
	n->left = c;
	if ( c != NULL )
		c->parent = n;

	/* move p to be g's left child */
	g->left = p;
	p->parent = g;

	/* move n to be g's right child */
	g->right = n;
	n->parent = g;

	/* if needed, make g a child of R */
	if ( R != NULL )
	{
		if ( left )
			R->left = g;
		else
			R->right = g;
		g->parent = R;
	}
	else
		g->parent = NULL;

	/* update balance factors */
	if ( g->balance == 0 )
		n->balance = p->balance = 0;
	else if ( g->balance == -1 )
	{
		n->balance = 1;
		p->balance = 0;
	}
	else /* g->balance == 1 */
	{
		n->balance = 0;
		p->balance = -1;
	}
	g->balance = 0;

	return g;
}


static node_t * bt_balance_tree( bt_t * const btree, node_t * n )
{
	int left_child = FALSE;
	int update_root = FALSE;
	node_t * p;
	CHECK_PTR_RET( n, NULL );
	
	p = n->parent;

	while( p != NULL )
	{
		/* is n the left child of p? */
		left_child = bt_is_left_child( p, n );

		/* case 1: p had balance factor of 0 */
		if ( p->balance == 0 )
		{
			/* tip balance left or right */
			p->balance = left_child ? -1 : 1;

			/* height changed so propagate upwards */
			n = p;
			p = n->parent;
		}

		/* case 2: p's shorter subtree has increased in height */
		else if ( ((p->balance < 0) && (left_child == FALSE)) ||
				  ((p->balance > 0) && (left_child == TRUE)) )
		{
			p->balance = left_child ? (p->balance - 1) : (p->balance + 1);

			/* the subtree's should be balanced in height now */
			ASSERT( p->balance == 0 );

			/* since the subtree rooted at p didn't change height, we don't need
			 * to propagate upwards */
			return p;
		}

		/* case 3: p's taller subtree has increased in height */
		else if ( ((p->balance < 0) && (left_child == TRUE)) ||
				  ((p->balance > 0) && (left_child == FALSE)) )
		{
			/* we're about to do rotations so we need to know if we need to update
			 * the btree's root pointer */
			update_root = (p->parent == NULL) ? TRUE : FALSE;

			/* there are four possibilities here depending on the balance of p and n.
			 * p's new balance factor should be either -- or ++ depending on whether
			 * n is p's left child or not. */
			p->balance = left_child ? (p->balance - 1) : (p->balance + 1);
			ASSERT( ((p->balance == -2) || (p->balance == 2)) );
			ASSERT( ((n->balance == -1) || (n->balance == 1)) );

			if ( (p->balance == -2) && (n->balance == -1) )
			{
				p = bt_rotate_right( p );
			}
			else if ( (p->balance == -2) && (n->balance == 1) )
			{
				p = bt_rotate_left_right( p );
			}
			else if ( (p->balance == 2) && (n->balance == -1) )
			{
				p = bt_rotate_right_left( p );
			}
			else if ( (p->balance == 2) && (n->balance == 1) )
			{
				p = bt_rotate_left( p );
			}

			if ( update_root )
				btree->tree = p;

			return p;
		}
	}

	return n;
}


static int bt_insert_node( bt_t * const btree,
						   void * const key,
						   void * const value )
{
	node_t ** p;
	node_t * n;
	node_t * parent = NULL;
	node_t * inorder_successor = NULL;
	node_t * inorder_predecessor = NULL;
	int left;
	CHECK_PTR_RET( btree, FALSE );
	CHECK_PTR_RET( key, FALSE );
	CHECK_PTR_RET( value, FALSE );
	CHECK_PTR_RET( btree->kcfn, FALSE );

	/* start at the root */
	p = &btree->tree;

	/* find the pointer to where the new node should go */
	while ( (*p) != NULL )
	{
		if ( (*(btree->kcfn))(key, (*p)->key ) < 0 )
		{
			parent = (*p);
			inorder_successor = parent;
			p = &((*p)->left);
			continue;
		}
		else if ( (*(btree->kcfn))(key, (*p)->key ) > 0 )
		{
			parent = (*p);
			inorder_predecessor = parent;
			p = &((*p)->right);
			continue;
		}
		else
		{
			/* duplicate key found */
			return FALSE;
		}
	}

	/* get a node from the free list */
	n = bt_get_node( &(btree->free_list) );
	CHECK_PTR_MSG( n, "failed to get a node to store a new value\n" );

	/* initialize it */
	n->parent = parent;
	n->left = NULL;
	n->right = NULL;
	n->next = inorder_successor; /* threading for faster traversal */
	n->prev = inorder_predecessor; /* threading for faster traversal */
	n->key = key;
	n->val = value;
	n->balance = 0;

	/* add it to the tree */
	(*p) = n;
	(btree->size)++;

	/* fix successor's prev pointer */
	if ( inorder_successor != NULL )
		inorder_successor->prev = n;

	if ( inorder_predecessor != NULL )
		inorder_predecessor->next = n;

	/* re-balance the tree */
	bt_balance_tree( btree, n );

	return TRUE;
}


/* adds a key/value pair to the btree. */
/* NOTE: if the number of values stored in the table will exceed a load 
 * factor of 65% then the btree grows to make more room. */
int bt_add( bt_t * const btree, 
			void * const key,
			void * const value )
{
	int_t i = 0;
	CHECK_PTR_RET(btree, FALSE);
	CHECK_PTR_RET(key, FALSE);
	CHECK_PTR_RET(value, FALSE);

	/* are we out of free nodes? */
	if ( btree->free_list == NULL )
	{
		bt_add_more_nodes( btree );
	}
	CHECK_PTR_RET_MSG( btree->free_list, FALSE, "failed to allocate more nodes\n" );

	/* add it to the btree */
	return bt_insert_node( btree, key, value );
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

static node_t * bt_find_tree_min( node_t * p )
{
	CHECK_PTR_RET( p, NULL );

	while( p->left != NULL )
	{
		p = p->left;
	}

	return p;
}

static node_t * bt_find_tree_max( node_t * p )
{
	CHECK_PTR_RET( p, NULL );

	while( p->right != NULL )
	{
		p = p->right;
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

static node_t * bt_replace_node( node_t * p, node_t * s )
{
	CHECK_PTR_RET( p, NULL );

	/* make p's parent point to s */
	if ( p->parent != NULL )
	{
		if ( p->parent->left == p )
		{
			p->parent->left = s;
		}
		else
		{
			p->parent->right = s;
		}
	}

	/* make s point back to p's parent */
	if ( s != NULL )
	{
		s->parent = p->parent;

		/* fix up prev/next pointers */
		if ( p->prev != NULL )
		{
			s->prev = p->prev;
			s->prev->next = s;
		}
		if ( p->next != NULL )
		{
			s->next = p->next;
			s->next->prev = s;
		}
	}
	else
	{
		/* replacing the node with NULL so make the node's prev's next point
		 * to the node's next and the node's next's prev point to the node's
		 * prev. */
		if ( p->prev != NULL )
		{
			p->prev->next = p->next;
		}
		if ( p->next != NULL )
		{
			p->next->prev = p->prev;
		}
	}

	/* clear out p's pointers */
	p->parent = NULL;
	p->left = NULL;
	p->right = NULL;
	p->next = NULL;
	p->prev = NULL;

	return p;
}


/* remove the value associated with the key from the btree */
void * bt_remove(bt_t * const btree, void * const key )
{
	void * val = NULL;
	node_t * p = NULL;
	node_t * s = NULL;
	CHECK_PTR_RET(btree, NULL);
	CHECK_PTR_RET(key, NULL);

	/* look up the node */
	p = bt_find_node( btree, key );

	CHECK_PTR_RET( p, NULL );

	/* case 1: p has no right child */
	if ( p->right == NULL )
	{
		/* replace p with p's left child */
		p = bt_replace_node( p, p->left );
	}

	/* case 2: p's right child has no left child */
	else if ( p->right->left == NULL )
	{
		/* attach p's left child as the left child of p's right child */
		p->right->left = p->left;
		p->left->parent = p->right;

		/* replace p with p's right child */
		p = bt_replace_node( p, p->right );
	}

	/* case 3: p's right child has a left child */
	else
	{
		/* find the in-order successor (the node with minimum value in right subtree) */
		s = bt_find_tree_min( p->right );

		/* the in-order successor has some interesting properties:
		 * - it exists
		 * - it cannot have a left child */
		
		/* replace s with s's right child */
		s = bt_replace_node( s, s->right );

		/* now replace p with s */
		p = bt_replace_node( p, s );
	}


	/* get the value pointer to pass back */
	val = p->val;
	p->val = NULL;
	p->next = NULL;

	/* clean up the node and put the node back on the free list */
	if ( (btree->kdfn != NULL) && (p->key != NULL) )
		(*(btree->kdfn))(p->key);
	p->key = NULL;
	bt_put_node( &(btree->free_list), p );

	return val;
}

void bt_print_node( node_t * const p, int const indent )
{
	CHECK_PTR( p );

	bt_print_node( p->right, indent + 1 );

	/* print the node */
#if defined(PORTABLE_64_BIT)
	printf( "%*s%d(%" PRId64 ")\n", (indent * 5), " ", p->balance, (int_t)p->val );
#else
	printf( "%*s%d(%" PRId32 ")\n", (indent * 5), " ", p->balance, (int_t)p->val );
#endif

	bt_print_node( p->left, indent + 1 );
}

void bt_print( bt_t * const btree )
{
	CHECK_PTR( btree );
	CHECK_PTR( btree->tree );

	bt_print_node( btree->tree, 1 );
}


static const bt_itr_t itr_end = NULL;

bt_itr_t bt_itr_begin(bt_t const * const btree)
{
	CHECK_PTR_RET( btree, itr_end );

	return bt_find_tree_min( btree->tree );
}


bt_itr_t bt_itr_next(bt_t const * const btree, bt_itr_t const itr)
{
	node_t * p = (node_t*)itr;
	CHECK_PTR_RET( btree, itr_end );
	CHECK_PTR_RET( p, itr_end );

	/* return the next node in the list */
	return p->next;
}


bt_itr_t bt_itr_end(bt_t const * const btree)
{
	return itr_end;
}


bt_itr_t bt_itr_rbegin(bt_t const * const btree)
{
	CHECK_PTR_RET( btree, itr_end );

	return bt_find_tree_max( btree->tree );
}


bt_itr_t bt_itr_rnext(bt_t const * const btree, bt_itr_t const itr)
{
	node_t * p = (node_t*)itr;
	CHECK_PTR_RET( btree, itr_end );
	CHECK_PTR_RET( p, itr_end );

	/* return the next node in the list */
	return p->prev;
}


bt_itr_t bt_itr_rend(bt_t const * const btree)
{
	return itr_end;
}


void* bt_itr_get(bt_t const * const btree, bt_itr_t const itr)
{
	node_t * p = (node_t*)itr;
	CHECK_PTR_RET( btree, NULL );
	CHECK_RET( (itr != itr_end), NULL );

	return p->val;
}

void* bt_itr_get_key(bt_t const * const btree, bt_itr_t const itr)
{
	node_t * p = (node_t*)itr;
	CHECK_PTR_RET( btree, NULL );
	CHECK_RET( (itr != itr_end), NULL );

	return p->key;
}

#if defined(UNIT_TESTING)

#include <CUnit/Basic.h>

void test_btree_private_functions( void )
{
}

#endif


