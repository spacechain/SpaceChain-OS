/**
 * @file
 * ipv4 multicast route.
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

#if LW_CFG_NET_MROUTER > 0

#define _PIM_VT 1

#include "ip4_mrt.h"
#include "net/if_flags.h"
#include "sys/time.h"
#include "lwip/tcpip.h"
#include "lwip/timeouts.h"
#include "lwip/inet_chksum.h"

extern u16_t *ip4_get_ip_id(void);

/* VIFI_INVALID */
#define VIFI_INVALID ((vifi_t)-1)

/* like mtod() */
#define ptod(p, type) (type)((p)->payload)

/* multicast route raw pcb */
static struct raw_pcb *ip_mroute_pcb = NULL;

/* multicast statistical variable */
static struct mrtstat mrtstat;
static struct pimstat pimstat;

/* table of mfc hash entries */
static LW_LIST_LINE_HEADER mfctable[MFCTBLSIZ];
static u_char nexpire[MFCTBLSIZ];
#define EXPIRE_TIMEOUT  (uint_t)(LW_TICK_HZ / 4) /* 4x / second */

/* table of virtual interfaces */
static struct vif viftable[MAXVIFS];
static vifi_t numvifs = 0;
#define UPCALL_EXPIRE   6 /* number of timeouts */

/* fake IPIP tunnel netif */
static struct netif multicast_decap_if;
static int have_encap_tunnel = 0;

/* fake PIM multicast register netif */
static struct netif multicast_register_if;
static int register_if_num = VIFI_INVALID;

/* one-back cache used by ipip_input to locate a tunnel's vif
 * given a datagram's src ip address. */
static u_long last_encap_src;
static struct vif *last_encap_vif;

/* multicast route init */
static int pim_assert = 0;
static const struct timeval pim_assert_interval = { 3, 0 };

/* mroute config */
static uint32_t mrt_api_config = 0;
static const uint32_t mrt_api_support = (MRT_MFC_FLAGS_DISABLE_WRONGVIF |
                                         MRT_MFC_FLAGS_BORDER_VIF |
                                         MRT_MFC_RP);
/* IPIP tunnel */
/* Note: the IPIP tunnel encapsulation adds the following in front of a
 * data packet:
 *
 * struct ipip_encap_hdr {
 *    struct ip_hdr iphdr;
 *    struct multicast_encap_iphdr encap;
 * }
 */
#define ENCAP_TTL   64
#define ENCAP_PROTO IPPROTO_IPIP /* 4 */

/* prototype IP hdr for encapsulated packets */
static struct ip_hdr multicast_encap_iphdr = {
  ((((4) << 4) | (sizeof(struct ip_hdr) >> 2))),
  0, /* tos */
  sizeof(struct ip_hdr), /* total length */
  0, /* id */
  0, /* frag offset */
  ENCAP_TTL, 
  ENCAP_PROTO, 
  0, /* checksum */
};

/* PIM tunnel */
/* Note: the PIM Register encapsulation adds the following in front of a
 * data packet:
 *
 * struct pim_encap_hdr {
 *    struct ip_hdr iphdr;
 *    struct pim_encap_pimhdr  pim;
 * }
 */
struct pim_encap_pimhdr {
  struct pim  pim;
  uint32_t    flags;
};

#define PIM_ENCAP_TTL   64

static struct ip_hdr pim_encap_iphdr = {
  ((((4) << 4) | (sizeof(struct ip_hdr) >> 2))),
  0, /* tos */
  sizeof(struct ip_hdr), /* total length */
  0, /* id */
  0, /* frag offset */
  PIM_ENCAP_TTL,
  IPPROTO_PIM,
  0, /* checksum */
};

static struct pim_encap_pimhdr pim_encap_pimhdr = {
  {
    PIM_MAKE_VT(PIM_VERSION, PIM_REGISTER), /* PIM vers and message type */
    0, /* reserved */
    0, /* checksum */
  },
  0 /* flags */
};

/* Hash function for a source, group entry */
#define MFCHASH(a, g) MFCHASHMOD(((a) >> 20) ^ ((a) >> 10) ^ (a) ^ \
                                 ((g) >> 20) ^ ((g) >> 10) ^ (g))

/* internal functions */
static err_t ip4_mrt_ip_mdq(struct pbuf *p, struct netif *ifp, struct mfc *rt);

/* ip4_mrt_rate_check */
static int ip4_mrt_rate_check (struct timeval *tv, const struct timeval *period)
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

/* ip4_mrt_rtdetq_free */
static void ip4_mrt_rtdetq_free (struct pbuf *p)
{
  struct rtdetq *rte = _LIST_ENTRY(p, struct rtdetq, p);
  
  mem_free(rte);
}

/* ip4_mrt_rtdetq_alloc */
struct rtdetq *ip4_mrt_rtdetq_alloc (size_t bsz, size_t res)
{
  struct rtdetq *rte;
  struct pbuf *ret;
  u16_t tot_len = (u16_t)(res + bsz);
  
  rte = (struct rtdetq *)mem_malloc(ROUND_UP(sizeof(struct rtdetq), MEM_ALIGNMENT) + tot_len);
  if (rte == NULL) {
    return (NULL);
  }
  
  rte->p.custom_free_function = ip4_mrt_rtdetq_free;
  
  ret = pbuf_alloced_custom(PBUF_RAW, tot_len, PBUF_RAM, &rte->p,
                            (char *)rte + ROUND_UP(sizeof(struct rtdetq), MEM_ALIGNMENT), 
                            tot_len);
  
  if (ret && res) {
    pbuf_header(ret, (u16_t)-res);
  }
  
  return (rte);
}

/* ip4_mrt_api_config */
static u8_t ip4_mrt_api_config (uint32_t *apival)
{
  u_long i;
  
  /* We can set the API capabilities only if it is the first operation
   * after MRT_INIT. I.e.:
   *  - there are no vifs installed
   *  - pim_assert is not enabled
   *  - the MFC table is empty */
  if (numvifs > 0) {
    *apival = 0;
    return (EPERM);
  }
  
  if (pim_assert) {
    *apival = 0;
    return (EPERM);
  }
  
  for (i = 0; i < MFCTBLSIZ; i++) {
    if (mfctable[i]) {
      *apival = 0;
      return (EPERM);
    }
  }
  
  mrt_api_config = *apival & mrt_api_support;
  *apival = mrt_api_config;
  
  return (0);
}

/* Find a route for a given origin IP address and Multicast group address
 * Type of service parameter to be added in the future!!! */
static LW_INLINE struct mfc *ip4_mrt_find_mfc (uint32_t origin, uint32_t grp)
{
  LW_LIST_LINE *pline;
  struct mfc *rt;
  
  MRTSTAT_INC(mrts_mfc_lookups);
  for (pline = mfctable[MFCHASH(origin, grp)]; pline != NULL; pline = _list_line_get_next(pline)) {
    rt = _LIST_ENTRY(pline, struct mfc, mfc_list);
    if ((rt->mfc_origin.s_addr == origin) && 
        (rt->mfc_mcastgrp.s_addr == grp) && 
        (rt->mfc_stall == NULL)) {
      break;
    }
  }
  
  if (pline == NULL) {
    MRTSTAT_INC(mrts_mfc_misses);
    return (NULL);
  }
  
  return (rt);
}

/* ip4_mrt_update_mfc */
static void ip4_mrt_update_mfc (struct mfc *rt, struct mfcctl2 *mfccp)
{
  int i;
  
  rt->mfc_parent = mfccp->mfcc_parent;
  
  for (i = 0; i < numvifs; i++) {
    rt->mfc_ttls[i] = mfccp->mfcc_ttls[i];
    rt->mfc_flags[i] = mfccp->mfcc_flags[i] & mrt_api_config & MRT_MFC_FLAGS_ALL;
  }
  
  /* set the RP address */
  if (mrt_api_config & MRT_MFC_RP) {
    rt->mfc_rp = mfccp->mfcc_rp;
  } else {
    rt->mfc_rp.s_addr = INADDR_ANY;
  }
}

/* ip4_mrt_init_mfc */
static void ip4_mrt_init_mfc (struct mfc *rt, struct mfcctl2 *mfccp)
{
  rt->mfc_origin   = mfccp->mfcc_origin;
  rt->mfc_mcastgrp = mfccp->mfcc_mcastgrp;
  ip4_mrt_update_mfc(rt, mfccp);

  /* initialize pkt counters per src-grp */
  rt->mfc_pkt_cnt  = 0;
  rt->mfc_byte_cnt = 0;
  rt->mfc_wrong_if = 0;
  timerclear(&rt->mfc_last_assert);
}

/* ip4_mrt_expire_mfc */
static void ip4_mrt_expire_mfc (struct mfc *rt)
{
  u_long hash = MFCHASH(rt->mfc_origin.s_addr, rt->mfc_mcastgrp.s_addr);

  while (rt->mfc_stall) {
    struct rtdetq *rte = _LIST_ENTRY(rt->mfc_stall, struct rtdetq, list);
    _List_Ring_Del(&rte->list, &rt->mfc_stall);
    mem_free(rte);
  }
  
  _List_Line_Del(&rt->mfc_list, &mfctable[hash]);
  mem_free(rt);
}

/* ip4_mrt_expire_upcalls */
static void ip4_mrt_expire_upcalls (void *arg)
{
  LW_LIST_LINE *pline;
  struct mfc *rt;
  int i;

  for (i = 0; i < MFCTBLSIZ; i++) {
    if (nexpire[i] == 0) {
      continue;
    }
    
    pline = mfctable[i];
    while (pline) {
      rt = _LIST_ENTRY(pline, struct mfc, mfc_list);
      pline = _list_line_get_next(pline);
      
      /* Skip real cache entries */
      if (rt->mfc_stall == NULL) {
        continue;
      }
      
      /* If it expires now */
      if ((rt->mfc_expire == 0) || (--rt->mfc_expire > 0)) {
        continue;
      }

      LWIP_DEBUGF(IP_DEBUG, ("ip4_mrt_expire_upcalls: expiring (%x %x)\n",
                  PP_NTOHL(rt->mfc_origin.s_addr),
                  PP_NTOHL(rt->mfc_mcastgrp.s_addr)));
        
      /* drop all the packets
       * free the pbuf with the pkt, if, timing info */
      ip4_mrt_expire_mfc(rt);
      
      MRTSTAT_INC(mrts_cache_cleanups);
      nexpire[i]--;
    }
  }

  sys_timeout(EXPIRE_TIMEOUT, ip4_mrt_expire_upcalls, NULL);
}

/* ip4_mrt_init */
static u8_t ip4_mrt_init (struct raw_pcb *pcb, int val)
{
  if (val == 0) {
    return (ENOPROTOOPT);
  }

  if (ip_mroute_pcb) {
    return (EADDRINUSE);
  }
  
  ip_mroute_pcb = pcb;
  
  lib_bzero(mfctable, sizeof(mfctable));
  lib_bzero(nexpire,  sizeof(nexpire));
  
  pim_assert = 0;
  
  sys_timeout(EXPIRE_TIMEOUT, ip4_mrt_expire_upcalls, NULL);
  
  LWIP_DEBUGF(IP_DEBUG, ("ip4_mrt_init\n"));
  return (0);
}

/* ip4_mrt_down */
static u8_t ip4_mrt_down (struct raw_pcb *pcb)
{
  int i, flags;
  vifi_t vifi;
  struct netif *ifp;
  struct ifreq ifr;
  
  if (ip_mroute_pcb == NULL) {
    return (EINVAL);
  }
  
  /* For each phyint in use, disable promiscuous reception of all IP
   * multicasts. */
  for (vifi = 0; vifi < numvifs; vifi++) {
    if ((viftable[vifi].v_lcl_addr.s_addr != INADDR_ANY) &&
        !(viftable[vifi].v_flags & (VIFF_TUNNEL | VIFF_REGISTER))) {
      ifp = viftable[vifi].v_ifp;
      if (ifp) {
        flags = netif_get_flags(ifp);
        if (flags & IFF_ALLMULTI) {
          ifr.ifr_name[0] = 0;
          ifr.ifr_flags   = flags & ~IFF_ALLMULTI;
          if (ifp->ioctl) {
            if (ifp->ioctl(ifp, SIOCSIFFLAGS, &ifr)) {
              ((struct sockaddr_in *)&(ifr.ifr_addr))->sin_family = AF_INET;
              ((struct sockaddr_in *)&(ifr.ifr_addr))->sin_addr.s_addr = INADDR_ANY;
              ifp->ioctl(ifp, SIOCDELMULTI, &ifr);
            }
          }
        }
      }
    }
  }
  
  lib_bzero(viftable, sizeof(viftable));
  
  numvifs = 0;
  pim_assert = 0;
  mrt_api_config = 0;
  
  sys_untimeout(ip4_mrt_expire_upcalls, NULL);
  
  for (i = 0; i < MFCTBLSIZ; i++) {
    while (mfctable[i]) {
      struct mfc *rt = _LIST_ENTRY(mfctable[i], struct mfc, mfc_list);
      ip4_mrt_expire_mfc(rt);
    }
  }
  
  lib_bzero(mfctable, sizeof(mfctable));
  lib_bzero(nexpire,  sizeof(nexpire));
  
  /* Reset de-encapsulation cache */
  last_encap_src = 0;
  last_encap_vif = NULL;
  have_encap_tunnel = 0;
  
  /* unregister if */
  register_if_num = VIFI_INVALID;
  
  ip_mroute_pcb = NULL;
  
  LWIP_DEBUGF(IP_DEBUG, ("ip4_mrt_down\n"));
  return (0);
}

/* ip4_mrt_add_vif */
static u8_t ip4_mrt_add_vif (struct vifctl *vifcp)
{
  struct vif *vifp = viftable + vifcp->vifc_vifi;
  struct ip4_addr ifa;
  struct netif *ifp;
  struct ifreq ifr;
  int flags;
  
  if (vifcp->vifc_vifi >= MAXVIFS) {
    return (EINVAL);
  }
  
  if (vifcp->vifc_flags & VIFF_USE_IFINDEX) {
    if ((vifcp->vifc_lcl_ifindex < 1) || 
        (vifcp->vifc_lcl_ifindex >= LW_CFG_NET_DEV_MAX)) {
      return (EADDRNOTAVAIL);
    }
  
  } else {
    if (vifcp->vifc_lcl_addr.s_addr == INADDR_ANY) {
      return (EADDRNOTAVAIL);
    }
  }
  
  if (vifp->v_lcl_addr.s_addr != INADDR_ANY) {
    return (EADDRINUSE);
  }
  
  if (vifcp->vifc_flags & VIFF_SRCRT) { /* Do not support old source route option */
    return (EOPNOTSUPP);
  }
  
  if (vifcp->vifc_flags & VIFF_REGISTER) { /* PIM tunnel */
    /*
     * Because VIFF_REGISTER does not really need a valid
     * local interface (e.g. it could be 127.0.0.2), we don't
     * check its address.
     */
    ifp = &multicast_register_if;
    if (multicast_register_if.flags == 0) {
      ifp->flags = NETIF_FLAG_LINK_UP | NETIF_FLAG_UP | NETIF_FLAG_IGMP;
      IP4_ADDR(ip_2_ip4(&((ifp)->ip_addr)), 127, 0, 0, 2);
      IP4_ADDR(ip_2_ip4(&((ifp)->netmask)), 255, 0, 0, 0);
      IP4_ADDR(ip_2_ip4(&((ifp)->gw)), 127, 0, 0, 2);
      ifp->name[0] = 'i';
      ifp->name[1] = 'm';
      ifp->num = (u8_t)vifcp->vifc_vifi;
    }
    
    if (register_if_num == VIFI_INVALID) {
      register_if_num = vifcp->vifc_vifi;
    }
  
  } else { /* TUNNEL or real */
    if (vifcp->vifc_flags & VIFF_USE_IFINDEX) {
      /* Find the interface with an index */
      NETIF_FOREACH(ifp) {
        if (netif_get_index(ifp) == vifcp->vifc_lcl_ifindex) {
          ifa.addr = netif_ip4_addr(ifp)->addr;
          vifcp->vifc_lcl_addr.s_addr = ifa.addr;
          break;
        }
      }
      
    } else {
      /* Find the interface with an address */
      ifa.addr = vifcp->vifc_lcl_addr.s_addr;
      NETIF_FOREACH(ifp) {
        if (ip4_addr_cmp(&ifa, netif_ip4_addr(ifp))) {
          break;
        }
      }
    }
    
    /* can not find netif */
    if (ifp == NULL) {
      return (EADDRNOTAVAIL);
    }
  
    if (vifcp->vifc_flags & VIFF_TUNNEL) { /* IPIP tunnel */
      /* An encapsulating tunnel is wanted.  Tell ipip_input() to
       * start paying attention to encapsulated packets. */
      if (have_encap_tunnel == 0) {
        have_encap_tunnel = 1;
        multicast_decap_if.name[0] = 'm';
        multicast_decap_if.name[1] = 'c';
        multicast_decap_if.num     = 254;
      }
    
      /* Set interface to fake encapsulator interface */
      ifp = &multicast_decap_if;
    
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
            ((struct sockaddr_in *)&(ifr.ifr_addr))->sin_family = AF_INET;
            ((struct sockaddr_in *)&(ifr.ifr_addr))->sin_addr.s_addr = INADDR_ANY;
            if (ifp->ioctl(ifp, SIOCADDMULTI, &ifr)) {
              return (EOPNOTSUPP);
            }
          }
        } else {
          return (EOPNOTSUPP);
        }
      }
    }
  }
  
  vifp->v_flags      = vifcp->vifc_flags & ~VIFF_USE_IFINDEX; /* VIFF_ flags defined above */
  vifp->v_threshold  = vifcp->vifc_threshold; /* min ttl required to forward on vif*/
  vifp->v_lcl_addr   = vifcp->vifc_lcl_addr;
  vifp->v_rmt_addr   = vifcp->vifc_rmt_addr;
  vifp->v_ifp        = ifp;
  
  /* initialize per vif pkt counters */
  vifp->v_pkt_in     = 0;
  vifp->v_pkt_out    = 0;
  vifp->v_bytes_in   = 0;
  vifp->v_bytes_out  = 0;
    
  /* Adjust numvifs up if the vifi is higher than numvifs */
  if (numvifs <= vifcp->vifc_vifi) {
    numvifs = vifcp->vifc_vifi + 1;
  }

  LWIP_DEBUGF(IP_DEBUG, ("ip4_mrt_add_vif #%d, lcladdr %x, %s %x, thresh %x, rate %d\n",
              vifcp->vifc_vifi, PP_NTOHL(vifcp->vifc_lcl_addr.s_addr),
              (vifcp->vifc_flags & VIFF_TUNNEL) ? "rmtaddr" : "mask", 
              PP_NTOHL(vifcp->vifc_rmt_addr.s_addr),
              vifcp->vifc_threshold, vifcp->vifc_rate_limit));
  return (0);
}

/* ip4_mrt_del_vif */
static u8_t ip4_mrt_del_vif (const vifi_t *vifip)
{
  struct vif *vifp = viftable + *vifip;
  vifi_t vifi;
  struct netif *ifp;
  struct ifreq ifr;
  int flags;
  
  if (*vifip >= numvifs) {
    return (EINVAL);
  }
  
  if (vifp->v_lcl_addr.s_addr == INADDR_ANY) {
    return (EADDRNOTAVAIL);
  }
  
  if (!(vifp->v_flags & (VIFF_TUNNEL | VIFF_REGISTER))) {
    ifp = vifp->v_ifp;
    if (ifp) {
      flags = netif_get_flags(ifp);
      if (flags & IFF_ALLMULTI) {
        ifr.ifr_name[0] = 0;
        ifr.ifr_flags   = flags & ~IFF_ALLMULTI;
        if (ifp->ioctl) {
          if (ifp->ioctl(ifp, SIOCSIFFLAGS, &ifr)) {
            ((struct sockaddr_in *)&(ifr.ifr_addr))->sin_family = AF_INET;
            ((struct sockaddr_in *)&(ifr.ifr_addr))->sin_addr.s_addr = INADDR_ANY;
            ifp->ioctl(ifp, SIOCDELMULTI, &ifr);
          }
        }
      }
    }
  }
  
  if (vifp == last_encap_vif) {
    last_encap_vif = NULL;
    last_encap_src = 0;
  }
  
  if (vifp->v_flags & VIFF_REGISTER) {
    register_if_num = VIFI_INVALID;
  }
  
  lib_bzero(vifp, sizeof (*vifp));
  
  /* Adjust numvifs down */
  for (vifi = numvifs; vifi > 0; vifi--) {
    if (viftable[vifi - 1].v_lcl_addr.s_addr != INADDR_ANY) {
      break;
    }
  }
  numvifs = vifi;
  
  LWIP_DEBUGF(IP_DEBUG, ("ip4_mrt_del_vif %d, numvifs %d\n", *vifip, numvifs));
  return (0);
}

/* ip4_mrt_add_mfc */
static u8_t ip4_mrt_add_mfc (struct mfcctl2 *mfccp)
{
  LW_LIST_LINE *pline;
  struct mfc *rt;
  u_long hash;
  u_short nstl;
  
  /* If an entry already exists, just update the fields */
  rt = ip4_mrt_find_mfc((uint32_t)mfccp->mfcc_origin.s_addr, (uint32_t)mfccp->mfcc_mcastgrp.s_addr);
  if (rt) {
    LWIP_DEBUGF(IP_DEBUG, ("ip4_mrt_add_mfc update o %x g %x p %x\n", 
                PP_NTOHL(mfccp->mfcc_origin.s_addr), 
                PP_NTOHL(mfccp->mfcc_mcastgrp.s_addr),
                mfccp->mfcc_parent));
    ip4_mrt_update_mfc(rt, mfccp);
    return (0);
  }
  
  /* Find the entry for which the upcall was made and update */
  hash = MFCHASH(mfccp->mfcc_origin.s_addr, mfccp->mfcc_mcastgrp.s_addr);
  for (pline = mfctable[hash], nstl = 0; pline != NULL; pline = _list_line_get_next(pline)) {
    rt = _LIST_ENTRY(pline, struct mfc, mfc_list);
    if ((rt->mfc_origin.s_addr == mfccp->mfcc_origin.s_addr) &&
        (rt->mfc_mcastgrp.s_addr == mfccp->mfcc_mcastgrp.s_addr) &&
        (rt->mfc_stall != NULL)) {
      if (nstl++) {
        LWIP_DEBUGF(IP_DEBUG, ("ip4_mrt_add_mfc %s o %x g %x p %x dbx %p\n",
                    "multiple kernel entries",
                    PP_NTOHL(mfccp->mfcc_origin.s_addr),
                    PP_NTOHL(mfccp->mfcc_mcastgrp.s_addr),
                    mfccp->mfcc_parent, rt->mfc_stall));
      }
      
      LWIP_DEBUGF(IP_DEBUG, ("ip4_mrt_add_mfc o %x g %x p %x dbg %p\n",
                  PP_NTOHL(mfccp->mfcc_origin.s_addr),
                  PP_NTOHL(mfccp->mfcc_mcastgrp.s_addr),
                  mfccp->mfcc_parent, rt->mfc_stall));
                  
      ip4_mrt_init_mfc(rt, mfccp);
      rt->mfc_expire = 0; /* Don't clean this guy up */
      nexpire[hash]--;
      
      /* free packets Qed at the end of this entry */
      while (rt->mfc_stall) {
        PLW_LIST_RING pring = _list_ring_get_prev(rt->mfc_stall);
        struct rtdetq *rte = _LIST_ENTRY(pring, struct rtdetq, list);
        _List_Ring_Del(&rte->list, &rt->mfc_stall);
        if (rte->ifp) {
          ip4_mrt_ip_mdq(&rte->p.pbuf, rte->ifp, rt);
        }
        mem_free(rte);
        rt->mfc_nstall--;
      }
    }
  }
  
  /* It is possible that an entry is being inserted without an upcall */
  if (nstl == 0) {
    LWIP_DEBUGF(IP_DEBUG, ("ip4_mrt_add_mfc no upcall h %d o %x g %x p %x\n",
                hash, PP_NTOHL(mfccp->mfcc_origin.s_addr),
                PP_NTOHL(mfccp->mfcc_mcastgrp.s_addr),
                mfccp->mfcc_parent));
                
    for (pline = mfctable[hash]; pline != NULL; pline = _list_line_get_next(pline)) {
      rt = _LIST_ENTRY(pline, struct mfc, mfc_list);
      if ((rt->mfc_origin.s_addr == mfccp->mfcc_origin.s_addr) &&
          (rt->mfc_mcastgrp.s_addr == mfccp->mfcc_mcastgrp.s_addr)) {
        ip4_mrt_init_mfc(rt, mfccp);
        if (rt->mfc_expire) {
          nexpire[hash]--;
        }
        rt->mfc_expire = 0;
        break;
      }
    }
    
    if (pline == NULL) {
      /* no upcall, so make a new entry */
      rt = (struct mfc *)mem_malloc(sizeof(struct mfc));
      if (rt == NULL) {
        return (ENOBUFS);
      }
      
      ip4_mrt_init_mfc(rt, mfccp);
      rt->mfc_expire = 0;
      rt->mfc_nstall = 0;
      rt->mfc_stall  = NULL;
      
      /* link into table */
      _List_Line_Add_Ahead(&rt->mfc_list, &mfctable[hash]);
    }
  }
  
  return (0);
}

/* ip4_mrt_del_mfc */
static u8_t ip4_mrt_del_mfc (struct mfcctl2 *mfccp)
{
  struct mfc *rt;
  u_long hash;
  
  LWIP_DEBUGF(IP_DEBUG, ("ip4_mrt_del_mfc orig %x mcastgrp %x\n",
              PP_NTOHL(mfccp->mfcc_origin.s_addr), 
              PP_NTOHL(mfccp->mfcc_mcastgrp.s_addr)));
              
  rt = ip4_mrt_find_mfc((uint32_t)mfccp->mfcc_origin.s_addr, (uint32_t)mfccp->mfcc_mcastgrp.s_addr);
  if (rt == NULL) {
    return (EADDRNOTAVAIL);
  }
  
  hash = MFCHASH(mfccp->mfcc_origin.s_addr, mfccp->mfcc_mcastgrp.s_addr);
  _List_Line_Del(&rt->mfc_list, &mfctable[hash]);
  mem_free(rt);
  
  return (0);
}

/* ip4_mrt_raw_send (send to daemon) */
static void ip4_mrt_raw_send (struct raw_pcb *pcb, struct pbuf *p, const ip4_addr_t *src)
{
  ip_addr_t ipsrc;

  ip_addr_copy_from_ip4(ipsrc, *src);
  pcb->recv(pcb->recv_arg, pcb, p, &ipsrc);
}

/* ip4_mrt_phyif_send */
static void ip4_mrt_phyint_send (struct vif *vifp, struct pbuf *p)
{
  struct ip_hdr *iphdr;
  struct pbuf *p_out;
  
  p_out = pbuf_alloc(PBUF_LINK, p->tot_len, PBUF_RAM); /* allocate a new pbuf for sending */
  if (p_out == NULL) {
    return; /* ENOBUFS */
  }
  pbuf_copy(p_out, p);
  
  iphdr = ptod(p_out, struct ip_hdr *);

  /* decrement TTL */
  IPH_TTL_SET(iphdr, IPH_TTL(iphdr) - 1);
  
  /* Incrementally update the IP checksum. */
  if (IPH_CHKSUM(iphdr) >= PP_HTONS(0xffffU - 0x100)) {
    IPH_CHKSUM_SET(iphdr, IPH_CHKSUM(iphdr) + PP_HTONS(0x100) + 1);
  } else {
    IPH_CHKSUM_SET(iphdr, IPH_CHKSUM(iphdr) + PP_HTONS(0x100));
  }
  
  /* TOS == 1 to determining this is a ip_mforward packet */
  ip4_output_if_src(p, NULL, LWIP_IP_HDRINCL, 0, 1, 0, vifp->v_ifp);
  pbuf_free(p_out);
}

/* ip4_mrt_encap_send */
static void ip4_mrt_encap_send (struct vif *vifp, struct pbuf *p)
{
  struct pbuf *p_out;
  struct ip_hdr *iphdr_out;
  struct ip_hdr *iphdr = ptod(p, struct ip_hdr *);
  struct netif *netif;
  ip4_addr_t src, dest;
  u16_t *id = ip4_get_ip_id();
  u16_t len;
  int mtu;
  
  mtu = 0xffff - sizeof(struct ip_hdr);
  if (PP_NTOHS(IPH_LEN(iphdr)) > mtu) {
    return; /* to big!!! */
  }
  
  p_out = pbuf_alloc(PBUF_LINK + IP_HLEN, p->tot_len, PBUF_RAM); /* allocate a new pbuf for sending */
  if (p_out == NULL) {
    return; /* ENOBUFS */
  }
  pbuf_copy(p_out, p);
  
  iphdr = ptod(p_out, struct ip_hdr *); /* inner iphdr */
  pbuf_header(p_out, IP_HLEN); /* move to outer header */
  iphdr_out = ptod(p_out, struct ip_hdr *); /* outer iphdr */
  
  /* decrement TTL */
  IPH_TTL_SET(iphdr, IPH_TTL(iphdr) - 1);
  
  /* Incrementally update the IP checksum. */
  if (IPH_CHKSUM(iphdr) >= PP_HTONS(0xffffU - 0x100)) {
    IPH_CHKSUM_SET(iphdr, IPH_CHKSUM(iphdr) + PP_HTONS(0x100) + 1);
  } else {
    IPH_CHKSUM_SET(iphdr, IPH_CHKSUM(iphdr) + PP_HTONS(0x100));
  }
  
  /* fill in the encapsulating IP header. */
  *iphdr_out = multicast_encap_iphdr;
  IPH_ID_SET(iphdr_out, PP_HTONS(*id));
  (*id)++;
  len = sizeof(multicast_encap_iphdr) + PP_NTOHS(IPH_LEN(iphdr));
  IPH_LEN_SET(iphdr_out, PP_HTONS(len));
  IPH_TOS_SET(iphdr_out, IPH_TOS(iphdr));
  iphdr_out->src.addr  = vifp->v_lcl_addr.s_addr;
  iphdr_out->dest.addr = vifp->v_rmt_addr.s_addr;
  
  src.addr = iphdr_out->src.addr;
  dest.addr = iphdr_out->dest.addr;
  netif = ip4_route_src(&src, &dest); /* find route */
  if (netif == NULL) {
    IP_STATS_INC(ip.rterr);
    goto out;
  }
  
  IF__NETIF_CHECKSUM_ENABLED(netif, NETIF_CHECKSUM_GEN_IP) {
    IPH_CHKSUM_SET(iphdr_out, inet_chksum(iphdr_out, IPH_HL(iphdr_out) << 2));
  }
  
  ip4_output_if_src(p_out, NULL, LWIP_IP_HDRINCL, 0, 0, 0, netif);
  
out:
  pbuf_free(p_out);
}

/* ip4_mrt_pim_send_rp */
static void ip4_mrt_pim_send_rp (const struct ip_hdr *iphdr, struct vif *vifp, struct pbuf *p, struct mfc *rt)
{
  struct ip_hdr *iphdr_out;
  struct pim_encap_pimhdr *pimhdr;
  struct netif *netif;
  ip4_addr_t src, dest;
  u16_t len = PP_NTOHS(IPH_LEN(iphdr));
  u16_t *id = ip4_get_ip_id();
  vifi_t vifi = rt->mfc_parent;
  
  if ((vifi >= numvifs) || (viftable[vifi].v_lcl_addr.s_addr == INADDR_ANY)) {
    return;
  }
  
  iphdr_out = ptod(p, struct ip_hdr *);
  *iphdr_out = pim_encap_iphdr;
  len = PP_HTONS(len + sizeof(pim_encap_iphdr) + sizeof(pim_encap_pimhdr));
  IPH_LEN_SET(iphdr_out, len);
  iphdr_out->src.addr = viftable[vifi].v_lcl_addr.s_addr;
  iphdr_out->dest.addr = rt->mfc_rp.s_addr;
  
  /* Copy the inner header TOS to the outer header, and take care of the
   * IP_DF bit. */
  IPH_TOS_SET(iphdr_out, IPH_TOS(iphdr));
  if (IPH_OFFSET(iphdr) & PP_HTONS(IP_DF)) {
    IPH_OFFSET_SET(iphdr_out, PP_HTONS(IP_DF));
    IPH_ID_SET(iphdr_out, 0);
  
  } else {
    IPH_ID_SET(iphdr_out, PP_HTONS(*id));
    (*id)++;
  }
  
  pimhdr = (struct pim_encap_pimhdr *)((caddr_t)iphdr_out + sizeof(pim_encap_iphdr));
  *pimhdr = pim_encap_pimhdr;
  /* If the iif crosses a border, set the Border-bit */
  if (rt->mfc_flags[vifi] & MRT_MFC_FLAGS_BORDER_VIF & mrt_api_config) {
    pimhdr->flags |= PP_HTONL(PIM_BORDER_REGISTER);
  }
  pimhdr->pim.pim_cksum = inet_chksum(pimhdr, sizeof(pim_encap_pimhdr));
  
  src.addr = iphdr_out->src.addr;
  dest.addr = iphdr_out->dest.addr;
  netif = ip4_route_src(&src, &dest); /* find route */
  if (netif == NULL) {
    IP_STATS_INC(ip.rterr);
    return;
  }
  
  /* Keep statistics */
  PIMSTAT_INC(pims_snd_registers_msgs);
  PIMSTAT_ADD(pims_snd_registers_bytes, len);
  
  IF__NETIF_CHECKSUM_ENABLED(netif, NETIF_CHECKSUM_GEN_IP) {
    IPH_CHKSUM_SET(iphdr_out, inet_chksum(iphdr_out, IPH_HL(iphdr_out) << 2));
  }
  
  ip4_output_if_src(p, NULL, LWIP_IP_HDRINCL, 0, 0, 0, netif);
}

/* ip4_mrt_pim_send_upcall */
static void ip4_mrt_pim_send_upcall (const struct ip_hdr *iphdr, struct vif *vifp, struct pbuf *p)
{
  struct igmpmsg *im;
  int len = PP_NTOHS(IPH_LEN(iphdr));
  ip4_addr_t src;
  
  /* Send message to routing daemon to install
   * a route into the kernel table */
  im = ptod(p, struct igmpmsg *);
  im->im_msgtype    = IGMPMSG_WHOLEPKT;
  im->im_mbz        = 0;
  im->im_vif        = vifp - viftable;
  im->im_src.s_addr = iphdr->src.addr;
  im->im_dst.s_addr = iphdr->dest.addr;
  
  src.addr = iphdr->src.addr;
  ip4_mrt_raw_send(ip_mroute_pcb, p, &src); /* send to daemon */
  
  /* Keep statistics */
  PIMSTAT_INC(pims_snd_registers_msgs);
  PIMSTAT_ADD(pims_snd_registers_bytes, len);
}

/* ip4_mrt_pim_send */
static void ip4_mrt_pim_send (struct vif *vifp, struct pbuf *p, struct mfc *rt)
{
  struct ip_hdr *iphdr = ptod(p, struct ip_hdr *);
  struct pbuf *p_pim;
  int mtu;
  
  mtu = 0xffff - sizeof(pim_encap_iphdr) - sizeof(pim_encap_pimhdr);
  if (PP_NTOHS(IPH_LEN(iphdr)) > mtu) {
    return; /* to big!!! */
  }
  
  if (mrt_api_config & MRT_MFC_RP) {
    if (rt->mfc_rp.s_addr == INADDR_ANY) {
      return; /* rt->mfc_rp.s_addr must not 0 */
    }
  }
  
  /* allocate a new pbuf for sending */
  p_pim = pbuf_alloc(PBUF_LINK + sizeof(pim_encap_iphdr) + sizeof(pim_encap_pimhdr), p->tot_len, PBUF_RAM);
  if (p_pim == NULL) {
    return; /* ENOBUF */
  }
  pbuf_copy(p_pim, p);
  iphdr = ptod(p_pim, struct ip_hdr *); /* inner iphdr */
  
  /* decrement TTL */
  IPH_TTL_SET(iphdr, IPH_TTL(iphdr) - 1);
  
  /* Incrementally update the IP checksum. */
  if (IPH_CHKSUM(iphdr) >= PP_HTONS(0xffffU - 0x100)) {
    IPH_CHKSUM_SET(iphdr, IPH_CHKSUM(iphdr) + PP_HTONS(0x100) + 1);
  } else {
    IPH_CHKSUM_SET(iphdr, IPH_CHKSUM(iphdr) + PP_HTONS(0x100));
  }

  if (mrt_api_config & MRT_MFC_RP) {
    pbuf_header(p_pim, sizeof(pim_encap_iphdr) + sizeof(pim_encap_pimhdr)); /* move p_pim to outer header */
    ip4_mrt_pim_send_rp(iphdr, vifp, p_pim, rt);
  
  } else {
    pbuf_header(p_pim, sizeof(struct igmpmsg));
    ip4_mrt_pim_send_upcall(iphdr, vifp, p_pim);
  }
  
  pbuf_free(p_pim);
}

/* ip4_mrt_ip_mdq */
static err_t ip4_mrt_ip_mdq (struct pbuf *p, struct netif *ifp, struct mfc *rt)
{
  struct ip_hdr *iphdr = ptod(p, struct ip_hdr *);
  vifi_t vifi;
  int plen = PP_NTOHS(IPH_LEN(iphdr));

  /*
   * Don't forward if it didn't arrive from the parent vif for its origin.
   */
  vifi = rt->mfc_parent;
  if ((vifi >= numvifs) || (viftable[vifi].v_ifp != ifp)) {
    LWIP_DEBUGF(IP_DEBUG, ("ip4_mrt_ip_mdq rx on wrong ifp %p (vifi %d, v_ifp %p)\n",
                ifp, (int)vifi, viftable[vifi].v_ifp));
    MRTSTAT_INC(mrts_wrong_if);
    ++rt->mfc_wrong_if;
    
    /* If we are doing PIM assert processing, send a message
     * to the routing daemon.
     *
     * XXX: A PIM-SM router needs the WRONGVIF detection so it
     * can complete the SPT switch, regardless of the type
     * of the iif (broadcast media, GRE tunnel, etc). */
    if (pim_assert && (vifi < numvifs) && viftable[vifi].v_ifp) {
      if (ifp == &multicast_register_if) {
        PIMSTAT_INC(pims_rcv_registers_wrongiif);
      }
    
      /* Locate the vifi for the incoming interface for this packet.
       * If none found, drop packet. */
      for (vifi = 0; vifi < numvifs && viftable[vifi].v_ifp != ifp; vifi++);
      if (vifi >= numvifs) { /* vif not found, drop packet */
        return (ERR_OK);
      }
      
      if (ip4_mrt_rate_check(&rt->mfc_last_assert, &pim_assert_interval)) {
        struct pbuf *toup;
        struct igmpmsg *im;
        int hlen = IPH_HL(iphdr) << 2;
        ip4_addr_t src;
        
        /* Send message to routing daemon to install
         * a route into the kernel table */
        toup = pbuf_alloc(PBUF_RAW, (u16_t)max(sizeof(struct igmpmsg), hlen), PBUF_RAM);
        if (toup == NULL) {
          return (ERR_OK); /* ENOBUFS */
        }
        
        pbuf_copy_partial(p, ptod(toup, void *), (u16_t)hlen, 0); /* copy header to toup */
        im = ptod(toup, struct igmpmsg *); /* igmp msg header */
        im->im_msgtype = IGMPMSG_WRONGVIF;
        im->im_mbz     = 0; /* must be zero */
        im->im_vif     = (u_char)vifi;

        MRTSTAT_INC(mrts_upcalls);
        
        src.addr = iphdr->src.addr;
        ip4_mrt_raw_send(ip_mroute_pcb, toup, &src); /* send to daemon */
        mem_free(toup);
      }
    }
    
    return (ERR_OK);
  }
  
  /* If I sourced this packet, it counts as output, else it was input. */
  if (iphdr->src.addr == viftable[vifi].v_lcl_addr.s_addr) {
    viftable[vifi].v_pkt_out++;
    viftable[vifi].v_bytes_out += plen;
  
  } else {
    viftable[vifi].v_pkt_in++;
    viftable[vifi].v_bytes_in += plen;
  }
  
  rt->mfc_pkt_cnt++;
  rt->mfc_byte_cnt += plen;
  
  /* For each vif, decide if a copy of the packet should be forwarded.
   * Forward if:
   *        - the ttl exceeds the vif's threshold
   *        - there are group members downstream on interface */
  for (vifi = 0; vifi < numvifs; vifi++) {
    if ((viftable[vifi].v_ifp) && 
        (rt->mfc_ttls[vifi] > 0) && 
        (IPH_TTL(iphdr) > rt->mfc_ttls[vifi])) {
      viftable[vifi].v_pkt_out++;
      viftable[vifi].v_bytes_out += plen;
      if (viftable[vifi].v_flags & VIFF_REGISTER) {
        ip4_mrt_pim_send(viftable + vifi, p, rt);
      
      } else if (viftable[vifi].v_flags & VIFF_TUNNEL) {
        ip4_mrt_encap_send(viftable + vifi, p);
      
      } else {
        ip4_mrt_phyint_send(viftable + vifi, p);
      }
    }
  }
  
  return (ERR_OK);
}

/* ip4_mrt_forward 
   return no ERR_OK ip_input will drop this packet */
err_t ip4_mrt_forward (struct pbuf *p, const struct ip_hdr *iphdr, struct netif *ifp)
{
#define IP_HDR_LEN  20  /* # bytes of fixed IP header (excluding options) */
#define TUNNEL_LEN  12  /* # bytes of IP option for tunnel encapsulation  */

#ifndef IPOPT_LSRR
#define IPOPT_LSRR  131
#endif

  static int srctun = 0;
  PLW_LIST_LINE pline;
  struct mfc *rt;
  vifi_t vifi;

  if (ip_mroute_pcb == NULL) { /* not init */
    return (ERR_OK);
  }
  
  LWIP_DEBUGF(IP_DEBUG, ("ip4_mrt_forward src %x, dst %x, ifp index %d\n",
              PP_NTOHL(iphdr->src.addr), PP_NTOHL(iphdr->dest.addr), netif_get_index(ifp)));
  
  if (IPH_HL(iphdr) < ((IP_HLEN + TUNNEL_LEN) >> 2) ||
      ((u_char *)(iphdr + 1))[1] != IPOPT_LSRR) {
    /* Packet arrived via a physical interface or
     * an encapsulated tunnel. */
  
  } else {
    /* Packet arrived through a source-route tunnel.
     * Source-route tunnels are no longer supported. */
    if ((srctun++ % 1000) == 0) {
      LWIP_DEBUGF(IP_DEBUG, ("ip4_mrt_forward: received source-routed packet from %x\n",
                  PP_NTOHL(iphdr->src.addr)));
    }
    return (ERR_OK);
  }
  
  /* Don't forward a packet with time-to-live of zero or one,
   * or a packet destined to a local-only group. */
  if (IPH_TTL(iphdr) <= 1 || 
      PP_NTOHL(iphdr->dest.addr) <= INADDR_MAX_LOCAL_GROUP) {
    return (ERR_OK);
  }
  
  /* Determine forwarding vifs from the forwarding cache table */
  /* Entry exists, so forward if necessary */
  rt = ip4_mrt_find_mfc((uint32_t)iphdr->src.addr, (uint32_t)iphdr->dest.addr);
  if (rt != NULL) {
    return (ip4_mrt_ip_mdq(p, ifp, rt));
  
  } else {
    struct rtdetq *rte;
    u_long hash;
    int hlen = IPH_HL(iphdr) << 2;
  
    /* If we don't have a route for packet's origin,
     * Make a copy of the packet &
     * send message to routing daemon */
    MRTSTAT_INC(mrts_no_route);
    
    LWIP_DEBUGF(IP_DEBUG, ("ip4_mrt_forward no rte s %x g %x\n",
                PP_NTOHL(iphdr->src.addr), PP_NTOHL(iphdr->dest.addr)));
    
    /* Allocate pbufs early so that we don't do extra work if we are
     * just going to fail anyway.  Make sure to pullup the header so
     * that other people can't step on it. */
    rte = ip4_mrt_rtdetq_alloc(p->tot_len, 0);
    if (rte == NULL) {
      return (ERR_OK);
    }
    pbuf_copy(&rte->p.pbuf, p);
    
    hash = MFCHASH(iphdr->src.addr, iphdr->dest.addr);
    for (pline = mfctable[hash]; pline != NULL; pline = _list_line_get_next(pline)) {
      rt = _LIST_ENTRY(pline, struct mfc, mfc_list);
      if ((rt->mfc_origin.s_addr == iphdr->src.addr) &&
          (rt->mfc_mcastgrp.s_addr == iphdr->dest.addr) &&
          (rt->mfc_stall != NULL)) {
        break;
      }
    }
    
    if (pline == NULL) {
      struct pbuf *toup;
      struct igmpmsg *im;
      ip4_addr_t src;
      int i;
      
      /* Locate the vifi for the incoming interface for this packet.
       * If none found, drop packet. */
      for (vifi = 0; vifi < numvifs && viftable[vifi].v_ifp != ifp; vifi++);
      if (vifi >= numvifs) { /* vif not found, drop packet */
        mem_free(rte);
        return (ERR_OK);
      }
      
      /* no upcall, so make a new entry */
      rt = (struct mfc *)mem_malloc(sizeof(struct mfc));
      if (rt == NULL) {
        mem_free(rte);
        return (ERR_OK);
      }
      
      /* Send message to routing daemon to install
       * a route into the kernel table */
      toup = pbuf_alloc(PBUF_RAW, (u16_t)max(sizeof(struct igmpmsg), hlen), PBUF_RAM);
      if (toup == NULL) {
        mem_free(rt);
        mem_free(rte);
        return (ERR_OK);
      }
      
      pbuf_copy_partial(p, ptod(toup, void *), (u16_t)hlen, 0); /* copy header to toup */
      im = ptod(toup, struct igmpmsg *); /* igmp msg header */
      im->im_msgtype = IGMPMSG_NOCACHE;
      im->im_mbz     = 0; /* must be zero */
      im->im_vif     = (u_char)vifi;

      MRTSTAT_INC(mrts_upcalls);
      
      src.addr = iphdr->src.addr;
      ip4_mrt_raw_send(ip_mroute_pcb, toup, &src); /* send to daemon */
      mem_free(toup);
      
      /* insert new entry at head of hash chain */
      rt->mfc_origin.s_addr   = iphdr->src.addr;
      rt->mfc_mcastgrp.s_addr = iphdr->dest.addr;
      rt->mfc_expire          = UPCALL_EXPIRE;
      nexpire[hash]++;
      for (i = 0; i < numvifs; i++) {
        rt->mfc_ttls[i] = 0;
        rt->mfc_flags[i] = 0;
      }
      rt->mfc_parent = (vifi_t)-1;
      
      /* initialize pkt counters per src-grp */
      rt->mfc_pkt_cnt  = 0;
      rt->mfc_byte_cnt = 0;
      rt->mfc_wrong_if = 0;
      timerclear(&rt->mfc_last_assert);
      rt->mfc_stall  = NULL;
      rt->mfc_nstall = 0;
      
      _List_Line_Add_Ahead(&rt->mfc_list, &mfctable[hash]);
      _List_Ring_Add_Ahead(&rte->list, &rt->mfc_stall);
      rt->mfc_nstall++;
      
    } else {
      if (rt->mfc_nstall > MAX_UPQ) {
        MRTSTAT_INC(mrts_upq_ovflw);
        mem_free(rte);
        return (ERR_OK);
      }
      
      _List_Ring_Add_Ahead(&rte->list, &rt->mfc_stall);
      rt->mfc_nstall++;
    }
    
    rte->ifp = ifp;
  }
  
  return (ERR_OK);
}

/* ip4_mrt_ipip_input (return: eaten) */
u8_t ip4_mrt_ipip_input (struct pbuf *p, struct netif *inp)
{
  struct vif *vifp;
  struct ip_hdr *iphdr = ptod(p, struct ip_hdr *);
  int hlen = IPH_HL(iphdr) << 2;
  
  if (!have_encap_tunnel) {
    return (0);
  }
  
  /* dump the packet if it's not to a multicast destination or if
   * we don't have an encapsulating tunnel with the source.
   * Note:  This code assumes that the remote site IP address
   * uniquely identifies the tunnel (i.e., that this site has
   * at most one tunnel with the remote site). */
  if (!IP_MULTICAST(PP_NTOHL(((struct ip_hdr *)((char *)iphdr + hlen))->dest.addr))) {
    MRTSTAT_INC(mrts_bad_tunnel);
    pbuf_free(p);
    return (1);
  }
  
  if (iphdr->src.addr != last_encap_src) {
    struct vif *vife;
    
    vifp = viftable;
    vife = vifp + numvifs;
    last_encap_src = iphdr->src.addr;
    last_encap_vif = NULL;
    for (; vifp < vife; ++vifp) {
      if (vifp->v_rmt_addr.s_addr == iphdr->src.addr) {
        if ((vifp->v_flags & (VIFF_TUNNEL | VIFF_SRCRT)) == VIFF_TUNNEL) {
          last_encap_vif = vifp;
          break;
        }
      }
    }
  }
  
  if ((vifp = last_encap_vif) == NULL) {
    last_encap_src = 0;
    MRTSTAT_INC(mrts_cant_tunnel);
    pbuf_free(p);
    LWIP_DEBUGF(IP_DEBUG, ("ip4_mrt_ipip_input: no tunnel with %x\n",
                PP_NTOHL(iphdr->src.addr)));
    return (1);
  }
  
  p->flags |= PBUF_FLAG_LLMCAST; /* input is a mcast packet */
  pbuf_header(p, (u16_t)-hlen); /* remove outer ip header */
  if (tcpip_inpkt(p, inp, ip_input)) { /* use inp ??? */
    pbuf_free(p); /* loop error */
  }
  
  return (1);
}

/* ip4_mrt_pim_input (return: eaten) */
u8_t ip4_mrt_pim_input (struct pbuf *p, struct netif *inp)
{
  struct ip_hdr *iphdr = ptod(p, struct ip_hdr *);
  struct pim *pim;
  int iphlen = IPH_HL(iphdr) << 2;
  int datalen = PP_NTOHS(IPH_LEN(iphdr)) - iphlen;
  int ip_tos;
  ip4_addr_t src;
  
  /* get source */
  src.addr = iphdr->src.addr;
  
  /* Keep statistics */
  PIMSTAT_INC(pims_rcv_total_msgs);
  PIMSTAT_ADD(pims_rcv_total_bytes, datalen);
  
  if (datalen < PIM_MINLEN) {
    PIMSTAT_INC(pims_rcv_tooshort);
    LWIP_DEBUGF(IP_DEBUG, ("ip4_mrt_pim_input short packet (%d) from 0x%08x\n",
                datalen, PP_NTOHL(iphdr->src.addr)));
    pbuf_free(p);
    return (1);
  }
  
  ip_tos = IPH_TOS(iphdr);
  
  if (p->len < (iphlen + PIM_REG_MINLEN)) {
    PIMSTAT_INC(pims_rcv_tooshort);
    pbuf_free(p);
    return (1);
  }
  
  pim = (struct pim *)((caddr_t)iphdr + iphlen);
  if ((PIM_VT_T(pim->pim_vt) == PIM_REGISTER) && (inet_chksum(pim, PIM_MINLEN) == 0)) {
    PIMSTAT_INC(pims_rcv_badsum);
    LWIP_DEBUGF(IP_DEBUG, ("ip4_mrt_pim_input invalid checksum\n"));
    pbuf_free(p);
    return (1);
  }
  
  /* PIM version check */
  if (PIM_VT_V(pim->pim_vt) < PIM_VERSION) {
    PIMSTAT_INC(pims_rcv_badversion);
    LWIP_DEBUGF(IP_DEBUG, ("ip4_mrt_pim_input bad version %d expect %d\n",
                (int)PIM_VT_V(pim->pim_vt), PIM_VERSION));
    pbuf_free(p);
    return (1);
  }
  
  if (PIM_VT_T(pim->pim_vt) == PIM_REGISTER) {
    /* Since this is a REGISTER, we'll make a copy of the register
     * headers ip + pim + u_int32 + encap_ip, to be passed up to the
     * routing daemon. */
    uint32_t *reghdr;
    struct ip_hdr *encap_iphdr;
    struct pbuf *input;
     
    if ((register_if_num >= numvifs) || (register_if_num == VIFI_INVALID)) {
      LWIP_DEBUGF(IP_DEBUG, ("ip4_mrt_pim_input register vif not set: %d\n", (int)register_if_num));
      pbuf_free(p);
      return (1);
    }
    
    reghdr = (uint32_t *)(pim + 1);
    encap_iphdr = (struct ip_hdr *)(reghdr + 1);
    
    LWIP_DEBUGF(IP_DEBUG, ("ip4_mrt_pim_input register: encap ip src 0x%08x len %d\n",
                PP_NTOHL(encap_iphdr->src.addr), PP_NTOHS(IPH_LEN(encap_iphdr))));
    
    /* verify the version number of the inner packet */
    if (IPH_V(encap_iphdr) != 4) {
      PIMSTAT_INC(pims_rcv_badregisters);
      LWIP_DEBUGF(IP_DEBUG, ("ip4_mrt_pim_input bad encap ip version\n"));
      pbuf_free(p);
      return (1);
    }
    
    /* verify the inner packet is destined to a mcast group */
    if (!IN_MULTICAST(PP_NTOHL(encap_iphdr->dest.addr))) {
      PIMSTAT_INC(pims_rcv_badregisters);
      LWIP_DEBUGF(IP_DEBUG, ("ip4_mrt_pim_input bad encap ip dest 0x%08x\n",
                  PP_NTOHL(encap_iphdr->dest.addr)));
      pbuf_free(p);
      return (1);
    }
    
    /* If a NULL_REGISTER, pass it to the daemon */
    if (PP_NTOHL((*reghdr)) & PIM_NULL_REGISTER) {
      goto pim_input_to_daemon;
    }
    
    if (IPH_TOS(encap_iphdr) != ip_tos) {
      IPH_TOS_SET(encap_iphdr, ip_tos);
      IPH_CHKSUM_SET(encap_iphdr, 0);
      IPH_CHKSUM_SET(encap_iphdr, inet_chksum(encap_iphdr, IPH_HL(encap_iphdr) << 2));
    }
    
    input = pbuf_alloc(PBUF_RAW, p->tot_len, PBUF_RAM);
    if (input == NULL) {
      pbuf_free(p);
      return (1);
    }
    pbuf_copy(input, p);
    input->flags |= PBUF_FLAG_LLMCAST; /* input is a mcast packet */
    
    /* remove outer ip and pimhdr */
    pbuf_header(input, (u16_t)-(iphlen + sizeof(struct pim_encap_pimhdr)));
    if (tcpip_inpkt(input, inp, ip_input)) { /* use inp ??? */
      pbuf_free(input); /* loop error */
    }
    
    PIMSTAT_INC(pims_rcv_registers_msgs);
    PIMSTAT_ADD(pims_rcv_registers_bytes, PP_NTOHS(IPH_LEN(encap_iphdr)));
    
    /* pass the 'head' only up to the daemon. This includes the
     * outer IP header, PIM header, PIM-Register header and the inner IP header */
    pbuf_realloc(p, (u16_t)(iphlen + sizeof(struct pim_encap_pimhdr) + PP_NTOHS(IPH_LEN(encap_iphdr))));
  }
  
pim_input_to_daemon:
  /* Pass the PIM message up to the daemon; if it is a Register message,
   * pass the 'head' only up to the daemon. This includes the
   * outer IP header, PIM header, PIM-Register header and the
   * inner IP header.
   * XXX: the outer IP header pkt size of a Register is not adjust to
   * reflect the fact that the inner multicast data is truncated. */
  if (ip_mroute_pcb) {
    ip4_mrt_raw_send(ip_mroute_pcb, p, &src); /* daemon will recv this packet */
  }
  pbuf_free(p);
  return (1); 
}

/* socket opt cmd: 
 * MRT_INIT / MRT_DONE / MRT_ADD_VIF / 
 * MRT_DEL_VIF / MRT_ADD_MFC / MRT_DEL_MFC */
u8_t ip4_mrt_setsockopt (struct raw_pcb *pcb, int optname, const void *optval, socklen_t optlen)
{
  struct mfcctl2 mfcctl2;
  
  if ((pcb != ip_mroute_pcb) && (optname != MRT_INIT)) {
    return (EPERM);
  }

  switch (optname) {
  
  case MRT_INIT:
    if (pcb && optval && (optlen >= sizeof(int))) {
      return (ip4_mrt_init(pcb, *(int *)optval));
    } else {
      return (EINVAL);
    }
    break;
    
  case MRT_DONE:
    return (ip4_mrt_down(pcb));
  
  case MRT_ADD_VIF:
    if (optval && (optlen >= sizeof(struct vifctl))) {
      return (ip4_mrt_add_vif((struct vifctl *)optval));
    } else {
      return (EINVAL);
    }
    break;
  
  case MRT_DEL_VIF:
    if (optval && (optlen >= sizeof(vifi_t))) {
      return (ip4_mrt_del_vif((vifi_t *)optval));
    } else {
      return (EINVAL);
    }
    break;
  
  case MRT_ADD_MFC:
  case MRT_DEL_MFC:
    if ((optname == MRT_ADD_MFC) && 
        (mrt_api_config & MRT_MFC_RP)) {
      if (optlen < sizeof(struct mfcctl2)) {
        return (EINVAL);
      }
      lib_memcpy(&mfcctl2, optval, sizeof(struct mfcctl2));
    
    } else {
      if (optlen < sizeof(struct mfcctl)) {
        return (EINVAL);
      }
      lib_memcpy(&mfcctl2, optval, sizeof(struct mfcctl));
      lib_bzero((caddr_t)&mfcctl2 + sizeof(struct mfcctl),
                sizeof(mfcctl2) - sizeof(struct mfcctl));
    }
    
    if (optname == MRT_ADD_MFC) {
      return (ip4_mrt_add_mfc(&mfcctl2));
    
    } else {
      return (ip4_mrt_del_mfc(&mfcctl2));
    }
    break;
    
  case MRT_ASSERT:
    if (optval && (optlen >= sizeof(int))) {
      pim_assert = *(int *)optval;
      return (0);
    } else {
      return (EINVAL);
    }
    break;
    
  case MRT_API_CONFIG:
    if (optval && (optlen >= sizeof(uint32_t))) {
      return (ip4_mrt_api_config((uint32_t *)optval));
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
 * MRT_VERSION / MRT_ASSERT */
u8_t ip4_mrt_getsockopt (struct raw_pcb *pcb, int optname, const void *optval, socklen_t *optlen)
{
  static int version = 0x0305;

  switch (optname) {
  
  case MRT_VERSION:
    if (optval && optlen && (*optlen >= sizeof(int))) {
      *(int *)optval = version;
    } else {
      return (EINVAL);
    }
    break;
  
  case MRT_ASSERT:
    if (optval && optlen && (*optlen >= sizeof(int))) {
      *(int *)optval = pim_assert;
    } else {
      return (EINVAL);
    }
    break;
    
  case MRT_API_SUPPORT:
    if (optval && optlen && (*optlen >= sizeof(uint32_t))) {
      *(uint32_t *)optval = mrt_api_support;
    } else {
      return (EINVAL);
    }
    break;
    
  case MRT_API_CONFIG:
    if (optval && optlen && (*optlen >= sizeof(uint32_t))) {
      *(uint32_t *)optval = mrt_api_config;
    } else {
      return (EINVAL);
    }
    break;
  
  default:
    return (ENOSYS);
  }
  
  return (0);
}

/* ip4_mrt_get_sg_cnt */
static u8_t ip4_mrt_get_sg_cnt (struct sioc_sg_req *req)
{
  struct mfc *rt;
  
  rt = ip4_mrt_find_mfc((uint32_t)req->src.s_addr, (uint32_t)req->grp.s_addr);
  if (rt == NULL) {
    req->pktcnt = req->bytecnt = req->wrong_if = 0xffffffff;
    return (EADDRNOTAVAIL);
    
  } else {
    req->pktcnt = rt->mfc_pkt_cnt;
    req->bytecnt = rt->mfc_byte_cnt;
    req->wrong_if = rt->mfc_wrong_if;
  }
  
  return (0);
}

/* ip4_mrt_get_vif_cnt */
static u8_t ip4_mrt_get_vif_cnt (struct sioc_vif_req *req)
{
  vifi_t vifi = req->vifi;
  
  if (vifi >= numvifs) {
    return (EINVAL);
  }
  
  req->icount = viftable[vifi].v_pkt_in;
  req->ocount = viftable[vifi].v_pkt_out;
  req->ibytes = viftable[vifi].v_bytes_in;
  req->obytes = viftable[vifi].v_bytes_out;
  
  return (0);
}

/* Handle ioctl commands to obtain information from the cache */
u8_t ip4_mrt_ioctl (int cmd, void *data)
{
  switch (cmd) {
  
  case SIOCGETVIFCNT:
    return (ip4_mrt_get_vif_cnt((struct sioc_vif_req *)data));
    
  case SIOCGETSGCNT:
    return (ip4_mrt_get_sg_cnt((struct sioc_sg_req *)data));
  
  default:
    break;
  }
  
  return (EINVAL);
}

/* ip4_mrt_clean */
void ip4_mrt_clean (struct raw_pcb *pcb)
{
  if (ip_mroute_pcb == pcb) {
    ip4_mrt_down(pcb);
  }
}

/* ip4_mrt_mcast_src */
u32_t ip4_mrt_mcast_src (int vifi)
{
  if (vifi >= 0 && vifi < numvifs) {
    return (viftable[vifi].v_lcl_addr.s_addr);
  } else {
    return (IPADDR_ANY);
  }
}

/* ip4_mrt_legal_vif_num */
int ip4_mrt_legal_vif_num (int vifi)
{
  if (vifi >= 0 && vifi < numvifs) {
    return (1);
  } else {
    return (0);
  }
}

/* ip4_mrt_is_on */
int ip4_mrt_is_on (void)
{
  return (ip_mroute_pcb ? 1 : 0);
}

/* ip4_mrt_traversal_mfc */
void ip4_mrt_traversal_mfc (void (*callback)(), void *arg0, void *arg1,
                            void *arg2, void *arg3, void *arg4, void *arg5)
{
  int i;
  struct vif *vifp;
  struct mfc *rt;
  LW_LIST_LINE *pline;
  
  for (i = 0; i < MFCTBLSIZ; i++) {
    for (pline = mfctable[i]; pline != NULL; pline = _list_line_get_next(pline)) {
      rt = _LIST_ENTRY(pline, struct mfc, mfc_list);
      if (rt->mfc_stall == NULL) {
        vifp = NULL;
        if (rt->mfc_parent < numvifs) {
          if (viftable[rt->mfc_parent].v_lcl_addr.s_addr) {
            vifp = &viftable[rt->mfc_parent];
          }
        }
        callback(rt, vifp, arg0, arg1, arg2, arg3, arg4, arg5);
      }
    }
  }
}

/* ip4_mrt_if_detach */
void ip4_mrt_if_detach (struct netif *ifp)
{
  PLW_LIST_LINE pline;
  struct mfc *rt;
  vifi_t vifi;
  u_long i;
  
  if (ip_mroute_pcb == NULL) {
    return;
  }
  
  MRT_LOCK();
  
  /* Tear down multicast forwarder state associated with this ifnet.
   * 1. Walk the vif list, matching vifs against this ifnet.
   * 2. Walk the multicast forwarding cache (mfc) looking for
   *    inner matches with this vif's index.
   * 3. Expire any matching multicast forwarding cache entries.
   * 4. Free vif state. This should disable ALLMULTI on the interface. */
  for (vifi = 0; vifi < numvifs; vifi++) {
    if (viftable[vifi].v_ifp != ifp) {
      continue;
    }
    for (i = 0; i < MFCTBLSIZ; i++) {
      pline = mfctable[i];
      while (pline) {
        rt = _LIST_ENTRY(pline, struct mfc, mfc_list);
        pline = _list_line_get_next(pline);
        if (rt->mfc_parent == vifi) {
          ip4_mrt_expire_mfc(rt);
        }
      }
    }
    ip4_mrt_del_vif(&vifi);
  }

  MRT_UNLOCK();
}

/* ip4_mrt_mrtstat */
struct mrtstat *ip4_mrt_mrtstat (void)
{
  return (&mrtstat);
}

/* ip4_mrt_pimstat */
struct pimstat *ip4_mrt_pimstat (void)
{
  return (&pimstat);
}

#endif /* LW_CFG_NET_MROUTER */
/*
 * end
 */
