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

#include "stats.h"

#include "header.h"
#include "log.h"
#include "opt.h"
#include "sig.h"
#include "marshal.h"

#include <string.h>
#include <time.h>

/* To rid us of gcc warnings, see the strftime(3) man page on Linux
   for more info. */

static size_t my_strftime(char* s, size_t max, const char* fmt,
    const struct tm* tm)
{
  return strftime(s, max, fmt, tm);
}

int stats_ctor(struct stats* st)
{
  memset(st, 0, sizeof(*st));

  st->start_time = time(NULL);
  st->hiq_start = st->start_time;
  st->last_rotate = st->start_time;

  return 0;
}

void stats_record_loss(struct stats* st)
{
  st->loss += 1;
}

void stats_record(struct stats* st, int bytes, int pending)
{
  /* Collect some stats.  All byte counts represent the bytes used
     in the original packets, not with the headers applied. */

  st->bytes_since_last_rotate += bytes - HEADER_LENGTH;
  st->bytes_total += bytes - HEADER_LENGTH;

  ++st->packets_since_last_rotate;
  ++st->packets_total;

  if ( 0 == pending ) {	/* No pending packets in queue. */
    if ( st->hiq ) {
      /* We just grabbed that last packet of a burst. */
      st->bytes_in_burst_since_last_rotate = st->bytes_in_burst;
      st->packets_in_burst_since_last_rotate = st->packets_in_burst;

      st->hiq = 0;
      st->bytes_in_burst = 0;
      st->packets_in_burst = 0;
    }
  } else {			/* In a burst. */
    if ( 0 == st->hiq ) {	/* This is a new burst. */
      st->hiq_last = st->hiq_start;
      st->hiq_start = time(NULL);
    }
    if ( pending > st->hiq )	/* Possible peak. */
      st->hiq = pending;

    st->bytes_in_burst += bytes - HEADER_LENGTH;
    st->packets_in_burst += 1;
  }

  if ( st->hiq > st->hiq_since_last_rotate )
    st->hiq_since_last_rotate = st->hiq ;

  // we might need to hurry-up or panic, 
  //  so if we're not already doing that, 
  //  check if we need to start it now
  //  (8 and 9 are "inducing" hurry-up or panic)
  if ( journaller_panic_mode == 0 ) {
    if ( pending >= (arg_queue_max_cnt-10) ) {
      journaller_panic_mode = PANIC_STARTUP ; // induce panic - xport_to_queue will close socket and wait for journaller_panic_mode == 3
      LOG_INF("PANIC, queue is max'd out! hi-burst: %lli packets, %lli bytes.\n",
          st->packets_in_burst_since_last_rotate, st->bytes_in_burst_since_last_rotate);
    } else
      if ( (pending >= (arg_queue_max_cnt*arg_hurryup_at)/100) ) {
        journaller_panic_mode = PANIC_HURRYUP ; // hurry-up - xport_to_queue will discard non-revenue events
        LOG_INF("HURRYUP start, pending=%i >= %i, hi-burst: %lli packets, %lli bytes.\n",
            pending,(arg_queue_max_cnt*arg_hurryup_at)/100,
            st->packets_in_burst_since_last_rotate, st->bytes_in_burst_since_last_rotate);
      }
  }

}

void stats_rotate(struct stats* st)
{
  double bps, pps;

  time_t now = time(NULL);
  time_t uptime;
  unsigned char* cp = (unsigned char*)&st->latest_rotate_header.sender_ip; cp -= 2 ; //FIXME: wtf!

  if ( st->loss ) {
    LOG_ER("*** %lld packets lost in this journal ***\n",
        st->loss);
  }

  LOG_INF("Network traffic since last rotate:\n");
  LOG_INF("%lld bytes, %lld packets in this journal.\n",
      st->bytes_since_last_rotate,
      st->packets_since_last_rotate);

  uptime = now - st->last_rotate;

  bps = (8. * (double)st->bytes_since_last_rotate) / (double)uptime;
  pps = ((double)st->packets_since_last_rotate) / (double)uptime;

  if ( bps > 1000000. )
    LOG_INF(" %g mbps, %g pps.\n", bps / 1000000., pps);
  else if ( bps > 1000. )
    LOG_INF(" %g kbps, %g pps.\n", bps / 1000., pps);
  else
    LOG_INF(" %g bps, %g pps.\n", bps, pps);

  LOG_INF("Highest queue utilization since last rotate: %d packets.\n",
      st->hiq_since_last_rotate);
  LOG_INF("Highest burst since last rotate: %lli packets, %lli bytes.\n",
      st->packets_in_burst_since_last_rotate, st->bytes_in_burst_since_last_rotate);

  if ( st->hurryup_discards[0] )
    LOG_INF("%u CM::Serve events discarded in hurry-up mode\n", st->hurryup_discards[0]) ;
  if ( st->hurryup_discards[1] )
    LOG_INF("%u DM::Serve events discarded in hurry-up mode\n", st->hurryup_discards[1]) ;
  if ( st->hurryup_discards[2] )
    LOG_INF("%u SS::Serve events discarded in hurry-up mode\n", st->hurryup_discards[2]) ;

  //TODO: it's *impossible* to get the sender_ip via unmarshal_any_f'ng_thing!
  //LOG_INF("Command::Rotate received from IP %s", inet_ntoa(inaddr)) ;
  LOG_INF("Command::Rotate received from IP %u.%u.%u.%u\n", cp[3], cp[2], cp[1], cp[0]) ;

  st->bytes_since_last_rotate = 0LL;
  st->packets_since_last_rotate = 0LL;
  st->hiq_since_last_rotate = 0;
  st->packets_in_burst_since_last_rotate = st->bytes_in_burst_since_last_rotate = 0LL ;
  st->loss = 0LL;
  st->last_rotate = now;
  memset(&st->hurryup_discards, 0, sizeof(st->hurryup_discards)) ;
}

void stats_report(struct stats* st)
{
  double bps, pps;
  char startbfr[100];
  char nowbfr[100];

  struct tm tm_st;

  time_t now = time(NULL);
  time_t uptime;

  if ( my_strftime(startbfr, sizeof(startbfr),
        "%c", localtime_r(&st->start_time, &tm_st)) == 0 ) {
    LOG_ER("strftime failure");
  }

  if ( my_strftime(nowbfr, sizeof(nowbfr),
        "%c", localtime_r(&now, &tm_st)) == 0 ) {
    LOG_ER("strftime failure");
  }

  uptime = now - st->start_time;

  bps = (8. * (double)st->bytes_total) / (double)uptime;
  pps = ((double)st->packets_total) / (double)uptime;

  LOG_INF("Total network traffic:\n");
  LOG_INF(" %lld bytes, %lld packets.\n",
      st->bytes_total,
      st->packets_total);

  if ( bps > 1000000. )
    LOG_INF(" %g mbps, %g pps.\n", bps / 1000000., pps);
  else if ( bps > 1000. )
    LOG_INF(" %g kbps, %g pps.\n", bps / 1000., pps);
  else
    LOG_INF(" %g bps, %g pps.\n", bps, pps);
}
