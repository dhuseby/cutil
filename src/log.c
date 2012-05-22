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

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <syslog.h>

#include "macros.h"
#include "log.h"

static int8_t * const priov[] =
{
	T("EMERG:"),
	T("ALERT:"),
	T("CRIT:"),
	T("ERR:"),
	T("WARNING:"),
	T("NOTICE:"),
	T("INFO:"),
	T("DEBUG:")
};

/* handles determining the proper log level from the string prefix and writing the
 * message out to syslog */
static size_t writer( void * cookie, char const * data, size_t leng )
{
	(void)cookie;
	int p = LOG_DEBUG;
	int len;

	/* scan the prefix list, priov, seeing if any match the beginning of the data */
	do
	{
		len = strlen( priov[p] );
	} while ( memcmp( data, priov[p], len ) && (--p >= 0) );

	/* no match? set it to LOG_INFO */
	if ( p < 0 )
	{
		p = LOG_INFO;
	}
	else
	{
		/* we matched a prefix, skip over it */
		data += len;
		leng -= len;
	}

	/* p is now either the log level matching the prefix, or LOG_INFO */
	syslog( p, "%.*s", leng, data );
	return leng;
}

static int noop( void ) { return 0; }

static cookie_io_functions_t log_fns =
{
	(void*) noop,
	(void*) writer,
	(void*) noop,
	(void*) noop
};

void start_logging( void )
{
	/* allow everything except LOG_DEBUG messages to get to syslog */
	setlogmask( LOG_UPTO( LOG_INFO ) );

	/* most systems route the LOG_DAEMON facility to /var/log/daemon.log */
	openlog( "cbot", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_DAEMON );

	/* redirect stderr writes to our custom writer function that outputs to syslog */
	setvbuf(stderr = fopencookie(NULL, "w", log_fns), NULL, _IOLBF, 0);
}

void stop_logging( void )
{
	closelog();
}

