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

#include "config.h"

#include "sig.h"
#include "opt.h"
#include "time_utils.h"
#include "log.h"
#include "perror.h"

#include <stdio.h>
#include <stdlib.h>

volatile sig_atomic_t gbl_done = 0;
volatile sig_atomic_t gbl_rotate_enqueue = 0;
volatile sig_atomic_t gbl_rotate_dequeue = 0;
volatile sig_atomic_t gbl_rotate_log = 0;

static void terminate_signal_handler(int signo)
{
  (void)signo; /* appease -Wall -Werror */
  __sync_bool_compare_and_swap(&gbl_done,0,1);
}

static void rotate_signal_handler(int signo)
{
  (void)signo; /* appease -Wall -Werror */
  __sync_bool_compare_and_swap(&gbl_rotate_enqueue,0,1);
  __sync_bool_compare_and_swap(&gbl_rotate_dequeue,0,1);
}

static void rotate_log_signal_handler(int signo)
{
  (void)signo; /* appease -Wall -Werror */
  __sync_bool_compare_and_swap(&gbl_rotate_log,0,1);
}

static void install(int signo, void (*handler)(int signo))
{
  struct sigaction sigact;

  memset(&sigact, 0, sizeof(sigact));

  sigact.sa_handler = handler;

  sigemptyset(&sigact.sa_mask);
  sigact.sa_flags = 0;

  if ( sigaction(signo, &sigact, NULL) < 0 )
    {
      PERROR("sigaction");
      exit(EXIT_FAILURE);
    }
}

void install_signal_handlers()
{
  struct sigaction sigact;

  /* Any of these signals shuts us down. */
  LOG_PROG("Installing termination signal handlers.\n");

  install(SIGINT, terminate_signal_handler);
  install(SIGQUIT, terminate_signal_handler);
  install(SIGTERM, terminate_signal_handler);

  memset(&sigact, 0, sizeof(sigact));
  sigact.sa_handler = SIG_IGN;

  sigemptyset(&sigact.sa_mask);
  sigact.sa_flags = 0;

  if ( sigaction(SIGPIPE, &sigact, NULL) < 0 )
    {
      PERROR("sigaction");
      exit(EXIT_FAILURE);
    }
}

void install_rotate_signal_handlers()
{
  /* This one triggers a journal rotate. */
  install(SIGHUP, rotate_signal_handler);

  /* This one triggers a log rotate. */
  install(SIGUSR1, rotate_log_signal_handler);
}

void install_interval_rotate_handlers (int should_install_handler)
{
  if (arg_journal_rotate_interval) {
    struct itimerval timer;
    struct timeval now;
    struct timeval next;
    if (-1 == gettimeofday (&now, 0))
      {
        PERROR("gettimeofday");
      }
    if (should_install_handler != 0 )
      {
        install(SIGALRM, rotate_signal_handler);
      }

    time_to_next_round_interval (now, arg_journal_rotate_interval, &next);
    /* Configure the timer to expire on the next round interval sec... */
    timer.it_value.tv_sec = next.tv_sec;
    timer.it_value.tv_usec = next.tv_usec;
    /* ... and every interval seconds after that. */
    timer.it_interval.tv_sec = arg_journal_rotate_interval;
    timer.it_interval.tv_usec = 0;
    LOG_INF("rotating every %d seconds\n", arg_journal_rotate_interval);
    /* set an interval timer to alarm for rotation */
    if (setitimer (ITIMER_REAL, &timer, NULL) < 0)
      {
        PERROR("setitimer");
      }
  }
}
