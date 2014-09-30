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
#include <paths.h>

#include "macros.h"
#include "list.h"
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

/* write log messages out to a list_t log */
static ssize_t listlog_writer( void * cookie, char const * data, size_t leng )
{
    list_t * list = (list_t*)cookie;
    uint8_t * str = CALLOC( 1, leng + 1 );
    MEMCPY( str, data, leng );
    list_push_tail( list, str );
    return leng;
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
static cookie_io_functions_t listlog_fns =
{
    (void*) noop,
    (void*) listlog_writer,
    (void*) noop,
    (void*) noop
};
#endif

log_t * start_logging( log_type_t type, void * param, int append )
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
            openlog( (uint8_t const * const)param, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_DAEMON );

#if defined linux
            /* redirect stderr writes to our custom writer function that outputs to syslog */
            setvbuf(stderr = fopencookie(NULL, "w", syslog_fns), NULL, _IOLBF, 0);
#elif defined __APPLE__
            /* redirect stderr writes to our custom writer function that outputs to syslog */
            setvbuf(stderr = fwopen( NULL, (writefn)syslog_writer ), NULL, _IOLBF, 0);
#endif
            break;

        case LOG_TYPE_FILE:
            CHECK_PTR_GOTO( param, _start_logging_fail );

            /* open the file */
            log->cookie = (void*)fopen( (uint8_t const * const)param, mode[append] );
            CHECK_PTR_GOTO( log->cookie, _start_logging_fail );

#if defined linux
            /* redirect stderr writes to our custom writer function that outputs to a file */
            setvbuf(stderr = fopencookie( log->cookie, "w", filelog_fns), NULL, _IOLBF, 0);
#elif defined __APPLE__
            /* redirect stderr writes to our custom writer function that outputs to a file */
            setvbuf(stderr = fwopen( log->cookie, (writefn)filelog_writer ), NULL, _IOLBF, 0);
#endif
            break;

        case LOG_TYPE_STDERR:
            /* do nothing */
            break;

        case LOG_TYPE_LIST:
            CHECK_PTR_GOTO( param, _start_logging_fail );

            /* clear the list if not appending */
            if ( !append )
                list_clear( (list_t*)param );

            /* remember the list pointer */
            log->cookie = param;

#if defined linux
            /* redirect stderr writes to our custom writer function that outputs to a list */
            setvbuf(stderr = fopencookie( log->cookie, "w", listlog_fns), NULL, _IOLBF, 0);
#elif defined __APPLE__
            /* redirect stderr writes to our custom writer function that outputs to a list */
            setvbuf(stderr = fwopen( log->cookie, (writefn)listlog_writer ), NULL, _IOLBF, 0);
#endif
            break;
 

        /* case ZEROMQ_LOG: */
        /* case UDP_LOG: */
    }

    /* record the type */
    log->type = type;

    return log;

_start_logging_fail:
    FREE( log );
    return NULL;
}

void stop_logging( log_t * log )
{
    CHECK_PTR( log );

    switch( log->type )
    {
        case LOG_TYPE_SYSLOG:
            closelog();
            /* reset stderr stream by opening /dev/null */
            freopen( _PATH_DEVNULL, "wb", stderr );
            break;

        case LOG_TYPE_FILE:
            fclose( (FILE*)log->cookie );
            /* reset stderr stream by opening /dev/null */
            freopen( _PATH_DEVNULL, "wb", stderr );
            break;

        case LOG_TYPE_STDERR:
            /* do nothing */
            break;

        case LOG_TYPE_LIST:
            /* reset stderr stream by opening /dev/null */
            freopen( _PATH_DEVNULL, "wb", stderr );
            break;
    }
    FREE( log );
}

