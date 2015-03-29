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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <sys/ioctl.h>

#include "debug.h"
#include "macros.h"
#include "list.h"
#include "cb.h"
#include "events.h"
#include "aiofd.h"

#if defined(UNIT_TESTING)
#include "test_flags.h"
extern evt_loop_t *el;
#endif

struct aiofd_s
{
  int         wfd;      /* read/write fd, if only one given, write-only otherwise */
  int         rfd;      /* read fd if two are given */
  list_t      wbuf;     /* array of buffers waiting to be written */
  evt_t      *wevt;     /* write event */
  evt_t      *revt;     /* read event */
  cb_t       *cb;       /* callback maanger */
  cb_t       *int_cb;   /* internall callback manager */
};

typedef struct aiofd_write_s
{
  void const    *data;  /* data to write */
  struct iovec const *iov; /* iovec to write */
  size_t      size;     /* size of buffer to write */
  size_t      nleft;    /* amount left to write */
  void       *wd;       /* per-write data to pass to low-level io fn */
} aiofd_write_t;

uint8_t const * const aiofd_cb[AIOFD_CB_COUNT] =
{
  UT("aiofd-read-evt"),
  UT("aiofd-write-evt"),
  UT("aiofd-error-evt"),
  UT("aiofd-read-io"),
  UT("aiofd-write-io"),
  UT("aiofd-readv-io"),
  UT("aiofd-writev-io"),
  UT("aiofd-nread-io")
};

/* helper macros for calling the callbacks */
#define READ_EVT(cb, ...) cb_call(cb, AIOFD_CB_NAME(AIOFD_READ_EVT), __VA_ARGS__)
#define WRITE_EVT(cb, ...) cb_call(cb, AIOFD_CB_NAME(AIOFD_WRITE_EVT), __VA_ARGS__)
#define ERROR_EVT(cb, ...) cb_call(cb, AIOFD_CB_NAME(AIOFD_ERROR_EVT), __VA_ARGS__)

#define READ_IO(cb, ...) cb_call(cb, AIOFD_CB_NAME(AIOFD_READ_IO), __VA_ARGS__)
#define WRITE_IO(cb, ...) cb_call(cb, AIOFD_CB_NAME(AIOFD_WRITE_IO), __VA_ARGS__)
#define READV_IO(cb, ...) cb_call(cb, AIOFD_CB_NAME(AIOFD_READV_IO), __VA_ARGS__)
#define WRITEV_IO(cb, ...) cb_call(cb, AIOFD_CB_NAME(AIOFD_WRITEV_IO), __VA_ARGS__)
#define NREAD_IO(cb, ...) cb_call(cb, AIOFD_CB_NAME(AIOFD_NREAD_IO), __VA_ARGS__)

/*
 * IO EVENT CALLBACKS
 */
static void aiofd_io_evt(aiofd_t *aiofd, evt_t *evt, int fd, evt_io_type_t type);
IO_CB(aiofd_io_evt, aiofd_t*);


static int_t aiofd_init(aiofd_t *aiofd, int wfd, int rfd, cb_t *cb);
static void aiofd_deinit(aiofd_t *aiofd);
static int_t aiofd_write_common(aiofd_t *aiofd, void const *buf,
                                struct iovec const * iov, size_t cnt,
                                size_t total, void *wd);


aiofd_t * aiofd_new(int wfd, int rfd, cb_t *cb)
{
  aiofd_t *aiofd = NULL;

  /* allocate the the aiofd struct */
  aiofd = (aiofd_t*)CALLOC(1, sizeof(aiofd_t));
  CHECK_PTR_RET(aiofd, NULL);

  /* initlialize the aiofd */
  if(!aiofd_init(aiofd, wfd, rfd, cb))
  {
    FREE(aiofd);
    return NULL;
  }

  return aiofd;
}

void aiofd_delete(void *aio)
{
  aiofd_t *aiofd = (aiofd_t*)aio;
  CHECK_PTR(aiofd);

  aiofd_deinit(aiofd);
  FREE((void*)aiofd);
}

int_t aiofd_enable_write_evt(aiofd_t *aiofd, int_t enable, evt_loop_t *el)
{
  CHECK_PTR_RET(aiofd, FALSE);
  CHECK_PTR_RET(aiofd->wevt, FALSE);

  if(enable)
  {
    DEBUG("starting write event\n");
    CHECK_RET(evt_start_event(aiofd->wevt, el) == EVT_OK, FALSE);
  }
  else
  {
    DEBUG("stopping write event\n");
    CHECK_RET(evt_stop_event(aiofd->wevt) == EVT_OK, FALSE);
  }
  return TRUE;
}

int_t aiofd_enable_read_evt(aiofd_t *aiofd, int_t enable, evt_loop_t *el)
{
  CHECK_PTR_RET(aiofd, FALSE);
  CHECK_PTR_RET(aiofd->revt, FALSE);

  if(enable)
  {
    DEBUG("starting read event\n");
    CHECK_RET(evt_start_event(aiofd->revt, el) == EVT_OK, FALSE);
  }
  else
  {
    DEBUG("stopping read event\n");
    CHECK_RET(evt_stop_event(aiofd->revt) == EVT_OK, FALSE);
  }
  return TRUE;
}

ssize_t aiofd_read(aiofd_t *aiofd, void *buf, size_t n)
{
  ssize_t res = 0;

  UNIT_TEST_RET(aiofd_read);

  CHECK_PTR_RET(aiofd, -1);
  CHECK_PTR_RET(buf, -1);
  CHECK_RET(n > 0, -1);

  /* call the low level read function */
  CHECK_RET(READ_IO(aiofd->cb, aiofd, aiofd->rfd, buf, n, &res), -1);

  switch (res)
  {
    case (ssize_t)0:
      errno = EPIPE;
    case (ssize_t)-1:
      /* call the error event callback */
      ERROR_EVT(aiofd->cb, NULL, aiofd, ERRNO);
      return -1;
    default:
      return res;
  }
}

ssize_t aiofd_readv(aiofd_t *aiofd, struct iovec *iov, size_t iovcnt)
{
  ssize_t res = 0;

  UNIT_TEST_RET(aiofd_readv);

  CHECK_PTR_RET(aiofd, -1);
  CHECK_PTR_RET(iov, -1);
  CHECK_RET((iovcnt > 0), -1);

  /* call the low-level readv function */
  CHECK_RET(READV_IO(aiofd->cb, aiofd, aiofd->rfd, iov, iovcnt, &res), -1);

  switch (res)
  {
    case (ssize_t)0:
      errno = EPIPE;
    case (ssize_t)-1:
      /* call the error event callback */
      ERROR_EVT(aiofd->cb, NULL, aiofd, ERRNO);
      return -1;
    default:
      return res;
  }
}


int_t aiofd_write(aiofd_t *aiofd, void const *buf, size_t n, void *wd)
{
  UNIT_TEST_RET(aiofd_write);
  return aiofd_write_common(aiofd, buf, NULL, n, n, wd);
}

int_t aiofd_writev(aiofd_t *aiofd, struct iovec const *iov, size_t iovcnt, void *wd)
{
  size_t i;
  size_t total = 0;

  UNIT_TEST_RET(aiofd_writev);

  /* calculate how many bytes are in the iovec */
  for (i = 0; i < iovcnt; i++)
  {
    total += iov[i].iov_len;
  }

  return aiofd_write_common(aiofd, NULL, iov, iovcnt, total, wd);
}

int_t aiofd_flush(aiofd_t *aiofd)
{
  UNIT_TEST_RET(aiofd_flush);

  CHECK_PTR_RET(aiofd, FALSE);

  CHECK_RET(FSYNC(aiofd->wfd) == 0, FALSE);
  CHECK_RET(FSYNC(aiofd->rfd) == 0, FALSE);

  return TRUE;
}


/*
 * PRIVATE
 */

static void aiofd_io_evt(aiofd_t *aiofd, evt_t *evt, int fd, evt_io_type_t type)
{
  int ret = 0;
  int_t listening = FALSE;
  size_t nread = 0;
  ssize_t written = 0;
  aiofd_write_t *wb = NULL;

  CHECK_PTR(aiofd);
  CHECK_PTR(evt);

  if((type & EVT_IO_READ) && (aiofd->rfd >= 0))
  {
    DEBUG("read event\n");

    /* get how much data is available to read */
    NREAD_IO(aiofd->cb, aiofd, aiofd->rfd, &nread, &listening, &ret);
    if((ret < 0) && (listening == FALSE))
    {
      DEBUG("calling error callback\n");
      ERROR_EVT(aiofd->cb, NULL, aiofd, ERRNO);
      return;
    }

    /* callback to tell client that there is data to read */
    DEBUG("calling read callback (nread = %d)\n", nread);
    READ_EVT(aiofd->cb, aiofd, nread);
  }
  else if((type & EVT_IO_WRITE) && (aiofd->wfd >= 0))
  {
    DEBUG("write event\n");

    while(list_count(&(aiofd->wbuf)) > 0)
    {
      /* we must have data to write */
      wb = list_get_head(&(aiofd->wbuf));

      if(!wb)
      {
        list_pop_head(&(aiofd->wbuf));
        DEBUG("bad write data pointer");
        return;
      }

      if(wb->iov)
      {
        /* call the low-level writev function */
        WRITEV_IO(aiofd->cb, wb->wd, aiofd, aiofd->wfd, wb->iov, wb->size, &written);
      }
      else
      {
        /* call the low-level write function */
        WRITE_IO(aiofd->cb, wb->wd, aiofd, aiofd->wfd, wb->data, wb->size, &written);
      }

      /* try to write the data to the socket */
      if(written < 0)
      {
        if((ERRNO == EAGAIN) || (ERRNO == EWOULDBLOCK))
        {
          DEBUG("write would block...waiting for next write event\n");
          return;
        }
        else
        {
          DEBUG("write error: %s (%d)\n", strerror(ERRNO), ERRNO);
          ERROR_EVT(aiofd->cb, wb->wd, aiofd, ERRNO);
          return;
        }
      }
      else
      {
        /* decrement how many bytes are left to write */
        wb->nleft -= written;

        /* check to see if everything has been written */
        if(wb->nleft <= 0)
        {
          /* remove the write buf from the queue */
          list_pop_head(&(aiofd->wbuf));

          /* call the write complete callback to let client know that a
           * particular buf has been written to the fd. */
          WRITE_EVT(aiofd->cb, wb->wd, aiofd, (wb->data ? wb->data : wb->iov), wb->size);

          /* free it */
          FREE(wb);
        }
      }
    }

    /* call the write complete callback with NULL buf to signal completion */
    WRITE_EVT(aiofd->cb, NULL, aiofd, NULL, 0);
  }
}

static int_t aiofd_init(aiofd_t *aiofd, int wfd, int rfd, cb_t *cb)
{
  UNIT_TEST_RET(aiofd_initialize);

  CHECK_PTR_RET(aiofd, FALSE);
  CHECK_RET((wfd >= 0) || (rfd >= 0), FALSE);
  CHECK_PTR_RET(cb, FALSE);

  MEMSET((void*)aiofd, 0, sizeof(aiofd_t));

  /* store the file descriptors */
  aiofd->wfd = wfd;
  aiofd->rfd = rfd;
  aiofd->cb = cb;

  /* initialize the write buf */
  CHECK_RET(list_init(&(aiofd->wbuf), 8, FREE), FALSE);

  /* create internal */
  aiofd->int_cb = cb_new();
  CHECK_GOTO(aiofd->int_cb, _aiofd_init_3);

  /* add the callbacks to the callback manager */
  ADD_IO_CB(aiofd->int_cb, aiofd_io_evt, aiofd);

  /* set up io event for wfd */
  if(wfd >= 0)
  {
    aiofd->wevt = evt_new_io_event(aiofd->int_cb, wfd, EVT_IO_WRITE);
    CHECK_GOTO(aiofd->wevt, _aiofd_init_2);
  }

  /* set up io event for rfd */
  if(rfd >= 0)
  {
    aiofd->revt = evt_new_io_event(aiofd->int_cb, rfd, EVT_IO_READ);
    CHECK_GOTO(aiofd->revt, _aiofd_init_1);
  }

  return TRUE;

_aiofd_init_1:
  evt_delete_event(aiofd->wevt);
_aiofd_init_2:
  cb_delete(aiofd->int_cb);
_aiofd_init_3:
  list_deinit(&(aiofd->wbuf));

  return FALSE;
}

static void aiofd_deinit(aiofd_t *aiofd)
{
  evt_delete_event(aiofd->revt);
  evt_delete_event(aiofd->wevt);
  cb_delete(aiofd->int_cb);
  list_deinit(&(aiofd->wbuf));
}

/* queue up data to write to the fd */
static int_t aiofd_write_common(aiofd_t *aiofd, void const *buf,
                                struct iovec const * iov, size_t cnt,
                                size_t total, void *wd)
{
    aiofd_write_t *wb = NULL;

    UNIT_TEST_RET(aiofd_write_common);

    CHECK_PTR_RET(aiofd, FALSE);
    CHECK_RET((buf || iov), FALSE);
    CHECK_RET(cnt > 0, FALSE);
    CHECK_RET(total > 0, FALSE);

    wb = CALLOC(1, sizeof(aiofd_write_t));
    if(wb == NULL)
    {
        DEBUG("failed to allocate write buf struct\n");
        return FALSE;
    }

    /* store the values */
    wb->data = buf;
    wb->iov = iov;
    wb->size = cnt;
    wb->nleft = total;
    wb->wd = wd;

    /* queue the write */
    CHECK_GOTO(list_push_tail(&(aiofd->wbuf), wb), _aiofd_wcom);

    return TRUE;

_aiofd_wcom:
    FREE(wb);
    return FALSE;
}

#if defined(UNIT_TESTING)

#include <CUnit/Basic.h>

#if 0
typedef struct cb_count_s
{
    int r, w, e;
} cb_count_t;

static cb_count_t cb_counts;
static int_t read_callback_ret = TRUE;
static int_t write_callback_ret = TRUE;
static int_t error_callback_ret = TRUE;

static int_t read_callback_fn(aiofd_t *aiofd, size_t nread, void *user_data)
{
    ((cb_count_t*)user_data)->r++;
    return read_callback_ret;
}

static int_t write_callback_fn(aiofd_t *aiofd, uint8_t const *buf, void *user_data, void *per_write_data)
{
    ((cb_count_t*)user_data)->w++;
    return write_callback_ret;
}

static int_t error_callback_fn(aiofd_t *aiofd, int err, void *user_data)
{
    ((cb_count_t*)user_data)->e++;
    return error_callback_ret;
}

static void test_aiofd_write_fn(void)
{
    evt_t evt;
    evt_params_t params;
    aiofd_t aiofd;
    aiofd_write_t *wb = NULL;
    int8_t const * const buf = "foo";
    int const size = 4;
    struct iovec iov;

    MEMSET(&evt, 0, sizeof(evt_t));
    MEMSET(&params, 0, sizeof(evt_params_t));
    MEMSET(&aiofd, 0, sizeof(aiofd_t));
    MEMSET(&iov, 0, sizeof(struct iovec));
    MEMSET(&cb_counts, 0, sizeof(cb_count_t));

    CU_ASSERT_EQUAL(aiofd_write_fn(NULL, NULL, NULL, NULL), EVT_BADPTR);
    CU_ASSERT_EQUAL(aiofd_write_fn(el, NULL, NULL, NULL), EVT_BADPTR);
    CU_ASSERT_EQUAL(aiofd_write_fn(el, &evt, NULL, NULL), EVT_BADPTR);
    CU_ASSERT_EQUAL(aiofd_write_fn(el, &evt, &params, NULL), EVT_BADPTR);

    CU_ASSERT_EQUAL(aiofd_write_fn(el, &evt, &params, &aiofd), EVT_BADPTR);
    aiofd.ops.write_fn = &aiofd_do_write;
    aiofd.ops.writev_fn = &aiofd_do_writev;

    /* nothing in the write buf, no write callback, should fall through to OK */
    list_initialize(&(aiofd.wbuf), 0, NULL);
    CU_ASSERT_EQUAL(aiofd_write_fn(el, &evt, &params, &aiofd), EVT_OK);

    /* set user data to the callback counter */
    aiofd.user_data = &cb_counts;

    aiofd.ops.write_evt_fn = &write_callback_fn;
    MEMSET(&cb_counts, 0, sizeof(cb_count_t));

    /* w callback should be called once, queue should be empty */
    CU_ASSERT_EQUAL(aiofd_write_fn(el, &evt, &params, &aiofd), EVT_OK);
    CU_ASSERT_EQUAL(cb_counts.r, 0);
    CU_ASSERT_EQUAL(cb_counts.w, 1);
    CU_ASSERT_EQUAL(cb_counts.e, 0);
    CU_ASSERT_EQUAL(list_count(&(aiofd.wbuf)), 0);

    /* push a NULL pointer into the write queue and confirm that it recovers gracefully */
    list_push_tail(&(aiofd.wbuf), NULL);
    MEMSET(&cb_counts, 0, sizeof(cb_count_t));

    CU_ASSERT_EQUAL(aiofd_write_fn(el, &evt, &params, &aiofd), EVT_BADPTR);
    CU_ASSERT_EQUAL(cb_counts.r, 0);
    CU_ASSERT_EQUAL(cb_counts.w, 0);
    CU_ASSERT_EQUAL(cb_counts.e, 0);
    CU_ASSERT_EQUAL(list_count(&(aiofd.wbuf)), 0);

    /* now queue up a non-iovec pending write */
    wb = CALLOC(1, sizeof(aiofd_write_t));
    wb->data = (void*)buf;
    wb->size = size;
    wb->iov = FALSE;
    wb->nleft = size;
    list_push_tail(&(aiofd.wbuf), wb);
    MEMSET(&cb_counts, 0, sizeof(cb_count_t));
    fake_write = TRUE;
    fake_write_ret = 2;

    /* results in 2 calls to write callback and an empty wbuf queue */
    CU_ASSERT_EQUAL(aiofd_write_fn(el, &evt, &params, &aiofd), EVT_OK);
    CU_ASSERT_EQUAL(cb_counts.r, 0);
    CU_ASSERT_EQUAL(cb_counts.w, 2); /* 1 for buf write complete, 1 for queue empty */
    CU_ASSERT_EQUAL(cb_counts.e, 0);
    CU_ASSERT_EQUAL(list_count(&(aiofd.wbuf)), 0); /* queue should be empty */

    /* remove write callback pointer */
    wb = CALLOC(1, sizeof(aiofd_write_t));
    wb->data = (void*)buf;
    wb->size = size;
    wb->iov = FALSE;
    wb->nleft = size;
    list_push_tail(&(aiofd.wbuf), wb);
    aiofd.ops.write_evt_fn = NULL;
    MEMSET(&cb_counts, 0, sizeof(cb_count_t));
    fake_write_ret = 4;

    /* results in no callback, queue should be empty */
    CU_ASSERT_EQUAL(aiofd_write_fn(el, &evt, &params, &aiofd), EVT_OK);
    CU_ASSERT_EQUAL(cb_counts.r, 0);
    CU_ASSERT_EQUAL(cb_counts.w, 0); /* NULL callback pointer */
    CU_ASSERT_EQUAL(cb_counts.e, 0);
    CU_ASSERT_EQUAL(list_count(&(aiofd.wbuf)), 0); /* queue should be empty */

    /* done with write, moving to writev */
    fake_write = FALSE;
    fake_write_ret = -1;

    /* queue up a writev */
    iov.iov_base = (void*)buf;
    iov.iov_len = size;
    wb = CALLOC(1, sizeof(aiofd_write_t));
    wb->data = (void*)&iov;
    wb->size = 1;
    wb->iov = TRUE;
    wb->nleft = size;
    list_push_tail(&(aiofd.wbuf), wb);
    aiofd.ops.write_evt_fn = &write_callback_fn;
    MEMSET(&cb_counts, 0, sizeof(cb_count_t));

    /* results in 2 write callbacks */
    CU_ASSERT_EQUAL(aiofd_write_fn(el, &evt, &params, &aiofd), EVT_OK);
    CU_ASSERT_EQUAL(cb_counts.r, 0);
    CU_ASSERT_EQUAL(cb_counts.w, 2); /* NULL callback pointer */
    CU_ASSERT_EQUAL(cb_counts.e, 0);
    CU_ASSERT_EQUAL(list_count(&(aiofd.wbuf)), 0); /* queue should be empty */

    fake_writev = TRUE;
    fake_writev_ret = 2;

    /* queue up a writev */
    wb = CALLOC(1, sizeof(aiofd_write_t));
    wb->data = (void*)&iov;
    wb->size = 1;
    wb->iov = TRUE;
    wb->nleft = size;
    list_push_tail(&(aiofd.wbuf), wb);
    aiofd.ops.write_evt_fn = &write_callback_fn;
    MEMSET(&cb_counts, 0, sizeof(cb_count_t));

    /* results in 2 calls to write callback and an empty wbuf queue */
    CU_ASSERT_EQUAL(aiofd_write_fn(el, &evt, &params, &aiofd), EVT_OK);
    CU_ASSERT_EQUAL(cb_counts.r, 0);
    CU_ASSERT_EQUAL(cb_counts.w, 2); /* NULL callback pointer */
    CU_ASSERT_EQUAL(cb_counts.e, 0);
    CU_ASSERT_EQUAL(list_count(&(aiofd.wbuf)), 0); /* queue should be empty */


    wb = CALLOC(1, sizeof(aiofd_write_t));
    wb->data = (void*)&iov;
    wb->size = 1;
    wb->iov = TRUE;
    wb->nleft = size;
    list_push_tail(&(aiofd.wbuf), wb);
    aiofd.ops.write_evt_fn = NULL;
    MEMSET(&cb_counts, 0, sizeof(cb_count_t));
    fake_writev_ret = 4;

    /* results in no callback, queue should be empty */
    CU_ASSERT_EQUAL(aiofd_write_fn(el, &evt, &params, &aiofd), EVT_OK);
    CU_ASSERT_EQUAL(cb_counts.r, 0);
    CU_ASSERT_EQUAL(cb_counts.w, 0); /* NULL callback pointer */
    CU_ASSERT_EQUAL(cb_counts.e, 0);
    CU_ASSERT_EQUAL(list_count(&(aiofd.wbuf)), 0); /* queue should be empty */

    wb = CALLOC(1, sizeof(aiofd_write_t));
    wb->data = (void*)&iov;
    wb->size = 1;
    wb->iov = TRUE;
    wb->nleft = size;
    list_push_tail(&(aiofd.wbuf), wb);
    aiofd.ops.write_evt_fn = &write_callback_fn;
    MEMSET(&cb_counts, 0, sizeof(cb_count_t));
    fake_writev_ret = -1;
    fake_errno = TRUE;
    fake_errno_value = EAGAIN;

    /* results in 1 callback, queue should not be empty */
    CU_ASSERT_EQUAL(aiofd_write_fn(el, &evt, &params, &aiofd), EVT_OK);
    CU_ASSERT_EQUAL(cb_counts.r, 0);
    CU_ASSERT_EQUAL(cb_counts.w, 1); /* NULL callback pointer */
    CU_ASSERT_EQUAL(cb_counts.e, 0);
    CU_ASSERT_EQUAL(list_count(&(aiofd.wbuf)), 1); /* queue should be empty */

    aiofd.ops.write_evt_fn = NULL;
    MEMSET(&cb_counts, 0, sizeof(cb_count_t));
    fake_errno_value = EWOULDBLOCK;

    /* results in no callback, queue should not be empty */
    CU_ASSERT_EQUAL(aiofd_write_fn(el, &evt, &params, &aiofd), EVT_OK);
    CU_ASSERT_EQUAL(cb_counts.r, 0);
    CU_ASSERT_EQUAL(cb_counts.w, 0); /* NULL callback pointer */
    CU_ASSERT_EQUAL(cb_counts.e, 0);
    CU_ASSERT_EQUAL(list_count(&(aiofd.wbuf)), 1); /* queue should be empty */

    MEMSET(&cb_counts, 0, sizeof(cb_count_t));
    fake_errno_value = EBADF;

    /* results in no callback, queue should not be empty */
    CU_ASSERT_EQUAL(aiofd_write_fn(el, &evt, &params, &aiofd), EVT_OK);
    CU_ASSERT_EQUAL(cb_counts.r, 0);
    CU_ASSERT_EQUAL(cb_counts.w, 0); /* NULL callback pointer */
    CU_ASSERT_EQUAL(cb_counts.e, 0);
    CU_ASSERT_EQUAL(list_count(&(aiofd.wbuf)), 1); /* queue should be empty */

    MEMSET(&cb_counts, 0, sizeof(cb_count_t));
    aiofd.ops.write_evt_fn = &write_callback_fn;
    fake_errno_value = EBADF;

    /* results in no callback, queue should not be empty */
    CU_ASSERT_EQUAL(aiofd_write_fn(el, &evt, &params, &aiofd), EVT_OK);
    CU_ASSERT_EQUAL(cb_counts.r, 0);
    CU_ASSERT_EQUAL(cb_counts.w, 0); /* NULL callback pointer */
    CU_ASSERT_EQUAL(cb_counts.e, 0);
    CU_ASSERT_EQUAL(list_count(&(aiofd.wbuf)), 1); /* queue should be empty */

    MEMSET(&cb_counts, 0, sizeof(cb_count_t));
    aiofd.ops.error_evt_fn = &error_callback_fn;
    fake_errno_value = EBADF;

    /* results in 1 write and 1 error callback, queue should not be empty */
    CU_ASSERT_EQUAL(aiofd_write_fn(el, &evt, &params, &aiofd), EVT_OK);
    CU_ASSERT_EQUAL(cb_counts.r, 0);
    CU_ASSERT_EQUAL(cb_counts.w, 0); /* NULL callback pointer */
    CU_ASSERT_EQUAL(cb_counts.e, 1);
    CU_ASSERT_EQUAL(list_count(&(aiofd.wbuf)), 1); /* queue should be empty */

    FREE(wb);

    fake_errno = FALSE;
    fake_errno_value = -1;
    fake_writev = FALSE;
    fake_writev_ret = -1;
}

static void test_aiofd_read_fn(void)
{
    evt_t evt;
    evt_params_t params;
    aiofd_t aiofd;

    MEMSET(&evt, 0, sizeof(evt_t));
    MEMSET(&params, 0, sizeof(evt_params_t));
    MEMSET(&aiofd, 0, sizeof(aiofd_t));
    MEMSET(&cb_counts, 0, sizeof(cb_count_t));

    CU_ASSERT_EQUAL(aiofd_read_fn(NULL, NULL, NULL, NULL), EVT_BADPTR);
    CU_ASSERT_EQUAL(aiofd_read_fn(el, NULL, NULL, NULL), EVT_BADPTR);
    CU_ASSERT_EQUAL(aiofd_read_fn(el, &evt, NULL, NULL), EVT_BADPTR);
    CU_ASSERT_EQUAL(aiofd_read_fn(el, &evt, &params, NULL), EVT_BADPTR);

    aiofd.user_data = &cb_counts;

    fake_ioctl = TRUE;
    fake_ioctl_ret = -1;

    MEMSET(&cb_counts, 0, sizeof(cb_count_t));

    /* should receive no callbacks */
    CU_ASSERT_EQUAL(aiofd_read_fn(el, &evt, &params, &aiofd), EVT_OK);
    CU_ASSERT_EQUAL(cb_counts.r, 0);
    CU_ASSERT_EQUAL(cb_counts.w, 0);
    CU_ASSERT_EQUAL(cb_counts.e, 0);

    aiofd.ops.error_evt_fn = &error_callback_fn;
    MEMSET(&cb_counts, 0, sizeof(cb_count_t));

    /* should receive an error callback */
    CU_ASSERT_EQUAL(aiofd_read_fn(el, &evt, &params, &aiofd), EVT_OK);
    CU_ASSERT_EQUAL(cb_counts.r, 0);
    CU_ASSERT_EQUAL(cb_counts.w, 0);
    CU_ASSERT_EQUAL(cb_counts.e, 1);

    MEMSET(&cb_counts, 0, sizeof(cb_count_t));
    fake_errno = TRUE;
    fake_errno_value = EBADF;

    /* should receive an error callback */
    CU_ASSERT_EQUAL(aiofd_read_fn(el, &evt, &params, &aiofd), EVT_OK);
    CU_ASSERT_EQUAL(cb_counts.r, 0);
    CU_ASSERT_EQUAL(cb_counts.w, 0);
    CU_ASSERT_EQUAL(cb_counts.e, 1);

    fake_errno = FALSE;
    fake_errno_value = -1;
    fake_ioctl_ret = 0;
    MEMSET(&cb_counts, 0, sizeof(cb_count_t));

    /* should receive no callbacks */
    CU_ASSERT_EQUAL(aiofd_read_fn(el, &evt, &params, &aiofd), EVT_OK);
    CU_ASSERT_EQUAL(cb_counts.r, 0);
    CU_ASSERT_EQUAL(cb_counts.w, 0);
    CU_ASSERT_EQUAL(cb_counts.e, 0);

    aiofd.ops.read_evt_fn = &read_callback_fn;
    MEMSET(&cb_counts, 0, sizeof(cb_count_t));

    /* should receive one read callback */
    CU_ASSERT_EQUAL(aiofd_read_fn(el, &evt, &params, &aiofd), EVT_OK);
    CU_ASSERT_EQUAL(cb_counts.r, 1);
    CU_ASSERT_EQUAL(cb_counts.w, 0);
    CU_ASSERT_EQUAL(cb_counts.e, 0);

    MEMSET(&cb_counts, 0, sizeof(cb_count_t));
    read_callback_ret = FALSE;

    /* should receive one read callback */
    CU_ASSERT_EQUAL(aiofd_read_fn(el, &evt, &params, &aiofd), EVT_OK);
    CU_ASSERT_EQUAL(cb_counts.r, 1);
    CU_ASSERT_EQUAL(cb_counts.w, 0);
    CU_ASSERT_EQUAL(cb_counts.e, 0);
}

void test_aiofd_write_common(void)
{
    aiofd_t aiofd;
    aiofd_write_t *wb = NULL;
    int8_t *buf = "foo";
    int const size = 4;
    struct iovec iov;

    MEMSET(&aiofd, 0, sizeof(aiofd_t));
    MEMSET(&iov, 0, sizeof(struct iovec));

    CU_ASSERT_FALSE(aiofd_write_common(NULL, NULL, 0, 0, FALSE, NULL));
    CU_ASSERT_FALSE(aiofd_write_common(&aiofd, NULL, 0, 0, FALSE, NULL));
    CU_ASSERT_FALSE(aiofd_write_common(&aiofd, buf, 0, 0, FALSE, NULL));
    CU_ASSERT_FALSE(aiofd_write_common(&aiofd, buf, size, 0, FALSE, NULL));

    fail_alloc = TRUE;
    CU_ASSERT_FALSE(aiofd_write_common(&aiofd, buf, size, size, FALSE, NULL));
    fail_alloc = FALSE;

    fake_list_push = TRUE;
    fake_list_push_ret = FALSE;
    CU_ASSERT_FALSE(aiofd_write_common(&aiofd, buf, size, size, FALSE, NULL));
    fake_list_push = FALSE;

    fake_event_start_handler = TRUE;
    fake_event_start_handler_ret = FALSE;
    CU_ASSERT_FALSE(aiofd_write_common(&aiofd, buf, size, size, FALSE, NULL));
    fake_event_start_handler = FALSE;
}
#endif

void test_aiofd_private_functions(void)
{
  /*
    test_aiofd_write_fn();
    test_aiofd_read_fn();
    test_aiofd_write_common();
    */
}

#endif


