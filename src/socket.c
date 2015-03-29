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
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <poll.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <netinet/tcp.h>

#include "debug.h"
#include "macros.h"
#include "cb.h"
#include "events.h"
#include "list.h"
#include "socket.h"

#if defined(UNIT_TESTING)
#include "test_flags.h"
extern evt_loop_t * el;
#endif

typedef struct addrinfo addrinfo_t;

typedef struct write_dst_s
{
  sockaddr_t        addr;
  socklen_t         addrlen;
} write_dst_t;

struct socket_s
{
  socket_type_t   type;           /* type of socket */
  int             fd;             /* fd for socket */
  int_t           connected;      /* is the socket connected? */
  int_t           bound;          /* is the socket bound? */
  int_t           listening;      /* are we listening? */
  uint8_t        *host;           /* host name */
  uint8_t        *port;           /* port number as string */
  int             ai_flags;       /* addr info flags */
  int             ai_family;      /* addr info family */
  sockaddr_t      addr;           /* place to stash the sockaddr_storage data */
  socklen_t       addrlen;        /* length of addr */
  sockaddr_t     *readaddr;       /* place to put addr for UDP read */
  socklen_t      *readaddrlen;    /* place to put addr len for UDP read */
  cb_t           *cb;             /* callbacks manager */
  cb_t           *int_cb;         /* internal callback manager */
  aiofd_t        *aiofd;          /* the async fd */
  evt_loop_t     *el;             /* the event loop for this socket */
};

uint8_t const * const socket_cb[S_CB_COUNT] =
{
  UT("socket-connect-evt"),
  UT("socket-disconnect-evt"),
  UT("socket-error-evt"),
  UT("socket-read-evt"),
  UT("socket-write-evt")
};

/* helper macros for calling callbacks */
#define S_CONN_EVT(cb, ...) cb_call(cb, S_CB_NAME(S_CONN_EVT), __VA_ARGS__)
#define S_DISC_EVT(cb, ...) cb_call(cb, S_CB_NAME(S_DISC_EVT), __VA_ARGS__)
#define S_ERR_EVT(cb, ...) cb_call(cb, S_CB_NAME(S_ERR_EVT), __VA_ARGS__)
#define S_READ_EVT(cb, ...) cb_call(cb, S_CB_NAME(S_READ_EVT), __VA_ARGS__)
#define S_WRITE_EVT(cb, ...) cb_call(cb, S_CB_NAME(S_WRITE_EVT), __VA_ARGS__)

/*
 * SOCKET EVENT CALLBACKS
 */

static void s_read_evt(socket_t *s, aiofd_t *a, size_t n);
static void s_write_evt(socket_t *s, write_dst_t *wd, aiofd_t *a, void *buf,
                        size_t n);
static void s_error_evt(socket_t *s, write_dst_t *wd, aiofd_t *a, int eno);
static void s_read_io(socket_t *s, aiofd_t *a, int fd, void *buf, size_t n,
                      ssize_t *r);
static void s_write_io(socket_t *s, write_dst_t *wd, aiofd_t *a, int fd,
                       void const *buf, size_t n, ssize_t *r);
static void s_readv_io(socket_t *s, aiofd_t *a, int fd, struct iovec *iov,
                       size_t n, ssize_t *r);
static void s_writev_io(socket_t *s, write_dst_t *wd, aiofd_t *a, int fd,
                        struct iovec const *iov, size_t n, ssize_t *r);
static void s_nread_io(socket_t *s, aiofd_t *a, int fd, size_t *n, int_t *l,
                       int *r);

/* declare the callbacks */
READ_EVT_CB(s_read_evt, socket_t*);
WRITE_EVT_CB(s_write_evt, socket_t*, write_dst_t*);
ERROR_EVT_CB(s_error_evt, socket_t*, write_dst_t*);
READ_IO_CB(s_read_io, socket_t*);
WRITE_IO_CB(s_write_io, socket_t*, write_dst_t*);
READV_IO_CB(s_readv_io, socket_t*);
WRITEV_IO_CB(s_writev_io, socket_t*, write_dst_t*);
NREAD_IO_CB(s_nread_io, socket_t*);

/*
 * HELPER FUNCTIONS
 */
static int_t s_init(socket_t *s, socket_type_t t, cb_t *cb,
                    uint8_t const *host, uint8_t const *port,
                    int ai_flags, int ai_family);
static void s_deinit(socket_t * s);
static int_t s_open(socket_t *s);
static int_t s_close(socket_t *s);
static int_t s_open_tcp(socket_t *s);
static int_t s_open_udp(socket_t *s);
static int_t s_open_unix(socket_t *s);
#ifdef DEBUG_ON
static uint8_t * s_get_addr_flags_string(int flags);
static uint8_t * s_get_addrinfo_string(addrinfo_t const *info);
#endif
static inline void * s_in_addr(sockaddr_t const *addr);
static inline in_port_t s_in_port(sockaddr_t const *addr);
static inline int_t s_validate_port(uint8_t const *port);
static int_t s_get_error(socket_t *s, int *errval);


socket_t * socket_new(socket_type_t t, cb_t *cb,
                      uint8_t const * host, uint8_t const * port,
                      int ai_flags, int ai_family)
{
  socket_t* s = NULL;

  /* allocate the socket struct */
  s = CALLOC(1, sizeof(socket_t));
  CHECK_PTR_RET(s, NULL);

  /* initlialize the socket */
  CHECK_GOTO(s_init(s, t, cb, host, port, ai_flags, ai_family), _s_new_1);

  return s;

_s_new_1:
  DEBUG("socket_new failure: %s\n", check_err_str_);
  FREE(s);
  return NULL;
}

void socket_delete(void * s)
{
  socket_t *sock = (socket_t*)s;
  CHECK_PTR(sock);
  s_deinit(sock);
  FREE(sock);
}

socket_ret_t socket_connect(socket_t *s, evt_loop_t *el)
{
  CHECK_PTR_RET(s, SOCKET_BADPARAM);
  CHECK_RET(!socket_is_connected(s), SOCKET_CONNECTED);
  CHECK_RET(VALID_SOCKET_TYPE(s->type), SOCKET_ERROR);

  /* first open the socket if needed */
  if(!s->aiofd)
    CHECK_RET(s_open(s), SOCKET_ERROR);

  /* try to make the connection */
  if(CONNECT(s->fd, (struct sockaddr*)&(s->addr), s->addrlen) < 0)
  {
    if(ERRNO != EINPROGRESS)
    {
      DEBUG("failed to initiate connect to the server\n");
      return SOCKET_CONNECT_FAIL;
    }
    DEBUG("connection in progress\n");
  }

  if(s->type != SOCKET_UDP)
  {
    /* start the socket write event processing to catch successful connect */
    CHECK_RET(aiofd_enable_write_evt(s->aiofd, TRUE, el), SOCKET_CONNECT_FAIL);

    /* store the event loop so we can enable read events if the connect
     * succeeds */
    s->el = el;
  }

  return SOCKET_OK;
}

socket_ret_t socket_disconnect(socket_t *s)
{
  CHECK_PTR_RET(s, SOCKET_BADPARAM);

  /* close the socket */
  CHECK_RET(s_close(s), SOCKET_ERROR);

  /* we're now disconnected */
  s->connected = FALSE;

  /* call the disconnect event callback */
  if(s->type != SOCKET_UDP)
  {
    S_DISC_EVT(s->cb, s);
  }

  return SOCKET_OK;
}

socket_ret_t socket_bind(socket_t *s)
{
  int r, on = 1;
  CHECK_PTR_RET(s, SOCKET_BADPARAM);
  CHECK_RET(!socket_is_bound(s), SOCKET_BOUND);
  CHECK_RET(VALID_SOCKET_TYPE(s->type), SOCKET_ERROR);

  /* first open the socket if needed */
  if(!s->aiofd)
    CHECK_RET(s_open(s), SOCKET_ERROR);

  /* turn on address reuse if we can */
  if((s->type == SOCKET_UDP) || (s->type == SOCKET_TCP))
  {
    r = SETSOCKOPT(s->fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    CHECK_GOTO(r == 0, _bind_fail_1);
    DEBUG("turned on IP socket address reuse\n");
  }

  /* bind the socket */
  r = BIND(s->fd, (struct sockaddr const *)&(s->addr), s->addrlen);
  CHECK_GOTO(r == 0, _bind_fail_1);

  DEBUG("socket is bound %p fd: %d\n", (void*)s, s->fd);

  /* flag the socket as bound */
  s->bound = TRUE;

  return SOCKET_OK;

_bind_fail_1:
  s_close(s);
  return SOCKET_ERROR;
}

socket_ret_t socket_listen(socket_t * s, int backlog, evt_loop_t *el)
{
  CHECK_PTR_RET(s, SOCKET_BADPARAM);
  CHECK_RET(socket_is_bound(s), SOCKET_BOUND);
  CHECK_RET(!socket_is_connected(s), SOCKET_CONNECTED);
  CHECK_PTR_RET(el, SOCKET_BADPARAM);
  CHECK_PTR_RET(s->aiofd, SOCKET_BADPARAM);

  /* don't call listen on UDP sockets */
  CHECK_RET(s->type != SOCKET_UDP, SOCKET_ERROR);

  /* start the socket read event processing to catch incoming connections */
  CHECK_RET(aiofd_enable_read_evt(s->aiofd, TRUE, el), SOCKET_ERROR);

  /* now begin listening for incoming connections */
  if(LISTEN(s->fd, backlog) < 0)
  {
    DEBUG("failed to listen (errno: %d)\n", errno);
    aiofd_enable_read_evt(s->aiofd, FALSE, NULL);
    return SOCKET_ERROR;
  }

  DEBUG("socket is listening %p fd: %d\n", (void*)s, s->fd);

  /* flag the socket as listening */
  s->listening = TRUE;

  return SOCKET_OK;
}

socket_t * socket_accept(socket_t *s, cb_t *cb, evt_loop_t *el)
{
  socket_t *c;

  CHECK_PTR_RET(s, NULL);
  CHECK_PTR_RET(el, NULL);
  CHECK_RET(socket_is_bound(s), NULL);
  CHECK_RET(VALID_SOCKET_TYPE(s->type), NULL);

  /* don't call accept on UDP sockets */
  CHECK_RET(s->type != SOCKET_UDP, NULL);

  /* create a new socket for the incoming connection */
  c = CALLOC(1, sizeof(socket_t));
  CHECK_PTR_RET_MSG(c, NULL, "failed to allocate new socket struct\n");

  /* initlialize the socket */
  CHECK_GOTO(s_init(c, s->type, cb, NULL, NULL, 0, 0), _accept_fail_1);

  /* accept the incoming connection */
  c->addrlen = sizeof(c->addr);
  c->fd = ACCEPT(s->fd, (struct sockaddr *)&(c->addr), &(c->addrlen));
  CHECK_GOTO(c->fd >= 0, _accept_fail_2);

  /* initialize the connection info */
  switch(s->type)
  {
    case SOCKET_UDP:
      break;
    case SOCKET_TCP:
      /* fill in the host string */
      FREE(c->host);
      c->host = CALLOC(HOST_BUF_LEN, sizeof(uint8_t));
      CHECK_PTR_GOTO(c->host, _accept_fail_2);
      CHECK_GOTO(socket_addr_str(&(c->addr), c->host, HOST_BUF_LEN), _accept_fail_2);

      /* fill in the port string */
      FREE(c->port);
      c->port = CALLOC(PORT_BUF_LEN, sizeof(uint8_t));
      CHECK_PTR_GOTO(c->port, _accept_fail_2);
      CHECK_GOTO(socket_port_str(&(c->addr), c->port, PORT_BUF_LEN), _accept_fail_2);
      break;

    case SOCKET_UNIX:
      /* store the connection information */
      c->host = UT(STRDUP(((struct sockaddr_un*)&(c->addr))->sun_path));
      CHECK_PTR_GOTO(c->host, _accept_fail_2);
      break;
  }

  /* set everthing else up */
  CHECK_GOTO(s_open(c), _accept_fail_2);

  DEBUG("socket connected\n");
  c->connected = TRUE;

  /* we're connected so start read event */
  CHECK_GOTO(aiofd_enable_read_evt(c->aiofd, TRUE, el), _accept_fail_2);

  return c;

_accept_fail_2:
  s_deinit(c);
_accept_fail_1:
  DEBUG("socket_accept failure: %s\n", check_err_str_);
  DEBUG("ERRNO is: %d -- %s\n", ERRNO, strerror(ERRNO));
  FREE(c);
  return NULL;
}

int_t socket_addr_str(sockaddr_t const *addr, uint8_t *buf, size_t n)
{
  CHECK_PTR_RET(addr, FALSE);
  CHECK_PTR_RET(buf, FALSE);
  CHECK_RET(n > 0, FALSE);

  CHECK_PTR_RET(inet_ntop(addr->ss_family, s_in_addr(addr), C(buf), n), FALSE);
  /* make sure it is null terminated */
  buf[n-1] = '\0';
  return TRUE;
}

int_t socket_port_str(sockaddr_t const *addr, uint8_t *buf, size_t n)
{
  CHECK_PTR_RET(addr, FALSE);
  CHECK_PTR_RET(buf, FALSE);
  CHECK_RET(n > 0, FALSE);

  CHECK_RET(snprintf(C(buf), n, "%hu", ntohs(s_in_port(addr))) >= 0, FALSE);
  /* make sure it is null terminated */
  buf[n-1] = '\0';
  return TRUE;
}

int_t socket_addr(socket_t const * s,
                       sockaddr_t * addr,
                       socklen_t * len)
{
    CHECK_PTR_RET(s, FALSE);
    CHECK_PTR_RET(addr, FALSE);
    CHECK_PTR_RET(len, FALSE);

    MEMCPY(addr, &(s->addr), sizeof(sockaddr_t));
    *(len) = s->addrlen;
    return TRUE;
}


/* check to see if connected */
int_t socket_is_connected(socket_t const * s)
{
    UNIT_TEST_RET(socket_connected);

    CHECK_PTR_RET(s, FALSE);
    CHECK_RET(s->fd >= 0, FALSE);
    return s->connected;
}


/* check to see if bound */
int_t socket_is_bound(socket_t const * s)
{
    UNIT_TEST_RET(socket_bound);

    CHECK_PTR_RET(s, FALSE);
    CHECK_RET(s->fd >= 0, FALSE);
    return s->bound;
}


int_t socket_is_listening(socket_t const * s)
{
    CHECK_PTR_RET(s, FALSE);
    return s->listening;
}

socket_type_t socket_get_type(socket_t * s)
{
    CHECK_PTR_RET(s, SOCKET_UNKNOWN);
    return s->type;
}

ssize_t socket_read(socket_t* s,
                     uint8_t * buffer,
                     size_t n)
{
    CHECK_PTR_RET(s, -1);
    CHECK_PTR_RET(buffer, -1);
    CHECK_RET(n > 0, -1);

    /* can't use this with UDP sockets that aren't connected */
    if ((s->type == SOCKET_UDP) && !socket_is_bound(s))
        return SOCKET_ERROR;

    return aiofd_read(s->aiofd, buffer, n);
}

ssize_t socket_readv(socket_t * s,
                      struct iovec * iov,
                      size_t iovcnt)
{
    CHECK_PTR_RET(s, -1);
    CHECK_PTR_RET(iov, -1);
    CHECK_RET(iovcnt > 0, -1);

    /* can't use this with UDP sockets that aren't connected */
    if ((s->type == SOCKET_UDP) && !socket_is_bound(s))
        return SOCKET_ERROR;

    return aiofd_readv(s->aiofd, iov, iovcnt);
}

ssize_t socket_read_from(socket_t* s,
                          uint8_t * buffer,
                          size_t n,
                          sockaddr_t * addr,
                          socklen_t * addrlen)
{
    CHECK_PTR_RET(s, -1);
    CHECK_PTR_RET(buffer, -1);
    CHECK_RET(n > 0, -1);

    /* initialize the place to put the source addr */
    s->readaddr = addr;
    s->readaddrlen = addrlen;

    return aiofd_read(s->aiofd, buffer, n);
}

ssize_t socket_readv_from(socket_t * s,
                           struct iovec * iov,
                           size_t iovcnt,
                           sockaddr_t * addr,
                           socklen_t * addrlen)
{
    CHECK_PTR_RET(s, -1);
    CHECK_PTR_RET(iov, -1);
    CHECK_RET(iovcnt > 0, -1);

    /* initialize the place to put the source addr */
    s->readaddr = addr;
    s->readaddrlen = addrlen;

    return aiofd_readv(s->aiofd, iov, iovcnt);
}

socket_ret_t socket_write(socket_t * s,
                           uint8_t const * buffer,
                           size_t n)
{
    CHECK_PTR_RET(s, SOCKET_BADPARAM);

    /* can't use this with UDP sockets that aren't connected */
    if ((s->type == SOCKET_UDP) && !socket_is_connected(s))
        return SOCKET_ERROR;

    return (aiofd_write(s->aiofd, buffer, n, NULL) ? SOCKET_OK : SOCKET_ERROR);
}

socket_ret_t socket_writev(socket_t * s,
                            struct iovec const * iov,
                            size_t iovcnt)
{
    CHECK_PTR_RET(s, SOCKET_BADPARAM);

    /* can't use this with UDP sockets that aren't connected */
    if ((s->type == SOCKET_UDP) && !socket_is_connected(s))
        return SOCKET_ERROR;

    return (aiofd_writev(s->aiofd, iov, iovcnt, NULL) ? SOCKET_OK : SOCKET_ERROR);
}

socket_ret_t socket_write_to(socket_t * s,
                              uint8_t const * buffer,
                              size_t n,
                              sockaddr_t const * addr,
                              socklen_t addrlen)
{
    write_dst_t * wd = NULL;
    CHECK_PTR_RET(s, SOCKET_BADPARAM);

    /* we'll get the pointer to this memory back in the write callback */
    wd = CALLOC(1, sizeof(write_dst_t));
    if (wd == NULL)
    {
        DEBUG("failed to calloc write destination struct\n");
        return SOCKET_ERROR;
    }

    /* copy the destination information */
    MEMCPY(&(wd->addr), addr, sizeof(sockaddr_t));
    wd->addrlen = addrlen;

    return (aiofd_write(s->aiofd, buffer, n, (void*)wd) ? SOCKET_OK : SOCKET_ERROR);
}

socket_ret_t socket_writev_to(socket_t * s,
                               struct iovec const * iov,
                               size_t iovcnt,
                               sockaddr_t const * addr,
                               socklen_t addrlen)
{
    write_dst_t * wd = NULL;
    CHECK_PTR_RET(s, SOCKET_BADPARAM);

    /* we'll get the pointer to this memory back in the write callback */
    wd = CALLOC(1, sizeof(write_dst_t));
    if (wd == NULL)
    {
        DEBUG("failed to calloc write destination struct\n");
        return SOCKET_ERROR;
    }

    /* copy the destination information */
    MEMCPY(&(wd->addr), addr, sizeof(sockaddr_t));
    wd->addrlen = addrlen;

    return (aiofd_writev(s->aiofd, iov, iovcnt, (void*)wd) ? SOCKET_OK : SOCKET_ERROR);
}

/* flush the socket output */
socket_ret_t socket_flush(socket_t* s)
{
    CHECK_PTR_RET(s, SOCKET_BADPARAM);
    return aiofd_flush(s->aiofd);
}

/*
 * PRIVATE
 */

static int_t s_init(socket_t *s, socket_type_t t, cb_t *cb,
                    uint8_t const *host, uint8_t const *port,
                    int ai_flags, int ai_family)
{
  UNIT_TEST_FAIL(s_init);

  CHECK_PTR_RET(s, FALSE);
  CHECK_RET(VALID_SOCKET_TYPE(t), FALSE);

  MEMSET((void*)s, 0, sizeof(socket_t));

  s->type = t;
  s->fd = -1;
  if (port != NULL)
  {
    s->port = UT(STRDUP(C(port)));
    CHECK_PTR_GOTO(s->port, _s_init_1);
  }

  if(host != NULL)
  {
    s->host = UT(STRDUP(C(host)));
    CHECK_PTR_GOTO(s->host, _s_init_2);
  }

  /* store addr info fields */
  s->ai_flags = ai_flags;
  s->ai_family = ai_family;

  /* store the user callbacks */
  s->cb = cb;

  /* hook up our internal callbacks */
  s->int_cb = cb_new();
  CHECK_PTR_GOTO(s->int_cb, _s_init_3);
  ADD_READ_EVT_CB(s->int_cb, s_read_evt, s);
  ADD_WRITE_EVT_CB(s->int_cb, s_write_evt, s);
  ADD_ERROR_EVT_CB(s->int_cb, s_error_evt, s);
  ADD_READ_IO_CB(s->int_cb, s_read_io, s);
  ADD_WRITE_IO_CB(s->int_cb, s_write_io, s);
  ADD_READV_IO_CB(s->int_cb, s_readv_io, s);
  ADD_WRITEV_IO_CB(s->int_cb, s_writev_io, s);
  ADD_NREAD_IO_CB(s->int_cb, s_nread_io, s);

_s_init_3:
  FREE(s->host);
_s_init_2:
  FREE(s->port);
_s_init_1:
  MEMSET((void*)s, 0, sizeof(socket_t));
  return FALSE;
}

static void s_deinit(socket_t * s)
{
  CHECK_PTR(s);
  socket_disconnect(s);
  FREE(s->host);
  FREE(s->port);
  cb_delete(s->int_cb);
  aiofd_delete(s->aiofd);
}

static int_t s_open(socket_t *s)
{
  CHECK_PTR_RET(s, FALSE);
  CHECK_RET(VALID_SOCKET_TYPE(s->type), FALSE);

  /* open the socket */
  switch(s->type)
  {
    case SOCKET_TCP:
      return s_open_tcp(s);
    case SOCKET_UDP:
      return s_open_udp(s);
    case SOCKET_UNIX:
      return s_open_unix(s);
  }
  return FALSE;
}

static int_t s_close(socket_t *s)
{
  struct stat st;
  CHECK_PTR_RET(s, FALSE);
  CHECK_RET(VALID_SOCKET_TYPE(s->type), FALSE);

  SHUTDOWN(s->fd, SHUT_RDWR);
  CHECK_RET(aiofd_enable_write_evt(s->aiofd, FALSE, NULL), FALSE);
  CHECK_RET(aiofd_enable_read_evt(s->aiofd, FALSE, NULL), FALSE);
  aiofd_delete(s->aiofd);
  s->aiofd = NULL;
  CLOSE(s->fd);
  s->fd = -1;

  if(s->type == SOCKET_UNIX)
  {
    /* do not unlink without checking first */
    MEMSET(&st, 0, sizeof(struct stat));
    if(STAT(C(s->host), &st) == 0)
    {
      /* we will only unlink a socket node, all other node types
       * are an error */
      if((st.st_mode & S_IFMT) == S_IFSOCK)
      {
        /* unlink the socket node */
        UNLINK(C(s->host));
      }
    }
  }

  return TRUE;
}

static int_t s_open_tcp(socket_t * s)
{
  struct addrinfo hints, *info, *p;
  int r, on = 1;
  int32_t flags;

  UNIT_TEST_RET(socket_create_tcp_socket);

  CHECK_PTR_RET(s, FALSE);
  CHECK_RET(s->type == SOCKET_TCP, FALSE);
  CHECK_RET(s->port != NULL, FALSE);

  /* open a socket if we don't already have one from a call to accept */
  if(s->fd == -1)
  {
    /* zero out the hints */
    MEMSET(&hints, 0, sizeof(struct addrinfo));

    hints.ai_family = s->ai_family;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = s->ai_flags;
    hints.ai_protocol = IPPROTO_TCP;

    r = GETADDRINFO(C(s->host), C(s->port), &hints, &info);
    CHECK_GOTO(r == 0, _open_tcp_fail_1);

    /* try all of the different family/socktype/protocol types */
    for(p = info; (p != NULL) && (s->fd == -1); p = p->ai_next)
    {
      /* try to open a socket */
      if((s->fd = SOCKET(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
      {
        DEBUG("failed to open TCP socket\n");
        continue;
      }
      DEBUG("created TCP socket [%s]\n", s_get_addrinfo_string(p));

      /* store the address info */
      MEMSET(&(s->addr), 0, sizeof(sockaddr_t));
      MEMCPY(&(s->addr), p->ai_addr, p->ai_addrlen);
      s->addrlen = p->ai_addrlen;
    }

    /* release the addr info data */
    FREEADDRINFO(info);
  }

  /* turn off TCP naggle algorithm */
  r = SETSOCKOPT(s->fd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));
  CHECK_GOTO(r == 0, _open_tcp_fail_1);
  DEBUG("turned on TCP no delay\n");

  /* set the socket to non blocking mode */
  flags = FCNTL(s->fd, F_GETFL);
  r = FCNTL(s->fd, F_SETFL, (flags | O_NONBLOCK));
  CHECK_GOTO(r != -1, _open_tcp_fail_1);
  DEBUG("TCP socket is now non-blocking\n");

  /* create the aiofd */
  s->aiofd = aiofd_new(s->fd, s->fd, s->int_cb);
  CHECK_PTR_GOTO(s->aiofd, _open_tcp_fail_1);
  DEBUG("aiofd initialized\n");

  return TRUE;

_open_tcp_fail_1:
  CLOSE(s->fd);
  s->fd = -1;
  return FALSE;
}

static int_t s_open_udp(socket_t *s)
{
  struct addrinfo hints, *info, *p;
  int r;
  int32_t flags;

  UNIT_TEST_RET(socket_create_udp_socket);

  CHECK_PTR_RET(s, FALSE);
  CHECK_RET(s->type == SOCKET_UDP, FALSE);
  CHECK_RET(s->port != NULL, FALSE);

  /* zero out the hints */
  MEMSET(&hints, 0, sizeof(struct addrinfo));

  hints.ai_family = s->ai_family;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = s->ai_flags;
  hints.ai_protocol = IPPROTO_UDP;

  r = GETADDRINFO(C(s->host), C(s->port), &hints, &info);
  CHECK_GOTO(r == 0, _open_udp_fail_1);

  for(p = info; (p != NULL) && (s->fd == -1); p = p->ai_next)
  {
    /* try to open a socket */
    if ((s->fd = SOCKET(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
    {
      DEBUG("failed to open UDP socket\n");
      continue;
    }
    DEBUG("created UDP socket [%s]\n", s_get_addrinfo_string(p));

    /* store the address info */
    MEMSET(&(s->addr), 0, sizeof(sockaddr_t));
    MEMCPY(&(s->addr), p->ai_addr, p->ai_addrlen);
    s->addrlen = p->ai_addrlen;
  }

  /* release the addr info data */
  FREEADDRINFO(info);

  /* set the socket to non blocking mode */
  flags = FCNTL(s->fd, F_GETFL);
  r = FCNTL(s->fd, F_SETFL, (flags | O_NONBLOCK));
  CHECK_GOTO(r != -1, _open_udp_fail_1);
  DEBUG("UDP socket is now non-blocking\n");

  /* create the aiofd */
  s->aiofd = aiofd_new(s->fd, s->fd, s->int_cb);
  CHECK_PTR_GOTO(s->aiofd, _open_udp_fail_1);
  DEBUG("aiofd initialized\n");

#if 0
  DEBUG("UDP socket events enabled: %p\n", (void*)s);
  /* start the socket read event processing... */
  aiofd_enable_read_evt(s->aiofd, TRUE);
  /* start the socket write event processing... */
  aiofd_enable_write_evt(s->aiofd, TRUE);
#endif

  return TRUE;

_open_udp_fail_1:
  CLOSE(s->fd);
  s->fd = -1;
  return FALSE;
}

static int_t s_open_unix(socket_t * s)
{
  int r;
  int32_t flags;

  UNIT_TEST_RET(socket_create_unix_socket);

  CHECK_PTR_RET(s, FALSE);
  CHECK_RET(s->type == SOCKET_UNIX, FALSE);
  CHECK_RET(s->host != NULL, FALSE);
  CHECK_RET(s->port == NULL, FALSE);

  if(s->fd == -1)
  {
    /* try to open a socket */
    if ((s->fd = SOCKET(PF_UNIX, SOCK_STREAM, 0)) < 0)
    {
      DEBUG("failed to open UNIX socket\n");
      return FALSE;
    }

    DEBUG("created UNIX socket\n");

    /* initialize the socket address struct */
    MEMSET(&(s->addr), 0, sizeof(sockaddr_t));
    s->addr.ss_family = AF_UNIX;
    s->addrlen = sizeof(struct sockaddr_un);

    /* store the connection information */
    s->host = UT(STRDUP(((struct sockaddr_un*)&(s->addr))->sun_path));
    CHECK_PTR_GOTO(s->host, _open_unix_fail_1);
  }

  /* set the socket to non blocking mode */
  flags = FCNTL(s->fd, F_GETFL);
  r = FCNTL(s->fd, F_SETFL, (flags | O_NONBLOCK));
  CHECK_GOTO(r != -1, _open_unix_fail_1);
  DEBUG("UNIX socket is now non-blocking\n");

  /* create the aiofd */
  s->aiofd = aiofd_new(s->fd, s->fd, s->int_cb);
  CHECK_PTR_GOTO(s->aiofd, _open_unix_fail_1);
  DEBUG("aiofd initialized\n");

  return TRUE;

_open_unix_fail_1:
  CLOSE(s->fd);
  s->fd = -1;
  return FALSE;
}

#ifdef DEBUG_ON
static uint8_t * s_get_addr_flags_string(int flags)
{
  static uint8_t buf[4096];
  uint8_t * p;
  MEMSET(buf, 0, 4096);
  p = &buf[0];

  if (flags & AI_NUMERICHOST)
  {
      p += sprintf(C(p), "AI_NUMERICHOST,");
  }
  if (flags & AI_PASSIVE)
  {
      p += sprintf(C(p), "AI_PASSIVE,");
  }
  if (flags & AI_CANONNAME)
  {
      p += sprintf(C(p), "AI_CANONNAME,");
  }
  if (flags & AI_ADDRCONFIG)
  {
      p += sprintf(C(p), "AI_ADDRCONFIG,");
  }
  if (flags & AI_V4MAPPED)
  {
      p += sprintf(C(p), "AI_V4MAPPED,");
  }
  if (flags & AI_ALL)
  {
      p += sprintf(C(p), "AI_ALL,");
  }
#if 0
  if (flags & AI_IDN)
  {
      p += sprintf(C(p), "AI_IDN");
  }
  if (flags & AI_CANONIDN)
  {
      p += sprintf(C(p), "AI_CANONIDN");
  }
  if (flags & AI_IDN_ALLOW_UNASSIGNED)
  {
      p += sprintf(C(p), "AI_IDN_ALLOW_UNASSIGNED");
  }
  if (flags & AI_IDN_USE_STD3_ASCII_RULES)
  {
      p += sprintf(C(p), "AI_IDN_USE_STD3_ASCII_RULES");
  }
#endif
  return &buf[0];
}

static uint8_t * s_get_addrinfo_string(addrinfo_t const * info)
{
  static uint8_t buf[4096];
  MEMSET(buf, 0, 4096);

  CHECK_PTR_RET(info, buf);

  sprintf(C(buf), "Family: %s, Type: %s, Proto: %s, Flags: %s",
          (info->ai_family == AF_INET ? "AF_INET" :
            (info->ai_family == AF_INET6 ? "AF_INET6" :
            (info->ai_family == AF_UNSPEC ? "AF_UNSPEC" : "UNKNOWN"))),
          (info->ai_socktype == SOCK_STREAM ? "SOCK_STREAM" :
            (info->ai_socktype == SOCK_DGRAM ? "SOCK_DGRAM" : "UNKNOWN")),
          (info->ai_protocol == IPPROTO_TCP ? "IPPROTO_TCP" :
            (info->ai_protocol == IPPROTO_UDP ? "IPPROTO_UDP" : "UNKNOWN")),
          C(s_get_addr_flags_string(info->ai_flags)));
  return &buf[0];
}
#endif

static inline void * s_in_addr(sockaddr_t const *addr)
{
  CHECK_PTR_RET(addr, NULL);
  if (addr->ss_family == AF_INET)
    return &(((struct sockaddr_in*)addr)->sin_addr);
  else
    return &(((struct sockaddr_in6*)addr)->sin6_addr);
}

static inline in_port_t s_in_port(sockaddr_t const *addr)
{
  CHECK_PTR_RET(addr, 0);
  if (addr->ss_family == AF_INET)
    return ((struct sockaddr_in*)addr)->sin_port;
  else
    return ((struct sockaddr_in6*)addr)->sin6_port;
}

static inline int_t s_validate_port(uint8_t const * port)
{
  uint8_t * endp = NULL;
  uint32_t pnum = 0;

  CHECK_PTR_RET(port, FALSE);

  /* convert port string to number */
  pnum = STRTOL(C(port), (char**)&endp, 10);

  /* make sure we parsed something */
  CHECK_RET(endp > port, FALSE);

  /* make sure there wasn't any garbage */
  CHECK_RET(*endp == '\0', FALSE);

  /* make sure the port is somewhere in the valid range */
  CHECK_RET(((pnum >= 0) && (pnum <= 65535)), FALSE);

  return TRUE;
}

static int_t s_get_error(socket_t *s, int *errval)
{
  socklen_t len = sizeof(int);
  CHECK_PTR_RET(s, FALSE);
  CHECK_PTR_RET(errval, FALSE);
#if defined(UNIT_TESTING)
  if (fake_socket_getsockopt)
  {
    *errval = fake_socket_errval;
    return fake_s_get_error_ret;
  }
#endif

  CHECK_RET(getsockopt(s->fd, SOL_SOCKET, SO_ERROR, errval, &len) == 0, FALSE);
  return TRUE;
}


static void s_read_evt(socket_t *s, aiofd_t *a, size_t n)
{
  socket_ret_t ret = SOCKET_OK;
  CHECK_PTR(s);
  CHECK_PTR(a);

  DEBUG("read callback for socket %p\n", (void*)s);

  if(s->type == SOCKET_UDP)
  {
    /* this is a normal connection socket, so pass the read event along */
    DEBUG("calling socket read callback for socket %p\n", (void*)s);
    S_READ_EVT(s->cb, s, n);
  }
  else
  {
    if(socket_is_bound(s) && socket_is_listening(s))
    {
      DEBUG("calling connect callback for incoming connection %p\n", (void*)s);
      S_CONN_EVT(s->cb, s, &ret);
      if(ret != SOCKET_OK)
      {
        DEBUG("failed to accept incoming connection!\n");
        /* stop the read event */
        aiofd_enable_read_evt(a, FALSE, NULL);
      }
    }
    else
    {
      if(n == 0)
      {
        /* the other side disconnected, so follow suit */
        socket_disconnect(s);

        /* stop the read event */
        aiofd_enable_read_evt(a, FALSE, NULL);
        return;
      }

      /* this is a normal connection socket, so pass the read event along */
      DEBUG("calling socket read callback for socket %p\n", (void*)s);
      S_READ_EVT(s->cb, s, n);
    }
  }
}

static void s_write_evt(socket_t *s, write_dst_t *wd, aiofd_t *a, void *buf,
                        size_t n)
{
  int errval = 0;
  socket_ret_t ret = SOCKET_OK;
  CHECK_PTR(a);
  CHECK_PTR(s);
  CHECK_PTR(s->cb);

  DEBUG("write callback for socket %p\n", (void*)s);

  switch(s->type)
  {
    case SOCKET_UDP:
      /* free up the struct that stored the UDP write destination */
      FREE(wd);

      /* call the write complete callback to let client know that a particular
      * buffer has been written to the socket. */
      DEBUG("calling write complete callback for socket %p\n", (void*)s);
      S_WRITE_EVT(s->cb, s, buf, n);

      if(buf == NULL)
      {
        /* stop the write event processing until we have data to write */
        DEBUG("stopping write events for socket %p\n", (void*)s);
        aiofd_enable_write_evt(a, FALSE, NULL);
      }
      break;

    case SOCKET_TCP:
    case SOCKET_UNIX:
      if(socket_is_connected(s))
      {
        if(buf == NULL)
        {
          /* stop the write event processing until we have data to write */
          DEBUG("stopping write events for socket %p\n", (void*)s);
          aiofd_enable_write_evt(a, FALSE, NULL);
          return;
        }

        /* call the write complete callback to let client know that a particular
         * buffer has been written to the socket. */
        DEBUG("calling write complete callback for socket %p\n", (void*)s);
        S_WRITE_EVT(s->cb, s, buf, n);
        return;
      }

      /* we're connecting the socket and this write event callback is to signal
      * that the connect has completed either successfully or failed */

      /* check to see if the connect() succeeded */
      if(!s_get_error(s, &errval))
      {
        DEBUG("failed to get socket option while checking connect %p\n", (void*)s);
        DEBUG("calling socket error callback\n");
        S_ERR_EVT(s->cb, s, errno);

        /* stop write event processing */
        DEBUG("stopping write events for socket %p\n", (void*)s);
        aiofd_enable_write_evt(a, FALSE, NULL);
        return;
      }

      if(errval == 0)
      {
        DEBUG("socket connected\n");
        s->connected = TRUE;

        DEBUG("calling connect callback for socket %p\n", (void*)s);
        S_CONN_EVT(s->cb, s, &ret);

        /* we're connected to start read event */
        DEBUG("enabling read event for socket %p\n", (void*)s);
        aiofd_enable_read_evt(s->aiofd, TRUE, s->el);
      }
      else
      {
        DEBUG("socket connect failed\n");
        DEBUG("calling socket error callback %p\n", (void*)s);
        S_ERR_EVT(s->cb, s, errno);

        /* stop write event processing */
        DEBUG("stopping write events for socket %p\n", (void*)s);
        aiofd_enable_write_evt(a, FALSE, NULL);
      }
      break;
  }
}

static void s_error_evt(socket_t *s, write_dst_t *wd, aiofd_t *a, int eno)
{
  CHECK_PTR(a);
  CHECK_PTR(s);

  DEBUG("calling socket error callback\n");
  S_ERR_EVT(s->cb, s, eno);
}

static void s_read_io(socket_t *s, aiofd_t *a, int fd, void *buf, size_t n,
                      ssize_t *r)
{
  ssize_t ret = 0;
  if(!s)
  {
    if(r)
      (*r) = SOCKET_BADPARAM;
    return;
  }

  /* NOTE: we can use blocking calls here because these are usually
   * only called in response to a read callback from the aiofd when
   * there is data to be read from the socket so this won't block. */
  if(socket_is_connected(s))
    ret = RECV(fd, buf, n, 0);
  else
    ret = RECVFROM(fd, buf, n, 0, (struct sockaddr *)s->readaddr, s->readaddrlen);

  if(r)
    (*r) = ret;
}

static void s_write_io(socket_t *s, write_dst_t *wd, aiofd_t *a, int fd,
                       void const *buf, size_t n, ssize_t *r)
{
  ssize_t ret = 0;
  if(!s)
  {
    if(r)
      (*r) = SOCKET_BADPARAM;
    return;
  }

  if(socket_is_connected(s))
    ret = SEND(fd, buf, n, 0);
  else
  {
    /* write destination struct pointer must not be null */
    if(wd)
      ret = SENDTO(fd, buf, n, 0, (struct sockaddr const *)&(wd->addr), wd->addrlen);
    else
      ret = SOCKET_WRITE_FAIL;
  }

  if(r)
    (*r) = ret;
}

static void s_readv_io(socket_t *s, aiofd_t *a, int fd, struct iovec *iov,
                       size_t n, ssize_t *r)
{
  ssize_t ret = 0;
  struct msghdr msg;
  if(!s)
  {
    if(r)
      (*r) = SOCKET_BADPARAM;
    return;
  }

  if(socket_is_connected(s))
    ret = READV(fd, iov, n);
  else
  {
    /* otherwise, use recvmsg to do scatter input and get the source addr */

    /* clear the msghdr */
    MEMSET(&msg, 0, sizeof(struct msghdr));

    /* set it up so that the data is read into the iovec passed in */
    msg.msg_iov = iov;
    msg.msg_iovlen = n;

    /* set it so that the sender address is stored in the socket */
    msg.msg_name = (void*)s->readaddr;
    msg.msg_namelen = (s->readaddrlen != NULL ? *(s->readaddrlen) : 0);

    /* receive the data */
    ret = RECVMSG(fd, &msg, 0);

    if(s->readaddrlen != NULL)
    {
      *(s->readaddrlen) = msg.msg_namelen;
    }
  }

  if(r)
    (*r) = ret;
}

static void s_writev_io(socket_t *s, write_dst_t *wd, aiofd_t *a, int fd,
                        struct iovec const *iov, size_t n, ssize_t *r)
{
  ssize_t ret = 0;
  struct msghdr msg;
  if(!s)
  {
    if(r)
      (*r) = SOCKET_BADPARAM;
    return;
  }

  if(socket_is_connected(s))
    ret = WRITEV(fd, iov, n);
  else
  {
    if(wd)
    {
      /* clear the msghdr */
      MEMSET(&msg, 0, sizeof(struct msghdr));

      /* set it up so that the data is sent from the iovec passed in */
      msg.msg_iov = (struct iovec *)iov;
      msg.msg_iovlen = n;

      /* set it so that the destination address is stored in the socket */
      msg.msg_name = (void*)&(wd->addr);
      msg.msg_namelen = wd->addrlen;

      /* send the data */
      ret = SENDMSG(fd, &msg, 0);
    }
    else
      ret = SOCKET_WRITE_FAIL;
  }

  if(r)
    (*r) = ret;
}

static void s_nread_io(socket_t *s, aiofd_t *a, int fd, size_t *n, int_t *l,
                       int *r)
{
  int ret = 0;
  size_t nread = 0;
  if(!s)
  {
    if(r)
      (*r) = SOCKET_BADPARAM;
    if(l)
      (*l) = FALSE;
    return;
  }

  ret = IOCTL(fd, FIONREAD, &nread);

  if(n)
    (*n) = nread;
  if(l)
    (*l) = s->listening;
  if(r)
    (*r) = ret;
}

#if defined(UNIT_TESTING)

#include <CUnit/Basic.h>

void test_socket_private_functions(void)
{
}

#if 0
socket_ret_t test_error_fn_ret = SOCKET_OK;
socket_ret_t test_connect_fn_ret = SOCKET_OK;

static void test_error_cb(uint8_t const * name, void * state, void * param)
{
  socket_event_t *sevt = (socket_event_t*)param;
  sevt->err = TRUE;
  sevt->ret = test_error_fn_ret;
}

static void test_connect_cb(uint8_t const * name, void * state, void * param)
{
  socket_event_t *sevt = (socket_event_t*)param;
  sevt->err = TRUE;
  sevt->ret = test_connect_fn_ret;
}

void test_socket_private_functions(void)
{
    int test_flag;
    uint8_t * const host = UT(STRDUP("foo.com"));
    uint8_t buf[64];
    socket_t s, *p;
    socket_event_t sevt;

    reset_test_flags();

    MEMSET(&s, 0, sizeof(socket_t));

    /* test socket_get_errror */
    CU_ASSERT_FALSE(s_get_error(NULL, NULL));
    CU_ASSERT_FALSE(s_get_error(&s, NULL));
    fake_socket_getsockopt = TRUE;
    fake_socket_errval = TRUE;
    fake_s_get_error_ret = TRUE;
    test_flag = FALSE;
    CU_ASSERT_TRUE(s_get_error(&s, &test_flag));
    CU_ASSERT_TRUE(test_flag);

    reset_test_flags();

    /* test socket_aiofd_write_evt_fn */
    CU_ASSERT_FALSE(socket_aiofd_write_evt_fn(NULL, NULL, NULL, NULL));
    CU_ASSERT_FALSE(socket_aiofd_write_evt_fn(&(s.aiofd), NULL, NULL, NULL));
    s.connected = TRUE;
    CU_ASSERT_TRUE(list_initialize(&(s.aiofd.wbuf), 2, NULL));
    CU_ASSERT_TRUE(list_push_tail(&(s.aiofd.wbuf), (void*)host));
    CU_ASSERT_TRUE(socket_aiofd_write_evt_fn(&(s.aiofd), NULL, (void*)(&s), NULL));
    CU_ASSERT_TRUE(socket_aiofd_write_evt_fn(&(s.aiofd), (uint8_t const *)&host, (void*)(&s), NULL));
    s.connected = FALSE;
    s.aiofd.rfd = STDIN_FILENO;
    CU_ASSERT_FALSE(socket_aiofd_write_evt_fn(&(s.aiofd), (uint8_t const *)&host, (void*)(&s), NULL));
    CU_ASSERT_TRUE(cb_add(s.cb, T("socket-error"), &s, &test_error_cb));
    sevt = (socket_event_t){ &s, FALSE, 0, NULL, SOCKET_OK };
    CU_ASSERT_FALSE(socket_aiofd_write_evt_fn(&(s.aiofd), (uint8_t const *)&host, (void*)(&s), NULL));
    CU_ASSERT_TRUE(sevt.err);
    fake_socket_getsockopt = TRUE;
    fake_socket_errval = 0;
    fake_s_get_error_ret = TRUE;
    list_clear(&(s.aiofd.wbuf));
    CU_ASSERT_FALSE(socket_aiofd_write_evt_fn(&(s.aiofd), (uint8_t const *)&host, (void*)(&s), NULL));
    fake_socket_errval = -1;
    CU_ASSERT_TRUE(cb_remove(s.cb, T("socket-error"), &s, &test_error_cb));
    s.connected = FALSE;
    CU_ASSERT_FALSE(socket_aiofd_write_evt_fn(&(s.aiofd), (uint8_t const *)&host, (void*)(&s), NULL));
    fake_socket_errval = 0;
    fake_socket_getsockopt = FALSE;

    reset_test_flags();

    /* test socket_aiofd_error_evt_fn */
    CU_ASSERT_FALSE(socket_aiofd_error_evt_fn(NULL, 0, NULL));
    CU_ASSERT_FALSE(socket_aiofd_error_evt_fn(&(s.aiofd), 0, NULL));
    CU_ASSERT_TRUE(socket_aiofd_error_evt_fn(&(s.aiofd), 0, (void*)(&s)));
    CU_ASSERT_TRUE(cb_add(s.cb, T("socket-error"), &s, &test_error_cb));
    sevt = (socket_event_t){ &s, FALSE, 0, NULL, SOCKET_OK };
    CU_ASSERT_TRUE(socket_aiofd_error_evt_fn(&(s.aiofd), 0, (void*)(&s)));
    CU_ASSERT_TRUE(sevt.err);

    reset_test_flags();

    /* test socket_aiofd_read_evt_fn */
    CU_ASSERT_FALSE(socket_aiofd_read_evt_fn(NULL, 0, NULL));
    CU_ASSERT_FALSE(socket_aiofd_read_evt_fn(&(s.aiofd), 0, NULL));
    s.aiofd.rfd = 0;
    CU_ASSERT_FALSE(socket_aiofd_read_evt_fn(&(s.aiofd), 0, (void*)(&s)));
    CU_ASSERT_TRUE(socket_aiofd_read_evt_fn(&(s.aiofd), 1, (void*)(&s)));

    /* call socket_aiofd_read_evt_fn as if we're bound and listening but
     * don't have a connect callback set so the test_flag should not get
     * changed to TRUE */
    s.bound = TRUE;
    s.aiofd.listen = TRUE;
    test_flag = FALSE;
    CU_ASSERT_TRUE(socket_aiofd_read_evt_fn(&(s.aiofd), 0, (void*)(&s)));
    CU_ASSERT_FALSE(test_flag);

    /* now we'll set up a connect callback and have it return an error */
    s.ops.connect_fn = &test_connect_fn;
    test_connect_fn_ret = SOCKET_ERROR;
    test_flag = FALSE;
    CU_ASSERT_FALSE(socket_aiofd_read_evt_fn(&(s.aiofd), 0, (void*)(&s)));
    CU_ASSERT_TRUE(test_flag);

    reset_test_flags();

    /* test s_init */
    fake_socket = TRUE;
    fake_setsockopt = TRUE;
    fake_fcntl = TRUE;
    fake_aiofd_initialize = TRUE;
    MEMSET(&s, 0, sizeof(socket_t));
    MEMSET(&ops, 0, sizeof(socket_ops_t));
    CU_ASSERT_FALSE(s_init(NULL, SOCKET_UNKNOWN, NULL, NULL));
    CU_ASSERT_FALSE(s_init(&s, SOCKET_UNKNOWN, NULL, NULL));
    CU_ASSERT_FALSE(s_init(&s, SOCKET_LAST, NULL, NULL));
    CU_ASSERT_TRUE(s_init(&s, SOCKET_TCP, NULL, NULL));

    reset_test_flags();

    /* test socket_is_connected */
    MEMSET(&s, 0, sizeof(socket_t));
    CU_ASSERT_FALSE(socket_is_connected(NULL));
    s.aiofd.rfd = -1;
    CU_ASSERT_FALSE(socket_is_connected(&s));
    s.aiofd.rfd = STDIN_FILENO;
    CU_ASSERT_FALSE(socket_is_connected(&s));
    s.connected = TRUE;
    CU_ASSERT_TRUE(socket_is_connected(&s));

    reset_test_flags();

    /* test socket_is_bound */
    MEMSET(&s, 0, sizeof(socket_t));
    CU_ASSERT_FALSE(socket_is_bound(NULL));
    s.aiofd.rfd = -1;
    CU_ASSERT_FALSE(socket_is_bound(&s));
    s.aiofd.rfd = STDIN_FILENO;
    CU_ASSERT_FALSE(socket_is_bound(&s));
    s.bound = TRUE;
    CU_ASSERT_TRUE(socket_is_bound(&s));

    reset_test_flags();

    /* test socket_connect */
#if 0
    MEMSET(&s, 0, sizeof(socket_t));
    CU_ASSERT_EQUAL(socket_connect(NULL, NULL, NULL, NULL), SOCKET_BADPARAM);
    CU_ASSERT_EQUAL(socket_connect(&s, NULL, NULL, NULL), SOCKET_BADHOSTNAME);
    CU_ASSERT_EQUAL(socket_connect(&s, "foo.com", NULL, NULL), SOCKET_INVALIDPORT);
    s.type = SOCKET_LAST;
    CU_ASSERT_EQUAL(socket_connect(&s, "foo.com", "0", el), SOCKET_ERROR);
    s.type = SOCKET_UNKNOWN;
    CU_ASSERT_EQUAL(socket_connect(&s, "foo.com", NULL, el), SOCKET_ERROR);
    s.type = SOCKET_TCP;
    s.host = STRDUP("foo.com");
    CU_ASSERT_EQUAL(socket_connect(&s, NULL, NULL, el), SOCKET_INVALIDPORT);
    fake_socket_connected = TRUE;
    fake_socket_connected_ret = TRUE;
    CU_ASSERT_EQUAL(socket_connect(&s, NULL, "1024", el), SOCKET_CONNECTED);
    FREE(s.port);
    s.port = STRDUP("1024");
    CU_ASSERT_EQUAL(socket_connect(&s, NULL, NULL, el), SOCKET_CONNECTED);
    FREE(s.port);
    s.port = NULL;
    s.type = SOCKET_UNIX;
    CU_ASSERT_EQUAL(socket_connect(&s, NULL, NULL, el), SOCKET_CONNECTED);
    s.type = SOCKET_TCP;
    fake_socket_connected_ret = FALSE;
    CU_ASSERT_EQUAL(socket_connect(&s, NULL, "1024", el), SOCKET_OK);
    fake_socket_connect = TRUE;
    fake_socket_connect_ret = TRUE;
    CU_ASSERT_EQUAL(socket_connect(&s, NULL, NULL, el), SOCKET_OK);
    fake_socket_connect_ret = FALSE;
    CU_ASSERT_EQUAL(socket_connect(&s, NULL, NULL, el), SOCKET_OK);
    s.type = SOCKET_UNIX;
    fake_socket_connect_ret = TRUE;
    CU_ASSERT_EQUAL(socket_connect(&s, NULL, NULL, el), SOCKET_OPEN_FAIL);
    fake_socket_connect_ret = FALSE;
    CU_ASSERT_EQUAL(socket_connect(&s, NULL, NULL, el), SOCKET_OPEN_FAIL);
    FREE(s.host);
    s.host = NULL;
    FREE(s.port);
    s.port = NULL;

    reset_test_flags();

    /* test socket_bind */
    fake_setsockopt = TRUE;
    fake_aiofd_enable_read_evt = TRUE;
    fake_socket_bound = TRUE;
    fake_socket_bind = TRUE;
    MEMSET(&s, 0, sizeof(socket_t));
    CU_ASSERT_EQUAL(socket_bind(NULL, NULL, NULL, NULL), SOCKET_BADPARAM);
    fake_socket_bound_ret = TRUE;
    CU_ASSERT_EQUAL(socket_bind(&s, NULL, NULL, NULL), SOCKET_BOUND);
    fake_socket_bound_ret = FALSE;
    s.type = SOCKET_LAST;
    CU_ASSERT_EQUAL(socket_bind(&s, "foo.com", "0", el), SOCKET_ERROR);
    s.type = SOCKET_UNKNOWN;
    CU_ASSERT_EQUAL(socket_bind(&s, "foo.com", NULL, el), SOCKET_ERROR);
    s.type = SOCKET_TCP;
    s.host = STRDUP("foo.com");
    CU_ASSERT_EQUAL(socket_bind(&s, NULL, NULL, el), SOCKET_INVALIDPORT);
    CU_ASSERT_EQUAL(socket_bind(&s, NULL, "12354", el), SOCKET_OPEN_FAIL);
    s.type = SOCKET_TCP;
    fake_socket_bind_ret = FALSE;
    fake_setsockopt_ret = 0;
    CU_ASSERT_EQUAL(socket_bind(&s, NULL, NULL, el), SOCKET_ERROR);
    fake_setsockopt_ret = 1;
    CU_ASSERT_EQUAL(socket_bind(&s, NULL, NULL, el), SOCKET_ERROR);
    fake_socket_bind_ret = TRUE;
    fake_aiofd_enable_read_evt_ret = FALSE;
    CU_ASSERT_EQUAL(socket_bind(&s, NULL, NULL, el), SOCKET_ERROR);
    fake_aiofd_enable_read_evt_ret = TRUE;
    CU_ASSERT_EQUAL(socket_bind(&s, NULL, NULL, el), SOCKET_ERROR);
    s.type = SOCKET_UNIX;
    fake_socket_bind_ret = FALSE;
    CU_ASSERT_EQUAL(socket_bind(&s, NULL, NULL, el), SOCKET_OPEN_FAIL);
    fake_socket_bind_ret = TRUE;
    fake_aiofd_enable_read_evt_ret = FALSE;
    CU_ASSERT_EQUAL(socket_bind(&s, NULL, NULL, el), SOCKET_OPEN_FAIL);
    fake_aiofd_enable_read_evt_ret = TRUE;
    CU_ASSERT_EQUAL(socket_bind(&s, NULL, NULL, el), SOCKET_OPEN_FAIL);
    FREE(s.port);
    s.port = NULL;
#endif
    reset_test_flags();

    /* test_socket_listen */
    MEMSET(&s, 0, sizeof(socket_t));
    CU_ASSERT_EQUAL(socket_listen(NULL, 0), SOCKET_BADPARAM);
    fake_listen = TRUE;
    fake_listen_ret = -1;
    fake_socket_bound = TRUE;
    fake_socket_bound_ret = FALSE;
    CU_ASSERT_EQUAL(socket_listen(&s, 0), SOCKET_BOUND);
    fake_socket_bound_ret = TRUE;
    fake_socket_connected = TRUE;
    fake_socket_connected_ret = TRUE;
    CU_ASSERT_EQUAL(socket_listen(&s, 0), SOCKET_CONNECTED);
    fake_socket_bound_ret = FALSE;
    CU_ASSERT_EQUAL(socket_listen(&s, 0), SOCKET_BOUND);
    fake_socket_bound_ret = TRUE;
    fake_socket_connected_ret = FALSE;
    fake_listen_ret = 0;
    CU_ASSERT_EQUAL(socket_listen(&s, 0), SOCKET_ERROR);
    fake_listen_ret = -1;
    CU_ASSERT_EQUAL(socket_listen(&s, 0), SOCKET_ERROR);

    reset_test_flags();

    /* test socket_read */
    fake_aiofd_read = TRUE;
    MEMSET(&s, 0, sizeof(socket_t));
    CU_ASSERT_EQUAL(socket_read(NULL, NULL, 0), -1);
    CU_ASSERT_EQUAL(socket_read(&s, NULL, 0), -1);
    CU_ASSERT_EQUAL(socket_read(&s, buf, 0), -1);
    fake_aiofd_read_ret = -1;
    CU_ASSERT_EQUAL(socket_read(&s, buf, 64), -1);
    fake_aiofd_read_ret = 64;
    CU_ASSERT_EQUAL(socket_read(&s, buf, 64), 64);
    fake_aiofd_read = FALSE;

    reset_test_flags();

    /* test socket_write */
    fake_aiofd_write = TRUE;
    MEMSET(&s, 0, sizeof(socket_t));
    CU_ASSERT_EQUAL(socket_write(NULL, NULL, 0), SOCKET_BADPARAM);
    fake_aiofd_write_ret = FALSE;
    CU_ASSERT_EQUAL(socket_write(&s, NULL, 0), SOCKET_ERROR);
    fake_aiofd_write_ret = TRUE;
    CU_ASSERT_EQUAL(socket_write(&s, NULL, 0), SOCKET_OK);
    fake_aiofd_write = FALSE;

    reset_test_flags();

    /* test socket_writev */
    fake_aiofd_writev = TRUE;
    MEMSET(&s, 0, sizeof(socket_t));
    CU_ASSERT_EQUAL(socket_writev(NULL, NULL, 0), SOCKET_BADPARAM);
    fake_aiofd_writev_ret = FALSE;
    CU_ASSERT_EQUAL(socket_writev(&s, NULL, 0), SOCKET_ERROR);
    fake_aiofd_writev_ret = TRUE;
    CU_ASSERT_EQUAL(socket_writev(&s, NULL, 0), SOCKET_OK);
    fake_aiofd_writev = FALSE;

    reset_test_flags();

    /* test socket_accept */
    fake_accept = TRUE;
    fake_socket_bound = TRUE;
    fake_setsockopt = TRUE;
    fake_fcntl = TRUE;
    fake_aiofd_initialize = TRUE;
    fake_aiofd_enable_read_evt = TRUE;
    MEMSET(&s, 0, sizeof(socket_t));
    MEMSET(&ops, 0, sizeof(socket_ops_t));
    CU_ASSERT_PTR_NULL(socket_accept(NULL, NULL, NULL, NULL));
    CU_ASSERT_PTR_NULL(socket_accept(&s, NULL, NULL, NULL));
    CU_ASSERT_PTR_NULL(socket_accept(&s, &ops, NULL, NULL));
    CU_ASSERT_PTR_NULL(socket_accept(&s, &ops, el, NULL));
    fake_socket_bound_ret = FALSE;
    CU_ASSERT_PTR_NULL(socket_accept(&s, &ops, el, NULL));
    fake_socket_bound_ret = TRUE;
    s.type = SOCKET_LAST;
    CU_ASSERT_PTR_NULL(socket_accept(&s, &ops, el, NULL));
    s.type = SOCKET_UNKNOWN;
    CU_ASSERT_PTR_NULL(socket_accept(&s, &ops, el, NULL));
    s.type = SOCKET_TCP;
    fail_alloc = TRUE;
    CU_ASSERT_PTR_NULL(socket_accept(&s, &ops, el, NULL));
    fail_alloc = FALSE;
    s.type = SOCKET_UNIX;
    s.host = host;
    fake_aiofd_initialize_ret = FALSE;
    fake_fcntl_ret = -1;
    fake_accept_ret = -1;
    CU_ASSERT_PTR_NULL(socket_accept(&s, &ops, el, NULL));
    fake_accept_ret = 0;
    CU_ASSERT_PTR_NULL(socket_accept(&s, &ops, el, NULL));
    s.type = SOCKET_TCP;
    fake_accept_ret = -1;
    CU_ASSERT_PTR_NULL(socket_accept(&s, &ops, el, NULL));
    fake_accept_ret = 0;
    fake_setsockopt_ret = -1;
    fake_fcntl_ret = -1;
    fake_aiofd_initialize_ret = FALSE;
    CU_ASSERT_PTR_NULL(socket_accept(&s, &ops, el, NULL));
    fake_fcntl_ret = 0;
    fake_aiofd_initialize_ret = FALSE;
    CU_ASSERT_PTR_NULL(socket_accept(&s, &ops, el, NULL));
    fake_setsockopt_ret = 0;
    fake_fcntl_ret = -1;
    fake_aiofd_initialize_ret = FALSE;
    CU_ASSERT_PTR_NULL(socket_accept(&s, &ops, el, NULL));
    fake_fcntl_ret = 0;
    fake_aiofd_initialize_ret = FALSE;
    CU_ASSERT_PTR_NULL(socket_accept(&s, &ops, el, NULL));
    fake_aiofd_initialize_ret = TRUE;
    fake_aiofd_enable_read_evt_ret = FALSE;
    CU_ASSERT_PTR_NULL(socket_accept(&s, &ops, el, NULL));
    test_flag = FALSE;
    ops.connect_fn = &test_connect_fn;
    CU_ASSERT_PTR_NULL(socket_accept(&s, &ops, el, &test_flag));
    CU_ASSERT_TRUE(test_flag);
    MEMSET(&ops, 0, sizeof(socket_ops_t));
    fake_aiofd_enable_read_evt_ret = TRUE;
    p = socket_accept(&s, &ops, el, NULL);
    CU_ASSERT_PTR_NOT_NULL(p);
    FREE(p);

    reset_test_flags();
    FREE(host);
}
#endif
#endif


