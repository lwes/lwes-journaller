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

#ifndef OPT_DOT_H
#define OPT_DOT_H

#include <stdio.h>

extern int            arg_args;
extern char*          arg_basename;
extern const char*    arg_interface;
extern int            arg_queue_test_interval;
extern const char*    arg_ip;
extern char**         arg_journalls;
extern char*          arg_disk_journals[10];
extern int            arg_journal_rotate_interval;
extern char*          arg_journ_name;
extern const char*    arg_journ_type;
extern int            arg_log_level;
extern const char*    arg_log_file;
extern int            arg_njournalls;
extern int            arg_nodaemonize;
extern const char*    arg_pid_file;
extern int            arg_port;
extern const char*    arg_proc_type;
extern int            arg_queue_max_cnt;
extern int            arg_queue_max_sz;
extern const char*    arg_queue_name;
extern const char*    arg_queue_type;
extern int            arg_rt;
extern int            arg_site;
extern int            arg_sockbuffer;
extern int            arg_ttl;
extern int            arg_journal_uid;
extern int            arg_version;
extern const char*    arg_xport;
extern int            arg_wakeup_interval_ms;

#ifdef HAVE_MONDEMAND
extern const char*    arg_mondemand_host;
extern const char*    arg_mondemand_ip;
extern const char*    arg_mondemand_interface;
extern int            arg_mondemand_port;
extern int            arg_mondemand_ttl;
extern const char*    arg_mondemand_program_id;
#endif

/* arg_proc_type: */
#define ARG_PROCESS "process"
#define ARG_THREAD  "thread"
#define ARG_SERIAL  "serial"

/* arg_queue_type: */
#define ARG_MQ      "mq"
#define ARG_MSG     "msg"

/* arg_journ_type: */
#define ARG_GZ      "gz"
#define ARG_FILE    "file"

/* arg_xport: */
#define ARG_UDP     "udp"

#define WAKEUP_MS   100

int process_options(int argc, const char* argv[], FILE *log);
void options_destructor (void);

#endif /* OPT_DOT_H */
