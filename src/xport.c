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

#include "xport.h"
#include "xport_udp.h"

#include "log.h"
#include "opt.h"

#include <string.h>
#include <stdio.h>

int xport_factory(struct xport* this_xport)
{
  /* Only UDP transport supported. */
  if ( strcmp(arg_xport, "udp") != 0 )
    {
      LOG_ER("unrecognized transport type \"%s\", try \"upd\"\n",
             arg_xport);
      return -1;
    }

  /* Since we currently have only one xport type: */
  return xport_udp_ctor (this_xport, arg_ip,
                         arg_interface, arg_port, arg_join_group);
}
