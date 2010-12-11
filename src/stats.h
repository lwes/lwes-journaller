/*======================================================================*
 * Copyright (c) 2008, Yahoo! Inc. All rights reserved.                 *
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

#include <time.h>
#include "header.h"

struct enqueuer_stats {
  long long socket_errors_since_last_rotate;
  
  long long bytes_received_total;
  long long bytes_received_since_last_rotate;

  long long packets_received_total;
  long long packets_received_since_last_rotate;

  time_t start_time;
  time_t last_rotate;
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
};

unsigned long long time_in_milliseconds (void);

int enqueuer_stats_ctor (struct enqueuer_stats* this_stats);
void stats_enqueuer_rotate (struct enqueuer_stats* this_stats);
void enqueuer_stats_record_socket_error (struct enqueuer_stats* this_stats);
void enqueuer_stats_record_datagram (struct enqueuer_stats* this_stats, int bytes);
void enqueuer_stats_rotate (struct enqueuer_stats* this_stats);
void enqueuer_stats_report (struct enqueuer_stats* this_stats);

int dequeuer_stats_ctor (struct dequeuer_stats* this_stats);
void dequeuer_stats_record (struct dequeuer_stats* this_stats, int bytes, int pending);
void dequeuer_stats_record_loss (struct dequeuer_stats* this_stats);
void dequeuer_stats_rotate (struct dequeuer_stats* this_stats);
void dequeuer_stats_report (struct dequeuer_stats* this_stats);

#endif /* STATS_DOT_H */
