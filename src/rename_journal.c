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

#include "config.h"

#include "rename_journal.h"

#include "log.h"
#include "sig.h"
#include "perror.h"
#include "opt.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>

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

  if ( chown(newpath, arg_journal_uid, -1) < 0 )
    {
      char buf[100] ;
      LOG_ER("rename: %s: - '%s' could not be granted to uid %d\n",
             strerror_r(errno,buf,sizeof(buf)), newpath, arg_journal_uid);
      return -1;
    }

  return 0;
}
