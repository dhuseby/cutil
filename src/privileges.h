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

#ifndef __PRIVILEGES_H__
#define __PRIVILEGES_H__

#include <sys/param.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct priv_state_s
{
    gid_t   gid;
    uid_t   uid;
    int     ngroups;
    gid_t   groups[NGROUPS_MAX];

} priv_state_t;

int drop_privileges( int permanent, priv_state_t * const orig );
int restore_privileges( priv_state_t const * const orig );

#endif/*__PRIVILEGES_H__*/

