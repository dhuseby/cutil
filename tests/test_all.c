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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include <CUnit/Basic.h>

#include <cutil/debug.h>
#include <cutil/macros.h>
#include <cutil/cb.h>
#include <cutil/events.h>

#include "test_macros.h"
#include "test_flags.h"

#if 0
SUITE( bitset );
SUITE( btree );
SUITE( buffer );
SUITE( child );
SUITE( privileges );
SUITE( sanitize );
#endif

SUITE( aiofd );
SUITE( cb );
SUITE( events );
SUITE( hashtable );
SUITE( list );
SUITE( pair );
SUITE( socket );

evt_loop_t * el = NULL;

int main()
{
  /* initialize the CUnit test registry */
  if ( CUE_SUCCESS != CU_initialize_registry() )
    return CU_get_error();

  /* add each suite of tests */
#if 0
  ADD_SUITE( bitset );
  ADD_SUITE( btree );
  ADD_SUITE( buffer );
  ADD_SUITE( child );
  ADD_SUITE( privileges );
  ADD_SUITE( sanitize );
#endif

  ADD_SUITE( aiofd );
  ADD_SUITE( cb );
  ADD_SUITE( events );
  ADD_SUITE( hashtable );
  ADD_SUITE( list );
  ADD_SUITE( pair );
  ADD_SUITE( socket );

  /* set up the event loop */
  el = evt_new();
  CHECK_PTR_RET( el, 0 );

  /* run all tests using the CUnit Basic interface */
  CU_basic_set_mode( CU_BRM_VERBOSE );
  CU_basic_run_tests();

  /* clean up */
  CU_cleanup_registry();

  /* clean up the event loop */
  evt_delete( el );

  return CU_get_error();
}

