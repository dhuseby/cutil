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

#include <stdint.h>
#include <stdarg.h>

#define DEBUG_ON

#include "debug.h"
#include "macros.h"
#include "list.h"
#include "hashtable.h"
#include "pair.h"
#include "cb.h"

#if defined(UNIT_TESTING)
#include "test_flags.h"
#endif

struct cb_s
{
  ht_t*   ht;
#ifdef DEBUG
  uint64_t cb_calls;
#endif
};

/* forward declare the helper functions */
static int_t cb_init(cb_t *cb);
static void cb_deinit(cb_t *cb);
static pair_t* find_bucket(ht_t * ht, uint8_t const * const name);
static list_itr_t get_cb_itr(list_t * l, void * ctx, cbfn fn);
static pair_t* find_cb(list_t * l, void * ctx, cbfn fn);
static int_t remove_cb(list_t * l, void * ctx, cbfn fn);

static uint_t cb_hash_fn(void const * const key);
static int_t cb_match_fn(void const * const l, void const * const r);
static void cb_delete_fn(void * p);

cb_t* cb_new(void)
{
  cb_t* cb = NULL;

  /* allocate the session struct */
  cb = CALLOC(1, sizeof(cb_t));
  CHECK_PTR_RET(cb, NULL);

  CHECK_GOTO(cb_init(cb), _cb_new_fail);

  return cb;

_cb_new_fail:
  FREE(cb);
  return NULL;
}

void cb_delete(void * p)
{
  cb_t* cb = (cb_t*)p;
  CHECK_PTR(cb);
  cb_deinit(cb);
  FREE(cb);
}

int_t cb_has(cb_t * cb, uint8_t const * const name)
{
  pair_t * bkt = NULL;
  list_t * l = NULL;
  CHECK_PTR_RET(cb, FALSE);
  CHECK_PTR_RET(name, FALSE);
  bkt = find_bucket(cb->ht, name);
  l = pair_second(bkt);
  return (list_count(l) > 0 ? TRUE : FALSE);
}

int_t cb_add(cb_t * cb, uint8_t const * const name, void * ctx, cbfn fn)
{
  int_t add = FALSE;
  pair_t * bkt = NULL;
  pair_t * addbkt = NULL;
  pair_t * p = NULL;
  pair_t * r = NULL;
  list_t * l = NULL;
  list_t * addl = NULL;
  uint8_t * s = NULL;

  CHECK_PTR_RET(cb, FALSE);
  CHECK_PTR_RET(name, FALSE);
  CHECK_PTR_RET(fn, FALSE);

  bkt = find_bucket(cb->ht, name);
  l = pair_second(bkt);
  p = find_cb(l, ctx, fn);

  /* callback already added */
  if (p != NULL)
    return FALSE;

  /* allocate a new bucket if needed */
  if ((l == NULL) && (bkt == NULL))
  {
    s = UT(STRDUP(C(name)));
    CHECK_GOTO(s, _cb_add_fail1);

    addl = list_new(1, &pair_delete);
    CHECK_GOTO(addl, _cb_add_fail2);
    l = addl;

    addbkt = pair_new(s, l);
    CHECK_GOTO(addbkt, _cb_add_fail3);
    bkt = addbkt;
    add = TRUE;
  }

  r = pair_new(ctx, fn);
  CHECK_GOTO(r, _cb_add_fail4);

  /* add record to the bucket list */
  CHECK_GOTO(list_push_tail(l, r), _cb_add_fail4);

  if (add)
  {
    /* add it to the ht, on fail, goto 3 so we don't double free r */
    CHECK_GOTO(ht_insert(cb->ht, bkt), _cb_add_fail3);
  }

  /* allocate a new pair */
  return TRUE;

_cb_add_fail4:
  pair_delete(r);
_cb_add_fail3:
  pair_delete(addbkt);
_cb_add_fail2:
  list_delete(addl);
_cb_add_fail1:
  FREE(s);
  return FALSE;
}

int_t cb_remove(cb_t * cb, uint8_t const * const name, void * ctx, cbfn fn)
{
  pair_t * bkt = NULL;

  CHECK_PTR_RET(cb, FALSE);
  CHECK_PTR_RET(name, FALSE);
  CHECK_PTR_RET(fn, FALSE);

  bkt = find_bucket(cb->ht, name);
  CHECK_PTR_RET(bkt, FALSE);
  return remove_cb(pair_second(bkt), ctx, fn);
}

int_t cb_call(cb_t * cb, uint8_t const * const name, ...)
{
  va_list args;
  int_t ret = FALSE;
  pair_t * bkt = NULL;
  list_t * l = NULL;
  pair_t * p = NULL;
  list_itr_t itr, end;
  void * ctx = NULL;
  cbfn fn = NULL;

  CHECK_PTR_RET(cb, FALSE);
  CHECK_PTR_RET(name, FALSE);

  bkt = find_bucket(cb->ht, name);
  CHECK_PTR_RET(bkt, FALSE);
  l = pair_second(bkt);
  CHECK_PTR_RET(l, FALSE);
  va_start(args, name);
  itr = list_itr_begin(l);
  end = list_itr_end(l);
  for (; itr != end; itr = list_itr_next(l, itr))
  {
    p = (pair_t*)list_get(l, itr);
    ctx = pair_first(p);
    fn = pair_second(p);
    if (!fn)
      continue;

#if DEBUG
    cb->cb_calls++;
#endif

    /* call the callback */
    (*fn)(ctx, args);
    ret = TRUE;
  }
  va_end(args);

  return ret;
}

/*
 * PRIVATE
 */

static int_t cb_init(cb_t *cb)
{
  UNIT_TEST_N_RET(cb_init);

  /* initialize the session */
  cb->ht = ht_new(1, &cb_hash_fn, &cb_match_fn, &cb_delete_fn);
  CHECK_PTR_RET(cb->ht, FALSE);

  return TRUE;
}

static void cb_deinit(cb_t *cb)
{
  ht_delete(cb->ht);
}



static pair_t* find_bucket(ht_t * ht, uint8_t const * const name)
{
  ht_itr_t itr;
  pair_t * bkt = NULL;
  pair_t * p = NULL;

  CHECK_PTR_RET(ht, NULL);
  CHECK_PTR_RET(name, NULL);

  /* allocate a temporary pair */
  p = pair_new((void*)name, NULL);
  CHECK_PTR_RET(p, NULL);

  /* look up bucket by name */
  itr = ht_find(ht, p);

  /* delete temporary pair */
  pair_delete(p);

  if (ITR_EQ(itr, ht_itr_end(ht)))
    return NULL;

  /* get the pointer to the bucket */
  bkt = (pair_t*)ht_get(ht, itr);
  CHECK_PTR_RET(bkt, FALSE);

  /* get the pointer to the bucket list */
  return bkt;
}

static list_itr_t get_cb_itr(list_t * l, void * ctx, cbfn fn)
{
  pair_t * p;
  list_itr_t litr, lend;
  CHECK_PTR_RET(l, list_itr_end(l));
  CHECK_PTR_RET(fn, list_itr_end(l));

  /* search the bucket list for duplicate */
  litr = list_itr_begin(l);
  lend = list_itr_end(l);
  for (; litr != lend; litr = list_itr_next(l, litr))
  {
    p = (pair_t*)list_get(l, litr);
    if ((ctx == pair_first(p)) && (fn == pair_second(p)))
    {
      return litr;
    }
  }

  return lend;
}

static pair_t* find_cb(list_t * l, void * ctx, cbfn fn)
{
  CHECK_PTR_RET(l, NULL);
  CHECK_PTR_RET(fn, NULL);

  return (pair_t*)list_get(l, get_cb_itr(l, ctx, fn));
}

static int_t remove_cb(list_t * l, void * ctx, cbfn fn)
{
  list_itr_t itr;
  pair_t * p = NULL;
  CHECK_PTR_RET(l, FALSE);
  CHECK_PTR_RET(fn, FALSE);

  itr = get_cb_itr(l, ctx, fn);
  if (itr == list_itr_end(l))
    return FALSE;

  /* get a pointer to the pair */
  p = (pair_t*)list_get(l, itr);

  /* remove it from the list */
  list_pop(l, itr);

  /* free the pair */
  pair_delete(p);

  return TRUE;
}

#define FNV_PRIME (0x01000193)
static uint_t fnv_key_hash(void const * const key)
{
  uint32_t hash = 0x811c9dc5;
  uint8_t const * p = (uint8_t const *)key;
  while ((*p) != '\0')
  {
    hash *= FNV_PRIME;
    hash ^= *p++;
  }
  return (uint_t)hash;
}

static uint_t cb_hash_fn(void const * const key)
{
  CHECK_PTR_RET(key, 0);
  /* hash the string */
  return fnv_key_hash(pair_first(key));
}

static int_t cb_match_fn(void const * const l, void const * const r)
{
  CHECK_PTR_RET(l, 0);
  CHECK_PTR_RET(r, 0);

  return (strcmp(pair_first(l), pair_first(r)) == 0);
}

static void cb_delete_fn(void * p)
{
  CHECK_PTR(p);

  /* free the string for the bucket */
  FREE(pair_first(p));

  /* delete the bucket list */
  list_delete(pair_second(p));

  /* delete the bucket pair */
  pair_delete(p);
}

#ifdef UNIT_TESTING

#include <CUnit/Basic.h>

void test_cb_private_functions(void)
{
}

#endif

