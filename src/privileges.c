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
#include "macros.h"

#if defined(UNIT_TESTING)
#include "test_flags.h"
#endif

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
	gid_t newgid = GETGID(), oldgid = GETEGID();
	uid_t newuid = GETUID(), olduid = GETEUID();

	if (!permanent) 
	{
		/* Save information about the privileges that are being dropped so that they
		 * can be restored later.
		 */
		orig_gid = oldgid;
		orig_uid = olduid;
		orig_ngroups = GETGROUPS(NGROUPS_MAX, orig_groups);
	}

	/* If root privileges are to be dropped, be sure to pare down the ancillary
	* groups for the process before doing anything else because the SETGROUPS(  )
	* system call requires root privileges.  Drop ancillary groups regardless of
	* whether privileges are being dropped temporarily or permanently.
	*/
	if (!olduid) 
		SETGROUPS(1, &newgid);

	if (newgid != oldgid) 
	{
		if (SETREGID((permanent ? newgid : -1), newgid) == -1) 
			abort();
	}

	if (newuid != olduid) 
	{
		if (SETREGID((permanent ? newuid : -1), newuid) == -1) 
			abort();
	}

	/* verify that the changes were successful */
	if (permanent) 
	{
		if ( (newgid != oldgid) && 
			 ((SETEGID(oldgid) != -1) || (GETEGID() != newgid)) )
		  abort();

		if ( (newuid != olduid) && 
			 ((SETEUID(olduid) != -1) || (GETEUID() != newuid)) )
		  abort();
	} 
	else 
	{
		if ( (newgid != oldgid) && (GETEGID() != newgid) )
			abort();

		if ( (newuid != olduid) && (GETEUID() != newuid) )
			abort();
	}
}

void restore_privileges( void )
{
	if ( GETEUID() != orig_uid )
	{
		if ( (SETEUID(orig_uid) == -1) || (GETEUID() != orig_uid) ) 
			abort();
	}

	if ( GETEGID() != orig_gid )
	{
		if ( (SETEGID(orig_gid) == -1) || (GETEGID() != orig_gid) ) 
			abort();
	}
	
	if (!orig_uid)
		SETGROUPS(orig_ngroups, orig_groups);
}

#if defined(UNIT_TESTING)

#include <CUnit/Basic.h>

void test_privileges_private_functions( void )
{
}

#endif

