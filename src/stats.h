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

#ifndef STATS_DOT_H
#define STATS_DOT_H

#include <stdio.h>
#include <time.h>
#include "header.h"

/* used to differentiate how we get a rotation signal, either via an event
 * or via a signal of some sort or we haven't yet for the current period.
 * For the moment, there isn't a way to clean set that a signal caused
 * the rotation, but as this only controls one printout, I'm just using
 * 2 states for now
 */
typedef enum {
    LJ_RT_NONE, LJ_RT_EVENT
} lj_rotation_t;

struct enqueuer_stats {
  long long socket_errors_since_last_rotate;

  long long bytes_received_total;
  long long bytes_received_since_last_rotate;

  long long packets_received_total;
  long long packets_received_since_last_rotate;

  time_t start_time;
  time_t last_rotate;
#ifdef HAVE_MONDEMAND
  struct mondemand_client *client;
#endif
};

struct dequeuer_stats {
  long long loss_since_last_rotate;

  long long bytes_written_total;
  long long bytes_written_since_last_rotate;

  long long packets_written_total;
  long long packets_written_since_last_rotate;

  long long bytes_written_in_burst;
  long long packets_written_in_burst;

  int hiq;                      /* Peak packet count in queue. */

  time_t hiq_start;             /* When this burst started. */
  time_t hiq_last;              /* When the previous burst started. */

  int hiq_since_last_rotate;    /* Highest high water mark since last rotate. */

  long long bytes_written_in_burst_since_last_rotate;
  long long packets_written_in_burst_since_last_rotate;

  time_t start_time;
  time_t last_rotate;

  char latest_rotate_header[HEADER_LENGTH] ; /* Of the Command::Rotate that was acted on */
  lj_rotation_t rotation_type;
#ifdef HAVE_MONDEMAND
  struct mondemand_client *client;
#endif
};

int enqueuer_stats_ctor (struct enqueuer_stats* stats);
void enqueuer_stats_dtor (struct enqueuer_stats* stats);
void enqueuer_stats_record_socket_error (struct enqueuer_stats* stats);
void enqueuer_stats_record_datagram (struct enqueuer_stats* stats, int bytes);
void enqueuer_stats_erase_datagram (struct enqueuer_stats* stats, int bytes);
void enqueuer_stats_rotate (struct enqueuer_stats* st, FILE *log);
void enqueuer_stats_report (struct enqueuer_stats* stats, FILE *log);
void enqueuer_stats_flush (struct enqueuer_stats* stats);

int dequeuer_stats_ctor (struct dequeuer_stats* stats);
void dequeuer_stats_record (struct dequeuer_stats* stats, int bytes, int pending);
void dequeuer_stats_record_loss (struct dequeuer_stats* stats);
void dequeuer_stats_rotate (struct dequeuer_stats* stats, FILE *log);
void dequeuer_stats_report (struct dequeuer_stats* stats, FILE *log);
void dequeuer_stats_flush (struct dequeuer_stats* stats);
void dequeuer_stats_dtor (struct dequeuer_stats* stats);

#endif /* STATS_DOT_H */
