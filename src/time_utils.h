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

#ifndef TIME_UTILS_DOT_H
#define TIME_UTILS_DOT_H

#include <sys/time.h>

void
time_to_next_round_interval (const struct timeval now,
                             const unsigned int interval,
                             struct timeval *next);

char *
micro_time (const struct timeval now);

void
micro_now (struct timeval *now);

unsigned long long
micro_timediff (struct timeval *start, struct timeval *end);

unsigned long long
millis_timestamp (struct timeval *val);
unsigned long long
micro_timestamp (struct timeval *val);

unsigned long long
millis_now (void);

#endif /* TIME_UTILS_DOT_H */
