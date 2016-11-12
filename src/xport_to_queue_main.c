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

#include "config.h"

#include "opt.h"
#include "sig.h"
#include "log.h"
#include "xport_to_queue.h"
#include <stdlib.h>
#include <stdio.h>

int main(int argc, const char* argv[])
{
  FILE *log = get_log(NULL);

  switch (process_options(argc, argv, log))
    {
      case 0:
        break;
      case -1:
        exit(EXIT_SUCCESS);
      default:
        exit(EXIT_FAILURE);
    }

  install_termination_signal_handlers (log);
  install_rotate_signal_handlers (log);
  install_log_rotate_signal_handlers (log, 0, SIGUSR1);

  int r = xport_to_queue (log);

  close_log (log);

  return r;
}
