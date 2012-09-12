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
#include "privileges.h"

#if defined(UNIT_TESTING)
#include "test_flags.h"
#endif

/* these two functions were lifted from "Secure Programming Cookbook for C and C++"
 * by Matt Messier and John Viega, O'Reilly July 2003, ISBN: 0-596-00394-3.
 *
 * I hope they don't mind I borrowed the code.
 */

int drop_privileges( int permanent, priv_state_t * const orig )
{
	gid_t newgid = GETGID(), oldgid = GETEGID();
	uid_t newuid = GETUID(), olduid = GETEUID();

	CHECK_RET( newgid != -1, FALSE );
	CHECK_RET( oldgid != -1, FALSE );
	CHECK_RET( newuid != -1, FALSE );
	CHECK_RET( olduid != -1, FALSE );

	if (!permanent) 
	{
		CHECK_PTR_RET( orig, FALSE );

		/* Save information about the privileges that are being dropped so that they
		 * can be restored later.
		 */
		orig->gid = oldgid;
		orig->uid = olduid;
		orig->ngroups = GETGROUPS(NGROUPS_MAX, orig->groups);
		CHECK_RET( orig->ngroups != -1, FALSE );
	}

	/* If root privileges are to be dropped, be sure to pare down the ancillary
	* groups for the process before doing anything else because the SETGROUPS(  )
	* system call requires root privileges.  Drop ancillary groups regardless of
	* whether privileges are being dropped temporarily or permanently.
	*/
	if ( !olduid ) 
		CHECK_RET( SETGROUPS(1, &newgid) != -1, FALSE );

	if ( newgid != oldgid ) 
		if ( SETREGID((permanent ? newgid : -1), newgid) == -1 )
			return FALSE;

	if ( newuid != olduid ) 
		if ( SETREUID((permanent ? newuid : -1), newuid) == -1 )
			return FALSE;

	/* verify that the changes were successful */
	if (permanent) 
	{
		if ( (newgid != oldgid) && 
			 ((SETEGID(oldgid) != -1) || (GETEGID() != newgid)) )
			return FALSE;

		if ( (newuid != olduid) && 
			 ((SETEUID(olduid) != -1) || (GETEUID() != newuid)) )
			return FALSE;
	} 
	else 
	{
		if ( (newgid != oldgid) && (GETEGID() != newgid) )
			return FALSE;

		if ( (newuid != olduid) && (GETEUID() != newuid) )
			return FALSE;
	}

	return TRUE;
}

int restore_privileges( priv_state_t const * const orig )
{
	CHECK_PTR_RET( orig, FALSE );

	if ( GETEUID() != orig->uid )
	{
		if ( (SETEUID(orig->uid) == -1) || (GETEUID() != orig->uid) ) 
			return FALSE;
	}

	if ( GETEGID() != orig->gid )
	{
		if ( (SETEGID(orig->gid) == -1) || (GETEGID() != orig->gid) ) 
			return FALSE;
	}
	
	if ( !orig->uid )
		if ( SETGROUPS(orig->ngroups, orig->groups) == -1 )
			return FALSE;

	return TRUE;
}

#if defined(UNIT_TESTING)

#include <CUnit/Basic.h>

void test_privileges_private_functions( void )
{
}

#endif

