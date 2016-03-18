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

#endif /* TIME_UTILS_DOT_H */
