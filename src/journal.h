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

#ifndef JOURNAL_DOT_H
#define JOURNAL_DOT_H

#include <stddef.h>
#include <stdio.h>

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
  void  (*destructor)   (struct journal* this_journal, FILE *log);

  int   (*open)         (struct journal* this_journal, int flags, FILE *log);
  int   (*close)        (struct journal* this_journal, FILE *log);

  int   (*read)         (struct journal* this_journal, void* buf, size_t count);
  int   (*write)        (struct journal* this_journal, void* buf, size_t count);
};

struct journal {
  struct journal_vtbl*  vtbl;
  void*                 priv;
};

int journal_factory(struct journal* jrn, const char* name, FILE *log);

#endif /* JOURNAL_DOT_H */
