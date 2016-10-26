/*======================================================================*
 * Copyright (c) 2010, OpenX Inc. All rights reserved.                  *
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

#ifndef LOG_MONDEMAND_H
#define LOG_MONDEMAND_H

#include "config.h"
#include "stats.h"

#ifdef HAVE_MONDEMAND
#include <mondemand.h>
#endif

void mondemand_enqueuer_stats_init (void);
void mondemand_dequeuer_stats_init (void);
void mondemand_enqueuer_stats (const struct enqueuer_stats* this_stats, time_t now);
void mondemand_dequeuer_stats (const struct dequeuer_stats* this_stats, time_t now);
void mondemand_enqueuer_flush (void);
void mondemand_dequeuer_flush (void);
void mondemand_enqueuer_stats_free (void);
void mondemand_dequeuer_stats_free (void);

#endif
