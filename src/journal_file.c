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

#include "journal.h"
#include "journal_file.h"

#include "rename_journal.h"
#include "log.h"
#include "opt.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>

#if HAVE_SYS_STATVFS_H
#include <sys/statvfs.h>
#endif

struct priv {
  char* path;
  FILE* fp;
  time_t ot;
  long long nbytes_written;
};

static void destructor(struct journal* this_journal)
{
  struct priv* ppriv;

  this_journal->vtbl->close(this_journal);

  ppriv = (struct priv*)this_journal->priv;

  free(ppriv->path);
  free(ppriv);

  this_journal->vtbl = 0;
  this_journal->priv = 0;
}

static int xopen(struct journal* this_journal, int flags)
{
  struct priv* ppriv = (struct priv*)this_journal->priv;
  const char* mode;
  struct stat buf;
  time_t epoch = 0; /* Crashed files may include data from the past. */

  /* If this journal is already open, return an error. */
  if ( ppriv->fp )
    {
      return -1;
    }

  switch ( flags )
    {
      case O_RDONLY:
        mode = "rb";
        break;

      case O_WRONLY:
        mode = "wb";
        if ( 0 == stat(ppriv->path, &buf) )
          {
            rename_journal(ppriv->path, &epoch);
          }
        break;

      case O_RDWR:
        mode = "w+b";
        break;

      default:
        return -1;
    }

  ppriv->fp = fopen(ppriv->path, mode);

  if ( ! ppriv->fp )
    {
      return -1;
    }

#if HAVE_SYS_STATVFS_H
  if ( flags == O_WRONLY )
    {
      struct statvfs stfsbuf;

      if ( -1 == statvfs(ppriv->path, &stfsbuf) )
        {
          LOG_WARN("Unable to determine free space available for %s.\n",
                   ppriv->path);
        }
      else
        {
          /* Check free space. */
          /* tmpfs give f_bsize==0 */
          long bsize = (stfsbuf.f_bsize!=0) ? stfsbuf.f_bsize : 4096 ;
          long lsz = ppriv->nbytes_written / bsize ;

          if ( lsz > (stfsbuf.f_bavail / 2.) )
            {
              LOG_WARN("Low on disk space for new log %s.\n", ppriv->path);
              LOG_WARN("Available space is %d blocks of %d bytes each.\n", stfsbuf.f_bavail, bsize);
              LOG_WARN("Last log file contained %lld bytes.\n", ppriv->nbytes_written);
            }
        }
    }
#endif

  ppriv->ot = time(NULL);
  ppriv->nbytes_written = 0;

  return 0;
}

static int xclose(struct journal* this_journal)
{
  struct priv* ppriv = (struct priv*)this_journal->priv;

  if ( ! ppriv->fp )
    {
      return -1;
    }

  if ( fclose(ppriv->fp) )
    {
      ppriv->fp = 0;
      return -1;
    }

  rename_journal(ppriv->path, &ppriv->ot);
  ppriv->fp = 0;
  return 0;
}

static int xread(struct journal* this_journal, void* ptr, size_t size)
{
  size_t ret = fread(ptr, size, 1, ((struct priv*)this_journal->priv)->fp);
  return (int)ret * size;
}

static int xwrite(struct journal* this_journal, void* ptr, size_t size)
{
  struct priv* ppriv = (struct priv*)this_journal->priv;

  size_t ret = fwrite(ptr, size, 1, ((struct priv*)this_journal->priv)->fp);
  ppriv->nbytes_written += ret * size;
  return (int)ret * size;
}

int journal_file_ctor(struct journal* this_journal, const char* path)
{
  static struct journal_vtbl vtbl = {
    destructor,
    xopen, xclose,
    xread, xwrite
  };

  struct priv* ppriv;

  this_journal->vtbl = 0;
  this_journal->priv = 0;

  ppriv = (struct priv*)malloc(sizeof(struct priv));
  if ( 0 == ppriv )
    {
      LOG_ER("Failed to allocate %d bytes for journal data.\n",
          sizeof(*ppriv));
      return -1;
    }
  memset(ppriv, 0, sizeof(*ppriv));

  if ( 0 == (ppriv->path = strdup(path)) )
    {
      free(ppriv);
      return -1;
    }

  this_journal->vtbl = &vtbl;
  this_journal->priv = ppriv;

  return 0;
}
