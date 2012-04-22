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

#include "debug.h"
#include "macros.h"
#include "pair.h"

struct pair_s
{
	void * first;
	void * second;
};

pair_t * pair_new( void * first, void * second )
{
	pair_t * pair = CALLOC( 0, sizeof(pair_t) );
	CHECK_PTR_RET( pair, NULL );

	pair->first = first;
	pair->second = second;
	return pair;
}

void pair_delete( void * p )
{
	pair_t * pair = (pair_t*)p;
	CHECK_PTR( pair );

	FREE( pair );
}

void * pair_first( pair_t const * const pair )
{
	CHECK_PTR_RET( pair, NULL );
	return pair->first;
}

void * pair_second( pair_t const * const pair )
{
	CHECK_PTR_RET( pair, NULL );
	return pair->second;
}

