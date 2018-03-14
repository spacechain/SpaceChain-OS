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

#ifndef __IP6_MRT_H
#define __IP6_MRT_H

#include "lwip/opt.h"
#include "lwip/def.h"

#if LW_CFG_NET_MROUTER > 0 && LWIP_IPV6

#include "time.h"
#include "lwip/ip.h"
#include "lwip/raw.h"
#include "lwip/pbuf.h"
#include "lwip/netif.h"
#include "netinet6/in6.h"
#include "netinet6/pim6.h"
#include "netinet6/ip6_mroute.h"

/* mroute lock */
/* tcpip safety call */
#ifndef MRT6_LOCK
#define MRT6_LOCK() \
        if (lock_tcpip_core) { \
          LOCK_TCPIP_CORE(); \
        }
#define MRT6_UNLOCK() \
        if (lock_tcpip_core) { \
          UNLOCK_TCPIP_CORE(); \
        }
#endif

/* route time */
#ifndef GET_TIME
#define GET_TIME(tv)  lib_gettimeofday(tv, NULL)
#endif

/* The kernel's multicast-interface structure. */
struct mif6 {
  u_char          m6_flags;         /* MIFF_ flags defined above */
  u_int           m6_rate_limit;    /* max rate */
  struct in6_addr m6_lcl_addr;      /* local interface address */
  struct netif   *m6_ifp;           /* pointer to interface */
  u_quad_t        m6_pkt_in;        /* # pkts in on interface */
  u_quad_t        m6_pkt_out;       /* # pkts out on interface */
  u_quad_t        m6_bytes_in;      /* # bytes in on interface */
  u_quad_t        m6_bytes_out;     /* # bytes out on interface */
};

/* The kernel's multicast forwarding cache entry structure */
struct mf6c {
  LW_LIST_LINE         mf6c_list;           /* hash table linkage */
  struct sockaddr_in6  mf6c_origin;         /* IPv6 origin of mcasts */
  struct sockaddr_in6  mf6c_mcastgrp;       /* multicast group associated */
  mifi_t               mf6c_parent;         /* incoming IF */
  struct if_set        mf6c_ifset;          /* set of outgoing IFs */
  u_quad_t             mf6c_pkt_cnt;        /* pkt count for src-grp */
  u_quad_t             mf6c_byte_cnt;       /* byte count for src-grp */
  u_quad_t             mf6c_wrong_if;       /* wrong if for src-grp */
  int                  mf6c_expire;         /* time to clean entry up */
  struct timeval       mf6c_last_assert;    /* last time I sent an assert */
  u_long               mf6c_nstall;         /* how mang (struct rtdetq6) in queue */
  PLW_LIST_RING        mf6c_stall;          /* pkts waiting for route */
};

#define MF6C_INCOMPLETE_PARENT ((mifi_t)-1)

#define MF6CTBLSIZ      256
#if (MF6CTBLSIZ & (MF6CTBLSIZ - 1)) == 0
#define MF6CHASHMOD(h)  ((h) & (MF6CTBLSIZ - 1))
#else
#define MF6CHASHMOD(h)  ((h) % MF6CTBLSIZ)
#endif

/* Argument structure used for pkt info. while upcall is made */
struct rtdetq6 {
  LW_LIST_RING        list;      /* list (struct rtdetq) */
  struct pbuf_custom  p;         /* A copy of the packet */
  struct netif       *ifp;       /* Interface pkt came in on */
};

/* max. no of pkts in upcall Q */
#define MAX_UPQ6    4

/* stat op */
#define MRT6STAT_ADD(name, val) (mrt6stat.name += val)
#define MRT6STAT_INC(name)      (mrt6stat.name += 1)

/* stat op */
#define PIM6STAT_ADD(name, val) (pim6stat.name += val)
#define PIM6STAT_INC(name)      (pim6stat.name += 1)

/* multicast router functions */
u8_t  ip6_mrt_setsockopt(struct raw_pcb *pcb, int optname, const void *optval, socklen_t optlen);
u8_t  ip6_mrt_getsockopt(struct raw_pcb *pcb, int optname, const void *optval, socklen_t *optlen);
u8_t  ip6_mrt_ioctl(int cmd, void *data);
err_t ip6_mrt_forward(struct pbuf *p, const struct ip6_hdr *ip6hdr, struct netif *ifp);
u8_t  ip6_mrt_pim_input(struct pbuf *p, u16_t off, struct netif *inp);
void  ip6_mrt_clean(struct raw_pcb *pcb);
int   ip6_mrt_legal_mif_num(int mifi);
void  ip6_mrt_if_detach(struct netif *ifp);
int   ip6_mrt_is_on(void);
void  ip6_mrt_traversal_mfc(void (*callback)(), void *arg0, void *arg1,
                            void *arg2, void *arg3, void *arg4, void *arg5);

/* multicast router statistics */
struct mrt6stat *ip6_mrt_mrt6stat(void);
struct pim6stat *ip6_mrt_pim6stat(void);

#endif /* LW_CFG_NET_MROUTER && LWIP_IPV6 */
#endif /* __IP6_MRT_H */
/*
 * end
 */
