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

#include "log.h"
#include "opt.h"
#include <string.h>

#if HAVE_LIBGEN_H
#include <libgen.h>
#endif

#include <popt.h>
#include <stdlib.h>
#include <unistd.h>

/* Base program name from argv[0]. */
char*        arg_basename      = 0;

/* Number of network reader processes/threads to run. */
int          arg_nreaders      = 1;

/* Set default queue type, types in preferred order. */
#if HAVE_MQUEUE_H
const char*  arg_queue_type    = ARG_MQ;
#elif HAVE_SYS_MSG_H
const char*  arg_queue_type    = ARG_MSG;
#else
#error No kernel message queue support on this platform.
#endif

/* Queue parameters. */
const char*  arg_queue_name    = "/lwes_journal";
int          arg_queue_max_sz  = 64*1024 - 1;
int          arg_queue_max_cnt = 10000;
int          arg_hurryup_at    = 80 ;
int          arg_hurrydown_at  = 50 ;
int          arg_join_group;
char*        arg_sink_ram      = NULL ;
char         nam_sink_ram[500] ; /* holds real /sink/ram/all_events.log.gz
                                  * filename */

#if HAVE_LIBZ
const char*  arg_journ_type    = ARG_GZ;
#else
const char*  arg_journ_type    = ARG_FILE;
#endif
int    arg_rotate_mask         = 30 ; /* ignore any duplicate Command::Rotate
                                       * within 30 seconds */
char*  arg_monitor_type        = NULL ;

#if HAVE_PTHREAD_H
const char*  arg_proc_type     = ARG_THREAD;
#else
const char*  arg_proc_type     = ARG_PROCESS;
#endif

/* network transport parameters */
const char*  arg_xport         = "udp";
const char*  arg_ip            = "224.0.0.69";
int          arg_port          = 9191;
const char*  arg_interface     = "";
int          arg_join_group    = 1;
int          arg_sockbuffer    = 16*1024*1024;
int          arg_ttl           = 16 ;
int          arg_nopong        = 0 ;

/* Set the logging level, see log.h. */
int    arg_log_level           =  LOG_MASK_ERROR
                                | LOG_MASK_WARNING
                                | LOG_MASK_INFO;
const char* arg_log_file       = NULL;

int    arg_rt                  = 0;

/* site ID used to set various config items from config file. */
int    arg_site                = 1;

/* Queue report interval. */
/*TODO: int    arg_interval     = 60;*/

const char*  arg_pid_file      = "/var/run/lwes-journaller.pid";

/* Journals specified and number of journals specified. */
char** arg_journalls;
int    arg_njournalls;
char*  arg_disk_journals[10];

int    arg_nodaemonize         = 0;

/* Print version, then exit. */
int    arg_version;

/* Print args to sub-programs, then exit. */
int    arg_args;

void process_options(int argc, const char* argv[])
{
  static const struct poptOption options[] = {
    { "args",          0,  POPT_ARG_NONE,   &arg_args,           0, "Print command line arguments, then exit", 0 },
    { "nodaemonize",   0,  POPT_ARG_NONE,   &arg_nodaemonize,    0, "Don't run in the background", 0 },
    { "hurryup-at",    0,  POPT_ARG_INT,    &arg_hurryup_at,     0, "Hurry-up if queue use > this, dflt=80", "percentage" },
    { "hurrydown-at",  0,  POPT_ARG_INT,    &arg_hurrydown_at,   0, "Quit hurry-up when queue use returns to < this, dflt=50", "percentage" },
    { "log-level",     0,  POPT_ARG_INT,    &arg_log_level,      0, "Set the output logging level - OFF=(1), ERROR=(2), WARNING=(4), INFO=(8), PROGRESS=(16)", "mask" },
    { "log-file",      0,  POPT_ARG_STRING, &arg_log_file,       0, "Set the output log file", "file" },
    { "interface",    'I', POPT_ARG_STRING, &arg_interface,      0, "Network interface to listen on", "ip" },
    /*TODO: { "report-interval", 's', POPT_ARG_INT, &arg_interval,       0, "Queue report interval", "seconds" },*/
    { "address",      'm', POPT_ARG_STRING, &arg_ip,             0, "IP address", "ip" },
    { "join-group",   'g', POPT_ARG_INT,    &arg_join_group,     0, "Join multicast group", "0/1" },
    { "journal-type", 'j', POPT_ARG_STRING, &arg_journ_type,     0, "Journal type", "{" ARG_GZ "," ARG_FILE "}" },
    { "monitor-type", 'j', POPT_ARG_STRING, &arg_monitor_type,   0, "Monitor type", 0 },
    { "nopong",        0,  POPT_ARG_NONE,   &arg_nopong,         0, "Don not reply to System::Ping", 0 },
    { "nreaders",     'r', POPT_ARG_INT,    &arg_nreaders,       0, "Number of network reading threads, dflt=1, max=5", 0 },
    { "pid-file",     'f', POPT_ARG_STRING, &arg_pid_file,       0, "PID file, dflt=NULL", "path" },
    { "port",         'p', POPT_ARG_INT,    &arg_port,           0, "Port number to listen on, dflt=9191", "short" },
    { "thread-type",  't', POPT_ARG_STRING, &arg_proc_type,      0, "Threading model, '" ARG_THREAD "' or '" ARG_PROCESS "', dflt="
#if HAVE_PTHREAD_H
      ARG_THREAD
#else
      ARG_PROCESS
#endif
        , 0 },
    { "queue-max-cnt", 0,  POPT_ARG_INT,    &arg_queue_max_cnt,  0, "Max messages for queue, dflt=10000", "int" },
    { "queue-max-sz",  0,  POPT_ARG_INT,    &arg_queue_max_sz,   0, "Max message size for queue, dflt=65535", "int" },
    { "queue-name",   'Q', POPT_ARG_STRING, &arg_queue_name,     0, "Queue name, should start with '/', dflt='/lwes_journal'", "string" },
    { "queue-type",   'q', POPT_ARG_STRING, &arg_queue_type,     0, "Queue type", "{" ARG_MSG "," ARG_MQ "}" },
    { "rotate-mask",  'd', POPT_ARG_INT,    &arg_rotate_mask,    0, "Rotate mask, dflt=30", "seconds" },
    { "real-time",    'R', POPT_ARG_NONE,   &arg_rt,             0, "Run threads with real-time priority", 0 },
    { "sink-ram",     'z', POPT_ARG_STRING, &arg_sink_ram,       0, "/sink/ram/", "NULL" },
    { "site",         'n', POPT_ARG_INT,    &arg_site,           0, "Site id", "int" },
    { "sockbuffer",    0,  POPT_ARG_INT,    &arg_sockbuffer,     0, "Receive socket buffer size", "bytes" },
    { "ttl",           0,  POPT_ARG_INT,    &arg_ttl,            0, "Emitting TTL value", "hops" },
    { "version",      'v', POPT_ARG_NONE,   &arg_version,        0, "Display version, then exit", 0 },
    { "xport-type",   'x', POPT_ARG_STRING, &arg_xport,          0, "Transport, dflt=udp", "{" ARG_UDP ", ...}" },

    POPT_AUTOHELP
    { NULL, 0, 0, NULL, 0, NULL, NULL }};

  int bad_options = 0;
  int rc;

#if HAVE_LIBGEN_H
  arg_basename = basename((char*)argv[0]);
#else
  arg_basename = (char*)argv[0];
#endif

  poptContext optCon = poptGetContext(NULL, argc, argv, options, 0);
  poptSetOtherOptionHelp(optCon, "");

  while ( (rc = poptGetNextOpt(optCon)) != -1 )
    {
      if ( rc < 0 )
        {
          switch ( rc )
            {
            case POPT_ERROR_BADOPT:
              /* You might waht to check options of other types here... */

            default:
              LOG_ER("%s: %s\n",
                     poptBadOption(optCon, POPT_BADOPTION_NOALIAS),
                     poptStrerror(rc));
              ++bad_options;
            }
        }
    }

  arg_journalls = (char**) poptGetArgs(optCon);

  /* Count the journals specified on the command line. */
  if ( arg_journalls )
    {
      for ( arg_njournalls=0; arg_journalls[arg_njournalls]; ++arg_njournalls )
        ;
    }

  /* Use default journal spec if none provided on the command line. */
  if ( 0 == arg_njournalls )
    {
      static const char* default_arg_journalls[] = { "/tmp/all_events.log.gz",
                                                     NULL };
      arg_journalls = (char**) default_arg_journalls;
      arg_njournalls = 1;
    }

  if ( arg_sink_ram != NULL )
    {
      strcpy(nam_sink_ram, arg_sink_ram) ;
      if ( nam_sink_ram[strlen(nam_sink_ram)-1] != '/' )
        {
          strcat(nam_sink_ram, "/") ;
        }
      strcat(nam_sink_ram, strrchr(arg_journalls[0],'/') + 1) ;

      arg_disk_journals[arg_njournalls] = NULL ;
      while ( --arg_njournalls >= 0 )
        {
          /* copy original arg_journalls to arg_disk_journals */
          arg_disk_journals[arg_njournalls] = arg_journalls[arg_njournalls] ;
        }

      arg_journalls[0] = nam_sink_ram ;
      arg_journalls[1] = NULL ;
      arg_njournalls = 1 ;
    }

  if ( arg_version )
    {
      printf("The packet journaller is a program for recording LWES\n"
             "messages for later processing.\n"
             "\n"
             "It listens for UDP packets on an interface/IP address/port\n"
             "combination and writes them to an optionally compressed\n"
             "packet journal file.\n"
             "\n"
             "It adds a 22 bytes header with the packet size and sender\n"
             "information.  See the README file for additional details.\n"
             "\n"
             "The journaller looks at the \"message type\" in each received\n"
             "packet and will rotate the logs if it matches\n"
             "\"Command::Rotate\".\n"
             "\n"
            );

      printf("Compile time features: \n"

             " process model(s): "
#if HAVE_PTHREAD_H
             "threads "
#endif
             "exec "
             ";\n"

             " queue type(s): "
#if HAVE_MQUEUE_H
             "mqueue "
#endif
#if HAVE_SYS_MSG_H
             "msg "
#endif
             ";\n"

             " journal type(s): "
#if HAVE_LIBZ
             "gz "
#endif
             "file "
             ";\n"

#if HAVE_SCHED_H
             " scheduling type(s): FIFO "
             ";\n"
#endif

             "\n"
            );

      printf("Bug reports to %s\n\n", PACKAGE_BUGREPORT);

      exit(EXIT_SUCCESS);
    }

  if ( arg_args )
    {
      char log_level_string[100];

      log_get_mask_string(log_level_string, sizeof(log_level_string));

      log_msg(LOG_INFO, __FILE__, __LINE__,
              "arguments:\n"
              "  arg_basename == \"%s\"\n"
              "  arg_hurryup_at == %d\n"
              "  arg_hurrydown_at == %d\n"
              "  arg_interface == \"%s\"\n"
              "  arg_ip == \"%s\"\n"
              "  arg_journ_type == \"%s\"\n"
              "  arg_monitor_type == \"%s\"\n"
              "  arg_pid_file == \"%s\"\n"
              "  arg_proc_type == \"%s\"\n"
              "  arg_queue_name == \"%s\"\n"
              "  arg_queue_type == \"%s\"\n"
              "  arg_xport == \"%s\"\n"
              /*"  arg_interval == %d\n"*/
              "  arg_join_group == %d\n"
              "  arg_log_level == %s (%d)\n"
              "  arg_log_file == %s\n"
              "  arg_njournalls == %d\n"
              "  %s" // -nopong generates own message
              "  arg_nreaders == %d\n"
              "  arg_port == %d\n"
              "  arg_queue_max_cnt == %d\n"
              "  arg_queue_max_sz == %d\n"
              "  arg_rt == %d\n"
              "  arg_sink_ram == %s\n"
              "  arg_site == %d\n"
              "  arg_ttl == %d\n",
              arg_basename,
              arg_hurryup_at,
              arg_hurrydown_at,
              arg_interface,
              arg_ip,
              arg_journ_type,
              arg_monitor_type,
              arg_pid_file,
              arg_proc_type,
              arg_queue_name,
              arg_queue_type,
              arg_xport,
              /*TODO: arg_interval,*/
              arg_join_group,
              log_level_string,
              arg_log_level,
              arg_log_file,
              arg_njournalls,
              arg_nopong?"  arg_nopong\n":"",
              arg_nreaders,
              arg_port,
              arg_queue_max_cnt,
              arg_queue_max_sz,
              arg_rt,
              arg_sink_ram,
              arg_site,
              arg_ttl);

      for ( arg_njournalls=0; arg_journalls[arg_njournalls]; ++arg_njournalls )
        {
          log_msg(LOG_INFO, __FILE__, __LINE__,
                  "journall[%d] == %s\n",
                  arg_njournalls,
                  arg_journalls[arg_njournalls]);
        }
      exit(EXIT_SUCCESS);
    }

  if (      arg_journalls == NULL
       || ! arg_journalls[0]
       || ! strrchr(arg_journalls[0],'/') )
    {
      LOG_ER("%s requires output pathnames in form "
             "'/path/to/archive/file.gz'\n", arg_basename);
      ++bad_options;
    }

  if ( bad_options )
    {
      exit(EXIT_FAILURE);
    }
}
