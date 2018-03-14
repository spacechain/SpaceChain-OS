/**
 * @file
 * IGMP protocol definitions
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
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
 * Author: Adam Dunkels <adam@sics.se>
 *
 */
#ifndef LWIP_HDR_PROT_IGMP_H
#define LWIP_HDR_PROT_IGMP_H

#include "lwip/arch.h"
#include "lwip/ip4_addr.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * IGMP constants
 */
#define IGMP_TTL                       1
#define IGMP_MINLEN                    8
#define IGMP_V3_MINLEN                 12   /* SylixOS Add IGMPv3 minlen */
#define ROUTER_ALERT                   0x9404U
#define ROUTER_ALERTLEN                4

/*
 * IGMP message types, including version number.
 */
#define IGMP_MEMB_QUERY                0x11 /* Membership query         */
#define IGMP_V1_MEMB_REPORT            0x12 /* Ver. 1 membership report */
#define IGMP_V2_MEMB_REPORT            0x16 /* Ver. 2 membership report */
#define IGMP_LEAVE_GROUP               0x17 /* Leave-group message      */
#define IGMP_V3_MEMB_REPORT            0x22 /* SylixOS Add Ver. 3 report*/

/* Group  membership states */
#define IGMP_GROUP_NON_MEMBER          0
#define IGMP_GROUP_DELAYING_MEMBER     1
#define IGMP_GROUP_IDLE_MEMBER         2

/**
 * IGMP packet format.
 */
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct igmp_msg {
  PACK_STRUCT_FLD_8(u8_t         igmp_msgtype);
  PACK_STRUCT_FLD_8(u8_t         igmp_maxresp);
  PACK_STRUCT_FIELD(u16_t        igmp_checksum);
  PACK_STRUCT_FLD_S(ip4_addr_p_t igmp_group_address);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/epstruct.h"
#endif

#if LWIP_IGMP_V3 /* RFC 3367 */
/**
 * IGMPv3 query packet format.
 */
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct igmp_v3_query {
  PACK_STRUCT_FLD_8(u8_t         igmp_v3_msgtype);
  PACK_STRUCT_FLD_8(u8_t         igmp_v3_maxresp);
  PACK_STRUCT_FIELD(u16_t        igmp_v3_checksum);
  PACK_STRUCT_FLD_S(ip4_addr_p_t igmp_v3_group_address);
  PACK_STRUCT_FLD_8(u8_t         igmp_v3_sqrv);
  PACK_STRUCT_FLD_8(u8_t         igmp_v3_qqic);
  PACK_STRUCT_FIELD(u16_t        igmp_v3_srccnt);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/epstruct.h"
#endif

#define IGMP_V3_QUERY_HLEN  sizeof(struct igmp_v3_query)

/**
 * IGMPv3 report packet header format.
 */
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct igmp_v3_report {
  PACK_STRUCT_FLD_8(u8_t         igmp_v3_msgtype);
  PACK_STRUCT_FLD_8(u8_t         igmp_v3_reserve1);
  PACK_STRUCT_FIELD(u16_t        igmp_v3_checksum);
  PACK_STRUCT_FIELD(u16_t        igmp_v3_reserve2);
  PACK_STRUCT_FIELD(u16_t        igmp_v3_reccnt);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/epstruct.h"
#endif

#define IGMP_V3_REPORT_HLEN  sizeof(struct igmp_v3_report)

#define IGMP_V3_REC_IS_IN   0x01    /* Type MODE_IS_INCLUDE */
#define IGMP_V3_REC_IS_EX   0x02    /* Type MODE_IS_EXCLUDE */
#define IGMP_V3_REC_TO_IN   0x03    /* Type CHANGE_TO_INCLUDE_MODE */
#define IGMP_V3_REC_TO_EX   0x04    /* Type CHANGE_TO_EXCLUDE_MODE */
#define IGMP_V3_REC_ALLOW   0x05    /* Type ALLOW_NEW_SOURCES */
#define IGMP_V3_REC_BLOCK   0x06    /* Type BLOCK_OLD_SOURCES */

/**
 * IGMPv3 report packet group record format.
 */
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct igmp_v3_record {
  PACK_STRUCT_FLD_8(u8_t         igmp_v3_rc_type);
  PACK_STRUCT_FLD_8(u8_t         igmp_v3_rc_auxlen);
  PACK_STRUCT_FIELD(u16_t        igmp_v3_rc_srccnt);
  PACK_STRUCT_FLD_S(ip4_addr_p_t igmp_v3_rc_group_address);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/epstruct.h"
#endif

#define IGMP_V3_RECORD_LEN(src_cnt)  (sizeof(struct igmp_v3_record) + (src_cnt << 2))
#endif /* LWIP_IGMP_V3 */

#ifdef __cplusplus
}
#endif

#endif /* LWIP_HDR_PROT_IGMP_H */
