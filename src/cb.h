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

#ifndef CB_H
#define CB_H

#include <stdarg.h>

/* callback function */
typedef void (*cbfn)(void * ctx, va_list args);

typedef struct cb_s cb_t;

/* alloc/dealloc cb structs */
cb_t* cb_new(void);
void cb_delete(void * p);

/* are there any callbacks registered to the given name? */
int_t cb_has(cb_t * cb, uint8_t const * const name);

/* stores, context + callback as tuple value under name key */
int_t cb_add(cb_t * cb, uint8_t const * const name, void * ctx, cbfn fn);
int_t cb_remove(cb_t * cb, uint8_t const * const name, void * ctx, cbfn fn);

/* calls all callbacks associated with the name, passing given params */
int_t cb_call(cb_t * cb, uint8_t const * const name, ...);

/* helper macros make declaring callbacks easier.  you only need to provide
 * the name, the pointer to the function, the type of the context object, and
 * the types of the parameters, if any.  the name of the CB_* macro
 * includes the number of parameters.  if your callback takes two parameters
 * use CB_2, etc.  here's an example:
 *
 * static void foo(my_t* ctx);
 * static void bar(my_t* ctx, int n, double m);
 * static void baz(another_t* ctx, void* s, char i);
 *
 * CB_0(foo, my_t*);
 * CB_2(bar, my_t*, int, double);
 * CB_1(baz, another_t*, void*, char);
 *
 * then later in your code, you only have to add calls to ADD_CB to
 * add each callback.
 *
 * ...
 * my_t *t;
 * another_t *a;
 * cb_t * cb = NULL;
 * cb = cb_new();
 * ADD_CB(cb, "foo", foo, t);
 * ADD_CB(cb, "bar", bar, t);
 * ADD_CB(cb, "baz", baz, a);
 * ...
 *
 *  NOTE: if you use these macros, you cannot have multiple callbacks
 *  registered under the same name.  the convenience of these macros reduces
 *  a little flexibility.
 *
 */

#define ADD_CB(cb,n,fn,c) cb_add(cb,n,c,&cb___##fn)
#define REMOVE_CB(cb,n,fn,c) cb_remove(cb,n,c,&cb___##fn)
#define CB_0(fn,x) \
  static void cb___##fn(void * ctx, va_list args) { \
    (*fn)((x)ctx); \
  }
#define CB_1(fn,x,a) \
  static void cb___##fn(void * ctx, va_list args) { \
    a a1 = va_arg(args,a); \
    (*fn)((x)ctx,a1); \
  }
#define CB_2(fn,x,a,b) \
  static void cb___##fn(void * ctx, va_list args) { \
    a a1 = va_arg(args,a); \
    b a2 = va_arg(args,b); \
    (*fn)((x)ctx,a1,a2); \
  }
#define CB_3(fn,x,a,b,c) \
  static void cb___##fn(void * ctx, va_list args) { \
    a a1 = va_arg(args,a); \
    b a2 = va_arg(args,b); \
    c a3 = va_arg(args,c); \
    (*fn)((x)ctx,a1,a2,a3); \
  }
#define CB_4(fn,x,a,b,c,d) \
  static void cb___##fn(void * ctx, va_list args) { \
    a a1 = va_arg(args,a); \
    b a2 = va_arg(args,b); \
    c a3 = va_arg(args,c); \
    d a4 = va_arg(args,d); \
    (*fn)((x)ctx,a1,a2,a3,a4); \
  }
#define CB_5(fn,x,a,b,c,d,e) \
  static void cb___##fn(void * ctx, va_list args) { \
    a a1 = va_arg(args,a); \
    b a2 = va_arg(args,b); \
    c a3 = va_arg(args,c); \
    d a4 = va_arg(args,d); \
    e a5 = va_arg(args,e); \
    (*fn)((x)ctx,a1,a2,a3,a4,a5); \
  }
#define CB_6(fn,x,a,b,c,d,e,f) \
  static void cb___##fn(void * ctx, va_list args) { \
    a a1 = va_arg(args,a); \
    b a2 = va_arg(args,b); \
    c a3 = va_arg(args,c); \
    d a4 = va_arg(args,d); \
    e a5 = va_arg(args,e); \
    f a6 = va_arg(args,f); \
    (*fn)((x)ctx,a1,a2,a3,a4,a5,a6); \
  }

#endif /* CB_H */

