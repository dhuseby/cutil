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

#ifndef TEST_FLAGS_H

/* malloc/calloc/realloc fail switch */
extern int_t fail_alloc;

/* system call flags */
extern int_t fake_accept;
extern int fake_accept_ret;
extern int_t fake_bind;
extern int fake_bind_ret;
extern int_t fake_connect;
extern int fake_connect_ret;
extern int_t fake_errno;
extern int fake_errno_value;
extern int_t fake_fcntl;
extern int fake_fcntl_ret;
extern int_t fake_fork;
extern int fake_fork_ret;
extern int_t fake_freeaddrinfo;
extern int fake_freeaddrinfo_ret;
extern int_t fake_fstat;
extern int fake_fstat_ret;
extern int_t fake_fsync;
extern int fake_fsync_ret;
extern int_t fake_getaddrinfo;
extern int fake_getaddrinfo_ret;
extern int_t fake_getdtablesize;
extern int fake_getdtablesize_ret;
extern int_t fake_getegid;
extern int fake_getegid_ret;
extern int_t fake_geteuid;
extern int fake_geteuid_ret;
extern int_t fake_getgid;
extern int fake_getgid_ret;
extern int_t fake_getgroups;
extern int fake_getgroups_ret;
extern int_t fake_getuid;
extern int fake_getuid_ret;
extern int_t fake_ioctl;
extern int fake_ioctl_ret;
extern int_t fake_listen;
extern int fake_listen_ret;
extern int_t fake_pipe;
extern int fake_pipe_ret;
extern int_t fake_read;
extern ssize_t fake_read_ret;
extern int_t fake_readv;
extern ssize_t fake_readv_ret;
extern int_t fake_recv;
extern ssize_t fake_recv_ret;
extern int_t fake_recvfrom;
extern ssize_t fake_recvfrom_ret;
extern int_t fake_recvmsg;
extern ssize_t fake_recvmsg_ret;
extern int_t fake_send;
extern ssize_t fake_send_ret;
extern int_t fake_sendmsg;
extern ssize_t fake_sendmsg_ret;
extern int_t fake_sendto;
extern ssize_t fake_sendto_ret;
extern int_t fake_setegid;
extern int fake_setegid_ret;
extern int_t fake_seteuid;
extern int fake_seteuid_ret;
extern int_t fake_setgroups;
extern int fake_setgroups_ret;
extern int_t fake_setregid;
extern int fake_setregid_ret;
extern int_t fake_setreuid;
extern int fake_setreuid_ret;
extern int_t fake_setsockopt;
extern int fake_setsockopt_ret;
extern int_t fake_shutdown;
extern int fake_shutdown_ret;
extern int_t fake_socket;
extern int fake_socket_ret;
extern int_t fake_stat;
extern int fake_stat_ret;
extern int_t fake_strcmp;
extern int fake_strcmp_ret;
extern int_t fake_strdup;
extern char* fake_strdup_ret;
extern int_t fake_strtol;
extern uint_t fake_strtol_ret;
extern int_t fake_unlink;
extern int fake_unlink_ret;
extern int_t fake_write;
extern ssize_t fake_write_ret;
extern int_t fake_writev;
extern ssize_t fake_writev_ret;

/* aiofd */
extern int_t fake_aiofd_flush;
extern int_t fake_aiofd_flush_ret;
extern int_t fake_aiofd_initialize;
extern int_t fake_aiofd_initialize_ret;
extern int_t fake_aiofd_read;
extern ssize_t fake_aiofd_read_ret;
extern int_t fake_aiofd_write;
extern int_t fake_aiofd_write_ret;
extern int_t fake_aiofd_readv;
extern size_t fake_aiofd_readv_ret;
extern int_t fake_aiofd_writev;
extern int_t fake_aiofd_writev_ret;
extern int_t fake_aiofd_write_common;
extern int_t fake_aiofd_write_common_ret;

/* bitset */
extern int_t fail_bitset_init;
extern int_t fail_bitset_deinit;

/* buffer */
extern int_t fail_buffer_init;
extern int_t fail_buffer_deinit;
extern int_t fail_buffer_init_alloc;

/* cb */
extern int_t fake_cb_init;
extern int fake_cb_init_count;
extern int_t fake_cb_init_ret;

/* child */

/* event */
extern int_t fake_ev_default_loop;
extern void* fake_ev_default_loop_ret;
extern int_t fake_event_init;
extern int fake_event_init_count;
extern int_t fake_event_init_ret;
extern int_t fake_event_start;
extern int_t fake_event_start_ret;
extern int_t fake_event_stop;
extern int_t fake_event_stop_ret;

/* hashtable */
extern int_t fake_ht_init;
extern int_t fake_ht_init_ret;
extern int_t fake_ht_deinit;
extern int_t fake_ht_deinit_ret;
extern int_t fake_ht_grow;
extern int_t fake_ht_grow_ret;
extern int_t fake_ht_find;

/* list */
extern int_t fake_list_count;
extern uint_t fake_list_count_ret;
extern int_t fake_list_capacity;
extern uint_t fake_list_capacity_ret;
extern int_t fake_list_grow;
extern int_t fake_list_grow_ret;
extern int_t fake_list_init;
extern int_t fake_list_init_ret;
extern int_t fake_list_deinit;
extern int_t fake_list_deinit_ret;
extern int_t fake_list_push;
extern int_t fake_list_push_ret;
extern int_t fake_list_get;
extern void* fake_list_get_ret;

/* sanitize */
extern int_t fake_open_devnull;
extern int_t fake_open_devnull_ret;

/* socket */
extern int_t fake_socket_getsockopt;
extern int fake_socket_errval;
extern int_t fake_socket_get_error_ret;
extern int_t fail_s_init;
extern int_t fake_socket_connected;
extern int_t fake_socket_connected_ret;
extern int_t fake_socket_connect;
extern int_t fake_socket_connect_ret;
extern int_t fake_socket_bound;
extern int_t fake_socket_bound_ret;
extern int_t fake_socket_lookup_host;
extern int_t fake_socket_lookup_host_ret;
extern int_t fake_socket_bind;
extern int_t fake_socket_bind_ret;
extern int_t fake_socket_create_tcp_socket;
extern int_t fake_socket_create_tcp_socket_ret;
extern int_t fake_socket_create_udp_socket;
extern int_t fake_socket_create_udp_socket_ret;
extern int_t fake_socket_create_unix_socket;
extern int_t fake_socket_create_unix_socket_ret;

void reset_test_flags( void );

#endif /*TEST_FLAGS_H*/

