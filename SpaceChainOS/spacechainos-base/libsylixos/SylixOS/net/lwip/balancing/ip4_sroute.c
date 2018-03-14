/**
 * @file
 * Lwip platform independent net ipv4 source route.
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
#include "ip4_sroute.h"

/* flags is good or bad */
#define SRTF_GOOD            (RTF_UP | RTF_RUNNING)
#define SRTF_BAD             (RTF_REJECT | RTF_BLACKHOLE)
#define SRTF_VALID(flags)    (((flags & SRTF_GOOD) == SRTF_GOOD) && !(flags & SRTF_BAD))

/* source route table statistical variable */
static unsigned int  srt_total, srt_active;

/* source route table */
#define SRT_PRIO_TABLE_SIZE 2

static LW_LIST_LINE_HEADER  srt_table[SRT_PRIO_TABLE_SIZE];

/* source route cache */
struct srt_cache {
  struct srt_entry *srt_entry;
};

static struct srt_cache  srt_cache;

#define SRT_CACHE_INVAL(sentry) \
        if (srt_cache.srt_entry == sentry) { \
          srt_cache.srt_entry = NULL; \
        }

/* ipv4 address match check */
#define SRT_MATCH_CHK(sentry, ipsrc_hbo, ipdest_hbo, matched_op) \
        if (((ipsrc_hbo)->addr >= (sentry)->srt_ssrc_hbo.addr) && \
            ((ipsrc_hbo)->addr <= (sentry)->srt_esrc_hbo.addr)) { \
          if (sentry->srt_mode == SRT_MODE_INCLUDE) { \
            if (((ipdest_hbo)->addr >= (sentry)->srt_sdest_hbo.addr) && \
                ((ipdest_hbo)->addr <= (sentry)->srt_edest_hbo.addr)) { \
              matched_op; \
            } \
          } else { \
            if (((ipdest_hbo)->addr < (sentry)->srt_sdest_hbo.addr) || \
                ((ipdest_hbo)->addr > (sentry)->srt_edest_hbo.addr)) { \
              matched_op; \
            } \
          } \
        }

/* source route table match */
static struct srt_entry *srt_match (LW_LIST_LINE_HEADER headr, const ip4_addr_t *ipsrc_hbo, const ip4_addr_t *ipdest_hbo)
{
  LW_LIST_LINE *pline;
  struct srt_entry *sentry;

  for (pline = headr; pline != NULL; pline = _list_line_get_next(pline)) {
    sentry = (struct srt_entry *)pline;
    if (SRTF_VALID(sentry->srt_flags)) {
      SRT_MATCH_CHK(sentry, ipsrc_hbo, ipdest_hbo, return (sentry));
    }
  }
  
  return (NULL);
}

/* source route table walk */
void srt_traversal_entry (VOIDFUNCPTR func, void *arg0, void *arg1, 
                          void *arg2, void *arg3, void *arg4, void *arg5)
{
  INT i;
  LW_LIST_LINE *pline;
  struct srt_entry *sentry;
  
  for (i = 0; i < SRT_PRIO_TABLE_SIZE; i++) {
    for (pline = srt_table[i]; pline != NULL; pline = _list_line_get_next(pline)) {
      sentry = (struct srt_entry *)pline;
      func(sentry, arg0, arg1, arg2, arg3, arg4, arg5);
    }
  }
}

/* search source route entry */
struct srt_entry *srt_search_entry (const ip4_addr_t *ipsrc, const ip4_addr_t *ipdest)
{
  INT i;
  LW_LIST_LINE *pline;
  struct srt_entry *sentry;
  ip4_addr_t ipsrc_hbo, ipdest_hbo;

  ipsrc_hbo.addr = PP_NTOHL(ipsrc->addr);
  ipdest_hbo.addr = PP_NTOHL(ipdest->addr);
  for (i = 0; i < SRT_PRIO_TABLE_SIZE; i++) {
    for (pline = srt_table[i]; pline != NULL; pline = _list_line_get_next(pline)) {
      sentry = (struct srt_entry *)pline;
      if (SRTF_VALID(sentry->srt_flags)) {
        SRT_MATCH_CHK(sentry, &ipsrc_hbo, &ipdest_hbo, return (sentry));
      }
    }
  }

  return (NULL);
}

/* find source route entry */
struct srt_entry *srt_find_entry (const ip4_addr_t *ipssrc, const ip4_addr_t *ipesrc,
                                  const ip4_addr_t *ipsdest, const ip4_addr_t *ipedest)
{
  INT i;
  LW_LIST_LINE *pline;
  struct srt_entry *sentry;
  ip4_addr_t ipssrc_hbo, ipesrc_hbo;
  ip4_addr_t ipsdest_hbo, ipedest_hbo;
  
  ipssrc_hbo.addr = PP_NTOHL(ipssrc->addr);
  ipesrc_hbo.addr = PP_NTOHL(ipesrc->addr);
  ipsdest_hbo.addr = PP_NTOHL(ipsdest->addr);
  ipedest_hbo.addr = PP_NTOHL(ipedest->addr);
  for (i = 0; i < SRT_PRIO_TABLE_SIZE; i++) {
    for (pline = srt_table[i]; pline != NULL; pline = _list_line_get_next(pline)) {
      sentry = (struct srt_entry *)pline;
      if ((ipssrc_hbo.addr == sentry->srt_ssrc_hbo.addr) &&
          (ipesrc_hbo.addr == sentry->srt_esrc_hbo.addr) &&
          (ipsdest_hbo.addr == sentry->srt_sdest_hbo.addr) &&
          (ipedest_hbo.addr == sentry->srt_edest_hbo.addr)) {
        return (sentry);
      }
    }
  }
  
  return (NULL);
}

/* add source route entry */
int  srt_add_entry (struct srt_entry *sentry)
{
  sentry->srt_flags &= ~RTF_DONE; /* not confirmed */
  sentry->srt_flags |= RTF_HOST;
  
  if (sentry->srt_ifname[0] == '\0') { /* No network interface is specified */
    return (-1);
  }
  
  sentry->srt_netif = netif_find(sentry->srt_ifname);
  if (sentry->srt_netif) {
    if (netif_is_up(sentry->srt_netif) && netif_is_link_up(sentry->srt_netif)) {
      sentry->srt_flags |= RTF_RUNNING;
      srt_active++;
    } else {
      sentry->srt_flags &= ~RTF_RUNNING;
    }
  }
  
  srt_total++;
  if (sentry->srt_prio == SRT_PRIO_HIGH) {
    _List_Line_Add_Ahead(&sentry->srt_list, &srt_table[SRT_PRIO_HIGH]);
  } else {
    _List_Line_Add_Ahead(&sentry->srt_list, &srt_table[SRT_PRIO_DEFAULT]);
  }

  SRT_CACHE_INVAL(sentry);
  
  sentry->srt_flags |= RTF_DONE; /* confirmed */
  
  return (0);
}

/* delete source route entry */
void srt_delete_entry (struct srt_entry *sentry)
{
  if (sentry->srt_flags & RTF_RUNNING) {
    srt_active--;
  }
  
  SRT_CACHE_INVAL(sentry);
  
  srt_total--;
  if (sentry->srt_prio == SRT_PRIO_HIGH) {
    _List_Line_Del(&sentry->srt_list, &srt_table[SRT_PRIO_HIGH]);
  } else {
    _List_Line_Del(&sentry->srt_list, &srt_table[SRT_PRIO_DEFAULT]);
  }
}

/* get source route entry total num */
void srt_total_entry (unsigned int *cnt)
{
  if (cnt) {
    *cnt = srt_total;
  }
}

/* netif_add_hook */
void srt_netif_add_hook (struct netif *netif)
{
  INT i;
  char ifname[IF_NAMESIZE];
  LW_LIST_LINE *pline;
  struct srt_entry *sentry;
  
  netif_get_name(netif, ifname);
  
  SRT_LOCK();
  for (i = 0; i < SRT_PRIO_TABLE_SIZE; i++) {
    for (pline = srt_table[i]; pline != NULL; pline = _list_line_get_next(pline)) {
      sentry = (struct srt_entry *)pline;
      if (!sentry->srt_netif && !lib_strcmp(sentry->srt_ifname, ifname)) {
        sentry->srt_netif = netif;
        sentry->srt_flags |= RTF_RUNNING;
        srt_active++;
      }
    }
  }
  SRT_UNLOCK();
}

/* rt_netif_remove_hook */
void srt_netif_remove_hook (struct netif *netif)
{
  INT i;
  LW_LIST_LINE *pline;
  struct srt_entry *sentry;
  
  SRT_LOCK();
  for (i = 0; i < SRT_PRIO_TABLE_SIZE; i++) {
    for (pline = srt_table[i]; pline != NULL; pline = _list_line_get_next(pline)) {
      sentry = (struct srt_entry *)pline;
      if (sentry->srt_netif == netif) {
        sentry->srt_netif = NULL;
        sentry->srt_flags &= ~RTF_RUNNING;
        SRT_CACHE_INVAL(sentry);
        srt_active--;
      }
    }
  }
  SRT_UNLOCK();
}

/* rt_route_src_hook */
struct netif *srt_route_search_hook (const ip4_addr_t *ipsrc, const ip4_addr_t *ipdest)
{
  struct srt_entry *sentry;
  ip4_addr_t ipsrc_hbo;
  ip4_addr_t ipdest_hbo;

  if (!ipsrc || ipdest->addr == IPADDR_BROADCAST) {
    return (NULL);
  }
  
  if (!srt_active) {
    return (NULL);
  }
  
  ipsrc_hbo.addr = PP_NTOHL(ipsrc->addr);
  ipdest_hbo.addr = PP_NTOHL(ipdest->addr);
  if (srt_cache.srt_entry && 
      SRTF_VALID(srt_cache.srt_entry->srt_flags)) {
    SRT_MATCH_CHK(srt_cache.srt_entry, &ipsrc_hbo, &ipdest_hbo, return (srt_cache.srt_entry->srt_netif));
  }
  
  sentry = srt_match(srt_table[SRT_PRIO_HIGH], &ipsrc_hbo, &ipdest_hbo);
  if (sentry) {
    srt_cache.srt_entry = sentry;
    return (sentry->srt_netif);
  }
  
  return (NULL);
}

/* rt_route_default_hook */
struct netif *srt_route_default_hook (const ip4_addr_t *ipsrc, const ip4_addr_t *ipdest)
{
  struct srt_entry *sentry;
  ip4_addr_t ipsrc_hbo;
  ip4_addr_t ipdest_hbo;

  if (!ipsrc || ipdest->addr == IPADDR_BROADCAST) {
    return (NULL);
  }

  if (!srt_active) {
    return (NULL);
  }

  ipsrc_hbo.addr = PP_NTOHL(ipsrc->addr);
  ipdest_hbo.addr = PP_NTOHL(ipdest->addr);
  if (srt_cache.srt_entry &&
      SRTF_VALID(srt_cache.srt_entry->srt_flags)) {
    SRT_MATCH_CHK(srt_cache.srt_entry, &ipsrc_hbo, &ipdest_hbo, return (srt_cache.srt_entry->srt_netif));
  }

  sentry = srt_match(srt_table[SRT_PRIO_DEFAULT], &ipsrc_hbo, &ipdest_hbo);
  if (sentry) {
    srt_cache.srt_entry = sentry;
    return (sentry->srt_netif);
  }

  return (NULL);
}

#endif /* LW_CFG_NET_ROUTER && LW_CFG_NET_BALANCING */
/*
 * end
 */
