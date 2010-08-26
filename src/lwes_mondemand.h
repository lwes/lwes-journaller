/*======================================================================*
 * Copyright (C) 2010 Light Weight Event System                         *
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
#ifndef LOG_MONDEMAND_H
#define LOG_MONDEMAND_H

#include "config.h"
#include "stats.h"
#include "log.h"

#ifdef HAVE_MONDEMAND
#include <mondemand.h>
#endif

void mondemand_enqueuer_stats (const struct enqueuer_stats* this_stats, time_t now);
void mondemand_dequeuer_stats (const struct dequeuer_stats* this_stats, time_t now);
void mondemand_log_msg (log_level_t level, const char *fname, int lineno, const char *buf);

#endif
