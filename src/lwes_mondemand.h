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

#include "stats.h"

void md_enqueuer_create (struct enqueuer_stats *stats);
void md_enqueuer_stats (const struct enqueuer_stats* stats);
void md_enqueuer_flush (const struct enqueuer_stats* stats);
void md_enqueuer_destroy(struct enqueuer_stats* stats);

void md_dequeuer_create (struct dequeuer_stats *stats);
void md_dequeuer_stats (const struct dequeuer_stats* stats);
void md_dequeuer_flush (const struct dequeuer_stats* stats);
void md_dequeuer_destroy (struct dequeuer_stats* stats);

#endif /* LOG_MONDEMAND_H */
