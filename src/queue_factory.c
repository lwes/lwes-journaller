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

#include "queue.h"

#include "queue_msg.h"
#include "queue_mqueue.h"

#include "log.h"
#include "opt.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h> // see http://www.tldp.org/HOWTO/C++-dlopen/index.html

int queue_factory (struct queue* this_queue)
{
  /* Create queue object. */
  if ( strcmp(arg_queue_type, ARG_MSG) == 0 )
    {
      if ( queue_msg_ctor(this_queue, arg_queue_name,
                          arg_queue_max_sz, arg_queue_max_cnt) < 0 )
        {
          LOG_ER("No SysV SHM support.\n");
          return -1;
        }
    }
  else if ( strcmp(arg_queue_type, ARG_MQ) == 0 )
    {
      if ( queue_mqueue_ctor(this_queue, arg_queue_name,
                             arg_queue_max_sz, arg_queue_max_cnt) < 0 )
        {
          LOG_ER("No POSIX mqueue support.\n");
          return -1;
        }
    }
  else
    {
      char pathname[256] ;
      char *err = NULL ;
      void* module = NULL ;
      strcpy(pathname, "/home/y/lib/liblwes-journaller-queue-") ;
      strcat(pathname, arg_queue_type) ;
      strcat(pathname, ".so") ;
      module = dlopen(pathname, RTLD_NOW) ;

      if ( module )
        {
          char symname[100] ;
          strcpy(symname, "lwes_journaller_queue_") ;
          strcat(symname, arg_queue_type) ;
          strcat(symname, "_LTX_init") ;
          lwes_journaller_queue_init_t init =
            (lwes_journaller_queue_init_t) dlsym(module, symname) ;
          if ( !dlerror() )
            {
              if ( (*init)(this_queue, arg_queue_name,
                           arg_queue_max_sz, arg_queue_max_cnt) < 0 )
                {
                  LOG_ER("Failed to create a dynamic queue method.\n") ;
                  return -1 ;
                }
              else
                {
                  return 0 ;
                }
            }
          else
            {
              err = dlerror() ;
            }
        }
      else
        {
          err = dlerror() ;
        }
      LOG_ER("Unrecognized queue type '%s', try \"" ARG_MSG "\" or "
             "\"" ARG_MQ "\".\n%s\n", arg_queue_type, err);
      return -1;
    }

  return 0;
}
