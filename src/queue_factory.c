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

#include "queue.h"

#include "queue_msg.h"
#include "queue_mqueue.h"

#include "log.h"
#include "opt.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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
      else
        {
          LOG_INF("Using SysV MsgQ.\n");
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
      else
        {
          LOG_INF("Using POSIX mqueue.\n");
        }
    }
  else
    {
      LOG_ER("Unrecognized queue type '%s', try \"" ARG_MSG "\" or "
             "\"" ARG_MQ "\".\n", arg_queue_type);
      return -1;
    }

  return 0;
}
