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
#ifndef LWIP_HDR_MCAST_H
#define LWIP_HDR_MCAST_H

#include "lwip/opt.h"

#if LWIP_UDP || LWIP_RAW /* don't build if not configured for use in lwipopts.h */

#include "lwip/def.h"
#include "lwip/ip.h"
#include "lwip/ip_addr.h"
#include "lwip/ip6_addr.h"
#include "lwip/netif.h"
#include "lwip/igmp.h"
#include "lwip/mld6.h"
#include "lwip/sockets.h"
#include "lwip/prot/ip4.h"
#include "lwip/prot/ip6.h"

#ifndef LWIP_MCAST_SRC_TBL_SIZE
#define LWIP_MCAST_SRC_TBL_SIZE  32
#endif /* !LWIP_MCAST_TABLE_SIZE */

#if LWIP_IPV4 && LWIP_IGMP
/** the IPv4 multicast filter */
struct ip4_mc {
  struct ip4_mc *next;
  /** the interface address */
  ip4_addr_t if_addr;
  /** the group address */
  ip4_addr_t multi_addr;
  /** the source address list filter mode 0: EXCLUDE 1: INCLUDE */
  u8_t fmode;
  /** the source address list */
  struct igmp_src *src;
};

/** The list of ip4_mc. */
#define IP4_MC_FOREACH(ipmc, mc) \
  for ((mc) = (ipmc)->mc4; (mc) != NULL; (mc) = (mc)->next)
#define IP4_MC_SRC_FOREACH(mc, src) \
  for ((src) = (mc)->src; (src) != NULL; (src) = (src)->next)
#endif /* LWIP_IPV4 && LWIP_IGMP */

#if LWIP_IPV6 && LWIP_IPV6_MLD
/** the IPv6 multicast filter */
struct ip6_mc {
  struct ip6_mc *next;
  /** the interface index */
  u8_t if_idx;
  /** the group address */
  ip6_addr_t multi_addr;
  /** the source address list filter mode 0: EXCLUDE 1: INCLUDE */
  u8_t fmode;
  /** the source address list */
  struct mld6_src *src;
};

/** The list of ip6_mc. */
#define IP6_MC_FOREACH(ipmc, mc) \
  for ((mc) = (ipmc)->mc6; (mc) != NULL; (mc) = (mc)->next)
#define IP6_MC_SRC_FOREACH(mc, src) \
  for ((src) = (mc)->src; (src) != NULL; (src) = (src)->next)
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */

#if (LWIP_IPV4 && LWIP_IGMP) || (LWIP_IPV6 && LWIP_IPV6_MLD)
/* multicast filter control block */
struct ip_mc {
#if !LWIP_SOCKET
#define IPPROTO_UDP 17
#define IPPROTO_RAW 255
#endif /* !LWIP_SOCKET */
  u8_t proto; /* IPPROTO_UDP or IPPROTO_RAW */

#if LWIP_IPV4 && LWIP_IGMP
  struct ip4_mc *mc4;
#endif /* LWIP_IPV4 && LWIP_IGMP */

#if LWIP_IPV6 && LWIP_IPV6_MLD
  struct ip6_mc *mc6;
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */
};

#if LWIP_IPV4 && LWIP_IGMP && LWIP_IGMP_V3
/* IGMPv3 must install a trigger function to report multicast filter changes */
typedef void (*mcast_trigger_ip4_fn)(struct netif *netif, const ip4_addr_t *multi_addr);
void    mcast_ip4_filter_trigger_callback(mcast_trigger_ip4_fn trigger_callback);
/* IGMPv3 use the following function to get specified group of total multicast filter source address array */
u16_t   mcast_ip4_filter_info(struct netif *netif, const ip4_addr_t *multi_addr, ip4_addr_p_t addr_array[], u16_t arr_cnt, u8_t *fmode);
u8_t    mcast_ip4_filter_interest(struct netif *netif, const ip4_addr_t *multi_addr, const ip4_addr_p_t src_addr[], u16_t arr_cnt);
#endif /* LWIP_IPV4 && LWIP_IGMP && LWIP_IGMP_V3 */

#if LWIP_IPV6 && LWIP_IPV6_MLD && LWIP_IPV6_MLD_V2
/* MLDv2 must install a trigger function to report multicast filter changes */
typedef void (*mcast_trigger_ip6_fn)(struct netif *netif, const ip6_addr_t *multi_addr);
void    mcast_ip6_filter_trigger_callback(mcast_trigger_ip6_fn trigger_callback);
/* MLDv2 use the following function to get specified group of total multicast filter source address array */
u16_t   mcast_ip6_filter_info(struct netif *netif, const ip6_addr_t *multi_addr, ip6_addr_p_t addr_array[], u16_t arr_cnt, u8_t *fmode);
u8_t    mcast_ip6_filter_interest(struct netif *netif, const ip6_addr_t *multi_addr, const ip6_addr_p_t src_addr[], u16_t arr_cnt);
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD && LWIP_IPV6_MLD_V2 */

/* UDP or RAW use the following functions */
void    mcast_pcb_remove(struct ip_mc *ipmc);
u8_t    mcast_input_local_match(struct ip_mc *ipmc, struct netif *inp);

/* The following functions is for Non-Socket API user */
err_t   mcast_join_netif(struct ip_mc *ipmc, struct netif *netif, const ip_addr_t *multi_addr, const ip_addr_t *src_addr);
err_t   mcast_join_group(struct ip_mc *ipmc, const ip_addr_t *if_addr, const ip_addr_t *multi_addr, const ip_addr_t *src_addr);
err_t   mcast_leave_netif(struct ip_mc *ipmc, struct netif *netif, const ip_addr_t *multi_addr, const ip_addr_t *src_addr);
err_t   mcast_leave_group(struct ip_mc *ipmc, const ip_addr_t *if_addr, const ip_addr_t *multi_addr, const ip_addr_t *src_addr);
err_t   mcast_block_netif(struct ip_mc *ipmc, struct netif *netif, const ip_addr_t *multi_addr, const ip_addr_t *blk_addr);
err_t   mcast_block_group(struct ip_mc *ipmc, const ip_addr_t *if_addr, const ip_addr_t *multi_addr, const ip_addr_t *blk_addr);
err_t   mcast_unblock_netif(struct ip_mc *ipmc, struct netif *netif, const ip_addr_t *multi_addr, const ip_addr_t *unblk_addr);
err_t   mcast_unblock_group(struct ip_mc *ipmc, const ip_addr_t *if_addr, const ip_addr_t *multi_addr, const ip_addr_t *unblk_addr);

/* The following functions is for socket API use */
#if LWIP_SOCKET
err_t   mcast_set_msfilter_netif(struct ip_mc *ipmc, struct netif *netif, const ip_addr_t *multi_addr, const struct ip_msfilter *imsf);
err_t   mcast_set_groupfilter_netif(struct ip_mc *ipmc, struct netif *netif, const ip_addr_t *multi_addr, const struct group_filter *gf);
err_t   mcast_get_msfilter_netif(struct ip_mc *ipmc, struct netif *netif, const ip_addr_t *multi_addr, struct ip_msfilter *imsf, socklen_t *bufsz);
err_t   mcast_get_groupfilter_netif(struct ip_mc *ipmc, struct netif *netif, const ip_addr_t *multi_addr, struct group_filter *gf, socklen_t *bufsz);

int     mcast_sock_add_drop_membership(struct ip_mc *ipmc, int optname, const struct ip_mreq *imr);
int     mcast_sock_add_drop_source_membership(struct ip_mc *ipmc, int optname, const struct ip_mreq_source *imr);
int     mcast_sock_block_unblock_source(struct ip_mc *ipmc, int optname, const struct ip_mreq_source *imr);
int     mcast_sock_set_msfilter(struct ip_mc *ipmc, int optname, const struct ip_msfilter *imsf);
int     mcast_sock_get_msfilter(struct ip_mc *ipmc, int optname, struct ip_msfilter *imsf, socklen_t *size);
int     mcast_sock_join_leave_group(struct ip_mc *ipmc, int optname, const struct group_req *gr);
int     mcast_sock_join_leave_source_group(struct ip_mc *ipmc, int optname, const struct group_source_req *gsr);
int     mcast_sock_block_unblock_source_group(struct ip_mc *ipmc, int optname, const struct group_source_req *gsr);
int     mcast_sock_set_groupfilter(struct ip_mc *ipmc, int optname, const struct group_filter *gf);
int     mcast_sock_get_groupfilter(struct ip_mc *ipmc, int optname, struct group_filter *gf, socklen_t *size);
int     mcast_sock_ipv6_add_drop_membership(struct ip_mc *ipmc, int optname, const struct ipv6_mreq *imr);
#endif /* LWIP_SOCKET */

#endif /* (LWIP_IPV4 && LWIP_IGMP) || (LWIP_IPV6 && LWIP_IPV6_MLD) */

#endif /* LWIP_UDP || LWIP_RAW */

#endif /* LWIP_HDR_MCAST_H */
