/* $Id: journal.h,v 1.2 2007/07/03 08:23:03 woodg Exp $
 */

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
