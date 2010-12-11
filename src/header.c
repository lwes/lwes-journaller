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
#include "stats.h"

#include "lwes.h"
#include "marshal.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define ntohll(x) ( ( (uint64_t)(ntohl( (uint32_t)((x << 32) >> 32) )) << 32) | ntohl( ((uint32_t)(x >> 32)) ) )

void header_add(void* buf, int count, unsigned long long tm, unsigned long addr, unsigned short port)
{
  unsigned char*     cp = (unsigned char*)buf;

  marshal_short(cp, count);    /* Size of message body. */
  marshal_ulong_long(cp, tm);  /* Now in msec. */
  marshal_long(cp, addr);      /* Sender IP address. */
  marshal_short(cp, port);     /* Sender port number. */
  marshal_short(cp, arg_site); /* Site ID number */
  /* TODO: should be perfectly alright, but un-QA'd  marshal_long(cp, 0); */
  /* reserved */
}

int header_is_rotate (void* buf)
{
  return toknam_eq((unsigned char *)buf + HEADER_LENGTH,
                   (unsigned char *)ROTATE_COMMAND);
  LOG_PROG("Command::Rotate message received.\n");
}

void header_fingerprint(void* buf, struct packet_check* pc)
{
  unsigned char* cp = (unsigned char*)buf;
  long long zero = 0LL;
  unsigned long long tm;
  short length;

  /* Extract the timestamp. */
  unmarshal_short(cp, length);
  unmarshal_ulong_long(cp, tm);

  /* Add phony timestamp of zero. */
  cp -= 8;
  marshal_long_long(cp, zero);

  pc->received = tm;
  /* pc->md5 = md5(); */
}

/////////////////////////////////////////////////////////////////////////////
// It's a small thing, but toknam_eq() is quicker than memcmp().
//   * memcmp does a tremendous amount of setup before comparing the first
//     character (it's designed to compare large strings efficently)
//   * we don't care if greater-than or less-than, just equal or not
//   * syntactically simpler (taking advantage of token's length byte)
int toknam_eq(const unsigned char* toknam, const unsigned char* nam)
{
  unsigned char len = *nam+1 ;
  while ( *toknam++ == *nam++ )
    if ( --len == 0 )
      return 1 ;
  return 0 ;
}

/////////////////////////////////////////////////////////////////////////////

void header_print (const char* buf)
{
  LOG_INF("header payload length: %u\n",   header_payload_length(buf));
  LOG_INF("receipt time:          %llu\n", header_receipt_time(buf));
  LOG_INF("now:                   %llu\n", time(NULL)*1000L);
  LOG_INF("sender IP text:        %s\n",   header_sender_ip_formatted(buf));
  LOG_INF("sender port:           %u\n",   header_sender_port(buf));
  LOG_INF("site id:               %u\n",   header_site_id(buf));
}

static uint16_t header_uint16(const char* bytes) {
  return ntohs(*((uint16_t*) bytes));
}

static uint32_t header_uint32(const char* bytes) {
  return ntohl(*((uint32_t*) bytes));
}

static uint64_t header_uint64(const char* bytes) {
  return (((uint64_t)header_uint32(bytes))<<32) | header_uint32(bytes+4);
}

uint16_t header_payload_length(const char* header)
{
  return header_uint16(header);
}

uint64_t header_receipt_time(const char* header) {
  return header_uint64(header+RECEIPT_TIME_OFFSET);
}

const char* header_sender_ip_formatted(const char* header) {
  struct in_addr addr = { header_uint32(header+SENDER_IP_OFFSET) };
  return inet_ntoa(addr);
}

uint16_t header_sender_port(const char* header) {
  return header_uint16(header+SENDER_PORT_OFFSET);
}

uint16_t header_site_id(const char* header) {
  return header_uint16(header+SITE_ID_OFFSET);
}

/* end-of-file */
