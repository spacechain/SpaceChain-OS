/**
 * @file
 * lowpan implementation, which is used for wireless ieee 802.15.4.
 *
 * RFC4944 and RFC6282, the ipv6 head compress format. The document 
 * of RFC6282 updates RFC4944.
 * 
 * NOTE: Add a dispatch field to support ipv4 and arp.
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
 * Author: Ren.Haibo <habbyren@qq.com>
 *
 */
#ifndef __LOWPAN_COMPRESS_H__
#define __LOWPAN_COMPRESS_H__

#include "lwip/opt.h"
#include "lwip/pbuf.h"
#include "lwip/inet.h"
#include "lwip/ethip6.h"
#include "netif/etharp.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LOWPAN_DISPATCH_IPV4        0x01 /* 00000001 = 1 ,this is defined by myself.*/
#define LOWPAN_DISPATCH_ARP         0x02 /* 00000010 = 2 ,this is defined by myself.*/
#define LOWPAN_DISPATCH_IPV6        0x41 /* 01000001 = 65 ,this is defined by rfc4944.*/
#define LOWPAN_DISPATCH_HC1         0x42 /* 01000010 = 66 ,this is defined by rfc4944.*/
#define LOWPAN_DISPATCH_IPHC        0x60 /* 011xxxxx = ... ,this is defined by rfc6282.*/
#define LOWPAN_DISPATCH_FRAG1       0xc0 /* 11000xxx ,this is defined by rfc4944.*/
#define LOWPAN_DISPATCH_FRAGN       0xe0 /* 11100xxx ,this is defined by rfc4944.*/

#define LOWPAN_DISPATCH_MASK        0xf8 /* 11111000 */

#define LOWPAN_FRAG_TIMEOUT         (10)    /* time-out 10 sec */

#define LOWPAN_FRAG1_HEAD_SIZE      0x4
#define LOWPAN_FRAGN_HEAD_SIZE      0x5

/*
 * According IEEE802.15.4 standard:
 *   - MTU is 127 octets
 *   - maximum MHR size is 37 octets
 *   - MFR size is 2 octets
 *
 * so minimal payload size that we may guarantee is:
 *   MTU - MHR - MFR = 88 octets
 */
#define LOWPAN_FRAG_SIZE            88

/*
 * Values of fields within the IPHC encoding first byte
 * (C stands for compressed and I for inline)
 */
#define LOWPAN_IPHC_TF              0x18

#define LOWPAN_IPHC_FL_C            0x10
#define LOWPAN_IPHC_TC_C            0x08
#define LOWPAN_IPHC_NH_C            0x04
#define LOWPAN_IPHC_TTL_1           0x01
#define LOWPAN_IPHC_TTL_64          0x02
#define LOWPAN_IPHC_TTL_255         0x03
#define LOWPAN_IPHC_TTL_I           0x00


/* Values of fields within the IPHC encoding second byte */
#define LOWPAN_IPHC_CID             0x80

#define LOWPAN_IPHC_SAC             0x40
#define LOWPAN_IPHC_SAM_00          0x00
#define LOWPAN_IPHC_SAM_01          0x10
#define LOWPAN_IPHC_SAM_10          0x20
#define LOWPAN_IPHC_SAM             0x30

#define LOWPAN_IPHC_SAM_BIT         4

#define LOWPAN_IPHC_M               0x08
#define LOWPAN_IPHC_DAC             0x04
#define LOWPAN_IPHC_DAM_00          0x00
#define LOWPAN_IPHC_DAM_01          0x01
#define LOWPAN_IPHC_DAM_10          0x02
#define LOWPAN_IPHC_DAM_11          0x03

#define LOWPAN_IPHC_DAM_BIT         0
/*
 * LOWPAN_UDP encoding (works together with IPHC)
 */
#define LOWPAN_NHC_UDP_MASK         0xF8
#define LOWPAN_NHC_UDP_ID           0xF0
#define LOWPAN_NHC_UDP_CHECKSUMC    0x04
#define LOWPAN_NHC_UDP_CHECKSUMI    0x00

#define LOWPAN_NHC_UDP_4BIT_PORT    0xF0B0
#define LOWPAN_NHC_UDP_4BIT_MASK    0xFFF0
#define LOWPAN_NHC_UDP_8BIT_PORT    0xF000
#define LOWPAN_NHC_UDP_8BIT_MASK    0xFF00

/* values for port compression, _with checksum_ ie bit 5 set to 0 */
#define LOWPAN_NHC_UDP_CS_P_00      0xF0    /* all inline */
#define LOWPAN_NHC_UDP_CS_P_01      0xF1    /* source 16bit inline,
                                            dest = 0xF0 + 8 bit inline */
#define LOWPAN_NHC_UDP_CS_P_10      0xF2    /* source = 0xF0 + 8bit inline,
                                            dest = 16 bit inline */
#define LOWPAN_NHC_UDP_CS_P_11      0xF3    /* source & dest = 0xF0B + 4bit inline */

/*
 * ipv6 address based on mac
 * second bit-flip (Universe/Local) is done according RFC2464
 */
#define is_addr_mac_addr_based(a, m)    \
    (((((u8_t *)((a)->s6_addr))[8])  == (((m)[0]) ^ 0x02)) &&   \
     ((((u8_t *)((a)->s6_addr))[9])  == (m)[1]) &&  \
     ((((u8_t *)((a)->s6_addr))[10]) == (m)[2]) &&  \
     ((((u8_t *)((a)->s6_addr))[11]) == 0xff) &&    \
     ((((u8_t *)((a)->s6_addr))[12]) == 0xfe) &&    \
     ((((u8_t *)((a)->s6_addr))[13]) == (m)[3]) &&  \
     ((((u8_t *)((a)->s6_addr))[14]) == (m)[4]) &&  \
     ((((u8_t *)((a)->s6_addr))[15]) == (m)[5]))

/* ipv6 address is unspecified */
#define is_addr_unspecified(a)  \
    ((((a)->addr[0]) == 0) &&   \
     (((a)->addr[1]) == 0) &&   \
     (((a)->addr[2]) == 0) &&   \
     (((a)->addr[3]) == 0))

/* compare ipv6 addresses prefixes */
#define ipaddr_prefixcmp(addr1, addr2, length) \
    (memcmp(addr1, addr2, length >> 3) == 0)

/* local link, i.e. FE80::/10 */
#define is_addr_link_local(a) ((((u16_t *)((a)->s6_addr))[0]) == htons(0xFE80))

/*
 * check whether we can compress the IID to 16 bits,
 * it's possible for unicast adresses with first 49 bits are zero only.
 */
#define lowpan_is_iid_16_bit_compressable(a)    \
    (((((u16_t *)((a)->s6_addr))[4]) == 0) &&   \
     ((((u8_t *)((a)->s6_addr))[10]) == 0) &&   \
     ((((u8_t *)((a)->s6_addr))[11]) == 0xff) &&    \
     ((((u8_t *)((a)->s6_addr))[12]) == 0xfe) &&    \
     ((((u8_t *)((a)->s6_addr))[13]) == 0))

/* multicast address */
#define is_addr_mcast(a) ((((u8_t *)((a)->s6_addr))[0]) == 0xFF)

/* check whether the 112-bit gid of the multicast address is mappable to: */

/* 9 bits, for FF02::1 (all nodes) and FF02::2 (all routers) addresses only. */
#define lowpan_is_mcast_addr_compressable(a)    \
    (((((u16_t *)((a)->s6_addr))[1]) == 0) &&   \
     ((((u16_t *)((a)->s6_addr))[2]) == 0) &&   \
     ((((u16_t *)((a)->s6_addr))[3]) == 0) &&   \
     ((((u16_t *)((a)->s6_addr))[4]) == 0) &&   \
     ((((u16_t *)((a)->s6_addr))[5]) == 0) &&   \
     ((((u16_t *)((a)->s6_addr))[6]) == 0) &&   \
     ((((u8_t *)((a)->s6_addr))[14])  == 0) &&  \
     (((((u8_t *)((a)->s6_addr))[15]) == 1) || ((((u8_t *)((a)->s6_addr))[15]) == 2)))

/* 48 bits, FFXX::00XX:XXXX:XXXX */
#define lowpan_is_mcast_addr_compressable48(a)  \
    (((((u16_t *)((a)->s6_addr))[1]) == 0) &&   \
     ((((u16_t *)((a)->s6_addr))[2]) == 0) &&   \
     ((((u16_t *)((a)->s6_addr))[3]) == 0) &&   \
     ((((u16_t *)((a)->s6_addr))[4]) == 0) &&   \
     ((((u8_t *)((a)->s6_addr))[10]) == 0))

/* 32 bits, FFXX::00XX:XXXX */
#define lowpan_is_mcast_addr_compressable32(a)  \
    (((((u16_t *)((a)->s6_addr))[1]) == 0) &&   \
     ((((u16_t *)((a)->s6_addr))[2]) == 0) &&   \
     ((((u16_t *)((a)->s6_addr))[3]) == 0) &&   \
     ((((u16_t *)((a)->s6_addr))[4]) == 0) &&   \
     ((((u16_t *)((a)->s6_addr))[5]) == 0) &&   \
     ((((u8_t *)((a)->s6_addr))[12]) == 0))

/* 8 bits, FF02::00XX */
#define lowpan_is_mcast_addr_compressable8(a)   \
    (((((u8_t *)((a)->s6_addr))[1])  == 2) &&   \
     ((((u16_t *)((a)->s6_addr))[1]) == 0) &&   \
     ((((u16_t *)((a)->s6_addr))[2]) == 0) &&   \
     ((((u16_t *)((a)->s6_addr))[3]) == 0) &&   \
     ((((u16_t *)((a)->s6_addr))[4]) == 0) &&   \
     ((((u16_t *)((a)->s6_addr))[5]) == 0) &&   \
     ((((u16_t *)((a)->s6_addr))[6]) == 0) &&   \
     ((((u8_t *)((a)->s6_addr))[14]) == 0))

#if LWIP_IPV6
void lowpan_context_addr_add(u8_t *prefix, u8_t number);
void lowpan_context_addr_delete(u8_t *prefix);
#endif  /* LWIP_IPV6 */ 
struct pbuf * lowpan_header_create(struct pbuf *p, u8_t *head, u16_t *len);
struct pbuf * lowpan_header_prase(struct pbuf *p, struct eth_hdr *ethhdr);

#ifdef __cplusplus
}
#endif

#endif  /* __LOWPAN_COMPRESS_H__ */
