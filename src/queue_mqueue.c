/*======================================================================*
 * Copyright (c) 2008, Yahoo! Inc. All rights reserved.                 *
 * Copyright (c) 2010-2016, OpenX Inc.   All rights reserved.           *
 *                                                                      *
 * Licensed under the New BSD License (the "License"); you may not use  *
 * this file except in compliance with the License.  Unless required    *
 * by applicable law or agreed to in writing, software distributed      *
 * under the License is distributed on an "AS IS" BASIS, WITHOUT        *
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.     *
 * See the License for the specific language governing permissions and  *
 * limitations under the License. See accompanying LICENSE file.        *
 *======================================================================*/

#include "config.h"

#include "queue.h"
#include "queue_mqueue.h"

#include "perror.h"
#include "opt.h"
#include "sig.h"

#if defined(HAVE_MQUEUE_H)

#include <sys/types.h>  /* Must be included before mqueue.h on FreeBSD 4.9. */
#include <mqueue.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

static void destructor (struct queue* this_queue)
{
  struct priv* ppriv = (struct priv*)this_queue->priv;

  free (ppriv->path);
  free (ppriv);

  this_queue->vtbl = 0;
  this_queue->priv = 0;
}

static int xopen (struct queue* this_queue, int flags)
{
  struct priv* ppriv = (struct priv*)this_queue->priv;

  if ( (mqd_t)-1 == ppriv->mq )
    {
      struct mq_attr attr;
      memset(&attr, 0, sizeof(attr));
      attr.mq_maxmsg = ppriv->max_cnt;
      attr.mq_msgsize = ppriv->max_sz;

      /* see http://www.die.net/doc/linux/man/man7/mq_overview.7.html */
      if ( (mqd_t)-1 == (ppriv->mq = mq_open (ppriv->path,
                                              flags | O_CREAT, 0666, &attr)) )
        {
          /* Try resetting the mqueue resource parameters before formally
           * failing see http://www.die.net/doc/linux/man/man2/getrlimit.2.html
           */
          struct rlimit rlim;
          attr.mq_maxmsg += 1;
          rlim.rlim_cur = RLIM_INFINITY;
          rlim.rlim_max = RLIM_INFINITY ;
          setrlimit (RLIMIT_MSGQUEUE, &rlim);
          attr.mq_maxmsg -= 1 ;
          if ( (mqd_t)-1 == (ppriv->mq = mq_open (ppriv->path,
                                                  flags | O_CREAT,
                                                  0666, &attr)) )
            {
              return QUEUE_ERROR;
            }
        }
    }
  return QUEUE_OK;
}

static int xclose (struct queue* this_queue)
{
  struct priv* ppriv = (struct priv*)this_queue->priv;

  if ( (mqd_t)-1 != ppriv->mq )
    {
      if ( (mq_close (ppriv->mq)) < 0 )
        {
          return QUEUE_ERROR;
        }
    }

  ppriv->mq = (mqd_t)-1;

  return QUEUE_OK;
}

static int xread (struct queue* this_queue, void* buf,
                  size_t count, int* pending)
{
  struct priv* ppriv = (struct priv*)this_queue->priv;
  struct mq_attr attr;
  int mq_rec_rtrn;
  unsigned int pri;

  if ( 0 == buf )
    {
      /* empty buffer */
      return QUEUE_MEM_ERROR;
    }
  if ( (mqd_t)-1 == ppriv->mq )
    {
      /* queue read with queue closed. */
      return QUEUE_CLOSED_ERROR;
    }

  /* use blocking reads unless we're shutting down. */
  int wakeup_secs = arg_wakeup_interval_ms / 1000;
  int wakeup_ms = arg_wakeup_interval_ms % 1000;
  struct timespec time_buf = { time(NULL) + wakeup_secs, wakeup_ms * 1000000 };
  mq_rec_rtrn = mq_timedreceive (ppriv->mq, buf, count, &pri, &time_buf);

  if (mq_rec_rtrn < 0 )
    {
      switch ( errno )
        {
          /* If we've been interrupted it's likely that we're shutting
             down, so no need to print errors. */
          case ETIMEDOUT:
          case EINTR:
            return QUEUE_INTR;

          default:
            break;
        }
    }

  if ( mq_getattr (ppriv->mq, &attr) < 0 )
    {
      /* failed to get attributes */
      return QUEUE_ERROR;
    }

  *pending = attr.mq_curmsgs;

  if (mq_rec_rtrn < 0)
    {
      return QUEUE_READ_ERROR;
    }
  else
    {
      return mq_rec_rtrn;
    }
}

static int xwrite (struct queue* this_queue, const void* buf, size_t count)
{
  struct priv* ppriv = (struct priv*)this_queue->priv;

  if ( 0 == buf )
    {
      /* empty buffer */
      return QUEUE_MEM_ERROR;
    }
  if ( (mqd_t)-1 == ppriv->mq )
    {
      /* queue write with queue closed. */
      return QUEUE_CLOSED_ERROR;
    }

  if ( 0 != mq_send (ppriv->mq, buf, count, MQ_PRIO_MAX-1) )
    {
      /* send error */
      return QUEUE_WRITE_ERROR;
    }

  return QUEUE_OK;
}

static void* alloc (struct queue* this_queue, size_t* newcount)
{
  void *data = malloc (*newcount = ((struct priv*)this_queue->priv)->max_sz);
  memset(data, 0, *newcount);
  return data;
}

static void dealloc (struct queue* this_queue, void* buf)
{
  (void) this_queue; /* appease -Wall -Werror */
  free (buf);
}

int queue_mqueue_ctor (struct queue* this_queue,
                       const char*   path,
                       size_t        max_sz,
                       size_t        max_cnt,
                       FILE *        log)
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
  if ( 0 == ppriv )
    {
      LOG_ER(log,
             "Failed to allocate %d bytes for queue data\n",
             sizeof(*ppriv));
      return QUEUE_MEM_ERROR;
    }
  memset(ppriv, 0, sizeof(*ppriv));

  if ( 0 == (ppriv->path = strdup(path)) )
    {
      LOG_ER(log, "Failed attempting to dup \"%s\"\n", path);
      free(ppriv);
      return QUEUE_MEM_ERROR;
    }

  ppriv->mq = (mqd_t)-1;
  ppriv->max_sz = max_sz;
  ppriv->max_cnt = max_cnt;

  this_queue->vtbl = &vtbl;
  this_queue->priv = ppriv;

  return QUEUE_OK;
}

#else  /* if defined(HAVE_MQUEUE_H) */

int queue_mqueue_ctor (struct queue* this_queue,
                       const char*   path,
                       size_t        max_sz,
                       size_t        max_cnt,
                       FILE *        log)
{
  this_queue->vtbl = 0;
  this_queue->priv = 0;
  (void)path;     /* appease -Wall -Werror */
  (void)max_sz;   /* appease -Wall -Werror */
  (void)max_cnt;  /* appease -Wall -Werror */
  (void)log;      /* appease -Wall -Werror */

  return QUEUE_ERROR;
}

#endif /* HAVE_MQUEUE_H */
