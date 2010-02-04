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

#ifndef XPORT_DOT_H
#define XPORT_DOT_H

#include <stddef.h>

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

int xport_factory(struct xport* this_xport);

#endif /* XPORT_DOT_H */
