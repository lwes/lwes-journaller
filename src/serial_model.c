/*======================================================================*
 * Copyright (c) 2010, OpenX Inc. All rights reserved.                  *
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
unsigned long long       depth_tm  = 0;
const unsigned long long depth_dtm = 10000;
unsigned long long       count     = 0;
unsigned long long       pending   = 0;

static void serial_open_journal(void);

static void serial_ctor(void)
{
  install_signal_handlers();
  install_rotate_signal_handlers();
  
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

static void serial_close_journal(void)
{
  enqueuer_stats_rotate(&est);
  dequeuer_stats_rotate(&dst);
  stats_flush(); /* need to flush here, as this should
                    be it's own process so would have it's
                    own mondemand structure */

  if (jrn.vtbl->close(&jrn) < 0) {
    LOG_ER("Can't close journal  \"%s\".\n", arg_journalls[0]);
    exit(EXIT_FAILURE);
  }
}

static void serial_rotate(void)
{
  serial_close_journal();
  serial_open_journal();
  gbl_rotate = 0;
}

static int serial_read(void)
{
  unsigned long addr;
  short         port;
  int           xpt_read_ret =
      xpt.vtbl->read(&xpt, buf+HEADER_LENGTH, BUFLEN-HEADER_LENGTH, &addr, &port);

  if (xpt_read_ret < 0)
    {
      enqueuer_stats_record_socket_error(&est);
      return -1;
    }

  tm     = time_in_milliseconds();
  buflen = xpt_read_ret + HEADER_LENGTH;
  enqueuer_stats_record_datagram(&est, buflen);
  header_add(buf, xpt_read_ret, tm, addr, port);
  ++count;
  
  return 0;
}

static int serial_handle_depth_test()
{
  if (toknam_eq((unsigned char *)buf + HEADER_LENGTH, (unsigned char *)SERIAL_DEPTH_COMMAND))
    {
      struct lwes_event_deserialize_tmp event_tmp;
      struct lwes_event_enumeration     keys;
      LWES_CONST_SHORT_STRING           key;
      LWES_TYPE                         type;
      int                               bytes_read;
      struct lwes_event*                event = lwes_event_create_no_name(NULL);
      
      bytes_read = lwes_event_from_bytes(event, (LWES_BYTE_P)&buf[HEADER_LENGTH],
                                         buflen-HEADER_LENGTH, 0, &event_tmp);
      if (bytes_read != buflen-HEADER_LENGTH)
        {
          LOG_ER("Only able to read %d bytes; expected %d\n", bytes_read, buflen);
        }
      else if (!lwes_event_keys(event, &keys))
        {
          LOG_ER("Unable to iterate over keys\n");
        }
      else
        {
          while (lwes_event_enumeration_next_element(&keys, &key, &type))
            {
              if (strncmp("count",key,6)==0)
                {
                  LWES_U_INT_64 value;
                  lwes_event_get_U_INT_64(event, key, &value);
                  pending  = count - value;
                  LOG_INF("Depth test reports a buffer length of %lld events.\n", pending);
                }
            }
        }
      
      lwes_event_destroy(event);
      
      return 1;
    }
  else
    {
      return 0;
    }
}

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
  serial_close_journal();
}

void serial_model(void)
{
  serial_ctor();

  do {
    if (serial_read() < 0)          continue;
    if (serial_handle_depth_test()) continue;
    serial_write();
    if (header_is_rotate(buf) || gbl_rotate) serial_rotate();
    if (tm >= depth_tm + depth_dtm) serial_send_buffer_depth_test();
  }
  while (!gbl_done);

  serial_dtor();
}
