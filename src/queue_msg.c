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
#include "queue_msg.h"

#include "perror.h"

#if HAVE_SYS_MSG_H

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

struct priv {
  char*         path;
  key_t         key;
  int           mq;
  size_t        max_sz;
  size_t        max_cnt;
  int           flags;
};

#define MSG_TYPE 1  /* Must be positive non-zero. */

struct local_msgbuf {
  long mtype;    /* Type of received/sent message. */
  char mtext[1]; /* Text of the message. */
};


static void destructor (struct queue* this_queue)
{
  struct priv* ppriv = (struct priv*)this_queue->priv;

  if ( -1 != ppriv->mq )
    {
      msgctl (ppriv->mq, IPC_RMID, NULL); /* We ignore errors at this point. */
    }

  free (ppriv->path);
  free (ppriv);

  this_queue->vtbl = 0;
  this_queue->priv = 0;
}

/* This function will increase the maximum message size limit on Linux
 * systems.  It will silently do nothing on other systems.
 */

static void set_max_msg_size (size_t max_sz)
{
  size_t old_max = 0;

  FILE* fp = fopen("/proc/sys/kernel/msgmax", "r+");
  if ( NULL == fp )
    {
      return;
    }

  if ( fscanf (fp, "%ld", &old_max) != 1 )
    {
      LOG_WARN("Unable to read old max_msg_size.\n");
    }
  else if ( max_sz > old_max )
    {
      rewind (fp);
      if ( fprintf (fp, "%ld", max_sz) < 0 )
        {
          LOG_WARN("Unable to set max_msg_size to %ld.\n", max_sz);
        }
    }

  fclose (fp);
}

static int xopen (struct queue* this_queue, int flags)
{
  struct priv* ppriv = (struct priv*)this_queue->priv;
  struct msqid_ds ds;
  int mq;

  if ( -1 != ppriv->mq )
    {
      LOG_ER("Queue already open.\n");
      return -1;
    }

  /* Get a handle to our message queue. */
  mq = msgget (ppriv->key, 0666 | IPC_CREAT);
  if ( -1 == mq )
    {
      PERROR("msgget()");
      return -1;
    }

  /* Set the queue size in bytes. */
  set_max_msg_size (ppriv->max_sz);

  memset (&ds, 0, sizeof(ds));
  ds.msg_qbytes = ppriv->max_sz * ppriv->max_cnt;
  if ( msgctl (mq, IPC_SET, &ds) )
    {
      if ( EPERM == errno )
        {
          LOG_ER("Permission denied attempting to set the queue "
                 "size using msgctl() -- you may need to be root.\n");
        }
      else
        {
          PERROR("msgctl()");
          return -1;
        }
    }

  ppriv->mq = mq;
  ppriv->flags = flags;

  return 0;
}

static int xclose (struct queue* this_queue)
{
  struct priv* ppriv = (struct priv*)this_queue->priv;

  ppriv->mq = -1;  /* No additional close operation required. */
  return 0;
}

static int xread (struct queue* this_queue, void* buf,
                  size_t count, int* pending)
{
  struct priv* ppriv = (struct priv*)this_queue->priv;

  struct local_msgbuf* mp;
  struct msqid_ds ds;
  int msgrcv_ret;

  if ( O_WRONLY == ppriv->flags )
    {
      LOG_ER("Queue read attempted without appropriate permission.\n");
      return -1;
    }
  if ( 0 == buf )
    {
      LOG_ER("Queue read with NULL buf pointer\n");
      return -1;
    }
  if ( -1 == ppriv->mq )
    {
      LOG_ER("Queue read with queue closed.\n");
      return -1;
    }
  if ( count > ppriv->max_sz )
    {
      LOG_ER("Queue read with  count (%d) > max_sz (%d).\n",
             count, ppriv->max_sz);
      return -1;
    }

  mp = (struct local_msgbuf*)(((char*)buf) - sizeof(mp->mtype));

  /* Receive bytes. */
  if ( (msgrcv_ret = msgrcv(ppriv->mq, mp, count, MSG_TYPE, 0)) < 0 )
    {
      switch ( errno )
        {
        default:
          PERROR("msgrcv");

          /* If we've been interrupted or if the message queue was
           * removed, it's likely that we're shutting down, so no need to
           * print errors. */
        case EIDRM:
        case EINTR:
          return msgrcv_ret;
        }
    }

  if ( MSG_TYPE != mp->mtype )
    {
      LOG_WARN("Unrecognized message type %d.\n", mp->mtype);
    }

  if ( msgctl(ppriv->mq, IPC_STAT, &ds) )
    {
      PERROR("msgctl(ppriv->mq, IPC_GET, &ds)");
      return -1;
    }

  *pending =  ds.msg_qnum;

  return msgrcv_ret;
}

static int xwrite (struct queue* this_queue, const void* buf, size_t count)
{
  int retry = 99;
  struct priv* ppriv = (struct priv*)this_queue->priv;

  struct local_msgbuf* mp;
  int msgsnd_ret;

  if ( O_RDONLY == ppriv->flags )
    {
      LOG_ER("Queue write attempted without appropriate permission.\n");
      return -1;
    }
  if ( 0 == buf )
    {
      LOG_ER("Queue write with NULL buf pointer.\n");
      return -1;
    }
  if ( -1 == ppriv->mq )
    {
      LOG_ER("Queue write with queue closed.\n");
      return -1;
    }
  if ( count > ppriv->max_sz )
    {
      LOG_ER("Queue write with count (%d) > max_sz (%d).\n",
             count, ppriv->max_sz);
      return -1;
    }

  mp = (struct local_msgbuf*)(((char*)buf) - sizeof(mp->mtype));

  mp->mtype = MSG_TYPE;  /* Must be positive non-zero. */

  while ( ((msgsnd_ret = msgsnd(ppriv->mq, mp, count, IPC_NOWAIT)) < 0)
          && retry )
    {
      /* We'll try msgsnd a few times, since with IPC_NOWAIT we might
       * get a transient error if the queue is full.
       */
      switch (errno )
        {
        case EAGAIN:
          --retry;
          LOG_ER("Queue full, will now retry msgsnd.\n");
          break;

        default:
          PERROR("msgsnd");
          return msgsnd_ret;
        }
    }

  return msgsnd_ret;
}

static void* alloc (struct queue* this_queue, size_t* newcount)
{
  struct local_msgbuf* mp;
  struct priv* ppriv = (struct priv*)this_queue->priv;
  char* ret = (char*)malloc(ppriv->max_sz + sizeof(mp->mtype));

  if ( ret )
    {
      *newcount = ppriv->max_sz;
      return ret + sizeof(mp->mtype);
    }

  return 0;
}

static void dealloc (struct queue* this_queue, void* buf)
{
  struct local_msgbuf* mp =
    (struct local_msgbuf*)((char*)buf - sizeof(mp->mtype));
  (void)this_queue; /* appease -Wall -Werror */
  free(mp);
}

int queue_msg_ctor (struct queue* this_queue,
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

  ppriv = (struct priv*)malloc (sizeof(struct priv));
  if ( 0 == ppriv )
    {
      LOG_ER("Failed to allocate %d bytes for queue data.\n", sizeof(*ppriv));
      return -1;
    }
  memset(ppriv, 0, sizeof(*ppriv));

  if ( 0 == (ppriv->path = strdup(path)) )
    {
      LOG_ER("Strdup failed attempting to dup \"%s\".\n", path);
      free(ppriv);
      return -1;
    }

  /* We try to convert the path into an appropriate numeric key to
     identify this queue. */
  if ( sscanf(path, "%d", (int*)&ppriv->key) != 1 )
    {
      /* A default key is provided. */
      ppriv->key = 1;
    }

  ppriv->mq = -1;
  ppriv->max_sz = max_sz;
  ppriv->max_cnt = max_cnt;

  this_queue->vtbl = &vtbl;
  this_queue->priv = ppriv;

  return 0;
}

#else  /* if HAVE_SYS_MSG_H */

int queue_msg_ctor (struct queue* this_queue,
                    const char*   path,
                    size_t        max_sz,
                    size_t        max_cnt)
{
  this_queue->vtbl = 0;
  this_queue->priv = 0;
  return -1;
}

#endif /* HAVE_SYS_MSG_H */
