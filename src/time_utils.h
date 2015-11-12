#ifndef TIME_UTILS_DOT_H
#define TIME_UTILS_DOT_H

#include <sys/time.h>

void
time_to_next_round_interval (const struct timeval now,
                             const unsigned int interval,
                             struct timeval *next);

char *
micro_time (const struct timeval now);

#endif /* TIME_UTILS_DOT_H */
