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

#ifndef STATS_DOT_H
#define STATS_DOT_H

#include <time.h>
#include "header.h"

struct stats {
  long long loss;

  long long bytes_total;
  long long bytes_since_last_rotate;

  long long packets_total;
  long long packets_since_last_rotate;

  long long bytes_in_burst;
  long long packets_in_burst;

  int hiq;			/* Peak packet count in queue. */

  time_t hiq_start;		/* When this burst started. */
  time_t hiq_last;		/* When the previous burst started. */

  int hiq_since_last_rotate;	/* Highest high water mark since last rotate. */

  long long bytes_in_burst_since_last_rotate;
  long long packets_in_burst_since_last_rotate;

  time_t start_time;
  time_t last_rotate;
  
  struct event_header latest_rotate_header ; /* Of the Command::Rotate that was acted on */

	long hurryup_discards[3] ;
};

int stats_ctor(struct stats* this_stats);
void stats_record(struct stats* this_stats, int bytes, int pending);
void stats_rotate(struct stats* this_stats);
void stats_record_loss(struct stats* this_stats);
void stats_report(struct stats* this_stats);
extern struct stats st ;

#endif /* STATS_DOT_H */
