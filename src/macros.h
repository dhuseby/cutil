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
#define WARN(fmt, ...) do { fprintf(stderr, "WARNING:%12s:%-5d -(%-5d)- " fmt,  __FILE__, __LINE__, getpid(), ##__VA_ARGS__); fflush(stderr); } while(0)
#define NOTICE(fmt, ...) do { fprintf(stderr, "NOTICE:%13s:%-5d -(%-5d)- " fmt,  __FILE__, __LINE__, getpid(), ##__VA_ARGS__); fflush(stderr); } while(0)
#define LOG(...) do { fprintf(stderr, __VA_ARGS__); fflush(stderr); } while(0)
#define FAIL(fmt, ...) do { fprintf(stderr, "ERR:%16s:%-5d -(%-5d)- " fmt,  __FILE__, __LINE__, getpid(), ##__VA_ARGS__); fflush(stderr); assert(0); } while(0)

/* runtime check macros */
#ifndef CHECK_ERR_STR
#define CHECK_ERR_STR
uint8_t * check_err_str_;
#endif
#define CHECK(x) do { if(!(x)) return; } while(0)
#define CHECK_MSG(x, ...) do { if(!(x)) { DEBUG(__VA_ARGS__); return; } } while(0)
#define CHECK_RET(x, y) do { if(!(x)) return (y); } while(0)
#define CHECK_RET_MSG(x, y, ...) do { if(!(x)) { DEBUG(__VA_ARGS__); return (y); } } while(0)
#define CHECK_GOTO(x, y) do { if(!(x)) { check_err_str_ = #x; goto y; } } while(0)
#define CHECK_PTR(x) do { if(!(x)) return; } while(0)
#define CHECK_PTR_MSG(x, ...) do { if(!(x)) { DEBUG(__VA_ARGS__); return; } } while(0)
#define CHECK_PTR_RET(x, y) do { if(!(x)) return (y); } while(0)
#define CHECK_PTR_RET_MSG(x, y, ...) do { if(!(x)) { DEBUG(__VA_ARGS__); return (y); } } while(0)
#define CHECK_PTR_GOTO(x, y) do { if(!(x)) { check_err_str_ = #x; goto y; } } while(0)

/* abstractions of the memory allocator */
#define FREE free
#define MEMCPY memcpy
#define MEMCMP memcmp
#define MEMSET memset
#define STRDUP strdup
#define STRCMP strcmp

#if defined(UNIT_TESTING)

/* system calls */
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

extern int fake_errno;
extern int fake_errno_value;
#define ERRNO (fake_errno ? fake_errno_value : errno)

extern int fake_fcntl;
extern int fake_fcntl_ret;
#define FCNTL(...) (fake_fcntl ? fake_fcntl_ret : fcntl(__VA_ARGS__))

extern int fake_fork;
extern int fake_fork_ret;
#define FORK(...) (fake_fork ? fake_fork_ret : fork(__VA_ARGS__))

extern int fake_fstat;
extern int fake_fstat_ret;
#define FSTAT(...) (fake_fstat ? fake_fstat_ret : fstat(__VA_ARGS__))

extern int fake_getdtablesize;
extern int fake_getdtablesize_ret;
#define GETDTABLESIZE(...) (fake_getdtablesize ? fake_getdtablesize_ret : getdtablesize(__VA_ARGS__))

extern int fake_getegid;
extern int fake_getegid_ret;
#define GETEGID(...) (fake_getegid ? fake_getegid_ret : getegid(__VA_ARGS__))

extern int fake_geteuid;
extern int fake_geteuid_ret;
#define GETEUID(...) (fake_geteuid ? fake_geteuid_ret : geteuid(__VA_ARGS__))

extern int fake_getgid;
extern int fake_getgid_ret;
#define GETGID(...) (fake_getgid ? fake_getgid_ret : getgid(__VA_ARGS__))

extern int fake_getgroups;
extern int fake_getgroups_ret;
#define GETGROUPS(...) (fake_getgroups ? fake_getgroups_ret : getgroups(__VA_ARGS__))

extern int fake_getuid;
extern int fake_getuid_ret;
#define GETUID(...) (fake_getuid ? fake_getuid_ret : getuid(__VA_ARGS__))

extern int fake_ioctl;
extern int fake_ioctl_ret;
#define IOCTL(...) (fake_ioctl ? fake_ioctl_ret : ioctl(__VA_ARGS__))

extern int fake_listen;
extern int fake_listen_ret;
#define LISTEN(...) (fake_listen ? fake_listen_ret : listen(__VA_ARGS__))

extern int fake_pipe;
extern int fake_pipe_ret;
#define PIPE(...) (fake_pipe ? fake_pipe_ret : pipe(__VA_ARGS__))

extern int fake_read;
extern int fake_read_ret;
#define READ(...) (fake_read ? fake_read_ret : read(__VA_ARGS__))

extern int fake_setegid;
extern int fake_setegid_ret;
#define SETEGID(...) (fake_setegid ? fake_setegid_ret : setegid(__VA_ARGS__))

extern int fake_seteuid;
extern int fake_seteuid_ret;
#define SETEUID(...) (fake_seteuid ? fake_seteuid_ret : seteuid(__VA_ARGS__))

extern int fake_setgroups;
extern int fake_setgroups_ret;
#define SETGROUPS(...) (fake_setgroups ? fake_setgroups_ret : setgroups(__VA_ARGS__))

extern int fake_setregid;
extern int fake_setregid_ret;
#define SETREGID(...) (fake_setregid ? fake_setregid_ret : setregid(__VA_ARGS__))

extern int fake_setreuid;
extern int fake_setreuid_ret;
#define SETREUID(...) (fake_setreuid ? fake_setreuid_ret : setreuid(__VA_ARGS__))

extern int fake_setsockopt;
extern int fake_setsockopt_ret;
#define SETSOCKOPT(...) (fake_setsockopt ? fake_setsockopt_ret : setsockopt(__VA_ARGS__))

extern int fake_socket;
extern int fake_socket_ret;
#define SOCKET(...) (fake_socket ? fake_socket_ret : socket(__VA_ARGS__))

extern int fake_write;
extern int fake_write_ret;
#define WRITE(...) (fake_write ? fake_write_ret : write(__VA_ARGS__))

extern int fake_writev;
extern int fake_writev_ret;
#define WRITEV(...) (fake_writev ? fake_writev_ret : writev(__VA_ARGS__))

/* event */
extern int fake_ev_default_loop;
extern void* fake_ev_default_loop_ret;
#define EV_DEFAULT_LOOP(...) (fake_ev_default_loop ? fake_ev_default_loop_ret : ev_default_loop(__VA_ARGS__))

#else

#define MALLOC malloc
#define CALLOC calloc
#define REALLOC realloc
#define ACCEPT accept
#define BIND bind
#define CONNECT connect
#define ERRNO errno
#define FCNTL fcntl
#define FSTAT fstat
#define FORK fork
#define GETDTABLESIZE getdtablesize
#define GETEGID getegid
#define GETEUID geteuid
#define GETGID getgid
#define GETGROUPS getgroups
#define GETUID getuid
#define IOCTL ioctl
#define LISTEN listen
#define PIPE pipe
#define READ read
#define SETEGID setegid
#define SETEUID seteuid
#define SETGROUPS setgroups
#define SETREGID setregid
#define SETREUID setreuid
#define SETSOCKOPT setsockopt
#define SOCKET socket
#define WRITE write
#define WRITEV writev

#define EV_DEFAULT_LOOP ev_default_loop

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
 
