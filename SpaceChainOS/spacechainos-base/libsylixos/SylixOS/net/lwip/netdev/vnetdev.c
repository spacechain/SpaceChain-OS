/**
 * @file
 * virtual net device driver.
 * as much as possible compatible with different versions of LwIP
 * Verification using sylixos(tm) real-time operating system
 */

/*
 * Copyright (c) 2006-2017 SylixOS Group.
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 * 4. This code has been or is applying for intellectual property protection 
 *    and can only be used with acoinfo software products.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 * 
 * Author: Han.hui <hanhui@acoinfo.com>
 *
 */

#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"

#if LW_CFG_NET_VNETDEV_EN > 0

#include "lwip/mem.h"
#include "lwip/netif.h"
#include "netdev.h"
#include "vnetdev.h"

/* virtual netdev pbuf free hook */
static void vnetdev_pbuf_free (struct pbuf *p)
{
  struct vnd_q *vndq = _LIST_ENTRY(p, struct vnd_q, p);
  
  mem_free(vndq);
}

/* virtual netdev pbuf alloc */
static struct vnd_q *vnetdev_pbuf_alloc (struct pbuf *p)
{
  struct vnd_q *vndq;
  struct pbuf *ret;
  u16_t reserve = ETH_PAD_SIZE + SIZEOF_VLAN_HDR;
  u16_t tot_len = (u16_t)(reserve + p->tot_len);
  
  vndq = (struct vnd_q *)mem_malloc(ROUND_UP(sizeof(struct vnd_q), MEM_ALIGNMENT) + tot_len);
  if (vndq == NULL) {
    return (NULL);
  }
  
  vndq->p.custom_free_function = vnetdev_pbuf_free;
  
  ret = pbuf_alloced_custom(PBUF_RAW, tot_len, PBUF_POOL, &vndq->p,
                            (char *)vndq + ROUND_UP(sizeof(struct vnd_q), MEM_ALIGNMENT), 
                            tot_len);
  if (ret) {
    pbuf_header(ret, (u16_t)-reserve);
    pbuf_copy(ret, p);
  }
  
  return (vndq);
}

/* virtual netdev functions: ioctl */
static int vnetdev_ioctl (struct netdev *netdev, int cmd, void *arg)
{
  struct ifreq *pifreq = (struct ifreq *)arg;

  if (cmd == SIOCSIFMTU) {
    if (pifreq && 
        (pifreq->ifr_mtu >= VNETDEV_MTU_MIN) &&
        (pifreq->ifr_mtu <= VNETDEV_MTU_MAX)) {
      netdev->mtu = pifreq->ifr_mtu;
      return (0);
    }
  }
  
  return (-1);
}

/* virtual netdev functions: transmit */
static int vnetdev_transmit (struct netdev *netdev, struct pbuf *p)
{
  struct vnetdev *vnetdev = (struct vnetdev *)netdev;
  struct netif *netif = (struct netif *)netdev->sys;
  struct vnd_q *vndq;
  
  if (!netif_is_link_up(netif)) {
    return (-1);
  }
  
  if ((vnetdev->cur_size + p->tot_len) > vnetdev->buf_size) {
error:
    netdev_linkinfo_drop_inc(netdev);
    netdev_statinfo_discards_inc(netdev, LINK_OUTPUT);
    return (-1);
  }
  
  vndq = vnetdev_pbuf_alloc(p);
  if (vndq == NULL) {
    goto error;
  }
  
  _List_Ring_Add_Ahead(&vndq->ring, &vnetdev->q); /* put to queue */
  vnetdev->cur_size += p->tot_len;
  
  netdev_linkinfo_xmit_inc(netdev);
  netdev_statinfo_total_add(netdev, LINK_OUTPUT, p->tot_len);
  if (((UINT8 *)p->payload)[0] & 1) {
    netdev_statinfo_mcasts_inc(netdev, LINK_OUTPUT);
  } else {
    netdev_statinfo_ucasts_inc(netdev, LINK_OUTPUT);
  }
  
  vnetdev->notify(vnetdev);
  return (0);
}

/* virtual netdev functions: receive */
static void vnetdev_receive (struct netdev *netdev, int (*input)(struct netdev *, struct pbuf *))
{
  _BugHandle(TRUE, TRUE, "Bug in here!\r\n");
}

/* create a virtual netdev */
int vnetdev_add (struct vnetdev *vnetdev, vndnotify notify, size_t bsize, int id, int type, void *priv)
{
  static struct netdev_funcs vnetdev_funcs = {
    NULL, NULL, NULL,NULL, 
    vnetdev_ioctl, NULL, 
    vnetdev_transmit,
    vnetdev_receive
  };
  
  int  if_flags;
  struct netdev *netdev = &vnetdev->netdev;

  vnetdev->id = id;
  vnetdev->type = type;
  vnetdev->notify = notify; /* save notify function */
  vnetdev->buf_size = bsize;

  netdev->magic_no = NETDEV_MAGIC;
  snprintf(netdev->dev_name, IF_NAMESIZE, "vnd-%d", id);
  lib_strcpy(netdev->if_name, "vn");
  netdev->if_hostname = "VND@SylixOS";
  
  netdev->init_flags = NETDEV_INIT_LOAD_PARAM
                     | NETDEV_INIT_LOAD_DNS
                     | NETDEV_INIT_IPV6_AUTOCFG;
  netdev->chksum_flags = NETDEV_CHKSUM_ENABLE_ALL; /* we need soft chksum */
  
  if (type == IF_VND_TYPE_RAW) {
    netdev->net_type = NETDEV_TYPE_RAW;
    if_flags = IFF_UP | IFF_POINTOPOINT | IFF_MULTICAST;
  
  } else {
    netdev->net_type = NETDEV_TYPE_ETHERNET;
    if_flags = IFF_UP | IFF_BROADCAST | IFF_MULTICAST;
  }
  
  netdev->speed = 0;
  netdev->mtu = VNETDEV_MTU_DEF;
  netdev->hwaddr_len = ETH_ALEN;
  netdev->priv = priv;
  netdev->drv = &vnetdev_funcs;
  
  return (netdev_add(netdev, NULL, NULL, NULL, if_flags));
}

/* delete a virtual netdev */
int vnetdev_delete (struct vnetdev *vnetdev)
{
  struct netdev *netdev = &vnetdev->netdev;
  PLW_LIST_RING pring;
  struct vnd_q *vndq;

  netdev_delete(netdev);
  while (vnetdev->q) {
    pring = _list_ring_get_prev(vnetdev->q);
    vndq = (struct vnd_q *)pring;
    _List_Ring_Del(&vndq->ring, &vnetdev->q);
    pbuf_free(&vndq->p.pbuf); /* delete all buffer */
  }
  
  return (0);
}

/* virtual netdev set linkup */
int vnetdev_linkup (struct vnetdev *vnetdev, int up)
{
  struct netdev *netdev = &vnetdev->netdev;
  struct netif *netif = (struct netif *)netdev->sys;
  PLW_LIST_RING pring;
  struct vnd_q *vndq;

  if (up) {
    netif_set_link_up(netif);
  
  } else {
    netif_set_link_down(netif);
    while (vnetdev->q) {
      pring = _list_ring_get_prev(vnetdev->q);
      vndq = (struct vnd_q *)pring;
      _List_Ring_Del(&vndq->ring, &vnetdev->q);
      pbuf_free(&vndq->p.pbuf); /* delete all buffer */
    }
  }
  
  return (0);
}

/* put a packet to virtual netdev as a recv */
int vnetdev_put (struct vnetdev *vnetdev, struct pbuf *p)
{
  struct netdev *netdev = &vnetdev->netdev;
  struct netif *netif = (struct netif *)netdev->sys;
  
  if (!netif_is_link_up(netif)) {
    return (-1);
  }
  
  if (vnetdev->type == IF_VND_TYPE_ETHERNET) {
    /* NOTICE: virtual net device recv not use netdev receive function 
     *         so we MUST move a pad size */
#if ETH_PAD_SIZE
    pbuf_header(p, ETH_PAD_SIZE);
#endif
  }
  
  if (netif->input(p, netif)) {
    netdev_linkinfo_drop_inc(netdev);
    netdev_statinfo_discards_inc(netdev, LINK_INPUT);
    return (-1);
  }
  
  netdev_linkinfo_recv_inc(netdev);
  netdev_statinfo_total_add(netdev, LINK_INPUT, p->tot_len);
  if (((UINT8 *)p->payload)[0] & 1) {
    netdev_statinfo_mcasts_inc(netdev, LINK_INPUT);
  } else {
    netdev_statinfo_ucasts_inc(netdev, LINK_INPUT);
  }
  return (0);
}

/* get a packet from virtual netdev as a send */
struct pbuf *vnetdev_get (struct vnetdev *vnetdev)
{
  PLW_LIST_RING pring;
  struct vnd_q *vndq;
  
  if (vnetdev->q) {
    pring = _list_ring_get_prev(vnetdev->q);
    vndq = (struct vnd_q *)pring;
    vnetdev->cur_size -= vndq->p.pbuf.tot_len;
    _List_Ring_Del(&vndq->ring, &vnetdev->q);
    return (&vndq->p.pbuf);
  }
  
  return (NULL);
}

/* get total bytes in virtual netdev buffer */
int vnetdev_nread (struct vnetdev *vnetdev)
{
  return ((int)vnetdev->cur_size);
}

/* get next input packet bytes */
int vnetdev_nrbytes (struct vnetdev *vnetdev)
{
  PLW_LIST_RING pring;
  struct vnd_q *vndq;
  
  if (vnetdev->q) {
    pring = _list_ring_get_prev(vnetdev->q);
    vndq = (struct vnd_q *)pring;
    return (vndq->p.pbuf.tot_len);
  }
  
  return (0);
}

/* get virtual netdev mtu */
int vnetdev_mtu (struct vnetdev *vnetdev)
{
  return (vnetdev->netdev.mtu);
}

/* get virtual netdev max packet len */
int vnetdev_maxplen (struct vnetdev *vnetdev)
{
  if (vnetdev->type == IF_VND_TYPE_ETHERNET) {
    return (vnetdev->netdev.mtu + ETH_HLEN + SIZEOF_VLAN_HDR);
  } else {
    return (vnetdev->netdev.mtu);
  }
}

/* flush virtual netdev buffer */
void vnetdev_flush (struct vnetdev *vnetdev)
{
  PLW_LIST_RING pring;
  struct vnd_q *vndq;

  while (vnetdev->q) {
    pring = _list_ring_get_prev(vnetdev->q);
    vndq = (struct vnd_q *)pring;
    _List_Ring_Del(&vndq->ring, &vnetdev->q);
    pbuf_free(&vndq->p.pbuf); /* delete all buffer */
  }
}

/* set virtual netdev buffer size */
int vnetdev_bufsize (struct vnetdev *vnetdev, size_t bsize)
{
  if (bsize < vnetdev->netdev.mtu) {
    return (-1);
  }
  
  vnetdev->buf_size = bsize;
  return (0);
}

#endif /* LW_CFG_NET_VNETDEV_EN */
/*
 * end
 */
