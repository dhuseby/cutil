/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */

#ifndef __AIOFD_H__
#define __AIOFD_H__

#include <sys/uio.h>
#include "events.h"
#include "list.h"

typedef struct aiofd_s aiofd_t;
typedef struct aiofd_ops_s aiofd_ops_t;

struct aiofd_s
{
	int			wfd;			/* read/write fd, if only one given, write-only otherwise */
	int			rfd;			/* read fd if two are given */
	int			listen;			/* is this a bound and listening socket fd? */
	list_t		wbuf;			/* array of buffers waiting to be written */
	evt_t		wevt;			/* write event */
	evt_t		revt;			/* read event */
	evt_loop_t*	el;				/* event loop we registered out evt with */
	void *		user_data;		/* context to pass to callbacks */

	struct aiofd_ops_s
	{
		int (*read_fn)( aiofd_t * const aiofd, size_t nread, void * user_data );
		int (*write_fn)( aiofd_t * const aiofd, uint8_t const * const buffer, void * user_data );
		int (*error_fn)( aiofd_t * const aiofd, int err, void * user_data );
	}			ops;
};


aiofd_t * aiofd_new( int const write_fd,
					 int const read_fd,
					 aiofd_ops_t * const ops,
					 evt_loop_t * const el,
					 void * user_data );
void aiofd_delete( void * aio );

int aiofd_initialize( aiofd_t * const aiofd, 
					  int const write_fd,
					  int const read_fd,
					  aiofd_ops_t * const ops,
					  evt_loop_t * const el,
					  void * user_data );
void aiofd_deinitialize( aiofd_t * const aiofd );

/* enables/disables processing of the read and write events */
int aiofd_enable_write_evt( aiofd_t * const aiofd, int enable );
int aiofd_enable_read_evt( aiofd_t * const aiofd, int enable );

/* read data from the fd */
int32_t aiofd_read( aiofd_t * const aiofd, 
					uint8_t * const buffer, 
					int32_t const n );

/* write data to the fd */
int aiofd_write( aiofd_t * const aiofd, 
				 uint8_t const * const buffer, 
				 size_t const n );

/* write iovec to the fd */
int aiofd_writev( aiofd_t * const aiofd,
				  struct iovec * iov,
				  size_t iovcnt );

/* flush the fd output */
int aiofd_flush( aiofd_t * const aiofd );

/* get/set the listening fd flag, used for bound and listening socket fd's */
int aiofd_set_listen( aiofd_t * const aiofd, int listen );
int aiofd_get_listen( aiofd_t * const aiofd );


#endif/*__AIOFD_H__*/

