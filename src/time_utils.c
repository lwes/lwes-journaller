/*======================================================================*
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
#include "time_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

void
time_to_next_round_interval (const struct timeval now,
                             const unsigned int interval,
                             struct timeval *next)
{
  /* this is the current second in the current day */
  time_t second_in_day = now.tv_sec - now.tv_sec / 86400 * 86400;

  /* this determine's the next round interval by first dividing by the interval
   * then multiplying which gives us the current interval start, then adding
   * the interval which gives us the next interval start */
  time_t next_interval = second_in_day / interval * interval + interval;

  /* do a similar calculation to determine approximate micros until next round
   * interval */
  unsigned long long now_millis = (((long long)now.tv_sec) * 1000LL)
                                   + (long long)(now.tv_usec/1000LL);

  /* next we'll subtract to get seconds to next interval */
  time_t seconds_to_next_interval = next_interval - second_in_day;

  /* calculate micros (but in milliseconds) */
  unsigned long long micros_to_next_second = 0;

  /* if we hit an exact second there are no millis to wait, but if we did
   * not we need to calculate the millis until the next second, and subtract
   * a second as the calculation above assumed round seconds
   */
  if (now_millis != 0) {
    micros_to_next_second =
      (((now_millis / 1000LL) * 1000LL + 1000LL) - now_millis) * 1000LL;
    seconds_to_next_interval -= 1;
  }

  next->tv_sec = seconds_to_next_interval;
  next->tv_usec = micros_to_next_second;
}

char *
micro_time (const struct timeval now)
{
  time_t nowtime;
  struct tm *nowtm;
  char tmbuf[64], out[64];

  nowtime = now.tv_sec;
  nowtm = gmtime(&nowtime);
  strftime(tmbuf, sizeof tmbuf, "%Y-%m-%d %H:%M:%S", nowtm);
  snprintf(out, sizeof out, "%s.%06d", tmbuf, (int)now.tv_usec);

  return strdup (out);
}

void
micro_now (struct timeval *now)
{
  gettimeofday(now, 0);
}

unsigned long long
millis_timestamp (struct timeval *val)
{
  return (long long)(val->tv_sec) * 1000LL + (long long)(val->tv_usec) / 1000LL;
}

unsigned long long
micro_timestamp (struct timeval *val)
{
  return (long long)(val->tv_sec) * 1000000LL + (long long)(val->tv_usec);
}

unsigned long long
micro_timediff (struct timeval *start, struct timeval *end)
{
  long long computed =
    (long long)((long long)end->tv_sec - (long long)start->tv_sec) * 1000000LL
      + (long long)((long long)end->tv_usec - (long long)start->tv_usec);
  if (computed < 0)
    {
      computed = 0LL;
    }
  return (unsigned long long)computed;
}

unsigned long long
millis_now (void)
{
  struct timeval t;
  micro_now(&t);
  return millis_timestamp (&t);
}
