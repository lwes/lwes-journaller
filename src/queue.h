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

#ifndef QUEUE_DOT_H
#define QUEUE_DOT_H

#include <stddef.h>

/* Queue methods:
 *
 * open() -- opens a queue, return 0 on success, -1 on error
 *
 * close() -- closes a queue, return 0 on success, -1 on error
 *
 * read() -- reads "count" bytes from a queue into "buf", return the
 * number of bytes read on success, -1 on error
 *
 * write() -- writes "count" bytes from "buf" to a queue, return the
 * number of bytes written on success, -1 on error
 *
 * alloc() -- return a buffer suitable for use with read and write
 *
 * dealloc() -- free a buffer returned by alloc().
 *
 */

struct queue;

struct queue_vtbl {
  void  (*destructor)   (struct queue* this_queue);

  int   (*open)         (struct queue* this_queue, int flags);
  int   (*close)        (struct queue* this_queue);

  int   (*read)         (struct queue* this_queue, void* buf, size_t count, int* pending);
  int   (*write)        (struct queue* this_queue, const void* buf, size_t count);

  void* (*alloc)        (struct queue* this_queue, size_t* newcount);
  void  (*dealloc)      (struct queue* this_queue, void* buf);
};

struct queue {
  struct queue_vtbl*	vtbl;
  void*			priv;
};

int queue_factory(struct queue* this_queue);
typedef int (*lwes_journaller_queue_init_t)(struct queue*, const char*, size_t, size_t) ;

#endif /* QUEUE_DOT_H */
