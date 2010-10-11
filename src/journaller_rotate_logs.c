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
#include "config.h"

#include "header.h"
#include "log.h"
#include "opt.h"
#include "perror.h"
#include "xport.h"

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, const char* argv[])
{
  int xpt_write_ret;
  struct xport xpt;
  const char rotate_event[] = ROTATE_COMMAND "\0";

  process_options(argc, argv);
  arg_ttl = 3 ; /* Command::Rotate should not be any other ... */

  if ( (xport_factory(&xpt) < 0) || (xpt.vtbl->open(&xpt, O_RDONLY) < 0) )
    {
      LOG_ER("Failed to create xport object.\n");
      exit(EXIT_FAILURE);
    }

  if ( (xpt_write_ret = xpt.vtbl->write(&xpt,
                                        rotate_event,
                                        sizeof(rotate_event))) < 0 )
    {
      LOG_ER("Xport Command::Rotate write error.");
      xpt.vtbl->destructor(&xpt);
      exit(EXIT_FAILURE);
    }
  
  xpt.vtbl->destructor(&xpt);

  return 0;
}
