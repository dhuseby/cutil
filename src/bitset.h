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

#ifndef __BITSET_H__
#define __BITSET_H__

#include <stdint.h>

typedef struct bitset_s
{
    size_t num_bits;
    uint32_t * bits;
} bitset_t;


bitset_t * bset_new( size_t const num_bits );
void bset_delete( void * bset );

int_t bset_initialize( bitset_t * const bset, size_t const num_bits );
int_t bset_deinitialize( bitset_t * const bset );

int_t bset_set( bitset_t * const bset, size_t const bit );
int_t bset_clear( bitset_t * const bset, size_t const bit );
int_t bset_test( bitset_t const * const bset, size_t const bit );

int_t bset_clear_all( bitset_t * const bset );
int_t bset_set_all( bitset_t * const bset );

#endif /*__BITSET_H__*/

