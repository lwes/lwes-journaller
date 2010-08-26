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

void* queue_to_journal(void* arg)
{
  struct queue que;
  struct journal* jrn;
  int jc;
  int jcurr = 0;
  void* buf = NULL ;
  size_t bufsiz;
  int pending ;

  time_t last_rotate = time(NULL);
  time_t this_rotate ;

  (void)arg; /* appease -Wall -Werror */

  dequeuer_stats_ctor(&dst);

  install_signal_handlers();
  install_rotate_signal_handlers(); /* This is the process or thread
                                       that does the rotate. */
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
  while ( 1 )
    {
      int que_read_ret;
      int jrn_write_ret;

      /* If we have a pending rotate to perform, do it now. */
      if ( gbl_rotate )
        { // <gbl_rotate>

          LOG_PROG("About to rotate journal.\n");

          dequeuer_stats_rotate(&dst);
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

          gbl_rotate = 0;
        } // </gbl_rotate>

      if ( (que_read_ret = que.vtbl->read(&que, buf, bufsiz, &pending)) < 0 )
        {
          /* queue is empty; if we're shutting down, exit this loop. */
          if (gbl_done) break;
          /* queue is empty */
          continue;
      }
      LOG_PROG("Read %d bytes from queue (%d pending).\n",
               que_read_ret, pending);

      // is this a command event?
      switch ( header_is_rotate(buf, &this_rotate) )
        {
          case 2:
              { // System::Ping
                ping(buf, que_read_ret) ;
                goto fallthru ;
              }

          case 1:
              { // Command::Rotate
                // is it a new enough Command::Rotate, or masked out?
                time_t since = this_rotate - last_rotate;
                if ( since < arg_rotate_mask )
                  {
                    continue ; // don't respond to duplicate Command::Rotate's
                  }

                last_rotate = this_rotate ;
                memcpy(&dst.latest_rotate_header, buf, HEADER_LENGTH) ;
                gbl_rotate = 1;

                // fall through to write this Command::Rotate out before
                // looping to actually rotate.
              }

          default:
fallthru:
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
        }
    } /* while ( ! gdb_done) */

  dequeuer_stats_rotate(&dst);
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

  return 0;
}
