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

#include <sys/types.h>
#include <limits.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <paths.h>

#include "debug.h"
#include "macros.h"

/* this function was lifted from "Secure Programming Cookbook for C and C++"
 * by Matt Messier and John Viega, O'Reilly July 2003, ISBN: 0-596-00394-3.
 *
 * I hope they don't mind I borrowed the code.
 */

#ifndef OPEN_MAX
#define OPEN_MAX (256)
#endif

static int open_devnull( int fd )
{
	FILE *f = 0;

	if ( fd == STDIN_FILENO )
	{
		f = freopen( _PATH_DEVNULL, "rb", stdin );
	}
	else if ( fd == STDOUT_FILENO )
	{
		f = freopen( _PATH_DEVNULL, "wb", stdout );
	}
	else if ( fd == STDERR_FILENO )
	{
		f = freopen( _PATH_DEVNULL, "wb", stderr );
	}

	return ( f && (fileno(f) == fd) );
}

void sanitize_files( void )
{
	int fd, fds;
	struct stat st;

	/* figure out the maximum file descriptor value */
	if ( (fds = getdtablesize()) == -1 )
	{
		fds = OPEN_MAX;
	}
	
	/* make sure all open descriptors other than the standard ones are closed */
	for ( fd = (STDERR_FILENO + 1); fd < fds; fd++ )
	{
		close( fd );
	}

	/* verify that the standard descriptors are open.  if they're not, attempt to
	 * open them use /dev/null.  if any are unsuccessful, abort. */
	for ( fd = STDIN_FILENO; fd <= STDERR_FILENO; fd++ )
	{
		if ( (fstat( fd, &st ) == -1) && ((errno != EBADF) || (!open_devnull(fd))) )
			abort();
	}
}

/* the standard clean environment */
static int8_t * clean_environ[] =
{
	"IFS= \t\n",
	"PATH=" _PATH_STDPATH,
	NULL
};

/* the default list of environment variables to preserve */
static int8_t * preserve_environ[] =
{
	"TZ",
	NULL
};

int8_t ** build_clean_environ( int preservec, int8_t ** preservev, int addc, int8_t ** addv )
{
	int i;
	int8_t ** new_environ;
	int8_t *ptr, *value, *var;
	size_t arr_size = 1, arr_ptr = 0, len, new_size = 0;

	/* get the size and count of the standard clean environment */
	for ( i = 0; (var = clean_environ[i]) != NULL; i++ )
	{
		new_size += strlen( var ) + 1;
		arr_size ++;
	}

	/* get the size and count of the environment vars being preserved by default preserve */
	for ( i = 0; (var = preserve_environ[i]) != NULL; i++ )
	{
		/* if the env var isn't in the current env, skip it */
		if ( !(value = getenv(var)) )
			continue;

		new_size += strlen( var ) + strlen( value ) + 2;  /* include '=' as well as \0 */
		arr_size++;
	}

	/* get the size and count of the environment vars being preserved by the client */
	if ( (preservec > 0) && (preservev != NULL) )
	{
		for ( i = 0; (i < preservec) && ((var = preservev[i]) != NULL); i++ )
		{
			/* if the env var isn't in the current env, skip it */
			if ( !(value = getenv(var)) )
				continue;

			new_size += strlen( var ) + strlen( value ) + 2; /* include '=' as well as \0 */
			arr_size++;
		}
	}

	/* get the size and count of the environment vars being added by the clien */
	if ( (addc > 0) && (addv != NULL) )
	{
		for ( i = 0; (i < addc) && ((var = addv[i]) != NULL); i++ )
		{
			new_size += strlen( var ) + 1;
			arr_size++;
		}
	}

	/* allocate the new environment variable array */
	new_size += (arr_size * sizeof(int8_t *));
	new_environ = (int8_t**)CALLOC( new_size, sizeof(int8_t) );
	if ( new_environ == NULL )
		abort();


	/* copy over the default basic environment */
	ptr = (int8_t*)new_environ + (arr_size * sizeof(int8_t*));
	for ( i = 0; (var = clean_environ[i]) != NULL; i++ )
	{
		new_environ[arr_ptr++] = ptr;
		len = strlen( var );
		MEMCPY( ptr, var, len + 1 );
		ptr += len + 1;
	}

	/* copy over the default preserve environment */
	for ( i = 0; (var = preserve_environ[i]) != NULL; i++ )
	{
		/* if hte env var isn't in the current env, skip it */
		if ( !(value = getenv(var)) )
			continue;

		new_environ[arr_ptr++] = ptr;
		len = strlen(var);
		MEMCPY( ptr, var, len );
		*(ptr + len + 1) = '=';
		MEMCPY( ptr + len + 2, value, strlen(value) + 1 );
		ptr += len + strlen( value ) + 2; /* include the '=' */
	}

	/* copy over the client preserve environment */
	if ( (preservec > 0) && (preservev != NULL) )
	{
		for ( i = 0; (i < preservec) && ((var = preservev[i]) != NULL); i++ )
		{
			if ( !(value = getenv(var)) )
				continue;

			new_environ[arr_ptr++] = ptr;
			len = strlen(var);
			MEMCPY( ptr, var, len );
			*(ptr + len + 1) = '=';
			MEMCPY( ptr + len + 2, value, strlen(value) + 1 );
			ptr += len + strlen( value ) + 2; /* include the '=' */
		}
	}

	/* copy over the client add environment */
	if ( (addc > 0) && (addv != NULL) )
	{
		for ( i = 0; (i < addc) && ((var = addv[i]) != NULL); i++ )
		{
			new_environ[arr_ptr++] = ptr;
			len = strlen(var);
			MEMCPY( ptr, var, len + 1 );
			ptr += len + 1;
		}
	}

	return new_environ;
}


