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

#ifndef __IP4_MRT_H
#define __IP4_MRT_H

#include "lwip/opt.h"
#include "lwip/def.h"

#if LW_CFG_NET_MROUTER > 0

#include "time.h"
#include "lwip/ip.h"
#include "lwip/raw.h"
#include "lwip/pbuf.h"
#include "lwip/netif.h"
#include "netinet/in.h"
#include "netinet/pim.h"
#include "netinet/ip_mroute.h"

/* mroute lock */
/* tcpip safety call */
#ifndef MRT_LOCK
#define MRT_LOCK() \
        if (lock_tcpip_core) { \
          LOCK_TCPIP_CORE(); \
        }
#define MRT_UNLOCK() \
        if (lock_tcpip_core) { \
          UNLOCK_TCPIP_CORE(); \
        }
#endif

/* route time */
#ifndef GET_TIME
#define GET_TIME(tv)  lib_gettimeofday(tv, NULL)
#endif

/* The kernel's virtual-interface structure. */
struct vif {
  u_char         v_flags;        /* VIFF_ flags defined above */
  u_char         v_threshold;    /* min ttl required to forward on vif*/
  struct in_addr v_lcl_addr;     /* local interface address */
  struct in_addr v_rmt_addr;     /* remote address (tunnels only) */
  struct netif  *v_ifp;          /* pointer to interface */
  u_long         v_pkt_in;       /* # pkts in on interface */
  u_long         v_pkt_out;      /* # pkts out on interface */
  u_long         v_bytes_in;     /* # bytes in on interface */
  u_long         v_bytes_out;    /* # bytes out on interface */
};

/* The kernel's multicast forwarding cache entry structure 
 * (A field for the type of service (mfc_tos) is to be added 
 * at a future point) */
struct mfc {
  LW_LIST_LINE   mfc_list;          /* mfc list (struct mfc) */
  struct in_addr mfc_origin;        /* IP origin of mcasts */
  struct in_addr mfc_mcastgrp;      /* multicast group associated */
  vifi_t         mfc_parent;        /* incoming vif */
  u_char         mfc_ttls[MAXVIFS]; /* forwarding ttls on vifs */
  u_long         mfc_pkt_cnt;       /* pkt count for src-grp */
  u_long         mfc_byte_cnt;      /* byte count for src-grp */
  u_long         mfc_wrong_if;      /* wrong if for src-grp */
  int            mfc_expire;        /* time to clean entry up */
  struct timeval mfc_last_assert;   /* last time I sent an assert */
  u_char         mfc_flags[MAXVIFS];/* the MRT_MFC_FLAGS_* flags */
  struct in_addr mfc_rp;            /* the RP address */
  u_long         mfc_nstall;        /* how mang (struct rtdetq) in queue */
  PLW_LIST_RING  mfc_stall;         /* packet (struct rtdetq) */
};

/* mfc hash table */
#define MFCTBLSIZ      256
#if (MFCTBLSIZ & (MFCTBLSIZ - 1)) == 0
#define MFCHASHMOD(h)  ((h) & (MFCTBLSIZ - 1))
#else
#define MFCHASHMOD(h)  ((h) % MFCTBLSIZ)
#endif

/* Argument structure used for pkt info. while upcall is made */
struct rtdetq {
  LW_LIST_RING        list;      /* list (struct rtdetq) */
  struct pbuf_custom  p;         /* A copy of the packet */
  struct netif       *ifp;       /* Interface pkt came in on */
  vifi_t              xmt_vif;   /* Saved copy of imo_multicast_vif */
};

/* max. no of pkts in upcall Q */
#define MAX_UPQ  4

/* stat op */
#define MRTSTAT_ADD(name, val)  (mrtstat.name += val)
#define MRTSTAT_INC(name)       (mrtstat.name += 1)

/* stat op */
#define PIMSTAT_ADD(name, val)  (pimstat.name += val)
#define PIMSTAT_INC(name)       (pimstat.name += 1)

/* multicast router functions */
u8_t  ip4_mrt_setsockopt(struct raw_pcb *pcb, int optname, const void *optval, socklen_t optlen);
u8_t  ip4_mrt_getsockopt(struct raw_pcb *pcb, int optname, const void *optval, socklen_t *optlen);
u8_t  ip4_mrt_ioctl(int cmd, void *data);
err_t ip4_mrt_forward(struct pbuf *p, const struct ip_hdr *iphdr, struct netif *ifp);
u8_t  ip4_mrt_ipip_input(struct pbuf *p, struct netif *inp);
u8_t  ip4_mrt_pim_input(struct pbuf *p, struct netif *inp);
void  ip4_mrt_clean(struct raw_pcb *pcb);
u32_t ip4_mrt_mcast_src(int vifi);
int   ip4_mrt_legal_vif_num(int vifi);
void  ip4_mrt_if_detach(struct netif *ifp);
int   ip4_mrt_is_on(void);
void  ip4_mrt_traversal_mfc(void (*callback)(), void *arg0, void *arg1,
                            void *arg2, void *arg3, void *arg4, void *arg5);

/* multicast router statistics */
struct mrtstat *ip4_mrt_mrtstat(void);
struct pimstat *ip4_mrt_pimstat(void);

#endif /* LW_CFG_NET_MROUTER */
#endif /* __IP4_MRT_H */
/*
 * end
 */
