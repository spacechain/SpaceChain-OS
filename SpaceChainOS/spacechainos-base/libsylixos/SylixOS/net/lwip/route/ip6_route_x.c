/**
 * @file
 * Lwip platform independent net ipv6 route.
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

#include "lwip/inet.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "net/route.h"
#include "ip6_route.h"

#if LWIP_IPV6

extern void  rt_build_sockaddr_dl(struct sockaddr_dl *sdl, struct netif *netif);

/* rt6_entry to struct rt_msghdr */
size_t rt6_entry_to_msghdr (struct rt_msghdr *rtmsghdr, const struct rt6_entry *entry6, u_char type)
{
  size_t size;
  struct sockaddr_in6 *sin6;
  struct sockaddr_dl *sdl;
  
  size = sizeof(struct rt_msghdr) + (SO_ROUND_UP(sizeof(struct sockaddr_in6)) * 4);
  if (entry6->rt6_netif) {
    size += SO_ROUND_UP(sizeof(struct sockaddr_dl));
  }

  if (!rtmsghdr) {
    return (size);
  }
  
  rtmsghdr->rtm_msglen  = (u_short)size;
  rtmsghdr->rtm_version = RTM_VERSION;
  rtmsghdr->rtm_type    = type;

  rtmsghdr->rtm_flags = (int)entry6->rt6_flags;
  rtmsghdr->rtm_addrs = RTA_DST | RTA_GATEWAY | RTA_NETMASK | RTA_GENMASK;
  rtmsghdr->rtm_pid   = entry6->rt6_pid;
  rtmsghdr->rtm_seq   = entry6->rt6_seq;
  rtmsghdr->rtm_errno = 0;
  rtmsghdr->rtm_use   = 0;
  rtmsghdr->rtm_inits = entry6->rt6_inits;
  rtmsghdr->rtm_rmx   = entry6->rt6_rmx;

  if (entry6->rt6_netif) {
    rtmsghdr->rtm_index  = netif_get_index(entry6->rt6_netif);
    rtmsghdr->rtm_addrs |= RTA_IFP;
  } else {
    rtmsghdr->rtm_index  = 0;
  }

  sin6 = (struct sockaddr_in6 *)(rtmsghdr + 1);
  sin6->sin6_len = sizeof(struct sockaddr_in6);
  sin6->sin6_family = AF_INET6;
  inet6_addr_from_ip6addr(&sin6->sin6_addr, &entry6->rt6_dest);

  sin6 = SA_NEXT(struct sockaddr_in6 *, sin6);
  sin6->sin6_len = sizeof(struct sockaddr_in6);
  sin6->sin6_family = AF_INET6;
  inet6_addr_from_ip6addr(&sin6->sin6_addr, &entry6->rt6_gateway);

  sin6 = SA_NEXT(struct sockaddr_in6 *, sin6);
  sin6->sin6_len = sizeof(struct sockaddr_in6);
  sin6->sin6_family = AF_INET6;
  inet6_addr_from_ip6addr(&sin6->sin6_addr, &entry6->rt6_netmask);

  sin6 = SA_NEXT(struct sockaddr_in6 *, sin6);
  sin6->sin6_len = sizeof(struct sockaddr_in6);
  sin6->sin6_family = AF_INET6;
  inet6_addr_from_ip6addr(&sin6->sin6_addr, &entry6->rt6_netmask);

  if (entry6->rt6_netif) {
    sdl = SA_NEXT(struct sockaddr_dl *, sin6);
    lib_bzero(sdl, sizeof(struct sockaddr_dl));
    rt_build_sockaddr_dl(sdl, entry6->rt6_netif);
  }
  
  return (size);
}

/* struct rt_msghdr to rt6_entry */
int rt6_msghdr_to_entry (struct rt6_entry *entry6, const struct rt_msghdr *rtmsghdr)
{
  struct netif *netif;
  struct sockaddr_in6 *sin6;
  
  if (!rtmsghdr || 
      ((rtmsghdr->rtm_addrs & (RTA_DST | RTA_GATEWAY)) != 
      (RTA_DST | RTA_GATEWAY))) {
    return (-1);
  }
  
  netif = netif_get_by_index(rtmsghdr->rtm_index);
  if (netif) {
    netif_get_name(netif, entry6->rt6_ifname);
  }
    
  entry6->rt6_flags  = rtmsghdr->rtm_flags;
  entry6->rt6_rmx    = rtmsghdr->rtm_rmx;
  entry6->rt6_refcnt = 0;
  entry6->rt6_pid    = rtmsghdr->rtm_pid;
  entry6->rt6_seq    = rtmsghdr->rtm_seq;
  entry6->rt6_inits  = rtmsghdr->rtm_inits;
  
  if (entry6->rt6_inits & RTV_HOPCOUNT) {
    entry6->rt6_metric = (int)rtmsghdr->rtm_rmx.rmx_hopcount;
  } else {
    entry6->rt6_metric = 0;
  }
    
  sin6 = (struct sockaddr_in6 *)(rtmsghdr + 1);
  inet6_addr_to_ip6addr(&entry6->rt6_dest, &sin6->sin6_addr);
    
  sin6 = SA_NEXT(struct sockaddr_in6 *, sin6);
  inet6_addr_to_ip6addr(&entry6->rt6_gateway, &sin6->sin6_addr);
    
  if (rtmsghdr->rtm_addrs & RTA_NETMASK) {
    sin6 = SA_NEXT(struct sockaddr_in6 *, sin6);
    inet6_addr_to_ip6addr(&entry6->rt6_netmask, &sin6->sin6_addr);
  } 
    
  if (rtmsghdr->rtm_addrs & RTA_GENMASK) {
    sin6 = SA_NEXT(struct sockaddr_in6 *, sin6);
    inet6_addr_to_ip6addr(&entry6->rt6_netmask, &sin6->sin6_addr);
  }
  
  return (0);
}

/* rt6_entry to struct rtentry */
void rt6_entry_to_rtentry (struct rtentry *rtentry, const struct rt6_entry *entry6)
{
  rtentry->rt_dst.sa_len = sizeof(struct sockaddr_in6);
  rtentry->rt_dst.sa_family = AF_INET6;
  rtentry->rt_gateway.sa_len = sizeof(struct sockaddr_in6);
  rtentry->rt_gateway.sa_family = AF_INET6;
  rtentry->rt_genmask.sa_len = sizeof(struct sockaddr_in6);
  rtentry->rt_genmask.sa_family = AF_INET6;

  inet6_addr_from_ip6addr(&((struct sockaddr_in6 *)&rtentry->rt_dst)->sin6_addr,     &entry6->rt6_dest);
  inet6_addr_from_ip6addr(&((struct sockaddr_in6 *)&rtentry->rt_gateway)->sin6_addr, &entry6->rt6_gateway);
  inet6_addr_from_ip6addr(&((struct sockaddr_in6 *)&rtentry->rt_genmask)->sin6_addr, &entry6->rt6_netmask);

  lib_strlcpy(rtentry->rt_ifname, entry6->rt6_ifname, IF_NAMESIZE);

  rtentry->rt_flags  = (u_short)entry6->rt6_flags;
  rtentry->rt_refcnt = (u_short)entry6->rt6_refcnt;
  rtentry->rt_metric = (short)entry6->rt6_metric;
  rtentry->rt_dev    = entry6->rt6_rtdev;

  rtentry->rt_hopcount = entry6->rt6_rmx.rmx_hopcount;
  rtentry->rt_irtt     = (u_short)entry6->rt6_rmx.rmx_rtt;
  rtentry->rt_mtu      = entry6->rt6_rmx.rmx_mtu;
  rtentry->rt_window   = 0; /* TCP_WND ? */
}

/* struct rtentry to rt6_entry */
void rt6_rtentry_to_entry (struct rt6_entry *entry6, const struct rtentry *rtentry)
{
  inet6_addr_to_ip6addr(&entry6->rt6_dest,    &((struct sockaddr_in6 *)&rtentry->rt_dst)->sin6_addr);
  inet6_addr_to_ip6addr(&entry6->rt6_gateway, &((struct sockaddr_in6 *)&rtentry->rt_gateway)->sin6_addr);
  inet6_addr_to_ip6addr(&entry6->rt6_netmask, &((struct sockaddr_in6 *)&rtentry->rt_genmask)->sin6_addr);
  ip6_addr_set_any(&entry6->rt6_ifaddr);
    
  lib_strlcpy(entry6->rt6_ifname, rtentry->rt_ifname, IF_NAMESIZE);
    
  entry6->rt6_flags  = rtentry->rt_flags;
  entry6->rt6_flags &= ~RTF_RUNNING;
  entry6->rt6_refcnt = rtentry->rt_refcnt;
  entry6->rt6_metric = rtentry->rt_metric;
  entry6->rt6_rtdev  = rtentry->rt_dev;
    
  entry6->rt6_rmx.rmx_hopcount = rtentry->rt_hopcount;
  entry6->rt6_rmx.rmx_rtt      = rtentry->rt_irtt;
  entry6->rt6_rmx.rmx_mtu      = rtentry->rt_mtu;
}

#endif /* LWIP_IPV6 */
#endif /* LW_CFG_NET_ROUTER */
/*
 * end
 */
