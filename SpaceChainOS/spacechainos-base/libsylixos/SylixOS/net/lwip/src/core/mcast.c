/**
 * @file
 * Multicast filter module\n
 */

/*
 * Copyright (c) 2018 Han.hui
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
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Han.hui <hanhui@acoinfo.com>
 *
 */

#include "lwip/opt.h"

#if LWIP_UDP || LWIP_RAW /* don't build if not configured for use in lwipopts.h */

#include "lwip/err.h"
#include "lwip/udp.h"
#include "lwip/raw.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/netif.h"
#include "lwip/errno.h"
#include "lwip/ip_addr.h"
#include "lwip/sockets.h"

#include <stddef.h> /* We need offsetof() */

#if LWIP_IPV4 && LWIP_IGMP
/**
 * ipv4 multicast filter trigger function
 */
#if LWIP_IGMP_V3
static mcast_trigger_ip4_fn mcast_ip4_trigger;
#define IP4_MC_TRIGGER_CALL(netif, multi_addr) \
  if (mcast_ip4_trigger) { \
    mcast_ip4_trigger((netif), (multi_addr)); \
  }
#else
#define IP4_MC_TRIGGER_CALL(netif, multi_addr)
#endif /* !LWIP_IGMP_V3 */

/**
 * ipv4 multicast filter find
 */
struct ip4_mc *
mcast_ip4_mc_find(struct ip_mc *ipmc, struct netif *netif, const ip4_addr_t *multi_addr, struct ip4_mc **mc_prev)
{
  struct ip4_mc *prev = NULL;
  struct ip4_mc *mc;
  
  IP4_MC_FOREACH(ipmc, mc) {
    if (ip4_addr_cmp(&mc->if_addr, netif_ip4_addr(netif)) && 
        ip4_addr_cmp(&mc->multi_addr, multi_addr)) { /* check interface and multicast address */
      if (mc_prev) {
        *mc_prev = prev;
      }
      return mc; /* found! */
    }
    prev = mc;
  }
  
  return NULL; /* not found! */
}

/**
 * ipv4 multicast filter find source
 */
static struct igmp_src *
mcast_ip4_mc_src_find(struct ip4_mc *mc, const ip4_addr_t *src_addr, struct igmp_src **src_prev)
{
  struct igmp_src *prev = NULL;
  struct igmp_src *src;
  
  IP4_MC_SRC_FOREACH(mc, src) {
    if (ip4_addr_cmp(&src->src_addr, src_addr)) { /* check source address */
      if (src_prev) {
        *src_prev = prev;
      }
      return src; /* found! */
    }
    prev = src;
  }
  
  return NULL; /* not found! */
}

/**
 * ipv4 multicast filter remove all source
 */
static void
mcast_ip4_mc_src_remove(struct igmp_src *src)
{
  struct igmp_src *next;
  
  while (src) {
    next = src->next;
    mem_free(src);
    src = next;
  }
}

#if LWIP_IGMP_V3
/**
 * install ipv4 multicast filter trigger function
 */
void
mcast_ip4_filter_trigger_callback(mcast_trigger_ip4_fn trigger_callback)
{
  mcast_ip4_trigger = trigger_callback;
}

/**
 * ipv4 multicast filter group source infomation (use ip4_addr_p_t for IGMPv3 speedup)
 */
u16_t
mcast_ip4_filter_info(struct netif *netif, const ip4_addr_t *multi_addr, ip4_addr_p_t addr_array[], u16_t arr_cnt, u8_t *fmode)
{
  static ip4_addr_p_t in_tbl[LWIP_MCAST_SRC_TBL_SIZE];
  static ip4_addr_p_t ex_tbl[LWIP_MCAST_SRC_TBL_SIZE];
  
  struct ip4_mc *mc;
  struct igmp_src *src;
  ip4_addr_t addr; /* for compare speed */
  u16_t i, j;
  u16_t cnt, in_cnt = 0, ex_cnt = 0;
  u16_t max_cnt = (u16_t)((arr_cnt > LWIP_MCAST_SRC_TBL_SIZE) ? LWIP_MCAST_SRC_TBL_SIZE : arr_cnt);
  u8_t match = 0;
  
#define LWIP_IP4_MC_GET(pcb, pcbs, type, tbl, c) \
  for ((pcb) = (pcbs); (pcb) != NULL; (pcb) = (pcb)->next) { \
    mc = mcast_ip4_mc_find(&(pcb)->ipmc, netif, multi_addr, NULL); \
    if ((mc == NULL) || (mc->fmode != (type))) { \
      continue; \
    } \
    match = 1; /* group matched */ \
    IP4_MC_SRC_FOREACH(mc, src) { \
      if ((c) < max_cnt) { \
        ip4_addr_set(&(tbl)[(c)], &src->src_addr); /* save a source */ \
        (c)++; \
      } else { \
        *fmode = MCAST_EXCLUDE; /* table overflow, we need all this group packet */ \
        return (0); \
      } \
    } \
  }
  
#if LWIP_UDP
  {
    struct udp_pcb *pcb;
    LWIP_IP4_MC_GET(pcb, udp_pcbs, MCAST_INCLUDE, in_tbl, in_cnt); /* copy all udp include source address to in_tbl[] */
  }
#endif /* LWIP_UDP */
    
#if LWIP_RAW
  {
    struct raw_pcb *pcb;
    LWIP_IP4_MC_GET(pcb, raw_pcbs, MCAST_INCLUDE, in_tbl, in_cnt); /* copy all raw include source address to in_tbl[] */
  }
#endif /* LWIP_UDP */

#if LWIP_UDP
  {
    struct udp_pcb *pcb;
    LWIP_IP4_MC_GET(pcb, udp_pcbs, MCAST_EXCLUDE, ex_tbl, ex_cnt); /* copy all udp exclude source address to ex_tbl[] */
  }
#endif /* LWIP_UDP */
    
#if LWIP_RAW
  {
    struct raw_pcb *pcb;
    LWIP_IP4_MC_GET(pcb, raw_pcbs, MCAST_EXCLUDE, ex_tbl, ex_cnt); /* copy all raw exclude source address to ex_tbl[] */
  }
#endif /* LWIP_UDP */

  if (ex_cnt) { /* at least have one exclude source address */
    *fmode = MCAST_EXCLUDE;
    for (i = 0; i < ex_cnt; i++) {
      ip4_addr_set(&addr, &ex_tbl[i]);
      for (j = 0; j < in_cnt; j++) {
        if (ip4_addr_cmp(&addr, &in_tbl[j])) { /* check exclude conflict with include table */
          ip4_addr_set_any(&ex_tbl[i]); /* remove from exclude table */
          break;
        }
      }
    }
    
    for (i = 0, cnt = 0; i < ex_cnt; i++) {
      if (!ip4_addr_isany(&ex_tbl[i])) {
        ip4_addr_set(&addr_array[cnt], &ex_tbl[i]);
        cnt++;
      }
    }
    
  } else if (in_cnt) { /* at least have one include source address */
    *fmode = MCAST_INCLUDE;
    for (i = 0; i < in_cnt; i++) {
      ip4_addr_set(&addr_array[i], &in_tbl[i]);
    }
    cnt = i;
  
  } else {
    if (match) { /* at least have one pcb matched */
      *fmode = MCAST_EXCLUDE;
    } else { /* no match! */
      *fmode = MCAST_INCLUDE;
    }
    cnt = 0;
  }

  return (cnt);
}

/**
 * ipv4 multicast filter source address interest (use ip4_addr_p_t for IGMPv3 speedup)
 */
u8_t
mcast_ip4_filter_interest(struct netif *netif, const ip4_addr_t *multi_addr, const ip4_addr_p_t src_addr[], u16_t arr_cnt)
{
  static ip4_addr_p_t ip_tbl[LWIP_MCAST_SRC_TBL_SIZE];
  u16_t i, j, cnt;
  u8_t fmode;
  
  cnt = mcast_ip4_filter_info(netif, multi_addr, ip_tbl, LWIP_MCAST_SRC_TBL_SIZE, &fmode);
  if (fmode == MCAST_EXCLUDE) {
    for (i = 0; i < cnt; i++) {
      for (j = 0; j < arr_cnt; j++) {
        if (ip4_addr_cmp(&src_addr[j], &ip_tbl[i])) {
          return 0;
        }
      }
    }
    return 1;
    
  } else {
    for (i = 0; i < cnt; i++) {
      for (j = 0; j < arr_cnt; j++) {
        if (ip4_addr_cmp(&src_addr[j], &ip_tbl[i])) {
          return 1;
        }
      }
    }
    return 0;
  }
}
#endif /* LWIP_IGMP_V3 */
#endif /* LWIP_IPV4 && LWIP_IGMP */

#if LWIP_IPV6 && LWIP_IPV6_MLD
/**
 * ipv6 multicast filter trigger function
 */
#if LWIP_IPV6_MLD_V2
static mcast_trigger_ip6_fn mcast_ip6_trigger;
#define IP6_MC_TRIGGER_CALL(netif, multi_addr) \
  if (mcast_ip6_trigger) { \
    mcast_ip6_trigger((netif), (multi_addr)); \
  }
#else
#define IP6_MC_TRIGGER_CALL(netif, multi_addr)
#endif /* !LWIP_IPV6_MLD_V2 */

/**
 * ipv6 multicast filter find
 */
static struct ip6_mc *
mcast_ip6_mc_find(struct ip_mc *ipmc, struct netif *netif, const ip6_addr_t *multi_addr, struct ip6_mc **mc_prev)
{
  struct ip6_mc *prev = NULL;
  struct ip6_mc *mc;
  
  IP6_MC_FOREACH(ipmc, mc) {
    if (((mc->if_idx == NETIF_NO_INDEX) || (mc->if_idx == netif_get_index(netif))) && 
        ip6_addr_cmp_zoneless(&mc->multi_addr, multi_addr)) { /* check interface and multicast address */
      if (mc_prev) {
        *mc_prev = prev;
      }
      return mc; /* found! */
    }
    prev = mc;
  }
  
  return NULL; /* not found! */
}

/**
 * ipv6 multicast filter find source
 */
static struct mld6_src *
mcast_ip6_mc_src_find(struct ip6_mc *mc, const ip6_addr_t *src_addr, struct mld6_src **src_prev)
{
  struct mld6_src *prev = NULL;
  struct mld6_src *src;
  
  IP6_MC_SRC_FOREACH(mc, src) {
    if (ip6_addr_cmp_zoneless(&src->src_addr, src_addr)) { /* check source address */
      if (src_prev) {
        *src_prev = prev;
      }
      return src; /* found! */
    }
    prev = src;
  }
  
  return NULL; /* not found! */
}

/**
 * ipv6 multicast filter remove all source
 */
static void
mcast_ip6_mc_src_remove(struct mld6_src *src)
{
  struct mld6_src *next;
  
  while (src) {
    next = src->next;
    mem_free(src);
    src = next;
  }
}

#if LWIP_IPV6_MLD_V2
/**
 * install ipv6 multicast filter trigger function
 */
void
mcast_ip6_filter_trigger_callback(mcast_trigger_ip6_fn trigger_callback)
{
  mcast_ip6_trigger = trigger_callback;
}

/**
 * ipv6 multicast filter group source infomation (use ip6_addr_p_t for MLDv2 speedup)
 */
u16_t
mcast_ip6_filter_info(struct netif *netif, const ip6_addr_t *multi_addr, ip6_addr_p_t addr_array[], u16_t arr_cnt, u8_t *fmode)
{
  static ip6_addr_p_t in_tbl[LWIP_MCAST_SRC_TBL_SIZE];
  static ip6_addr_p_t ex_tbl[LWIP_MCAST_SRC_TBL_SIZE];
  
  struct ip6_mc *mc;
  struct mld6_src *src;
  ip6_addr_t addr; /* for compare speed */
  u16_t i, j;
  u16_t cnt, in_cnt = 0, ex_cnt = 0;
  u16_t max_cnt = (u16_t)((arr_cnt > LWIP_MCAST_SRC_TBL_SIZE) ? LWIP_MCAST_SRC_TBL_SIZE : arr_cnt);
  u8_t match = 0;
  
#define LWIP_IP6_MC_GET(pcb, pcbs, type, tbl, c) \
  for ((pcb) = (pcbs); (pcb) != NULL; (pcb) = (pcb)->next) { \
    mc = mcast_ip6_mc_find(&(pcb)->ipmc, netif, multi_addr, NULL); \
    if ((mc == NULL) || (mc->fmode != (type))) { \
      continue; \
    } \
    match = 1; \
    IP6_MC_SRC_FOREACH(mc, src) { \
      if ((c) < max_cnt) { \
        ip6_addr_copy_to_packed((tbl)[(c)], src->src_addr); /* save a source */ \
        (c)++; \
      } else { \
        *fmode = MCAST_EXCLUDE; /* table overflow, we need all this group packet */ \
        return (0); \
      } \
    } \
  }
  
#if LWIP_UDP
  {
    struct udp_pcb *pcb;
    LWIP_IP6_MC_GET(pcb, udp_pcbs, MCAST_INCLUDE, in_tbl, in_cnt); /* copy all udp include source address to in_tbl[] */
  }
#endif /* LWIP_UDP */
    
#if LWIP_RAW
  {
    struct raw_pcb *pcb;
    LWIP_IP6_MC_GET(pcb, raw_pcbs, MCAST_INCLUDE, in_tbl, in_cnt); /* copy all raw include source address to in_tbl[] */
  }
#endif /* LWIP_UDP */

#if LWIP_UDP
  {
    struct udp_pcb *pcb;
    LWIP_IP6_MC_GET(pcb, udp_pcbs, MCAST_EXCLUDE, ex_tbl, ex_cnt); /* copy all udp exclude source address to ex_tbl[] */
  }
#endif /* LWIP_UDP */
    
#if LWIP_RAW
  {
    struct raw_pcb *pcb;
    LWIP_IP6_MC_GET(pcb, raw_pcbs, MCAST_EXCLUDE, ex_tbl, ex_cnt); /* copy all raw exclude source address to ex_tbl[] */
  }
#endif /* LWIP_UDP */

  if (ex_cnt) { /* at least have one exclude source address */
    *fmode = MCAST_EXCLUDE;
    for (i = 0; i < ex_cnt; i++) {
      ip6_addr_copy_from_packed(addr, ex_tbl[i]);
      for (j = 0; j < in_cnt; j++) {
        if (ip6_addr_cmp_zoneless(&addr, &in_tbl[j])) { /* check exclude conflict with include table */
          ip6_addr_copy_to_packed(ex_tbl[i], *IP6_ADDR_ANY6); /* remove from exclude table */
          break;
        }
      }
    }
    
    for (i = 0, cnt = 0; i < ex_cnt; i++) {
      if (!ip6_addr_isany(&ex_tbl[i])) {
        ip6_addr_copy_to_packed(addr_array[cnt], ex_tbl[i]);
        cnt++;
      }
    }
    
  } else if (in_cnt) { /* at least have one include source address */
    *fmode = MCAST_INCLUDE;
    for (i = 0; i < in_cnt; i++) {
      ip6_addr_copy_to_packed(addr_array[i], in_tbl[i]);
    }
    cnt = i;
  
  } else {
    if (match) { /* at least have one pcb matched */
      *fmode = MCAST_EXCLUDE;
    } else { /* no match! */
      *fmode = MCAST_INCLUDE;
    }
    cnt = 0;
  }

  return (cnt);
}

/**
 * ipv6 multicast filter source address interest (use ip6_addr_p_t for MLDv2 speedup)
 */
u8_t
mcast_ip6_filter_interest(struct netif *netif, const ip6_addr_t *multi_addr, const ip6_addr_p_t src_addr[], u16_t arr_cnt)
{
  static ip6_addr_p_t ip_tbl[LWIP_MCAST_SRC_TBL_SIZE];
  u16_t i, j, cnt;
  u8_t fmode;
  
  cnt = mcast_ip6_filter_info(netif, multi_addr, ip_tbl, LWIP_MCAST_SRC_TBL_SIZE, &fmode);
  if (fmode == MCAST_EXCLUDE) {
    for (i = 0; i < cnt; i++) {
      for (j = 0; j < arr_cnt; j++) {
        if (ip6_addr_cmp_zoneless(&src_addr[j], &ip_tbl[i])) {
          return 0;
        }
      }
    }
    return 1;
    
  } else {
    for (i = 0; i < cnt; i++) {
      for (j = 0; j < arr_cnt; j++) {
        if (ip6_addr_cmp_zoneless(&src_addr[j], &ip_tbl[i])) {
          return 1;
        }
      }
    }
    return 0;
  }
}
#endif /* LWIP_IPV6_MLD_V2 */
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */

#if (LWIP_IPV4 && LWIP_IGMP) || (LWIP_IPV6 && LWIP_IPV6_MLD)
/** Join a multicast group (Can with a source specified)
 *
 * @param ipmc multicast filter control block
 * @param netif the network interface which should join a new group.
 * @param multi_addr the ipv6 address of the group to join
 * @param src_addr multicast source address (can be NULL)
 * @return lwIP error definitions
 */
err_t
mcast_join_netif(struct ip_mc *ipmc, struct netif *netif, const ip_addr_t *multi_addr, const ip_addr_t *src_addr)
{
#if LWIP_UDP
  if (ipmc->proto == IPPROTO_UDP) {
    err_t err;
    struct udp_pcb *pcb;
    /* prepare UDP pcb to udp_pcbs list */
    pcb = (struct udp_pcb *)((u8_t *)ipmc - offsetof(struct udp_pcb, ipmc));
    if (pcb->local_port == 0) {
      err = udp_bind(pcb, &pcb->local_ip, pcb->local_port);
      if (err != ERR_OK) {
        LWIP_DEBUGF(UDP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS, ("mcast_join_netif: forced port bind failed\n"));
        return err;
      }
    }
  }
#endif /* LWIP_UDP */

#if LWIP_IPV4 && LWIP_IGMP
  if (IP_IS_V4(multi_addr)) {
    struct ip4_mc *mc;
    struct igmp_src *src;
    
    mc = mcast_ip4_mc_find(ipmc, netif, ip_2_ip4(multi_addr), NULL);
    if (mc) {
      if (src_addr) {
        if ((mc->fmode == MCAST_EXCLUDE) && (mc->src)) {
          return ERR_VAL; /* filter mode not include mode */
        }
        src = mcast_ip4_mc_src_find(mc, ip_2_ip4(src_addr), NULL);
        if (src) {
          return ERR_ALREADY; /* already in source list */
        }
        
        src = (struct igmp_src *)mem_malloc(sizeof(struct igmp_src));
        if (src == NULL) {
          return ERR_MEM; /* no memory */
        }
        ip4_addr_set(&src->src_addr, ip_2_ip4(src_addr));
        src->next = mc->src;
        mc->src = src;
        mc->fmode = MCAST_INCLUDE; /* change to include mode */
        IP4_MC_TRIGGER_CALL(netif, ip_2_ip4(multi_addr)); /* trigger a report */
      }
      return ERR_OK;
    }
    
    mc = (struct ip4_mc *)mem_malloc(sizeof(struct ip4_mc)); /* Make a new mc */
    if (mc == NULL) {
      igmp_leavegroup_netif(netif, ip_2_ip4(multi_addr));
      return ERR_MEM; /* no memory */
    }
    ip4_addr_set(&mc->if_addr, netif_ip4_addr(netif));
    ip4_addr_set(&mc->multi_addr, ip_2_ip4(multi_addr));
    
    if (src_addr) { /* have a source specified */
      mc->fmode = MCAST_INCLUDE;
      src = (struct igmp_src *)mem_malloc(sizeof(struct igmp_src));
      if (src == NULL) {
        mem_free(mc);
        return ERR_MEM; /* no memory */
      }
      ip4_addr_set(&src->src_addr, ip_2_ip4(src_addr));
      src->next = NULL;
      mc->src = src;
      
    } else {
      mc->fmode = MCAST_EXCLUDE; /* no source specified */
      mc->src = NULL;
    }
    
    mc->next = ipmc->mc4;
    ipmc->mc4 = mc;
    igmp_joingroup_netif(netif, ip_2_ip4(multi_addr));
  } else 
#endif /* LWIP_IPV4 && LWIP_IGMP */
  {
#if LWIP_IPV6 && LWIP_IPV6_MLD
    struct ip6_mc *mc;
    struct mld6_src *src;
    
    mc = mcast_ip6_mc_find(ipmc, netif, ip_2_ip6(multi_addr), NULL);
    if (mc) {
      if (src_addr) {
        if ((mc->fmode == MCAST_EXCLUDE) && (mc->src)) {
          return ERR_VAL; /* filter mode not include mode */
        }
        src = mcast_ip6_mc_src_find(mc, ip_2_ip6(src_addr), NULL);
        if (src) {
          return ERR_ALREADY; /* already in source list */
        }
        
        src = (struct mld6_src *)mem_malloc(sizeof(struct mld6_src));
        if (src == NULL) {
          return ERR_MEM; /* no memory */
        }
        ip6_addr_set(&src->src_addr, ip_2_ip6(src_addr));
        src->next = mc->src;
        mc->src = src;
        mc->fmode = MCAST_INCLUDE; /* change to include mode */
        IP6_MC_TRIGGER_CALL(netif, ip_2_ip6(multi_addr)); /* trigger a report */
      }
      return ERR_OK;
    }
    
    mc = (struct ip6_mc *)mem_malloc(sizeof(struct ip6_mc)); /* Make a new mc */
    if (mc == NULL) {
      mld6_leavegroup_netif(netif, ip_2_ip6(multi_addr));
      return ERR_MEM; /* no memory */
    }
    mc->if_idx = netif_get_index(netif);
    ip6_addr_set(&mc->multi_addr, ip_2_ip6(multi_addr));
    
    if (src_addr) {
      mc->fmode = MCAST_INCLUDE;
      src = (struct mld6_src *)mem_malloc(sizeof(struct mld6_src));
      if (src == NULL) {
        mld6_leavegroup_netif(netif, ip_2_ip6(multi_addr));
        mem_free(mc);
        return ERR_MEM; /* no memory */
      }
      ip6_addr_set(&src->src_addr, ip_2_ip6(src_addr));
      src->next = NULL;
      mc->src = src;
      
    } else {
      mc->fmode = MCAST_EXCLUDE; /* no source specified */
      mc->src = NULL;
    }
    
    mc->next = ipmc->mc6;
    ipmc->mc6 = mc;
    mld6_joingroup_netif(netif, ip_2_ip6(multi_addr));
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */
  }
  return ERR_OK;
}

/** Join a multicast group (Can with a source specified)
 *
 * @param ipmc multicast filter control block
 * @param if_addr the network interface address.
 * @param multi_addr the ipv6 address of the group to join
 * @param src_addr multicast source address (can be NULL)
 * @return lwIP error definitions
 */
err_t
mcast_join_group(struct ip_mc *ipmc, const ip_addr_t *if_addr, const ip_addr_t *multi_addr, const ip_addr_t *src_addr)
{
  err_t err = ERR_VAL; /* no matching interface */
  
#if LWIP_IPV4 && LWIP_IGMP
  if (IP_IS_V4(multi_addr)) {
    struct netif *netif;
    
    NETIF_FOREACH(netif) {
      if ((netif->flags & NETIF_FLAG_IGMP) && ((ip4_addr_isany(ip_2_ip4(if_addr)) || ip4_addr_cmp(netif_ip4_addr(netif), ip_2_ip4(if_addr))))) {
        err = mcast_join_netif(ipmc, netif, multi_addr, src_addr);
        if (err != ERR_OK) {
          return (err);
        }
      }
    }
  } else 
#endif /* LWIP_IPV4 && LWIP_IGMP */
  {
#if LWIP_IPV6 && LWIP_IPV6_MLD
    struct netif *netif;
    
    NETIF_FOREACH(netif) {
      if (ip6_addr_isany(ip_2_ip6(if_addr)) ||
          netif_get_ip6_addr_match(netif, ip_2_ip6(if_addr)) >= 0) {
        err = mcast_join_netif(ipmc, netif, multi_addr, src_addr);
        if (err != ERR_OK) {
          return (err);
        }
      }
    }
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */
  }
  return err;
}

/** Leave or drop a source from group on a network interface.
 *
 * @param ipmc multicast filter control block
 * @param netif the network interface which should leave group.
 * @param multi_addr the address of the group to leave
 * @param src_addr multicast source address
 * @return lwIP error definitions
 */
err_t
mcast_leave_netif(struct ip_mc *ipmc, struct netif *netif, const ip_addr_t *multi_addr, const ip_addr_t *src_addr)
{
#if LWIP_IPV4 && LWIP_IGMP
  if (IP_IS_V4(multi_addr)) {
    struct ip4_mc *mc_prev;
    struct ip4_mc *mc;
    struct igmp_src *src_prev;
    struct igmp_src *src;
    
    mc = mcast_ip4_mc_find(ipmc, netif, ip_2_ip4(multi_addr), &mc_prev);
    if (mc == NULL) {
      return ERR_VAL;
    }
    
    if (src_addr) {
      if ((mc->fmode == MCAST_EXCLUDE) && (mc->src)) {
        return ERR_VAL; /* drop source membership must in include mode */
      }
      
      src = mcast_ip4_mc_src_find(mc, ip_2_ip4(src_addr), &src_prev);
      if (src) {
        if (src_prev) {
          src_prev->next = src->next;
        } else {
          mc->src = src->next;
        }
        mem_free(src);
      } else {
        return ERR_VAL;
      }
      if (mc->src) {
        IP4_MC_TRIGGER_CALL(netif, ip_2_ip4(multi_addr)); /* trigger a report */
        return ERR_OK;
      }
    } else { /* we want drop this group */
      mcast_ip4_mc_src_remove(mc->src);
    }
    
    igmp_leavegroup_netif(netif, ip_2_ip4(multi_addr));
    if (mc_prev) {
      mc_prev->next = mc->next;
    } else {
      ipmc->mc4 = mc->next;
    }
    mem_free(mc);
  } else
#endif /* LWIP_IPV4 && LWIP_IGMP */
  {
#if LWIP_IPV6 && LWIP_IPV6_MLD
    struct ip6_mc *mc_prev;
    struct ip6_mc *mc;
    struct mld6_src *src_prev;
    struct mld6_src *src;
    
    mc = mcast_ip6_mc_find(ipmc, netif, ip_2_ip6(multi_addr), &mc_prev);
    if (mc == NULL) {
      return ERR_VAL;
    }
    
    if (src_addr) {
      if ((mc->fmode == MCAST_EXCLUDE) && (mc->src)) {
        return ERR_VAL; /* drop source membership must in include mode */
      }
      
      src = mcast_ip6_mc_src_find(mc, ip_2_ip6(src_addr), &src_prev);
      if (src) {
        if (src_prev) {
          src_prev->next = src->next;
        } else {
          mc->src = src->next;
        }
        mem_free(src);
      } else {
        return ERR_VAL;
      }
      if (mc->src) {
        IP6_MC_TRIGGER_CALL(netif, ip_2_ip6(multi_addr)); /* trigger a report */
        return ERR_OK;
      }
    } else { /* we want drop this group */
      mcast_ip6_mc_src_remove(mc->src);
    }

    mld6_leavegroup_netif(netif, ip_2_ip6(multi_addr));
    if (mc_prev) {
      mc_prev->next = mc->next;
    } else {
      ipmc->mc6 = mc->next;
    }
    mem_free(mc);
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */
  }
  return ERR_OK;
}

/** Leave or drop a source from group on a network interface.
 *
 * @param ipmc multicast filter control block
 * @param if_addr the network interface address.
 * @param multi_addr the address of the group to leave
 * @param src_addr multicast source address
 * @return lwIP error definitions
 */
err_t
mcast_leave_group(struct ip_mc *ipmc, const ip_addr_t *if_addr, const ip_addr_t *multi_addr, const ip_addr_t *src_addr)
{
  err_t res, err = ERR_VAL; /* no matching interface */

#if LWIP_IPV4 && LWIP_IGMP
  if (IP_IS_V4(multi_addr)) {
    struct netif *netif;
    
    NETIF_FOREACH(netif) {
      if ((netif->flags & NETIF_FLAG_IGMP) && ((ip4_addr_isany(ip_2_ip4(if_addr)) || ip4_addr_cmp(netif_ip4_addr(netif), ip_2_ip4(if_addr))))) {
        res = mcast_leave_netif(ipmc, netif, multi_addr, src_addr);
        if (err != ERR_OK) {
          err = res;
        }
      }
    }
  } else 
#endif /* LWIP_IPV4 && LWIP_IGMP */
  {
#if LWIP_IPV6 && LWIP_IPV6_MLD
    struct netif *netif;
    
    NETIF_FOREACH(netif) {
      if (ip6_addr_isany(ip_2_ip6(if_addr)) ||
          netif_get_ip6_addr_match(netif, ip_2_ip6(if_addr)) >= 0) {
        res = mcast_leave_netif(ipmc, netif, multi_addr, src_addr);
        if (err != ERR_OK) {
          err = res;
        }
      }
    }
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */
  }
  return err;
}

/** Add a block source address to a multicast group
 *
 * @param ipmc multicast filter control block
 * @param netif the network interface which group we already join.
 * @param multi_addr the address of the group to add source
 * @param blk_addr block multicast source address
 * @return lwIP error definitions
 */
err_t
mcast_block_netif(struct ip_mc *ipmc, struct netif *netif, const ip_addr_t *multi_addr, const ip_addr_t *blk_addr)
{
#if LWIP_IPV4 && LWIP_IGMP
  if (IP_IS_V4(multi_addr)) {
    struct ip4_mc *mc;
    struct igmp_src *src;
    
    mc = mcast_ip4_mc_find(ipmc, netif, ip_2_ip4(multi_addr), NULL);
    if (mc == NULL) {
      return ERR_VAL;
    }
    if (mc->fmode != MCAST_EXCLUDE) { /* we must in exclude mode */
      return ERR_VAL;
    }
    
    src = mcast_ip4_mc_src_find(mc, ip_2_ip4(blk_addr), NULL);
    if (src == NULL) {
      src = (struct igmp_src *)mem_malloc(sizeof(struct igmp_src));
      if (src == NULL) {
        return ERR_MEM;
      }
      ip4_addr_set(&src->src_addr, ip_2_ip4(blk_addr));
      src->next = mc->src;
      mc->src = src;
      IP4_MC_TRIGGER_CALL(netif, ip_2_ip4(multi_addr)); /* trigger a report */
    }
  } else
#endif /* LWIP_IPV4 && LWIP_IGMP */
  {
#if LWIP_IPV6 && LWIP_IPV6_MLD
    struct ip6_mc *mc;
    struct mld6_src *src;
    
    mc = mcast_ip6_mc_find(ipmc, netif, ip_2_ip6(multi_addr), NULL);
    if (mc == NULL) {
      return ERR_VAL;
    }
    if (mc->fmode != MCAST_EXCLUDE) { /* we must in exclude mode */
      return ERR_VAL;
    }
    
    src = mcast_ip6_mc_src_find(mc, ip_2_ip6(blk_addr), NULL);
    if (src == NULL) {
      src = (struct mld6_src *)mem_malloc(sizeof(struct mld6_src));
      if (src == NULL) {
        return ERR_MEM;
      }
      ip6_addr_set(&src->src_addr, ip_2_ip6(blk_addr));
      src->next = mc->src;
      mc->src = src;
      IP6_MC_TRIGGER_CALL(netif, ip_2_ip6(multi_addr)); /* trigger a report */
    }
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */
  }
  return ERR_OK;
}

/** Add a block source address to a multicast group
 *
 * @param ipmc multicast filter control block
 * @param if_addr the network interface address.
 * @param multi_addr the address of the group to add source
 * @param blk_addr block multicast source address
 * @return lwIP error definitions
 */
err_t
mcast_block_group(struct ip_mc *ipmc, const ip_addr_t *if_addr, const ip_addr_t *multi_addr, const ip_addr_t *blk_addr)
{
  err_t err = ERR_VAL; /* no matching interface */
  
#if LWIP_IPV4 && LWIP_IGMP
  if (IP_IS_V4(multi_addr)) {
    struct netif *netif;
    
    NETIF_FOREACH(netif) {
      if ((netif->flags & NETIF_FLAG_IGMP) && ((ip4_addr_isany(ip_2_ip4(if_addr)) || ip4_addr_cmp(netif_ip4_addr(netif), ip_2_ip4(if_addr))))) {
        err = mcast_block_netif(ipmc, netif, multi_addr, blk_addr);
        if (err != ERR_OK) {
          return (err);
        }
      }
    }
  } else 
#endif /* LWIP_IPV4 && LWIP_IGMP */
  {
#if LWIP_IPV6 && LWIP_IPV6_MLD
    struct netif *netif;
    
    NETIF_FOREACH(netif) {
      if (ip6_addr_isany(ip_2_ip6(if_addr)) ||
          netif_get_ip6_addr_match(netif, ip_2_ip6(if_addr)) >= 0) {
        err = mcast_block_netif(ipmc, netif, multi_addr, blk_addr);
        if (err != ERR_OK) {
          return (err);
        }
      }
    }
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */
  }
  return err;
}

/** Remove a block source address from a multicast group
 *
 * @param ipmc multicast filter control block
 * @param netif the network interface which group we already join.
 * @param multi_addr the address of the group to add source
 * @param unblk_addr unblock multicast source address
 * @return lwIP error definitions
 */
err_t
mcast_unblock_netif(struct ip_mc *ipmc, struct netif *netif, const ip_addr_t *multi_addr, const ip_addr_t *unblk_addr)
{
#if LWIP_IPV4 && LWIP_IGMP
  if (IP_IS_V4(multi_addr)) {
    struct ip4_mc *mc;
    struct igmp_src *src_prev;
    struct igmp_src *src;
    
    mc = mcast_ip4_mc_find(ipmc, netif, ip_2_ip4(multi_addr), NULL);
    if (mc == NULL) {
      return ERR_VAL;
    }
    if (mc->fmode != MCAST_EXCLUDE) { /* we must in exclude mode */
      return ERR_VAL;
    }
    
    src = mcast_ip4_mc_src_find(mc, ip_2_ip4(unblk_addr), &src_prev);
    if (src == NULL) {
      return ERR_VAL;
    }
    if (src_prev) {
      src_prev->next = src->next;
    } else {
      mc->src = src->next;
    }
    mem_free(src);
    IP4_MC_TRIGGER_CALL(netif, ip_2_ip4(multi_addr)); /* trigger a report */
  } else
#endif /* LWIP_IPV4 && LWIP_IGMP */
  {
#if LWIP_IPV6 && LWIP_IPV6_MLD
    struct ip6_mc *mc;
    struct mld6_src *src_prev;
    struct mld6_src *src;
    
    mc = mcast_ip6_mc_find(ipmc, netif, ip_2_ip6(multi_addr), NULL);
    if (mc == NULL) {
      return ERR_VAL;
    }
    if (mc->fmode != MCAST_EXCLUDE) { /* we must in exclude mode */
      return ERR_VAL;
    }
    
    src = mcast_ip6_mc_src_find(mc, ip_2_ip6(unblk_addr), &src_prev);
    if (src == NULL) {
      return ERR_VAL;
    }
    if (src_prev) {
      src_prev->next = src->next;
    } else {
      mc->src = src->next;
    }
    mem_free(src);
    IP6_MC_TRIGGER_CALL(netif, ip_2_ip6(multi_addr)); /* trigger a report */
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */
  }
  return ERR_OK;
}

/** Remove a block source address from a multicast group
 *
 * @param ipmc multicast filter control block
 * @param if_addr the network interface address.
 * @param multi_addr the address of the group to add source
 * @param unblk_addr unblock multicast source address
 * @return lwIP error definitions
 */
err_t
mcast_unblock_group(struct ip_mc *ipmc, const ip_addr_t *if_addr, const ip_addr_t *multi_addr, const ip_addr_t *unblk_addr)
{
  err_t res, err = ERR_VAL; /* no matching interface */

#if LWIP_IPV4 && LWIP_IGMP
  if (IP_IS_V4(multi_addr)) {
    struct netif *netif;
    
    NETIF_FOREACH(netif) {
      if ((netif->flags & NETIF_FLAG_IGMP) && ((ip4_addr_isany(ip_2_ip4(if_addr)) || ip4_addr_cmp(netif_ip4_addr(netif), ip_2_ip4(if_addr))))) {
        res = mcast_unblock_netif(ipmc, netif, multi_addr, unblk_addr);
        if (err != ERR_OK) {
          err = res;
        }
      }
    }
  } else 
#endif /* LWIP_IPV4 && LWIP_IGMP */
  {
#if LWIP_IPV6 && LWIP_IPV6_MLD
    struct netif *netif;
    
    NETIF_FOREACH(netif) {
      if (ip6_addr_isany(ip_2_ip6(if_addr)) ||
          netif_get_ip6_addr_match(netif, ip_2_ip6(if_addr)) >= 0) {
        res = mcast_unblock_netif(ipmc, netif, multi_addr, unblk_addr);
        if (err != ERR_OK) {
          err = res;
        }
      }
    }
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */
  }
  return err;
}

#if LWIP_SOCKET
/** Set filter to a multicast group (ip_msfilter)
 *
 * @param ipmc multicast filter control block
 * @param netif the network interface which group we already join.
 * @param multi_addr the address of the group to add source
 * @param imsf filter
 * @return lwIP error definitions
 */
err_t
mcast_set_msfilter_netif(struct ip_mc *ipmc, struct netif *netif, const ip_addr_t *multi_addr, const struct ip_msfilter *imsf)
{
#if LWIP_IPV4 && LWIP_IGMP
  if (IP_IS_V4(multi_addr)) {
    struct ip4_mc *mc;
    struct igmp_src *src, *head = NULL;
    u32_t i;
    
    mc = mcast_ip4_mc_find(ipmc, netif, ip_2_ip4(multi_addr), NULL);
    if (mc == NULL) {
      return ERR_VAL;
    }
    
    for (i = 0; i < imsf->imsf_numsrc; i++) {
      src = (struct igmp_src *)mem_malloc(sizeof(struct igmp_src));
      if (src == NULL) {
        mcast_ip4_mc_src_remove(head);
        return ERR_MEM;
      }
      inet_addr_to_ip4addr(&src->src_addr, &imsf->imsf_slist[i]);
      src->next = head;
      head = src;
    }
    
    mc->fmode = (u8_t)imsf->imsf_fmode;
    mcast_ip4_mc_src_remove(mc->src);
    mc->src = head;
    IP4_MC_TRIGGER_CALL(netif, ip_2_ip4(multi_addr)); /* trigger a report */
  } else
#endif /* LWIP_IPV4 && LWIP_IGMP */
  {
#if LWIP_IPV6 && LWIP_IPV6_MLD
    return EADDRNOTAVAIL;
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */
  }
  return ERR_OK;
}

/** Set filter to a multicast group (group_filter)
 *
 * @param ipmc multicast filter control block
 * @param netif the network interface which group we already join.
 * @param multi_addr the address of the group to add source
 * @param gf filter
 * @return lwIP error definitions
 */
err_t
mcast_set_groupfilter_netif(struct ip_mc *ipmc, struct netif *netif, const ip_addr_t *multi_addr, const struct group_filter *gf)
{
#if LWIP_IPV4 && LWIP_IGMP
  if (IP_IS_V4(multi_addr)) {
    struct ip4_mc *mc;
    struct igmp_src *src, *head = NULL;
    u32_t i;
    
    mc = mcast_ip4_mc_find(ipmc, netif, ip_2_ip4(multi_addr), NULL);
    if (mc == NULL) {
      return ERR_VAL;
    }
    
    for (i = 0; i < gf->gf_numsrc; i++) {
      src = (struct igmp_src *)mem_malloc(sizeof(struct igmp_src));
      if (src == NULL) {
        mcast_ip4_mc_src_remove(head);
        return ERR_MEM;
      }
      inet_addr_to_ip4addr(&src->src_addr, &(((struct sockaddr_in *)&(gf->gf_slist[i]))->sin_addr));
      src->next = head;
      head = src;
    }
    
    mc->fmode = (u8_t)gf->gf_fmode;
    mcast_ip4_mc_src_remove(mc->src);
    mc->src = head;
    IP4_MC_TRIGGER_CALL(netif, ip_2_ip4(multi_addr)); /* trigger a report */
  } else
#endif /* LWIP_IPV4 && LWIP_IGMP */
  {
#if LWIP_IPV6 && LWIP_IPV6_MLD
    struct ip6_mc *mc;
    struct mld6_src *src, *head = NULL;
    u32_t i;
    
    mc = mcast_ip6_mc_find(ipmc, netif, ip_2_ip6(multi_addr), NULL);
    if (mc == NULL) {
      return ERR_VAL;
    }
    
    for (i = 0; i < gf->gf_numsrc; i++) {
      src = (struct mld6_src *)mem_malloc(sizeof(struct mld6_src));
      if (src == NULL) {
        mcast_ip6_mc_src_remove(head);
        return ERR_MEM;
      }
      inet6_addr_to_ip6addr(&src->src_addr, &(((struct sockaddr_in6 *)&(gf->gf_slist[i]))->sin6_addr));
      src->next = head;
      head = src;
    }
    
    mc->fmode = (u8_t)gf->gf_fmode;
    mcast_ip6_mc_src_remove(mc->src);
    mc->src = head;
    IP6_MC_TRIGGER_CALL(netif, ip_2_ip6(multi_addr)); /* trigger a report */
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */
  }
  return ERR_OK;
}

/** Get filter from a multicast group (ip_msfilter)
 *
 * @param ipmc multicast filter control block
 * @param netif the network interface which group we already join.
 * @param multi_addr the address of the group to add source
 * @param imsf filter
 * @param bufsz filter buffer size
 * @return lwIP error definitions
 */
err_t
mcast_get_msfilter_netif(struct ip_mc *ipmc, struct netif *netif, const ip_addr_t *multi_addr, struct ip_msfilter *imsf, socklen_t *bufsz)
{
#if LWIP_IPV4 && LWIP_IGMP
  if (IP_IS_V4(multi_addr)) {
    struct ip4_mc *mc;
    struct igmp_src *src;
    u32_t i, cnt;
    
    mc = mcast_ip4_mc_find(ipmc, netif, ip_2_ip4(multi_addr), NULL);
    if (mc == NULL) {
      return ERR_VAL;
    }
    
    cnt = 0;
    src = mc->src;
    while (src) {
      cnt++;
      src = src->next;
    }
    if (IP_MSFILTER_SIZE(cnt) > *bufsz) {
      return ERR_BUF;
    }
    *bufsz = IP_MSFILTER_SIZE(cnt);
    
    imsf->imsf_numsrc = cnt;
    src = mc->src;
    for (i = 0; i < cnt; i++) {
      inet_addr_from_ip4addr(&imsf->imsf_slist[i], &src->src_addr);
      src = src->next;
    }
    imsf->imsf_fmode = mc->fmode;
  } else
#endif /* LWIP_IPV4 && LWIP_IGMP */
  {
#if LWIP_IPV6 && LWIP_IPV6_MLD
    return ERR_VAL;
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */
  }
  return ERR_OK;
}

/** Get filter from a multicast group (group_filter)
 *
 * @param ipmc multicast filter control block
 * @param netif the network interface which group we already join.
 * @param multi_addr the address of the group to add source
 * @param gf filter
 * @param bufsz filter buffer size
 * @return lwIP error definitions
 */
err_t
mcast_get_groupfilter_netif(struct ip_mc *ipmc, struct netif *netif, const ip_addr_t *multi_addr, struct group_filter *gf, socklen_t *bufsz)
{
#if LWIP_IPV4 && LWIP_IGMP
  if (IP_IS_V4(multi_addr)) {
    struct ip4_mc *mc;
    struct igmp_src *src;
    u32_t i, cnt;
    
    mc = mcast_ip4_mc_find(ipmc, netif, ip_2_ip4(multi_addr), NULL);
    if (mc == NULL) {
      return ERR_VAL;
    }
    
    cnt = 0;
    src = mc->src;
    while (src) {
      cnt++;
      src = src->next;
    }
    if (GROUP_FILTER_SIZE(cnt) > *bufsz) {
      return ERR_BUF;
    }
    *bufsz = GROUP_FILTER_SIZE(cnt);
    
    gf->gf_numsrc = cnt;
    src = mc->src;
    for (i = 0; i < cnt; i++) {
      inet_addr_from_ip4addr(&(((struct sockaddr_in *)&(gf->gf_slist[i]))->sin_addr), &src->src_addr);
      gf->gf_slist[i].ss_len = sizeof(struct sockaddr_in);
      gf->gf_slist[i].ss_family = AF_INET;
      src = src->next;
    }
    gf->gf_fmode = mc->fmode;
  } else
#endif /* LWIP_IPV4 && LWIP_IGMP */
  {
#if LWIP_IPV6 && LWIP_IPV6_MLD
    struct ip6_mc *mc;
    struct mld6_src *src;
    u32_t i, cnt;
    
    mc = mcast_ip6_mc_find(ipmc, netif, ip_2_ip6(multi_addr), NULL);
    if (mc == NULL) {
      return ERR_VAL;
    }
    
    cnt = 0;
    src = mc->src;
    while (src) {
      cnt++;
      src = src->next;
    }
    if (GROUP_FILTER_SIZE(cnt) > *bufsz) {
      return ERR_BUF;
    }
    *bufsz = GROUP_FILTER_SIZE(cnt);
    
    gf->gf_numsrc = cnt;
    src = mc->src;
    for (i = 0; i < cnt; i++) {
      inet6_addr_from_ip6addr(&(((struct sockaddr_in6 *)&(gf->gf_slist[i]))->sin6_addr), &src->src_addr);
      gf->gf_slist[i].ss_len = sizeof(struct sockaddr_in6);
      gf->gf_slist[i].ss_family = AF_INET6;
      src = src->next;
    }
    gf->gf_fmode = mc->fmode;
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */
  }
  return ERR_OK;
}
#endif /* LWIP_SOCKET */

/** Remove multicast filter when pcb remove
 *
 * @param ipmc multicast filter control block
 */
void
mcast_pcb_remove(struct ip_mc *ipmc)
{
#if LWIP_IPV4 && LWIP_IGMP
  {
    struct ip4_mc *mc, *next;
    
    mc = ipmc->mc4;
    while (mc) {
      next = mc->next;
      mcast_ip4_mc_src_remove(mc->src);
      igmp_leavegroup(&mc->if_addr, &mc->multi_addr);
      mem_free(mc);
      mc = next;
    }
  }
#endif /* LWIP_IPV4 && LWIP_IGMP */

#if LWIP_IPV6 && LWIP_IPV6_MLD
  {
    struct ip6_mc *mc, *next;
    struct netif *netif;
    
    mc = ipmc->mc6;
    while (mc) {
      next = mc->next;
      mcast_ip6_mc_src_remove(mc->src);
      netif = netif_get_by_index(mc->if_idx);
      if (netif) {
        mld6_leavegroup_netif(netif, &mc->multi_addr);
      }
      mem_free(mc);
      mc = next;
    }
  }
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */
}

/** Common code to see if the current input multicast packet matches the pcb
 * (current input packet is accessed via ip(4/6)_current_* macros)
 *
 * @param ipmc multicast filter control block
 * @param inp network interface on which the datagram was received (only used for IPv4)
 * @return 1 on match, 0 otherwise
 */
u8_t
mcast_input_local_match(struct ip_mc *ipmc, struct netif *inp)
{
#if LWIP_IPV4 && LWIP_IGMP
  if (!ip_current_is_v6()) {
    struct igmp_src *src;
    struct ip4_mc *mc;
    ip4_addr_t *multi_addr = ip4_current_dest_addr();
    ip4_addr_t *src_addr = ip4_current_src_addr();
    
    IP4_MC_FOREACH(ipmc, mc) {
      if (ip4_addr_cmp(&mc->if_addr, netif_ip4_addr(inp)) && 
          ip4_addr_cmp(&mc->multi_addr, multi_addr)) {
        if (mc->fmode == MCAST_EXCLUDE) {
          IP4_MC_SRC_FOREACH(mc, src) {
            if (ip4_addr_cmp(&src->src_addr, src_addr)) {
              return 0;
            }
          }
          return 1; /* MCAST_EXCLUDE src_addr must not in src list */
        
        } else { /* MCAST_INCLUDE */
          IP4_MC_SRC_FOREACH(mc, src) {
            if (ip4_addr_cmp(&src->src_addr, src_addr)) {
              return 1;
            }
          }
          return 0; /* MCAST_INCLUDE src_addr must in src list */
        }
      }
    }
  } else 
#endif /* LWIP_IPV4 && LWIP_IGMP */
  {
#if LWIP_IPV6 && LWIP_IPV6_MLD
    struct mld6_src *src;
    struct ip6_mc *mc;
    ip6_addr_t *multi_addr = ip6_current_dest_addr();
    ip6_addr_t *src_addr = ip6_current_src_addr();
    
    IP6_MC_FOREACH(ipmc, mc) {
      if (((mc->if_idx == NETIF_NO_INDEX) || (mc->if_idx == netif_get_index(inp))) &&
          ip6_addr_cmp_zoneless(&mc->multi_addr, multi_addr)) {
        if (mc->fmode == MCAST_EXCLUDE) {
          IP6_MC_SRC_FOREACH(mc, src) {
            if (ip6_addr_cmp_zoneless(&src->src_addr, src_addr)) {
              return 0;
            }
          }
          return 1; /* MCAST_EXCLUDE src_addr must not in src list */
          
        } else { /* MCAST_INCLUDE */
          IP6_MC_SRC_FOREACH(mc, src) {
            if (ip6_addr_cmp_zoneless(&src->src_addr, src_addr)) {
              return 1;
            }
          }
          return 0; /* MCAST_INCLUDE src_addr must in src list */
        }
      }
    }
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */
  }
  return 0;
}

#if LWIP_SOCKET
/**
 * setsockopt() IP_ADD_MEMBERSHIP / IP_DROP_MEMBERSHIP command
 */
int
mcast_sock_add_drop_membership(struct ip_mc *ipmc, int optname, const struct ip_mreq *imr)
{
  struct netif *netif;
  ip_addr_t if_addr;
  ip_addr_t multi_addr;
  err_t err;
  
  inet_addr_to_ip4addr(ip_2_ip4(&if_addr), &imr->imr_interface);
  inet_addr_to_ip4addr(ip_2_ip4(&multi_addr), &imr->imr_multiaddr);
  IP_SET_TYPE_VAL(if_addr, IPADDR_TYPE_V4);
  IP_SET_TYPE_VAL(multi_addr, IPADDR_TYPE_V4);
  
  if (!ip4_addr_ismulticast(ip_2_ip4(&multi_addr))) {
    return EADDRNOTAVAIL;
  }
  
  if (ip4_addr_isany(ip_2_ip4(&if_addr))) { /* To all network interface */
    if (optname == IP_ADD_MEMBERSHIP) {
      err = mcast_join_group(ipmc, &if_addr, &multi_addr, NULL);
    } else {
      err = mcast_leave_group(ipmc, &if_addr, &multi_addr, NULL);
    }
    
  } else { /* To specified network interface */
    if (!(ip_2_ip4(&if_addr)->addr & PP_HTONL(IP_CLASSA_NET))) { /* IS a BSD style index ? */
      u8_t idx = (u8_t)PP_NTOHL(ip_2_ip4(&if_addr)->addr);
      netif = netif_get_by_index(idx);
      if (netif == NULL) {
        return ENXIO;
      }
      *ip_2_ip4(&if_addr) = *netif_ip4_addr(netif);
  
    } else {
      NETIF_FOREACH(netif) {
        if (ip4_addr_cmp(ip_2_ip4(&if_addr), netif_ip4_addr(netif))) {
          break;
        }
      }
      if (netif == NULL) {
        return ENXIO;
      }
    }
  
    if (optname == IP_ADD_MEMBERSHIP) {
      err = mcast_join_netif(ipmc, netif, &multi_addr, NULL);
    } else {
      err = mcast_leave_netif(ipmc, netif, &multi_addr, NULL);
    }
  }
  
  return (u8_t)err_to_errno(err);
}

/**
 * setsockopt() IP_ADD_SOURCE_MEMBERSHIP / IP_DROP_SOURCE_MEMBERSHIP command
 */
int
mcast_sock_add_drop_source_membership(struct ip_mc *ipmc, int optname, const struct ip_mreq_source *imr)
{
  struct netif *netif;
  ip_addr_t if_addr;
  ip_addr_t multi_addr;
  ip_addr_t src_addr;
  err_t err;
  
  inet_addr_to_ip4addr(ip_2_ip4(&if_addr), &imr->imr_interface);
  inet_addr_to_ip4addr(ip_2_ip4(&multi_addr), &imr->imr_multiaddr);
  inet_addr_to_ip4addr(ip_2_ip4(&src_addr), &imr->imr_sourceaddr);
  IP_SET_TYPE_VAL(if_addr, IPADDR_TYPE_V4);
  IP_SET_TYPE_VAL(multi_addr, IPADDR_TYPE_V4);
  IP_SET_TYPE_VAL(src_addr, IPADDR_TYPE_V4);
  
  if (!ip4_addr_ismulticast(ip_2_ip4(&multi_addr)) || ip4_addr_isany(ip_2_ip4(&src_addr))) {
    return EADDRNOTAVAIL;
  }
  
  if (ip4_addr_isany(ip_2_ip4(&if_addr))) { /* To all network interface */
    if (optname == IP_ADD_SOURCE_MEMBERSHIP) {
      err = mcast_join_group(ipmc, &if_addr, &multi_addr, &src_addr);
    } else {
      err = mcast_leave_group(ipmc, &if_addr, &multi_addr, &src_addr);
    }
  
  } else { /* To specified network interface */
    if (!(ip_2_ip4(&if_addr)->addr & PP_HTONL(IP_CLASSA_NET))) { /* IS a BSD style index ? */
      u8_t idx = (u8_t)PP_NTOHL(ip_2_ip4(&if_addr)->addr);
      netif = netif_get_by_index(idx);
      if (netif == NULL) {
        return ENXIO;
      }
      *ip_2_ip4(&if_addr) = *netif_ip4_addr(netif);

    } else {
      NETIF_FOREACH(netif) {
        if (ip4_addr_cmp(ip_2_ip4(&if_addr), netif_ip4_addr(netif))) {
          break;
        }
      }
      if (netif == NULL) {
        return ENXIO;
      }
    }
  
    if (optname == IP_ADD_SOURCE_MEMBERSHIP) {
      err = mcast_join_netif(ipmc, netif, &multi_addr, &src_addr);
    } else {
      err = mcast_leave_netif(ipmc, netif, &multi_addr, &src_addr);
    }
  }
  
  return err_to_errno(err);
}

/**
 * setsockopt() IP_BLOCK_SOURCE / IP_UNBLOCK_SOURCE command
 */
int
mcast_sock_block_unblock_source(struct ip_mc *ipmc, int optname, const struct ip_mreq_source *imr)
{
  struct netif *netif;
  ip_addr_t if_addr;
  ip_addr_t multi_addr;
  ip_addr_t blk_addr;
  err_t err;
  
  inet_addr_to_ip4addr(ip_2_ip4(&if_addr), &imr->imr_interface);
  inet_addr_to_ip4addr(ip_2_ip4(&multi_addr), &imr->imr_multiaddr);
  inet_addr_to_ip4addr(ip_2_ip4(&blk_addr), &imr->imr_sourceaddr);
  IP_SET_TYPE_VAL(if_addr, IPADDR_TYPE_V4);
  IP_SET_TYPE_VAL(multi_addr, IPADDR_TYPE_V4);
  IP_SET_TYPE_VAL(blk_addr, IPADDR_TYPE_V4);
  
  if (!ip4_addr_ismulticast(ip_2_ip4(&multi_addr)) || ip4_addr_isany(ip_2_ip4(&blk_addr))) {
    return EADDRNOTAVAIL;
  }
  
  if (ip4_addr_isany(ip_2_ip4(&if_addr))) { /* To all network interface */
    if (optname == IP_BLOCK_SOURCE) {
      err = mcast_block_group(ipmc, &if_addr, &multi_addr, &blk_addr);
    } else {
      err = mcast_unblock_group(ipmc, &if_addr, &multi_addr, &blk_addr);
    }
    
  } else { /* To specified network interface */
    if (!(ip_2_ip4(&if_addr)->addr & PP_HTONL(IP_CLASSA_NET))) { /* IS a BSD style index ? */
      u8_t idx = (u8_t)PP_NTOHL(ip_2_ip4(&if_addr)->addr);
      netif = netif_get_by_index(idx);
      if (netif == NULL) {
        return ENXIO;
      }
      *ip_2_ip4(&if_addr) = *netif_ip4_addr(netif);
  
    } else {
      NETIF_FOREACH(netif) {
        if (ip4_addr_cmp(ip_2_ip4(&if_addr), netif_ip4_addr(netif))) {
          break;
        }
      }
      if (netif == NULL) {
        return ENXIO;
      }
    }
    
    if (optname == IP_BLOCK_SOURCE) {
      err = mcast_block_netif(ipmc, netif, &multi_addr, &blk_addr);
    } else {
      err = mcast_unblock_netif(ipmc, netif, &multi_addr, &blk_addr);
    }
  }
  
  return err_to_errno(err);
}

/**
 * setsockopt() IP_MSFILTER command
 */
int
mcast_sock_set_msfilter(struct ip_mc *ipmc, int optname, const struct ip_msfilter *imsf)
{
  struct netif *netif;
  ip_addr_t if_addr;
  ip_addr_t multi_addr;
  
  inet_addr_to_ip4addr(ip_2_ip4(&if_addr), &imsf->imsf_interface);
  inet_addr_to_ip4addr(ip_2_ip4(&multi_addr), &imsf->imsf_multiaddr);
  IP_SET_TYPE_VAL(if_addr, IPADDR_TYPE_V4);
  IP_SET_TYPE_VAL(multi_addr, IPADDR_TYPE_V4);
  
  if (!ip4_addr_ismulticast(ip_2_ip4(&multi_addr))) {
    return EADDRNOTAVAIL;
  }
  
  if (!(ip_2_ip4(&if_addr)->addr & PP_HTONL(IP_CLASSA_NET))) { /* IS a BSD style index ? */
    u8_t idx = (u8_t)PP_NTOHL(ip_2_ip4(&if_addr)->addr);
    netif = netif_get_by_index(idx);
    if (netif == NULL) {
      return ENXIO;
    }
    *ip_2_ip4(&if_addr) = *netif_ip4_addr(netif);
  
  } else {
    NETIF_FOREACH(netif) {
      if (ip4_addr_cmp(ip_2_ip4(&if_addr), netif_ip4_addr(netif))) {
        break;
      }
    }
    if (netif == NULL) {
      return ENXIO;
    }
  }
  
  return err_to_errno(mcast_set_msfilter_netif(ipmc, netif, &multi_addr, imsf));
}

/**
 * getsockopt() IP_MSFILTER command
 */
int
mcast_sock_get_msfilter(struct ip_mc *ipmc, int optname, struct ip_msfilter *imsf, socklen_t *size)
{
  struct netif *netif;
  ip_addr_t if_addr;
  ip_addr_t multi_addr;
  
  inet_addr_to_ip4addr(ip_2_ip4(&if_addr), &imsf->imsf_interface);
  inet_addr_to_ip4addr(ip_2_ip4(&multi_addr), &imsf->imsf_multiaddr);
  IP_SET_TYPE_VAL(if_addr, IPADDR_TYPE_V4);
  IP_SET_TYPE_VAL(multi_addr, IPADDR_TYPE_V4);
  
  if (!ip4_addr_ismulticast(ip_2_ip4(&multi_addr))) {
    return EADDRNOTAVAIL;
  }
  
  if (!(ip_2_ip4(&if_addr)->addr & PP_HTONL(IP_CLASSA_NET))) { /* IS a BSD style index ? */
    u8_t idx = (u8_t)PP_NTOHL(ip_2_ip4(&if_addr)->addr);
    netif = netif_get_by_index(idx);
    if (netif == NULL) {
      return ENXIO;
    }
    *ip_2_ip4(&if_addr) = *netif_ip4_addr(netif);
  
  } else {
    NETIF_FOREACH(netif) {
      if (ip4_addr_cmp(ip_2_ip4(&if_addr), netif_ip4_addr(netif))) {
        break;
      }
    }
    if (netif == NULL) {
      return ENXIO;
    }
  }
  
  return err_to_errno(mcast_get_msfilter_netif(ipmc, netif, &multi_addr, imsf, size));
}

/**
 * setsockopt() MCAST_JOIN_GROUP / MCAST_LEAVE_GROUP command
 */
int
mcast_sock_join_leave_group(struct ip_mc *ipmc, int optname, const struct group_req *gr)
{
  struct netif *netif;
  ip_addr_t multi_addr;
  err_t err;
  
  if (gr->gr_group.ss_family == AF_INET) {
    inet_addr_to_ip4addr(ip_2_ip4(&multi_addr), &(((struct sockaddr_in *)&(gr->gr_group))->sin_addr));
    IP_SET_TYPE_VAL(multi_addr, IPADDR_TYPE_V4);
    if (!ip4_addr_ismulticast(ip_2_ip4(&multi_addr))) {
      return EADDRNOTAVAIL;
    }
  
  } else if (gr->gr_group.ss_family == AF_INET6) {
    inet6_addr_to_ip6addr(ip_2_ip6(&multi_addr), &(((struct sockaddr_in6 *)&(gr->gr_group))->sin6_addr));
    IP_SET_TYPE_VAL(multi_addr, IPADDR_TYPE_V6);
    if (!ip6_addr_ismulticast(ip_2_ip6(&multi_addr))) {
      return EADDRNOTAVAIL;
    }

  } else {
    return EADDRNOTAVAIL;
  }

  if (gr->gr_interface) {
    netif = netif_get_by_index((u8_t)gr->gr_interface);
  } else {
    netif = netif_default; /* To default network interface */
  }
  if (netif == NULL) {
    return ENXIO;
  }
  
  if (optname == MCAST_JOIN_GROUP) {
    err = mcast_join_netif(ipmc, netif, &multi_addr, NULL);
  } else {
    err = mcast_leave_netif(ipmc, netif, &multi_addr, NULL);
  }
  
  return err_to_errno(err);
}

/**
 * setsockopt() MCAST_JOIN_SOURCE_GROUP / MCAST_LEAVE_SOURCE_GROUP command
 */
int
mcast_sock_join_leave_source_group(struct ip_mc *ipmc, int optname, const struct group_source_req *gsr)
{
  struct netif *netif;
  ip_addr_t multi_addr;
  ip_addr_t src_addr;
  err_t err;
  
  if (gsr->gsr_group.ss_family == AF_INET) {
    inet_addr_to_ip4addr(ip_2_ip4(&multi_addr), &(((struct sockaddr_in *)&(gsr->gsr_group))->sin_addr));
    inet_addr_to_ip4addr(ip_2_ip4(&src_addr), &(((struct sockaddr_in *)&(gsr->gsr_source))->sin_addr));
    IP_SET_TYPE_VAL(multi_addr, IPADDR_TYPE_V4);
    IP_SET_TYPE_VAL(src_addr, IPADDR_TYPE_V4);
    if (!ip4_addr_ismulticast(ip_2_ip4(&multi_addr)) || ip4_addr_isany(ip_2_ip4(&src_addr))) {
      return EADDRNOTAVAIL;
    }
  
  } else if (gsr->gsr_group.ss_family == AF_INET6) {
    inet6_addr_to_ip6addr(ip_2_ip6(&multi_addr), &(((struct sockaddr_in6 *)&(gsr->gsr_group))->sin6_addr));
    inet6_addr_to_ip6addr(ip_2_ip6(&src_addr), &(((struct sockaddr_in6 *)&(gsr->gsr_source))->sin6_addr));
    IP_SET_TYPE_VAL(multi_addr, IPADDR_TYPE_V6);
    IP_SET_TYPE_VAL(src_addr, IPADDR_TYPE_V6);
    if (!ip6_addr_ismulticast(ip_2_ip6(&multi_addr)) || ip6_addr_isany(ip_2_ip6(&src_addr))) {
      return EADDRNOTAVAIL;
    }
  
  } else {
    return EADDRNOTAVAIL;
  }
  
  if (gsr->gsr_interface) {
    netif = netif_get_by_index((u8_t)gsr->gsr_interface);
  } else {
    netif = netif_default; /* To default network interface */
  }
  if (netif == NULL) {
    return ENXIO;
  }
  
  if (optname == MCAST_JOIN_SOURCE_GROUP) {
    err = mcast_join_netif(ipmc, netif, &multi_addr, &src_addr);
  } else {
    err = mcast_leave_netif(ipmc, netif, &multi_addr, &src_addr);
  }
  
  return err_to_errno(err);
}

/**
 * setsockopt() MCAST_BLOCK_SOURCE / MCAST_UNBLOCK_SOURCE command
 */
int
mcast_sock_block_unblock_source_group(struct ip_mc *ipmc, int optname, const struct group_source_req *gsr)
{
  struct netif *netif;
  ip_addr_t multi_addr;
  ip_addr_t blk_addr;
  err_t err;
  
  if (gsr->gsr_group.ss_family == AF_INET) {
    inet_addr_to_ip4addr(ip_2_ip4(&multi_addr), &(((struct sockaddr_in *)&(gsr->gsr_group))->sin_addr));
    inet_addr_to_ip4addr(ip_2_ip4(&blk_addr), &(((struct sockaddr_in *)&(gsr->gsr_source))->sin_addr));
    IP_SET_TYPE_VAL(multi_addr, IPADDR_TYPE_V4);
    IP_SET_TYPE_VAL(blk_addr, IPADDR_TYPE_V4);
    if (!ip4_addr_ismulticast(ip_2_ip4(&multi_addr)) || ip4_addr_isany(ip_2_ip4(&blk_addr))) {
      return EADDRNOTAVAIL;
    }
  
  } else if (gsr->gsr_group.ss_family == AF_INET6) {
    inet6_addr_to_ip6addr(ip_2_ip6(&multi_addr), &(((struct sockaddr_in6 *)&(gsr->gsr_group))->sin6_addr));
    inet6_addr_to_ip6addr(ip_2_ip6(&blk_addr), &(((struct sockaddr_in6 *)&(gsr->gsr_source))->sin6_addr));
    IP_SET_TYPE_VAL(multi_addr, IPADDR_TYPE_V6);
    IP_SET_TYPE_VAL(blk_addr, IPADDR_TYPE_V6);
    if (!ip6_addr_ismulticast(ip_2_ip6(&multi_addr)) || ip6_addr_isany(ip_2_ip6(&blk_addr))) {
      return EADDRNOTAVAIL;
    }
  
  } else {
    return EADDRNOTAVAIL;
  }
  
  if (gsr->gsr_interface) {
    netif = netif_get_by_index((u8_t)gsr->gsr_interface);
  } else {
    netif = netif_default; /* To default network interface */
  }
  if (netif == NULL) {
    return ENXIO;
  }
  
  if (optname == MCAST_BLOCK_SOURCE) {
    err = mcast_block_netif(ipmc, netif, &multi_addr, &blk_addr);
  } else {
    err = mcast_unblock_netif(ipmc, netif, &multi_addr, &blk_addr);
  }
  
  return err_to_errno(err);
}

/**
 * setsockopt() MCAST_MSFILTER command
 */
int
mcast_sock_set_groupfilter(struct ip_mc *ipmc, int optname, const struct group_filter *gf)
{
  struct netif *netif;
  ip_addr_t multi_addr;
  
  if (gf->gf_group.ss_family == AF_INET) {
    inet_addr_to_ip4addr(ip_2_ip4(&multi_addr), &(((struct sockaddr_in *)&(gf->gf_group))->sin_addr));
    IP_SET_TYPE_VAL(multi_addr, IPADDR_TYPE_V4);
    if (!ip4_addr_ismulticast(ip_2_ip4(&multi_addr))) {
      return EADDRNOTAVAIL;
    }
  
  } else if (gf->gf_group.ss_family == AF_INET6) {
    inet6_addr_to_ip6addr(ip_2_ip6(&multi_addr), &(((struct sockaddr_in6 *)&(gf->gf_group))->sin6_addr));
    IP_SET_TYPE_VAL(multi_addr, IPADDR_TYPE_V6);
    if (!ip6_addr_ismulticast(ip_2_ip6(&multi_addr))) {
      return EADDRNOTAVAIL;
    }
  
  } else {
    return EADDRNOTAVAIL;
  }
  
  if (gf->gf_interface) {
    netif = netif_get_by_index((u8_t)gf->gf_interface);
  } else {
    netif = netif_default; /* To default network interface */
  }
  if (netif == NULL) {
    return ENXIO;
  }
  
  return err_to_errno(mcast_set_groupfilter_netif(ipmc, netif, &multi_addr, gf));
}

/**
 * getsockopt() MCAST_MSFILTER command
 */
int
mcast_sock_get_groupfilter(struct ip_mc *ipmc, int optname, struct group_filter *gf, socklen_t *size)
{
  struct netif *netif;
  ip_addr_t multi_addr;
  
  if (gf->gf_group.ss_family == AF_INET) {
    inet_addr_to_ip4addr(ip_2_ip4(&multi_addr), &(((struct sockaddr_in *)&(gf->gf_group))->sin_addr));
    IP_SET_TYPE_VAL(multi_addr, IPADDR_TYPE_V4);
    if (!ip4_addr_ismulticast(ip_2_ip4(&multi_addr))) {
      return EADDRNOTAVAIL;
    }
  
  } else if (gf->gf_group.ss_family == AF_INET6) {
    inet6_addr_to_ip6addr(ip_2_ip6(&multi_addr), &(((struct sockaddr_in6 *)&(gf->gf_group))->sin6_addr));
    IP_SET_TYPE_VAL(multi_addr, IPADDR_TYPE_V6);
    if (!ip6_addr_ismulticast(ip_2_ip6(&multi_addr))) {
      return EADDRNOTAVAIL;
    }
  
  } else {
    return EADDRNOTAVAIL;
  }
  
  if (gf->gf_interface) {
    netif = netif_get_by_index((u8_t)gf->gf_interface);
  } else {
    netif = netif_default; /* To default network interface */
  }
  if (netif == NULL) {
    return ENXIO;
  }
  
  return err_to_errno(mcast_get_groupfilter_netif(ipmc, netif, &multi_addr, gf, size));
}

/**
 * setsockopt() IPV6_JOIN_GROUP / IPV6_LEAVE_GROUP command
 */
int
mcast_sock_ipv6_add_drop_membership(struct ip_mc *ipmc, int optname, const struct ipv6_mreq *imr)
{
  struct netif *netif;
  ip_addr_t multi_addr;
  err_t err;
  
  inet6_addr_to_ip6addr(ip_2_ip6(&multi_addr), &imr->ipv6mr_multiaddr);
  IP_SET_TYPE_VAL(multi_addr, IPADDR_TYPE_V6);
  
  if (!ip6_addr_ismulticast(ip_2_ip6(&multi_addr))) {
    return EADDRNOTAVAIL;
  }
  
  if (imr->ipv6mr_interface) {
    netif = netif_get_by_index((u8_t)imr->ipv6mr_interface);
  } else {
    netif = netif_default; /* To default network interface */
  }
  if (netif == NULL) {
    return ENXIO;
  }
  
  if (optname == IPV6_JOIN_GROUP) {
    err = mcast_join_netif(ipmc, netif, &multi_addr, NULL);
  } else {
    err = mcast_leave_netif(ipmc, netif, &multi_addr, NULL);
  }
  
  return err_to_errno(err);
}

#endif /* LWIP_SOCKET */

#endif /* (LWIP_IPV4 && LWIP_IGMP) || (LWIP_IPV6 && LWIP_IPV6_MLD) */

#endif /* LWIP_UDP || LWIP_RAW */
