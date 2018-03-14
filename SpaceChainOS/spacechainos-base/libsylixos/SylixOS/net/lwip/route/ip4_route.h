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

#ifndef __IP4_ROUTE_H
#define __IP4_ROUTE_H

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
struct rt_entry {
  LW_LIST_LINE      rt_list;    /* hash list */
  
  struct ip4_addr   rt_dest;    /* destination address */
  struct ip4_addr   rt_netmask; /* destination net mask */
  struct ip4_addr   rt_gateway; /* gateway address */
  struct ip4_addr   rt_ifaddr;  /* which address will use in netif */
  
  u_long            rt_flags;   /* flags */
  struct netif     *rt_netif;   /* netif */
  char              rt_ifname[IF_NAMESIZE]; /* ifname */
  
  u_long            rt_inits;   /* which metrics we are initializing */
  struct rt_metrics rt_rmx;     /* metrics */
  int               rt_metric;  /* value of metric */
  u_long            rt_refcnt;  /* references counter */
  int               rt_type;
#define RT_TYPE_USER     0
#define RT_TYPE_BUILDIN  1
  
  void             *rt_rtdev;   /* came for rtentry rt_dev */
  pid_t             rt_pid;
  int               rt_seq;
};

/* route internal functions */
void rt_traversal_entry(VOIDFUNCPTR func, void *arg0, void *arg1, 
                        void *arg2, void *arg3, void *arg4, void *arg5);
struct rt_entry *rt_find_entry(const ip4_addr_t *ipdest, 
                               const ip4_addr_t *ipnetmask, 
                               const ip4_addr_t *ipgateway,
                               const char *ifname, u_long flags);
int  rt_add_entry(struct rt_entry *entry);
void rt_delete_entry(struct rt_entry *entry);
void rt_total_entry(unsigned int *cnt);
int  rt_change_default(const ip4_addr_t *ipgateway, const char *ifname);
int  rt_get_default(struct rt_entry *entry);

/* xchg funcs */
void rt_build_sockaddr_dl(struct sockaddr_dl *sdl, struct netif *netif);
size_t rt_entry_to_msghdr(struct rt_msghdr *rtmsghdr, const struct rt_entry *entry, u_char type);
int  rt_msghdr_to_entry(struct rt_entry *entry, const struct rt_msghdr *rtmsghdr);
void rt_entry_to_rtentry(struct rtentry *rtentry, const struct rt_entry *entry);
void rt_rtentry_to_entry(struct rt_entry *entry, const struct rtentry *rtentry);

/* netif hooks */
void rt_netif_add_hook(struct netif *netif);
void rt_netif_remove_hook(struct netif *netif);
void rt_netif_invcache_hook(struct netif *netif);

/* tcpip hooks */
struct netif *rt_route_search_hook(const ip4_addr_t *ipsrc, const ip4_addr_t *ipdest);
ip4_addr_t *rt_route_gateway_hook(struct netif *netif, const ip4_addr_t *ipdest);

#endif /* __IP4_ROUTE_H */
/*
 * end
 */
