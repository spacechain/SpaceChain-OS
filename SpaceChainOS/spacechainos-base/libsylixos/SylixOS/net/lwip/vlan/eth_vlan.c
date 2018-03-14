/**
 * @file
 * Ethernet VLAN.
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

#define __SYLIXOS_KERNEL
#include "SylixOS.h"

#if LW_CFG_NET_VLAN_EN > 0

#include "net/if_vlan.h"
#include "netif/etharp.h"
#include "eth_vlan.h"

#define VLAN_TAG_MASK       0xfff
#define VLAN_TAG_OFFSET     0
#define VLAN_TAG_GET(id)    (u16_t)((id) & VLAN_TAG_MASK)

#define VLAN_PRI_MASK       0x3
#define VLAN_PRI_OFFSET     13
#define VLAN_PRI_GET(id)    (u16_t)(((id) >> VLAN_PRI_OFFSET) & VLAN_TAG_MASK)

#define VLAN_ID_VALID(id)       (VLAN_TAG_GET(id) != VLAN_TAG_MASK)
#define VLAN_ID_SET(tag, pri)   (u16_t)(((tag) & VLAN_TAG_MASK) | (((pri) & VLAN_PRI_MASK) << VLAN_PRI_OFFSET))

/* vlan id get */
int ethernet_vlan_get (struct vlanreq *req)
{
  struct netif *netif;
  SYS_ARCH_DECL_PROTECT(lev);
  
  netif = netif_find(req->vlr_ifname);
  if (netif == NULL) {
    return (ETH_VLAN_ENODEV);
  }
  
  if (!(netif->flags & (NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET))) {
    return (ETH_VLAN_ENOETH);
  }
  
  SYS_ARCH_PROTECT(lev);
  req->vlr_tag = VLAN_TAG_GET(netif->vlanid);
  req->vlr_pri = VLAN_PRI_GET(netif->vlanid);
  SYS_ARCH_UNPROTECT(lev);
  
  return (ETH_VLAN_OK);
}

/* vlan id set */
int ethernet_vlan_set (const struct vlanreq *req)
{
  struct netif *netif;
  SYS_ARCH_DECL_PROTECT(lev);
  
  netif = netif_find(req->vlr_ifname);
  if (netif == NULL) {
    return (ETH_VLAN_ENODEV);
  }
  
  if (!(netif->flags & (NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET))) {
    return (ETH_VLAN_ENOETH);
  }
  
  SYS_ARCH_PROTECT(lev);
  netif->vlanid = VLAN_ID_SET(req->vlr_tag, req->vlr_pri);
  SYS_ARCH_UNPROTECT(lev);
  
  return (ETH_VLAN_OK);
}

/* traversal all netif vlan id */
void ethernet_vlan_traversal (VOIDFUNCPTR func, void *arg0, void *arg1, 
                              void *arg2, void *arg3, void *arg4, void *arg5)
{
  struct netif *netif;
  struct vlanreq req;
  SYS_ARCH_DECL_PROTECT(lev);
  
  NETIF_FOREACH(netif) {
    if ((netif->flags & (NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET)) && 
        VLAN_ID_VALID(netif->vlanid)) {
      netif_get_name(netif, req.vlr_ifname);
      
      SYS_ARCH_PROTECT(lev);
      req.vlr_tag = VLAN_TAG_GET(netif->vlanid);
      req.vlr_pri = VLAN_PRI_GET(netif->vlanid);
      SYS_ARCH_UNPROTECT(lev);
      
      func(&req, arg0, arg1, arg2, arg3, arg4, arg5);
    }
  }
}

/* netif vlan table cnt */
static void ethernet_vlan_counter (struct vlanreq *req, int *cnt)
{
  (*cnt) += 1;
}

/* get netif vlan total num */
void ethernet_vlan_total (unsigned int *cnt)
{
  int count = 0;

  ethernet_vlan_traversal(ethernet_vlan_counter, &count, NULL, NULL, NULL, NULL, NULL);
  if (cnt) {
    *cnt = count;
  }
}

/* lwip vlan set hook */
int ethernet_vlan_set_hook (struct netif *netif, struct pbuf *p, const struct eth_addr *src, 
                            const struct eth_addr *dst, u16_t eth_type)
{
  if (VLAN_ID_VALID(netif->vlanid)) {
    return ((int)netif->vlanid);
  }
  
  return (-1);
}

/* lwip vlan check hook */
int ethernet_vlan_check_hook (struct netif *netif, const struct eth_hdr *ethhdr, 
                              const struct eth_vlan_hdr *vlanhdr)
{
  if (VLAN_ID_VALID(netif->vlanid)) {
    if (VLAN_TAG_GET(netif->vlanid) != VLAN_TAG_GET(vlanhdr->prio_vid)) {
      return (0);
    }
  }
  
  return (1);
}

#endif /* LW_CFG_NET_VLAN_EN */
/*
 * end
 */
