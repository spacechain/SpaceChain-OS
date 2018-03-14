/**
 * @file
 * Lwip platform independent net ipv6 source route.
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

#if LW_CFG_NET_ROUTER > 0 && LW_CFG_NET_BALANCING > 0

#include "lwip/inet.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "net/sroute.h"
#include "ip6_sroute.h"

#if LWIP_IPV6

/* flags is good or bad */
#define SRTF_GOOD            (RTF_UP | RTF_RUNNING)
#define SRTF_BAD             (RTF_REJECT | RTF_BLACKHOLE)
#define SRTF_VALID(flags)    (((flags & SRTF_GOOD) == SRTF_GOOD) && !(flags & SRTF_BAD))

/* source route table statistical variable */
static unsigned int  srt6_total, srt6_active;

/* source route hash table */
#define SRT_PRIO_TABLE_SIZE 2

static LW_LIST_LINE_HEADER  srt6_table[SRT_PRIO_TABLE_SIZE];

/* source route cache */
struct srt6_cache {
  struct srt6_entry *srt6_entry;
};

static struct srt6_cache  srt6_cache;

#define SRT6_CACHE_INVAL(sentry) \
        if (srt6_cache.srt6_entry == sentry) { \
          srt6_cache.srt6_entry = NULL; \
        }

/* ipv6 address cmp */
static int srt6_addr_cmp (const ip6_addr_t *ipaddr1_hbo, const ip6_addr_t *ipaddr2_hbo)
{
  if (ipaddr1_hbo->addr[0] < ipaddr2_hbo->addr[0]) {
    return (-1);
  } else if (ipaddr1_hbo->addr[0] > ipaddr2_hbo->addr[0]) {
    return (1);
  } else {
    if (ipaddr1_hbo->addr[1] < ipaddr2_hbo->addr[1]) {
      return (-1);
    } else if (ipaddr1_hbo->addr[1] > ipaddr2_hbo->addr[1]) {
      return (1);
    } else {
      if (ipaddr1_hbo->addr[2] < ipaddr2_hbo->addr[2]) {
        return (-1);
      } else if (ipaddr1_hbo->addr[2] > ipaddr2_hbo->addr[2]) {
        return (1);
      } else {
        if (ipaddr1_hbo->addr[3] < ipaddr2_hbo->addr[3]) {
          return (-1);
        } else if (ipaddr1_hbo->addr[3] > ipaddr2_hbo->addr[3]) {
          return (1);
        } else {
          return (0);
        }
      }
    }
  }
}

/* ipv6 address match check */
#define SRT6_MATCH_CHK(sentry, ip6src_hbo, ip6dest_hbo, matched_op) \
        if ((srt6_addr_cmp(ip6src_hbo, &(sentry)->srt6_ssrc_hbo) >= 0) && \
            (srt6_addr_cmp(ip6src_hbo, &(sentry)->srt6_esrc_hbo) <= 0)) { \
          if (sentry->srt6_mode == SRT_MODE_INCLUDE) { \
            if ((srt6_addr_cmp(ip6dest_hbo, &(sentry)->srt6_sdest_hbo) >= 0) && \
                (srt6_addr_cmp(ip6dest_hbo, &(sentry)->srt6_edest_hbo) <= 0)) { \
              matched_op; \
            } \
          } else { \
            if ((srt6_addr_cmp(ip6dest_hbo, &(sentry)->srt6_sdest_hbo) < 0) || \
                (srt6_addr_cmp(ip6dest_hbo, &(sentry)->srt6_edest_hbo) > 0)) { \
              matched_op; \
            } \
          } \
        }

/* source route table match */
static struct srt6_entry *srt6_match (LW_LIST_LINE_HEADER headr, const ip6_addr_t *ip6src_hbo, const ip6_addr_t *ip6dest_hbo)
{
  LW_LIST_LINE *pline;
  struct srt6_entry *sentry;

  for (pline = headr; pline != NULL; pline = _list_line_get_next(pline)) {
    sentry = (struct srt6_entry *)pline;
    if (SRTF_VALID(sentry->srt6_flags)) {
      SRT6_MATCH_CHK(sentry, ip6src_hbo, ip6dest_hbo, return (sentry));
    }
  }
  
  return (NULL);
}

/* source route table walk */
void srt6_traversal_entry (VOIDFUNCPTR func, void *arg0, void *arg1, 
                           void *arg2, void *arg3, void *arg4, void *arg5)
{
  INT i;
  LW_LIST_LINE *pline;
  struct srt6_entry *sentry;
  
  for (i = 0; i < SRT_PRIO_TABLE_SIZE; i++) {
    for (pline = srt6_table[i]; pline != NULL; pline = _list_line_get_next(pline)) {
      sentry = (struct srt6_entry *)pline;
      func(sentry, arg0, arg1, arg2, arg3, arg4, arg5);
    }
  }
}

/* search source route entry */
struct srt6_entry *srt6_search_entry (const ip6_addr_t *ip6src, const ip6_addr_t *ip6dest)
{
  INT i;
  LW_LIST_LINE *pline;
  struct srt6_entry *sentry;
  ip6_addr_t ip6src_hbo, ip6dest_hbo;

  srt6_addr_ntoh(&ip6src_hbo, ip6src);
  srt6_addr_ntoh(&ip6dest_hbo, ip6dest);
  for (i = 0; i < SRT_PRIO_TABLE_SIZE; i++) {
    for (pline = srt6_table[i]; pline != NULL; pline = _list_line_get_next(pline)) {
      sentry = (struct srt6_entry *)pline;
      if (SRTF_VALID(sentry->srt6_flags)) {
        SRT6_MATCH_CHK(sentry, &ip6src_hbo, &ip6dest_hbo, return (sentry));
      }
    }
  }

  return (NULL);
}

/* find source route entry */
struct srt6_entry *srt6_find_entry (const ip6_addr_t *ip6ssrc, const ip6_addr_t *ip6esrc,
                                    const ip6_addr_t *ip6sdest, const ip6_addr_t *ip6edest)
{
  INT i;
  LW_LIST_LINE *pline;
  struct srt6_entry *sentry;
  ip6_addr_t ip6ssrc_hbo, ip6esrc_hbo;
  ip6_addr_t ip6sdest_hbo, ip6edest_hbo;
  
  srt6_addr_ntoh(&ip6ssrc_hbo, ip6ssrc);
  srt6_addr_ntoh(&ip6esrc_hbo, ip6esrc);
  srt6_addr_ntoh(&ip6sdest_hbo, ip6sdest);
  srt6_addr_ntoh(&ip6edest_hbo, ip6edest);
  for (i = 0; i < SRT_PRIO_TABLE_SIZE; i++) {
    for (pline = srt6_table[i]; pline != NULL; pline = _list_line_get_next(pline)) {
      sentry = (struct srt6_entry *)pline;
      if (ip6_addr_cmp(&ip6ssrc_hbo, &sentry->srt6_ssrc_hbo) &&
          ip6_addr_cmp(&ip6esrc_hbo, &sentry->srt6_esrc_hbo) &&
          ip6_addr_cmp(&ip6sdest_hbo, &sentry->srt6_sdest_hbo) &&
          ip6_addr_cmp(&ip6edest_hbo, &sentry->srt6_edest_hbo)) {
        return (sentry);
      }
    }
  }
  
  return (NULL);
}

/* add source route entry */
int  srt6_add_entry (struct srt6_entry *sentry)
{
  sentry->srt6_flags &= ~RTF_DONE; /* not confirmed */
  sentry->srt6_flags |= RTF_HOST;
  
  if (sentry->srt6_ifname[0] == '\0') { /* No network interface is specified */
    return (-1);
  }
  
  sentry->srt6_netif = netif_find(sentry->srt6_ifname);
  if (sentry->srt6_netif) {
    if (netif_is_up(sentry->srt6_netif) && netif_is_link_up(sentry->srt6_netif)) {
      sentry->srt6_flags |= RTF_RUNNING;
      srt6_active++;
    } else {
      sentry->srt6_flags &= ~RTF_RUNNING;
    }
  }
  
  srt6_total++;
  if (sentry->srt6_prio == SRT_PRIO_HIGH) {
    _List_Line_Add_Ahead(&sentry->srt6_list, &srt6_table[SRT_PRIO_HIGH]);
  } else {
    _List_Line_Add_Ahead(&sentry->srt6_list, &srt6_table[SRT_PRIO_DEFAULT]);
  }
  
  SRT6_CACHE_INVAL(sentry);
  
  sentry->srt6_flags |= RTF_DONE; /* confirmed */
  
  return (0);
}

/* delete source route entry */
void srt6_delete_entry (struct srt6_entry *sentry)
{
  if (sentry->srt6_flags & RTF_RUNNING) {
    srt6_active--;
  }
  
  SRT6_CACHE_INVAL(sentry);
  
  srt6_total--;
  if (sentry->srt6_prio == SRT_PRIO_HIGH) {
    _List_Line_Del(&sentry->srt6_list, &srt6_table[SRT_PRIO_HIGH]);
  } else {
    _List_Line_Del(&sentry->srt6_list, &srt6_table[SRT_PRIO_DEFAULT]);
  }
}

/* get source route entry total num */
void srt6_total_entry (unsigned int *cnt)
{
  if (cnt) {
    *cnt = srt6_total;
  }
}

/* netif_add_hook */
void srt6_netif_add_hook (struct netif *netif)
{
  INT i;
  char ifname[IF_NAMESIZE];
  LW_LIST_LINE *pline;
  struct srt6_entry *sentry;
  
  netif_get_name(netif, ifname);
  
  SRT_LOCK();
  for (i = 0; i < SRT_PRIO_TABLE_SIZE; i++) {
    for (pline = srt6_table[i]; pline != NULL; pline = _list_line_get_next(pline)) {
      sentry = (struct srt6_entry *)pline;
      if (!sentry->srt6_netif && !lib_strcmp(sentry->srt6_ifname, ifname)) {
        sentry->srt6_netif = netif;
        sentry->srt6_flags |= RTF_RUNNING;
        srt6_active++;
      }
    }
  }
  SRT_UNLOCK();
}

/* rt_netif_remove_hook */
void srt6_netif_remove_hook (struct netif *netif)
{
  INT i;
  LW_LIST_LINE *pline;
  struct srt6_entry *sentry;
  
  SRT_LOCK();
  for (i = 0; i < SRT_PRIO_TABLE_SIZE; i++) {
    for (pline = srt6_table[i]; pline != NULL; pline = _list_line_get_next(pline)) {
      sentry = (struct srt6_entry *)pline;
      if (sentry->srt6_netif == netif) {
        sentry->srt6_netif = NULL;
        sentry->srt6_flags &= ~RTF_RUNNING;
        SRT6_CACHE_INVAL(sentry);
        srt6_active--;
      }
    }
  }
  SRT_UNLOCK();
}

/* rt_route_src_hook */
struct netif *srt6_route_search_hook (const ip6_addr_t *ip6src, const ip6_addr_t *ip6dest)
{
  struct srt6_entry *sentry;
  ip6_addr_t ip6src_hbo;
  ip6_addr_t ip6dest_hbo;
  
  if (!srt6_active) {
    return (NULL);
  }
  
  srt6_addr_ntoh(&ip6src_hbo, ip6src);
  srt6_addr_ntoh(&ip6dest_hbo, ip6dest);
  if (srt6_cache.srt6_entry && 
      SRTF_VALID(srt6_cache.srt6_entry->srt6_flags)) {
    SRT6_MATCH_CHK(srt6_cache.srt6_entry, &ip6src_hbo, &ip6dest_hbo, return (srt6_cache.srt6_entry->srt6_netif));
  }
  
  sentry = srt6_match(srt6_table[SRT_PRIO_HIGH], &ip6src_hbo, &ip6dest_hbo);
  if (sentry) {
    srt6_cache.srt6_entry = sentry;
    return (sentry->srt6_netif);
  }
  
  return (NULL);
}

/* rt_route_default_hook */
struct netif *srt6_route_default_hook (const ip6_addr_t *ip6src, const ip6_addr_t *ip6dest)
{
  struct srt6_entry *sentry;
  ip6_addr_t ip6src_hbo;
  ip6_addr_t ip6dest_hbo;

  if (!srt6_active) {
    return (NULL);
  }

  srt6_addr_ntoh(&ip6src_hbo, ip6src);
  srt6_addr_ntoh(&ip6dest_hbo, ip6dest);
  if (srt6_cache.srt6_entry &&
      SRTF_VALID(srt6_cache.srt6_entry->srt6_flags)) {
    SRT6_MATCH_CHK(srt6_cache.srt6_entry, &ip6src_hbo, &ip6dest_hbo, return (srt6_cache.srt6_entry->srt6_netif));
  }

  sentry = srt6_match(srt6_table[SRT_PRIO_DEFAULT], &ip6src_hbo, &ip6dest_hbo);
  if (sentry) {
    srt6_cache.srt6_entry = sentry;
    return (sentry->srt6_netif);
  }

  return (NULL);
}

#endif /* LWIP_IPV6 */
#endif /* LW_CFG_NET_ROUTER && LW_CFG_NET_BALANCING */
/*
 * end
 */
