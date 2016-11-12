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

#include <signal.h>
#include <stdio.h>

extern volatile sig_atomic_t gbl_done;
extern volatile sig_atomic_t gbl_rotate_enqueue;
extern volatile sig_atomic_t gbl_rotate_dequeue;
extern volatile sig_atomic_t gbl_rotate_main_log;
extern volatile sig_atomic_t gbl_rotate_enqueue_log;
extern volatile sig_atomic_t gbl_rotate_dequeue_log;

extern void install_termination_signal_handlers (FILE *log);
extern void install_rotate_signal_handlers (FILE *log);
extern void install_log_rotate_signal_handlers (FILE *log, int is_main, int sig);
extern void install_interval_rotate_handlers (FILE *log, int should_install_handler);

#define CAS_ON(var) __sync_bool_compare_and_swap(&var,0,1);
#define CAS_OFF(var) __sync_bool_compare_and_swap(&var,1,0);

#endif /* SIG_DOT_H */
