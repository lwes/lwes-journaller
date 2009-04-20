/* $Id: journal_gz.h,v 1.1.1.1 2004/05/20 21:21:03 hightowg Exp $
 */

#ifndef JOURNAL_GZ_DOT_H
#define JOURNAL_GZ_DOT_H

#define JOURNAL_GZ_EXT ".gz"

int journal_gz_ctor(struct journal* this_journal, const char* full_path_to_journal_file);

#endif /* JOURNAL_GZ_DOT_H */
