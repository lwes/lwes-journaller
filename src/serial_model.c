/*======================================================================*
 * Copyright (c) 2010, OpenX Inc. All rights reserved.                  *
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

#include "journal.h"
#include "log.h"
#include "opt.h"
#include "serial_model.h"
#include "sig.h"
#include "stats.h"
#include "xport.h"

#include <fcntl.h>
#include <lwes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFLEN               (65535)
#define SERIAL_DEPTH_COMMAND ("\026Internal::Queue::Depth")

struct xport             xpt;
unsigned char            buf[BUFLEN];
int                      buflen;
struct journal           jrn;
struct enqueuer_stats    est;
struct dequeuer_stats    dst;
unsigned long long       tm;
struct lwes_emitter*     emitter   = NULL;
/* these are used for doing a depth test, which every depth_dtm milliseconds
 * sends an event SERIAL_DEPTH_COMMAND to this journaller, and later takes
 * that and uses it to estimate how many events were in the operating system
 * queue
 */
unsigned long long       depth_tm  = 0;
unsigned long long       depth_dtm = 10000;
/* count is the total number of events received by this journaller */
unsigned long long       count     = 0;
/* pending is the number of events received between sending and receiving
 * a queue depth test event.  It represents the number of events in 
 * the recv buffer of the operating system.
 */
unsigned long long       pending   = 0;

static void serial_open_journal(void);

static void serial_ctor(void)
{
  install_signal_handlers();
  install_rotate_signal_handlers();
  install_interval_rotate_handlers(1);

  depth_dtm = arg_queue_test_interval;

  if ( enqueuer_stats_ctor (&est) < 0 )
    {
      LOG_ER("Failed to create initialize enqueuer stats.\n");
      exit(EXIT_FAILURE);
    }

  if ( dequeuer_stats_ctor (&dst) < 0 )
    {
      LOG_ER("Failed to create initialize dequeuer stats.\n");
      exit(EXIT_FAILURE);
    }

  memset(buf, 0, BUFLEN);           /* Clear the message. */

  if ( (xport_factory(&xpt) < 0) || (xpt.vtbl->open(&xpt, O_RDONLY) < 0) )
    {
      LOG_ER("Failed to create xport object.\n");
      exit(EXIT_FAILURE);
    }

  if (arg_njournalls != 1)
    {
      LOG_ER("Expected 1 journal name pattern, but found %d\n", arg_njournalls);
      exit(EXIT_FAILURE);
    }

  /* Create journal object. */
  if (journal_factory(&jrn, arg_journalls[0]) < 0)
    {
      LOG_ER("Failed to create journal object for \"%s\".\n", arg_journalls[0]);
      exit(EXIT_FAILURE);
    }

  serial_open_journal();

  emitter = lwes_emitter_create( (LWES_CONST_SHORT_STRING) arg_ip,
                                 (LWES_CONST_SHORT_STRING) NULL, //arg_interface,
                                 (LWES_U_INT_32) arg_port, 0, 60 );
}

static void serial_open_journal(void)
{
  if (jrn.vtbl->open(&jrn, O_WRONLY) < 0)
    {
      LOG_ER("Failed to open the journal \"%s\".\n", arg_journalls[0]);
      exit(EXIT_FAILURE);
    }
}

static void serial_close_journal(int is_rotate_event)
{
  if (gbl_done || gbl_rotate_enqueue || is_rotate_event)
    {
      enqueuer_stats_rotate(&est);
      if (!gbl_done)
        {
          enqueuer_stats_flush();
        }
      __sync_bool_compare_and_swap(&gbl_rotate_enqueue,1,0);
    }

  if (gbl_done || gbl_rotate_dequeue || is_rotate_event)
    {
      dequeuer_stats_rotate(&dst);
      if (!gbl_done)
        {
          dequeuer_stats_flush();
        }
      __sync_bool_compare_and_swap(&gbl_rotate_dequeue,1,0);
    }

  if (jrn.vtbl->close(&jrn) < 0) {
    LOG_ER("Can't close journal  \"%s\".\n", arg_journalls[0]);
    exit(EXIT_FAILURE);
  }
}

static void serial_rotate(int is_rotate_event)
{
  serial_close_journal(is_rotate_event);
  serial_open_journal();
}

static int serial_read(void)
{
  unsigned long addr;
  short port;

  /* 0 out part of the the event name so if we get a rotate event the program
   * will not continually rotate */
  memset(buf, 0, HEADER_LENGTH+20);

  int xpt_read_ret =
      xpt.vtbl->read(&xpt, buf+HEADER_LENGTH, BUFLEN-HEADER_LENGTH, &addr, &port);

  if (xpt_read_ret >= 0)
    {
      tm     = time_in_milliseconds();
      buflen = xpt_read_ret + HEADER_LENGTH;
      enqueuer_stats_record_datagram(&est, buflen);
      header_add(buf, xpt_read_ret, tm, addr, port);
      ++count;
      return 0;
    }
  else if (xpt_read_ret == XPORT_INTR)
    {
      return XPORT_INTR; /* return something special if interrupted
                            (we might be rotating via a signal or
                            shutting down */
    }
  else
    {
      enqueuer_stats_record_socket_error(&est);
      return -1;
    }
}

static int serial_handle_depth_test()
{
  /* check for the serial depth event */
  if (toknam_eq((unsigned char *)buf + HEADER_LENGTH,
                (unsigned char *)SERIAL_DEPTH_COMMAND))
    {
      struct lwes_event_deserialize_tmp event_tmp;
      int                               bytes_read;
      struct lwes_event*                event = lwes_event_create_no_name(NULL);

      /* don't include these internal events in our stats */
      enqueuer_stats_erase_datagram(&est, buflen);

      /* we should have one, so deserialize it */
      bytes_read = lwes_event_from_bytes(event,
                                         (LWES_BYTE_P)&buf[HEADER_LENGTH],
                                         buflen-HEADER_LENGTH,
                                         0, &event_tmp);
      if (bytes_read != buflen-HEADER_LENGTH)
        {
          LOG_ER("Only able to read %d bytes; expected %d\n", bytes_read, buflen);
        }
      else
        {
          /* get the previous count */
          LWES_U_INT_64 previous_count;
          lwes_event_get_U_INT_64(event, "count", &previous_count);
          /* determine the difference and remove one for this special event */
          pending  = count - previous_count - 1;
          LOG_INF("Depth test reports a buffer length of %lld events.\n", pending);
        }

      lwes_event_destroy(event);
      return 1;
    }
  else
    {
      return 0;
    }
}

/* this code attempts to determine if there are any events in the operating
 * system queue by sending an event with the current received count
 * and later checking it
 */
static void serial_send_buffer_depth_test(void)
{
  struct lwes_event *event =
      lwes_event_create( (struct lwes_event_type_db *) NULL,
                         (LWES_SHORT_STRING) SERIAL_DEPTH_COMMAND+1);
  if (event == NULL) return;
  if (lwes_event_set_U_INT_64(event,"count",count) < 0)
    {
      LOG_ER("Unable to add count to depth event");
    }
  else
    {
      lwes_emitter_emitto((char*) "127.0.0.1", NULL, arg_port, emitter, event);
    }
  lwes_event_destroy(event);

  depth_tm = time_in_milliseconds();
}

static void serial_write(void)
{
  /* Write the packet out to the journal. */
  int jrn_write_ret = jrn.vtbl->write(&jrn, buf, buflen);

  if (jrn_write_ret != buflen)
    {
      LOG_ER("Journal write error -- attempted to write %d bytes, "
             "write returned %d.\n", buflen, jrn_write_ret);
      dequeuer_stats_record_loss(&dst);
    }

  dequeuer_stats_record(&dst, buflen, pending);
}

static void serial_dtor(void)
{
  serial_close_journal(0);
  lwes_emitter_destroy(emitter);
  xpt.vtbl->destructor(&xpt);
  jrn.vtbl->destructor(&jrn);
  enqueuer_stats_dtor(&est);
  dequeuer_stats_dtor(&dst);
}

void serial_model(void)
{
  serial_ctor();

  do {
    int is_rotate_event = 0;
    int read_ret = serial_read();
    /* -1 is an error we don't deal with, so just skip out of the loop */
    if (read_ret == -1)             continue;
    /* depth tests are not written to the journal */
    if (serial_handle_depth_test()) continue;
    /* XPORT_INTR from read means we were interrupted and should not
     * write, so write when we are not interrupted, this is for backward
     * compatibility when we didn't do rotation signals correctly here
     */
    if (read_ret != XPORT_INTR ) serial_write();
    /* check for rotation event, or signal's and rotate if necessary */
    if (header_is_rotate(buf)) {
      memcpy(&dst.latest_rotate_header, buf, HEADER_LENGTH) ;
      dst.rotation_type = LJ_RT_EVENT;
      is_rotate_event = 1;
    }
    if (is_rotate_event || gbl_rotate_dequeue || gbl_rotate_enqueue) {
      serial_rotate(is_rotate_event);
      is_rotate_event = 0;
    }
    /* maybe send depth test */
    if (tm >= depth_tm + depth_dtm) serial_send_buffer_depth_test();
  }
  while (!gbl_done);

  serial_dtor();
}
