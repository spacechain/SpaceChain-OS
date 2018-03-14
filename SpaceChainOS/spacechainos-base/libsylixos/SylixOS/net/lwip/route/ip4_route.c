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
#include "unistd.h" /* for getpid() */

#if LW_CFG_NET_ROUTER > 0

#include "lwip/inet.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "net/route.h"
#include "ip4_route.h"

/* flags is good or bad */
#define RTF_GOOD            (RTF_UP | RTF_RUNNING)
#define RTF_BAD             (RTF_REJECT | RTF_BLACKHOLE)
#define RTF_VALID(flags)    (((flags & RTF_GOOD) == RTF_GOOD) && !(flags & RTF_BAD))

/* route table statistical variable */
static unsigned int  rt_total, rt_active;

/* buildin metric base */
static int rt_bi_mbase;

/* change default pid save */
static pid_t rt_pid_default;

/* route hash table */
#define RT_HASH_SHIFT   5
#define RT_HASH_SIZE    (1 << RT_HASH_SHIFT)
#define RT_HASH_MASK    (RT_HASH_SIZE - 1)

/* route hash index */
#if BYTE_ORDER == BIG_ENDIAN
#define RT_HASH_INDEX(ipaddr)   (((ipaddr)->addr >> (RT_HASH_SIZE - RT_HASH_SHIFT)) & RT_HASH_MASK)
#else
#define RT_HASH_INDEX(ipaddr)   (((ipaddr)->addr >> (8 - RT_HASH_SHIFT)) & RT_HASH_MASK)
#endif

/* route hash table */
static LW_LIST_LINE_HEADER  rt_table[RT_HASH_SIZE];

/* route cache */
struct rt_cache {
  struct rt_entry *rt_entry;
  ip4_addr_t rt_dest;
};

static struct rt_cache  rt_cache;

#define RT_CACHE_INVAL(entry) \
        if (rt_cache.rt_entry == entry) { \
          rt_cache.rt_entry = NULL; \
          rt_cache.rt_dest.addr = IPADDR_ANY; \
        }

/* gateway cache */
struct gw_cache {
  struct rt_entry *rt_entry;
  ip4_addr_t rt_dest;
  ip4_addr_t rt_gateway;
};

static struct gw_cache  gw_cache[LW_CFG_NET_DEV_MAX];

#define GW_CACHE_INVAL(num, entry) \
        LWIP_ASSERT("GW_CACHE_INVAL() num invalide!", (num) < LW_CFG_NET_DEV_MAX); \
        if (gw_cache[num].rt_entry == entry) { \
          gw_cache[num].rt_entry = NULL; \
          gw_cache[num].rt_dest.addr = IPADDR_ANY; \
          gw_cache[num].rt_gateway.addr = IPADDR_ANY; \
        }
        
/* netif is valid */
#define RT_NETIF_AVLID(netif) \
        ((netif_ip4_addr(netif)->addr != IPADDR_ANY) && \
         (netif_ip4_netmask(netif)->addr != IPADDR_ANY))

/* route table match */
static struct rt_entry *rt_match (const ip4_addr_t *ipdest)
{
  int hash = RT_HASH_INDEX(ipdest);
  LW_LIST_LINE *pline;
  struct rt_entry *entry;
  struct rt_entry *entry_net = NULL;
  
  for (pline = rt_table[hash]; pline != NULL; pline = _list_line_get_next(pline)) {
    entry = (struct rt_entry *)pline;
    if (RTF_VALID(entry->rt_flags)) {
      if (netif_is_up(entry->rt_netif) && netif_is_link_up(entry->rt_netif)) {
        if (entry->rt_flags & RTF_HOST) {
          if (entry->rt_dest.addr == ipdest->addr) {
            return (entry); /* match to host */
          }
      
        } else if (entry_net == NULL) {
          if (ip4_addr_netcmp(&entry->rt_dest, ipdest, &entry->rt_netmask)) {
            entry_net = entry; /* match to net */
          }
        }
      }
    }
  }
  
  return (entry_net);
}

/* route table cnt */
static void rt_counter (struct rt_entry *entry, int *cnt)
{
  (*cnt) += 1;
}

/* buildin route table walk call */
static void  rt_traversal_buildin (VOIDFUNCPTR func, void *arg0, void *arg1, 
                                   void *arg2, void *arg3, void *arg4, void *arg5)
{
  struct netif *netif;
  struct rt_entry entry;
  
  lib_bzero(&entry, sizeof(struct rt_entry));
  entry.rt_type = RT_TYPE_BUILDIN;
  entry.rt_metric = rt_bi_mbase + 1;
  
  if (netif_default) {
    if (RT_NETIF_AVLID(netif_default)) {
      entry.rt_dest.addr = IPADDR_ANY;
      entry.rt_netmask.addr = IPADDR_ANY;
      entry.rt_gateway.addr = netif_ip4_gw(netif_default)->addr;
      entry.rt_ifaddr.addr = netif_ip4_addr(netif_default)->addr;
      entry.rt_netif = netif_default;
      netif_get_name(netif_default, entry.rt_ifname);
      
      if (netif_is_up(netif_default) && netif_is_link_up(netif_default)) {
        entry.rt_flags = RTF_UP | RTF_GATEWAY | RTF_RUNNING | RTF_LOCAL;
      } else {
        entry.rt_flags = RTF_UP | RTF_GATEWAY | RTF_LOCAL;
      }
      
      func(&entry, arg0, arg1, arg2, arg3, arg4, arg5);
    }
  }
  
  NETIF_FOREACH(netif) {
    if (RT_NETIF_AVLID(netif)) {
      entry.rt_dest.addr = netif_ip4_addr(netif)->addr;  /* destination address */
      entry.rt_netmask.addr = IPADDR_NONE;
      entry.rt_gateway.addr = IPADDR_ANY;
      entry.rt_ifaddr.addr = netif_ip4_addr(netif)->addr;
      entry.rt_netif = netif;
      netif_get_name(netif, entry.rt_ifname);
    
      if (netif_is_up(netif) && netif_is_link_up(netif)) {
        entry.rt_flags = RTF_UP | RTF_HOST | RTF_RUNNING | RTF_LOCAL;
      } else {
        entry.rt_flags = RTF_UP | RTF_HOST | RTF_LOCAL;
      }
    
      func(&entry, arg0, arg1, arg2, arg3, arg4, arg5);
    
      entry.rt_netmask.addr = netif_ip4_netmask(netif)->addr;
      if (!ip4_addr_isloopback(&entry.rt_dest) &&
          ((netif->flags & NETIF_FLAG_BROADCAST) == 0)) {
        entry.rt_dest.addr = netif_ip4_gw(netif)->addr;
  
      } else {
        entry.rt_dest.addr = netif_ip4_addr(netif)->addr & netif_ip4_netmask(netif)->addr;
        entry.rt_flags &= ~RTF_HOST;
      }
    
      func(&entry, arg0, arg1, arg2, arg3, arg4, arg5);
    }
  }
}

/* route table walk call */
static void  rt_traversal_table (VOIDFUNCPTR func, void *arg0, void *arg1, 
                                 void *arg2, void *arg3, void *arg4, void *arg5)
{
  int i;
  LW_LIST_LINE *pline;
  struct rt_entry *entry;
  
  for (i = 0; i < RT_HASH_SIZE; i++) {
    for (pline = rt_table[i]; pline != NULL; pline = _list_line_get_next(pline)) {
      entry = (struct rt_entry *)pline;
      if (rt_bi_mbase < entry->rt_metric) {
        rt_bi_mbase = entry->rt_metric; /* buildin alway has low priority */
      }
      func(entry, arg0, arg1, arg2, arg3, arg4, arg5);
    }
  }
}

/* walk call all route */
void  rt_traversal_entry (VOIDFUNCPTR func, void *arg0, void *arg1, 
                          void *arg2, void *arg3, void *arg4, void *arg5)
{
  rt_bi_mbase = -1;
  rt_traversal_table(func, arg0, arg1, arg2, arg3, arg4, arg5);
  rt_traversal_buildin(func, arg0, arg1, arg2, arg3, arg4, arg5);
}

/* find route entry */
struct rt_entry *rt_find_entry (const ip4_addr_t *ipdest, 
                                const ip4_addr_t *ipnetmask, 
                                const ip4_addr_t *ipgateway,
                                const char *ifname, u_long host)
{
  int hash = RT_HASH_INDEX(ipdest);
  LW_LIST_LINE *pline;
  struct rt_entry *entry;
  
  for (pline = rt_table[hash]; pline != NULL; pline = _list_line_get_next(pline)) {
    entry = (struct rt_entry *)pline;
    if (((host & RTF_HOST) == (entry->rt_flags & RTF_HOST)) &&
        (entry->rt_dest.addr == ipdest->addr)) {
      if (!ip4_addr_isany(ipnetmask)) {
        if (entry->rt_netmask.addr != ipnetmask->addr) {
          continue;
        }
      }
      if (!ip4_addr_isany(ipgateway)) {
        if (entry->rt_gateway.addr != ipgateway->addr) {
          continue;
        }
      }
      if (ifname && ifname[0]) {
        if (lib_strcmp(entry->rt_ifname, ifname)) {
          continue;
        }
      }
      return (entry);
    }
  }
  
  return (NULL);
}

/* add route entry */
int  rt_add_entry (struct rt_entry *entry)
{
  int hash = RT_HASH_INDEX(&entry->rt_dest);
  LW_LIST_LINE *pline;
  struct rt_entry *entry_in;
  struct netif *netif, *netif_p = NULL;
  
  entry->rt_flags &= ~RTF_DONE; /* not confirmed */
  entry->rt_type = RT_TYPE_USER;
  entry->rt_pid  = getpid();
  
  if (!(entry->rt_flags & RTF_HOST)) { /* to sub net */
    if (entry->rt_netmask.addr == IPADDR_ANY) {
      if (IP_CLASSA(entry->rt_dest.addr)) {
        entry->rt_netmask.addr = IP_CLASSA_NET;
      } else if (IP_CLASSB(entry->rt_dest.addr)) {
        entry->rt_netmask.addr = IP_CLASSB_NET;
      } else if (IP_CLASSC(entry->rt_dest.addr)) {
        entry->rt_netmask.addr = IP_CLASSC_NET;
      }
    }
  }

  if (entry->rt_ifname[0] == '\0') { /* No network interface is specified */
    if (entry->rt_gateway.addr == IPADDR_ANY) {
      return (-1);
    }
    NETIF_FOREACH(netif) {
      if (entry->rt_gateway.addr == netif_ip4_gw(netif)->addr) {
        netif_p = netif;
        break;
      
      } else if (RT_NETIF_AVLID(netif)) {
        if (ip4_addr_netcmp(&entry->rt_gateway, 
                            netif_ip4_addr(netif), 
                            netif_ip4_netmask(netif))) {
          netif_p = netif;
        }
      }
    }
    if (!netif_p) {
      return (-1);
    }
    netif_get_name(netif_p, entry->rt_ifname);
    entry->rt_netif = netif_p;
    
  } else {
    entry->rt_netif = netif_find(entry->rt_ifname);
  }
  
  if (entry->rt_netif) {
    if (netif_is_up(entry->rt_netif) && netif_is_link_up(entry->rt_netif)) {
      entry->rt_flags |= RTF_RUNNING;
      rt_active++;
    } else {
      entry->rt_flags &= ~RTF_RUNNING;
    }
  }
  
  rt_total++;
  if (!rt_table[hash]) { /* add to metric priority queue */
    _List_Line_Add_Ahead(&entry->rt_list, &rt_table[hash]);
  
  } else {
    entry_in = (struct rt_entry *)rt_table[hash];
    if (entry_in->rt_metric >= entry->rt_metric) {
      _List_Line_Add_Ahead(&entry->rt_list, &rt_table[hash]);
    
    } else {
      pline = rt_table[hash];
      for (;;) {
        if (_list_line_get_next(pline)) {
          pline = _list_line_get_next(pline);
          entry_in = (struct rt_entry *)pline;
          if (entry_in->rt_metric >= entry->rt_metric) {
            _List_Line_Add_Left(&entry->rt_list, &entry_in->rt_list);
            break;
          }
          
        } else {
          _List_Line_Add_Right(&entry->rt_list, &entry_in->rt_list);
          break;
        }
      }
    }
  }
  
  RT_CACHE_INVAL(entry);
  if (entry->rt_netif) {
    GW_CACHE_INVAL(entry->rt_netif->num, entry);
  }
  
  entry->rt_flags |= RTF_DONE; /* confirmed */
  
  return (0);
}

/* delete route entry */
void rt_delete_entry (struct rt_entry *entry)
{
  int hash = RT_HASH_INDEX(&entry->rt_dest);
  
  if (entry->rt_flags & RTF_RUNNING) {
    rt_active--;
  }
  
  RT_CACHE_INVAL(entry);
  if (entry->rt_netif) {
    GW_CACHE_INVAL(entry->rt_netif->num, entry);
  }
  
  rt_total--;
  _List_Line_Del(&entry->rt_list, &rt_table[hash]);
}

/* get route entry total num */
void rt_total_entry (unsigned int *cnt)
{
  int count = 0;

  rt_traversal_entry(rt_counter, &count, NULL, NULL, NULL, NULL, NULL);
  if (cnt) {
    *cnt = count;
  }
}

/* change default */
int  rt_change_default (const ip4_addr_t *ipgateway, const char *ifname)
{
  struct netif *netif, *netif_p = LW_NULL;
  
  if (ifname && ifname[0]) {
    netif = netif_find(ifname);
    if (!netif) {
      return (-1);
    }
  } else {
    NETIF_FOREACH(netif) {
      if (ipgateway->addr == netif_ip4_gw(netif)->addr) {
        netif_p = netif;
        break;
      
      } else if (RT_NETIF_AVLID(netif)) {
        if (ip4_addr_netcmp(ipgateway, 
                            netif_ip4_addr(netif), 
                            netif_ip4_netmask(netif))) {
          netif_p = netif;
        }
      }
    }
    if (!netif_p) {
      return (-1);
    }
    netif = netif_p;
  }
  
  if (ipgateway->addr != IPADDR_NONE) { /* can not set to IPADDR_NONE */
    netif_set_gw(netif, ipgateway);
  }
  netif_set_default(netif);
  rt_pid_default = getpid();
  return (0);
}

/* get default */
int  rt_get_default (struct rt_entry *entry)
{
  lib_bzero(entry, sizeof(struct rt_entry));
  entry->rt_metric = 1;
  entry->rt_pid = rt_pid_default;
  
  if (netif_default) {
    entry->rt_dest.addr = IPADDR_ANY;
    entry->rt_netmask.addr = IPADDR_ANY;
    entry->rt_gateway.addr = netif_ip4_gw(netif_default)->addr;
    entry->rt_ifaddr.addr = netif_ip4_addr(netif_default)->addr;
    entry->rt_netif = netif_default;
    netif_get_name(netif_default, entry->rt_ifname);
    
    if (netif_is_up(netif_default) && netif_is_link_up(netif_default)) {
      entry->rt_flags = RTF_UP | RTF_GATEWAY | RTF_RUNNING | RTF_LOCAL;
    } else {
      entry->rt_flags = RTF_UP | RTF_GATEWAY | RTF_LOCAL;
    }
    return (0);
    
  } else {
    return (-1);
  }
}

/* netif_add_hook */
void  rt_netif_add_hook (struct netif *netif)
{
  int i;
  char ifname[IF_NAMESIZE];
  LW_LIST_LINE *pline;
  struct rt_entry *entry;
  
  netif_get_name(netif, ifname);
  
  RT_LOCK();
  for (i = 0; i < RT_HASH_SIZE; i++) {
    for (pline = rt_table[i]; pline != NULL; pline = _list_line_get_next(pline)) {
      entry = (struct rt_entry *)pline;
      if (!entry->rt_netif && !lib_strcmp(entry->rt_ifname, ifname)) {
        entry->rt_netif = netif;
        entry->rt_flags |= RTF_RUNNING;
        rt_active++;
      }
    }
  }
  RT_UNLOCK();
}

/* rt_netif_remove_hook */
void  rt_netif_remove_hook (struct netif *netif)
{
  int i;
  LW_LIST_LINE *pline;
  struct rt_entry *entry;
  
  RT_LOCK();
  for (i = 0; i < RT_HASH_SIZE; i++) {
    for (pline = rt_table[i]; pline != NULL; pline = _list_line_get_next(pline)) {
      entry = (struct rt_entry *)pline;
      if (entry->rt_netif == netif) {
        entry->rt_netif = NULL;
        entry->rt_flags &= ~RTF_RUNNING;
        RT_CACHE_INVAL(entry);
        GW_CACHE_INVAL(netif->num, entry);
        rt_active--;
      }
    }
  }
  RT_UNLOCK();
}

/* rt_netif_invcache_hook */
void  rt_netif_invcache_hook (struct netif *netif)
{
  RT_LOCK();
  if (rt_cache.rt_entry) {
    if (rt_cache.rt_entry->rt_netif == netif) {
      RT_CACHE_INVAL(rt_cache.rt_entry);
      GW_CACHE_INVAL(netif->num, rt_cache.rt_entry);
    }
  }
  RT_UNLOCK();
}

/* rt_route_src_hook */
struct netif *rt_route_search_hook (const ip4_addr_t *ipsrc, const ip4_addr_t *ipdest)
{
  struct netif *netif;
  struct gw_cache *gwc;
  struct rt_entry *entry;
  
  if ((ipdest->addr == IPADDR_BROADCAST) && 
      ipsrc && (ipsrc->addr != IPADDR_ANY)) {
    NETIF_FOREACH(netif) {
      if (ip4_addr_cmp(netif_ip4_addr(netif), ipsrc)) {
        return (netif);
      }
    }
  }
  
  if (!rt_active) {
    return (NULL);
  }
  
  if (rt_cache.rt_entry && 
      RTF_VALID(rt_cache.rt_entry->rt_flags)) {
    if (rt_cache.rt_dest.addr == ipdest->addr) {
      return (rt_cache.rt_entry->rt_netif);
    
    } else if (!(rt_cache.rt_entry->rt_flags & RTF_HOST)) {
      if (ip4_addr_netcmp(&rt_cache.rt_entry->rt_dest, ipdest, 
                          &rt_cache.rt_entry->rt_netmask)) {
        rt_cache.rt_dest.addr = ipdest->addr;
        return (rt_cache.rt_entry->rt_netif);
      }
    }
  }
  
  entry = rt_match(ipdest);
  if (entry) {
    rt_cache.rt_entry = entry;
    rt_cache.rt_dest.addr = ipdest->addr;
    gwc = &gw_cache[entry->rt_netif->num];
    gwc->rt_entry = entry;
    gwc->rt_dest.addr = ipdest->addr;
    gwc->rt_gateway.addr = entry->rt_gateway.addr;
    return (entry->rt_netif);
  }
  
  return (NULL);
}

/* rt_route_src_hook */
ip4_addr_t *rt_route_gateway_hook (struct netif *netif, const ip4_addr_t *ipdest)
{
  struct gw_cache *gwc;
  struct rt_entry *entry;

  if (!rt_active) {
    return (NULL);
  }
  
  gwc = &gw_cache[netif->num];
  if (gwc->rt_entry &&
      RTF_VALID(gwc->rt_entry->rt_flags)) {
    if (gwc->rt_dest.addr == ipdest->addr) {
      if ((gwc->rt_gateway.addr != IPADDR_ANY) &&
          (gwc->rt_entry->rt_netif == netif)) {
        return (&gwc->rt_gateway);
      } else {
        return (NULL);
      }
    
    } else if (!(gwc->rt_entry->rt_flags & RTF_HOST)) {
      if (ip4_addr_netcmp(&gwc->rt_entry->rt_dest, ipdest, 
                          &gwc->rt_entry->rt_netmask)) {
        gwc->rt_dest.addr = ipdest->addr;
        if ((gwc->rt_gateway.addr != IPADDR_ANY) &&
            (gwc->rt_entry->rt_netif == netif)) {
          return (&gwc->rt_gateway);
        } else {
          return (NULL);
        }
      }
    }
  }
  
  entry = rt_match(ipdest);
  if (entry) {
    gwc->rt_entry = entry;
    gwc->rt_dest.addr = ipdest->addr;
    gwc->rt_gateway.addr = entry->rt_gateway.addr;
    if ((gwc->rt_gateway.addr != IPADDR_ANY) &&
        (entry->rt_netif == netif)) {
      return (&gwc->rt_gateway);
    } else {
      return (NULL);
    }
  }
  
  return (NULL);
}

#endif /* LW_CFG_NET_ROUTER */
/*
 * end
 */
