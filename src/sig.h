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

#ifndef SIG_DOT_H
#define SIG_DOT_H

extern volatile int gbl_done;
extern volatile int gbl_rotate_enqueue;
extern volatile int gbl_rotate_dequeue;
extern volatile int gbl_rotate_log;

extern void install_signal_handlers (void);
extern void install_rotate_signal_handlers (void);
extern void install_interval_rotate_handlers (int should_install_handler);

#endif /* SIG_DOT_H */
