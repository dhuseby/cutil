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

#include <cutil/debug.h>
#include <cutil/macros.h>

/* malloc/calloc/realloc fail switch */
int fail_alloc = FALSE;

/* system call flags */
int fake_accept = FALSE;
int fake_accept_ret = -1;
int fake_bind = FALSE;
int fake_bind_ret = -1;
int fake_connect = FALSE;
int fake_connect_ret = -1;
int fake_connect_errno = FALSE;
int fake_connect_errno_value = 0;
int fake_fcntl = FALSE;
int fake_fcntl_ret = -1;
int fake_fork = FALSE;
int fake_fork_ret = -1;
int fake_listen = FALSE;
int fake_listen_ret = -1;
int fake_pipe = FALSE;
int fake_pipe_ret = -1;
int fake_setsockopt = FALSE;
int fake_setsockopt_ret = -1;
int fake_socket = FALSE;
int fake_socket_ret = -1;

/* aiofd */
int fake_aiofd_initialize = FALSE;
int fake_aiofd_initialize_ret = FALSE;
int fake_aiofd_read = FALSE;
int fake_aiofd_read_ret = 0;
int fake_aiofd_write = FALSE;
int fake_aiofd_write_ret = FALSE;
int fake_aiofd_writev = FALSE;
int fake_aiofd_writev_ret = FALSE;
int fake_aiofd_enable_read_evt = FALSE;
int fake_aiofd_enable_read_evt_ret = FALSE;

/* bitset */
int fail_bitset_init = FALSE;
int fail_bitset_deinit = FALSE;

/* buffer */
int fail_buffer_init = FALSE;
int fail_buffer_deinit = FALSE;
int fail_buffer_init_alloc = FALSE;

/* child */

/* hashtable */
int fake_ht_init = FALSE;
int fake_ht_init_ret = FALSE;
int fake_ht_deinit = FALSE;
int fake_ht_deinit_ret = FALSE;
int fake_ht_grow = FALSE;
int fake_ht_grow_ret = FALSE;
int fake_ht_find = FALSE;

/* list */
int fake_list_grow = FALSE;
int fake_list_grow_ret = FALSE;
int fake_list_init = FALSE;
int fake_list_init_ret = FALSE;
int fake_list_deinit = FALSE;
int fake_list_deinit_ret = FALSE;
int fake_list_push = FALSE;
int fake_list_push_ret = FALSE;
int fake_list_get = FALSE;
void* fake_list_get_ret = NULL;

/* socket */
int fake_socket_getsockopt = FALSE;
int fake_socket_errval = 0;
int fake_socket_get_error_ret = FALSE;
int fail_socket_initialize = FALSE;
int fake_socket_connected = FALSE;
int fake_socket_connected_ret = FALSE;
int fake_socket_connect = FALSE;
int fake_socket_connect_ret = FALSE;
int fake_socket_bound = FALSE;
int fake_socket_bound_ret = FALSE;
int fake_socket_lookup_host = FALSE;
int fake_socket_lookup_host_ret = FALSE;
int fake_socket_bind = FALSE;
int fake_socket_bind_ret = FALSE;

void reset_test_flags( void )
{
	/* malloc/calloc/realloc fail switch */
	fail_alloc = FALSE;

	/* system call flags */
	fake_accept = FALSE;
	fake_accept_ret = -1;
	fake_bind = FALSE;
	fake_bind_ret = -1;
	fake_connect = FALSE;
	fake_connect_ret = -1;
	fake_connect_errno = FALSE;
	fake_connect_errno_value = 0;
	fake_fcntl = FALSE;
	fake_fcntl_ret = -1;
	fake_fork = FALSE;
	fake_fork_ret = -1;
	fake_listen = FALSE;
	fake_listen_ret = -1;
	fake_pipe = FALSE;
	fake_pipe_ret = -1;
	fake_setsockopt = FALSE;
	fake_setsockopt_ret = -1;
	fake_socket = FALSE;
	fake_socket_ret = -1;

	/* aiofd */
	fake_aiofd_initialize = FALSE;
	fake_aiofd_initialize_ret = FALSE;
	fake_aiofd_read = FALSE;
	fake_aiofd_read_ret = 0;
	fake_aiofd_write = FALSE;
	fake_aiofd_write_ret = FALSE;
	fake_aiofd_writev = FALSE;
	fake_aiofd_writev_ret = FALSE;
	fake_aiofd_enable_read_evt = FALSE;
	fake_aiofd_enable_read_evt_ret = FALSE;

	/* bitset */
	fail_bitset_init = FALSE;
	fail_bitset_deinit = FALSE;

	/* buffer */
	fail_buffer_init = FALSE;
	fail_buffer_deinit = FALSE;
	fail_buffer_init_alloc = FALSE;

	/* child */

	/* hashtable */
	fake_ht_init = FALSE;
	fake_ht_init_ret = FALSE;
	fake_ht_deinit = FALSE;
	fake_ht_deinit_ret = FALSE;
	fake_ht_grow = FALSE;
	fake_ht_grow_ret = FALSE;
	fake_ht_find = FALSE;

	/* list */
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

	/* socket */
	fake_socket_getsockopt = FALSE;
	fake_socket_errval = 0;
	fake_socket_get_error_ret = FALSE;
	fail_socket_initialize = FALSE;
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
}

