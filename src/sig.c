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
volatile sig_atomic_t gbl_rotate_main_log = 0;
volatile sig_atomic_t gbl_rotate_enqueue_log = 0;
volatile sig_atomic_t gbl_rotate_dequeue_log = 0;

static void terminate_signal_handler(int signo)
{
  (void)signo; /* appease -Wall -Werror */
  CAS_ON(gbl_done);
}

static void rotate_signal_handler(int signo)
{
  (void)signo; /* appease -Wall -Werror */
  CAS_ON(gbl_rotate_enqueue);
  CAS_ON(gbl_rotate_dequeue);
}

static void rotate_main_log_signal_handler(int signo)
{
  (void)signo; /* appease -Wall -Werror */
  CAS_ON(gbl_rotate_main_log);
}

static void rotate_enqueue_log_signal_handler (int signo)
{
  (void)signo; /* appease -Wall -Werror */
  CAS_ON(gbl_rotate_enqueue_log);
}

static void rotate_dequeue_log_signal_handler (int signo)
{
  (void)signo; /* appease -Wall -Werror */
  CAS_ON(gbl_rotate_dequeue_log);
}

static void install(FILE *log, int signo, void (*handler)(int signo))
{
  struct sigaction sigact;

  memset(&sigact, 0, sizeof(sigact));

  /* a null handler means to ignore this signal */
  if (handler == NULL)
    {
      sigact.sa_handler = SIG_IGN;
    }
  else
    {
      sigact.sa_handler = handler;
    }

  sigemptyset(&sigact.sa_mask);
  sigact.sa_flags = 0;

  if ( sigaction(signo, &sigact, NULL) < 0 )
    {
      PERROR(log,"sigaction");
      exit(EXIT_FAILURE);
    }
}

void install_termination_signal_handlers(FILE *log)
{
  /* Any of these signals shuts us down. */
  LOG_PROG(log, "Installing termination signal handlers.\n");

  install(log, SIGINT, terminate_signal_handler);
  install(log, SIGQUIT, terminate_signal_handler);
  install(log, SIGTERM, terminate_signal_handler);
  install(log, SIGPIPE, NULL);
}

void install_rotate_signal_handlers(FILE *log)
{
  LOG_PROG(log, "Installing rotate signal handlers.\n");
  /* This one triggers a journal rotate. */
  install(log, SIGHUP, rotate_signal_handler);
}

/* log rotation is strange, what's happening is that in order to work
 * correctly with thread, process, and serial mode we need to jump
 * through a few hoops with signal handling.
 *
 * In the process model the main process catches SIGUSR1 and uses
 * that to both rotate logs and send signals to the child processes.
 * For the child processes I use 2 different signals since they
 * end up tied to two separate globals.  That way the same code
 * can be used in the threaded and process models at a certain level.
 */
void install_log_rotate_signal_handlers(FILE *log, int is_main, int sig)
{
  LOG_PROG(log, "Installing rotate signal handlers using %d.\n", sig);
  switch (sig)
    {
      case SIGUSR1:
        if (is_main) {
          install(log, sig, rotate_main_log_signal_handler);
        } else {
          install(log, sig, rotate_enqueue_log_signal_handler);
        }
        break;
      case SIGUSR2:
        install(log, sig, rotate_dequeue_log_signal_handler);
        break;

      default:
        break;
    }
}

void install_interval_rotate_handlers (FILE *log, int should_install_handler)
{
  if (arg_journal_rotate_interval) {
    struct itimerval timer;
    struct timeval now;
    struct timeval next;
    if (-1 == gettimeofday (&now, 0))
      {
        PERROR(log, "gettimeofday");
      }
    if (should_install_handler != 0 )
      {
        install(log, SIGALRM, rotate_signal_handler);
      }

    time_to_next_round_interval (now, arg_journal_rotate_interval, &next);
    /* Configure the timer to expire on the next round interval sec... */
    timer.it_value.tv_sec = next.tv_sec;
    timer.it_value.tv_usec = next.tv_usec;
    /* ... and every interval seconds after that. */
    timer.it_interval.tv_sec = arg_journal_rotate_interval;
    timer.it_interval.tv_usec = 0;
    LOG_INF(log, "rotating every %d seconds\n", arg_journal_rotate_interval);
    /* set an interval timer to alarm for rotation */
    if (setitimer (ITIMER_REAL, &timer, NULL) < 0)
      {
        PERROR(log,"setitimer");
      }
  }
}
