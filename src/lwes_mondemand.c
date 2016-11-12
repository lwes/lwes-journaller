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

#include "config.h"
#include "lwes_mondemand.h"
#include "opt.h"

#ifdef HAVE_MONDEMAND
#include <mondemand.h>

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

static
struct mondemand_client *
create_client (void)
{
  struct mondemand_client *client = NULL;
  if (arg_mondemand_host != NULL && arg_mondemand_ip != NULL)
    {
      struct mondemand_transport *transport = NULL;
      client = mondemand_client_create(arg_mondemand_program_id);
      mondemand_set_context(client,"host",arg_mondemand_host);
      transport =
        mondemand_transport_lwes_create( arg_mondemand_ip,
                                         arg_mondemand_port,NULL,0,0);
      if (transport)
        {
          mondemand_add_transport (client, transport);
        }
      else
        {
          mondemand_client_destroy (client);
          return NULL;
        }
    }
  return client;
}

void
md_enqueuer_create (struct enqueuer_stats *stats)
{
  stats->client = create_client();
}

void
md_enqueuer_stats (const struct enqueuer_stats* stats)
{
  if (stats->client==NULL) return;
  mondemand_set (stats->client, socket_errors_since_last_rotate);
  mondemand_inc (stats->client, bytes_received_total);
  mondemand_set (stats->client, bytes_received_since_last_rotate);
  mondemand_inc (stats->client, packets_received_total);
  mondemand_set (stats->client, packets_received_since_last_rotate);
}

void md_enqueuer_flush (const struct enqueuer_stats* stats) {
  mondemand_flush_stats (stats->client);
}

void md_enqueuer_destroy(struct enqueuer_stats* stats)
{
  mondemand_client_destroy (stats->client);
  stats->client = NULL;
}

void
md_dequeuer_create (struct dequeuer_stats *stats)
{
  stats->client = create_client();
}

void
md_dequeuer_stats (const struct dequeuer_stats* stats)
{
  if (stats->client==NULL) return;
  mondemand_set (stats->client, loss_since_last_rotate);
  mondemand_inc (stats->client, bytes_written_total);
  mondemand_set (stats->client, bytes_written_since_last_rotate);
  mondemand_inc (stats->client, packets_written_total);
  mondemand_set (stats->client, packets_written_since_last_rotate);
  mondemand_inc (stats->client, bytes_written_in_burst);
  mondemand_inc (stats->client, packets_written_in_burst);
  mondemand_set (stats->client, hiq);
  mondemand_set (stats->client, hiq_start);
  mondemand_set (stats->client, hiq_last);
  mondemand_set (stats->client, hiq_since_last_rotate);
  mondemand_set (stats->client, bytes_written_in_burst_since_last_rotate);
  mondemand_set (stats->client, packets_written_in_burst_since_last_rotate);
  mondemand_set (stats->client, start_time);
  mondemand_set (stats->client, last_rotate);
}

void md_dequeuer_flush (const struct dequeuer_stats* stats) {
  mondemand_flush_stats (stats->client);
}

void md_dequeuer_destroy(struct dequeuer_stats* stats)
{
  mondemand_client_destroy (stats->client);
  stats->client = NULL;
}

#else

void
md_enqueuer_create (struct enqueuer_stats *stats)
{
  (void)stats; /* appease -Wall -Werror */
}

void
md_enqueuer_stats (const struct enqueuer_stats* stats)
{
  (void)stats; /* appease -Wall -Werror */
}

void
md_enqueuer_flush (const struct enqueuer_stats* stats)
{
  (void)stats; /* appease -Wall -Werror */
}

void
md_enqueuer_destroy(struct enqueuer_stats* stats)
{
  (void)stats; /* appease -Wall -Werror */
}

void
md_dequeuer_create (struct dequeuer_stats *stats)
{
  (void)stats; /* appease -Wall -Werror */
}

void
md_dequeuer_stats (const struct dequeuer_stats* stats)
{
  (void)stats; /* appease -Wall -Werror */
}

void
md_dequeuer_flush (const struct dequeuer_stats* stats)
{
  (void)stats; /* appease -Wall -Werror */
}

void
md_dequeuer_destroy (struct dequeuer_stats* stats)
{
  (void)stats; /* appease -Wall -Werror */
}

#endif /* HAVE_MONDEMAND */
