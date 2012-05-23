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

#include <sys/param.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>

#include "debug.h"

/* these two functions were lifted from "Secure Programming Cookbook for C and C++"
 * by Matt Messier and John Viega, O'Reilly July 2003, ISBN: 0-596-00394-3.
 *
 * I hope they don't mind I borrowed the code.
 */

static int orig_ngroups = -1;
static gid_t orig_gid = -1;
static uid_t orig_uid = -1;
static gid_t orig_groups[NGROUPS_MAX];

void drop_privileges( int permanent )
{
	gid_t newgid = getgid(), oldgid = getegid();
	uid_t newuid = getuid(), olduid = geteuid();

	if (!permanent) 
	{
		/* Save information about the privileges that are being dropped so that they
		 * can be restored later.
		 */
		orig_gid = oldgid;
		orig_uid = olduid;
		orig_ngroups = getgroups(NGROUPS_MAX, orig_groups);
	}

	/* If root privileges are to be dropped, be sure to pare down the ancillary
	* groups for the process before doing anything else because the setgroups(  )
	* system call requires root privileges.  Drop ancillary groups regardless of
	* whether privileges are being dropped temporarily or permanently.
	*/
	if (!olduid) 
		setgroups(1, &newgid);

	if (newgid != oldgid) 
	{
		if (setregid((permanent ? newgid : -1), newgid) == -1) 
			abort();
	}

	if (newuid != olduid) 
	{
		if (setregid((permanent ? newuid : -1), newuid) == -1) 
			abort();
	}

	/* verify that the changes were successful */
	if (permanent) 
	{
		if ( (newgid != oldgid) && 
			 ((setegid(oldgid) != -1) || (getegid() != newgid)) )
		  abort();

		if ( (newuid != olduid) && 
			 ((seteuid(olduid) != -1) || (geteuid() != newuid)) )
		  abort();
	} 
	else 
	{
		if ( (newgid != oldgid) && (getegid() != newgid) )
			abort();

		if ( (newuid != olduid) && (geteuid() != newuid) )
			abort();
	}
}

void restore_privileges( void )
{
	if ( geteuid() != orig_uid )
	{
		if ( (seteuid(orig_uid) == -1) || (geteuid() != orig_uid) ) 
			abort();
	}

	if ( getegid() != orig_gid )
	{
		if ( (setegid(orig_gid) == -1) || (getegid() != orig_gid) ) 
			abort();
	}
	
	if (!orig_uid)
		setgroups(orig_ngroups, orig_groups);
}

