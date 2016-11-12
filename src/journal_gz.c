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

#include "journal.h"
#include "journal_gz.h"

#include "rename_journal.h"
#include "log.h"
#include "opt.h"

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>

#if HAVE_SYS_STATVFS_H
#include <sys/statvfs.h>
#endif

#if HAVE_LIBZ

#include <zlib.h>

struct priv {
  char* path;
  gzFile fp;
  time_t ot;
  long long nbytes_written;
};

static void destructor(struct journal* this_journal, FILE *log)
{
  struct priv* ppriv;

  this_journal->vtbl->close(this_journal, log);

  ppriv = (struct priv*)this_journal->priv;
  free(ppriv->path);
  free(ppriv);

  this_journal->vtbl = 0;
  this_journal->priv = 0;
}

static int xopen(struct journal* this_journal, int flags, FILE *log)
{
  struct priv* ppriv = (struct priv*)this_journal->priv;
  const char* mode;
  struct stat stbuf;
  time_t epoch = 0; /* Crashed files may include data from the past. */

  /* If this journal is already open, return an error. */
  if ( ppriv->fp )
    return -1;

  switch ( flags ) {
  case O_RDONLY:
    mode = "rb";
    break;

  case O_WRONLY:
    mode = "wb";
    if ( 0 == stat(ppriv->path, &stbuf) ) {
      epoch = stbuf.st_ctime;
      if (stbuf.st_size > 0) rename_journal(ppriv->path, &epoch, log);
    }
    break;

  case O_RDWR:
    mode = "w+b";
    break;

  default:
    return -1;
  }

  ppriv->fp = gzopen(ppriv->path, mode);

  if ( ! ppriv->fp )
    return -1;

#if HAVE_SYS_STATVFS_H
  if ( flags == O_WRONLY )
    {
      struct statvfs stfsbuf;

      if ( -1 == statvfs(ppriv->path, &stfsbuf) )
        {
          LOG_WARN(log,"Unable to determine free space available for %s.\n",
                   ppriv->path);
        }
      else
        {
          /* Check free space. */

          /* tmpfs give f_bsize==0 */
          long bsize = (stfsbuf.f_bsize!=0) ? stfsbuf.f_bsize : 4096 ;
          long lsz = ppriv->nbytes_written / bsize ;

          if ( lsz > (abs(stfsbuf.f_bavail) / 2.) )
            {
              LOG_WARN(log,"Low on disk space for new gz log %s.\n",
                       ppriv->path);
              LOG_WARN(log,"Available space is %d blocks of %d bytes each.\n",
                       stfsbuf.f_bavail, bsize);
              LOG_WARN(log,"Last log file contained %lld bytes.\n",
                       ppriv->nbytes_written);
            }
        }
    }
#endif

  ppriv->ot = time(NULL);
  ppriv->nbytes_written = 0;

  return 0;
}

static int xclose(struct journal* this_journal, FILE *log)
{
  struct priv* ppriv = (struct priv*)this_journal->priv;

  if ( ! ppriv->fp )
    {
      return -1;
    }

  if ( gzclose(ppriv->fp) )
    {
      ppriv->fp = 0;
      return -1;
    }

  rename_journal(ppriv->path, &ppriv->ot, log);
  ppriv->fp = 0;
  return 0;
}

static int xread(struct journal* this_journal, void* ptr, size_t size)
{
  return gzread(((struct priv*)this_journal->priv)->fp, ptr, size);
}

static int xwrite(struct journal* this_journal, void* ptr, size_t size)
{
  struct priv* ppriv = (struct priv*)this_journal->priv;

  int ret = gzwrite(ppriv->fp, ptr, size);
  if ( ret > 0 )
    ppriv->nbytes_written += ret;
  return ret;
}

static int tailmatch(const char* str, const char* tail)
{
  size_t strsz = strlen(str);
  size_t tailsz = strlen(tail);

  if ( tailsz > strsz )
    return 0;

  return strcmp(str + (strsz - tailsz), tail) == 0;
}

int journal_gz_ctor(struct journal* this_journal, const char* path, FILE *log)
{
  static struct journal_vtbl vtbl = {
      destructor,
      xopen, xclose,
      xread, xwrite
  };

  struct priv* ppriv;

  this_journal->vtbl = 0;
  this_journal->priv = 0;

  if ( ! tailmatch(path, JOURNAL_GZ_EXT) )
    {
      LOG_WARN(log, "Compressed journal file (\"%s\") doesn't end with "
                     "expected extension (\"%s\").\n",
                     path, JOURNAL_GZ_EXT);
    }

  ppriv = (struct priv*)malloc(sizeof(struct priv));
  if ( 0 == ppriv )
    {
      LOG_ER(log, "Malloc failed attempting to allocate %d bytes.\n",
                   sizeof(*ppriv));
      return -1;
    }
  memset(ppriv, 0, sizeof(*ppriv));

  if ( 0 == (ppriv->path = strdup(path)) )
    {
      LOG_ER(log,"The strdup() function failed attempting to dup \"%s\".\n",
                  path);
      free(ppriv);
      return -1;
    }

  this_journal->vtbl = &vtbl;
  this_journal->priv = ppriv;

  return 0;
}

#else  /* if HAVE_LIBZ */

int journal_gz_ctor(struct journal* this_journal, const char* path, FILE *log)
{
  this_journal->vtbl = 0;
  this_journal->priv = 0;
  (void)path;  /* appease -Wall -Werror */

  return -1;
}

#endif /* if HAVE_LIBZ */
