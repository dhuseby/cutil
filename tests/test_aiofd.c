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
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <CUnit/Basic.h>

#include <cutil/debug.h>
#include <cutil/macros.h>
#include <cutil/cb.h>
#include <cutil/aiofd.h>

#include "test_macros.h"
#include "test_flags.h"

#define REPEAT (128)
#define SIZEMAX (128)
#define MULTIPLE (8)
#define BUFSIZE (256)

extern evt_loop_t *el;
extern void test_aiofd_private_functions(void);

static int read_evts = 0;
static int write_evts = 0;
static int error_evts = 0;
static int reads = 0;
static int writes = 0;
static int readvs = 0;
static int writevs = 0;
static int nreads = 0;
static int listening = FALSE;
static cb_t *cb = NULL;
static uint8_t const * const fname = UT("foo.txt");
static uint8_t gbuf[BUFSIZE];
static struct iovec giov;
static int_t usev = FALSE;
static ssize_t to_read = 0;
static ssize_t have_read = 0;

static void read_evt(int *p, aiofd_t *aiofd, size_t nread)
{
  ssize_t r;
  if(p)
    (*p)++;

  if(nread > 0)
  {
    if(usev)
      r = aiofd_readv(aiofd, &giov, 1);
    else
      r = aiofd_read(aiofd, (void*)gbuf, nread);

    if(r >= 0)
      have_read += r;
  }

  if(have_read >= to_read)
  {
    /* stop the read event */
    aiofd_enable_read_evt(aiofd, FALSE, NULL);

    /* stop the event loop */
    evt_stop(el, FALSE);
  }
}

static void write_evt(int *p, void *wd, aiofd_t *aiofd, void *buf,
                      size_t size)
{
  if(p)
    (*p)++;

  /* if buf is NULL, all queued data has been written */
  if(buf == NULL)
  {
    /* stop the write event */
    aiofd_enable_write_evt(aiofd, FALSE, NULL);

    /* stop the event loop */
    evt_stop(el, FALSE);
  }
}

static void error_evt(int *p, void *wd, aiofd_t *aiofd, int eno)
{
  if(p)
    (*p)++;
}

static void read_io(int *p, aiofd_t *aiofd, int fd, void *buf, size_t n,
                    ssize_t *res)
{
  ssize_t r = 0;

  if(p)
    (*p)++;

  /* do the read */
  r = READ(fd, buf, n);
  if(res)
    (*res) = r;
}

static void write_io(int *p, void *wd, aiofd_t *aiofd, int fd,
                     void const * buf, size_t n, ssize_t *res)
{
  ssize_t w = 0;

  if(p)
    (*p)++;

  /* do the write */
  w = WRITE(fd, buf, n);
  if(res)
    (*res) = w;
}

static void readv_io(int *p, aiofd_t *aiofd, int fd, struct iovec *iov,
                     size_t n, ssize_t *res)
{
  ssize_t r = 0;

  if(p)
    (*p)++;

  /* do the readv */
  r = READV(fd, iov, n);
  if(res)
    (*res) = r;
}

static void writev_io(int *p, void *wd, aiofd_t *aiofd, int fd,
                      struct iovec const *iov, size_t n, ssize_t *res)
{
  ssize_t w = 0;

  if(p)
    (*p)++;

  /* do the writev */
  w = WRITEV(fd, iov, n);
  if(res)
    (*res) = w;
}

static void nread_io(int *p, aiofd_t *aiofd, int fd, size_t *nread,
                     int_t *l, int *res)
{
  int r = 0;
  size_t n = 0;
  if(p)
    (*p)++;

  r = IOCTL(fd, FIONREAD, &n);

  if(nread)
    (*nread) = n;
  if(l)
    (*l) = listening;
  if(res)
    (*res) = r;
}

/* declare the callbacks */
READ_EVT_CB(read_evt, int*);
WRITE_EVT_CB(write_evt, int*, void*);
ERROR_EVT_CB(error_evt, int*, void*);
READ_IO_CB(read_io, int*);
WRITE_IO_CB(write_io, int*, void*);
READV_IO_CB(readv_io, int*);
WRITEV_IO_CB(writev_io, int*, void*);
NREAD_IO_CB(nread_io, int*);

static void test_aiofd_newdel(void)
{
  int i;
  aiofd_t *aiofd;

  /* make sure there is an event loop */
  CU_ASSERT_PTR_NOT_NULL(el);

  /* make sure we have a callback manager */
  CU_ASSERT_PTR_NOT_NULL(cb);

  for (i = 0; i < REPEAT; i++)
  {
    aiofd = NULL;
    aiofd = aiofd_new(fileno(stdout), fileno(stdin), cb);

    CU_ASSERT_PTR_NOT_NULL_FATAL(aiofd);

    aiofd_delete(aiofd);
  }
}

static void test_aiofd_new_fail_alloc(void)
{
  int i;

  /* make sure there is an event loop */
  CU_ASSERT_PTR_NOT_NULL(el);

  /* make sure we have a callback manager */
  CU_ASSERT_PTR_NOT_NULL(cb);

  fail_alloc = TRUE;
  for (i = 0; i < REPEAT; i++)
  {
    CU_ASSERT_PTR_NULL(aiofd_new(fileno(stdout), fileno(stdin), cb))
  }
  fail_alloc = FALSE;
}

static void test_aiofd_new_fail_init(void)
{
  int i;

  /* make sure there is an event loop */
  CU_ASSERT_PTR_NOT_NULL(el);

  /* make sure we have a callback manager */
  CU_ASSERT_PTR_NOT_NULL(cb);

  fake_aiofd_initialize = TRUE;
  fake_aiofd_initialize_ret = FALSE;
  for (i = 0; i < REPEAT; i++)
  {
    CU_ASSERT_PTR_NULL(aiofd_new(fileno(stdout), fileno(stdin), cb))
  }
  fake_aiofd_initialize = FALSE;
}

static void test_aiofd_new_fail_list_init(void)
{
  int i;

  /* make sure there is an event loop */
  CU_ASSERT_PTR_NOT_NULL(el);

  /* make sure we have a callback manager */
  CU_ASSERT_PTR_NOT_NULL(cb);

  fake_list_init = TRUE;
  fake_list_init_ret = FALSE;
  for (i = 0; i < REPEAT; i++)
  {
    /* fails the call to list_init */
    CU_ASSERT_PTR_NULL(aiofd_new(fileno(stdout), fileno(stdin), cb))
  }
  fake_list_init = FALSE;
}

static void test_aiofd_new_fail_cb_init(void)
{
  int i;

  /* make sure there is an event loop */
  CU_ASSERT_PTR_NOT_NULL(el);

  /* make sure we have a callback manager */
  CU_ASSERT_PTR_NOT_NULL(cb);

  fake_cb_init = TRUE;
  fake_cb_init_ret = FALSE;
  fake_cb_init_count = 0;
  for (i = 0; i < REPEAT; i++)
  {
    /* fails the call to cb_new */
    CU_ASSERT_PTR_NULL(aiofd_new(fileno(stdout), fileno(stdin), cb))
  }
  fake_cb_init = FALSE;
}

static void test_aiofd_new_fail_evt_init(void)
{
  int i;

  /* make sure there is an event loop */
  CU_ASSERT_PTR_NOT_NULL(el);

  /* make sure we have a callback manager */
  CU_ASSERT_PTR_NOT_NULL(cb);

  fake_event_init = TRUE;
  fake_event_init_ret = FALSE;
  for (i = 0; i < REPEAT; i++)
  {
    /* fails the second call to evt_new_io_event */
    fake_event_init_count = 1;
    CU_ASSERT_PTR_NULL(aiofd_new(fileno(stdout), fileno(stdin), cb))
  }
  fake_event_init_count = 0;
  for (i = 0; i < REPEAT; i++)
  {
    /* fails the first call to evt_new_io_event */
    CU_ASSERT_PTR_NULL(aiofd_new(fileno(stdout), fileno(stdin), cb))
  }
  fake_event_init = FALSE;
}

static void test_aiofd_start_stop_write(void)
{
  aiofd_t *aiofd = NULL;

  /* make sure there is an event loop */
  CU_ASSERT_PTR_NOT_NULL(el);

  /* make sure we have a callback manager */
  CU_ASSERT_PTR_NOT_NULL(cb);

  aiofd = aiofd_new(fileno(stdout), fileno(stdin), cb);
  CU_ASSERT_PTR_NOT_NULL(aiofd);

  CU_ASSERT_FALSE(aiofd_enable_write_evt(NULL, TRUE, NULL));
  CU_ASSERT_FALSE(aiofd_enable_write_evt(NULL, FALSE, NULL));
  CU_ASSERT_FALSE(aiofd_enable_write_evt(NULL, TRUE, el));
  CU_ASSERT_FALSE(aiofd_enable_write_evt(NULL, FALSE, el));
  CU_ASSERT_TRUE(aiofd_enable_write_evt(aiofd, TRUE, el));
  CU_ASSERT_TRUE(aiofd_enable_write_evt(aiofd, FALSE, NULL));
  CU_ASSERT_TRUE(aiofd_enable_write_evt(aiofd, TRUE, el));
  CU_ASSERT_TRUE(aiofd_enable_write_evt(aiofd, FALSE, el));

  fake_event_start = TRUE;
  fake_event_start_ret = EVT_ERROR;
  CU_ASSERT_FALSE(aiofd_enable_write_evt(aiofd, TRUE, el));
  fake_event_start = FALSE;

  fake_event_stop = TRUE;
  fake_event_stop_ret = EVT_ERROR;
  CU_ASSERT_FALSE(aiofd_enable_write_evt(aiofd, FALSE, NULL));
  fake_event_stop = FALSE;

  aiofd_delete(aiofd);
}

static void test_aiofd_start_stop_read(void)
{
  aiofd_t *aiofd = NULL;

  /* make sure there is an event loop */
  CU_ASSERT_PTR_NOT_NULL(el);

  /* make sure we have a callback manager */
  CU_ASSERT_PTR_NOT_NULL(cb);

  aiofd = aiofd_new(fileno(stdout), fileno(stdin), cb);
  CU_ASSERT_PTR_NOT_NULL(aiofd);

  CU_ASSERT_FALSE(aiofd_enable_read_evt(NULL, TRUE, NULL));
  CU_ASSERT_FALSE(aiofd_enable_read_evt(NULL, FALSE, NULL));
  CU_ASSERT_FALSE(aiofd_enable_read_evt(NULL, TRUE, el));
  CU_ASSERT_FALSE(aiofd_enable_read_evt(NULL, FALSE, el));
  CU_ASSERT_TRUE(aiofd_enable_read_evt(aiofd, TRUE, el));
  CU_ASSERT_TRUE(aiofd_enable_read_evt(aiofd, FALSE, NULL));
  CU_ASSERT_TRUE(aiofd_enable_read_evt(aiofd, TRUE, el));
  CU_ASSERT_TRUE(aiofd_enable_read_evt(aiofd, FALSE, el));

  fake_event_start = TRUE;
  fake_event_start_ret = EVT_ERROR;
  CU_ASSERT_FALSE(aiofd_enable_read_evt(aiofd, TRUE, NULL));
  fake_event_start = FALSE;

  fake_event_stop = TRUE;
  fake_event_stop_ret = EVT_ERROR;
  CU_ASSERT_FALSE(aiofd_enable_read_evt(aiofd, FALSE, NULL));
  fake_event_stop = FALSE;

  aiofd_delete(aiofd);
}

static void test_aiofd_delete_null(void)
{
  aiofd_delete(NULL);
}

static void test_aiofd_new_prereqs(void)
{
  /* make sure there is an event loop */
  CU_ASSERT_PTR_NOT_NULL(el);

  /* make sure we have a callback manager */
  CU_ASSERT_PTR_NOT_NULL(cb);

  CU_ASSERT_PTR_NULL(aiofd_new(-1, -1, NULL));
  CU_ASSERT_PTR_NULL(aiofd_new(fileno(stdout), -1, NULL));
  CU_ASSERT_PTR_NULL(aiofd_new(fileno(stdout), fileno(stdin), NULL));
}

static void test_aiofd_write(void)
{
  aiofd_t *aiofd = NULL;
  int fd[2];
  uint8_t *buf = UT("foo");
  size_t n = 4;

  /* make sure there is an event loop */
  CU_ASSERT_PTR_NOT_NULL(el);

  /* make sure we have a callback manager */
  CU_ASSERT_PTR_NOT_NULL(cb);

  /* open the pipe */
  CU_ASSERT_NOT_EQUAL(pipe2(fd, O_NONBLOCK), -1);

  aiofd = aiofd_new(fd[1], -1, cb);
  CU_ASSERT_PTR_NOT_NULL(aiofd);

  CU_ASSERT_FALSE(aiofd_write(NULL, NULL, 0, NULL));
  CU_ASSERT_FALSE(aiofd_write(aiofd, NULL, 0, NULL));
  CU_ASSERT_FALSE(aiofd_write(aiofd, (void const*)buf, 0, NULL));

  write_evts = 0;
  error_evts = 0;
  writes = 0;
  CU_ASSERT_TRUE(aiofd_write(aiofd, (void const*)buf, 4, NULL));
  CU_ASSERT_EQUAL(error_evts, 0);
  CU_ASSERT_EQUAL(write_evts, 0);
  CU_ASSERT_EQUAL(writes, 0);

  /* queue up a write */
  CU_ASSERT_TRUE(aiofd_enable_write_evt(aiofd, TRUE, el));

  /* start the event loop to process the writes */
  evt_run(el);

  /* when we get here, the event loop has been stopped */
  CU_ASSERT_EQUAL(write_evts, 2);
  CU_ASSERT_EQUAL(error_evts, 0);
  CU_ASSERT_TRUE(writes > 0);

  /* close the file */
  close(fd[0]);
  close(fd[1]);

  aiofd_delete(aiofd);
}

static void test_aiofd_read(void)
{
  aiofd_t *aiofd;
  int fd[2];

  /* make sure there is an event loop */
  CU_ASSERT_PTR_NOT_NULL(el);

  /* make sure we have a callback manager */
  CU_ASSERT_PTR_NOT_NULL(cb);

  /* open the pipe */
  CU_ASSERT_NOT_EQUAL(pipe2(fd, O_NONBLOCK), -1);
  write(fd[1], "foo", 4);

  aiofd = aiofd_new(-1, fd[0], cb);
  CU_ASSERT_PTR_NOT_NULL(aiofd);

  CU_ASSERT_EQUAL(aiofd_read(NULL, NULL, 0), -1);
  CU_ASSERT_EQUAL(aiofd_read(aiofd, NULL, 0), -1);
  CU_ASSERT_EQUAL(aiofd_read(aiofd, gbuf, 0), -1);

  read_evts = 0;
  error_evts = 0;
  nreads = 0;
  reads = 0;
  usev = FALSE;
  have_read = 0;
  to_read = 4;
  MEMSET(gbuf, 0, BUFSIZE);

  /* turn on the read event */
  CU_ASSERT_TRUE(aiofd_enable_read_evt(aiofd, TRUE, el));

  /* start the event loop to process the reads */
  evt_run(el);

  /* when we get here, the event loop has been stopped */
  CU_ASSERT_EQUAL(read_evts, 1);
  CU_ASSERT_EQUAL(error_evts, 0);
  CU_ASSERT_TRUE(nreads > 0);
  CU_ASSERT_TRUE(reads > 0);
  CU_ASSERT_STRING_EQUAL("foo", gbuf);

  aiofd_delete(aiofd);
  close(fd[0]);
  close(fd[1]);
}

static void test_aiofd_writev(void)
{
  aiofd_t *aiofd;
  int fd[2];
  uint8_t *buf = UT("foo");
  struct iovec iov;

  MEMSET(&iov, 0, sizeof(struct iovec));

  iov.iov_base = (void*)buf;
  iov.iov_len = 4;

  /* make sure there is an event loop */
  CU_ASSERT_PTR_NOT_NULL(el);

  /* make sure we have a callback manager */
  CU_ASSERT_PTR_NOT_NULL(cb);

  /* open the pipe */
  CU_ASSERT_NOT_EQUAL(pipe2(fd, O_NONBLOCK), -1);

  aiofd = aiofd_new(fd[1], -1, cb);
  CU_ASSERT_PTR_NOT_NULL(aiofd);

  CU_ASSERT_FALSE(aiofd_writev(NULL, NULL, 0, NULL));
  CU_ASSERT_FALSE(aiofd_writev(aiofd, NULL, 0, NULL));
  CU_ASSERT_FALSE(aiofd_writev(aiofd, &iov, 0, NULL));

  write_evts = 0;
  error_evts = 0;
  writevs = 0;
  CU_ASSERT_TRUE(aiofd_writev(aiofd, &iov, 1, NULL));
  CU_ASSERT_EQUAL(error_evts, 0);
  CU_ASSERT_EQUAL(write_evts, 0);
  CU_ASSERT_EQUAL(writevs, 0);

  /* queue up a write */
  CU_ASSERT_TRUE(aiofd_enable_write_evt(aiofd, TRUE, el));

  /* start the event loop to process the writes */
  evt_run(el);

  /* when we get here, the event loop has been stopped */
  CU_ASSERT_EQUAL(write_evts, 2);
  CU_ASSERT_TRUE(writevs > 0);

  close(fd[0]);
  close(fd[1]);

  aiofd_delete(aiofd);
}

static void test_aiofd_readv(void)
{
  aiofd_t *aiofd;
  int fd[2];

  /* make sure there is an event loop */
  CU_ASSERT_PTR_NOT_NULL(el);

  /* make sure we have a callback manager */
  CU_ASSERT_PTR_NOT_NULL(cb);

  /* open the pipe */
  CU_ASSERT_NOT_EQUAL(pipe2(fd, O_NONBLOCK), -1);
  write(fd[1], "foo", 4);

  aiofd = aiofd_new(-1, fd[0], cb);
  CU_ASSERT_PTR_NOT_NULL(aiofd);

  CU_ASSERT_EQUAL(aiofd_readv(NULL, NULL, 0), -1);
  CU_ASSERT_EQUAL(aiofd_readv(aiofd, NULL, 0), -1);
  CU_ASSERT_EQUAL(aiofd_readv(aiofd, &giov, 0), -1);

  read_evts = 0;
  error_evts = 0;
  nreads = 0;
  reads = 0;
  usev = TRUE;
  have_read = 0;
  to_read = 4;
  MEMSET(gbuf, 0, BUFSIZE);
  giov.iov_base = (void*)gbuf;
  giov.iov_len = BUFSIZE;

  /* turn on the read event */
  CU_ASSERT_TRUE(aiofd_enable_read_evt(aiofd, TRUE, el));

  /* start the event loop to process the reads */
  evt_run(el);

  /* when we get here, the event loop has been stopped */
  CU_ASSERT_EQUAL(read_evts, 1);
  CU_ASSERT_EQUAL(error_evts, 0);
  CU_ASSERT_TRUE(nreads > 0);
  CU_ASSERT_TRUE(readvs > 0);
  CU_ASSERT_STRING_EQUAL("foo", (char*)giov.iov_base);

  aiofd_delete(aiofd);
  close(fd[0]);
  close(fd[1]);
}

static void test_aiofd_flush_null(void)
{
  CU_ASSERT_FALSE(aiofd_flush(NULL));
}

static int init_aiofd_suite(void)
{
  srand(0xDEADBEEF);
  reset_test_flags();

  /* set up the callback manager */
  cb = cb_new();
  ADD_READ_EVT_CB(cb, read_evt, &read_evts);
  ADD_WRITE_EVT_CB(cb, write_evt, &write_evts);
  ADD_ERROR_EVT_CB(cb, error_evt, &error_evts);
  ADD_READ_IO_CB(cb, read_io, &reads);
  ADD_WRITE_IO_CB(cb, write_io, &writes);
  ADD_READV_IO_CB(cb, readv_io, &readvs);
  ADD_WRITEV_IO_CB(cb, writev_io, &writevs);
  ADD_NREAD_IO_CB(cb, nread_io, &nreads);

  return 0;
}

static int deinit_aiofd_suite(void)
{
  /* clean up callback manager */
  cb_delete(cb);
  reset_test_flags();
  return 0;
}

static CU_pSuite add_aiofd_tests(CU_pSuite pSuite)
{
  ADD_TEST("new/delete of aiofd", test_aiofd_newdel);
  ADD_TEST("new of aiofd fail alloc", test_aiofd_new_fail_alloc);
  ADD_TEST("fail init of aiofd", test_aiofd_new_fail_init);
  ADD_TEST("fail aiofd list init", test_aiofd_new_fail_list_init);
  ADD_TEST("fail aiofd cb init", test_aiofd_new_fail_cb_init);
  ADD_TEST("fail aiofd evt init", test_aiofd_new_fail_evt_init);
  ADD_TEST("aiofd write start/stop", test_aiofd_start_stop_write);
  ADD_TEST("aiofd read start/stop", test_aiofd_start_stop_read);
  ADD_TEST("aiofd delete NULL", test_aiofd_delete_null);
  ADD_TEST("aiofd new prereqs", test_aiofd_new_prereqs);
  ADD_TEST("aiofd write", test_aiofd_write);
  ADD_TEST("aiofd read", test_aiofd_read);
  ADD_TEST("aiofd writev", test_aiofd_writev);
  ADD_TEST("aiofd readv", test_aiofd_readv);
  ADD_TEST("aiofd flush null", test_aiofd_flush_null);

  ADD_TEST("test aiofd private functions", test_aiofd_private_functions);
  return pSuite;
}

CU_pSuite add_aiofd_test_suite()
{
  CU_pSuite pSuite = NULL;

  /* add the suite to the registry */
  pSuite = CU_add_suite("Async I/O fd Tests", init_aiofd_suite, deinit_aiofd_suite);
  CHECK_PTR_RET(pSuite, NULL);

  /* add in aiofd specific tests */
  CHECK_PTR_RET(add_aiofd_tests(pSuite), NULL);

  return pSuite;
}

