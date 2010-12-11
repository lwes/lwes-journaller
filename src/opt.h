/*======================================================================*
 * Copyright (c) 2008, Yahoo! Inc. All rights reserved.                 *
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

extern int            arg_args;
extern char*          arg_basename;
extern const char*    arg_interface;
/*TODO: extern int      arg_interval;*/
extern const char*    arg_ip;
extern int            arg_join_group;
extern char**         arg_journalls;
extern char*          arg_disk_journals[10];
extern char*          arg_journ_name;
extern const char*    arg_journ_type;
extern char*          arg_monitor_type;
extern int            arg_log_level;
extern const char*    arg_log_file;
extern int            arg_njournalls;
extern int            arg_nodaemonize;
extern int            arg_nreaders;
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

#ifdef HAVE_MONDEMAND
extern const char*    arg_mondemand_host;
extern const char*    arg_mondemand_ip;
extern int            arg_mondemand_port;
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

void process_options(int argc, const char* argv[]);

#endif /* OPT_DOT_H */
