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
