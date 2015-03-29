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

#include <cutil/debug.h>
#include <cutil/macros.h>

/* malloc/calloc/realloc fail switch */
int_t fail_alloc = FALSE;

/* system call flags */
int_t fake_accept = FALSE;
int fake_accept_ret = -1;
int_t fake_bind = FALSE;
int fake_bind_ret = -1;
int_t fake_close = FALSE;
int fake_close_ret = -1;
int_t fake_connect = FALSE;
int fake_connect_ret = -1;
int_t fake_errno = FALSE;
int fake_errno_value = 0;
int_t fake_fcntl = FALSE;
int fake_fcntl_ret = -1;
int_t fake_fork = FALSE;
int fake_fork_ret = -1;
int_t fake_freeaddrinfo = FALSE;
int fake_freeaddrinfo_ret = -1;
int_t fake_fstat = FALSE;
int fake_fstat_ret = -1;
int_t fake_fsync = FALSE;
int fake_fsync_ret = -1;
int_t fake_getaddrinfo = FALSE;
int fake_getaddrinfo_ret = -1;
int_t fake_getdtablesize = FALSE;
int fake_getdtablesize_ret = -1;
int_t fake_getegid = FALSE;
int fake_getegid_ret = -1;
int_t fake_geteuid = FALSE;
int fake_geteuid_ret = -1;
int_t fake_getgid = FALSE;
int fake_getgid_ret = -1;
int_t fake_getgroups = FALSE;
int fake_getgroups_ret = -1;
int_t fake_getuid = FALSE;
int fake_getuid_ret = -1;
int_t fake_ioctl = FALSE;
int fake_ioctl_ret = -1;
int_t fake_listen = FALSE;
int fake_listen_ret = -1;
int_t fake_pipe = FALSE;
int fake_pipe_ret = -1;
int_t fake_read = FALSE;
ssize_t fake_read_ret = -1;
int_t fake_readv = FALSE;
ssize_t fake_readv_ret = -1;
int_t fake_recv = FALSE;
ssize_t fake_recv_ret = -1;
int_t fake_recvfrom = FALSE;
ssize_t fake_recvfrom_ret = -1;
int_t fake_recvmsg = FALSE;
ssize_t fake_recvmsg_ret = -1;
int_t fake_send = FALSE;
ssize_t fake_send_ret = -1;
int_t fake_sendmsg = FALSE;
ssize_t fake_sendmsg_ret = -1;
int_t fake_sendto = FALSE;
ssize_t fake_sendto_ret = -1;
int_t fake_setegid = FALSE;
int fake_setegid_ret = -1;
int_t fake_seteuid = FALSE;
int fake_seteuid_ret = -1;
int_t fake_setgroups = FALSE;
int fake_setgroups_ret = -1;
int_t fake_setregid = FALSE;
int fake_setregid_ret = -1;
int_t fake_setreuid = FALSE;
int fake_setreuid_ret = -1;
int_t fake_setsockopt = FALSE;
int fake_setsockopt_ret = -1;
int_t fake_shutdown = FALSE;
int fake_shutdown_ret = -1;
int_t fake_socket = FALSE;
int fake_socket_ret = -1;
int_t fake_stat = FALSE;
int fake_stat_ret = -1;
int_t fake_strcmp = FALSE;
int fake_strcmp_ret = 0;
int_t fake_strdup = FALSE;
char* fake_strdup_ret = NULL;
int_t fake_strtol = FALSE;
uint_t fake_strtol_ret = LONG_MIN;
int_t fake_unlink = FALSE;
int fake_unlink_ret = -1;
int_t fake_write = FALSE;
ssize_t fake_write_ret = -1;
int_t fake_writev = FALSE;
ssize_t fake_writev_ret = -1;

/* aiofd */
int_t fake_aiofd_flush = FALSE;
int_t fake_aiofd_flush_ret = FALSE;
int_t fake_aiofd_initialize = FALSE;
int_t fake_aiofd_initialize_ret = FALSE;
int_t fake_aiofd_read = FALSE;
ssize_t fake_aiofd_read_ret = -1;
int_t fake_aiofd_write = FALSE;
int_t fake_aiofd_write_ret = FALSE;
int_t fake_aiofd_readv = FALSE;
ssize_t fake_aiofd_readv_ret = -1;
int_t fake_aiofd_writev = FALSE;
int_t fake_aiofd_writev_ret = FALSE;
int_t fake_aiofd_write_common = FALSE;
int_t fake_aiofd_write_common_ret = FALSE;

/* bitset */
int_t fail_bitset_init = FALSE;
int_t fail_bitset_deinit = FALSE;

/* buffer */
int_t fail_buffer_init = FALSE;
int_t fail_buffer_deinit = FALSE;
int_t fail_buffer_init_alloc = FALSE;

/* cb */
int_t fake_cb_init = FALSE;
int fake_cb_init_count = 0;
int_t fake_cb_init_ret = FALSE;

/* child */

/* event */
int_t fake_ev_default_loop = FALSE;
void* fake_ev_default_loop_ret = NULL;
int_t fake_event_init = FALSE;
int fake_event_init_count = 0;
int_t fake_event_init_ret = FALSE;
int_t fake_event_start = FALSE;
int_t fake_event_start_ret = FALSE;
int_t fake_event_stop = FALSE;
int_t fake_event_stop_ret = FALSE;

/* hashtable */
int_t fake_ht_init = FALSE;
int_t fake_ht_init_ret = FALSE;
int_t fake_ht_deinit = FALSE;
int_t fake_ht_deinit_ret = FALSE;
int_t fake_ht_grow = FALSE;
int_t fake_ht_grow_ret = FALSE;
int_t fake_ht_find = FALSE;

/* list */
int_t fake_list_count = FALSE;
uint_t fake_list_count_ret = 0;
int_t fake_list_capacity = FALSE;
uint_t fake_list_capacity_ret = 0;
int_t fake_list_grow = FALSE;
int_t fake_list_grow_ret = FALSE;
int_t fake_list_init = FALSE;
int_t fake_list_init_ret = FALSE;
int_t fake_list_deinit = FALSE;
int_t fake_list_deinit_ret = FALSE;
int_t fake_list_push = FALSE;
int_t fake_list_push_ret = FALSE;
int_t fake_list_get = FALSE;
void* fake_list_get_ret = NULL;

/* sanitize */
int_t fake_open_devnull = FALSE;
int_t fake_open_devnull_ret = FALSE;

/* socket */
int_t fake_socket_getsockopt = FALSE;
int fake_socket_errval = 0;
int_t fake_socket_get_error_ret = FALSE;
int_t fail_s_init = FALSE;
int_t fake_socket_connected = FALSE;
int_t fake_socket_connected_ret = FALSE;
int_t fake_socket_connect = FALSE;
int_t fake_socket_connect_ret = FALSE;
int_t fake_socket_bound = FALSE;
int_t fake_socket_bound_ret = FALSE;
int_t fake_socket_lookup_host = FALSE;
int_t fake_socket_lookup_host_ret = FALSE;
int_t fake_socket_bind = FALSE;
int_t fake_socket_bind_ret = FALSE;
int_t fake_socket_create_tcp_socket = FALSE;
int_t fake_socket_create_tcp_socket_ret = FALSE;
int_t fake_socket_create_udp_socket = FALSE;
int_t fake_socket_create_udp_socket_ret = FALSE;
int_t fake_socket_create_unix_socket = FALSE;
int_t fake_socket_create_unix_socket_ret = FALSE;


void reset_test_flags( void )
{
  /* malloc/calloc/realloc fail switch */
  fail_alloc = FALSE;

  /* system call flags */
  fake_accept = FALSE;
  fake_accept_ret = -1;
  fake_bind = FALSE;
  fake_bind_ret = -1;
  fake_close = FALSE;
  fake_close_ret = -1;
  fake_connect = FALSE;
  fake_connect_ret = -1;
  fake_errno = FALSE;
  fake_errno_value = 0;
  fake_fcntl = FALSE;
  fake_fcntl_ret = -1;
  fake_fork = FALSE;
  fake_fork_ret = -1;
  fake_fstat = FALSE;
  fake_fstat_ret = -1;
  fake_getdtablesize = FALSE;
  fake_getdtablesize_ret = -1;
  fake_getegid = FALSE;
  fake_getegid_ret = -1;
  fake_geteuid = FALSE;
  fake_geteuid_ret = -1;
  fake_getgid = FALSE;
  fake_getgid_ret = -1;
  fake_getgroups = FALSE;
  fake_getgroups_ret = -1;
  fake_getuid = FALSE;
  fake_getuid_ret = -1;
  fake_ioctl = FALSE;
  fake_ioctl_ret = -1;
  fake_listen = FALSE;
  fake_listen_ret = -1;
  fake_pipe = FALSE;
  fake_pipe_ret = -1;
  fake_read = FALSE;
  fake_read_ret = -1;
  fake_setegid = FALSE;
  fake_setegid_ret = -1;
  fake_seteuid = FALSE;
  fake_seteuid_ret = -1;
  fake_setgroups = FALSE;
  fake_setgroups_ret = -1;
  fake_setregid = FALSE;
  fake_setregid_ret = -1;
  fake_setreuid = FALSE;
  fake_setreuid_ret = -1;
  fake_setsockopt = FALSE;
  fake_setsockopt_ret = -1;
  fake_shutdown = FALSE;
  fake_shutdown_ret = -1;
  fake_socket = FALSE;
  fake_socket_ret = -1;
  fake_write = FALSE;
  fake_write_ret = -1;
  fake_writev = FALSE;
  fake_writev_ret = -1;

  /* aiofd */
  fake_aiofd_flush = FALSE;
  fake_aiofd_flush_ret = FALSE;
  fake_aiofd_initialize = FALSE;
  fake_aiofd_initialize_ret = FALSE;
  fake_aiofd_read = FALSE;
  fake_aiofd_read_ret = 0;
  fake_aiofd_write = FALSE;
  fake_aiofd_write_ret = FALSE;
  fake_aiofd_writev = FALSE;
  fake_aiofd_writev_ret = FALSE;
  fake_aiofd_write_common = FALSE;
  fake_aiofd_write_common_ret = FALSE;

  /* bitset */
  fail_bitset_init = FALSE;
  fail_bitset_deinit = FALSE;

  /* buffer */
  fail_buffer_init = FALSE;
  fail_buffer_deinit = FALSE;
  fail_buffer_init_alloc = FALSE;

  /* cb */
  fake_cb_init = FALSE;
  fake_cb_init_count = 0;
  fake_cb_init_ret = FALSE;

  /* child */

  /* event */
  fake_ev_default_loop = FALSE;
  fake_ev_default_loop_ret = NULL;
  fake_event_init = FALSE;
  fake_event_init_count = 0;
  fake_event_init_ret = FALSE;
  fake_event_start = FALSE;
  fake_event_start_ret = FALSE;
  fake_event_stop = FALSE;
  fake_event_stop_ret = FALSE;

  /* hashtable */
  fake_ht_init = FALSE;
  fake_ht_init_ret = FALSE;
  fake_ht_deinit = FALSE;
  fake_ht_deinit_ret = FALSE;
  fake_ht_grow = FALSE;
  fake_ht_grow_ret = FALSE;
  fake_ht_find = FALSE;

  /* list */
  fake_list_count = FALSE;
  fake_list_count_ret = 0;
  fake_list_capacity = FALSE;
  fake_list_capacity_ret = 0;
  fake_list_grow = FALSE;
  fake_list_grow_ret = FALSE;
  fake_list_init = FALSE;
  fake_list_init_ret = FALSE;
  fake_list_deinit = FALSE;
  fake_list_deinit_ret = FALSE;
  fake_list_push = FALSE;
  fake_list_push_ret = FALSE;
  fake_list_get = FALSE;
  fake_list_get_ret = NULL;

  /* sanitize */
  fake_open_devnull = FALSE;
  fake_open_devnull_ret = FALSE;

  /* socket */
  fake_socket_getsockopt = FALSE;
  fake_socket_errval = 0;
  fake_socket_get_error_ret = FALSE;
  fake_s_init = FALSE;
  fake_socket_connected = FALSE;
  fake_socket_connected_ret = FALSE;
  fake_socket_connect = FALSE;
  fake_socket_connect_ret = FALSE;
  fake_socket_bound = FALSE;
  fake_socket_bound_ret = FALSE;
  fake_socket_lookup_host = FALSE;
  fake_socket_lookup_host_ret = FALSE;
  fake_socket_bind = FALSE;
  fake_socket_bind_ret = FALSE;
  fake_socket_create_tcp_socket = FALSE;
  fake_socket_create_tcp_socket_ret = FALSE;
  fake_socket_create_udp_socket = FALSE;
  fake_socket_create_udp_socket_ret = FALSE;
  fake_socket_create_unix_socket = FALSE;
  fake_socket_create_unix_socket_ret = FALSE;
}

