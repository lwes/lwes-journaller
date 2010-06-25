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

#ifndef HEADER_DOT_H
#define HEADER_DOT_H

#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/*
 * The header added by the journaller is 22 bytes, defined as:
 *
 * short      Size of message body.
 * long long  Receive time in msec.
 * long       Sender IP address.
 * short      Sender port number.
 * short      Site ID number
 * long       Reserved, set to zero.
 *
 * All fields are stored in "network byte order," meaning the most
 * significant byte appears first (big endian).  The header is added
 * at the "first touch" of the data by the journaller when it receives
 * a packet on the xport in xport_to_queue.
 *
 */

struct event_header {
/* note: datatypes may be wrong (should be big-endian), 
         but are of the correct sizes. */
  uint16_t payload_length ;   /* Size of message body. */
  uint64_t receipt_time ;     /* Now in msec. */
  uint32_t sender_ip ;        /* Sender IP address. */
  uint16_t sender_port ;      /* Sender port number. */
  uint16_t site_id ;          /* Site ID number */
  uint32_t future_extention ; /* reserved */
  unsigned char event_type[0] ; /* begin of event-type token */
} ;
#define HEADER_LENGTH (22)
#define RECEIPT_TIME   (2)

struct packet_check {
  long long received;
  char md5[16];
};

extern void header_add(void* buf, int count, unsigned long addr, unsigned short port);
extern int  header_is_rotate(void* buf, time_t* when);
extern void header_fingerprint(void* buf, struct packet_check* pc);
extern int  toknam_eq(const unsigned char* toknam, const unsigned char* nam) ;
extern int ping (void* buf, size_t bufsiz);


/* The character at the beginning of the string is the length byte.
   Strings in events are Pascal style. */
#define ROTATE_COMMAND    "\017Command::Rotate"

#define JOURNALLER_PING_EVENT_TYPE "\014System::Ping"
#define JOURNALLER_PONG_EVENT_TYPE "\014System::Pong"
#define JOURNALLER_PING_SENDER_IP_FIELD "SenderIP"
#define JOURNALLER_PING_RETURN_PORT_FIELD "ReturnPort"

#endif /* HEADER_DOT_H */
