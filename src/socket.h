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

#ifndef SOCKET_H
#define SOCKET_H

#include "aiofd.h"
#include "events.h"
#include "list.h"

typedef enum socket_ret_e
{
  /* non-error return value */
  SOCKET_OK           = 1,
  SOCKET_INPUT        = 2,
  SOCKET_OUTPUT       = 3,

  /* errors */
  SOCKET_ERROR        = -1,
  SOCKET_BADPARAM     = -2,
  SOCKET_BADHOSTNAME  = -3,
  SOCKET_INVALIDPORT  = -4,
  SOCKET_TIMEOUT      = -5,
  SOCKET_POLLERR      = -6,
  SOCKET_CONNECTED    = -7,
  SOCKET_BOUND        = -8,
  SOCKET_OPEN_FAIL    = -9,
  SOCKET_CONNECT_FAIL = -10,
  SOCKET_BIND_FAIL    = -11,
  SOCKET_OPENED       = -12,
  SOCKET_WRITE_FAIL   = -13
} socket_ret_t;

typedef enum socket_type_e
{
  SOCKET_TCP,
  SOCKET_UDP,
  SOCKET_UNIX
} socket_type_t;

#define SOCKET_UNKNOWN (-1)
#define SOCKET_FIRST (SOCKET_TCP)
#define SOCKET_LAST (SOCKET_UNIX)
#define SOCKET_COUNT (SOCKET_LAST - SOCKET_FIRST + 1)
#define VALID_SOCKET_TYPE(t) ((t >= SOCKET_FIRST) && (t <= SOCKET_LAST))
#define HOST_BUF_LEN (1024)
#define PORT_BUF_LEN (8)

typedef struct socket_s socket_t;
typedef struct sockaddr_storage sockaddr_t;

/* NOTE: The connect_fn callback is used for connection based sockets (e.g.
 * TCP and Unix sockets).  For sockets that are bound and listening, it gets
 * called for each new incoming connection.  The new socket that is created
 * from the accept call also receives a connect_fn callback.  Sockets that are
 * connecting, the connect_fn callback gets called when the connect is complete.
 *
 * For UDP sockets, the connect_fn callback never gets called.
 *
 * Same goes with the disconnect_fn callback, UDP sockets never receive them.
 */

/*
 * SOCKET_CALLBACKS
 */

typedef enum socket_cb_e
{
  S_CONN_EVT,
  S_DISC_EVT,
  S_ERR_EVT,
  S_READ_EVT,
  S_WRITE_EVT
} socket_cb_t;

#define S_CB_FIRST (S_CONN_EVT)
#define S_CB_LAST (S_WRITE_EVT)
#define S_CB_COUNT (S_CB_LAST - S_CB_FIRST + 1)
#define VALID_S_CB(t) ((t >= S_CB_FIRST) && (t <= S_CB_LAST))

extern uint8_t const * const socket_cb[S_CB_COUNT];
#define S_CB_NAME(x) (VALID_S_CB(x) ? socket_cb[x] : NULL)

/* helper macros for declaring and adding socket callbacks */
#define S_CONNECT_EVT_CB(fn,ctx) CB_2(fn,ctx,socket_t*,socket_ret_t*)
#define S_DISCONNECT_EVT_CB(fn,ctx) CB_1(fn,ctx,socket_t*)
#define S_ERROR_EVT_CB(fn,ctx) CB_2(fn,ctx,socket_t*,int)
#define S_READ_EVT_CB(fn,ctx) CB_2(fn,ctx,socket_t*,size_t)
#define S_WRITE_EVT_CB(fn,ctx) CB_4(fn,ctx,socket_t*,void*,size_t)

#define S_ADD_CONNECT_EVT_CB(cb,fn,ctx) ADD_CB(cb,S_CB_NAME(S_CONN_EVT),fn,ctx)
#define S_ADD_DISCONNECT_EVT_CB(cb,fn,ctx) ADD_CB(cb,S_CB_NAME(S_DISC_EVT),fn,ctx)
#define S_ADD_ERROR_EVT_CB(cb,fn,ctx) ADD_CB(cb,S_CB_NAME(S_ERR_EVT),fn,ctx)
#define S_ADD_READ_EVT_CB(cb,fn,ctx) ADD_CB(cb,S_CB_NAME(S_READ_EVT),fn,ctx)
#define S_ADD_WRITE_EVT_CB(cb,fn,ctx) ADD_CB(cb,S_CB_NAME(S_WRITE_EVT),fn,ctx)

/* create/destroy a socket */
/* NOTE: ai_flags and ai_family have the same meaning as ai_flags
 * and ai_family in the struct addrinfo used by getaddrinfo() */
socket_t* socket_new(socket_type_t t, cb_t * cb,
                     uint8_t const * host, uint8_t const * port,
                     int ai_flags, int ai_family);
void socket_delete(void * s);

/* connect a socket as a client*/
socket_ret_t socket_connect(socket_t * s, evt_loop_t *el);
socket_ret_t socket_disconnect(socket_t *s);

/* bind a socket to a specified IP/port or inode */
socket_ret_t socket_bind(socket_t * s);

/* listen for incoming connections */
socket_ret_t socket_listen(socket_t * s, int backlog, evt_loop_t *el);

/* accept an incoming connection */
socket_t* socket_accept(socket_t * s, cb_t *cb, evt_loop_t *el);

/* helper functions */
int_t socket_addr_str(sockaddr_t const * addr, uint8_t * buf, size_t n);
int_t socket_port_str(sockaddr_t const * addr, uint8_t * buf, size_t n);
int_t socket_addr(socket_t const * s, sockaddr_t * addr, socklen_t * len);
int_t socket_is_connected(socket_t const * s);
int_t socket_is_bound(socket_t const * s);
int_t socket_is_listening(socket_t const * s);
socket_type_t socket_get_type(socket_t *s);

/* socket I/O functions */
ssize_t socket_read(socket_t *s, uint8_t *buf, size_t n);
ssize_t socket_readv(socket_t *s, struct iovec *iov, size_t n);
ssize_t socket_read_from(socket_t *s, uint8_t *buf, size_t n,
                         sockaddr_t *addr, socklen_t *addrlen);
ssize_t socket_readv_from(socket_t *s, struct iovec *iov, size_t n,
                          sockaddr_t *addr, socklen_t *addrlen);
socket_ret_t socket_write(socket_t *s, uint8_t const *buf, size_t n);
socket_ret_t socket_writev(socket_t *s, struct iovec const *iov, size_t n);
socket_ret_t socket_write_to(socket_t *s, uint8_t const *buf, size_t n,
                             sockaddr_t const *addr, socklen_t addrlen);
socket_ret_t socket_writev_to(socket_t *s, struct iovec const *iov, size_t n,
                               sockaddr_t const *addr, socklen_t addrlen);
socket_ret_t socket_flush(socket_t *s);

#endif /*SOCKET_H*/
