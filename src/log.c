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
#include "log.h"
#include "opt.h"

void log_msg(int level, const char* fname, int lineno, const char* format, ...) {
  char buf[1024];
  va_list ap;

  if(level <= 0)
  {
    printf ("logging level <= 0\n");
    return;
  }

  va_start(ap, format);
  vsnprintf(buf, sizeof(buf), format, ap);
  va_end(ap);

  fprintf(stdout, "%s:%d : %s", fname, lineno, buf);
}

void log_get_level_string(char* str, int len) {
  *str = '\0';

  if ( LOG_MASK_ERROR & arg_log_level )
    strncat(str, "ERROR ", len - strlen(str));

  if ( LOG_MASK_WARNING & arg_log_level )
    strncat(str, "WARNING ", len - strlen(str));

  if ( LOG_MASK_INFO & arg_log_level )
    strncat(str, "INFO ", len - strlen(str));

  if ( LOG_MASK_PROGRESS & arg_log_level )
    strncat(str, "PROGRESS ", len - strlen(str));

  str[len - 1] = '\0';
}

