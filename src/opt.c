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

#define _GNU_SOURCE
#include "config.h"

#include "log.h"
#include "opt.h"

#if HAVE_LIBGEN_H
#include <libgen.h>
#endif

#include <string.h>
#include <popt.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

/* Base program name from argv[0]. */
char*        arg_basename      = 0;

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

#if HAVE_LIBZ
const char*  arg_journ_type    = ARG_GZ;
#else
const char*  arg_journ_type    = ARG_FILE;
#endif

#if HAVE_PTHREAD_H
const char*  arg_proc_type     = ARG_THREAD;
#else
const char*  arg_proc_type     = ARG_PROCESS;
#endif

/* network transport parameters */
const char*  arg_xport         = "udp";
const char*  arg_ip            = "224.0.0.69";
int          arg_port          = 9191;
const char*  arg_interface     = NULL;
int          arg_sockbuffer    = 16*1024*1024;
int          arg_ttl           = 16 ;

/* Set the logging level, see log.h. */
int    arg_log_level           =  LOG_MASK_ERROR
                                | LOG_MASK_WARNING
                                | LOG_MASK_INFO;
const char* arg_log_file       = NULL;

int    arg_rt                  = 0;

/* site ID used to set various config items from config file. */
int    arg_site                = 1;

/* intended owner of journal files */
int         arg_journal_uid    = 0;
const char* arg_journal_user   = NULL;

/* Queue report interval. */
int    arg_queue_test_interval = 10000;

const char*  arg_pid_file      = "/var/run/lwes-journaller.pid";

/* Journals specified and number of journals specified. */
char** arg_journalls;
int    arg_njournalls;
char*  arg_disk_journals[10];

/* Journal rotate interval in seconds (will attempt to rotate on 
 * round intervals starting at beginning of day)
 */
int    arg_journal_rotate_interval = 0;
int    arg_wakeup_interval_ms = WAKEUP_MS;

int    arg_nodaemonize         = 0;

/* Print version, then exit. */
int    arg_version;

/* Print args to sub-programs, then exit. */
int    arg_args;

#ifdef HAVE_MONDEMAND
const char*    arg_mondemand_host = NULL;
const char*    arg_mondemand_ip    = NULL;
const char*    arg_mondemand_interface = NULL;
int            arg_mondemand_port  = 20402;
int            arg_mondemand_ttl   = 10;
const char*    arg_mondemand_program_id = "lwes-journaller";
#endif

int process_options(int argc, const char* argv[], FILE *log)
{
  static const struct poptOption options[] = {
    { "args",          0,  POPT_ARG_NONE,   &arg_args,           0, "Print command line arguments, then exit", 0 },
    { "nodaemonize",   0,  POPT_ARG_NONE,   &arg_nodaemonize,    0, "Don't run in the background", 0 },
    { "log-level",     0,  POPT_ARG_INT,    &arg_log_level,      0, "Set the output logging level - OFF=(1), ERROR=(2), WARNING=(4), INFO=(8), PROGRESS=(16)", "mask" },
    { "log-file",      0,  POPT_ARG_STRING, &arg_log_file,       0, "Set the output log file", "file" },
    { "interface",    'I', POPT_ARG_STRING, &arg_interface,      0, "Network interface to listen on", "ip" },
    { "queue-test-interval", 'q', POPT_ARG_INT, &arg_queue_test_interval, 0, "Queue depth test interval for serial mode (dflt=10000)", "milliseconds" },
    { "address",      'm', POPT_ARG_STRING, &arg_ip,             0, "IP address", "ip" },
    { "journal-type", 'j', POPT_ARG_STRING, &arg_journ_type,     0, "Journal type", "{" ARG_GZ "," ARG_FILE "}" },
    { "journal-rotate-interval", 'i', POPT_ARG_INT, &arg_journal_rotate_interval,     0, "Journal rotation interval in seconds (default off)", 0 },
    { "pid-file",     'f', POPT_ARG_STRING, &arg_pid_file,       0, "PID file, dflt=NULL", "path" },
    { "port",         'p', POPT_ARG_INT,    &arg_port,           0, "Port number to listen on, dflt=9191", "short" },
    { "thread-type",  't', POPT_ARG_STRING, &arg_proc_type,      0, "Threading model, '" ARG_THREAD "' or '" ARG_PROCESS "' or '" ARG_SERIAL "', dflt="
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
    { "real-time",    'R', POPT_ARG_NONE,   &arg_rt,             0, "Run threads with real-time priority", 0 },
    { "site",         'n', POPT_ARG_INT,    &arg_site,           0, "Site id", "int" },
    { "sockbuffer",    0,  POPT_ARG_INT,    &arg_sockbuffer,     0, "Receive socket buffer size", "bytes" },
    { "ttl",           0,  POPT_ARG_INT,    &arg_ttl,            0, "Emitting TTL value", "hops" },
    { "wakeup", 'w', POPT_ARG_INT, &arg_wakeup_interval_ms, 0, "How often to break checking for signals", "milliseconds" },
    { "user",          0,  POPT_ARG_STRING, &arg_journal_user,   0, "Owner of journal files", "user" },
    { "version",      'v', POPT_ARG_NONE,   &arg_version,        0, "Display version, then exit", 0 },
    { "xport-type",   'x', POPT_ARG_STRING, &arg_xport,          0, "Transport, dflt=udp", "{" ARG_UDP ", ...}" },
#ifdef HAVE_MONDEMAND
    { "mondemand-host", 0, POPT_ARG_STRING, &arg_mondemand_host, 0, "Mondemand monitoring host", "string" },
    { "mondemand-ip",   0, POPT_ARG_STRING, &arg_mondemand_ip,   0, "Mondemand monitoring ip", "ip-address" },
    { "mondemand-interface",   0, POPT_ARG_STRING, &arg_mondemand_interface,   0, "Mondemand monitoring interface to emit out of of", "ip-address" },
    { "mondemand-port", 0, POPT_ARG_INT,    &arg_mondemand_port, 0, "Mondemand monitoring port dflt=20402", "port" },
    { "mondemand-ttl", 0, POPT_ARG_INT,    &arg_mondemand_ttl, 0, "Mondemand monitoring ttl (default is 10)", "ttl" },
    { "mondemand-program-id", 0, POPT_ARG_STRING, &arg_mondemand_program_id, 0, "Mondemand program id (default is 'lwes-journaller')", "program-id" },
#endif

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
              /* You might want to check options of other types here... */

            default:
              LOG_ER(log, "%s: %s\n",
                     poptBadOption(optCon, POPT_BADOPTION_NOALIAS),
                     poptStrerror(rc));
              ++bad_options;
            }
        }
    }

  char ** possible_arg_journalls = (char**) poptGetArgs(optCon);

  /* Count the journals specified on the command line. */
  if ( possible_arg_journalls )
    {
      for ( arg_njournalls=0;
            possible_arg_journalls[arg_njournalls];
            ++arg_njournalls )
        ;
    }

  /* Use default journal spec if none provided on the command line. */
  if ( 0 == arg_njournalls )
    {
      arg_journalls = (char**) malloc(sizeof(char*));
      arg_journalls[0] = strdup ("/tmp/all_events.log.gz");
      arg_njournalls = 1;
    }
  else
    {
      arg_journalls = (char **)malloc(sizeof(char*)*arg_njournalls);
      int i = 0;
      for (i = 0; possible_arg_journalls[i]; ++i)
        {
          arg_journalls[i] = strdup (possible_arg_journalls[i]);
        }
    }

  /* convert the journal file username to a uid */
  if ( arg_journal_user != NULL)
    {
      struct passwd *pw_entry = getpwnam(arg_journal_user);
      if (pw_entry==NULL)
        {
          LOG_ER(log, "%s was told to use a file owner (%s) that could not be found\n",
                 arg_basename, arg_journal_user);
          ++bad_options;
        }
      else
        {
          arg_journal_uid = pw_entry->pw_uid;
        }
    }
  else
    {
      /* default to the effective id of the user */
      arg_journal_uid = geteuid();
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

      return -1;
    }

  if ( arg_args )
    {
      char log_level_string[100];

      log_get_mask_string(log_level_string, sizeof(log_level_string));

      printf("arguments:\n"
              "  arg_basename == \"%s\"\n"
              "  arg_interface == \"%s\"\n"
              "  arg_ip == \"%s\"\n"
              "  arg_journ_type == \"%s\"\n"
              "  arg_pid_file == \"%s\"\n"
              "  arg_proc_type == \"%s\"\n"
              "  arg_queue_name == \"%s\"\n"
              "  arg_queue_type == \"%s\"\n"
              "  arg_xport == \"%s\"\n"
              /*"  arg_interval == %d\n"*/
              "  arg_log_level == %s (%d)\n"
              "  arg_log_file == %s\n"
              "  arg_njournalls == %d\n"
              "  arg_port == %d\n"
              "  arg_queue_max_cnt == %d\n"
              "  arg_queue_max_sz == %d\n"
              "  arg_rt == %d\n"
              "  arg_site == %d\n"
              "  arg_ttl == %d\n"
              "  arg_journal_user == %s\n"
              "  arg_journal_uid == %d\n"
#ifdef HAVE_MONDEMAND
              "  arg_mondemand_host == %s\n"
              "  arg_mondemand_ip == %s\n"
              "  arg_mondemand_interface == %s\n"
              "  arg_mondemand_port == %d\n"
              "  arg_mondemand_ttl == %d\n"
              "  arg_mondemand_program_id == %s\n"
#endif
              ,
              arg_basename,
              arg_interface,
              arg_ip,
              arg_journ_type,
              arg_pid_file,
              arg_proc_type,
              arg_queue_name,
              arg_queue_type,
              arg_xport,
              /*TODO: arg_interval,*/
              log_level_string,
              arg_log_level,
              arg_log_file,
              arg_njournalls,
              arg_port,
              arg_queue_max_cnt,
              arg_queue_max_sz,
              arg_rt,
              arg_site,
              arg_ttl,
              arg_journal_user,
              arg_journal_uid
#ifdef HAVE_MONDEMAND
             , arg_mondemand_host,
               arg_mondemand_ip,
               arg_mondemand_interface,
               arg_mondemand_port,
               arg_mondemand_ttl,
               arg_mondemand_program_id
#endif
              );

      for ( arg_njournalls=0; arg_journalls[arg_njournalls]; ++arg_njournalls )
        {
          printf("journall[%d] == %s\n",
                 arg_njournalls,
                 arg_journalls[arg_njournalls]);
        }
      return -1;
    }

#ifdef HAVE_MONDEMAND
  if (    arg_mondemand_host == NULL
       && arg_mondemand_ip   != NULL )
    {
      /* mondemand was requested, but no hostname was specified.  Choose one. */
      char host[256];
      if ( gethostname(host,sizeof(host)-1) == 0 )
        {
          /* this strndup is an intentional one-time memory leak. */
          arg_mondemand_host = strndup(host,sizeof(host)-1);
        }
      else
        {
          LOG_WARN(log,
                   "Mondemand requires a hostname, but none was provided or"
                   " available.  Disabling mondemand.");
          arg_mondemand_ip = NULL;
        }
    }
#endif

  if (      arg_journalls == NULL
       || ! arg_journalls[0]
       || ! strrchr(arg_journalls[0],'/') )
    {
      LOG_ER(log, "%s requires output pathnames in form "
             "'/path/to/archive/file.gz'\n", arg_basename);
      ++bad_options;
    }

  if (arg_journal_rotate_interval < 0)
    {
      LOG_ER(log, "--journal-rotate-interval should be positive\n");
      ++bad_options;
    }

  if ( strcmp(arg_xport, "udp") != 0 )
    {
      LOG_ER(log, "unrecognized transport type \"%s\", try \"udp\"\n",
             arg_xport);
      ++bad_options;
    }

  poptFreeContext(optCon);
  return bad_options;
}

void options_destructor (void)
{
  int i;
  for (i = 0; i < arg_njournalls; i++)
    {
      free(arg_journalls[i]);
    }
  free(arg_journalls);
}

