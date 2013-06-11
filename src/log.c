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

#if defined(__APPLE__)
typedef int (*writefn)(void *, const char *, int);
#endif

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
static ssize_t syslog_writer( void * cookie, char const * data, size_t leng )
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
    syslog( p, "%.*s", (int)leng, data );
    return leng;
}

/* write log messages out to the file log */
static ssize_t filelog_writer( void * cookie, char const * data, size_t leng )
{
    FILE* flog = (FILE*)cookie;
    return fprintf( flog, "%.*s", (int)leng, data );
}

static int noop( void ) { return 0; }

#ifdef linux
static cookie_io_functions_t syslog_fns =
{
    (void*) noop,
    (void*) syslog_writer,
    (void*) noop,
    (void*) noop
};
static cookie_io_functions_t filelog_fns =
{
    (void*) noop,
    (void*) filelog_writer,
    (void*) noop,
    (void*) noop
};
#endif

log_t * start_logging( log_type_t type, int8_t const * const param, int append )
{
    char const * const mode[2] = { "w+", "a+" };
    log_t * log = (log_t*)CALLOC( 1, sizeof(log_t) );
    CHECK_PTR_RET( log, NULL );

    switch ( type )
    {
        case LOG_TYPE_SYSLOG:

            /* allow everything except LOG_DEBUG messages to get to syslog */
            setlogmask( LOG_UPTO( LOG_INFO ) );

            /* most systems route the LOG_DAEMON facility to /var/log/daemon.log */
            openlog( param, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_DAEMON );

#if defined linux
            /* redirect stderr writes to our custom writer function that outputs to syslog */
            setvbuf(stderr = fopencookie(NULL, "w", syslog_fns), NULL, _IOLBF, 0);
#elif defined __APPLE__
            /* redirect stderr writes to our custom writer function that outputs to syslog */
            setvbuf(stderr = fwopen( NULL, (writefn)syslog_writer ), NULL, _IOLBF, 0);
#endif
            break;

        case LOG_TYPE_FILE:

            /* open the file */
            log->cookie = (void*)fopen( param, mode[append] );

            if ( log->cookie == NULL )
            {
                WARN( "failed to open log file\n" );
                FREE(log);
                return NULL;
            }

#if defined linux
            /* redirect stderr writes to our custom writer function that outputs to syslog */
            setvbuf(stderr = fopencookie( log->cookie, "w", filelog_fns), NULL, _IOLBF, 0);
#elif defined __APPLE__
            /* redirect stderr writes to our custom writer function that outputs to syslog */
            setvbuf(stderr = fwopen( log->cookie, (writefn)filelog_writer ), NULL, _IOLBF, 0);
#endif
            break;

        case LOG_TYPE_STDERR:
            /* do nothing */
            break;

        /* case ZEROMQ_LOG: */
        /* case UDP_LOG: */
    }

    return log;
}

void stop_logging( log_t * log )
{
    CHECK_PTR( log );

    switch( log->type )
    {
        case LOG_TYPE_SYSLOG:
            closelog();
            break;

        case LOG_TYPE_FILE:
            fclose( (FILE*)log->cookie );
            break;

        case LOG_TYPE_STDERR:
            /* do nothing */
            break;
    }
    FREE( log );
}

