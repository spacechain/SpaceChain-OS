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
#include "unistd.h" /* for getpid() */

#if LW_CFG_NET_ROUTER > 0

#include "lwip/inet.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "net/route.h"
#include "ip6_route.h"

#if LWIP_IPV6

#include "lwip/prot/nd6.h"
#include "lwip/priv/nd6_priv.h"

/* flags is good or bad */
#define RTF_GOOD            (RTF_UP | RTF_RUNNING)
#define RTF_BAD             (RTF_REJECT | RTF_BLACKHOLE)
#define RTF_VALID(flags)    (((flags & RTF_GOOD) == RTF_GOOD) && !(flags & RTF_BAD))

/* route table statistical variable */
static unsigned int  rt6_total, rt6_active;

/* buildin metric base */
static int rt6_bi_mbase;

/* route hash table */
#define RT6_HASH_SHIFT   5
#define RT6_HASH_SIZE    (1 << RT6_HASH_SHIFT)
#define RT6_HASH_MASK    (RT6_HASH_SIZE - 1)

/* route hash index */
#if BYTE_ORDER == BIG_ENDIAN
#define RT6_HASH_INDEX(ip6addr) (((ip6addr)->addr[0] >> (RT6_HASH_SIZE - RT6_HASH_SHIFT)) & RT6_HASH_MASK)
#else
#define RT6_HASH_INDEX(ip6addr) (((ip6addr)->addr[0] >> (8 - RT6_HASH_SHIFT)) & RT6_HASH_MASK)
#endif

/* route hash table */
static LW_LIST_LINE_HEADER  rt6_table[RT6_HASH_SIZE];

/* route cache */
struct rt6_cache {
  struct rt6_entry *rt6_entry;
  ip6_addr_t rt6_dest;
};

static struct rt6_cache  rt6_cache;

#define RT6_CACHE_INVAL(entry) \
        if (rt6_cache.rt6_entry == entry) { \
          rt6_cache.rt6_entry = NULL; \
          ip6_addr_set_any(&rt6_cache.rt6_dest); \
        }

/* gateway cache */
struct gw6_cache {
  struct rt6_entry *rt6_entry;
  ip6_addr_t rt6_dest;
  ip6_addr_t rt6_gateway;
};

static struct gw6_cache  gw6_cache[LW_CFG_NET_DEV_MAX];

#define GW6_CACHE_INVAL(num, entry) \
        LWIP_ASSERT("GW6_CACHE_INVAL() num invalide!", (num) < LW_CFG_NET_DEV_MAX); \
        if (gw6_cache[num].rt6_entry == entry) { \
          gw6_cache[num].rt6_entry = NULL; \
          ip6_addr_set_any(&gw6_cache[num].rt6_dest); \
          ip6_addr_set_any(&gw6_cache[num].rt6_gateway); \
        }
        
/* route rt6_netmask_to_prefix */
int  rt6_netmask_to_prefix (struct ip6_addr *netmask)
{
  int prefix = 0;
  u_char val;
  u_char *pnt;
  
  pnt = (u_char *)netmask;
  while ((*pnt == 0xff) && prefix < 128) {
    prefix += 8;
    pnt++;
  }
  
  if (prefix < 128) {
    val = *pnt;
    while (val) {
      prefix++;
      val <<= 1;
    }
  }
  
  return (prefix);
}

/* route rt6_netmask_from_prefix */
void  rt6_netmask_from_prefix (struct ip6_addr *netmask, int prefix)
{
  static const u_char maskbit[] = {0x00, 0x80, 0xc0, 0xe0, 0xf0,
                                   0xf8, 0xfc, 0xfe, 0xff};

  u_char *pnt;
  int bit;
  int offset;

  ip6_addr_set_any(netmask);
  pnt = (u_char *)netmask;

  offset = prefix >> 3; /* / 8 */
  bit = prefix & 7; /* % 8 */

  while (offset--) {
    *pnt++ = 0xff;
  }

  if (bit) {
    *pnt = maskbit[bit];
  }
}

/* route rt6_net_cmp */
static int rt6_net_cmp (const ip6_addr_t *ip6_a, const ip6_addr_t *ip6_b, const ip6_addr_t *ip6msk)
{
  return ((((ip6_a)->addr[0] & (ip6msk)->addr[0]) == ((ip6_b)->addr[0] & (ip6msk)->addr[0])) &&
          (((ip6_a)->addr[1] & (ip6msk)->addr[1]) == ((ip6_b)->addr[1] & (ip6msk)->addr[1])) &&
          (((ip6_a)->addr[2] & (ip6msk)->addr[2]) == ((ip6_b)->addr[2] & (ip6msk)->addr[2])) &&
          (((ip6_a)->addr[3] & (ip6msk)->addr[3]) == ((ip6_b)->addr[3] & (ip6msk)->addr[3])));
}

/* route table match */
static struct rt6_entry *rt6_match (const ip6_addr_t *ip6dest)
{
  int hash = RT6_HASH_INDEX(ip6dest);
  LW_LIST_LINE *pline;
  struct rt6_entry *entry;
  struct rt6_entry *entry_net = NULL;
  
  if (ip6_addr_islinklocal(ip6dest)) {
    return (NULL);
  }
  
  for (pline = rt6_table[hash]; pline != NULL; pline = _list_line_get_next(pline)) {
    entry = (struct rt6_entry *)pline;
    if (RTF_VALID(entry->rt6_flags)) {
      if (netif_is_up(entry->rt6_netif) && netif_is_link_up(entry->rt6_netif)) {
        if (entry->rt6_flags & RTF_HOST) {
          if (ip6_addr_cmp(&entry->rt6_dest, ip6dest)) {
            return (entry); /* match to host */
          }
        
        } else if (entry_net == NULL) {
          if (rt6_net_cmp(&entry->rt6_dest, ip6dest, &entry->rt6_netmask)) {
            entry_net = entry; /* match to net */
          }
        }
      }
    }
  }
  
  return (entry_net);
}

/* route table cnt */
static void rt6_counter (struct rt6_entry *entry, int *cnt)
{
  (*cnt) += 1;
}

/* buildin route table walk call */
static void  rt6_traversal_buildin (VOIDFUNCPTR func, void *arg0, void *arg1, 
                                    void *arg2, void *arg3, void *arg4, void *arg5)
{
  int i;
  struct netif *netif;
  struct rt6_entry entry;
  
  lib_bzero(&entry, sizeof(struct rt6_entry));
  entry.rt6_type = RT6_TYPE_BUILDIN;
  entry.rt6_metric = rt6_bi_mbase + 1;
  
  NETIF_FOREACH(netif) {
    for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
      if (ip6_addr_isvalid(netif_ip6_addr_state(netif, i))) {
        ip6_addr_set(&entry.rt6_dest, netif_ip6_addr(netif, i));
        rt6_netmask_from_prefix(&entry.rt6_netmask, 128);
        ip6_addr_set_any(&entry.rt6_gateway);
        ip6_addr_set(&entry.rt6_ifaddr, netif_ip6_addr(netif, i));
        entry.rt6_netif = netif;
        netif_get_name(netif, entry.rt6_ifname);
        
        if (netif_is_up(netif) && netif_is_link_up(netif)) {
          entry.rt6_flags = RTF_UP | RTF_HOST | RTF_RUNNING | RTF_LOCAL;
        } else {
          entry.rt6_flags = RTF_UP | RTF_HOST | RTF_LOCAL;
        }
        
        func(&entry, arg0, arg1, arg2, arg3, arg4, arg5);
        
        if (ip6_addr_islinklocal(netif_ip6_addr(netif, i))) {
          entry.rt6_dest.addr[2] = 0;
          entry.rt6_dest.addr[3] = 0;
          rt6_netmask_from_prefix(&entry.rt6_netmask, 64);
          entry.rt6_flags &= ~RTF_HOST;
          
          func(&entry, arg0, arg1, arg2, arg3, arg4, arg5);
          
          entry.rt6_dest.addr[0] = PP_HTONL(0xff000000UL);
          entry.rt6_dest.addr[1] = 0;
          entry.rt6_dest.addr[2] = 0;
          entry.rt6_dest.addr[3] = 0;
          rt6_netmask_from_prefix(&entry.rt6_netmask, 8);
          
          func(&entry, arg0, arg1, arg2, arg3, arg4, arg5);
        }
      }
    }
  }
}

/* route table walk call */
static void  rt6_traversal_table (VOIDFUNCPTR func, void *arg0, void *arg1, 
                                  void *arg2, void *arg3, void *arg4, void *arg5)
{
  int i;
  LW_LIST_LINE *pline;
  struct rt6_entry *entry;
  
  for (i = 0; i < RT6_HASH_SIZE; i++) {
    for (pline = rt6_table[i]; pline != NULL; pline = _list_line_get_next(pline)) {
      entry = (struct rt6_entry *)pline;
      if (rt6_bi_mbase < entry->rt6_metric) {
        rt6_bi_mbase = entry->rt6_metric; /* buildin alway has low priority */
      }
      func(entry, arg0, arg1, arg2, arg3, arg4, arg5);
    }
  }
}

/* buildin route table walk call */
static void  rt6_traversal_nd6 (VOIDFUNCPTR func, void *arg0, void *arg1, 
                                void *arg2, void *arg3, void *arg4, void *arg5)
{
  int i, j;
  struct nd6_destination_cache_entry *destination;
  struct nd6_router_list_entry *nd6_entry;
  struct rt6_entry entry;
  struct netif *netif;
  
  lib_bzero(&entry, sizeof(struct rt6_entry));
  entry.rt6_type = RT6_TYPE_ND6;
  entry.rt6_metric = rt6_bi_mbase + 1;
  
  for (i = 0; i < LWIP_ND6_NUM_DESTINATIONS; i++) {
    destination = &destination_cache[i];
    if (!ip6_addr_isany_val(destination->destination_addr)) {
      ip6_addr_set(&entry.rt6_dest, &destination->destination_addr);
      rt6_netmask_from_prefix(&entry.rt6_netmask, 64);
      ip6_addr_set(&entry.rt6_gateway, &destination->next_hop_addr);
      netif = NULL;
      for (j = 0; j < LWIP_ND6_NUM_ROUTERS; j++) {
        nd6_entry = &default_router_list[j];
        if ((nd6_entry->neighbor_entry != NULL) &&
            (nd6_entry->neighbor_entry->state == ND6_REACHABLE)) {
          if (ip6_addr_cmp(&destination->next_hop_addr,
                           &nd6_entry->neighbor_entry->next_hop_address)) {
            netif = nd6_entry->neighbor_entry->netif;
            break;
          }
        }
      }
      if (netif) {
        for (j = 0; j < LWIP_IPV6_NUM_ADDRESSES; j++) {
          if (ip6_addr_isvalid(netif_ip6_addr_state(netif, j)) &&
              netif_ip6_addr_isstatic(netif, j) &&
              ip6_addr_netcmp(&destination->next_hop_addr, netif_ip6_addr(netif, j))) {
            break;
          }
        }
        if (j < LWIP_IPV6_NUM_ADDRESSES) {
          ip6_addr_set(&entry.rt6_ifaddr, netif_ip6_addr(netif, j));
          entry.rt6_netif = netif;
          netif_get_name(netif, entry.rt6_ifname);
          
          if (netif_is_up(netif) && netif_is_link_up(netif)) {
            entry.rt6_flags = RTF_UP | RTF_HOST | RTF_RUNNING | RTF_DYNAMIC;
          } else {
            entry.rt6_flags = RTF_UP | RTF_HOST | RTF_DYNAMIC;
          }
        
          func(&entry, arg0, arg1, arg2, arg3, arg4, arg5);
        }
      }
    }
  }
  
  for (i = 0; i < LWIP_ND6_NUM_ROUTERS; i++) {
    nd6_entry = &default_router_list[i];
    if ((nd6_entry->neighbor_entry != NULL) &&
        (nd6_entry->neighbor_entry->state == ND6_REACHABLE)) {
      netif = nd6_entry->neighbor_entry->netif;
      ip6_addr_set_any(&entry.rt6_dest);
      rt6_netmask_from_prefix(&entry.rt6_netmask, 64);
      ip6_addr_set(&entry.rt6_gateway, &nd6_entry->neighbor_entry->next_hop_address);
      ip6_addr_set_any(&entry.rt6_ifaddr);
      entry.rt6_netif = netif;
      netif_get_name(netif, entry.rt6_ifname);
      
      if (netif_is_up(netif) && netif_is_link_up(netif)) {
        entry.rt6_flags = RTF_UP | RTF_GATEWAY | RTF_RUNNING | RTF_LOCAL;
      } else {
        entry.rt6_flags = RTF_UP | RTF_GATEWAY | RTF_LOCAL;
      }
      
      func(&entry, arg0, arg1, arg2, arg3, arg4, arg5);
    }
  }
}

/* walk call all route */
void  rt6_traversal_entry (VOIDFUNCPTR func, void *arg0, void *arg1, 
                           void *arg2, void *arg3, void *arg4, void *arg5)
{
  rt6_bi_mbase = -1;
  rt6_traversal_table(func, arg0, arg1, arg2, arg3, arg4, arg5);
  rt6_traversal_nd6(func, arg0, arg1, arg2, arg3, arg4, arg5);
  rt6_traversal_buildin(func, arg0, arg1, arg2, arg3, arg4, arg5);
}

/* find route entry */
struct rt6_entry *rt6_find_entry (const ip6_addr_t *ip6dest, 
                                  const ip6_addr_t *ip6netmask, 
                                  const ip6_addr_t *ip6gateway, 
                                  const char *ifname, u_long host)
{
  int hash = RT6_HASH_INDEX(ip6dest);
  LW_LIST_LINE *pline;
  struct rt6_entry *entry;
  
  for (pline = rt6_table[hash]; pline != NULL; pline = _list_line_get_next(pline)) {
    entry = (struct rt6_entry *)pline;
    if (((host & RTF_HOST) == (entry->rt6_flags & RTF_HOST)) &&
        (ip6_addr_cmp(&entry->rt6_dest, ip6dest))) {
      if (!ip6_addr_isany(ip6netmask)) {
        if (!ip6_addr_cmp(&entry->rt6_netmask, ip6netmask)) {
          continue;
        }
      }
      if (!ip6_addr_isany(ip6gateway)) {
        if (!ip6_addr_cmp(&entry->rt6_gateway, ip6gateway)) {
          continue;
        }
      }
      if (ifname && ifname[0]) {
        if (lib_strcmp(entry->rt6_ifname, ifname)) {
          continue;
        }
      }
      return (entry);
    }
  }
  
  return (NULL);
}

/* add route entry */
int  rt6_add_entry (struct rt6_entry *entry)
{
  int i;
  int hash = RT6_HASH_INDEX(&entry->rt6_dest);
  LW_LIST_LINE *pline;
  struct rt6_entry *entry_in;
  struct netif *netif, *netif_p = NULL;
  
  entry->rt6_flags &= ~RTF_DONE; /* not confirmed */
  entry->rt6_type = RT6_TYPE_USER;
  entry->rt6_pid = getpid();
  
  if (!(entry->rt6_flags & RTF_HOST)) { /* to sub net */
    if (ip6_addr_isany_val(entry->rt6_netmask)) {
      rt6_netmask_from_prefix(&entry->rt6_netmask, 64);
    }
  }
  
  if (entry->rt6_ifname[0] == '\0') { /* No network interface is specified */
    if (ip6_addr_isany_val(entry->rt6_gateway)) {
      return (-1);
    }
    NETIF_FOREACH(netif) {
      for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
        if (ip6_addr_isvalid(netif_ip6_addr_state(netif, i)) &&
            netif_ip6_addr_isstatic(netif, i) &&
            ip6_addr_netcmp(&entry->rt6_gateway, netif_ip6_addr(netif, i))) {
          netif_p = netif;
          break;
        }
      }
      if (netif_p) {
        break;
      }
    }
    if (!netif_p) {
      return (-1);
    }
    netif_get_name(netif_p, entry->rt6_ifname);
    entry->rt6_netif = netif_p;
  
  } else {
    entry->rt6_netif = netif_find(entry->rt6_ifname);
  }
    
  if (entry->rt6_netif) {
    if (netif_is_up(entry->rt6_netif) && netif_is_link_up(entry->rt6_netif)) {
      entry->rt6_flags |= RTF_RUNNING;
      rt6_active++;
    } else {
      entry->rt6_flags &= ~RTF_RUNNING;
    }
  }
  
  rt6_total++;
  if (!rt6_table[hash]) { /* add to metric priority queue */
    _List_Line_Add_Ahead(&entry->rt6_list, &rt6_table[hash]);
  
  } else {
    entry_in = (struct rt6_entry *)rt6_table[hash];
    if (entry_in->rt6_metric >= entry->rt6_metric) {
      _List_Line_Add_Ahead(&entry->rt6_list, &rt6_table[hash]);
    
    } else {
      pline = rt6_table[hash];
      for (;;) {
        if (_list_line_get_next(pline)) {
          pline = _list_line_get_next(pline);
          entry_in = (struct rt6_entry *)pline;
          if (entry_in->rt6_metric >= entry->rt6_metric) {
            _List_Line_Add_Left(&entry->rt6_list, &entry_in->rt6_list);
            break;
          }
          
        } else {
          _List_Line_Add_Right(&entry->rt6_list, &entry_in->rt6_list);
          break;
        }
      }
    }
  }
  
  RT6_CACHE_INVAL(entry);
  if (entry->rt6_netif) {
    GW6_CACHE_INVAL(entry->rt6_netif->num, entry);
  }
  
  entry->rt6_flags |= RTF_DONE; /* confirmed */
  
  return (0);
}

/* delete route entry */
void rt6_delete_entry (struct rt6_entry *entry)
{
  int hash = RT6_HASH_INDEX(&entry->rt6_dest);
  
  if (entry->rt6_flags & RTF_RUNNING) {
    rt6_active--;
  }
  
  RT6_CACHE_INVAL(entry);
  if (entry->rt6_netif) {
    GW6_CACHE_INVAL(entry->rt6_netif->num, entry);
  }
  
  rt6_total--;
  _List_Line_Del(&entry->rt6_list, &rt6_table[hash]);
}

/* get route entry total num */
void rt6_total_entry (unsigned int *cnt)
{
  int count = 0;

  rt6_traversal_entry(rt6_counter, &count, NULL, NULL, NULL, NULL, NULL);
  if (cnt) {
    *cnt = count;
  }
}

/* netif_add_hook */
void  rt6_netif_add_hook (struct netif *netif)
{
  int i;
  char ifname[IF_NAMESIZE];
  LW_LIST_LINE *pline;
  struct rt6_entry *entry;
  
  netif_get_name(netif, ifname);
  
  RT_LOCK();
  for (i = 0; i < RT6_HASH_SIZE; i++) {
    for (pline = rt6_table[i]; pline != NULL; pline = _list_line_get_next(pline)) {
      entry = (struct rt6_entry *)pline;
      if (!entry->rt6_netif && !lib_strcmp(entry->rt6_ifname, ifname)) {
        entry->rt6_netif = netif;
        entry->rt6_flags |= RTF_RUNNING;
        rt6_active++;
      }
    }
  }
  RT_UNLOCK();
}

/* rt_netif_remove_hook */
void  rt6_netif_remove_hook (struct netif *netif)
{
  int i;
  LW_LIST_LINE *pline;
  struct rt6_entry *entry;
  
  RT_LOCK();
  for (i = 0; i < RT6_HASH_SIZE; i++) {
    for (pline = rt6_table[i]; pline != NULL; pline = _list_line_get_next(pline)) {
      entry = (struct rt6_entry *)pline;
      if (entry->rt6_netif == netif) {
        entry->rt6_netif = NULL;
        entry->rt6_flags &= ~RTF_RUNNING;
        RT6_CACHE_INVAL(entry);
        GW6_CACHE_INVAL(netif->num, entry);
        rt6_active--;
      }
    }
  }
  RT_UNLOCK();
}

/* rt6_netif_invcache_hook */
void  rt6_netif_invcache_hook (struct netif *netif)
{
  RT_LOCK();
  if (rt6_cache.rt6_entry) {
    if (rt6_cache.rt6_entry->rt6_netif == netif) {
      RT6_CACHE_INVAL(rt6_cache.rt6_entry);
      GW6_CACHE_INVAL(netif->num, rt6_cache.rt6_entry);
    }
  }
  RT_UNLOCK();
}

/* rt6_route_src_hook */
struct netif *rt6_route_search_hook (const ip6_addr_t *ipsrc, const ip6_addr_t *ip6dest)
{
  struct gw6_cache *gwc;
  struct rt6_entry *entry;
  
  if (!rt6_active) {
    return (NULL);
  }
  
  if (rt6_cache.rt6_entry && 
      RTF_VALID(rt6_cache.rt6_entry->rt6_flags)) {
    if (ip6_addr_cmp(&rt6_cache.rt6_dest, ip6dest)) {
      return (rt6_cache.rt6_entry->rt6_netif);
    
    } else if (!(rt6_cache.rt6_entry->rt6_flags & RTF_HOST)) {
      if (rt6_net_cmp(&rt6_cache.rt6_entry->rt6_dest, ip6dest, 
                      &rt6_cache.rt6_entry->rt6_netmask)) {
        ip6_addr_copy(rt6_cache.rt6_dest, *ip6dest);
        return (rt6_cache.rt6_entry->rt6_netif);
      }
    }
  }
  
  entry = rt6_match(ip6dest);
  if (entry) {
    rt6_cache.rt6_entry = entry;
    ip6_addr_copy(rt6_cache.rt6_dest, *ip6dest);
    gwc = &gw6_cache[entry->rt6_netif->num];
    gwc->rt6_entry = entry;
    ip6_addr_copy(gwc->rt6_dest, *ip6dest);
    ip6_addr_copy(gwc->rt6_gateway, entry->rt6_gateway);
    return (entry->rt6_netif);
  }
  
  return (NULL);
}

/* rt6_route_gateway_hook */
ip6_addr_t *rt6_route_gateway_hook(struct netif *netif, const ip6_addr_t *ip6dest)
{
  struct gw6_cache *gwc;
  struct rt6_entry *entry;

  if (!rt6_active) {
    return (NULL);
  }
  
  gwc = &gw6_cache[netif->num];
  if (gwc->rt6_entry &&
      RTF_VALID(gwc->rt6_entry->rt6_flags)) {
    if (ip6_addr_cmp(&gwc->rt6_dest, ip6dest)) {
      if (!ip6_addr_isany_val(gwc->rt6_gateway) &&
          (gwc->rt6_entry->rt6_netif == netif)) {
        return (&gwc->rt6_gateway);
      } else {
        return (NULL);
      }
    
    } else if (!(gwc->rt6_entry->rt6_flags & RTF_HOST)) {
      if (rt6_net_cmp(&gwc->rt6_entry->rt6_dest, ip6dest, 
                      &gwc->rt6_entry->rt6_netmask)) {
        ip6_addr_copy(gwc->rt6_dest, *ip6dest);
        if (!ip6_addr_isany_val(gwc->rt6_gateway) &&
            (gwc->rt6_entry->rt6_netif == netif)) {
          return (&gwc->rt6_gateway);
        } else {
          return (NULL);
        }
      }
    }
  }
  
  entry = rt6_match(ip6dest);
  if (entry) {
    gwc->rt6_entry = entry;
    ip6_addr_copy(gwc->rt6_dest, *ip6dest);
    ip6_addr_copy(gwc->rt6_gateway, entry->rt6_gateway);
    if (!ip6_addr_isany_val(gwc->rt6_gateway) &&
        (entry->rt6_netif == netif)) {
      return (&gwc->rt6_gateway);
    } else {
      return (NULL);
    }
  }
  
  return (NULL);
}

#endif /* LWIP_IPV6 */
#endif /* LW_CFG_NET_ROUTER */
/*
 * end
 */
