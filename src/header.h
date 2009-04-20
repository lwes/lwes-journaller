/* $Id: header.h,v 1.14 2008/02/23 02:15:45 woodg Exp $
 */

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
  uint16_t payload_length ; /* Size of message body. */
  uint64_t receipt_time ;	  /* Now in msec. */
  uint32_t sender_ip ;      /* Sender IP address. */
  uint16_t sender_port ;    /* Sender port number. */
  uint16_t site_id ;        /* Site ID number */
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
extern int	non_revenue_bearing (const unsigned char* buf) ;

/* The character at the beginning of the string is the length byte.
   Strings in events are Pascal style. */
#define ROTATE_COMMAND    "\017Command::Rotate"

#define JOURNALLER_PING_EVENT_TYPE "\014System::Ping"
#define JOURNALLER_PONG_EVENT_TYPE "\014System::Pong"
#define JOURNALLER_PING_SENDER_IP_FIELD "SenderIP"
#define JOURNALLER_PING_RETURN_PORT_FIELD "ReturnPort"

// Induce Journaller Panic Mode
#define JOURNALLER_PANIC "\021Journaller::Panic"
#define JOURNALLER_HURRYUP "\021Journaller::Hurryup"
#define JOURNALLER_CM_SERVE "\011CM::Serve"
#define JOURNALLER_DM_SERVE "\011DM::Serve"
#define JOURNALLER_SS_SERVE "\011SS::Serve"

#endif /* HEADER_DOT_H */
