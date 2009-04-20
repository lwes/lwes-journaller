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

#ifndef OPT_DOT_H
#define OPT_DOT_H

extern int      arg_args;
extern char*    arg_basename;
extern char*    arg_interface;
/*TODO: extern int      arg_interval;*/
extern char*    arg_ip;
extern int      arg_hurryup_at;
extern int      arg_hurrydown_at;
extern int      arg_join_group;
extern char**   arg_journalls;
extern char*		arg_disk_journals[10];
extern char*    arg_journ_name;
extern char*    arg_journ_type;
extern char*    arg_monitor_type;
extern int      arg_log_level;
extern int      arg_njournalls;
extern int      arg_nodaemonize;
extern int      arg_nopong;
extern int      arg_nreaders;
extern char*    arg_pid_file;
extern int      arg_port;
extern char*    arg_proc_type;
extern int      arg_queue_max_cnt;
extern int      arg_queue_max_sz;
extern char*    arg_queue_name;
extern char*    arg_queue_type;
extern int      arg_rotate;
extern int      arg_rotate_mask;
extern int      arg_rt;
extern char*    arg_sink_ram;
extern int      arg_site;
extern int      arg_sockbuffer;
extern int			arg_ttl ;
extern int      arg_version;
extern char*    arg_xport;

/* arg_proc_type: */
#define ARG_PROCESS "process"
#define ARG_THREAD  "thread"

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
