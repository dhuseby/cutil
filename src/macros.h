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

#ifndef MACROS_H
#define MACROS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>

/*
 * local definitions for porting purposes
 */

/* casting macro for string constants */
#define T(x)    (int8_t*)(x)
#define UT(x)   (uint8_t*)(x)
#define C(x)    (char*)(x)

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

/* BOOLEANS */
#define FALSE ((int_t)0)
#define TRUE  ((int_t)1)

/* does the platform have strnlen? */
#if defined(__APPLE__)
#define MISSING_STRNLEN
#define MISSING_64BIT_ENDIAN
#endif

/* array size */
#define ARRAY_SIZE( x ) (sizeof(x) / sizeof(x[0]))

/* try to deduce the maximum number of signals on this platform, cribbed from libev */
#if defined EV_NSIG
/* use what's provided */
#elif defined NSIG
# define EV_NSIG (NSIG)
#elif defined _NSIG
# define EV_NSIG (_NSIG)
#elif defined SIGMAX
# define EV_NSIG (SIGMAX+1)
#elif defined SIG_MAX
# define EV_NSIG (SIG_MAX+1)
#elif defined _SIG_MAX
# define EV_NSIG (_SIG_MAX+1)
#elif defined MAXSIG
# define EV_NSIG (MAXSIG+1)
#elif defined MAX_SIG
# define EV_NSIG (MAX_SIG+1)
#elif defined SIGARRAYSIZE
# define EV_NSIG (SIGARRAYSIZE) /* Assume ary[SIGARRAYSIZE] */
#elif defined _sys_nsig
# define EV_NSIG (_sys_nsig) /* Solaris 2.5 */
#else
# error "unable to find value for NSIG, please report"
/* to make it compile regardless, just remove the above line, */
/* but consider reporting it, too! :) */
# define EV_NSIG 65
#endif

/* used for debugging purposes */
#define ASSERT(x) assert(x)
#define WARN(fmt, ...)   do { fprintf(stderr, "WARNING:%12s:%-5d -(%-5d)- " fmt,  __FILE__, __LINE__, getpid(), ##__VA_ARGS__); fflush(stderr); } while(0)
#define NOTICE(fmt, ...) do { fprintf(stderr, "NOTICE:%13s:%-5d -(%-5d)- " fmt,  __FILE__, __LINE__, getpid(), ##__VA_ARGS__); fflush(stderr); } while(0)
#define LOG(fmt, ...)    do { fprintf(stderr, "INFO:%15s:%-5d -(%-5d)- " fmt, __FILE__, __LINE__, getpid(), ##__VA_ARGS__); fflush(stderr); } while(0)
#define FAIL(fmt, ...)   do { fprintf(stderr, "ERR:%16s:%-5d -(%-5d)- " fmt,  __FILE__, __LINE__, getpid(), ##__VA_ARGS__); fflush(stderr); assert(0); } while(0)

/* runtime check macros */
#if !defined(CHECK_ERR_STR)
#define CHECK_ERR_STR
uint8_t * check_err_str_;
#endif
#define CHECK(x) do { if(!(x)) return; } while(0)
#define CHECK_MSG(x, ...) do { if(!(x)) { DEBUG(__VA_ARGS__); return; } } while(0)
#define CHECK_RET(x, y) do { if(!(x)) return (y); } while(0)
#define CHECK_RET_MSG(x, y, ...) do { if(!(x)) { DEBUG(__VA_ARGS__); return (y); } } while(0)
#define CHECK_GOTO(x, y) do { if(!(x)) { check_err_str_ = (uint8_t*)#x; goto y; } } while(0)
#define CHECK_PTR(x) do { if(!(x)) return; } while(0)
#define CHECK_PTR_MSG(x, ...) do { if(!(x)) { DEBUG(__VA_ARGS__); return; } } while(0)
#define CHECK_PTR_RET(x, y) do { if(!(x)) return (y); } while(0)
#define CHECK_PTR_RET_MSG(x, y, ...) do { if(!(x)) { DEBUG(__VA_ARGS__); return (y); } } while(0)
#define CHECK_PTR_GOTO(x, y) do { if(!(x)) { check_err_str_ = (uint8_t*)#x; goto y; } } while(0)

#if defined(UNIT_TESTING)

#define UNIT_TEST_RET(x) do { if( fake_##x ) return ( fake_##x##_ret ); } while(0)
#define UNIT_TEST_FAIL(x) do { if( fail_##x ) return FALSE; } while(0)
#define UNIT_TEST_N_RET(x) \
do { \
  if( fake_##x ) { \
    if (fake_##x##_count == 0) { \
      return ( fake_##x##_ret ); \
    } \
    fake_##x##_count--; \
  } \
} while(0)

/* system calls */
extern int_t fail_alloc;
#define MALLOC(...) (fail_alloc ? NULL : malloc(__VA_ARGS__))
#define CALLOC(...) (fail_alloc ? NULL : calloc(__VA_ARGS__))
#define REALLOC(...) (fail_alloc ? NULL : realloc(__VA_ARGS__))
#define FREE free
#define MEMSET memset
#define MEMCMP memcmp
#define MEMCPY memcpy

extern int_t fake_accept;
extern int fake_accept_ret;
#define ACCEPT(...) (fake_accept ? fake_accept_ret : accept(__VA_ARGS__))

extern int_t fake_bind;
extern int fake_bind_ret;
#define BIND(...) (fake_bind ? fake_bind_ret : bind(__VA_ARGS__))

extern int_t fake_close;
extern int fake_close_ret;
#define CLOSE(...) (fake_close ? fake_close_ret : close(__VA_ARGS__))

extern int_t fake_connect;
extern int fake_connect_ret;
#define CONNECT(...) (fake_connect ? fake_connect_ret : connect(__VA_ARGS__))

extern int_t fake_errno;
extern int fake_errno_value;
#define ERRNO (fake_errno ? fake_errno_value : errno)

extern int_t fake_fcntl;
extern int fake_fcntl_ret;
#define FCNTL(...) (fake_fcntl ? fake_fcntl_ret : fcntl(__VA_ARGS__))

extern int_t fake_fork;
extern int fake_fork_ret;
#define FORK(...) (fake_fork ? fake_fork_ret : fork(__VA_ARGS__))

extern int_t fake_freeaddrinfo;
extern int fake_freeaddrinfo_ret;
#define FREEADDRINFO(...) (fake_freeaddrinfo ? fake_freeaddrinfo_ret : freeaddrinfo(__VA_ARGS__))

extern int_t fake_fstat;
extern int fake_fstat_ret;
#define FSTAT(...) (fake_fstat ? fake_fstat_ret : fstat(__VA_ARGS__))

extern int_t fake_fsync;
extern int fake_fsync_ret;
#define FSYNC(...) (fake_fsync ? fake_fsync_ret : fsync(__VA_ARGS__))

extern int_t fake_getaddrinfo;
extern int fake_getaddrinfo_ret;
#define GETADDRINFO(...) (fake_getaddrinfo ? fake_getaddrinfo_ret : getaddrinfo(__VA_ARGS__))

extern int_t fake_getdtablesize;
extern int fake_getdtablesize_ret;
#define GETDTABLESIZE(...) (fake_getdtablesize ? fake_getdtablesize_ret : getdtablesize(__VA_ARGS__))

extern int_t fake_getegid;
extern int fake_getegid_ret;
#define GETEGID(...) (fake_getegid ? fake_getegid_ret : getegid(__VA_ARGS__))

extern int_t fake_geteuid;
extern int fake_geteuid_ret;
#define GETEUID(...) (fake_geteuid ? fake_geteuid_ret : geteuid(__VA_ARGS__))

extern int_t fake_getgid;
extern int fake_getgid_ret;
#define GETGID(...) (fake_getgid ? fake_getgid_ret : getgid(__VA_ARGS__))

extern int_t fake_getgroups;
extern int fake_getgroups_ret;
#define GETGROUPS(...) (fake_getgroups ? fake_getgroups_ret : getgroups(__VA_ARGS__))

extern int_t fake_getuid;
extern int fake_getuid_ret;
#define GETUID(...) (fake_getuid ? fake_getuid_ret : getuid(__VA_ARGS__))

extern int_t fake_ioctl;
extern int fake_ioctl_ret;
#define IOCTL(...) (fake_ioctl ? fake_ioctl_ret : ioctl(__VA_ARGS__))

extern int_t fake_listen;
extern int fake_listen_ret;
#define LISTEN(...) (fake_listen ? fake_listen_ret : listen(__VA_ARGS__))

extern int_t fake_pipe;
extern int fake_pipe_ret;
#define PIPE(...) (fake_pipe ? fake_pipe_ret : pipe(__VA_ARGS__))

extern int_t fake_read;
extern ssize_t fake_read_ret;
#define READ(...) (fake_read ? fake_read_ret : read(__VA_ARGS__))

extern int_t fake_readv;
extern ssize_t fake_readv_ret;
#define READV(...) (fake_readv ? fake_readv_ret : readv(__VA_ARGS__))

extern int_t fake_recv;
extern ssize_t fake_recv;
#define RECV(...) (fake_recv ? fake_recv_ret : recv(__VA_ARGS__))

extern int_t fake_recvfrom;
extern ssize_t fake_recvfrom_ret;
#define RECVFROM(...) (fake_recvfrom ? fake_recvfrom_ret : recvfrom(__VA_ARGS__))

extern int_t fake_recvmsg;
extern ssize_t fake_recvmsg_ret;
#define RECVMSG(...) (fake_recvmsg ? fake_recvmsg_ret : recvmsg(__VA_ARGS__))

extern int_t fake_send;
extern ssize_t fake_send_ret;
#define SEND(...) (fake_send ? fake_send_ret : send(__VA_ARGS__))

extern int_t fake_sendmsg;
extern ssize_t fake_sendmsg_ret;
#define SENDMSG(...) (fake_sendmsg ? fake_sendmsg_ret : sendmsg(__VA_ARGS__))

extern int_t fake_sendto;
extern ssize_t fake_sendto_ret;
#define SENDTO(...) (fake_sendto ? fake_sendto_ret : sendto(__VA_ARGS__))

extern int_t fake_setegid;
extern int fake_setegid_ret;
#define SETEGID(...) (fake_setegid ? fake_setegid_ret : setegid(__VA_ARGS__))

extern int_t fake_seteuid;
extern int fake_seteuid_ret;
#define SETEUID(...) (fake_seteuid ? fake_seteuid_ret : seteuid(__VA_ARGS__))

extern int_t fake_setgroups;
extern int fake_setgroups_ret;
#define SETGROUPS(...) (fake_setgroups ? fake_setgroups_ret : setgroups(__VA_ARGS__))

extern int_t fake_setregid;
extern int fake_setregid_ret;
#define SETREGID(...) (fake_setregid ? fake_setregid_ret : setregid(__VA_ARGS__))

extern int_t fake_setreuid;
extern int fake_setreuid_ret;
#define SETREUID(...) (fake_setreuid ? fake_setreuid_ret : setreuid(__VA_ARGS__))

extern int_t fake_setsockopt;
extern int fake_setsockopt_ret;
#define SETSOCKOPT(...) (fake_setsockopt ? fake_setsockopt_ret : setsockopt(__VA_ARGS__))

extern int_t fake_shutdown;
extern int fake_shutdown_ret;
#define SHUTDOWN(...) (fake_shutdown ? fake_shutdown_ret : shutdown(__VA_ARGS__))

extern int_t fake_socket;
extern int fake_socket_ret;
#define SOCKET(...) (fake_socket ? fake_socket_ret : socket(__VA_ARGS__))

extern int_t fake_stat;
extern int fake_stat_ret;
#define STAT(...) (fake_stat ? fake_stat_ret : stat(__VA_ARGS__))

extern int_t fake_strcmp;
extern int fake_strcmp_ret;
#define STRCMP(...) (fake_strcmp ? fake_strcmp_ret : strcmp(__VA_ARGS__))

extern int_t fake_strdup;
extern char* fake_strdup_ret;
#define STRDUP(...) (fake_strdup ? fake_strdup_ret : strdup(__VA_ARGS__))

extern int_t fake_strtol;
extern uint_t fake_strtol_ret;
#define STRTOL(...) (fake_strtol ? fake_strtol_ret : strtol(__VA_ARGS__))

extern int_t fake_unlink;
extern int fake_unlink_ret;
#define UNLINK(...) (fake_unlink ? fake_unlink_ret : unlink(__VA_ARGS__))

extern int_t fake_write;
extern ssize_t fake_write_ret;
#define WRITE(...) (fake_write ? fake_write_ret : write(__VA_ARGS__))

extern int_t fake_writev;
extern ssize_t fake_writev_ret;
#define WRITEV(...) (fake_writev ? fake_writev_ret : writev(__VA_ARGS__))

/* event */
extern int_t fake_ev_default_loop;
extern void* fake_ev_default_loop_ret;
#define EV_DEFAULT_LOOP(...) (fake_ev_default_loop ? fake_ev_default_loop_ret : ev_default_loop(__VA_ARGS__))

#else /* UNIT_TESTING */

/* UNIT_TESTING IS NOT DEFINED */

/* abstractions of system functions */
#if !defined(ACCEPT)
#define ACCEPT accept
#endif

#if !defined(BIND)
#define BIND bind
#endif

#if !defined(CALLOC)
#define CALLOC calloc
#endif

#if !defined(CLOSE)
#define CLOSE close
#endif

#if !defined(CONNECT)
#define CONNECT connect
#endif

#if !defined(ERRNO)
#define ERRNO errno
#endif

#if !defined(FCNTL)
#define FCNTL fcntl
#endif

#if !defined(FORK)
#define FORK fork
#endif

#if !defined(FREE)
#define FREE free
#endif

#if !defined(FREEADDRINFO)
#define FREEADDRINFO freeaddrinfo
#endif

#if !defined(FSTAT)
#define FSTAT fstat
#endif

#if !defined(FSYNC)
#define FSYNC fsync
#endif

#if !defined(GETADDRINFO)
#define GETADDRINFO getaddrinfo
#endif

#if !defined(GETDTABLESIZE)
#define GETDTABLESIZE getdtablesize
#endif

#if !defined(GETEGID)
#define GETEGID getegid
#endif

#if !defined(GETEUID)
#define GETEUID geteuid
#endif

#if !defined(GETGID)
#define GETGID getgid
#endif

#if !defined(GETGROUPS)
#define GETGROUPS getgroups
#endif

#if !defined(GETUID)
#define GETUID getuid
#endif

#if !defined(IOCTL)
#define IOCTL ioctl
#endif

#if !defined(LISTEN)
#define LISTEN listen
#endif

#if !defined(MALLOC)
#define MALLOC malloc
#endif

#if !defined(MEMCMP)
#define MEMCMP memcmp
#endif

#if !defined(MEMCPY)
#define MEMCPY memcpy
#endif

#if !defined(MEMSET)
#define MEMSET memset
#endif

#if !defined(PIPE)
#define PIPE pipe
#endif

#if !defined(READ)
#define READ read
#endif

#if !defined(READV)
#define READV readv
#endif

#if !defined(REALLOC)
#define REALLOC realloc
#endif

#if !defined(RECV)
#define RECV recv
#endif

#if !defined(RECVFROM)
#define RECVFROM recvfrom
#endif

#if !defined(RECVMSG)
#define RECVMSG recvmsg
#endif

#if !defined(SEND)
#define SEND send
#endif

#if !defined(SENDMSG)
#define SENDMSG sendmsg
#endif

#if !defined(SENDTO)
#define SENDTO sendto
#endif

#if !defined(SETEGID)
#define SETEGID setegid
#endif

#if !defined(SETEUID)
#define SETEUID seteuid
#endif

#if !defined(SETGROUPS)
#define SETGROUPS setgroups
#endif

#if !defined(SETREGID)
#define SETREGID setregid
#endif

#if !defined(SETREUID)
#define SETREUID setreuid
#endif

#if !defined(SETSOCKOPT)
#define SETSOCKOPT setsockopt
#endif

#if !defined(SHUTDOWN)
#define SHUTDOWN shutdown
#endif

#if !defined(SOCKET)
#define SOCKET socket
#endif

#if !defined(STAT)
#define STAT stat
#endif

#if !defined(STRCMP)
#define STRCMP strcmp
#endif

#if !defined(STRDUP)
#define STRDUP strdup
#endif

#if !defined(STRTOL)
#define STRTOL strtol
#endif

#if !defined(UNLINK)
#define UNLINK unlink
#endif

#if !defined(WRITE)
#define WRITE write
#endif

#if !defined(WRITEV)
#define WRITEV writev
#endif

#define EV_DEFAULT_LOOP ev_default_loop

/* define these to be nothing when not unit testing */
#define UNIT_TEST_RET(x) {;}
#define UNIT_TEST_FAIL(x) {;}
#define UNIT_TEST_N_RET(x) {;}

#endif /* UNIT_TESTING */

#endif /*MACROS_H*/
