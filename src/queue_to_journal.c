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
#include "header.h"
#include "queue_to_journal.h"

#include "journal.h"
#include "log.h"
#include "opt.h"
#include "queue.h"
#include "sig.h"
#include "stats.h"
#include "perror.h"
#include "xport.h"
#include "stats.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct dequeuer_stats dst ;

static int rotate(struct journal* jrn, int jcurr)
{
  unsigned long long t0 = time_in_milliseconds(), t1;

  if ( jrn[jcurr].vtbl->close(&jrn[jcurr]) < 0 )
    {
      LOG_ER("Can't close journal  \"%s\".\n", arg_journalls[jcurr]);
      exit(EXIT_FAILURE);
    }

  jcurr = (jcurr + 1) % arg_njournalls;

  if ( jrn[jcurr].vtbl->open(&jrn[jcurr], O_WRONLY) < 0 )
    {
      LOG_ER("Failed to open the journal \"%s\".\n", arg_journalls[jcurr]);
      exit(EXIT_FAILURE);
    }

  t1 = time_in_milliseconds();
  LOG_INF("Rotated in %0.2f seconds\n", (t1-t0)/1000000.);

  return jcurr;
}

int queue_to_journal(void)
{
  struct queue que;
  struct journal* jrn;
  int jc;
  int jcurr = 0;
  void* buf = NULL ;
  size_t bufsiz;
  int pending = 0;
  unsigned long long t0, receive_time, max_receive_time=0,
      total_receive_time=0, write_time, max_write_time=0, total_write_time=0;

  dequeuer_stats_ctor(&dst);

  /* Create queue object. */
  if ( (queue_factory(&que) < 0) || (que.vtbl->open(&que, O_RDONLY) < 0) )
    {
      LOG_ER("Failed to create or open the queue.\n");
      exit(EXIT_FAILURE);
    }

  jrn = (struct journal*)malloc(arg_njournalls * sizeof(struct journal));
  if ( 0 == jrn )
    {
      LOG_ER("Failed to allocate space (%d bytes) for journal objects.\n",
             arg_njournalls * sizeof(struct journal));
      exit(EXIT_FAILURE);
    }

  for ( jc=0; jc<arg_njournalls; ++jc )
    {
      /* Create journal objects. */
      if ( journal_factory(&jrn[jc], arg_journalls[jc]) < 0 )
        {
          LOG_ER("Failed to create journal object for \"%s\".\n",
                 arg_journalls[jc]);
          exit(EXIT_FAILURE);
        }
    }

  if ( jrn[jcurr].vtbl->open(&jrn[jcurr], O_WRONLY) < 0 )
    {
      LOG_ER("Failed to open the journal \"%s\".\n",
             arg_journalls[jcurr]);
      exit(EXIT_FAILURE);
    }

  buf = que.vtbl->alloc(&que, &bufsiz);
  if ( NULL == buf )
    {
      LOG_ER("unable to allocate %d bytes for message buffer.\n", bufsiz);
      exit(EXIT_FAILURE);
    }
  memset(buf, 0, bufsiz);

  /* Read a packet from the queue, write it to the journal. */
  while ( ! gbl_done )
    {
      int que_read_ret;
      int jrn_write_ret;

      /* 0 out part of the the event name so if we get a rotate event
       * the program will not continually rotate */
      memset(buf, 0, HEADER_LENGTH+20);

      t0 = time_in_milliseconds();
      que_read_ret = que.vtbl->read(&que, buf, bufsiz, &pending);
      if (que_read_ret >= 0)
        {
          receive_time = time_in_milliseconds() - t0;
          max_receive_time =
            max_receive_time < receive_time ? receive_time : max_receive_time;
          total_receive_time += receive_time;
          dequeuer_stats_record(&dst, que_read_ret-HEADER_LENGTH, pending);
        }
      else if (que_read_ret == QUEUE_INTR )
        {
          /* ignore expected interrupts */
        }
      else
        {
          LOG_INF("Received other interruption\n");
          continue; /* no event, so do not process the rest */
        }

      // is this a command event?
      if ( header_is_rotate(buf) || gbl_rotate_dequeue || gbl_done )
        {
          if (header_is_rotate (buf))
            {
              // is it a new enough Command::Rotate, or masked out?
              memcpy(&dst.latest_rotate_header, buf, HEADER_LENGTH);
              dst.rotation_type = LJ_RT_EVENT;
            }
          dequeuer_stats_rotate(&dst);

          /* if we are shutting down the destructor will flush stats,
           * so skip so we don't get duplicate events sent to mondemand
           * if it is enabled
           */
          if (! gbl_done )
            {
              dequeuer_stats_flush();
            }

          LOG_INF("Maximum receive time was %0.2f seconds;"
                  " total receive time was %0.2f seconds\n",
                  max_receive_time/1000000., total_receive_time/1000000.);
          LOG_INF("Maximum write time was %0.2f seconds;"
                  " total write time was %0.2f seconds\n",
                  max_write_time/1000000., total_write_time/1000000.);
          LOG_INF("About to rotate journal (%d pending).\n", pending);
          jcurr = rotate(jrn,jcurr);
          max_receive_time = 0;
          max_write_time = 0;
          total_receive_time = 0;
          total_write_time = 0;

          if (gbl_rotate_dequeue)
            {
              __sync_bool_compare_and_swap(&gbl_rotate_dequeue,1,0);
            }
        }

      if (que_read_ret != QUEUE_INTR)
        {
          t0 = time_in_milliseconds();
          /* Write the packet out to the journal. */
          if ( (jrn_write_ret = jrn[jcurr].vtbl->write(&jrn[jcurr],
                                                       buf, que_read_ret))
               != que_read_ret )
            {
              LOG_ER("Journal write error -- attempted to write %d bytes, "
                     "write returned %d.\n", que_read_ret, jrn_write_ret);
              dequeuer_stats_record_loss(&dst);
            }
          write_time = time_in_milliseconds() - t0;
          max_write_time =
            max_write_time < write_time ? write_time : max_write_time;
          total_write_time += write_time;
        }
    } /* while ( ! gbl_done) */

  if ( jrn[jcurr].vtbl->close(&jrn[jcurr]) < 0 )
    {
      LOG_ER("Can't close journal  \"%s\".\n", arg_journalls[jcurr]);
    }
  for ( jc=0; jc<arg_njournalls; ++jc )
    {
      jrn[jc].vtbl->destructor(&jrn[jc]);
    }
  free(jrn);

  /* Empty the journaller system queue upon shutdown */
  while ( (que.vtbl->read(&que, buf, bufsiz, &pending) >= 0)
          && arg_queue_max_cnt-- )
    ;

  que.vtbl->dealloc(&que, buf);
  que.vtbl->destructor(&que);

  dequeuer_stats_rotate(&dst);
  dequeuer_stats_report(&dst);
  dequeuer_stats_dtor(&dst);

  return 0;
}
