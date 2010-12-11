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
