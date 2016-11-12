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

#ifndef QUEUE_MSG_DOT_H
#define QUEUE_MSG_DOT_H

#include <stdio.h>

int queue_msg_ctor (struct queue* this_queue,
                    const char*   path,
                    size_t        max_sz,
                    size_t        max_cnt,
                    FILE *        log);

#endif /* QUEUE_MSG_DOT_H */
