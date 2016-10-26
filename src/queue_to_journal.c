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

  dequeuer_stats_rotate(&dst);
  dequeuer_stats_flush();

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
  int pending = 0, write_pending = 0;
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

  /* Read a packet from the queue, write it to the journal. */
  while ( ! gbl_done )
    {
      int que_read_ret;
      int jrn_write_ret;

      /* If we have a pending rotate to perform, do it now. */
      if ( gbl_rotate_dequeue )
        {
          LOG_INF("Maximum receive time was %0.2f seconds;"
                  " total receive time was %0.2f seconds\n",
                  max_receive_time/1000000., total_receive_time/1000000.);
          LOG_INF("Maximum write time was %0.2f seconds;"
                  " total write time was %0.2f seconds\n",
                  max_write_time/1000000., total_write_time/1000000.);
          LOG_INF("About to rotate journal (%d pending).\n", pending);
          write_pending = 1;
          jcurr = rotate(jrn,jcurr);
          max_receive_time = 0;
          max_write_time = 0;
          total_receive_time = 0;
          total_write_time = 0;

          gbl_rotate_dequeue = 0;
        }

      t0 = time_in_milliseconds();
      if ( (que_read_ret = que.vtbl->read(&que, buf, bufsiz, &pending)) < 0 )
        {
          /* queue is empty */
          if (gbl_done) break; /* if we're shutting down, exit this loop. */
          continue;            /* no event, so do not process the rest */
        }
      receive_time = time_in_milliseconds() - t0;
      if ( max_receive_time < receive_time ) max_receive_time = receive_time;
      total_receive_time += receive_time;

      LOG_PROG("Read %d bytes from queue (%d pending).\n",
               que_read_ret, pending);
      if (write_pending)
        {
          LOG_INF("Done with rotating journal (%d pending).\n", pending);
          write_pending = 0;
        }

      // is this a command event?
      if ( header_is_rotate(buf) )
        { // Command::Rotate
          // is it a new enough Command::Rotate, or masked out?
          memcpy(&dst.latest_rotate_header, buf, HEADER_LENGTH) ;
          dst.rotation_type = LJ_RT_EVENT;
          gbl_rotate_dequeue = 1;
          gbl_rotate_enqueue = 1;
        }

      t0 = time_in_milliseconds();
      dequeuer_stats_record(&dst, que_read_ret-HEADER_LENGTH, pending);
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
      if ( max_write_time < write_time ) max_write_time = write_time;
      total_write_time += write_time;
    } /* while ( ! gbl_done) */

  dequeuer_stats_rotate(&dst);
  dequeuer_stats_flush();

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

  dequeuer_stats_report(&dst);
  dequeuer_stats_dtor(&dst);

  return 0;
}
