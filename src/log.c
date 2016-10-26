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
#include "sig.h"

static FILE* log = NULL;

static FILE* get_log()
{
  /* if no log file was specified, log to stdout. */
  /* note that this means /dev/null unless --nodaemonize is set */
  if (arg_log_file==NULL) return stdout;

  /* if a log file rotation has been requested, close the current handle. */
  /* it is probably the case that logrotate(8) has moved it to another name. */
  if (gbl_rotate_log && log)
  {
    fclose(log);
    log = NULL;
    gbl_rotate_log = 0;
  }

  /* if we have no log open now, open one. */
  if (log==NULL)
  {
    log = fopen(arg_log_file,"a+");
  }

  /* if we still have no log open, use stdout. */
  return log==NULL ? stdout : log;
}

static int is_logging(log_level_t level)
{
  return !!((1 << level) & arg_log_level);
}

static const char* log_get_level_string(int level)
{
  switch (level)
  {
    case LOG_OFF:
      return "OFF";
    case LOG_ERROR:
      return "ERR";
    case LOG_WARNING:
      return "WARN";
    case LOG_INFO:
      return "INFO";
    case LOG_PROGRESS:
      return "PROG";
    default:
      return "OTHER";
  }
}

void log_msg(log_level_t level, const char* fname, int lineno, const char* format, ...)
{
  char buf[1024];
  va_list ap;
  FILE* log;
  char timestr[100];
  time_t t;

  /* check for illegal logging level */
  if(level <= 0 || level > LOG_PROGRESS)
  {
    printf ("logging level was %d\n", level);
    return;
  }

  /* check for ignored message logging level */
  if(! is_logging(level)) return;

  va_start(ap, format);
  vsnprintf(buf, sizeof(buf), format, ap);
  va_end(ap);

  t = time(NULL);
  if (strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", gmtime(&t))==0)
  {
    strncpy(timestr, "TIME ERROR", sizeof(timestr)-1);
  }
  log = get_log();
  fprintf(log, "%s %s %s:%d : %s", timestr, log_get_level_string(level), fname, lineno, buf);
  fflush(log);
}

static void log_append_masked_level(char* str, int len, int level)
{
  if (!is_logging(level)) return;
  strncat(str, log_get_level_string(level), len - strlen(str));
  strncat(str, " ", len - strlen(str));
}

void log_get_mask_string(char* str, int len)
{
  *str = '\0';

  log_append_masked_level(str, len, LOG_ERROR);
  log_append_masked_level(str, len, LOG_WARNING);
  log_append_masked_level(str, len, LOG_INFO);
  log_append_masked_level(str, len, LOG_PROGRESS);

  if (len>0) str[len - 1] = '\0';
}
