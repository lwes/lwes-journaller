/*======================================================================*
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

#include "lwes_mondemand.h"
#include "opt.h"
#include "stats.h"
#include <string.h>

#ifdef HAVE_MONDEMAND

struct mondemand_client *enqueue_client;
struct mondemand_client *dequeue_client;
/* set a counter in mondemand.
 * NOTE: we need to drop into the lower level call since the journaller
 * is keeping counters itself, and there's no high level call to set a counter
 * in mondemand
 */
#define mondemand_inc(client, x) \
  mondemand_stats_perform_op(client, __FILE__, __LINE__, \
                             MONDEMAND_SET, MONDEMAND_COUNTER, \
                             #x, (MondemandStatValue)(stats->x))
/* set a gauge in mondemand */
#define mondemand_set(client, x) mondemand_set_key_by_val(client, #x, (MondemandStatValue)(stats->x))

/* FIXME: I don't like this, but trying to get serial/thread and process to
 * all work properly, so lots of code duplication for the moment
 */
static void enqueue_init()
{
  if (enqueue_client==NULL
      && arg_mondemand_host!=NULL && arg_mondemand_ip!=NULL)
    {
      struct mondemand_transport *transport = NULL;
      enqueue_client = mondemand_client_create(arg_mondemand_program_id);
      mondemand_set_context(enqueue_client,"host",arg_mondemand_host);
      transport = mondemand_transport_lwes_create(arg_mondemand_ip,arg_mondemand_port,NULL,0,0);
      if (transport)
        {
          mondemand_add_transport(enqueue_client, transport);
        }
      else
        {
          LOG_WARN("Unable to create mondemand transport to connect to %s:%d",arg_mondemand_ip,arg_mondemand_port);
        }
    }
}

static void dequeue_init()
{
  if (dequeue_client==NULL
      && arg_mondemand_host!=NULL && arg_mondemand_ip!=NULL)
    {
      struct mondemand_transport *transport = NULL;
      dequeue_client = mondemand_client_create(arg_mondemand_program_id);
      mondemand_set_context(dequeue_client,"host",arg_mondemand_host);
      transport = mondemand_transport_lwes_create(arg_mondemand_ip,arg_mondemand_port,NULL,0,0);
      if (transport)
        {
          mondemand_add_transport(dequeue_client, transport);
        }
      else
        {
          LOG_WARN("Unable to create mondemand transport to connect to %s:%d",arg_mondemand_ip,arg_mondemand_port);
        }
    }
}

void mondemand_enqueuer_stats (const struct enqueuer_stats* stats, time_t now)
{
  (void)now; // appease -Wall -Werror
  enqueue_init();
  if (enqueue_client==NULL) return;
  mondemand_set(enqueue_client, socket_errors_since_last_rotate);
  mondemand_inc(enqueue_client, bytes_received_total);
  mondemand_set(enqueue_client, bytes_received_since_last_rotate);
  mondemand_inc(enqueue_client, packets_received_total);
  mondemand_set(enqueue_client, packets_received_since_last_rotate);
}

void mondemand_dequeuer_stats (const struct dequeuer_stats* stats, time_t now)
{
  (void)now; // appease -Wall -Werror
  dequeue_init();
  if (dequeue_client==NULL) return;
  mondemand_set(dequeue_client, loss_since_last_rotate);
  mondemand_inc(dequeue_client, bytes_written_total);
  mondemand_set(dequeue_client, bytes_written_since_last_rotate);
  mondemand_inc(dequeue_client, packets_written_total);
  mondemand_set(dequeue_client, packets_written_since_last_rotate);
  mondemand_inc(dequeue_client, bytes_written_in_burst);
  mondemand_inc(dequeue_client, packets_written_in_burst);
  mondemand_set(dequeue_client, hiq);
  mondemand_set(dequeue_client, hiq_start);
  mondemand_set(dequeue_client, hiq_last);
  mondemand_set(dequeue_client, hiq_since_last_rotate);
  mondemand_set(dequeue_client, bytes_written_in_burst_since_last_rotate);
  mondemand_set(dequeue_client, packets_written_in_burst_since_last_rotate);
  mondemand_set(dequeue_client, start_time);
  mondemand_set(dequeue_client, last_rotate);
}

void mondemand_enqueuer_flush (void) {
  enqueue_init();
  if (enqueue_client != NULL)
    {
      mondemand_flush_stats(enqueue_client);
    }
}

void mondemand_dequeuer_flush (void) {
  dequeue_init();
  if (dequeue_client != NULL)
    {
      mondemand_flush_stats(dequeue_client);
    }
}

static int get_mondemand_level (log_level_t level)
{
  switch (level)
    {
    case LOG_ERROR:
      return M_LOG_ERR;
    case LOG_WARNING:
      return M_LOG_WARNING;
    case LOG_INFO:
      return M_LOG_INFO;
    default:
      return M_LOG_DEBUG;
    }
}

void mondemand_log_msg (log_level_t level, const char *fname, int lineno, const char *buf)
{
  const int mondemand_level = get_mondemand_level(level);

  struct mondemand_client *client =
    (enqueue_client != NULL ? enqueue_client : dequeue_client);
  if (mondemand_level_is_enabled(client, mondemand_level))
  {
    mondemand_log_real(client, fname, lineno, level, MONDEMAND_NULL_TRACE_ID, "%s", buf);
  }
}

#else /* HAVE_MONDEMAND */

void mondemand_enqueuer_stats (const struct enqueuer_stats* stats, time_t now)
{
  (void)stats;
  (void)now;
}

void mondemand_dequeuer_stats (const struct dequeuer_stats* stats, time_t now)
{
  (void)stats;
  (void)now;
}

void mondemand_enqueuer_flush (void) {
}

void mondemand_dequeuer_flush (void) {
}

void mondemand_log_msg (log_level_t level, const char *fname, int lineno, const char *buf)
{
  (void)level;
  (void)fname;
  (void)lineno;
  (void)buf;
}

#endif /* HAVE_MONDEMAND */
