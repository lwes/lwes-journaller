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

#include "xport.h"
#include "xport_udp.h"

#include "log.h"
#include "opt.h"

#include <string.h>
#include <stdio.h>

int xport_factory(struct xport* this_xport)
{
  /* Only UDP transport supported. */
  if ( strcmp(arg_xport, "udp") != 0 ) {
    LOG_ER("unrecognized transport type \"%s\", try \"upd\"\n",
           arg_xport);
    return -1;
  }

  /* Since we currently have only one xport type: */
  return xport_udp_ctor(this_xport, arg_ip, arg_interface, arg_port, arg_join_group);
}
