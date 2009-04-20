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

#ifndef JOURNAL_DOT_H
#define JOURNAL_DOT_H

#include <stddef.h>

/* Journal methods:
 *
 * open() -- opens a journal file, return 0 on success, -1 on error
 *
 * close() -- closes a journal file, return 0 on success, -1 on error
 *
 * read() -- reads "count" bytes from a journal into "buf", return the
 * number of bytes read on success, -1 on error
 *
 * write() -- writes "count" bytes from "buf" into a journal, return
 * the number of bytes written on success, -1 on error
 *
 */

struct journal;

struct journal_vtbl {
  void  (*destructor)   (struct journal* this_journal);

  int   (*open)         (struct journal* this_journal, int flags);
  int   (*close)        (struct journal* this_journal);

  int   (*read)         (struct journal* this_journal, void* buf, size_t count);
  int   (*write)        (struct journal* this_journal, void* buf, size_t count);
};

struct journal {
  struct journal_vtbl*  vtbl;
  void*                 priv;
};

int journal_factory(struct journal* jrn, const char* name);
typedef int (*lwes_journaller_journal_init_t)(struct journal*, const char*) ;

#endif /* JOURNAL_DOT_H */
