/**
 * @file
 * Lwip platform independent net ipv4 route.
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

#if LW_CFG_NET_ROUTER > 0

#include "net/if_type.h"
#include "net/if_ether.h"
#include "lwip/inet.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "net/route.h"
#include "ip4_route.h"

/* netif to sockaddr_dl */
void  rt_build_sockaddr_dl (struct sockaddr_dl *sdl, struct netif *netif)
{
  sdl->sdl_len = sizeof(struct sockaddr_dl);
  sdl->sdl_family = AF_LINK;
  sdl->sdl_index = netif_get_index(netif);
  sdl->sdl_slen = 0;
  
  if (netif->flags & NETIF_FLAG_ETHERNET) {
    sdl->sdl_type = IFT_ETHER;
    sdl->sdl_alen = ETH_ALEN;

  } else if (!(netif->flags & NETIF_FLAG_BROADCAST)) {
    sdl->sdl_type = IFT_PPP;
    sdl->sdl_alen = 0;

  } else {
    sdl->sdl_type = IFT_OTHER;
    sdl->sdl_alen = 0;
  }
  
  netif_get_name(netif, sdl->sdl_data);
  sdl->sdl_nlen = (u_char)lib_strlen(sdl->sdl_data);
}

/* rt_entry to struct rt_msghdr */
size_t  rt_entry_to_msghdr (struct rt_msghdr *rtmsghdr, const struct rt_entry *entry, u_char type)
{
  size_t size;
  struct sockaddr_in *sin;
  struct sockaddr_dl *sdl;
  
  size = sizeof(struct rt_msghdr) + (SO_ROUND_UP(sizeof(struct sockaddr_in)) * 4);
  if (entry->rt_netif) {
    size += SO_ROUND_UP(sizeof(struct sockaddr_dl));
  }

  if (!rtmsghdr) {
    return (size);
  }
  
  rtmsghdr->rtm_msglen  = (u_short)size;
  rtmsghdr->rtm_version = RTM_VERSION;
  rtmsghdr->rtm_type    = type;

  rtmsghdr->rtm_flags = (int)entry->rt_flags;
  rtmsghdr->rtm_addrs = RTA_DST | RTA_GATEWAY | RTA_NETMASK | RTA_GENMASK;
  rtmsghdr->rtm_pid   = entry->rt_pid;
  rtmsghdr->rtm_seq   = entry->rt_seq;
  rtmsghdr->rtm_errno = 0;
  rtmsghdr->rtm_use   = 0;
  rtmsghdr->rtm_inits = entry->rt_inits;
  rtmsghdr->rtm_rmx   = entry->rt_rmx;

  if (entry->rt_netif) {
    rtmsghdr->rtm_index  = netif_get_index(entry->rt_netif);
    rtmsghdr->rtm_addrs |= RTA_IFP;
  } else {
    rtmsghdr->rtm_index  = 0;
  }

  sin = (struct sockaddr_in *)(rtmsghdr + 1);
  sin->sin_len = sizeof(struct sockaddr_in);
  sin->sin_family = AF_INET;
  sin->sin_addr.s_addr = entry->rt_dest.addr;

  sin = SA_NEXT(struct sockaddr_in *, sin);
  sin->sin_len = sizeof(struct sockaddr_in);
  sin->sin_family = AF_INET;
  sin->sin_addr.s_addr = entry->rt_gateway.addr;

  sin = SA_NEXT(struct sockaddr_in *, sin);
  sin->sin_len = sizeof(struct sockaddr_in);
  sin->sin_family = AF_INET;
  sin->sin_addr.s_addr = entry->rt_netmask.addr;

  sin = SA_NEXT(struct sockaddr_in *, sin);
  sin->sin_len = sizeof(struct sockaddr_in);
  sin->sin_family = AF_INET;
  sin->sin_addr.s_addr = entry->rt_netmask.addr;

  if (entry->rt_netif) {
    sdl = SA_NEXT(struct sockaddr_dl *, sin);
    lib_bzero(sdl, sizeof(struct sockaddr_dl));
    rt_build_sockaddr_dl(sdl, entry->rt_netif);
  }
  
  return (size);
}

/* struct rt_msghdr to rt_entry */
int  rt_msghdr_to_entry (struct rt_entry *entry, const struct rt_msghdr *rtmsghdr)
{
  struct netif *netif;
  struct sockaddr_in *sin;

  if (!rtmsghdr || 
      ((rtmsghdr->rtm_addrs & (RTA_DST | RTA_GATEWAY)) != 
      (RTA_DST | RTA_GATEWAY))) {
    return (-1);
  }
  
  netif = netif_get_by_index(rtmsghdr->rtm_index);
  if (netif) {
    netif_get_name(netif, entry->rt_ifname);
  }
  
  entry->rt_flags  = rtmsghdr->rtm_flags;
  entry->rt_rmx    = rtmsghdr->rtm_rmx;
  entry->rt_refcnt = 0;
  entry->rt_pid    = rtmsghdr->rtm_pid;
  entry->rt_seq    = rtmsghdr->rtm_seq;
  entry->rt_inits  = rtmsghdr->rtm_inits;
  
  if (entry->rt_inits & RTV_HOPCOUNT) {
    entry->rt_metric = (int)rtmsghdr->rtm_rmx.rmx_hopcount;
  } else {
    entry->rt_metric = 0;
  }
    
  sin = (struct sockaddr_in *)(rtmsghdr + 1);
  entry->rt_dest.addr = sin->sin_addr.s_addr;
    
  sin = SA_NEXT(struct sockaddr_in *, sin);
  entry->rt_gateway.addr = sin->sin_addr.s_addr;
    
  if (rtmsghdr->rtm_addrs & RTA_NETMASK) {
    sin = SA_NEXT(struct sockaddr_in *, sin);
    entry->rt_netmask.addr = sin->sin_addr.s_addr;
  } 
    
  if (rtmsghdr->rtm_addrs & RTA_GENMASK) {
    sin = SA_NEXT(struct sockaddr_in *, sin);
    entry->rt_netmask.addr = sin->sin_addr.s_addr;
  }
  
  return (0);
}

/* rt_entry to struct rtentry */
void  rt_entry_to_rtentry (struct rtentry *rtentry, const struct rt_entry *entry)
{
  rtentry->rt_dst.sa_len = sizeof(struct sockaddr_in);
  rtentry->rt_dst.sa_family = AF_INET;
  rtentry->rt_gateway.sa_len = sizeof(struct sockaddr_in);
  rtentry->rt_gateway.sa_family = AF_INET;
  rtentry->rt_genmask.sa_len = sizeof(struct sockaddr_in);
  rtentry->rt_genmask.sa_family = AF_INET;

  inet_addr_from_ip4addr(&((struct sockaddr_in *)&rtentry->rt_dst)->sin_addr, &entry->rt_dest);
  inet_addr_from_ip4addr(&((struct sockaddr_in *)&rtentry->rt_gateway)->sin_addr, &entry->rt_gateway);
  inet_addr_from_ip4addr(&((struct sockaddr_in *)&rtentry->rt_genmask)->sin_addr, &entry->rt_netmask);
  
  lib_strlcpy(rtentry->rt_ifname, entry->rt_ifname, IF_NAMESIZE);
  
  rtentry->rt_flags  = (u_short)entry->rt_flags;
  rtentry->rt_refcnt = (u_short)entry->rt_refcnt;
  rtentry->rt_metric = (short)entry->rt_metric;
  rtentry->rt_dev    = entry->rt_rtdev;
  
  rtentry->rt_hopcount = entry->rt_rmx.rmx_hopcount;
  rtentry->rt_irtt     = (u_short)entry->rt_rmx.rmx_rtt;
  rtentry->rt_mtu      = entry->rt_rmx.rmx_mtu;
  rtentry->rt_window   = 0; /* TCP_WND ? */
}

/* struct rtentry to rt_entry */
void  rt_rtentry_to_entry (struct rt_entry *entry, const struct rtentry *rtentry)
{
  inet_addr_to_ip4addr(&entry->rt_dest, &((struct sockaddr_in *)&rtentry->rt_dst)->sin_addr);
  inet_addr_to_ip4addr(&entry->rt_gateway, &((struct sockaddr_in *)&rtentry->rt_gateway)->sin_addr);
  inet_addr_to_ip4addr(&entry->rt_netmask, &((struct sockaddr_in *)&rtentry->rt_genmask)->sin_addr);
  entry->rt_ifaddr.addr = IPADDR_ANY;
  
  lib_strlcpy(entry->rt_ifname, rtentry->rt_ifname, IF_NAMESIZE);
  
  entry->rt_flags  = rtentry->rt_flags;
  entry->rt_flags &= ~RTF_RUNNING;
  entry->rt_refcnt = rtentry->rt_refcnt;
  entry->rt_metric = rtentry->rt_metric;
  entry->rt_rtdev  = rtentry->rt_dev;
    
  entry->rt_rmx.rmx_hopcount = rtentry->rt_hopcount;
  entry->rt_rmx.rmx_rtt      = rtentry->rt_irtt;
  entry->rt_rmx.rmx_mtu      = rtentry->rt_mtu;
}

#endif /* LW_CFG_NET_ROUTER */
/*
 * end
 */
