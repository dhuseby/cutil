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

#ifndef AIOFD_H
#define AIOFD_H

#include <sys/uio.h>
#include "events.h"
#include "list.h"

/*
 * AIOFD CALLBACKS
 */
typedef enum aiofd_cb_e
{
  AIOFD_READ_EVT,
  AIOFD_WRITE_EVT,
  AIOFD_ERROR_EVT,
  AIOFD_READ_IO,
  AIOFD_WRITE_IO,
  AIOFD_READV_IO,
  AIOFD_WRITEV_IO,
  AIOFD_NREAD_IO
} aiofd_cb_t;

#define AIOFD_CB_FIRST (AIOFD_READ_EVT)
#define AIOFD_CB_LAST (AIOFD_NREAD_IO)
#define AIOFD_CB_COUNT (AIOFD_CB_LAST - AIOFD_CB_FIRST + 1)
#define VALID_AIOFD_CB(t) ((t >= AIOFD_CB_FIRST) && (t <= AIOFD_CB_LAST))

extern uint8_t const * const aiofd_cb[AIOFD_CB_COUNT];
#define AIOFD_CB_NAME(x) (VALID_AIOFD_CB(x) ? aiofd_cb[x] : NULL)

/* helper macros for declaring and adding aiofd callbacks */
#define READ_EVT_CB(fn,ctx) CB_2(fn,ctx,aiofd_t*,size_t)
#define WRITE_EVT_CB(fn,ctx,wd) CB_4(fn,ctx,wd,aiofd_t*,void*,size_t)
#define ERROR_EVT_CB(fn,ctx,wd) CB_3(fn,ctx,wd,aiofd_t*,int)

#define READ_IO_CB(fn,ctx) CB_5(fn,ctx,aiofd_t*,int,void*,size_t,ssize_t*)
#define WRITE_IO_CB(fn,ctx,wd) CB_6(fn,ctx,wd,aiofd_t*,int,void const *,size_t,ssize_t*)
#define READV_IO_CB(fn,ctx) CB_5(fn,ctx,aiofd_t*,int,struct iovec*,size_t,ssize_t*)
#define WRITEV_IO_CB(fn,ctx,wd) CB_6(fn,ctx,wd,aiofd_t*,int,struct iovec const*,size_t,ssize_t*)
#define NREAD_IO_CB(fn,ctx) CB_5(fn,ctx,aiofd_t*,int,size_t*,int_t*,int*)

#define ADD_READ_EVT_CB(cb,fn,ctx) ADD_CB(cb,AIOFD_CB_NAME(AIOFD_READ_EVT),fn,ctx)
#define ADD_WRITE_EVT_CB(cb,fn,ctx) ADD_CB(cb,AIOFD_CB_NAME(AIOFD_WRITE_EVT),fn,ctx)
#define ADD_ERROR_EVT_CB(cb,fn,ctx) ADD_CB(cb,AIOFD_CB_NAME(AIOFD_ERROR_EVT),fn,ctx)

#define ADD_READ_IO_CB(cb,fn,ctx) ADD_CB(cb,AIOFD_CB_NAME(AIOFD_READ_IO),fn,ctx)
#define ADD_WRITE_IO_CB(cb,fn,ctx) ADD_CB(cb,AIOFD_CB_NAME(AIOFD_WRITE_IO),fn,ctx)
#define ADD_READV_IO_CB(cb,fn,ctx) ADD_CB(cb,AIOFD_CB_NAME(AIOFD_READV_IO),fn,ctx)
#define ADD_WRITEV_IO_CB(cb,fn,ctx) ADD_CB(cb,AIOFD_CB_NAME(AIOFD_WRITEV_IO),fn,ctx)
#define ADD_NREAD_IO_CB(cb,fn,ctx) ADD_CB(cb,AIOFD_CB_NAME(AIOFD_NREAD_IO),fn,ctx)

typedef struct aiofd_s aiofd_t;

aiofd_t * aiofd_new(int wfd, int rfd, cb_t *cb);
void aiofd_delete(void *aio);

/* enables/disables processing of the read and write events */
int_t aiofd_enable_write_evt(aiofd_t *aiofd, int_t enable, evt_loop_t *el);
int_t aiofd_enable_read_evt(aiofd_t *aiofd, int_t enable, evt_loop_t *el);

/* read data from the fd */
ssize_t aiofd_read(aiofd_t *aiofd, void *buf, size_t n);

/* read from fd into iovec (scatter input) */
ssize_t aiofd_readv(aiofd_t *aiofd, struct iovec *iov, size_t iovcnt);

/* write data to the fd */
int_t aiofd_write(aiofd_t *aiofd, void const *buf, size_t n, void *wd);

/* write iovec to the fd (gather output) */
int_t aiofd_writev(aiofd_t *aiofd, struct iovec const *iov, size_t iovcnt, void *wd);

/* flush the fd output */
int_t aiofd_flush(aiofd_t *aiofd);

/* get/set the listening fd flag, used for bound and listening socket fd's */
int_t aiofd_set_listen(aiofd_t *aiofd, int_t listen);
int_t aiofd_get_listen(aiofd_t const *aiofd);

#endif /*AIOFD_H*/
