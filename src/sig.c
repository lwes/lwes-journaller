/*======================================================================*
 * Copyright (c) 2008, Yahoo! Inc. All rights reserved.                 *
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

#include "log.h"
#include "perror.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

volatile int gbl_done = 0;
volatile int gbl_rotate = 0;
volatile int gbl_rotate_log = 0;

static void terminate_signal_handler(int signo)
{
  gbl_done = signo;
}

static void rotate_signal_handler(int signo)
{
  (void)signo; /* appease -Wall -Werror */
  gbl_rotate = 1;
}

static void rotate_log_signal_handler(int signo)
{
  (void)signo; /* appease -Wall -Werror */
  gbl_rotate_log = 1;
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
  LOG_PROG("Installing rotate signal handlers.\n");

  /* This one triggers a journal rotate. */
  install(SIGHUP, rotate_signal_handler);

  /* This one triggers a log rotate. */
  install(SIGUSR1, rotate_log_signal_handler);
}
