/**
 * @file
 * The Ad hoc On Demand Distance Vector (AODV) routing protocol 
 * 
 * Verification using sylixos(tm) real-time operating system
 */

/*
 * Copyright (c) 2006-2014 SylixOS Group.
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
 * Author: Han.hui <sylixos@gmail.com>
 *
 */
#ifndef __AODV_ROUTE_H
#define __AODV_ROUTE_H

#include "lwip/inet.h"
#include "lwip/netif.h"

#include "aodv_param.h"
#include "aodv_timer.h"

/* INADDR_ANY */
extern struct in_addr aodv_addr_any;

/* Neighbor struct for active routes in Route Table */
typedef struct aodv_precursor {
    struct aodv_precursor *next;
    struct aodv_precursor *prev;
    struct in_addr neighbor;
} aodv_precursor_t;

#define SEQNO_INC(s) ((s == 0) ? 0 : ((s == 0xFFFFFFFF) ? s = 1 : s++))

/* Route table entries */
typedef struct aodv_rtnode {
  struct aodv_rtnode *next; /* list */
  struct aodv_rtnode *prev;
  
  struct netif *netif; /* net interface */
  
  struct in_addr dest_addr; /* address of the destination */
  struct in_addr next_hop; /* address of the next hop to the destination */
                           /* if AODV_RT_INET_DEST next hop is gateway, this is a fake route node */
  u32_t dest_seqno; /* destination sequence number */
  
  u16_t flags; /* routing flags */
  u8_t state; /* the state of this node entry */
  u8_t hcnt; /* distance (in hops) to the destination */
  
  struct aodv_timer rt_timer; /* rhe timer associated with this entry */
  struct aodv_timer ack_timer; /* RREP_ack timer for this destination */
  struct aodv_timer hello_timer;
  
  u32_t last_hello_time;
  u8_t hello_cnt;
  
  s32_t nprec; /* number of precursors */
  struct aodv_precursor *precursors; /* list of neighbors using the route */
} aodv_rtnode_t;

/* route entry flags */
#define AODV_RT_UNIDIR      0x01
#define AODV_RT_REPAIR      0x02
#define AODV_RT_INV_SEQNO   0x04
#define AODV_RT_INET_DEST   0x08
#define AODV_RT_GATEWAY     0x10

/* route entry states */
#define AODV_INVALID        0
#define AODV_VALID          1

#define AODV_RT_TABLESIZE   32 /* must be a power of 2 */
#define AODV_RT_TABLEMASK   (AODV_RT_TABLESIZE - 1)

/* Route table */
typedef struct aodv_rt {
  unsigned int num_entries;
  unsigned int num_active;
  struct aodv_rtnode *hash_tbl[AODV_RT_TABLESIZE];
} aodv_rt_t;

extern struct aodv_rt aodv_rt_tbl; /* aodv routing table */

#define AODV_RT_NUM_ENTRIES()   (aodv_rt_tbl.num_entries)
#define AODV_RT_NUM_ACTIVE()    (aodv_rt_tbl.num_active)

typedef void (*aodv_rt_hook_handler)(void *rt, void *arg0, void *arg1,
                                     void *arg2, void *arg3, void *arg4);

void aodv_rt_init(void);
int aodv_rt_netif_index_get(struct aodv_rtnode *rt);
struct aodv_rtnode *aodv_rt_insert(struct in_addr *dest_addr, struct in_addr *next,
                                   u8_t hops, u32_t seqno, u32_t life, u8_t state,
                                   u16_t flags, struct netif *netif);
struct aodv_rtnode *aodv_rt_update(struct aodv_rtnode *rt, struct in_addr *next,
                                   u8_t hops, u32_t seqno,
                                   u32_t lifetime, u8_t state,
                                   u16_t flags);
struct aodv_rtnode *aodv_rt_update_timeout(struct aodv_rtnode *rt, u32_t lifetime);
void aodv_rt_update_route_timeouts(struct aodv_rtnode *fwd_rt,
                                   struct aodv_rtnode *rev_rt);
struct aodv_rtnode *aodv_rt_find(struct in_addr *dest_addr);
struct aodv_rtnode *aodv_rt_find_gateway(void);
void aodv_rt_traversal(aodv_rt_hook_handler hook,
                       void *arg0, void *arg1, void *arg2, void *arg3, void *arg4);
int aodv_rt_update_inet_rt(struct aodv_rtnode *gw, u32_t life);
int aodv_rt_invalidate(struct aodv_rtnode *rt);
void aodv_rt_delete(struct aodv_rtnode *rt);
void aodv_rt_delete_netif(struct netif *netif);
void aodv_precursor_add(struct aodv_rtnode *rt, struct in_addr *addr);
void aodv_precursor_remove(struct aodv_rtnode *rt, struct in_addr *addr);
void aodv_precursor_list_destroy(struct aodv_rtnode *rt);

#if AODV_MCAST
struct aodv_mrtnode;

/* next hop */
typedef struct aodv_mrtnexthop {
  struct aodv_mrtnexthop *next;
  struct in_addr addr;
  u32_t grp_seqno; /* Group sequence number */
  u8_t flags;
  u8_t grp_hcnt;
} aodv_mrtnexthop_t;

#define AODV_MRT_NEXTHOP_UP  0x01 /* upstream */
#define AODV_MRT_NEXTHOP_ACT 0x02 /* active */

/* source */
typedef struct aodv_mrtsrc {
  struct aodv_mrtsrc *next;
  struct in_addr src_addr; /* source address */
} aodv_mrtsrc_t;

/* orig */
typedef struct aodv_mrtorig {
  struct aodv_mrtorig *next;
  struct in_addr orig_addr;
  u32_t grp_seqno; /* Group sequence number */
  u8_t grp_hcnt;
  struct aodv_timer cleanup_timer;
  struct aodv_mrtnode *up;
} aodv_mrtorig_t;

/* Multicast route table entries */
typedef struct aodv_mrtnode {
  struct aodv_mrtnode *next; /* list */
  struct aodv_mrtnode *prev;
  
  struct netif *netif; /* net interface */
  
  struct in_addr grp_addr; /* Multicast group IP address */
  struct in_addr grp_leader; /* Group Leader IP Address */
  u32_t grp_seqno; /* Group sequence number */
  
  u16_t flags; /* routing flags */
  u8_t leader_hcnt; /* distance (in hops) to group Leader */
  
  struct aodv_timer mrt_timer;
  struct aodv_timer grph_timer;
  struct aodv_timer prune_timer;
  struct aodv_timer report_up_timer;
  
  u8_t grph_update_cnt;
  u8_t activated_upstream_cnt;
  u16_t activated_downstream_cnt;
  
  struct aodv_mrtnexthop *next_hop;
  struct aodv_mrtorig *origlist;
  
#if !defined(MRT_ADD_MFC) && !defined(DVMRP_ADD_MRT)
  struct aodv_mrtsrc *sources; /* source list */
#endif /* !defined(MRT_ADD_MFC) && !defined(DVMRP_ADD_MRT) */
} aodv_mrtnode_t;

/* Multicast route entry flags */
#define AODV_MRT_LEADER  0x01 /* we are group leader */
#define AODV_MRT_MEMBER  0x02 /* we are group member */
#define AODV_MRT_ROUTER  0x04 /* we are router */
#define AODV_MRT_GATEWAY 0x08 /* we are gateway */
#define AODV_MRT_BROKEN  0x10 /* broken */
#define AODV_MRT_REPAIR  0x20 /* in repair */

#define AODV_MRT_TABLESIZE  32 /* must be a power of 2 */
#define AODV_MRT_TABLEMASK  (AODV_MRT_TABLESIZE - 1)

/* Multicast route table */
typedef struct aodv_mrt {
  unsigned int num_entries;
  struct aodv_mrtnode *hash_tbl[AODV_MRT_TABLESIZE];
} aodv_mrt_t;

extern struct aodv_mrt aodv_mrt_tbl; /* aodv multicast routing table */

#define AODV_MRT_NUM_ENTRIES()   (aodv_mrt_tbl.num_entries)
#define AODV_MRT_NUM_ACTIVE()    (aodv_mrt_tbl.num_active)

void aodv_mrt_init(void);
int aodv_mrt_netif_index_get(struct aodv_mrtnode *mrt);
struct aodv_mrtnode *aodv_mrt_insert(struct in_addr *grp_addr, struct in_addr *grp_leader,
                                     u8_t hops, u32_t grp_seqno, u32_t life, u16_t flags, struct netif *netif);
struct aodv_mrtnode *aodv_mrt_update_timeout(struct aodv_mrtnode *mrt, u32_t lifetime);
struct aodv_mrtnode *aodv_mrt_find(struct in_addr *grp_addr);
void aodv_mrt_delete(struct aodv_mrtnode *mrt);
void aodv_mrt_delete_netif(struct netif *netif);
void aodv_mrt_traversal(aodv_rt_hook_handler hook,
                        void *arg0, void *arg1, void *arg2, void *arg3, void *arg4);
void aodv_mrt_set_leader(struct aodv_mrtnode *mrt, struct in_addr *leader, u8_t leader_hcnt);
void aodv_mrt_set_membership(struct aodv_mrtnode *mrt, u8_t ismember);
void aodv_mrt_set_leader_hcnt(struct aodv_mrtnode *mrt, u8_t leader_hcnt);
void aodv_mrt_become_leader(struct aodv_mrtnode *mrt);
void aodv_mrt_start_routing(struct aodv_mrtnode *mrt);
void aodv_mrt_stop_routing(struct aodv_mrtnode *mrt);
struct aodv_mrtnexthop *aodv_mrt_nexthop_find(struct aodv_mrtnode *mrt,
                                              struct in_addr *addr);
struct aodv_mrtnexthop *aodv_mrt_nexthop_add(struct aodv_mrtnode *mrt, 
                                             struct in_addr *addr, 
                                             u8_t direct, 
                                             u32_t grp_seqno);
void aodv_mrt_nexthop_remove(struct aodv_mrtnode *mrt, struct in_addr *addr);
void aodv_mrt_nexthop_remove_all(struct aodv_mrtnode *mrt);
void aodv_mrt_nexthop_active(struct aodv_mrtnode *mrt, struct in_addr *addr);
void aodv_mrt_nexthop_deactive(struct aodv_mrtnode *mrt, struct in_addr *addr);
struct aodv_mrtnexthop *aodv_mrt_get_activated_upstream(struct aodv_mrtnode *mrt);
struct aodv_mrtnexthop *aodv_mrt_get_best_upstream(struct aodv_mrtnode *mrt);
struct aodv_mrtsrc *aodv_mrt_source_find(struct aodv_mrtnode *mrt, struct in_addr *src);
void aodv_mrt_source_add(struct in_addr *src, struct in_addr *dest_grp);
void aodv_mrt_source_remove_all(struct aodv_mrtnode *mrt);
struct aodv_mrtorig *aodv_mrt_origlist_find(struct aodv_mrtnode *mrt, struct in_addr *orig);
struct aodv_mrtorig *aodv_mrt_origlist_add(struct aodv_mrtnode *mrt, struct in_addr *orig);
void aodv_mrt_origlist_remove(void *arg);
void aodv_mrt_origlist_remove_all(struct aodv_mrtnode *mrt);

#endif /* AODV_MCAST */

#endif /* __AODV_ROUTE_H */
