/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
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

#ifndef __TEST_FLAGS_H__

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
extern int_t fake_aiofd_enable_read_evt;
extern int_t fake_aiofd_enable_read_evt_ret;

/* bitset */
extern int_t fail_bitset_init;
extern int_t fail_bitset_deinit;

/* buffer */
extern int_t fail_buffer_init;
extern int_t fail_buffer_deinit;
extern int_t fail_buffer_init_alloc;

/* cb */

/* child */

/* event */
extern int_t fake_ev_default_loop;
extern void* fake_ev_default_loop_ret;
extern int_t fake_event_handler_init;
extern int fake_event_handler_init_count;
extern int_t fake_event_handler_init_ret;
extern int_t fake_event_start_handler;
extern int_t fake_event_start_handler_ret;
extern int_t fake_event_stop_handler;
extern int_t fake_event_stop_handler_ret;

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
extern int fake_list_count_ret;
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
extern int_t fail_socket_initialize;
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

#endif/*__TEST_FLAGS_H__*/

