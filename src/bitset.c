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
#include "bitset.h"

#if defined(UNIT_TESTING)
#include "test_flags.h"
#endif

#define DWORDS_NEEDED(x) ((x + 0x1f) & ~0x1f)
#define DWORD_INDEX(x) (x & ~0x1f)
#define BIT(x) ((uint32_t)(1 << (x & 0x1f)))

bitset_t * bset_new( size_t const num_bits )
{
	bitset_t * bset = NULL;
	CHECK_RET( num_bits > 0, NULL );

	bset = (bitset_t*)CALLOC(1, sizeof(bitset_t));
	CHECK_PTR_RET( bset, NULL );

	if ( !bset_initialize( bset, num_bits ) )
	{
		FREE( bset );
		bset = NULL;
	}

	return bset;
}

void bset_delete( void * bset )
{
	bitset_t * bitset = (bitset_t*)bset;
	CHECK_PTR( bitset );
	bset_deinitialize( bitset );
	FREE( bitset );
}

int bset_initialize( bitset_t * const bset, size_t const num_bits )
{
#if defined(UNIT_TESTING)
	CHECK_RET( !fail_bitset_init, FALSE );
#endif
	CHECK_PTR_RET( bset, FALSE );

	bset->bits = NULL;
	if ( num_bits > 0 )
	{
		bset->bits = CALLOC( DWORDS_NEEDED( num_bits ), sizeof(uint32_t) );
		CHECK_PTR_RET( bset->bits, FALSE );
	}
	bset->num_bits = num_bits;
	return TRUE;
}

int bset_deinitialize( bitset_t * const bset )
{
#if defined(UNIT_TESTING)
	CHECK_RET( !fail_bitset_deinit, FALSE );
#endif
	CHECK_PTR_RET( bset, FALSE );
	CHECK_RET( bset->num_bits > 0, FALSE );
	CHECK_PTR_RET( bset->bits, FALSE );

	FREE( bset->bits );
	bset->bits = NULL;
	bset->num_bits = 0;
	return TRUE;
}

int bset_set( bitset_t * const bset, size_t const bit )
{
	CHECK_PTR_RET( bset, FALSE );
	CHECK_RET( (bit < bset->num_bits), FALSE );
	bset->bits[ DWORD_INDEX(bit) ] |= BIT(bit);
	return TRUE;
}

int bset_clear( bitset_t * const bset, size_t const bit )
{
	CHECK_PTR_RET( bset, FALSE );
	CHECK_RET( (bit < bset->num_bits), FALSE );
	bset->bits[ DWORD_INDEX(bit) ] &= ~BIT(bit);
	return TRUE;
}

int bset_test( bitset_t const * const bset, size_t const bit )
{
	CHECK_PTR_RET( bset, FALSE );
	CHECK_RET( (bit < bset->num_bits), FALSE );
	return (bset->bits[ DWORD_INDEX(bit) ] & BIT(bit) ? TRUE : FALSE);
}

int bset_clear_all( bitset_t * const bset )
{
	size_t i;
	CHECK_PTR_RET( bset, FALSE );
	CHECK_RET( bset->num_bits > 0, FALSE );
	CHECK_PTR_RET( bset->bits, FALSE );
	for ( i = 0; i < DWORDS_NEEDED( bset->num_bits ); i++ )
	{
		bset->bits[i] = 0;
	}
	return TRUE;
}

int bset_set_all( bitset_t * const bset )
{
	size_t i;
	CHECK_PTR_RET( bset, FALSE );
	CHECK_RET( bset->num_bits > 0, FALSE );
	CHECK_PTR_RET( bset->bits, FALSE );
	for ( i = 0; i < DWORDS_NEEDED( bset->num_bits ); i++ )
	{
		bset->bits[i] = 0xFFFFFFFF;
	}
	return TRUE;
}

#if defined(UNIT_TESTING)

#include <CUnit/Basic.h>

void test_bitset_private_functions(void)
{
}

#endif


