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
#ifndef __AODV_PROTO_H
#define __AODV_PROTO_H

#include "lwip/opt.h"
#include "lwip/err.h"
#include "lwip/udp.h"

#include "aodv_param.h"
#include "aodv_timer.h"
#include "aodv_route.h"

/* AODV state */
typedef struct aodv_state {
  u32_t seqno;
  u32_t rreq_id;
} aodv_state_t;

extern struct aodv_state aodv_host_state;

#define AODV_PORT     654
#define AODV_MTUNNEL  656

/* AODV Message types */
#define AODV_RREQ     1
#define AODV_RREP     2
#define AODV_RERR     3
#define AODV_RREP_ACK 4
#define AODV_MACT     5
#define AODV_GRPH     6

void aodv_udp_new(int aodv_if_index);
void aodv_udp_remove(int aodv_if_index);
err_t aodv_udp_sendto(struct pbuf *p, struct in_addr *dest_addr, u8_t ttl, int aodv_if_index);

#if AODV_MCAST
/**
 * IGMP packet format.
 */
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct igmp_msg {
  PACK_STRUCT_FIELD(u8_t           igmp_msgtype);
  PACK_STRUCT_FIELD(u8_t           igmp_maxresp);
  PACK_STRUCT_FIELD(u16_t          igmp_checksum);
  PACK_STRUCT_FIELD(ip4_addr_p_t   igmp_group_address);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/epstruct.h"
#endif

/*
 * IGMP message types, including version number.
 */
#define IGMP_MEMB_QUERY     0x11 /* Membership query         */
#define IGMP_V1_MEMB_REPORT 0x12 /* Ver. 1 membership report */
#define IGMP_V2_MEMB_REPORT 0x16 /* Ver. 2 membership report */
#define IGMP_LEAVE_GROUP    0x17 /* Leave-group message      */

void aodv_igmp_new(int aodv_if_index);
void aodv_igmp_remove(int aodv_if_index);
void aodv_igmp_sendreply(struct aodv_mrtnode *mrt, struct netif *netif);

void aodv_mproxy_new(void); /* multicast gateway proxy */
void aodv_mproxy_remove(void);

#endif /* AODV_MCAST */

/**
 *  An generic AODV extensions header
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     Type      |     Length    |         data...               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                          data...                              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct aodv_ext {
  PACK_STRUCT_FIELD(u8_t type);
  PACK_STRUCT_FIELD(u8_t length);
  /* Type specific data follows here */
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/epstruct.h"
#endif

/* MACROS to access AODV extensions... */
#define AODV_EXT_HDR_SIZE  sizeof(struct aodv_ext)
#define AODV_EXT_DATA(ext) ((char *)((char *)ext + AODV_EXT_HDR_SIZE))
#define AODV_EXT_NEXT(ext) ((struct aodv_ext *)((char *)ext + AODV_EXT_HDR_SIZE + ext->length))
#define AODV_EXT_SIZE(ext) (AODV_EXT_HDR_SIZE + ext->length)

/* AODV Extension types */
#define RREQ_EXT                    1
#define RREP_EXT                    1
#define RREP_HELLO_INTERVAL_EXT     2
#define RREP_HELLO_NEIGHBOR_SET_EXT 3
#define RREP_INET_DEST_EXT          4

/* Multicast Extension types */
#define RREQ_GRP_REBUILD_EXT        5
#define RREQ_GRP_MERGE_EXT          9
#define RREP_GRP_MERGE_EXT          9

/* RREQ leader ext */
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct grp_leader_ext {
  PACK_STRUCT_FIELD(u8_t type);
  PACK_STRUCT_FIELD(u8_t length);
  PACK_STRUCT_FIELD(u32_t grp_leader);
  PACK_STRUCT_FIELD(u32_t prev_hop);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/epstruct.h"
#endif

/* RREQ grp_rebuild_ext */
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct grp_rebuild_ext {
  PACK_STRUCT_FIELD(u8_t type);
  PACK_STRUCT_FIELD(u8_t length);
  PACK_STRUCT_FIELD(u16_t grp_hcnt); /* 16bit (Alignment) */
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/epstruct.h"
#endif

/* RREQ grp_info_ext */
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct grp_info_ext {
  PACK_STRUCT_FIELD(u8_t type);
  PACK_STRUCT_FIELD(u8_t length);
  PACK_STRUCT_FIELD(u32_t grp_hcnt);
  PACK_STRUCT_FIELD(u32_t grp_leader);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/epstruct.h"
#endif

/* RREQ grp_merge_ext */
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct grp_merge_ext {
  PACK_STRUCT_FIELD(u8_t type);
  PACK_STRUCT_FIELD(u8_t length);
  PACK_STRUCT_FIELD(u16_t res);
  PACK_STRUCT_FIELD(u32_t grp_addr);
  PACK_STRUCT_FIELD(u32_t grp_seqno);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/epstruct.h"
#endif

#define GRP_LEADER_EXT        (struct grp_leader_ext)
#define GRP_LEADER_EXT_SIZE   (sizeof(GRP_LEADER_EXT))
#define GLE_TYPE              ( 3 )
                              /* 2 = size of header */
#define GLE_LENGTH            (GRP_LEADER_EXT_SIZE - 2)  

#define GRP_REBUILD_EXT       struct grp_rebuild_ext
#define GRP_REBUILD_EXT_SIZE  ( sizeof(GRP_REBUILD_EXT) )
#define GRE_TYPE              ( 4 )
                              /* 2 = size of header */
#define GRE_LENGTH            (GRP_REBUILD_EXT_SIZE - 2)

#define GRP_INFO_EXT          (struct grp_info_ext)
#define GRP_INFO_EXT_SIZE     (sizeof(GRP_INFO_EXT))
#define GIE_TYPE              ( 5 )
                              /* 2 = size of header */
#define GIE_LENGTH            (GRP_INFO_EXT_SIZE - 2)

/* NOTE: This is a non-standard extension.  The draft is
   unclear about the tree merge process.  We have created
   a new extension for our tree merge.  The type number
   was chosen in an attempt to avoid collisions with any
   future extensions. */
#define GRP_MERGE_EXT         struct grp_merge_ext
#define GRP_MERGE_EXT_SIZE    (sizeof(GRP_MERGE_EXT))
#define GME_TYPE              ( 9 )
                              /* 2 = size of header */
#define GME_LENGTH            (GRP_MERGE_EXT_SIZE - 2)

/**
 *  RREQ (routing request)
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     Type      |J|R|G|D|U|   Reserved          |   Hop Count   |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                            RREQ ID                            |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    Destination IP Address                     |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                  Destination Sequence Number                  |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    Originator IP Address                      |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                  Originator Sequence Number                   |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct aodv_rreq {
  PACK_STRUCT_FIELD(u8_t type);
  PACK_STRUCT_FIELD(u8_t flags);
  PACK_STRUCT_FIELD(u8_t rsvd);
  PACK_STRUCT_FIELD(u8_t hcnt);
  PACK_STRUCT_FIELD(u32_t rreq_id);
  PACK_STRUCT_FIELD(u32_t dest_addr);
  PACK_STRUCT_FIELD(u32_t dest_seqno);
  PACK_STRUCT_FIELD(u32_t orig_addr);
  PACK_STRUCT_FIELD(u32_t orig_seqno);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/epstruct.h"
#endif

/* RREQ flags */
#define RREQ_JOIN       0x80
#define RREQ_REPAIR     0x40
#define RREQ_GRATUITOUS 0x20
#define RREQ_DEST_ONLY  0x10
#define RREQ_UNKWN_SEQ  0x08

/* Multicast special flag, DO NOT add to aodv_rreq.flags */
#define RREQ_GRP_REBUILD  0x04

#define RREQ_SIZE       sizeof(struct aodv_rreq)

/**
 * A data structure to buffer information about received RREQ's
 */
typedef struct aodv_rreq_record {
  struct aodv_rreq_record *next;
  struct aodv_rreq_record *prev;
  
  struct in_addr orig_addr;    /* Source of the RREQ */
  u32_t rreq_id; /* RREQ's broadcast ID */
  struct aodv_timer rec_timer;
} aodv_rreq_record_t;

/**
 * blacklist
 */
typedef struct aodv_blacklist {
  struct aodv_blacklist *next;
  struct aodv_blacklist *prev;
  
  struct in_addr dest_addr;
  struct aodv_timer bl_timer;
} aodv_blacklist_t;

struct pbuf *aodv_rreq_create(u8_t flags, struct in_addr *dest_addr,
                              u32_t dest_seqno, struct in_addr *orig_addr,
                              u8_t grp_hcnt, struct in_addr *grp_addr);
void aodv_rreq_send(struct in_addr *dest_addr, u32_t dest_seqno, u8_t ttl, u8_t flags);
void aodv_rreq_send_ext(struct in_addr *dest_addr, u32_t dest_seqno, u8_t ttl, u8_t flags, 
                        u8_t grp_hcnt, struct in_addr *grp_addr);
void aodv_rreq_forward(struct pbuf *p, u8_t ttl);
void aodv_rreq_process(struct pbuf *p, 
                       struct in_addr *ip_src,
                       struct in_addr *ip_dst, 
                       u8_t ttl,
                       struct netif *netif);
void aodv_rreq_route_discovery(struct in_addr *dest_addr, u8_t flags,
                               struct pbuf *p, struct in_addr *grp_addr, struct in_addr *rev_addr);
void aodv_rreq_local_repair(struct aodv_rtnode *rt, struct in_addr *src_addr,
                            struct pbuf *p);
struct aodv_rreq_record *aodv_rreq_record_find(struct in_addr *orig_addr, u32_t rreq_id);
struct aodv_rreq_record *aodv_rreq_record_insert(struct in_addr *orig_addr, u32_t rreq_id);
struct aodv_blacklist *aodv_rreq_blacklist_find(struct in_addr *dest_addr);
void aodv_rreq_blacklist_timeout(void *arg);
struct aodv_blacklist *aodv_rreq_blacklist_insert(struct in_addr *dest_addr);

/**
 *  RREP (routing reply)
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     Type      |R|A|    Reserved     |Prefix Sz|   Hop Count   |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                     Destination IP address                    |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                  Destination Sequence Number                  |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    Originator IP address                      |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                           Lifetime                            |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct aodv_rrep {
  PACK_STRUCT_FIELD(u8_t type);
  PACK_STRUCT_FIELD(u8_t flags);
  PACK_STRUCT_FIELD(u8_t prefix); /* upper 5bits */
  PACK_STRUCT_FIELD(u8_t hcnt);
  PACK_STRUCT_FIELD(u32_t dest_addr);
  PACK_STRUCT_FIELD(u32_t dest_seqno);
  PACK_STRUCT_FIELD(u32_t orig_addr);
  PACK_STRUCT_FIELD(u32_t lifetime);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/epstruct.h"
#endif

/* RREP flags */
#define RREP_REPAIR    0x80
#define RREP_ACK       0x40

/* RREP prefix */
#define RREP_FREFIX(rrep)               ((rrep)->prefix >> 3)
#define RREP_FREFIX_SET(rrep, prefix)   ((rrep)->prefix = (u8_t)((prefix) << 3))

#define RREP_SIZE   sizeof(struct aodv_rrep)

/**
 *  RREP-ACK
    0                   1
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     Type      |   Reserved    |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct aodv_rrep_ack {
  PACK_STRUCT_FIELD(u8_t type);
  PACK_STRUCT_FIELD(u8_t rsvd);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/epstruct.h"
#endif

#define RREP_ACK_SIZE   sizeof(struct aodv_rrep_ack)

struct pbuf *aodv_rrep_create(u8_t flags, u8_t prefix, u8_t hcnt, struct in_addr *dest_addr,
                              u32_t dest_seqno, struct in_addr *orig_addr, u32_t life, u16_t ext_size,
                              struct in_addr *grp_addr);
struct aodv_ext *aodv_rrep_ext_add(struct pbuf *p, u8_t type, u16_t offset, u8_t len, char *data);
struct pbuf *aodv_rrep_ack_create(void);
void aodv_rrep_ack_process(struct pbuf *p, struct in_addr *ip_src, struct in_addr *ip_dest);
void aodv_rrep_send(struct pbuf *p, 
                    struct aodv_rtnode *rev_rt,
                    struct aodv_rtnode *fwd_rt);
void aodv_rrep_forward(struct pbuf *p, 
                       struct aodv_rtnode *rev_rt, 
                       struct aodv_rtnode *fwd_rt, 
                       u8_t ttl);
void aodv_rrep_process(struct pbuf *p, 
                       struct in_addr *ip_src,
                       struct in_addr *ip_dst, 
                       u8_t ttl,
                       struct netif *netif);
                    
/**
 *  RERR (routing error)
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     Type      |N|          Reserved           |   DestCount   |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |            Unreachable Destination IP Address (1)             |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |         Unreachable Destination Sequence Number (1)           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-|
   |  Additional Unreachable Destination IP Addresses (if needed)  |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |Additional Unreachable Destination Sequence Numbers (if needed)|
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct aodv_rerr {
  PACK_STRUCT_FIELD(u8_t type);
  PACK_STRUCT_FIELD(u8_t flags);
  PACK_STRUCT_FIELD(u8_t rsvd);
  PACK_STRUCT_FIELD(u8_t dest_count);
  PACK_STRUCT_FIELD(u32_t dest_addr);
  PACK_STRUCT_FIELD(u32_t dest_seqno);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/epstruct.h"
#endif

/* RERR flags */
#define RERR_NODELETE   0x80

#define RERR_SIZE   sizeof(struct aodv_rerr)

#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct aodv_rerr_udest {
  PACK_STRUCT_FIELD(u32_t dest_addr);
  PACK_STRUCT_FIELD(u32_t dest_seqno);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/epstruct.h"
#endif

#define RERR_UDEST_SIZE     sizeof(struct aodv_rerr_udest)

struct pbuf *aodv_rerr_create(u8_t flags, struct in_addr *dest_addr,
                              u32_t dest_seqno);
void aodv_rerr_add_udest(struct pbuf *p, struct in_addr *udest,
                         u32_t udest_seqno);
void aodv_rerr_process(struct pbuf *p, struct in_addr *ip_src, struct in_addr *ip_dst);

#if AODV_MCAST
/* Multicast AODV packet */
/**
 *  Multicast Activation (MACT) Message Format
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     Type      |J|P|G|U|R|     Reserved        |   Hop Count   |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                  Multicast Group IP address                   |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                       Source IP address                       |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                     Source Sequence Number                    |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct aodv_mact {
  PACK_STRUCT_FIELD(u8_t type);
  PACK_STRUCT_FIELD(u8_t flags);
  PACK_STRUCT_FIELD(u8_t rsvd);
  PACK_STRUCT_FIELD(u8_t hcnt);
  PACK_STRUCT_FIELD(u32_t grp_addr);
  PACK_STRUCT_FIELD(u32_t orig_addr);
  PACK_STRUCT_FIELD(u32_t orig_seqno);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/epstruct.h"
#endif

/* MACT flags */
#define MACT_JOIN       0x80
#define MACT_PRUNE      0x40
#define MACT_GRPLEADR   0x20

/* MACT sylixos extern flags */
#define MACT_REPORT_UP  0x10    /* downstream periodically report to upstream */
#define MACT_REDISCOVER 0x08    /* upstream report to downstream, downstream need Re-Discover the group */

#define MACT_SIZE       sizeof(struct aodv_mact)

void aodv_mact_send(u8_t flags, u8_t hcnt, struct in_addr *grp_addr, struct in_addr *dest_addr, u8_t ttl);
void aodv_mact_mcast(u8_t flags, u8_t hcnt, struct in_addr *grp_addr, u8_t ttl);
void aodv_mact_process(struct pbuf *p);
struct aodv_mrtnexthop *aodv_mact_activate_best_upstream(struct aodv_mrtnode *mrt);
void aodv_mact_make_downstream_leader(struct aodv_mrtnode *mrt);

/**
 *  Group Hello (GRPH) Message Format
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     Type      |U|O|       Reserved            |   Hop Count   |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    Group Leader IP address                    |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                  Multicast Group IP address                   |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |               Multicast Group Sequence Number                 |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct aodv_grph {
  PACK_STRUCT_FIELD(u8_t type);
  PACK_STRUCT_FIELD(u8_t flags);
  PACK_STRUCT_FIELD(u8_t rsvd);
  PACK_STRUCT_FIELD(u8_t hcnt);
  PACK_STRUCT_FIELD(u32_t grp_leader);
  PACK_STRUCT_FIELD(u32_t grp_addr);
  PACK_STRUCT_FIELD(u32_t grp_seqno);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/epstruct.h"
#endif

/* GRPH flags */
#define GRPH_UPDATE     0x80
#define GRPH_OFFMTREE   0x40

#define GRPH_SIZE       sizeof(struct aodv_grph)

struct pbuf *aodv_grph_create(u8_t flags, u8_t hcnt, struct in_addr *grp_leader, 
                              struct in_addr *grp_addr, u32_t grp_seqno);
void aodv_grph_send(u8_t flags, u8_t hcnt, struct in_addr *grp_addr, u32_t grp_seqno);
void aodv_grph_forward(struct pbuf *p, u8_t ttl);
void aodv_grph_forward_ucast(struct aodv_mrtnode *mrt, struct pbuf *p, u8_t ttl);
void aodv_grph_process(struct pbuf *p, struct in_addr *src_addr, u8_t ttl, struct netif *netif);

#endif /* AODV_MCAST */

#endif /* __AODV_PROTO_H */
