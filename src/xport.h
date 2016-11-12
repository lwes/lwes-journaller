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

#ifndef XPORT_DOT_H
#define XPORT_DOT_H

#include <stddef.h>
#include <stdio.h>

#define XPORT_INTR -2

struct xport;

struct xport_vtbl {
  void  (*destructor) (struct xport* this_xport);

  int   (*open)       (struct xport* this_xport, int flags);
  int   (*close)      (struct xport* this_xport);

  int   (*read)       (struct xport* this_xport, void* buf, size_t count,
                       unsigned long* addr, short* port);
  int   (*write)      (struct xport* this_xport, const void* buf, size_t count);
};

struct xport {
  struct xport_vtbl*    vtbl;
  void*                 priv;
};

int xport_factory(struct xport* this_xport, FILE *log);

#endif /* XPORT_DOT_H */
