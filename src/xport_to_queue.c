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

#include "xport_to_queue.h"

#include "header.h"
#include "opt.h"
#include "perror.h"
#include "queue.h"
#include "sig.h"
#include "xport.h"
#include "stats.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void skd();

void* xport_to_queue(void* arg)
{
  struct xport xpt;
  struct queue que;

  unsigned char* buf = 0;
  size_t bufsiz;

  int read_errors = 0;

  install_signal_handlers();

  if ( arg_rt )
    skd();

  if ( (queue_factory(&que) < 0) || (que.vtbl->open(&que, O_WRONLY) < 0) ) {
    LOG_ER("Failed to create or open queue object.\n");
    exit(EXIT_FAILURE);
  }

  /* Can we drop root here? */

  if ( (xport_factory(&xpt) < 0) || (xpt.vtbl->open(&xpt, O_RDONLY) < 0) ) {
    LOG_ER("Failed to create xport object.\n");
    exit(EXIT_FAILURE);
  }

  buf = (unsigned char*)que.vtbl->alloc(&que, &bufsiz);
  if ( 0 == buf ) {
    LOG_ER("unable to allocate %d bytes for message buffer.\n", bufsiz);
    exit(EXIT_FAILURE);
  }
  memset(buf, 0, HEADER_LENGTH); /* Clear the header portion of the message. */

  /* Read a packet from the transport, write it to the queue. */
  while ( ! gbl_done ) {
    int xpt_read_ret;
    int que_write_ret;

    unsigned long addr;
    short port;

    switch ( journaller_panic_mode ) {
      case PANIC_STARTUP: // panic! system queue is max'd out (receiver live-lock?)
        // close the socket
        xpt.vtbl->close(&xpt) ;
        journaller_panic_mode = PANIC_IN_EFFECT ;
      case PANIC_IN_EFFECT: // waiting for system queue to empty
        continue ; // socket is closed, so we can't read it anyway
      case PANIC_SHUTDOWN: // panic is over
        // reopen the socket
        LOG_PROG("PANIC shutdown - reopening the multicast socket\n") ;
        if ( (xport_factory(&xpt) < 0) || (xpt.vtbl->open(&xpt, O_RDONLY) < 0) ) {
          LOG_ER("Failed to reopen xport object after PANIC.\n");
          exit(EXIT_FAILURE);
        }
        LOG_PROG("PANIC shutdown - opened the multicast socket\n") ;
        journaller_panic_mode = PANIC_NOT ;
        break ;
      case PANIC_HURRYUP: // hurry-up mode
        break ;
      case PANIC_NOT:
        break ;
    }

    if ( (xpt_read_ret = xpt.vtbl->read(&xpt,
            buf + HEADER_LENGTH,
            bufsiz - HEADER_LENGTH,
            &addr, &port)) < 0 ) {
      ++read_errors;
      continue;
    }

    /* Return info about packet read. */
    LOG_PROG("Read %d bytes\n", xpt_read_ret);
    LOG_PROG("From %08lx:%04x.\n", addr, port&0xffff);
    if ( read_errors ) {
      LOG_PROG("read_errors == %d\n", read_errors);
    }
    read_errors = 0;

    // if hurry-up, discard non-revenue-bearing events
    if ( (journaller_panic_mode == PANIC_HURRYUP) && non_revenue_bearing(buf) )
      continue ;

    header_add(buf, xpt_read_ret, addr, port);

    if ( (que_write_ret = que.vtbl->write(&que,
            buf,
            xpt_read_ret + HEADER_LENGTH)) < 0 ) {
      LOG_ER("Queue write error attempting to write %d bytes.\n",
          xpt_read_ret + HEADER_LENGTH);
      continue;
    } else {
      LOG_PROG("Queue write of %d bytes.\n",
          xpt_read_ret + HEADER_LENGTH);
    }
  }
  que.vtbl->dealloc(&que, buf);

  xpt.vtbl->destructor(&xpt);
  que.vtbl->destructor(&que);

  return 0;
}

#if HAVE_SCHED_H
#include <sched.h>

static void skd()
{
  struct sched_param sp;

  sp.sched_priority = sched_get_priority_max(SCHED_FIFO);
  if ( sched_setscheduler(0, SCHED_FIFO, &sp) ) {
    PERROR("sched_setscheduler");
    LOG_WARN("Increasing thread priority failed"
        ", will run with standard priorities");
  } else {
    LOG_WARN("Running with FIFO priority");
  }
}
#else
static void skd()
{
  LOG_WARN("No real-time scheduler support on this platform.\n");
}
#endif