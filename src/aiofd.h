/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */

#ifndef __AIOFD_H__
#define __AIOFD_H__

typedef struct aiofd_s aiofd_t;

typedef struct aiofd_s
{

	struct aiofd_ops_s
	{
		int32_t (*read_fn)( socket_t * const s, size_t nread, void * user_data );
		int32_t (*write_fn)( socket_t * const s, uint8_t const * const buffer, void * user_data );
	}	ops;
}

#endif/*__AIOFD_H__*/

