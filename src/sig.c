/*======================================================================*
 * Copyright (C) 2008 Light Weight Event System                         *
 * All rights reserved.                                                 *
 *                                                                      *
 * This program is free software; you can redistribute it and/or modify *
 * it under the terms of the GNU General Public License as published by *
 * the Free Software Foundation; either version 2 of the License, or    *
 * (at your option) any later version.                                  *
 *                                                                      *
 * This program is distributed in the hope that it will be useful,      *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 * GNU General Public License for more details.                         *
 *                                                                      *
 * You should have received a copy of the GNU General Public License    *
 * along with this program; if not, write to the Free Software          *
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,                   *
 * Boston, MA 02110-1301 USA.                                           *
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
volatile PANIC_MODE journaller_panic_mode = PANIC_NOT ;

static void terminate_signal_handler(int signo)
{
  gbl_done = signo;
}

static void rotate_signal_handler(int signo)
{
  (void)signo; /* appease -Wall -Werror */
  gbl_rotate = 1;
}

static void install(int signo, void (*handler)(int signo))
{
  struct sigaction sigact;

  memset(&sigact, 0, sizeof(sigact));

  sigact.sa_handler = handler;

  sigemptyset(&sigact.sa_mask);
  sigact.sa_flags = 0;

  if ( sigaction(signo, &sigact, NULL) < 0 ) {
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

  if ( sigaction(SIGPIPE, &sigact, NULL) < 0 ) {
    PERROR("sigaction");
    exit(EXIT_FAILURE);
  }
}

void install_rotate_signal_handlers()
{
  /* This one triggers a log rotate. */
  LOG_PROG("Installing rotate signal handlers.\n");

  install(SIGHUP, rotate_signal_handler);
}
