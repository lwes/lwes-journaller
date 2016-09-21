/*======================================================================*
 * Copyright (c) 2008, Yahoo! Inc. All rights reserved.                 *
 *                                                                      *
 * Licensed under the New BSD License (the "License"); you may not use  *
 * this file except in compliance with the License.  Unless required    *
 * by applicable law or agreed to in writing, software distributed      *
 * under the License is distributed on an "AS IS" BASIS, WITHOUT        *
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.     *
 * See the License for the specific language governing permissions and  *
 * limitations under the License. See accompanying LICENSE file.        *
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

int journal_factory(struct journal* jrn, const char* name)
{
  if ( strcmp(arg_journ_type, ARG_FILE) == 0 )
    {
      if ( journal_file_ctor(jrn, name) < 0 )
        {
          LOG_ER("Failed to create a plain file journal.\n");
          return -1;
        }
    }
  else if ( strcmp(arg_journ_type, ARG_GZ) == 0 )
    {
      if ( journal_gz_ctor(jrn, name) < 0 )
        {
          LOG_ER("Failed to create a GZ compressed journal.\n");
          return -1;
        }
    }
  else
    {
      LOG_ER("Unrecognized journal type \"%s\", try \""
             ARG_FILE "\" or \"" ARG_GZ "\".\n",
             arg_journ_type);
      return -1;
    }

  return 0;
}
