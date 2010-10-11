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

void header_add(void* buf, int count, unsigned long addr, unsigned short port)
{
  unsigned char*     cp = (unsigned char*)buf;
  unsigned long long tm = time_in_milliseconds();

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
  if ( toknam_eq((unsigned char *)buf + HEADER_LENGTH,
                 (unsigned char *)ROTATE_COMMAND) )
    {
      LOG_PROG("Command::Rotate message received.\n");
      return 1;
    }

  return 0;
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
/* globals */
static struct lwes_emitter *emitter = NULL ;
static struct lwes_event_deserialize_tmp *dtmp = NULL;
#define JOURNALLER_PING_SENDER_IP_FIELD "SenderIP"
#define JOURNALLER_PING_RETURN_IP_FIELD "ReturnIP"
#define JOURNALLER_PING_RETURN_PORT_FIELD "ReturnPort"
#define JOURNALLER_PING_DEFAULT_PORT 64646

static int
journaller_ping_transport_send_pong (char *address, int port)
{
  LOG_PROG("Sending System::Pong to %s:%i\n", address, port);

  struct lwes_event *pong_event =
    lwes_event_create( (struct lwes_event_type_db *) NULL,
        (LWES_SHORT_STRING) JOURNALLER_PONG_EVENT_TYPE);
  if ( pong_event == NULL )
    {
      return 1 ;
    }

  lwes_emitter_emitto(address, NULL/*arg_interface*/, port, emitter, pong_event);
  lwes_event_destroy(pong_event);

  LOG_PROG("Sending System::Pong finished\n");
  return 0;
}

int
ping (void* buf, size_t bufsiz)
{
  unsigned short int return_port ;
  struct in_addr return_ip ; memset(&return_ip, 0, sizeof(return_ip)) ;
  struct lwes_event *ping_event ;
  char* evt = (char*)buf ;

  if ( arg_nopong )
    {
      return 1 ;
    }

  /* setup System::Pong's transport */
  if ( emitter == NULL )
    {
      emitter = lwes_emitter_create( (LWES_CONST_SHORT_STRING) arg_ip,
          (LWES_CONST_SHORT_STRING) NULL, //arg_interface,
          (LWES_U_INT_32) arg_port, 0, 60 );
    }
  if ( emitter == NULL )
    {
      return -1 ;
    }

  if ( dtmp == NULL )
    {
      dtmp = (struct lwes_event_deserialize_tmp *)
        malloc(sizeof(struct lwes_event_deserialize_tmp));
      if(dtmp == NULL)
        {
          return -1;
        }
    }

  ping_event = lwes_event_create_no_name(NULL);

  if ( ping_event != NULL )
    {

      lwes_event_from_bytes (ping_event,
                             (LWES_BYTE_P)&evt[HEADER_LENGTH],
                             bufsiz-HEADER_LENGTH, 0, dtmp);

      if( lwes_event_get_IP_ADDR (ping_event,
                                  JOURNALLER_PING_RETURN_IP_FIELD,
                                  &return_ip) != 0 )
        {
          if ( lwes_event_get_IP_ADDR(ping_event,
                                      JOURNALLER_PING_SENDER_IP_FIELD,
                                      &return_ip) != 0 )
            { // none-of-the-above, so use header's sender-ip
              char* xxx = (char*)&return_ip ;
              xxx[0] = evt[13] ;
              xxx[1] = evt[12] ;
              xxx[2] = evt[11] ;
              xxx[3] = evt[10] ;
            }
        }

      if( lwes_event_get_U_INT_16(ping_event,
                                  JOURNALLER_PING_RETURN_PORT_FIELD,
                                  &return_port) != 0 )
        {
          return_port = JOURNALLER_PING_DEFAULT_PORT;
        }

      lwes_event_get_U_INT_16(ping_event, JOURNALLER_PING_RETURN_PORT_FIELD,
                              &return_port);

      journaller_ping_transport_send_pong(inet_ntoa(return_ip), return_port);
      /* cleanup */
      lwes_event_destroy(ping_event);
    }

  /* we're done */
  return 0;
}

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
