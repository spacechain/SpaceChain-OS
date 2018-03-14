/**
 * @file
 * ipv6 multicast route.
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
#include "lwip/opt.h"

#if LW_CFG_NET_MROUTER > 0 && LWIP_IPV6

#include "ip6_mrt.h"
#include "net/if_flags.h"
#include "sys/time.h"
#include "lwip/icmp6.h"
#include "lwip/tcpip.h"
#include "lwip/timeouts.h"
#include "lwip/inet_chksum.h"

/* VIFI_INVALID */
#define MIFI_INVALID ((mifi_t)-1)

/* like mtod() */
#define ptod(p, type) (type)((p)->payload)

/* multicast route raw pcb */
static struct raw_pcb *ip6_mroute_pcb = NULL;

/* multicast statistical variable */
static struct mrt6stat mrt6stat;
static struct pim6stat pim6stat;

/* table of mf6c hash entries */
static LW_LIST_LINE_HEADER mf6ctable[MF6CTBLSIZ];
static u_char n6expire[MF6CTBLSIZ];
#define EXPIRE_TIMEOUT  (uint_t)(LW_TICK_HZ / 4) /* 4x / second */

/* table of virtual interfaces */
static struct mif6 mif6table[MAXMIFS];
static mifi_t nummifs = 0;
#define UPCALL_EXPIRE   6 /* number of timeouts */

/* Private variables. */
static mifi_t register_if6_num = MIFI_INVALID;
static struct netif multicast_register_if6;

/* multicast route init */
static int pim6_assert = 0;
static const struct timeval pim6_assert_interval = { 3, 0 };

/* Hash function for a source, group entry */
#define MF6CHASH(a, g) MF6CHASHMOD((a).s6_addr32[0] ^ (a).s6_addr32[1] ^ \
                                   (a).s6_addr32[2] ^ (a).s6_addr32[3] ^ \
                                   (g).s6_addr32[0] ^ (g).s6_addr32[1] ^ \
                                   (g).s6_addr32[2] ^ (g).s6_addr32[3])

/* internal function */
static err_t ip6_mrt_ip_mdq(struct pbuf *p, struct netif *ifp, struct mf6c *rt);

/* ip6_mrt_rate_check */
static int ip6_mrt_rate_check (struct timeval *tv, const struct timeval *period)
{
  struct timeval temp, now;
  int ret;
  
  GET_TIME(&now);
  
  timeradd(tv, period, &temp);
  if (timercmp(&temp, &now, <)) {
    ret = 1;
  } else {
    ret = 0;
  }
  
  tv->tv_sec = now.tv_sec;
  tv->tv_usec = now.tv_usec;
  
  return (ret);
}

/* ip6_mrt_rtdetq_free */
static void ip6_mrt_rtdetq_free (struct pbuf *p)
{
  struct rtdetq6 *rte = _LIST_ENTRY(p, struct rtdetq6, p);
  
  mem_free(rte);
}

/* ip6_mrt_rtdetq_alloc */
struct rtdetq6 *ip6_mrt_rtdetq_alloc (size_t bsz, size_t res)
{
  struct rtdetq6 *rte;
  struct pbuf *ret;
  u16_t tot_len = (u16_t)(res + bsz);
  
  rte = (struct rtdetq6 *)mem_malloc(ROUND_UP(sizeof(struct rtdetq6), MEM_ALIGNMENT) + tot_len);
  if (rte == NULL) {
    return (NULL);
  }
  
  rte->p.custom_free_function = ip6_mrt_rtdetq_free;
  
  ret = pbuf_alloced_custom(PBUF_RAW, tot_len, PBUF_RAM, &rte->p,
                            (char *)rte + ROUND_UP(sizeof(struct rtdetq6), MEM_ALIGNMENT), 
                            tot_len);
  
  if (ret && res) {
    pbuf_header(ret, (u16_t)-res);
  }
  
  return (rte);
}

/* Find a route for a given origin IPv6 address and Multicast group address. */
static LW_INLINE struct mf6c *ip6_mrt_find_mfc (struct in6_addr *origin, struct in6_addr *grp)
{
  LW_LIST_LINE *pline;
  struct mf6c *rt;
  
  MRT6STAT_INC(mrt6s_mfc_lookups);
  for (pline = mf6ctable[MF6CHASH(*origin, *grp)]; pline != NULL; pline = _list_line_get_next(pline)) {
    rt = _LIST_ENTRY(pline, struct mf6c, mf6c_list);
    if (IN6_ARE_ADDR_EQUAL(&rt->mf6c_origin.sin6_addr, origin) &&
        IN6_ARE_ADDR_EQUAL(&rt->mf6c_mcastgrp.sin6_addr, grp) &&
        (rt->mf6c_stall == NULL)) {
      break;
    }
  }
  
  if (pline == NULL) {
    MRT6STAT_INC(mrt6s_mfc_misses);
    return (NULL);
  }
  
  return (rt);
}

/* ip6_mrt_expire_mfc */
static void ip6_mrt_expire_mfc (struct mf6c *rt)
{
  u_long hash = MF6CHASH(rt->mf6c_origin.sin6_addr, rt->mf6c_mcastgrp.sin6_addr);

  while (rt->mf6c_stall) {
    struct rtdetq6 *rte = _LIST_ENTRY(rt->mf6c_stall, struct rtdetq6, list);
    _List_Ring_Del(&rte->list, &rt->mf6c_stall);
    mem_free(rte);
  }
  
  _List_Line_Del(&rt->mf6c_list, &mf6ctable[hash]);
  mem_free(rt);
}

/* ip6_mrt_expire_upcalls */
static void ip6_mrt_expire_upcalls (void *arg)
{
#if IP_DEBUG
  char ip6bufo[INET6_ADDRSTRLEN];
  char ip6bufg[INET6_ADDRSTRLEN];
#endif

  LW_LIST_LINE *pline;
  struct mf6c *rt;
  int i;

  for (i = 0; i < MF6CTBLSIZ; i++) {
    if (n6expire[i] == 0) {
      continue;
    }
    
    pline = mf6ctable[i];
    while (pline) {
      rt = _LIST_ENTRY(pline, struct mf6c, mf6c_list);
      pline = _list_line_get_next(pline);
      
      /* Skip real cache entries */
      if (rt->mf6c_stall == NULL) {
        continue;
      }
      
      /* If it expires now */
      if ((rt->mf6c_expire == 0) || (--rt->mf6c_expire > 0)) {
        continue;
      }

      LWIP_DEBUGF(IP6_DEBUG, ("ip6_mrt_expire_upcalls: expiring (%s %s)\n",
                  inet6_ntoa_r(rt->mf6c_origin.sin6_addr, ip6bufo, INET6_ADDRSTRLEN),
                  inet6_ntoa_r(rt->mf6c_mcastgrp.sin6_addr, ip6bufg, INET6_ADDRSTRLEN)));
        
      /* drop all the packets
       * free the pbuf with the pkt, if, timing info */
      ip6_mrt_expire_mfc(rt);
      
      MRT6STAT_INC(mrt6s_cache_cleanups);
      n6expire[i]--;
    }
  }

  sys_timeout(EXPIRE_TIMEOUT, ip6_mrt_expire_upcalls, NULL);
}

/* ip6_mrt_init */
static u8_t ip6_mrt_init (struct raw_pcb *pcb, int val)
{
  if (val == 0) {
    return (ENOPROTOOPT);
  }

  if (ip6_mroute_pcb) {
    return (EADDRINUSE);
  }
  
  ip6_mroute_pcb = pcb;
  
  lib_bzero(mf6ctable, sizeof(mf6ctable));
  lib_bzero(n6expire,  sizeof(n6expire));
  
  pim6_assert = 0;
  
  sys_timeout(EXPIRE_TIMEOUT, ip6_mrt_expire_upcalls, NULL);
  
  LWIP_DEBUGF(IP6_DEBUG, ("ip6_mrt_init\n"));
  return (0);
}

/* ip6_mrt_down */
static u8_t ip6_mrt_down (struct raw_pcb *pcb)
{
  int i, flags;
  mifi_t mifi;
  struct netif *ifp;
  struct ifreq ifr;
  
  if (ip6_mroute_pcb == NULL) {
    return (EINVAL);
  }
  
  /* For each phyint in use, disable promiscuous reception of all IPv6
   * multicasts. */
  for (mifi = 0; mifi < nummifs; mifi++) {
    if (mif6table[mifi].m6_ifp &&
      !(mif6table[mifi].m6_flags & MIFF_REGISTER)) {
      ifp = mif6table[mifi].m6_ifp;
      if (ifp) {
        flags = netif_get_flags(ifp);
        if (flags & IFF_ALLMULTI) {
          ifr.ifr_name[0] = 0;
          ifr.ifr_flags   = flags & ~IFF_ALLMULTI;
          if (ifp->ioctl) {
            if (ifp->ioctl(ifp, SIOCSIFFLAGS, &ifr)) {
              ((struct sockaddr_in6 *)&(ifr.ifr_addr))->sin6_family = AF_INET6;
              ((struct sockaddr_in6 *)&(ifr.ifr_addr))->sin6_addr = in6addr_any;
              ifp->ioctl(ifp, SIOCDELMULTI, &ifr);
            }
          }
        }
      }
    }
  }
  
  lib_bzero(mif6table, sizeof(mif6table));
  
  nummifs = 0;
  pim6_assert = 0;
  
  sys_untimeout(ip6_mrt_expire_upcalls, NULL);
  
  for (i = 0; i < MF6CTBLSIZ; i++) {
    while (mf6ctable[i]) {
      struct mf6c *rt = _LIST_ENTRY(mf6ctable[i], struct mf6c, mf6c_list);
      ip6_mrt_expire_mfc(rt);
    }
  }
  
  lib_bzero(mf6ctable, sizeof(mf6ctable));
  lib_bzero(n6expire,  sizeof(n6expire));
  
  /* unregister if */
  register_if6_num = MIFI_INVALID;
  
  ip6_mroute_pcb = NULL;
  
  LWIP_DEBUGF(IP6_DEBUG, ("ip6_mrt_down\n"));
  return (0);
}

/* ip6_mrt_add_mif */
static u8_t ip6_mrt_add_mif (struct mif6ctl *mifcp)
{
#if IP_DEBUG
  char ifname[NETIF_NAMESIZE];
#endif

  struct mif6 *mifp;
  struct netif *ifp;
  struct ifreq ifr;
  int flags;
  
  if (mifcp->mif6c_mifi >= MAXMIFS) {
    return (EINVAL);
  }
  
  mifp = mif6table + mifcp->mif6c_mifi;
  if (mifp->m6_ifp != NULL) {
    return (EADDRINUSE); /* XXX: is it appropriate? */
  }
  
  if ((mifcp->mif6c_pifi == 0) || 
      (mifcp->mif6c_pifi >= LW_CFG_NET_DEV_MAX)) {
    return (ENXIO);
  }
  
  ifp = netif_get_by_index((u8_t)mifcp->mif6c_pifi);
  if (mifcp->mif6c_flags & MIFF_REGISTER) {
    ifp = &multicast_register_if6;
    if (multicast_register_if6.flags == 0) {
      ifp->flags = NETIF_FLAG_LINK_UP | NETIF_FLAG_UP | NETIF_FLAG_IGMP;
      IP4_ADDR(ip_2_ip4(&((ifp)->ip_addr)), 127, 0, 0, 2);
      IP4_ADDR(ip_2_ip4(&((ifp)->netmask)), 255, 0, 0, 0);
      IP4_ADDR(ip_2_ip4(&((ifp)->gw)), 127, 0, 0, 2);
      ifp->name[0] = 'i';
      ifp->name[1] = 'm';
      ifp->num = (u8_t)mifcp->mif6c_pifi;
    }
    
    if (register_if6_num == MIFI_INVALID) {
      register_if6_num = mifcp->mif6c_pifi;
    }
  
  } else {
    /* Make sure the interface supports multicast */
    flags = netif_get_flags(ifp);
    if ((flags & IFF_MULTICAST) == 0) {
      return (EOPNOTSUPP);
    }
    
    if (!(flags & IFF_ALLMULTI)) {
      ifr.ifr_name[0] = 0;
      ifr.ifr_flags   = flags | IFF_ALLMULTI;
      if (ifp->ioctl) {
        if (ifp->ioctl(ifp, SIOCSIFFLAGS, &ifr)) {
          ((struct sockaddr_in6 *)&(ifr.ifr_addr))->sin6_family = AF_INET6;
          ((struct sockaddr_in6 *)&(ifr.ifr_addr))->sin6_addr = in6addr_any;
          if (ifp->ioctl(ifp, SIOCADDMULTI, &ifr)) {
            return (EOPNOTSUPP);
          }
        }
      } else {
        return (EOPNOTSUPP);
      }
    }
  }
  
  mifp->m6_flags     = mifcp->mif6c_flags;
  mifp->m6_ifp       = ifp;

  /* initialize per mif pkt counters */
  mifp->m6_pkt_in    = 0;
  mifp->m6_pkt_out   = 0;
  mifp->m6_bytes_in  = 0;
  mifp->m6_bytes_out = 0;

  /* Adjust nummifs up if the mifi is higher than nummifs */
  if (nummifs <= mifcp->mif6c_mifi) {
    nummifs = mifcp->mif6c_mifi + 1;
  }
  
  LWIP_DEBUGF(IP6_DEBUG, ("ip6_mrt_add_mif #%d, phyint %s", mifcp->mif6c_mifi,
              netif_get_name(ifp, ifname)));
  return (0);
}

/* ip6_mrt_del_vif */
static u8_t ip6_mrt_del_mif (mifi_t *mifip)
{
  struct mif6 *mifp = mif6table + *mifip;
  mifi_t mifi;
  struct netif *ifp;
  struct ifreq ifr;
  int flags;
  
  if (*mifip >= nummifs) {
    return (EINVAL);
  }
  
  if (mifp->m6_ifp == NULL) {
    return (EINVAL);
  }
  
  if (!(mifp->m6_flags & MIFF_REGISTER)) {
    ifp = mifp->m6_ifp;
    flags = netif_get_flags(ifp);
    if (flags & IFF_ALLMULTI) {
      ifr.ifr_name[0] = 0;
      ifr.ifr_flags   = flags & ~IFF_ALLMULTI;
      if (ifp->ioctl) {
        if (ifp->ioctl(ifp, SIOCSIFFLAGS, &ifr)) {
          ((struct sockaddr_in6 *)&(ifr.ifr_addr))->sin6_family = AF_INET6;
          ((struct sockaddr_in6 *)&(ifr.ifr_addr))->sin6_addr = in6addr_any;
          ifp->ioctl(ifp, SIOCDELMULTI, &ifr);
        }
      }
    }
    
  } else {
    register_if6_num = MIFI_INVALID;
  }
  
  lib_bzero(mifp, sizeof(*mifp));
  
  /* Adjust numvifs down */
  for (mifi = nummifs; mifi > 0; mifi--) {
    if (mif6table[mifi - 1].m6_ifp) {
      break;
    }
  }
  nummifs = mifi;
  
  LWIP_DEBUGF(IP6_DEBUG, ("ip6_mrt_del_vif %d, nummifs %d\n", *mifip, nummifs));
  return (0);
}

/* ip6_mrt_add_mfc */
static u8_t ip6_mrt_add_mfc (struct mf6cctl *mfccp)
{
#if IP_DEBUG
  char ip6bufo[INET6_ADDRSTRLEN];
  char ip6bufg[INET6_ADDRSTRLEN];
#endif

  LW_LIST_LINE *pline;
  struct mf6c *rt;
  u_long hash;
  u_short nstl;
  
  /* If an entry already exists, just update the fields */
  rt = ip6_mrt_find_mfc(&mfccp->mf6cc_origin.sin6_addr, &mfccp->mf6cc_mcastgrp.sin6_addr);
  if (rt) {
    LWIP_DEBUGF(IP6_DEBUG, ("ip6_mrt_add_mfc update o %x g %x p %x\n", 
                inet6_ntoa_r(rt->mf6c_origin.sin6_addr, ip6bufo, INET6_ADDRSTRLEN),
                inet6_ntoa_r(rt->mf6c_mcastgrp.sin6_addr, ip6bufg, INET6_ADDRSTRLEN),
                mfccp->mf6cc_parent));
    rt->mf6c_parent = mfccp->mf6cc_parent;
    rt->mf6c_ifset = mfccp->mf6cc_ifset;
    return (0);
  }
  
  /* Find the entry for which the upcall was made and update */
  hash = MF6CHASH(mfccp->mf6cc_origin.sin6_addr, mfccp->mf6cc_mcastgrp.sin6_addr);
  for (pline = mf6ctable[hash], nstl = 0; pline != NULL; pline = _list_line_get_next(pline)) {
    rt = _LIST_ENTRY(pline, struct mf6c, mf6c_list);
    if (IN6_ARE_ADDR_EQUAL(&rt->mf6c_origin.sin6_addr, &mfccp->mf6cc_origin.sin6_addr) &&
        IN6_ARE_ADDR_EQUAL(&rt->mf6c_mcastgrp.sin6_addr, &mfccp->mf6cc_mcastgrp.sin6_addr) &&
        (rt->mf6c_stall != NULL)) {
      if (nstl++) {
        LWIP_DEBUGF(IP6_DEBUG, ("ip6_mrt_add_m6fc: %s o %s g %s p %x dbx %p\n",
                    "multiple kernel entries",
                    inet6_ntoa_r(rt->mf6c_origin.sin6_addr, ip6bufo, INET6_ADDRSTRLEN),
                    inet6_ntoa_r(rt->mf6c_mcastgrp.sin6_addr, ip6bufg, INET6_ADDRSTRLEN),
                    mfccp->mf6cc_parent, rt->mf6c_stall));
      }
      
      LWIP_DEBUGF(IP6_DEBUG, ("o %s g %s p %x dbg %p",
                  inet6_ntoa_r(rt->mf6c_origin.sin6_addr, ip6bufo, INET6_ADDRSTRLEN),
                  inet6_ntoa_r(rt->mf6c_mcastgrp.sin6_addr, ip6bufg, INET6_ADDRSTRLEN),
                  mfccp->mf6cc_parent, rt->mf6c_stall));
      
      rt->mf6c_origin   = mfccp->mf6cc_origin;
      rt->mf6c_mcastgrp = mfccp->mf6cc_mcastgrp;
      rt->mf6c_parent   = mfccp->mf6cc_parent;
      rt->mf6c_ifset    = mfccp->mf6cc_ifset;
      
      /* initialize pkt counters per src-grp */
      rt->mf6c_pkt_cnt  = 0;
      rt->mf6c_byte_cnt = 0;
      rt->mf6c_wrong_if = 0;

      rt->mf6c_expire = 0;    /* Don't clean this guy up */
      n6expire[hash]--;
    
      /* free packets Qed at the end of this entry */
      while (rt->mf6c_stall) {
        PLW_LIST_RING pring = _list_ring_get_prev(rt->mf6c_stall);
        struct rtdetq6 *rte = _LIST_ENTRY(pring, struct rtdetq6, list);
        _List_Ring_Del(&rte->list, &rt->mf6c_stall);
        if (rte->ifp) {
          ip6_mrt_ip_mdq(&rte->p.pbuf, rte->ifp, rt);
        }
        mem_free(rte);
        rt->mf6c_nstall--;
      }
    }
  }
  
  /* It is possible that an entry is being inserted without an upcall */
  if (nstl == 0) {
    LWIP_DEBUGF(IP6_DEBUG, ("no upcall h %lu o %s g %s p %x", hash,
                inet6_ntoa_r(rt->mf6c_origin.sin6_addr, ip6bufo, INET6_ADDRSTRLEN),
                inet6_ntoa_r(rt->mf6c_mcastgrp.sin6_addr, ip6bufg, INET6_ADDRSTRLEN),
                mfccp->mf6cc_parent));
    for (pline = mf6ctable[hash]; pline != NULL; pline = _list_line_get_next(pline)) {
      if (IN6_ARE_ADDR_EQUAL(&rt->mf6c_origin.sin6_addr, &mfccp->mf6cc_origin.sin6_addr) &&
          IN6_ARE_ADDR_EQUAL(&rt->mf6c_mcastgrp.sin6_addr, &mfccp->mf6cc_mcastgrp.sin6_addr)) {
        rt->mf6c_origin   = mfccp->mf6cc_origin;
        rt->mf6c_mcastgrp = mfccp->mf6cc_mcastgrp;
        rt->mf6c_parent   = mfccp->mf6cc_parent;
        rt->mf6c_ifset    = mfccp->mf6cc_ifset;
        
        /* initialize pkt counters per src-grp */
        rt->mf6c_pkt_cnt  = 0;
        rt->mf6c_byte_cnt = 0;
        rt->mf6c_wrong_if = 0;

        if (rt->mf6c_expire) {
          n6expire[hash]--;
        }
        rt->mf6c_expire = 0;
      }
    }
    
    if (rt == NULL) {
      /* no upcall, so make a new entry */
      rt = (struct mf6c *)mem_malloc(sizeof(struct mf6c));
      if (rt == NULL) {
        return (ENOBUFS);
      }
      
      /* insert new entry at head of hash chain */
      rt->mf6c_origin   = mfccp->mf6cc_origin;
      rt->mf6c_mcastgrp = mfccp->mf6cc_mcastgrp;
      rt->mf6c_parent   = mfccp->mf6cc_parent;
      rt->mf6c_ifset    = mfccp->mf6cc_ifset;
      
      /* initialize pkt counters per src-grp */
      rt->mf6c_pkt_cnt  = 0;
      rt->mf6c_byte_cnt = 0;
      rt->mf6c_wrong_if = 0;
      rt->mf6c_expire   = 0;
      rt->mf6c_stall    = NULL;
      rt->mf6c_nstall   = 0;
      timerclear(&rt->mf6c_last_assert);

      /* link into table */
      _List_Line_Add_Ahead(&rt->mf6c_list, &mf6ctable[hash]);
    }
  }
  
  return (0);
}

/* ip6_mrt_del_mfc */
static u8_t ip6_mrt_del_mfc (struct mf6cctl *mfccp)
{
#if IP_DEBUG
  char ip6bufo[INET6_ADDRSTRLEN];
  char ip6bufg[INET6_ADDRSTRLEN];
#endif
  
  struct mf6c *rt;
  u_long hash;
  
  LWIP_DEBUGF(IP6_DEBUG, ("orig %s mcastgrp %s",
              inet6_ntoa_r(mfccp->mf6cc_origin.sin6_addr, ip6bufo, INET6_ADDRSTRLEN),
              inet6_ntoa_r(mfccp->mf6cc_mcastgrp.sin6_addr, ip6bufg, INET6_ADDRSTRLEN)));
  
  rt = ip6_mrt_find_mfc(&mfccp->mf6cc_origin.sin6_addr, &mfccp->mf6cc_mcastgrp.sin6_addr);
  if (rt == NULL) {
    return (EADDRNOTAVAIL);
  }
  
  hash = MF6CHASH(mfccp->mf6cc_origin.sin6_addr, mfccp->mf6cc_mcastgrp.sin6_addr);
  _List_Line_Del(&rt->mf6c_list, &mf6ctable[hash]);
  mem_free(rt);
  
  return (0);
}

/* ip6_mrt_raw_send (send to daemon) */
static void ip6_mrt_raw_send (struct raw_pcb *pcb, struct pbuf *p, const ip6_addr_t *src)
{
  ip_addr_t ipsrc;

  ip_addr_copy_from_ip6(ipsrc, *src);
  pcb->recv(pcb->recv_arg, pcb, p, &ipsrc);
}

/* ip6_mrt_phyif_send */
static void ip6_mrt_phyint_send (struct mif6 *mifp, struct pbuf *p)
{
  struct ip6_hdr *ip6hdr;
  struct pbuf *p_out;
  
  p_out = pbuf_alloc(PBUF_LINK, p->tot_len, PBUF_RAM); /* allocate a new pbuf for sending */
  if (p_out == NULL) {
    return; /* ENOBUFS */
  }
  pbuf_copy(p_out, p);
  
  ip6hdr = ptod(p_out, struct ip6_hdr *);
  
  /* decrement HL */
  IP6H_HOPLIM_SET(ip6hdr, IP6H_HOPLIM(ip6hdr) - 1);
  
  if (mifp->m6_ifp->mtu && (p->tot_len > mifp->m6_ifp->mtu)) { /* ipv6 forward do not fragment */
    /* Don't send ICMP messages in response to ICMP messages */
    if (IP6H_NEXTH(ip6hdr) != IP6_NEXTH_ICMP6) {
      icmp6_packet_too_big(p, mifp->m6_ifp->mtu);
    }
  
  } else {
    /* TC == 1 to determining this is a ip6_mforward packet */
    ip6_output_if_src(p, NULL, LWIP_IP_HDRINCL, 0, 1, 0, mifp->m6_ifp);
  }
  pbuf_free(p_out);
}

/* ip6_mrt_pim_send */
static void ip6_mrt_pim_send (struct mif6 *mifp, struct pbuf *p)
{
  struct ip6_hdr *ip6hdr;
  struct mrt6msg *im6;
  struct pbuf *p_pim;
  ip6_addr_t src;

  PIM6STAT_INC(pim6s_snd_registers);
  
  /* allocate a new pbuf for sending */
  p_pim = pbuf_alloc(PBUF_RAW + sizeof(struct mrt6msg), p->tot_len, PBUF_RAM);
  if (p_pim == NULL) {
    return; /* ENOBUF */
  }
  pbuf_copy(p_pim, p);
  ip6hdr = ptod(p_pim, struct ip6_hdr *);
  
  /* decrement HL */
  IP6H_HOPLIM_SET(ip6hdr, IP6H_HOPLIM(ip6hdr) - 1);
  
  pbuf_header(p_pim, sizeof(struct mrt6msg));
  pbuf_copy_partial(p, ptod(p_pim, void *), (u16_t)sizeof(struct ip6_hdr), 0); /* copy header to toup */
  
  im6 = ptod(p, struct mrt6msg *);
  im6->im6_msgtype = MRT6MSG_WHOLEPKT;
  im6->im6_mbz     = 0;
  im6->im6_mif     = mifp - mif6table;
  
  MRT6STAT_INC(mrt6s_upcalls);
  
  ip6_addr_copy_from_packed(src, ip6hdr->src);
  ip6_addr_set_zone(&src, p->if_idx);
  ip6_mrt_raw_send(ip6_mroute_pcb, p, &src); /* send to daemon */
  pbuf_free(p_pim);
}

/* ip6_mrt_ip_mdq */
static err_t ip6_mrt_ip_mdq (struct pbuf *p, struct netif *ifp, struct mf6c *rt)
{
  struct ip6_hdr *ip6hdr = ptod(p, struct ip6_hdr *);
  mifi_t mifi;
  struct mif6 *mifp;
  int plen = p->tot_len;
  
  /* Don't forward if it didn't arrive from the parent mif
   * for its origin. */
  mifi = rt->mf6c_parent;
  if ((mifi >= nummifs) || (mif6table[mifi].m6_ifp != ifp)) {
    /* came in the wrong interface */
    LWIP_DEBUGF(IP6_DEBUG, ("wrong if: ifid %d mifi %d mififid %x", netif_get_index(ifp),
                mifi, netif_get_index(mif6table[mifi].m6_ifp)));
    MRT6STAT_INC(mrt6s_wrong_if);
    rt->mf6c_wrong_if++;
    
    /* If we are doing PIM processing, and we are forwarding
     * packets on this interface, send a message to the
     * routing daemon. */
    /* have to make sure this is a valid mif */
    if (pim6_assert && (mifi < nummifs && mif6table[mifi].m6_ifp)) {
      /* Locate the vifi for the incoming interface for this packet.
       * If none found, drop packet. */
      for (mifi = 0; mifi < nummifs && mif6table[mifi].m6_ifp != ifp; mifi++);
      if (mifi >= nummifs) { /* mif not found, drop packet */
        return (ERR_OK);
      }
      
      if (ip6_mrt_rate_check(&rt->mf6c_last_assert, &pim6_assert_interval)) {
        struct pbuf *toup;
        struct mrt6msg *im6;
        ip6_addr_t src;
        
        /* Send message to routing daemon to install
         * a route into the kernel table */
        toup = pbuf_alloc(PBUF_RAW, (u16_t)sizeof(struct ip6_hdr), PBUF_RAM);
        if (toup == NULL) {
          return (ERR_OK); /* ENOBUFS */
        }
        
        pbuf_copy_partial(p, ptod(toup, void *), (u16_t)sizeof(struct ip6_hdr), 0); /* copy header to toup */
        im6 = ptod(toup, struct mrt6msg *); /* mrt6msg msg header */
        im6->im6_msgtype = MRT6MSG_WRONGMIF;
        im6->im6_mbz = 0;
        im6->im6_mif = mifi;
        
        MRT6STAT_INC(mrt6s_upcalls);
        ip6_addr_copy_from_packed(src, ip6hdr->src);
        ip6_addr_set_zone(&src, p->if_idx);
        ip6_mrt_raw_send(ip6_mroute_pcb, p, &src); /* send to daemon */
        pbuf_free(toup);
      }
    }
    
    return (ERR_OK);
  }
  
  /* If I sourced this packet, it counts as output, else it was input. */
  if (p->if_idx == NETIF_NO_INDEX) {
    mif6table[mifi].m6_pkt_out++;
    mif6table[mifi].m6_bytes_out += plen;
  
  } else {
    mif6table[mifi].m6_pkt_in++;
    mif6table[mifi].m6_bytes_in += plen;
  }
  
  rt->mf6c_pkt_cnt++;
  rt->mf6c_byte_cnt += plen;
  
  for (mifp = mif6table, mifi = 0; mifi < nummifs; mifp++, mifi++) {
    if (IF_ISSET(mifi, &rt->mf6c_ifset) && mifp->m6_ifp) {
      if (!(mif6table[rt->mf6c_parent].m6_flags & MIFF_REGISTER) &&
          !(mif6table[mifi].m6_flags & MIFF_REGISTER)) {
        /*
         * TODO: Scope check
         */
      }
      
      mifp->m6_pkt_out++;
      mifp->m6_bytes_out += plen;
      
      if (mifp->m6_flags & MIFF_REGISTER) {
        ip6_mrt_pim_send(mifp, p);
      
      } else {
        ip6_mrt_phyint_send(mifp, p);
      }
    }
  }
  
  return (ERR_OK);
}

/* ip6_mrt_forward 
   return no ERR_OK ip_input will drop this packet */
err_t ip6_mrt_forward (struct pbuf *p, const struct ip6_hdr *ip6hdr, struct netif *ifp)
{
#if IP_DEBUG
  char ip6bufo[INET6_ADDRSTRLEN];
  char ip6bufg[INET6_ADDRSTRLEN];
#endif

  int scope;
  struct in6_addr src, dest;
  PLW_LIST_LINE pline;
  struct mf6c *rt;
  mifi_t mifi;

  if (ip6_mroute_pcb == NULL) { /* not init */
    return (ERR_OK);
  }
  
  if (IP6H_HOPLIM(ip6hdr) <= 1) {
    return (ERR_OK);
  }
  
  scope = ip6_addr_multicast_scope(&ip6hdr->dest);
  if ((scope == IP6_MULTICAST_SCOPE_INTERFACE_LOCAL) || 
      (scope == IP6_MULTICAST_SCOPE_LINK_LOCAL)) {
    return (ERR_OK);
  }
  
  if (ip6_addr_isany_val(ip6hdr->src)) {
    return (ERR_OK);
  }
  
  inet6_addr_from_ip6addr(&src, &ip6hdr->src);
  inet6_addr_from_ip6addr(&dest, &ip6hdr->dest);
  
  /* Determine forwarding vifs from the forwarding cache table */
  /* Entry exists, so forward if necessary */
  rt = ip6_mrt_find_mfc(&src, &dest);
  if (rt != NULL) {
    return (ip6_mrt_ip_mdq(p, ifp, rt));
  
  } else {
    struct rtdetq6 *rte;
    u_long hash;
    
    /* If we don't have a route for packet's origin,
     * Make a copy of the packet & send message to routing daemon. */
    MRT6STAT_INC(mrt6s_no_route);
    
    LWIP_DEBUGF(IP6_DEBUG, ("no rte s %s g %s",
                inet6_ntoa_r(&src, ip6bufo, INET6_ADDRSTRLEN),
                inet6_ntoa_r(&dest, ip6bufg, INET6_ADDRSTRLEN)));
              
    /* Allocate pbufs early so that we don't do extra work if we are
     * just going to fail anyway.  Make sure to pullup the header so
     * that other people can't step on it. */
    rte = ip6_mrt_rtdetq_alloc(p->tot_len, 0);
    if (rte == NULL) {
      return (ERR_OK);
    }
    pbuf_copy(&rte->p.pbuf, p);
    
    hash = MF6CHASH(src, dest);
    for (pline = mf6ctable[hash]; pline != NULL; pline = _list_line_get_next(pline)) {
      rt = _LIST_ENTRY(pline, struct mf6c, mf6c_list);
      if (IN6_ARE_ADDR_EQUAL(&rt->mf6c_origin.sin6_addr, &src) &&
          IN6_ARE_ADDR_EQUAL(&rt->mf6c_mcastgrp.sin6_addr, &dest) &&
          (rt->mf6c_stall != NULL)) {
        break;
      }
    }
    
    if (pline == NULL) {
      struct pbuf *toup;
      struct mrt6msg *im6;
      ip6_addr_t up_src;
      
      /* Locate the vifi for the incoming interface for this packet.
       * If none found, drop packet. */
      for (mifi = 0; mifi < nummifs && mif6table[mifi].m6_ifp != ifp; mifi++);
      if (mifi >= nummifs) { /* mif not found, drop packet */
        mem_free(rte);
        return (ERR_OK);
      }
      
      /* no upcall, so make a new entry */
      rt = (struct mf6c *)mem_malloc(sizeof(struct mf6c));
      if (rt == NULL) {
        mem_free(rte);
        return (ERR_OK);
      }
      
      /* Send message to routing daemon to install
       * a route into the kernel table */
      toup = pbuf_alloc(PBUF_RAW, (u16_t)sizeof(struct ip6_hdr), PBUF_RAM);
      if (toup == NULL) {
        mem_free(rt);
        mem_free(rte);
        return (ERR_OK);
      }
      
      pbuf_copy_partial(p, ptod(toup, void *), (u16_t)sizeof(struct ip6_hdr), 0); /* copy header to toup */
      im6 = ptod(toup, struct mrt6msg *); /* igmp msg header */
      im6->im6_msgtype = MRT6MSG_NOCACHE;
      im6->im6_mbz     = 0; /* must be zero */
      im6->im6_mif     = (u_char)mifi;
      
      MRT6STAT_INC(mrt6s_upcalls);
      
      inet6_addr_to_ip6addr(&up_src, &src);
      ip6_addr_set_zone(&up_src, p->if_idx);
      ip6_mrt_raw_send(ip6_mroute_pcb, toup, &up_src); /* send to daemon */
      mem_free(toup);
      
      /* insert new entry at head of hash chain */
      lib_bzero(rt, sizeof(*rt));
      rt->mf6c_origin.sin6_family = AF_INET6;
      rt->mf6c_origin.sin6_len    = sizeof(struct sockaddr_in6);
      rt->mf6c_origin.sin6_addr   = src;
      rt->mf6c_mcastgrp.sin6_family = AF_INET6;
      rt->mf6c_mcastgrp.sin6_len    = sizeof(struct sockaddr_in6);
      rt->mf6c_mcastgrp.sin6_addr   = dest;
      
      rt->mf6c_expire = UPCALL_EXPIRE;
      n6expire[hash]++;
      rt->mf6c_parent = MF6C_INCOMPLETE_PARENT;
      
      _List_Line_Add_Ahead(&rt->mf6c_list, &mf6ctable[hash]);
      _List_Ring_Add_Ahead(&rte->list, &rt->mf6c_stall);
      rt->mf6c_nstall++;
    
    } else {
      if (rt->mf6c_nstall > MAX_UPQ6) {
        MRT6STAT_INC(mrt6s_upq_ovflw);
        mem_free(rte);
        return (ERR_OK);
      }
      
      _List_Ring_Add_Ahead(&rte->list, &rt->mf6c_stall);
      rt->mf6c_nstall++;
    }
    
    rte->ifp = ifp;
  }
  
  return (ERR_OK);
}

/* ip6_mrt_pim_input (return: eaten) */
u8_t ip6_mrt_pim_input (struct pbuf *p, u16_t off, struct netif *inp)
{
  struct pim *pim; /* pointer to a pim struct */
  struct ip6_hdr *ip6hdr = ptod(p, struct ip6_hdr *);
  ip6_addr_t src;
  int pimlen;
  int minlen;
  
  /* get source */
  ip6_addr_copy_from_packed(src, ip6hdr->src);
  
  PIM6STAT_INC(pim6s_rcv_total);
  
  pimlen = p->tot_len - off;
  if (pimlen < PIM_MINLEN) {
    PIM6STAT_INC(pim6s_rcv_tooshort);
    LWIP_DEBUGF(IP6_DEBUG, ("PIM packet too short"));
    pbuf_free(p);
    return (1);
  }
  
  /* if the packet is at least as big as a REGISTER, go ahead
   * and grab the PIM REGISTER header size, to avoid another
   * possible m_pullup() later.
   *
   * PIM_MINLEN       == pimhdr + u_int32 == 8
   * PIM6_REG_MINLEN   == pimhdr + reghdr + eip6hdr == 4 + 4 + 40 */
  minlen = (pimlen >= PIM6_REG_MINLEN) ? PIM6_REG_MINLEN : PIM_MINLEN;
  
  if (p->len < (off + minlen)) {
    PIM6STAT_INC(pim6s_rcv_tooshort);
    pbuf_free(p);
    return (1);
  }
  
  pim = (struct pim *)((caddr_t)ip6hdr + off);
  
  /*
   * TODO: chksum check
   */
  /* PIM version check */
  if (pim->pim_ver != PIM_VERSION) {
    PIM6STAT_INC(pim6s_rcv_badversion);
    LWIP_DEBUGF(IP6_DEBUG, ("incorrect version %d, expecting %d",
                pim->pim_ver, PIM_VERSION));
    pbuf_free(p);
    return (1);
  }
  
  if (pim->pim_type == PIM_REGISTER) {
    struct ip6_hdr *encap_ip6hdr;
    uint32_t *reghdr;
    struct pbuf *input;
  
    PIM6STAT_INC(pim6s_rcv_registers);
    if ((register_if6_num >= nummifs) || (register_if6_num == MIFI_INVALID)) {
      LWIP_DEBUGF(IP6_DEBUG, ("register mif not set: %d", register_if6_num));
      pbuf_free(p);
      return (1);
    }
    
    reghdr = (uint32_t *)(pim + 1);
    encap_ip6hdr = (struct ip6_hdr *)(reghdr + 1);
  
    if (PP_NTOHL(*reghdr) & PIM_NULL_REGISTER) {
      goto pim_input_to_daemon;
    }
    
    if (pimlen < PIM6_REG_MINLEN) {
      PIM6STAT_INC(pim6s_rcv_tooshort);
      PIM6STAT_INC(pim6s_rcv_badregisters);
      pbuf_free(p);
      return (1);
    }
    
    /* verify the version number of the inner packet */
    if (IP6H_V(encap_ip6hdr) != 6) {
      PIM6STAT_INC(pim6s_rcv_badregisters);
      pbuf_free(p);
      return (1);
    }
    
    /* verify the inner packet is destined to a mcast group */
    if (!ip6_addr_ismulticast(&encap_ip6hdr->dest)) {
      PIM6STAT_INC(pim6s_rcv_badregisters);
      pbuf_free(p);
      return (1);
    }
    
    /* make a copy of the whole header to pass to the daemon later. */
    input = pbuf_alloc(PBUF_RAW, p->tot_len, PBUF_RAM);
    if (input == NULL) {
      pbuf_free(p);
      return (1);
    }
    pbuf_copy(input, p);
    input->flags |= PBUF_FLAG_LLMCAST; /* input is a mcast packet */
    
    /* remove outer ip and pimhdr */
    pbuf_header(input, (u16_t)-(off + PIM_MINLEN));
    if (tcpip_inpkt(input, inp, ip_input)) { /* use inp ??? */
      pbuf_free(input); /* loop error */
    }
    
    /* pass the 'head' only up to the daemon. This includes the
     * outer IP header, PIM header, PIM-Register header and the inner IP header */
    pbuf_realloc(p, (u16_t)(off + PIM6_REG_MINLEN));
  }
  
pim_input_to_daemon:
  /* Pass the PIM message up to the daemon; if it is a Register message,
   * pass the 'head' only up to the daemon. This includes the
   * outer IP header, PIM header, PIM-Register header and the
   * inner IP header.
   * XXX: the outer IP header pkt size of a Register is not adjust to
   * reflect the fact that the inner multicast data is truncated. */
  if (ip6_mroute_pcb) {
    ip6_mrt_raw_send(ip6_mroute_pcb, p, &src); /* daemon will recv this packet */
  }
  pbuf_free(p);
  return (1); 
}

/* socket opt cmd: 
 * MRT6_INIT / MRT6_DONE / MRT6_ADD_VIF / 
 * MRT6_DEL_VIF / MRT6_ADD_MFC / MRT6_DEL_MFC */
u8_t ip6_mrt_setsockopt (struct raw_pcb *pcb, int optname, const void *optval, socklen_t optlen)
{
  if ((pcb != ip6_mroute_pcb) && (optname != MRT6_INIT)) {
    return (EPERM);
  }
  
  switch (optname) {
  
  case MRT6_INIT:
    if (pcb && optval && (optlen >= sizeof(int))) {
      return (ip6_mrt_init(pcb, *(int *)optval));
    } else {
      return (EINVAL);
    }
    break;
    
  case MRT6_DONE:
    return (ip6_mrt_down(pcb));
    
  case MRT6_ADD_MIF:
    if (optval && (optlen >= sizeof(struct mif6ctl))) {
      return (ip6_mrt_add_mif((struct mif6ctl *)optval));
    } else {
      return (EINVAL);
    }
    break;
    
  case MRT6_DEL_MIF:
    if (optval && (optlen >= sizeof(mifi_t))) {
      return (ip6_mrt_del_mif((mifi_t *)optval));
    } else {
      return (EINVAL);
    }
    break;
    
  case MRT6_ADD_MFC:
    if (optval && (optlen >= sizeof(struct mf6cctl))) {
      return (ip6_mrt_add_mfc((struct mf6cctl *)optval));
    } else {
      return (EINVAL);
    }
    break;
    
  case MRT6_DEL_MFC:
    if (optval && (optlen >= sizeof(struct mf6cctl))) {
      return (ip6_mrt_del_mfc((struct mf6cctl *)optval));
    } else {
      return (EINVAL);
    }
    break;
    
  case MRT6_ASSERT:
    if (optval && (optlen >= sizeof(int))) {
      pim6_assert = *(int *)optval;
      return (0);
    } else {
      return (EINVAL);
    }
    break;
    
  default:
    return (ENOSYS);
  }
  
  return (ENOSYS); /* It's impossible to run here */
}

/* socket opt cmd: 
 * MRT6_ASSERT */
u8_t ip6_mrt_getsockopt (struct raw_pcb *pcb, int optname, const void *optval, socklen_t *optlen)
{
  switch (optname) {
  
  case MRT6_ASSERT:
    if (optval && optlen && (*optlen >= sizeof(int))) {
      *(int *)optval = pim6_assert;
    } else {
      return (EINVAL);
    }
    break;
    
  default:
    return (ENOSYS);
  }
  
  return (0);
}

/* ip6_mrt_get_sg_cnt */
static u8_t ip6_mrt_get_sg_cnt (struct sioc_sg_req6 *req)
{
  struct mf6c *rt;
  
  rt = ip6_mrt_find_mfc(&req->src.sin6_addr, &req->grp.sin6_addr);
  if (rt == NULL) {
    req->pktcnt = req->bytecnt = req->wrong_if = 0xffffffff;
    return (EADDRNOTAVAIL);
    
  } else {
    req->pktcnt = rt->mf6c_pkt_cnt;
    req->bytecnt = rt->mf6c_byte_cnt;
    req->wrong_if = rt->mf6c_wrong_if;
  }
  
  return (0);
}

/* ip6_mrt_get_mif_cnt */
static u8_t ip6_mrt_get_mif_cnt (struct sioc_mif_req6 *req)
{
  mifi_t mifi = req->mifi;
  
  if (mifi >= nummifs) {
    return (EINVAL);
  }
  
  req->icount = mif6table[mifi].m6_pkt_in;
  req->ocount = mif6table[mifi].m6_pkt_out;
  req->ibytes = mif6table[mifi].m6_bytes_in;
  req->obytes = mif6table[mifi].m6_bytes_out;
  
  return (0);
}

/* Handle ioctl commands to obtain information from the cache */
u8_t ip6_mrt_ioctl (int cmd, void *data)
{
  switch (cmd) {
  
  case SIOCGETMIFCNT_IN6:
    return (ip6_mrt_get_mif_cnt((struct sioc_mif_req6 *)data));
    
  case SIOCGETSGCNT_IN6:
    return (ip6_mrt_get_sg_cnt((struct sioc_sg_req6 *)data));
  
  default:
    break;
  }
  
  return (EINVAL);
}

/* ip6_mrt_clean */
void ip6_mrt_clean (struct raw_pcb *pcb)
{
  if (ip6_mroute_pcb == pcb) {
    ip6_mrt_down(pcb);
  }
}

/* ip6_mrt_legal_mif_num */
int ip6_mrt_legal_mif_num (int mifi)
{
  if (mifi >= 0 && mifi < nummifs) {
    return (1);
  } else {
    return (0);
  }
}

/* ip6_mrt_is_on */
int ip6_mrt_is_on (void)
{
  return (ip6_mroute_pcb ? 1 : 0);
}

/* ip6_mrt_traversal_mfc */
void ip6_mrt_traversal_mfc (void (*callback)(), void *arg0, void *arg1,
                            void *arg2, void *arg3, void *arg4, void *arg5)
{
  int i;
  struct mif6 *mifp;
  struct mf6c *rt;
  LW_LIST_LINE *pline;
  
  for (i = 0; i < MF6CTBLSIZ; i++) {
    for (pline = mf6ctable[i]; pline != NULL; pline = _list_line_get_next(pline)) {
      rt = _LIST_ENTRY(pline, struct mf6c, mf6c_list);
      if (rt->mf6c_stall == NULL) {
        mifp = NULL;
        if (rt->mf6c_parent < nummifs) {
          if (mif6table[rt->mf6c_parent].m6_ifp) {
            mifp = &mif6table[rt->mf6c_parent];
          }
        }
        callback(rt, mifp, arg0, arg1, arg2, arg3, arg4, arg5);
      }
    }
  }
}

/* ip6_mrt_if_detach */
void ip6_mrt_if_detach (struct netif *ifp)
{
  PLW_LIST_LINE pline;
  struct mf6c *rt;
  mifi_t mifi;
  u_long i;
  
  if (ip6_mroute_pcb == NULL) {
    return;
  }
  
  MRT6_LOCK();
  
  /* Tear down multicast forwarder state associated with this ifnet.
   * 1. Walk the mif list, matching mifs against this ifnet.
   * 2. Walk the multicast forwarding cache (mfc) looking for
   *    inner matches with this vif's index.
   * 3. Expire any matching multicast forwarding cache entries.
   * 4. Free vif state. This should disable ALLMULTI on the interface. */
  for (mifi = 0; mifi < nummifs; mifi++) {
    if (mif6table[mifi].m6_ifp != ifp) {
      continue;
    }
    for (i = 0; i < MF6CTBLSIZ; i++) {
      pline = mf6ctable[i];
      while (pline) {
        rt = _LIST_ENTRY(pline, struct mf6c, mf6c_list);
        pline = _list_line_get_next(pline);
        if (rt->mf6c_parent == mifi) {
          ip6_mrt_expire_mfc(rt);
        }
      }
    }
    ip6_mrt_del_mif(&mifi);
  }

  MRT6_UNLOCK();
}

/* ip6_mrt_mrt6stat */
struct mrt6stat *ip6_mrt_mrt6stat (void)
{
  return (&mrt6stat);
}

/* ip6_mrt_pim6stat */
struct pim6stat *ip6_mrt_pim6stat (void)
{
  return (&pim6stat);
}

#endif /* LW_CFG_NET_MROUTER && LWIP_IPV6 */
/*
 * end
 */
