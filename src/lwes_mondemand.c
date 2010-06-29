#include "lwes_mondemand.h"
#include "opt.h"
#include <string.h>

#ifdef HAVE_MONDEMAND

struct mondemand_client *client;
#define mondemand_inc(x) mondemand_increment_key_by_val(client, #x, stats->x)

static void init()
{
  if (client==NULL && arg_mondemand_host!=NULL && arg_mondemand_ip!=NULL)
    {
      struct mondemand_transport *transport = NULL;
      client = mondemand_client_create("lwes-journaller");
      mondemand_set_context(client,"host",arg_mondemand_host);
      transport = mondemand_transport_lwes_create(arg_mondemand_ip,arg_mondemand_port,NULL,0,0);
      if (transport)
        {
          mondemand_add_transport(client, transport);
        }
      else
        {
          LOG_WARN("Unable to create mondemand transport to connect to %s:%d",arg_mondemand_ip,arg_mondemand_port);
        }
    }
}

void mondemand_stats (const struct stats* stats, time_t now)
{
  init();
  if (client==NULL) return;
  mondemand_inc(loss);
  mondemand_inc(bytes_total);
  mondemand_inc(bytes_since_last_rotate);
  mondemand_inc(packets_total);
  mondemand_inc(packets_since_last_rotate);
  mondemand_inc(bytes_in_burst);
  mondemand_inc(packets_in_burst);
  mondemand_inc(hiq);
  mondemand_inc(hiq_start);
  mondemand_inc(hiq_last);
  mondemand_inc(hiq_since_last_rotate);
  mondemand_inc(bytes_in_burst_since_last_rotate);
  mondemand_inc(packets_in_burst_since_last_rotate);
  mondemand_inc(start_time);
  mondemand_inc(last_rotate);
  mondemand_increment_key_by_val(client, "rotate", now);
  mondemand_flush_stats(client);
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
  if (mondemand_level_is_enabled(client, mondemand_level))
  {
    mondemand_log_real(client, fname, lineno, level, MONDEMAND_NULL_TRACE_ID, "%s", buf);
  }
}

#else /* HAVE_MONDEMAND */

void mondemand_stats (const struct stats* stats, time_t now)
{
}

void mondemand_log_msg (log_level_t level, const char *fname, int lineno, const char *buf)
{
}

#endif /* HAVE_MONDEMAND */
