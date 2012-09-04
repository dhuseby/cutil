/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
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

#ifndef __MACROS_H__
#define __MACROS_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>

/* 
 * local definitions for porting purposes
 */

/* BOOLEANS */
#define FALSE ((int)0)
#define TRUE  ((int)1)

/* array size */
#define ARRAY_SIZE( x ) (sizeof(x) / sizeof(x[0]))

/* used for debugging purposes */
#define ASSERT(x) assert(x)
#define WARN(fmt, ...) { fprintf(stderr, "WARNING:%12s:%-5d -(%-5d)- " fmt,  __FILE__, __LINE__, getpid(), ##__VA_ARGS__); fflush(stderr); }
#define NOTICE(fmt, ...) { fprintf(stderr, "NOTICE:%13s:%-5d -(%-5d)- " fmt,  __FILE__, __LINE__, getpid(), ##__VA_ARGS__); fflush(stderr); }
#define LOG(...) { fprintf(stderr, __VA_ARGS__); fflush(stderr); }
#define FAIL(fmt, ...) { fprintf(stderr, "ERR:%16s:%-5d -(%-5d)- " fmt,  __FILE__, __LINE__, getpid(), ##__VA_ARGS__); fflush(stderr); assert(0); }

/* runtime check macros */
#define CHECK(x) { if(!(x)) return; }
#define CHECK_MSG(x, ...) { if(!(x)) { DEBUG(__VA_ARGS__); return; } }
#define CHECK_RET(x, y) { if(!(x)) return (y); }
#define CHECK_RET_MSG(x, y, ...) { if(!(x)) { DEBUG(__VA_ARGS__); return (y); } }
#define CHECK_PTR(x) { if(!(x)) return; }
#define CHECK_PTR_MSG(x, ...) { if(!(x)) { DEBUG(__VA_ARGS__); return; } }
#define CHECK_PTR_RET(x, y) { if(!(x)) return (y); }
#define CHECK_PTR_RET_MSG(x, y, ...) { if(!(x)) { DEBUG(__VA_ARGS__); return (y); } }

/* abstractions of the memory allocator */
#define FREE free
#define MEMCPY memcpy
#define MEMCMP memcmp
#define MEMSET memset
#define STRDUP strdup

#if defined(UNIT_TESTING)
extern int fail_alloc;
#define MALLOC(...) (fail_alloc ? NULL : malloc(__VA_ARGS__))
#define CALLOC(...) (fail_alloc ? NULL : calloc(__VA_ARGS__))
#define REALLOC(...) (fail_alloc ? NULL : realloc(__VA_ARGS__))

extern int fake_accept;
extern int fake_accept_ret;
#define ACCEPT(...) (fake_accept ? fake_accept_ret : accept(__VA_ARGS__))

extern int fake_bind;
extern int fake_bind_ret;
#define BIND(...) (fake_bind ? fake_bind_ret : bind(__VA_ARGS__))

extern int fake_connect;
extern int fake_connect_ret;
#define CONNECT(...) (fake_connect ? fake_connect_ret : connect(__VA_ARGS__))

extern int fake_connect_errno;
extern int fake_connect_errno_value;
#define ERRNO (fake_connect_errno ? fake_connect_errno_value : errno)

extern int fake_fcntl;
extern int fake_fcntl_ret;
#define FCNTL(...) (fake_fcntl ? fake_fcntl_ret : fcntl(__VA_ARGS__))

extern int fake_listen;
extern int fake_listen_ret;
#define LISTEN(...) (fake_listen ? fake_listen_ret : listen(__VA_ARGS__))

extern int fake_setsockopt;
extern int fake_setsockopt_ret;
#define SETSOCKOPT(...) (fake_setsockopt ? fake_setsockopt_ret : setsockopt(__VA_ARGS__))

extern int fake_socket;
extern int fake_socket_ret;
#define SOCKET(...) (fake_socket ? fake_socket_ret : socket(__VA_ARGS__))

#else

#define MALLOC malloc
#define CALLOC calloc
#define REALLOC realloc
#define ACCEPT accept
#define BIND bind
#define CONNECT connect
#define FCNTL fcntl
#define LISTEN listen
#define SETSOCKOPT setsockopt
#define SOCKET socket
#define ERRNO errno

#endif

/* casting macro for string constants */
#define T(x)    (int8_t*)(x)
#define UT(x)	(uint8_t*)(x)
#define C(x)	(char*)(x)

/* are we on a 64-bit platform? */
#if defined(_WIN64) || defined(__amd64__) || defined(__x86_64__)
#define PORTABLE_64_BIT
typedef uint64_t uint_t;
typedef int64_t int_t;
#else
#define PORTABLE_32_BIT
typedef uint32_t uint_t;
typedef int32_t int_t;
#endif

/* does the platform have strnlen? */
#if defined(__APPLE__)
#define MISSING_STRNLEN
#define MISSING_64BIT_ENDIAN
#endif

#endif/*__MACROS_H__*/
 
