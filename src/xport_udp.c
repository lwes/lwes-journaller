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

#include "xport.h"
#include "xport_udp.h"

#include "perror.h"
#include "opt.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

struct priv {
    int                   fd;

    struct in_addr        address;
    short                 port;    /* In network byte order. */
    char*                 iface;

    struct sockaddr_in    ip_addr;
    struct ip_mreq        mreq;

    int                   join_group;

    int                   bind;
    int                   joined_multi_group;
};


static void destructor (struct xport* this_xport)
{
  struct priv* ppriv = (struct priv*)this_xport->priv;

  free (ppriv->iface);
  free (ppriv);

  this_xport->vtbl = 0;
  this_xport->priv = 0;
}


static int xopen (struct xport* this_xport, int flags)
{
  char optval = arg_ttl ; // Configured TTL
  struct priv* ppriv = (struct priv*)this_xport->priv;
  (void)flags; /* appease -Wall -Werror */

  memset(&ppriv->ip_addr, 0, sizeof(ppriv->ip_addr));

  ppriv->ip_addr.sin_family = AF_INET;
  ppriv->ip_addr.sin_addr = ppriv->address;
  ppriv->ip_addr.sin_port = ppriv->port;

  ppriv->mreq.imr_multiaddr = ppriv->ip_addr.sin_addr;

  if ( ppriv->iface && ppriv->iface[0] )
    {
      ppriv->mreq.imr_interface.s_addr = inet_addr(ppriv->iface);
    }
  else
    {
      ppriv->mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    }

  if ( (ppriv->fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
    {
      PERROR("socket");
      return -1;
    }
  setsockopt(ppriv->fd, SOL_IP, IP_MULTICAST_TTL, &optval, 1) ;

  /* If the interface is specified, then we set the interface. */
  if ( ppriv->iface && ppriv->iface[0] )
    {
      struct in_addr addr;

      addr.s_addr = inet_addr(ppriv->iface);
      if ( setsockopt(ppriv->fd, IPPROTO_IP, IP_MULTICAST_IF,
                      &addr, sizeof(addr)) < 0 )
        {
          PERROR("can't set interface");
          return -1;
        }
    }

  return 0;
}

static int xclose (struct xport* this_xport)
{
  struct priv* ppriv = (struct priv*)this_xport->priv;

  if ( -1 == ppriv->fd )
    {
      LOG_ER("Close transport called with xport already closed.\n");
      return -1;
    }

  if ( ppriv->joined_multi_group )
    {
      /* remove ourselves from the multicast channel */
      if ( setsockopt(ppriv->fd, IPPROTO_IP, IP_DROP_MEMBERSHIP,
                      &ppriv->mreq, sizeof(ppriv->mreq)) < 0 )
        {
          PERROR("can't drop multicast group");
        }
    }

  close(ppriv->fd);
  ppriv->fd = -1;

  return 0;
}

static int xread (struct xport* this_xport, void* buf, size_t count,
                  unsigned long* addr, short* port)
{
  struct priv* ppriv = (struct priv*)this_xport->priv;
  int recvfrom_ret;

  struct sockaddr_in sender_ip_addr;
  socklen_t          sender_ip_socket_size;
  struct sockaddr_in ip_addr;

  /* Set the size for use below: */
  sender_ip_socket_size = (socklen_t)sizeof(sender_ip_addr);

  /* This should be done only with multi-cast addresses. */
  if ( ! ppriv->bind )
    {
      int on = 1;

      LOG_PROG("About to join multi-cast group.\n");

      /* Set address for reuse. */
      if ( setsockopt(ppriv->fd, SOL_SOCKET, SO_REUSEADDR,
                      &on, sizeof(on)) < 0 )
        {
          PERROR("setsockopt -- can't reuse address\n");
          return -1;
        }

      /* Set the receive buffer size. */
      if ( arg_sockbuffer )
        {
          if ( setsockopt(ppriv->fd, SOL_SOCKET, SO_RCVBUF,
                          (char*)&arg_sockbuffer, sizeof(arg_sockbuffer)) < 0 )
            {
              PERROR("Setsockopt:SO_RCVBUF");
            }
          /* We'll try to continue with the default buffer size. */
        }

      /* Bind the socket to the port. */
      memset(&ip_addr, 0, sizeof(ip_addr));
      ip_addr.sin_family = AF_INET;
      ip_addr.sin_addr.s_addr = htonl(INADDR_ANY);
      ip_addr.sin_port = ppriv->port;

      if ( bind(ppriv->fd, (struct sockaddr*)&ip_addr, sizeof(ip_addr) ) < 0 )
        {
          PERROR("can't bind to local address\n");
          return -1;
        }
      ppriv->bind = 1;

      if ( ppriv->join_group && !ppriv->joined_multi_group )
        {
          /* add the multicast channel given */
          if ( setsockopt(ppriv->fd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                          &ppriv->mreq, sizeof(ppriv->mreq)) < 0 )
            {
              PERROR("setsockopt can't add to multicast group\n");
              return -1;
            }
          ppriv->joined_multi_group = 1;
        }
    }

  LOG_PROG("about to call recvfrom().\n");

  if ( (recvfrom_ret = recvfrom(ppriv->fd, buf, count, 0, 
                                (struct sockaddr*)&sender_ip_addr,
                                &sender_ip_socket_size)) < 0 )
    {
      switch ( errno )
        {
          default:
            PERROR("recvfrom");

          case EINTR:			/* Quiet return on interrupt. */
            break;
        }
    }

  *addr = sender_ip_addr.sin_addr.s_addr;
  *port = ntohs(sender_ip_addr.sin_port);

  return recvfrom_ret;
}


static int xwrite (struct xport* this_xport, const void* buf, size_t count)
{
  struct priv* ppriv = (struct priv*)this_xport->priv;

  LOG_PROG("about to call sendto().\n");

  return sendto (ppriv->fd, buf, count, 0,
                 (struct sockaddr*)&ppriv->ip_addr, sizeof(ppriv->ip_addr));
}


int xport_udp_ctor (struct xport* this_xport,
                    const char*   address,
                    const char*   iface,
                    short         port,
                    int           join)
{
  static struct xport_vtbl vtbl = {
      destructor,

      xopen, xclose,
      xread, xwrite
  };

  struct priv* ppriv;

  this_xport->vtbl = 0;
  this_xport->priv = 0;

  ppriv = (struct priv*)malloc(sizeof(*ppriv));
  if ( 0 == ppriv )
    {
      LOG_ER("Failed to allocate %d bytes for xport data.\n", sizeof(*ppriv));
      return -1;
    }
  memset(ppriv, 0, sizeof(*ppriv));

  ppriv->fd = -1;
  if ( 0 == inet_aton(address, &ppriv->address) )
    {
      LOG_ER("invalid address \"%s\"\n", address);
      free(ppriv);
      return -1;
    }

  ppriv->port = htons(port);

  ppriv->iface = strdup(iface);
  if ( 0 == ppriv->iface )
    {
      LOG_ER("strdup failed attempting to allocate %d bytes\n", strlen(iface));
      free(ppriv);
      return -1;
    }

  ppriv->join_group = join;

  this_xport->vtbl = &vtbl;
  this_xport->priv = ppriv;

  return 0;
}
