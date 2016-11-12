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

#include "stats.h"

#include "log.h"
#include "lwes_mondemand.h"
#include "time_utils.h"

#include <string.h>  /* memset */

#ifdef HAVE_MONDEMAND
#include <mondemand.h>
#endif

#define log_rates(log, bps, pps, notes) \
          LOG_INF(log, " %g %sbps, %g pps%s.\n", \
                  bps > 1000000. ? bps / 1000000. : \
                  bps > 1000.    ? bps / 1000. : \
                                   bps, \
                  bps > 1000000. ? "m" : \
                  bps > 1000.    ? "k" : \
                                   "", \
                  pps, \
                  notes)

int enqueuer_stats_ctor (struct enqueuer_stats* st)
{
  memset (st, 0, sizeof(*st));

  st->start_time = time (NULL);
  st->last_rotate = st->start_time;
  md_enqueuer_create (st);

  return 0;
}

void enqueuer_stats_dtor (struct enqueuer_stats* st)
{
  md_enqueuer_destroy (st);
}

void enqueuer_stats_record_socket_error (struct enqueuer_stats* st)
{
  ++st->socket_errors_since_last_rotate;
}

void enqueuer_stats_record_datagram (struct enqueuer_stats* st, int bytes)
{
  st->bytes_received_since_last_rotate += bytes;
  st->bytes_received_total             += bytes;
  ++st->packets_received_since_last_rotate;
  ++st->packets_received_total;
}

void enqueuer_stats_erase_datagram (struct enqueuer_stats* st, int bytes)
{
  st->bytes_received_since_last_rotate -= bytes;
  st->bytes_received_total             -= bytes;
  --st->packets_received_since_last_rotate;
  --st->packets_received_total;
}

void enqueuer_stats_rotate(struct enqueuer_stats* st, FILE *log)
{
  double rbps, rpps;

  time_t now = time(NULL);
  time_t uptime;

  if ( st->socket_errors_since_last_rotate )
    {
      LOG_ER(log,"*** %lld packets had socket errors in this journal ***\n",
                 st->socket_errors_since_last_rotate);
    }

  LOG_INF(log,"Socket read errors since last rotate: %lld\n",
              st->socket_errors_since_last_rotate);

  LOG_INF(log,"Events read since last rotate:\n");
  LOG_INF(log," %lld bytes, %lld packets received.\n",
          st->bytes_received_since_last_rotate,
          st->packets_received_since_last_rotate);
  uptime = now - st->last_rotate;
  rbps = (8. * (double)st->bytes_received_since_last_rotate) / (double)uptime;
  rpps = ((double)st->packets_received_since_last_rotate) / (double)uptime;
  log_rates(log, rbps,rpps," received");

  LOG_INF(log, "Enqueuer stats summary v2:\t%d\t%lld\t%lld\t%lld\t%lld\t%lld\t%d\n",
          now, st->socket_errors_since_last_rotate,
          st->bytes_received_total, st->bytes_received_since_last_rotate,
          st->packets_received_total, st->packets_received_since_last_rotate,
          uptime);

  md_enqueuer_stats (st);

  st->socket_errors_since_last_rotate = 0LL;
  st->bytes_received_since_last_rotate = 0LL;
  st->packets_received_since_last_rotate = 0LL;
  st->last_rotate = now;
}

void enqueuer_stats_report (struct enqueuer_stats* st, FILE *log)
{
  double rbps, rpps;
  time_t now = time(NULL);
  time_t uptime;

  uptime = now - st->start_time;

  rbps = (8. * (double)st->bytes_received_total) / (double)uptime;
  rpps = ((double)st->packets_received_total) / (double)uptime;

  LOG_INF(log, "Total network traffic received:\n");
  LOG_INF(log, " %lld bytes, %lld packets.\n",
               st->bytes_received_total,
               st->packets_received_total);
  log_rates(log, rbps,rpps," received");
}

void enqueuer_stats_flush(struct enqueuer_stats* st)
{
  md_enqueuer_flush (st);
}


int dequeuer_stats_ctor (struct dequeuer_stats* st)
{
  memset (st, 0, sizeof(*st));

  st->start_time = time (NULL);
  st->hiq_start = st->start_time;
  st->last_rotate = st->start_time;
  st->rotation_type = LJ_RT_NONE;
  md_dequeuer_create (st);

  return 0;
}

void dequeuer_stats_dtor (struct dequeuer_stats* st)
{
  md_dequeuer_destroy (st);
}

void dequeuer_stats_record (struct dequeuer_stats* st, int bytes, int pending)
{
  /* Collect some stats.  All byte counts represent the bytes used
     in the original packets, not with the headers applied. */

  st->bytes_written_since_last_rotate += bytes;
  st->bytes_written_total             += bytes;

  ++st->packets_written_since_last_rotate;
  ++st->packets_written_total;

  /* No pending packets in queue. */
  if ( 0 == pending )
    {
      if ( st->hiq )
        {
          /* We just grabbed that last packet of a burst. */
          st->bytes_written_in_burst_since_last_rotate = st->bytes_written_in_burst;
          st->packets_written_in_burst_since_last_rotate = st->packets_written_in_burst;

          st->hiq = 0;
          st->bytes_written_in_burst = 0;
          st->packets_written_in_burst = 0;
        }
    }
  else  /* In a burst. */
    {
      /* This is a new burst. */
      if ( 0 == st->hiq )
        {
          st->hiq_last = st->hiq_start;
          st->hiq_start = time(NULL);
        }
      /* Possible peak. */
      if ( pending > st->hiq )
        {
          st->hiq = pending;
        }

      st->bytes_written_in_burst += bytes;
      st->packets_written_in_burst += 1;
    }

  if ( st->hiq > st->hiq_since_last_rotate )
    {
      st->hiq_since_last_rotate = st->hiq ;
    }
}

void dequeuer_stats_record_loss (struct dequeuer_stats* st)
{
  st->loss_since_last_rotate += 1;
}


void dequeuer_stats_rotate(struct dequeuer_stats* st, FILE *log)
{
  double wbps, wpps;

  time_t now = time(NULL);
  time_t uptime;

  if ( st->loss_since_last_rotate )
    {
      LOG_ER(log, "*** %lld packets lost in this journal ***\n",
             st->loss_since_last_rotate);
    }

  LOG_INF(log, "Events written since last rotate:\n");
  LOG_INF(log, " %lld bytes, %lld packets in this journal.\n",
          st->bytes_written_since_last_rotate,
          st->packets_written_since_last_rotate);
  uptime = now - st->last_rotate;
  wbps = (8. * (double)st->bytes_written_since_last_rotate) / (double)uptime;
  wpps = ((double)st->packets_written_since_last_rotate) / (double)uptime;
  log_rates(log, wbps,wpps," written");

  LOG_INF(log, "Highest queue utilization since last rotate: %d packets.\n",
          st->hiq_since_last_rotate);
  LOG_INF(log, "Highest burst since last rotate: %lli packets, %lli bytes.\n",
          st->packets_written_in_burst_since_last_rotate,
          st->bytes_written_in_burst_since_last_rotate);

  if (st->rotation_type == LJ_RT_EVENT) {
    LOG_INF(log, "Command::Rotate from IP %s traversed the queue in %ld ms\n",
            header_sender_ip_formatted(st->latest_rotate_header),
            millis_now()-header_receipt_time(st->latest_rotate_header));
  }

  LOG_INF(log, "Dequeuer stats summary v2:\t%d\t%lld\t%lld\t%lld\t%lld\t%lld\t%lld\t%lld\t%d\t%d\t%d\t%d\t%lld\t%lld\t%d\n",
          now, st->loss_since_last_rotate,
          st->bytes_written_total, st->bytes_written_since_last_rotate,
          st->packets_written_total, st->packets_written_since_last_rotate,
          st->bytes_written_in_burst, st->packets_written_in_burst, st->hiq,
          st->hiq_start, st->hiq_last, st->hiq_since_last_rotate,
          st->bytes_written_in_burst_since_last_rotate,
          st->packets_written_in_burst_since_last_rotate, uptime);

  md_dequeuer_stats (st);

  st->bytes_written_since_last_rotate = 0LL;
  st->packets_written_since_last_rotate = 0LL;
  st->hiq_since_last_rotate = 0;
  st->packets_written_in_burst_since_last_rotate = 0LL;
  st->bytes_written_in_burst_since_last_rotate = 0LL;
  st->loss_since_last_rotate = 0LL;
  st->last_rotate = now;
  st->rotation_type = LJ_RT_NONE;
}

void dequeuer_stats_report (struct dequeuer_stats* st, FILE *log)
{
  double wbps, wpps;
  time_t now = time(NULL);
  time_t uptime;

  uptime = now - st->start_time;

  wbps = (8. * (double)st->bytes_written_total) / (double)uptime;
  wpps = ((double)st->packets_written_total) / (double)uptime;

  LOG_INF(log, "Total network traffic recorded:\n");
  LOG_INF(log, " %lld bytes, %lld packets.\n",
               st->bytes_written_total,
               st->packets_written_total);
  log_rates(log,wbps,wpps," written");
}

void dequeuer_stats_flush (struct dequeuer_stats* st) {
  md_dequeuer_flush (st);
}


