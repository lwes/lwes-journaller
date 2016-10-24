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

#ifndef HEADER_DOT_H
#define HEADER_DOT_H

#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/*
 * The header added by the journaller is 22 bytes, defined as:
 *
 * uint16_t  Size of message body.
 * uint64_t  Receive time in msec.
 * uint32_t  Sender IP address.
 * uint16_t  Sender port number.
 * uint16_t  Site ID number
 * uint32_t  Reserved, set to zero.
 *
 * All fields are stored in "network byte order," meaning the most
 * significant byte appears first (big endian).  The header is added
 * at the "first touch" of the data by the journaller when it receives
 * a packet on the xport in xport_to_queue.
 *
 */
#define RECEIPT_TIME_OFFSET (2)
#define SENDER_IP_OFFSET    (RECEIPT_TIME_OFFSET+8)
#define SENDER_PORT_OFFSET  (SENDER_IP_OFFSET+4)
#define SITE_ID_OFFSET      (SENDER_PORT_OFFSET+2)
#define EXTENSION_OFFSET    (SITE_ID_OFFSET+2)
#define EVENT_TYPE_OFFSET   (EXTENSION_OFFSET+4)
#define HEADER_LENGTH       (EVENT_TYPE_OFFSET)

struct packet_check {
  long long received;
  char md5[16];
};

extern void header_add(void* buf, int count, unsigned long long tm, unsigned long addr, unsigned short port);
extern int  header_is_rotate(void* buf);
extern void header_fingerprint(void* buf, struct packet_check* pc);
extern int  toknam_eq(const unsigned char* toknam, const unsigned char* nam) ;

extern void header_print(const char* buf);

uint16_t    header_payload_length(const char* header);       /* Size of message body. */
uint64_t    header_receipt_time(const char* header);         /* Now in msec. */
const char* header_sender_ip_formatted(const char* header);  /* Sender IP address, formatted. do not free(). */
uint16_t    header_sender_port(const char* header);          /* Sender port number. */
uint16_t    header_site_id(const char* header);              /* Site ID number */

/* The character at the beginning of the string is the length byte.
   Strings in events are Pascal style. */
#define ROTATE_COMMAND    "\017Command::Rotate"

#endif /* HEADER_DOT_H */
