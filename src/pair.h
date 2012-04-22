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

#ifndef __PAIR_H__
#define __PAIR_H__

#include <stdint.h>

/* opaque handle for the pair */
typedef struct pair_s pair_t;

pair_t * pair_new( void * first, void * second );
void pair_delete( void * p );

void * pair_first( pair_t const * const pair );
void * pair_second( pair_t const * const pair );

#endif/*__PAIR_H__*/

