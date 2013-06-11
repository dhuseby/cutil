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

#ifndef __LOG_H__
#define __LOG_H__

typedef enum log_type_s
{
    LOG_TYPE_SYSLOG,
    LOG_TYPE_FILE,
    LOG_TYPE_STDERR

} log_type_t;

typedef struct log_s
{
    log_type_t type;
    void * cookie;

} log_t;

log_t * start_logging( log_type_t type, int8_t const * const param, int append );
void stop_logging( log_t * log );

#endif/*__LOG_H__*/

