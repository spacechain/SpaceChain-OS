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

#ifndef __IP6_ROUTE_H
#define __IP6_ROUTE_H

#if LWIP_IPV6

#include "net/if.h"
#include "net/if_dl.h"
#include "net/route.h"

/* tcpip safety call */
#ifndef RT_LOCK
#define RT_LOCK() \
        if (lock_tcpip_core) { \
          LOCK_TCPIP_CORE(); \
        }
#define RT_UNLOCK() \
        if (lock_tcpip_core) { \
          UNLOCK_TCPIP_CORE(); \
        }
#endif

/* socket size round up */
#ifndef SO_ROUND_UP
#define SO_ROUND_UP(len)    ROUND_UP(len, sizeof(size_t))
#define SA_ROUND_UP(x)      SO_ROUND_UP(((struct sockaddr *)(x))->sa_len)
#define SA_NEXT(t, x)       (t)((PCHAR)(x) + SA_ROUND_UP(x))
#endif

/* route entry */
struct rt6_entry {
  LW_LIST_LINE      rt6_list;    /* hash list */
  
  struct ip6_addr   rt6_dest;    /* destination address */
  struct ip6_addr   rt6_netmask; /* destination net mask */
  struct ip6_addr   rt6_gateway; /* gateway address */
  struct ip6_addr   rt6_ifaddr;  /* which address will use in netif */
  
  u_long            rt6_flags;   /* flags */
  struct netif     *rt6_netif;   /* netif */
  char              rt6_ifname[IF_NAMESIZE]; /* ifname */
  
  u_long            rt6_inits;   /* which metrics we are initializing */
  struct rt_metrics rt6_rmx;     /* metrics */
  int               rt6_metric;  /* value of metric */
  u_long            rt6_refcnt;  /* references counter */
  int               rt6_type;
#define RT6_TYPE_USER     0
#define RT6_TYPE_BUILDIN  1
#define RT6_TYPE_ND6      2
  
  void             *rt6_rtdev;   /* came for rtentry rt_dev */
  pid_t             rt6_pid;
  int               rt6_seq;
};

/* tools */
int  rt6_netmask_to_prefix(struct ip6_addr *netmask);
void rt6_netmask_from_prefix(struct ip6_addr *netmask, int prefix);

/* route internal functions */
void rt6_traversal_entry(VOIDFUNCPTR func, void *arg0, void *arg1, 
                          void *arg2, void *arg3, void *arg4, void *arg5);
struct rt6_entry *rt6_find_entry(const ip6_addr_t *ip6dest, 
                                 const ip6_addr_t *ip6netmask, 
                                 const ip6_addr_t *ip6gateway, 
                                 const char *ifname, u_long host);
int  rt6_add_entry(struct rt6_entry *entry);
void rt6_delete_entry(struct rt6_entry *entry);
void rt6_total_entry(unsigned int *cnt);

/* xchg funcs */
#define rt6_build_sockaddr_dl rt_build_sockaddr_dl
size_t rt6_entry_to_msghdr(struct rt_msghdr *rtmsghdr, const struct rt6_entry *entry6, u_char type);
int  rt6_msghdr_to_entry(struct rt6_entry *entry6, const struct rt_msghdr *rtmsghdr);
void rt6_entry_to_rtentry(struct rtentry *rtentry, const struct rt6_entry *entry6);
void rt6_rtentry_to_entry(struct rt6_entry *entry6, const struct rtentry *rtentry);

/* netif hooks */
void rt6_netif_add_hook(struct netif *netif);
void rt6_netif_remove_hook(struct netif *netif);
void rt6_netif_invcache_hook(struct netif *netif);

/* tcpip hooks */
struct netif *rt6_route_search_hook(const ip6_addr_t *ip6src, const ip6_addr_t *ip6dest);
ip6_addr_t *rt6_route_gateway_hook(struct netif *netif, const ip6_addr_t *ip6dest);

#endif /* LWIP_IPV6 */
#endif /* __IP6_ROUTE_H */
/*
 * end
 */
