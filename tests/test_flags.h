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
extern int fail_alloc;

/* system call flags */
extern int fake_accept;
extern int fake_accept_ret;
extern int fake_bind;
extern int fake_bind_ret;
extern int fake_connect;
extern int fake_connect_ret;
extern int fake_connect_errno;
extern int fake_connect_errno_value;
extern int fake_fcntl;
extern int fake_fcntl_ret;
extern int fake_fork;
extern int fake_fork_ret;
extern int fake_listen;
extern int fake_listen_ret;
extern int fake_pipe;
extern int fake_pipe_ret;
extern int fake_setsockopt;
extern int fake_setsockopt_ret;
extern int fake_socket;
extern int fake_socket_ret;

/* aiofd */
extern int fake_aiofd_initialize;
extern int fake_aiofd_initialize_ret;
extern int fake_aiofd_read;
extern int fake_aiofd_read_ret;
extern int fake_aiofd_write;
extern int fake_aiofd_write_ret;
extern int fake_aiofd_writev;
extern int fake_aiofd_writev_ret;
extern int fake_aiofd_enable_read_evt;
extern int fake_aiofd_enable_read_evt_ret;

/* bitset */
extern int fail_bitset_init;
extern int fail_bitset_deinit;

/* buffer */
extern int fail_buffer_init;
extern int fail_buffer_deinit;
extern int fail_buffer_init_alloc;

/* child */

/* hashtable */
extern int fake_ht_init;
extern int fake_ht_init_ret;
extern int fake_ht_deinit;
extern int fake_ht_deinit_ret;
extern int fake_ht_grow;
extern int fake_ht_grow_ret;
extern int fake_ht_find;

/* list */
extern int fake_list_grow;
extern int fake_list_grow_ret;
extern int fake_list_init;
extern int fake_list_init_ret;
extern int fake_list_deinit;
extern int fake_list_deinit_ret;
extern int fake_list_push;
extern int fake_list_push_ret;
extern int fake_list_get;
extern void* fake_list_get_ret;

/* socket */
extern int fake_socket_getsockopt;
extern int fake_socket_errval;
extern int fake_socket_get_error_ret;
extern int fail_socket_initialize;
extern int fake_socket_connected;
extern int fake_socket_connected_ret;
extern int fake_socket_connect;
extern int fake_socket_connect_ret;
extern int fake_socket_bound;
extern int fake_socket_bound_ret;
extern int fake_socket_lookup_host;
extern int fake_socket_lookup_host_ret;
extern int fake_socket_bind;
extern int fake_socket_bind_ret;

void reset_test_flags( void );

#endif/*__TEST_FLAGS_H__*/

