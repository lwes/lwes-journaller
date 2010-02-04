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

#include "rename_journal.h"

#include "log.h"
#include "sig.h"
#include "perror.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <time.h>

int rename_journal(const char* path, time_t* last_rotate)
{
  char empty[1] = "";
  char* ext;
  char base[PATH_MAX];
  char newpath[PATH_MAX];
  char timebfr[100];        /* Needs to be big enough for strftime below */
  time_t now = time(NULL);  /* get current time */
  struct tm tm_now;

#ifndef WIN32 //TODO: someday figure out localtime_r() on WIN32, but not needed now
  if ( strftime(timebfr, sizeof(timebfr), "%Y%m%d%H%M%S",
                localtime_r(&now, &tm_now)) == 0 )
    {
      LOG_ER("strftime failed in rename_journal(\"%s\", ...)\n", path);
      return -1;
    }
#endif

  if ( 0 != (ext = strrchr(path, '.')) )
    {
      strncpy(base, path, ext - path);
      base[ext - path] = '\0';
    }
  else
    {
      strcpy(base, path);
      ext = empty;
    }

  if ( ! gbl_rotate ) // sink-ram rotate ?
    {
      ext = empty ;
    }

  snprintf(newpath, sizeof(newpath), "%s.%s.%ld.%ld%s",
           base, timebfr, *last_rotate, now, ext);
  *last_rotate = now;

  if ( gbl_rotate )
    {
      LOG_INF("Naming new journal file \"%s\".\n", newpath);
    }

  if ( rename(path, newpath) < 0 )
    {
      char buf[100] ;
      LOG_ER("rename: %s: - '%s' -> '%s'\n",
             strerror_r(errno,buf,sizeof(buf)), path, newpath);
      return -1;
    }

  return 0;
}
