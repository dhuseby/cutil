/* Copyright (c) 2012-2015 David Huseby
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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

#if defined(UNIT_TESTING)
#include "test_flags.h"
#endif

/* these functions were lifted from "Secure Programming Cookbook for C and C++"
 * by Matt Messier and John Viega, O'Reilly July 2003, ISBN: 0-596-00394-3.
 *
 * I hope they don't mind I borrowed the code.
 */

#ifndef OPEN_MAX
#define OPEN_MAX (256)
#endif

static int_t open_devnull( int fd )
{
    FILE *f = 0;

    UNIT_TEST_RET( open_devnull );

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

int_t sanitize_files( int keep[], int nfds )
{
    int skip;
    int i, fd, fds;
    struct stat st;

    /* figure out the maximum file descriptor value */
    if ( (fds = GETDTABLESIZE()) == -1 )
    {
        fds = OPEN_MAX;
    }
    
    /* make sure all open descriptors other than the standard ones, and
     * the file descriptors we want to keep, are closed */
    for ( fd = (STDERR_FILENO + 1); fd < fds; fd++ )
    {
        skip = FALSE;
        for ( i = 0; i < nfds; i++ )
        {
            if ( keep[i] == fd )
                skip = TRUE;
        }

        if ( !skip )
            close( fd );
    }

    /* verify that the standard descriptors are open.  if they're not, attempt to
     * open them use /dev/null.  if any are unsuccessful, fail */
    for ( fd = STDIN_FILENO; fd <= STDERR_FILENO; fd++ )
    {
        if ( (FSTAT( fd, &st ) == -1) && ((errno != EBADF) || (!open_devnull(fd))) )
            return FALSE;
    }

    return TRUE;
}

int_t reset_signals_to_default( sigset_t const * const sigs )
{
    int i;
    struct sigaction sa;
    CHECK_PTR_RET( sigs, FALSE );

    MEMSET( &sa, 0, sizeof( struct sigaction ) );
    sa.sa_handler = SIG_DFL;
    sigemptyset( &sa.sa_mask );

    for ( i = 0; i < EV_NSIG; i++ )
    {
        if ( sigismember( sigs, i ) )
        {
            CHECK_RET_MSG( sigaction( i, &sa, NULL ) == 0, FALSE, "failed to set handler for signal %s\n" );
        }
    }
    return TRUE;
}

/* the standard clean environment */
static uint8_t * clean_environ[] =
{
    UT("IFS= \t\n"),
    UT("PATH=" _PATH_STDPATH),
    UT(NULL)
};

/* the default list of environment variables to preserve */
static uint8_t * preserve_environ[] =
{
    UT("TZ"),
    UT(NULL)
};

uint8_t ** build_clean_environ( int preservec, uint8_t ** preservev, int addc, uint8_t ** addv )
{
    int i;
    uint8_t ** new_environ;
    uint8_t *ptr, *value, *var;
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
    new_size += (arr_size * sizeof(uint8_t *));
    new_environ = (uint8_t**)CALLOC( new_size, sizeof(uint8_t) );
    CHECK_PTR_RET( new_environ, FALSE );

    /* copy over the default basic environment */
    ptr = (uint8_t*)new_environ + (arr_size * sizeof(uint8_t*));
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
        *(ptr + len) = '=';
        MEMCPY( ptr + len + 1, value, strlen(value) + 1 );
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
            *(ptr + len) = '=';
            MEMCPY( ptr + len + 1, value, strlen(value) + 1 );
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

#if defined(UNIT_TESTING)

#include <CUnit/Basic.h>

void test_sanitize_files( void )
{
}

void test_sanitize_private_functions( void )
{
    test_sanitize_files();
}

#endif


