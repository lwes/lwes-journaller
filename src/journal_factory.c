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

#include "journal.h"
#include "journal_gz.h"
#include "journal_file.h"

#include "log.h"
#include "opt.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h> // see http://www.tldp.org/HOWTO/C++-dlopen/index.html

int journal_factory(struct journal* jrn, const char* name)
{
  if ( strcmp(arg_journ_type, ARG_FILE) == 0 ) {
    if ( journal_file_ctor(jrn, name) < 0 ) {
      LOG_ER("Failed to create a plain file journal.\n");
      return -1;
    }
  } else if ( strcmp(arg_journ_type, ARG_GZ) == 0 ) {
    if ( journal_gz_ctor(jrn, name) < 0 ) {
      LOG_ER("Failed to create a GZ compressed journal.\n");
      return -1;
    }
  } else {
    char pathname[256] ;
    char *err = NULL ;
    void* module = NULL ;
// TODO: Change /home/y/lib
    strcpy(pathname, "/home/y/lib/liblwes-journaller-journal-") ;
    strcat(pathname, arg_journ_type) ;
    strcat(pathname, ".so") ;
    module = dlopen(pathname, RTLD_NOW) ;

    if ( module ) {
      char symname[100] ;
      strcpy(symname, "lwes_journaller_journal_") ;
      strcat(symname, arg_journ_type) ;
      strcat(symname, "_LTX_init") ;
      lwes_journaller_journal_init_t init = (lwes_journaller_journal_init_t) dlsym(module, symname) ;
      if ( !dlerror() ) {
        if ( (*init)(jrn, name) < 0 ) {
          LOG_ER("Failed to create a dynamic journal method.\n") ;
          return -1 ;
        } else {
          return 0 ;
        }
      } else {
        err = dlerror() ;
      }
    } else {
      err = dlerror() ;
    }

    LOG_ER("Unrecognized journal type \"%s\", try \"" ARG_FILE "\" or \"" ARG_GZ "\".\n%s\n",
        arg_journ_type, err);
    return -1;
  }

  return 0;
}
