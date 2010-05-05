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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include "log.h"
#include "opt.h"

#define LOG_INTERVAL        86400
#define MAX_LOG_FILE_LENGTH  1024
#define TIMESTAMP_LENGTH      100

static FILE* log          = NULL;
time_t       log_time     = 0;

static void rotate(const char *time_str) {
  char new_log_file[MAX_LOG_FILE_LENGTH+TIMESTAMP_LENGTH+2];
  char *cp;
  int index;
  struct stat *stat_buf;
  /* there's no point in trying to rotate a nonexistent log file */
  if (stat(arg_log_file)!=0) return;
  /* add the date to the filename */
  strncpy(new_log_file,arg_log_file,sizeof(new_log_file));
  strncat(new_log_file,"-",sizeof(new_log_file));
  strncat(new_log_file,time_str,sizeof(new_log_file));
  cp = strrchr(new_log_file,' ');
  if (cp != NULL)
  {
    *cp = '\0';
  }
  /* as long as we're pointing at an existing file, increment a suffix. */
  cp = new_log_file+strlen(new_log_file);
  for (index=0; 1; ++index)
  {
    *cp = '\0';
    if (index>0) sprintf(cp,".%d",index);
    if (stat(new_log_file)==0) continue;
    if (errno==ENOENT) break;
  }
  /* we finally have a filename to which we can rotate the log. */
  rename(arg_log_file, new_log_file);
}

static FILE* get_log(time_t time, const char *time_str)
{
  /* if no log file was specified, log to stdout. */
  /* note that this means /dev/null unless --nodaemonize is set */
  if (arg_log_file==NULL) return stdout;

  if (log!=NULL && time-log_time>LOG_INTERVAL)
  {
    fclose(log);
    log = NULL;
    rotate(time_str);
  }
  
  /* if we have no log open, open one. */
  if (log==NULL)
  {
    rotate(time_str);
    log = fopen(arg_log_file,"a+");
  }
  
  /* if we still have no log open, use stdout. */
  return log==NULL ? stdout : log;
}

void log_msg(int level, const char* fname, int lineno, const char* format, ...)
{
  char buf[1024];
  va_list ap;
  char timestr[TIMESTAMP_LENGTH];
  time_t t;

  /* check for illegal logging level */
  if(level <= 0)
  {
    printf ("logging level <= 0\n");
    return;
  }

  /* check for ignored message logging level */
  if((level & arg_log_level) == 0)
  {
    return;
  }

  va_start(ap, format);
  vsnprintf(buf, sizeof(buf), format, ap);
  va_end(ap);

  t = time(NULL);
  if (strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", gmtime(&t))==0)
  {
    strncpy(timestr, "TIME ERROR", sizeof(timestr)-1);
  }
  fprintf(get_log(t,timestr), "%s %s:%d : %s", timestr, fname, lineno, buf);
  log_time = t;
}

void log_get_level_string(char* str, int len)
{
  *str = '\0';

  if ( LOG_MASK_ERROR & arg_log_level )
    {
      strncat(str, "ERROR ", len - strlen(str));
    }

  if ( LOG_MASK_WARNING & arg_log_level )
    {
      strncat(str, "WARNING ", len - strlen(str));
    }

  if ( LOG_MASK_INFO & arg_log_level )
    {
      strncat(str, "INFO ", len - strlen(str));
    }

  if ( LOG_MASK_PROGRESS & arg_log_level )
    {
      strncat(str, "PROGRESS ", len - strlen(str));
    }

  str[len - 1] = '\0';
}
