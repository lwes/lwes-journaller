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

#include "header.h"
#include "log.h"
#include "opt.h"
#include "sig.h"
#include "marshal.h"
#include "perror.h"
#include "lwes_mondemand.h"

#include <string.h>
#include <sys/time.h>
#include <time.h>

/* To rid us of gcc warnings, see the strftime(3) man page on Linux
   for more info. */

static size_t my_strftime (char* s, size_t max, const char* fmt,
                           const struct tm* tm)
{
  return strftime (s, max, fmt, tm);
}

unsigned long long time_in_milliseconds ()
{
  struct timeval t;

  if ( -1 == gettimeofday(&t, 0) ) /* This is where we need to */
    PERROR("gettimeofday"); /*  timestamp the packet. */

  return (((long long)t.tv_sec) * 1000LL) + (long long)(t.tv_usec/1000);
}

int enqueuer_stats_ctor (struct enqueuer_stats* st)
{
  memset (st, 0, sizeof(*st));

  st->start_time = time (NULL);
  st->last_rotate = st->start_time;
  mondemand_enqueuer_stats_init();

  return 0;
}
void enqueuer_stats_dtor (struct enqueuer_stats* st)
{
  (void)st;
  printf ("about to clear mondemand enqueuer\n");
  mondemand_enqueuer_stats_free();
}

int dequeuer_stats_ctor (struct dequeuer_stats* st)
{
  memset (st, 0, sizeof(*st));

  st->start_time = time (NULL);
  st->hiq_start = st->start_time;
  st->last_rotate = st->start_time;
  st->rotation_type = LJ_RT_NONE;
  mondemand_dequeuer_stats_init();

  return 0;
}

void dequeuer_stats_dtor (struct dequeuer_stats* st)
{
  (void)st;
  printf ("about to clear mondemand dequeuer\n");
  mondemand_dequeuer_stats_free();
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


static void log_rates(log_mask_t level, const char* file, int line,
                      double bps, double pps, const char* notes)
{
  if ( bps > 1000000. )
    {
      log_msg(level, file, line, " %g mbps, %g pps%s.\n", bps / 1000000., pps, notes);
    }
  else if ( bps > 1000. )
    {
      log_msg(level, file, line, " %g kbps, %g pps%s.\n", bps / 1000., pps, notes);
    }
  else
    {
      log_msg(level, file, line, " %g bps, %g pps%s.\n", bps, pps, notes);
    }
}

void enqueuer_stats_rotate(struct enqueuer_stats* st)
{
  double rbps, rpps;

  time_t now = time(NULL);
  time_t uptime;

  if ( st->socket_errors_since_last_rotate )
    {
      LOG_ER("*** %lld packets had socket errors in this journal ***\n",
             st->socket_errors_since_last_rotate);
    }

  LOG_INF("Socket read errors since last rotate: %lld\n",
          st->socket_errors_since_last_rotate);

  LOG_INF("Events read since last rotate:\n");
  LOG_INF(" %lld bytes, %lld packets received.\n",
          st->bytes_received_since_last_rotate,
          st->packets_received_since_last_rotate);
  uptime = now - st->last_rotate;
  rbps = (8. * (double)st->bytes_received_since_last_rotate) / (double)uptime;
  rpps = ((double)st->packets_received_since_last_rotate) / (double)uptime;
  log_rates(LOG_INFO,__FILE__,__LINE__,rbps,rpps," received");

  LOG_INF("Enqueuer stats summary v2:\t%d\t%lld\t%lld\t%lld\t%lld\t%lld\t%d\n",
          now, st->socket_errors_since_last_rotate,
          st->bytes_received_total, st->bytes_received_since_last_rotate,
          st->packets_received_total, st->packets_received_since_last_rotate,
          uptime);

  mondemand_enqueuer_stats(st,now);

  st->socket_errors_since_last_rotate = 0LL;
  st->bytes_received_since_last_rotate = 0LL;
  st->packets_received_since_last_rotate = 0LL;
  st->last_rotate = now;
}

void enqueuer_stats_report(struct enqueuer_stats* st)
{
  double rbps, rpps;
  char startbfr[100];
  char nowbfr[100];

  struct tm tm_st;

  time_t now = time(NULL);
  time_t uptime;

  if ( my_strftime(startbfr, sizeof(startbfr),
                   "%c", localtime_r(&st->start_time, &tm_st)) == 0 )
    {
      LOG_ER("strftime failure");
    }

  if ( my_strftime(nowbfr, sizeof(nowbfr),
                   "%c", localtime_r(&now, &tm_st)) == 0 )
    {
      LOG_ER("strftime failure");
    }

  uptime = now - st->start_time;

  rbps = (8. * (double)st->bytes_received_total) / (double)uptime;
  rpps = ((double)st->packets_received_total) / (double)uptime;

  LOG_INF("Total network traffic received:\n");
  LOG_INF(" %lld bytes, %lld packets.\n",
          st->bytes_received_total,
          st->packets_received_total);
  log_rates(LOG_INFO,__FILE__,__LINE__,rbps,rpps," received");
}

void enqueuer_stats_flush(void) {
  mondemand_enqueuer_flush ();
}

void dequeuer_stats_rotate(struct dequeuer_stats* st)
{
  double wbps, wpps;

  time_t now = time(NULL);
  time_t uptime;

  if ( st->loss_since_last_rotate )
    {
      LOG_ER("*** %lld packets lost in this journal ***\n",
             st->loss_since_last_rotate);
    }

  LOG_INF("Events written since last rotate:\n");
  LOG_INF(" %lld bytes, %lld packets in this journal.\n",
          st->bytes_written_since_last_rotate,
          st->packets_written_since_last_rotate);
  uptime = now - st->last_rotate;
  wbps = (8. * (double)st->bytes_written_since_last_rotate) / (double)uptime;
  wpps = ((double)st->packets_written_since_last_rotate) / (double)uptime;
  log_rates(LOG_INFO,__FILE__,__LINE__,wbps,wpps," written");

  LOG_INF("Highest queue utilization since last rotate: %d packets.\n",
          st->hiq_since_last_rotate);
  LOG_INF("Highest burst since last rotate: %lli packets, %lli bytes.\n",
          st->packets_written_in_burst_since_last_rotate,
          st->bytes_written_in_burst_since_last_rotate);

  if (st->rotation_type == LJ_RT_EVENT) {
    LOG_INF("Command::Rotate from IP %s traversed the queue in %ld ms\n",
            header_sender_ip_formatted(st->latest_rotate_header),
            time_in_milliseconds()-header_receipt_time(st->latest_rotate_header));
  }

  LOG_INF("Dequeuer stats summary v2:\t%d\t%lld\t%lld\t%lld\t%lld\t%lld\t%lld\t%lld\t%d\t%d\t%d\t%d\t%lld\t%lld\t%d\n",
          now, st->loss_since_last_rotate,
          st->bytes_written_total, st->bytes_written_since_last_rotate,
          st->packets_written_total, st->packets_written_since_last_rotate,
          st->bytes_written_in_burst, st->packets_written_in_burst, st->hiq,
          st->hiq_start, st->hiq_last, st->hiq_since_last_rotate,
          st->bytes_written_in_burst_since_last_rotate,
          st->packets_written_in_burst_since_last_rotate, uptime);

  mondemand_dequeuer_stats(st,now);

  st->bytes_written_since_last_rotate = 0LL;
  st->packets_written_since_last_rotate = 0LL;
  st->hiq_since_last_rotate = 0;
  st->packets_written_in_burst_since_last_rotate = 0LL;
  st->bytes_written_in_burst_since_last_rotate = 0LL;
  st->loss_since_last_rotate = 0LL;
  st->last_rotate = now;
  st->rotation_type = LJ_RT_NONE;
}

void dequeuer_stats_report(struct dequeuer_stats* st)
{
  double wbps, wpps;
  char startbfr[100];
  char nowbfr[100];

  struct tm tm_st;

  time_t now = time(NULL);
  time_t uptime;

  if ( my_strftime(startbfr, sizeof(startbfr),
                   "%c", localtime_r(&st->start_time, &tm_st)) == 0 )
    {
      LOG_ER("strftime failure");
    }

  if ( my_strftime(nowbfr, sizeof(nowbfr),
                   "%c", localtime_r(&now, &tm_st)) == 0 )
    {
      LOG_ER("strftime failure");
    }

  uptime = now - st->start_time;

  wbps = (8. * (double)st->bytes_written_total) / (double)uptime;
  wpps = ((double)st->packets_written_total) / (double)uptime;

  LOG_INF("Total network traffic recorded:\n");
  LOG_INF(" %lld bytes, %lld packets.\n",
          st->bytes_written_total,
          st->packets_written_total);
  log_rates(LOG_INFO,__FILE__,__LINE__,wbps,wpps," written");
}

void dequeuer_stats_flush(void) {
  mondemand_dequeuer_flush ();
}


