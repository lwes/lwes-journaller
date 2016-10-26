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

#include "xport_to_queue.h"

#include "header.h"
#include "opt.h"
#include "perror.h"
#include "queue.h"
#include "sig.h"
#include "xport.h"
#include "stats.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void skd(void);

struct enqueuer_stats est ;

int xport_to_queue(void)
{
  struct xport xpt;
  struct queue que;

  unsigned char* buf = 0;
  size_t bufsiz;

  enqueuer_stats_ctor(&est);

  if ( arg_rt )
    {
      skd();
    }

  if ( (queue_factory(&que) < 0) || (que.vtbl->open(&que, O_WRONLY) < 0) )
    {
      LOG_ER("Failed to create or open queue object.\n");
      exit(EXIT_FAILURE);
    }

  /* Can we drop root here? */

  if ( (xport_factory(&xpt) < 0) || (xpt.vtbl->open(&xpt, O_RDONLY) < 0) )
    {
      LOG_ER("Failed to create xport object.\n");
      exit(EXIT_FAILURE);
    }

  buf = (unsigned char*)que.vtbl->alloc(&que, &bufsiz);
  if ( 0 == buf )
    {
      LOG_ER("unable to allocate %d bytes for message buffer.\n", bufsiz);
      exit(EXIT_FAILURE);
    }
  memset(buf, 0, HEADER_LENGTH); /* Clear the header portion of the message. */

  /* Read a packet from the transport, write it to the queue. */
  while ( ! gbl_done )
    {
      int que_write_ret;

      unsigned long long tm;
      unsigned long addr;
      short port;
      int xpt_read_ret = xpt.vtbl->read(&xpt,
                                        buf + HEADER_LENGTH,
                                        bufsiz - HEADER_LENGTH,
                                        &addr, &port);
      if (xpt_read_ret >= 0 )
        {
          tm = time_in_milliseconds();
          enqueuer_stats_record_datagram(&est, xpt_read_ret + HEADER_LENGTH);
          header_add(buf, xpt_read_ret, tm, addr, port);
        }
      else if (xpt_read_ret == XPORT_INTR)
        {
          ;
        }
      else
        {
          LOG_INF("Received other interruption\n");
          enqueuer_stats_record_socket_error(&est);
          continue;
        }

      if ( header_is_rotate (buf) || gbl_rotate_enqueue || gbl_done)
        { // Command::Rotate: here we just collect some stats.
          enqueuer_stats_rotate(&est);
          enqueuer_stats_flush();
          gbl_rotate_enqueue = 0;
        }

      if (xpt_read_ret != XPORT_INTR)
        {
          if ( (que_write_ret = que.vtbl->write(&que,
                                                buf,
                                                xpt_read_ret + HEADER_LENGTH)) < 0 )
            {
              LOG_ER("Queue write error attempting to write %d bytes.\n",
                     xpt_read_ret + HEADER_LENGTH);
              continue;
            }
          else
            {
              LOG_PROG("Queue write of %d bytes.\n",
                       xpt_read_ret + HEADER_LENGTH);
            }
        }
    }
  LOG_INF("xport shutting down\n");
  que.vtbl->dealloc(&que, buf);

  xpt.vtbl->destructor(&xpt);
  que.vtbl->destructor(&que);

  enqueuer_stats_report(&est);
  enqueuer_stats_dtor(&est);

  return 0;
}

#if HAVE_SCHED_H
#include <sched.h>

static void skd()
{
  struct sched_param sp;

  sp.sched_priority = sched_get_priority_max(SCHED_FIFO);
  if ( sched_setscheduler(0, SCHED_FIFO, &sp) )
    {
      PERROR("sched_setscheduler");
      LOG_WARN("Increasing thread priority failed"
               ", will run with standard priorities\n");
    }
  else
    {
      LOG_INF("Running with FIFO priority\n");
    }
}
#else
static void skd()
{
  LOG_WARN("No real-time scheduler support on this platform.\n");
}
#endif
