/*======================================================================*
 * Copyright (C) 2008 Light Weight Event System                         *
 * All rights reserved.                                                 *
 *                                                                      *
 * This program is free software; you can redistribute it and/or modify *
 * it under the terms of the GNU General Public License as published by *
 * the Free Software Foundation; either version 2 of the License, or    *
 * (at your option) any later version.                                  *
 *                                                                      *
 * This program is distributed in the hope that it will be useful,      *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 * GNU General Public License for more details.                         *
 *                                                                      *
 * You should have received a copy of the GNU General Public License    *
 * along with this program; if not, write to the Free Software          *
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,                   *
 * Boston, MA 02110-1301 USA.                                           *
 *======================================================================*/

#include "config.h"

#include "queue.h"
#include "queue_mqueue.h"

#include "perror.h"
#include "opt.h"

#if defined(HAVE_MQUEUE_H)

#include <sys/types.h>		/* Must be included before mqueue.h on FreeBSD 4.9. */
#include <mqueue.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The following is for Solaris. */
#if !defined(MQ_PRIO_MAX) && defined(_POSIX_MQ_PRIO_MAX)
#define MQ_PRIO_MAX _POSIX_MQ_PRIO_MAX
#endif

struct priv {
  char*         path;
  mqd_t         mq;
  size_t        max_sz;
  size_t        max_cnt;
};

static void destructor(struct queue* this_queue)
{
  struct priv* ppriv = (struct priv*)this_queue->priv;

  free(ppriv->path);
  free(ppriv);

  this_queue->vtbl = 0;
  this_queue->priv = 0;
}

static int xopen(struct queue* this_queue, int flags)
{
  struct priv* ppriv = (struct priv*)this_queue->priv;

  if ( (mqd_t)-1 == ppriv->mq ) {
    struct mq_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.mq_maxmsg = ppriv->max_cnt;
    attr.mq_msgsize = ppriv->max_sz;

		errno = 0 ;
		// see http://www.die.net/doc/linux/man/man7/mq_overview.7.html
    if ( (mqd_t)-1 == (ppriv->mq = mq_open(ppriv->path, flags | O_CREAT, 0666, &attr)) ) {
			// Try resetting the mqueue resource parameters before formally failing
			// see http://www.die.net/doc/linux/man/man2/getrlimit.2.html
			struct rlimit rlim ;
			PERROR("mq_open"); errno = 0 ;
			attr.mq_maxmsg += 1 ;
			rlim.rlim_cur = RLIM_INFINITY ; //attr.mq_maxmsg * sizeof(struct msg_msg *) + attr.mq_maxmsg * attr.mq_msgsize ;
			rlim.rlim_max = RLIM_INFINITY ; //rlim.rlim_cur ;
			if ( setrlimit(RLIMIT_MSGQUEUE, &rlim) )
				PERROR("setrlimit");
			attr.mq_maxmsg -= 1 ;

			if ( (mqd_t)-1 == (ppriv->mq = mq_open(ppriv->path, flags | O_CREAT, 0666, &attr)) ) {
				LOG_ER("mq_open(path=\"%s\", flags=0x%04x, 0666, {attr.mq_maxmsg=%i, attr.mq_msgsize=%i})\n",
					ppriv->path, flags | O_CREAT, attr.mq_maxmsg, attr.mq_msgsize);
	      return -1;
			}
    }
  }

  return 0;
}

static int xclose(struct queue* this_queue)
{
  struct priv* ppriv = (struct priv*)this_queue->priv;

  if ( (mqd_t)-1 != ppriv->mq ) {
    if ( (mq_close(ppriv->mq)) < 0 ) {
      PERROR("mq_close");
      return -1;
    }
  }

  ppriv->mq = (mqd_t)-1;

  return 0;
}

static int xread(struct queue* this_queue, void* buf, size_t count, int* pending)
{
  struct priv* ppriv = (struct priv*)this_queue->priv;
  struct mq_attr attr;
  int mq_rec_rtrn;
  int pri;

  if ( 0 == buf ) {
    LOG_ER("queue read with NULL buf pointer\n");
    return -1;
  }
  if ( (mqd_t)-1 == ppriv->mq ) {
    LOG_ER("queue read with queue closed.\n");
    return -1;
  }

  LOG_PROG("about to call mq_receive().\n");

  if ( (mq_rec_rtrn = mq_receive(ppriv->mq, buf, count, &pri)) < 0 ) {
    switch ( errno ) {
    default:
      PERROR("mq_receive"); 

      /* If we've been interrupted it's likely that we're shutting
	 down, so no need to print errors. */
    case EINTR:
      return mq_rec_rtrn;
    }
  }

  if ( MQ_PRIO_MAX - 1 != pri ) {
    LOG_ER("queue read returned message with unrecognized priority (pri=%d).\n",
           pri);
  }

  LOG_PROG("mq_receive() returned %d.\n", mq_rec_rtrn);

  if ( mq_getattr(ppriv->mq, &attr) < 0 ) {
    PERROR("mq_getattr");
    return -1;
  }

  *pending = attr.mq_curmsgs;

  return mq_rec_rtrn;
}

static int xwrite(struct queue* this_queue, const void* buf, size_t count)
{
  struct priv* ppriv = (struct priv*)this_queue->priv;

  if ( 0 == buf ) {
    LOG_ER("queue write with NULL buf pointer\n");
    return -1;
  }
  if ( (mqd_t)-1 == ppriv->mq ) {
    LOG_ER("queue write with queue closed.\n");
    return -1;
  }

  LOG_PROG("about to call mq_send().\n");

  if ( 0 != mq_send(ppriv->mq, buf, count, MQ_PRIO_MAX-1) ) {
    PERROR("mq_send");
    return -1;
  }

  return 0;
}

static void* alloc(struct queue* this_queue, size_t* newcount)
{
  return malloc(*newcount = ((struct priv*)this_queue->priv)->max_sz);
}

static void dealloc(struct queue* this_queue, void* buf)
{
  free(buf);
}

int queue_mqueue_ctor(struct queue* this_queue,
		      const char*   path,
		      size_t        max_sz,
		      size_t        max_cnt)
{
  static struct queue_vtbl vtbl = {
    destructor,
    xopen, xclose,
    xread, xwrite,
    alloc, dealloc
  };

  struct priv* ppriv;

  this_queue->vtbl = 0;
  this_queue->priv = 0;

  ppriv = (struct priv*)malloc(sizeof(struct priv));
  if ( 0 == ppriv ) {
    LOG_ER("malloc failed attempting to allocate %d bytes\n",
           sizeof(*ppriv));
    return -1;
  }
  memset(ppriv, 0, sizeof(*ppriv));

  if ( 0 == (ppriv->path = strdup(path)) ) {
    LOG_ER("strdup failed attempting to dup \"%s\"\n",
           path);
    free(ppriv);
    return -1;
  }

  ppriv->mq = (mqd_t)-1;
  ppriv->max_sz = max_sz;
  ppriv->max_cnt = max_cnt;

  this_queue->vtbl = &vtbl;
  this_queue->priv = ppriv;

  return 0;
}

#else  /* if defined(HAVE_MQUEUE_H) */

int queue_mqueue_ctor(struct queue* this_queue,
		      const char*   path,
		      size_t        max_sz,
		      size_t        max_cnt)
{
  this_queue->vtbl = 0;
  this_queue->priv = 0;
  return -1;
}

#endif /* HAVE_MQUEUE_H */
