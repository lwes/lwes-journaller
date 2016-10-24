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
#include <lwes.h>

struct ppriv {
  char *address;
  short port;
  char *iface;

  struct lwes_net_connection conn;
};

static void destructor (struct xport* this_xport)
{
  struct ppriv* ppriv = (struct ppriv*)this_xport->priv;

  if ( ppriv->address != NULL )
    {
      free (ppriv->address);
    }
  if ( ppriv->iface != NULL )
    {
      free (ppriv->iface);
    }

  free (ppriv);
  this_xport->vtbl = 0;
  this_xport->priv = 0;
}

static int xopen (struct xport* this_xport, int flags)
{
  struct ppriv* ppriv=
    (struct ppriv *)this_xport->priv;
  (void)flags; /* appease -Wall -Werror */

  if (lwes_net_open (&ppriv->conn, ppriv->address,
                     ppriv->iface, ppriv->port) < 0 )
    {
      return -1;
    }
  if (lwes_net_set_ttl (&ppriv->conn, arg_ttl) < 0)
    {
      return -2;
    }

  return 0;
}

static int xclose (struct xport* this_xport)
{
  struct ppriv* ppriv=
    (struct ppriv *)this_xport->priv;

  return lwes_net_close (&ppriv->conn);
}

static int xread (struct xport* this_xport, void* buf, size_t count,
                  unsigned long* addr, short* port)
{
  struct ppriv* ppriv=
    (struct ppriv *)this_xport->priv;
  int recvfrom_ret;

  if ( (recvfrom_ret =
          lwes_net_recv_bytes_by (&ppriv->conn, buf, count,
                                  arg_wakup_interval_ms)) < 0 )
    {
      switch (recvfrom_ret)
        {
          case -2:
            recvfrom_ret = XPORT_INTR;
            break;
        }
    }

  /* FIXME: provide interface in lwes_net_connection for this */
  *addr = ppriv->conn.sender_ip_addr.sin_addr.s_addr;
  *port = ntohs(ppriv->conn.sender_ip_addr.sin_port);

  return recvfrom_ret;
}


static int xwrite (struct xport* this_xport, const void* buf, size_t count)
{
  struct ppriv* ppriv=
    (struct ppriv *)this_xport->priv;

  return lwes_net_send_bytes (&ppriv->conn, (LWES_BYTE_P)buf, count);
}

int xport_udp_ctor (struct xport* this_xport,
                    const char*   address,
                    const char*   iface,
                    short         port)
{
  static struct xport_vtbl vtbl = {
      destructor,

      xopen, xclose,
      xread, xwrite
  };

  struct ppriv* ppriv;

  this_xport->vtbl = 0;
  this_xport->priv = 0;

  ppriv = (struct ppriv*)malloc(sizeof(*ppriv));
  if ( 0 == ppriv )
    {
      LOG_ER("Failed to allocate %d bytes for xport data.\n", sizeof(*ppriv));
      return -1;
    }
  memset(ppriv, 0, sizeof(*ppriv));

  ppriv->address = (address != NULL ? strdup(address) : NULL);
  ppriv->port = port;
  ppriv->iface = (iface != NULL ? strdup(iface) : NULL);

  this_xport->vtbl = &vtbl;
  this_xport->priv = ppriv;

  return 0;
}
