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

#ifndef QUEUE_DOT_H
#define QUEUE_DOT_H

#include <stddef.h>

#define QUEUE_INTR -2

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
  struct queue_vtbl* vtbl;
  void*              priv;
};

int queue_factory(struct queue* this_queue);
typedef int (*lwes_journaller_queue_init_t)(struct queue*, const char*, size_t, size_t) ;

#endif /* QUEUE_DOT_H */
