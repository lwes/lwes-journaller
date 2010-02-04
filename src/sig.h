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

#ifndef SIG_DOT_H
#define SIG_DOT_H

extern volatile int gbl_done;
extern volatile int gbl_rotate;
typedef enum {
    PANIC_NOT,        // quiescent
    PANIC_STARTUP,    // signal to xport_to_queue to start panic
    PANIC_IN_EFFECT,  // panic mode is in effect
    PANIC_SHUTDOWN,   // signal to xport_to_queue that panic is done
    PANIC_HURRYUP     // hurry-up mode is in effect
} PANIC_MODE ;
extern volatile PANIC_MODE journaller_panic_mode ;

void install_signal_handlers ();
void install_rotate_signal_handlers ();

#endif /* SIG_DOT_H */
