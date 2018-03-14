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

#include "lwip/opt.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/inet.h"
#include "lwip/udp.h"

#include "lowpan_if.h"
#include "lowpan_compress.h"

#include "string.h"

#if LWIP_IPV6
/**
 * An address context for IPHC address compression
 * each context can have upto 8 bytes
 */
struct lowpan_addr_context {
  struct lowpan_addr_context *next;
  u8_t number;
  u8_t prefix[8];
}lowpan_addr_context_t;

/* TTL uncompression values */
static const u8_t lowpan_ttl_values[] = {0, 1, 64, 255};

/**
 * Uncompression of linklocal:
 * 0 -> 16 bytes from packet
 * 1 -> 2  bytes from prefix - bunch of zeroes and 8 from packet
 * 2 -> 2  bytes from prefix - zeroes + 2 from packet
 * 3 -> 2  bytes from prefix - infer 8 bytes from lladdr
 *
 * NOTE: => the uncompress function does change 0xf to 0x10
 * NOTE: 0x00 => no-autoconfig => unspecified
 */
static const u8_t lowpan_unc_llconf[] = {0x0f, 0x28, 0x22, 0x20};

/*
 * Uncompression of ctx-based:
 * 0 -> 0 bits  from packet [unspecified / reserved]
 * 1 -> 8 bytes from prefix - bunch of zeroes and 8 from packet
 * 2 -> 8 bytes from prefix - zeroes + 2 from packet
 * 3 -> 8 bytes from prefix - infer 8 bytes from lladdr
 */
static const u8_t lowpan_unc_ctxconf[] = {0x00, 0x88, 0x82, 0x80};

/*
 * Uncompression of ctx-base
 * 0 -> 0 bits from packet
 * 1 -> 2 bytes from prefix - bunch of zeroes 5 from packet
 * 2 -> 2 bytes from prefix - zeroes + 3 from packet
 * 3 -> 2 bytes from prefix - infer 1 bytes from lladdr
 */
static const u8_t lowpan_unc_mxconf[] = {0x0f, 0x25, 0x23, 0x21};

/* Link local prefix */
static const u8_t lowpan_llprefix[] = {0xfe, 0x80};

/* Addresses contexts for IPHC. */
static struct lowpan_addr_context *unc_ctx_addr = NULL;

/**
 * Compress the 64 bit address
 *
 * @param The pbuf which is the ieee 802.15.4 frame.
 * @return NONE
 */
static u8_t
lowpan_compress_addr_64 (u8_t **hc06_ptr, u8_t shift,
                         const struct in6_addr *ipaddr,
                         const u8_t *mac_addr)
{
  if (is_addr_mac_addr_based(ipaddr, mac_addr)) {
    return (3 << shift);  /* 0-bits */
  
  } else if (lowpan_is_iid_16_bit_compressable(ipaddr)) {
    /* compress IID to 16 bits xxxx::XXXX */
    MEMCPY(*hc06_ptr, &(((u16_t *)(ipaddr->s6_addr))[7]), 2);
    *hc06_ptr += 2;
    return (2 << shift); /* 16-bits */
  
  } else {
    /* do not compress IID => xxxx::IID */
    MEMCPY(*hc06_ptr, &(((u16_t *)(ipaddr->s6_addr))[4]), 8);
    *hc06_ptr += 8;
    return (1 << shift); /* 64-bits */
  }
}

static void
lowpan_ip_ds6_set_addr_iid (struct in6_addr *ipaddr, u8_t *mac_addr)
{
  MEMCPY(&(((u8_t *)((ipaddr)->s6_addr))[8]), mac_addr, 3);
  (((u8_t *)((ipaddr)->s6_addr))[11]) = 0xff;
  (((u8_t *)((ipaddr)->s6_addr))[12]) = 0xfe;
  MEMCPY(&(((u8_t *)((ipaddr)->s6_addr))[13]), (u8_t *)mac_addr + 3, 3);
  (((u8_t *)((ipaddr)->s6_addr))[8]) ^= 0x02;
}

/*
 * Uncompress addresses based on a prefix and a postfix with zeroes in
 * between. If the postfix is zero in length it will use the link address
 * to configure the IP address (autoconf style).
 * pref_post_count takes a byte where the first nibble specify prefix count
 * and the second postfix count (NOTE: 15/0xf => 16 bytes copy).
 */
static void
lowpan_uncompress_addr (u8_t **hc06_ptr, struct in6_addr *ipaddr,
                        u8_t const *prefix, u8_t pref_post_count, u8_t *mac_addr)
{
  u8_t prefcount = pref_post_count >> 4;
  u8_t postcount = pref_post_count & 0x0f;

  /* full nibble 15 => 16 */
  prefcount = (prefcount == 15 ? 16 : prefcount);
  postcount = (postcount == 15 ? 16 : postcount);

  if (prefcount > 0) {
    MEMCPY(ipaddr, prefix, prefcount);
  }

  if (prefcount + postcount < 16) {
    memset(&(((u8_t *)((ipaddr)->s6_addr))[prefcount]), 0, (16 - (prefcount + postcount)));
  }

  if (postcount > 0) {
    MEMCPY(&(((u8_t *)((ipaddr)->s6_addr))[16 - postcount]), *hc06_ptr, postcount);
    if (postcount == 2 && prefcount < 11) {
      /* 16 bits uncompression => 0000:00ff:fe00:XXXX */
      ((u8_t *)((ipaddr)->s6_addr))[11] = 0xff;
      ((u8_t *)((ipaddr)->s6_addr))[12] = 0xfe;
    }
    *hc06_ptr += postcount;
  
  } else if (prefcount > 0) {
    /* no IID based configuration if no prefix and no data */
    lowpan_ip_ds6_set_addr_iid(ipaddr, mac_addr);
  }
}

/*
 * find the context corresponding to prefix ipaddr
 * 
 * @param ipaddr The ipaddr to find
 *
 */
static struct lowpan_addr_context *
lowpan_context_addr_lookup_by_prefix(u8_t *prefix)
{
  struct lowpan_addr_context *p_ctx;
  
  p_ctx = unc_ctx_addr;
  while(p_ctx != NULL) {
    if(memcmp(p_ctx->prefix, prefix, 8) == 0) {
      return p_ctx;
    }
    p_ctx = p_ctx->next;
  }
  
  return NULL;
}

/*
 * find the context with the given number 
 * 
 * @param number The given number to find
 *
 */
static struct lowpan_addr_context*
lowpan_context_addr_lookup_by_number(u8_t number)
{
  struct lowpan_addr_context *p_ctx;
  
  p_ctx = unc_ctx_addr;
  while(p_ctx != NULL) {
    if(p_ctx->number == number) {
      return p_ctx;
    }
    p_ctx = p_ctx->next;
  }
  
  return NULL;
}

/*
 * Add an Ipv6 prefix to support context base Compress
 * 
 * @param prefix The prefix to support
 * @param number The number of this prefix
 */
void 
lowpan_context_addr_add(u8_t *prefix, u8_t number)
{
  struct lowpan_addr_context *p_ctx;
  
  p_ctx = lowpan_context_addr_lookup_by_prefix(prefix);
  if(p_ctx == NULL) {
    p_ctx = mem_malloc(sizeof(struct lowpan_addr_context));
    if(p_ctx != NULL) {
      p_ctx->number = number;
      SMEMCPY(p_ctx->prefix, prefix, 8);
      p_ctx->next = unc_ctx_addr;
      unc_ctx_addr = p_ctx; 
    }
  }
}

/*
 * Delete an Ipv6 prefix to not support context base Compress
 * 
 * @param prefix The prefix to delete
 *
 */
void 
lowpan_context_addr_delete(u8_t *prefix)
{
  struct lowpan_addr_context *p_ctx;
  struct lowpan_addr_context *p_ctx_prev = NULL;
  
  p_ctx = lowpan_context_addr_lookup_by_prefix(prefix);
  if(p_ctx != NULL) {
    /* If it is the list head */
     if(p_ctx == unc_ctx_addr) {
        unc_ctx_addr = p_ctx->next;
     } else {
       p_ctx_prev = unc_ctx_addr;
       while(p_ctx_prev != NULL) {
         if(p_ctx_prev->next == p_ctx) {
           p_ctx_prev->next = p_ctx->next;
           break;
         }
       }
     }
     mem_free(p_ctx);
  }
}
#endif  /* LWIP_IPV6 */

/**
 * Compress IP/UDP header.
 * This function is called by the lowpan code to create a compressed
 * lowpan packet in the pbuf. The ethernet header and original 
 * IP/UDP header is removed, and the new header and its length 
 * saved in the output param.
 *
 * For ipv6, the compression is according to RFC6282. For LOWPAN_UDP 
 * compression, we either compress both ports or none.
 * General format with the compression is as follows:
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |0|1|1|TF |N|HLI|C|S|SAM|M|D|DAM| SCI   | DCI   | comp. IPv6 ip6hdr|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | compressed IPv6 fields .....                                  |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | LOWPAN_UDP    | non compressed UDP fields ...                 |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | L4 data ...                                                   |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * note: The context number 00 is reserved for the link local prefix.
 * For unicast addresses, if we cannot compress the prefix, we neither
 * compress the IID.
 * @param p The pbuf which is ready for compressing.
 * @param head The new header buffer, must be large enough to save the result.
 * @param len The result header length.
 * @return NULL Convert failure,
 *         p  The compressed packet
 */
struct pbuf *
lowpan_header_create (struct pbuf *p, u8_t *head, u16_t *len)
{
#if LWIP_IPV6
  u8_t tmp, iphc0, iphc1, *hc06_ptr;
  u8_t nexth;
  struct ip6_hdr *ip6hdr;
  struct udp_hdr *udphdr;
  struct lowpan_addr_context *p_ctx;
#endif  /* LWIP_IPV6 */
  struct eth_hdr ethhdr;

  if (p->len <= SIZEOF_ETH_HDR) {
    /* a packet with only an ethernet header (or less) is not valid for us */
    return NULL;
  }
  
  /* First, save the ethernet header */
  ethhdr = *((struct eth_hdr *)p->payload);

  /* If the ethernet dest address is multicast, then use the boradcast address. */
  if (ethhdr.dest.addr[0] & 0x01) {
    memset(ethhdr.dest.addr, 0xFF, 6);
  }

   /* Delete the ethernet header, then start to compress the ipv6 packet.*/
  if (pbuf_header(p, -(s16_t)SIZEOF_ETH_HDR)) {
    LWIP_ASSERT("Can't move over header in packet", 0);
    return NULL;
  }

  /* If the packet is arp or ipv4, just add the dispatch, then return, no need to compress. */
  if ((ethhdr.type == PP_HTONS(ETHTYPE_IP))) {
    *head = LOWPAN_DISPATCH_IPV4;
    *len = 1;
    return p;
  
  } else if ((ethhdr.type == PP_HTONS(ETHTYPE_ARP))) {
    *head = LOWPAN_DISPATCH_ARP;
    *len = 1;
    return p;
  }
  
#if LWIP_IPV6
  /* The ip packet must be a ipv6 packet. Identify the IP header */
  ip6hdr = (struct ip6_hdr *)p->payload;
  if (IP6H_V(ip6hdr) != 6) {
    return NULL;
  }

  hc06_ptr = head + 2;

  /**
   * As we copy some bit-length fields, in the IPHC encoding bytes,
   * we sometimes use |=
   * If the field is 0, and the current bit value in memory is 1,
   * this does not work. We therefore reset the IPHC encoding here
   */
  iphc0 = LOWPAN_DISPATCH_IPHC;
  iphc1 = 0;
  
  /*
   * Address handling needs to be made first since it might
   * cause an extra byte with [ SCI | DCI ]
   *
   */

  /* check if dest context exists (for allocating third byte) */
  /* TODO: fix this so that it remembers the looked up values for
     avoiding two lookups - or set the lookup values immediately */
  if(lowpan_context_addr_lookup_by_prefix((u8_t *)ip6hdr->dest.addr) != NULL ||
     lowpan_context_addr_lookup_by_prefix((u8_t *)ip6hdr->src.addr) != NULL) {
    /* set context flag and increase hc06_ptr */
    iphc1 |= LOWPAN_IPHC_CID;
    head[2] = 0;
    hc06_ptr++;
  }
  /**
   * Traffic class, flow label
   * If flow label is 0, compress it. If traffic class is 0, compress it
   * We have to process both in the same time as the offset of traffic
   * class depends on the presence of version and flow label
   */

  /* hc06 format of TC is ECN | DSCP , original one is DSCP | ECN */
  tmp = IP6H_TC(ip6hdr);
  tmp = ((tmp & 0x03) << 6) | (tmp >> 2);

  if (IP6H_FL(ip6hdr) == 0) {
    /* flow label can be compressed */
    iphc0 |= LOWPAN_IPHC_FL_C;
    if (IP6H_TC(ip6hdr) == 0) {
      /* compress (elide) all */
      iphc0 |= LOWPAN_IPHC_TC_C;
    } else {
      /* compress only the flow label */
      *hc06_ptr = tmp;
      hc06_ptr += 1;
    }
  } else {
    /* Flow label cannot be compressed */
    if (IP6H_TC(ip6hdr) == 0) {
      /* compress only traffic class */
      iphc0 |= LOWPAN_IPHC_TC_C;
      *hc06_ptr++ = (tmp & 0xc0) | ((IP6H_FL(ip6hdr) >> 16) & 0x0F);
      *hc06_ptr++ = ((IP6H_FL(ip6hdr) >> 8) & 0xFF);
      *hc06_ptr++ = ((IP6H_FL(ip6hdr)) & 0xFF);
    
    } else {
      /* compress nothing */
      MEMCPY(hc06_ptr, &ip6hdr, 4);
      /* replace the top byte with new ECN | DSCP format */
      *hc06_ptr = tmp;
      hc06_ptr += 4;
    }
  }

  /* NOTE: payload length is always compressed */

  /* Next Header is compress if UDP */
  if (IP6H_NEXTH(ip6hdr) == IP6_NEXTH_UDP) {
    iphc0 |= LOWPAN_IPHC_NH_C;
  }

  if ((iphc0 & LOWPAN_IPHC_NH_C) == 0) {
    *hc06_ptr = IP6H_NEXTH(ip6hdr);
    hc06_ptr += 1;
  }

  /*
   * Hop limit
   * if 1:   compress, encoding is 01
   * if 64:  compress, encoding is 10
   * if 255: compress, encoding is 11
   * else do not compress
   */
  switch (IP6H_HOPLIM(ip6hdr)) {
  
  case 1:
    iphc0 |= LOWPAN_IPHC_TTL_1;
    break;
  
  case 64:
    iphc0 |= LOWPAN_IPHC_TTL_64;
    break;
  
  case 255:
    iphc0 |= LOWPAN_IPHC_TTL_255;
    break;
  
  default:
    *hc06_ptr = IP6H_HOPLIM(ip6hdr);
    hc06_ptr += 1;
    break;
  }

  /* source address compression */
  if (is_addr_unspecified(&ip6hdr->src)) {
    iphc1 |= LOWPAN_IPHC_SAC;
    iphc1 |= LOWPAN_IPHC_SAM_00;
  } else if((p_ctx = lowpan_context_addr_lookup_by_prefix((u8_t *)ip6hdr->src.addr))
     != NULL) {
    /* elide the prefix - indicate by CID and set context + SAC */
    iphc1 |= LOWPAN_IPHC_CID | LOWPAN_IPHC_SAC;
    head[2] |= p_ctx->number << 4;
    /* compession compare with this nodes address (source) */

    iphc1 |= lowpan_compress_addr_64(&hc06_ptr,
                 LOWPAN_IPHC_SAM_BIT, (struct in6_addr *)&ip6hdr->src, ethhdr.src.addr);
    /* No context found for this address */
  } else if (is_addr_link_local((struct in6_addr *)&ip6hdr->src)) {
    iphc1 |= lowpan_compress_addr_64(&hc06_ptr,
                 LOWPAN_IPHC_SAM_BIT, (struct in6_addr *)&ip6hdr->src, ethhdr.src.addr);
  } else {
    MEMCPY(hc06_ptr, &ip6hdr->src.addr[0], 16);
    hc06_ptr += 16;
  }

  /* destination address compression */
  if (is_addr_mcast((struct in6_addr *)&ip6hdr->dest)) {
    iphc1 |= LOWPAN_IPHC_M;
    if (lowpan_is_mcast_addr_compressable8((struct in6_addr *)&ip6hdr->dest)) {
      iphc1 |= LOWPAN_IPHC_DAM_11;
      /* use last byte */
      *hc06_ptr = ((u8_t *)ip6hdr->dest.addr)[15];
      hc06_ptr += 1;
    } else if (lowpan_is_mcast_addr_compressable32((struct in6_addr *)&ip6hdr->dest)) {
      iphc1 |= LOWPAN_IPHC_DAM_10;
      /* second byte + the last three */
      *hc06_ptr = ((u8_t *)ip6hdr->dest.addr)[1];
      MEMCPY(hc06_ptr + 1, &(((u8_t *)ip6hdr->dest.addr)[13]), 3);
      hc06_ptr += 4;
    } else if (lowpan_is_mcast_addr_compressable48((struct in6_addr *)&ip6hdr->dest)) {
      iphc1 |= LOWPAN_IPHC_DAM_01;
      /* second byte + the last five */
      *hc06_ptr = ((u8_t *)ip6hdr->dest.addr)[1];
      MEMCPY(hc06_ptr + 1, &(((u8_t *)ip6hdr->dest.addr)[11]), 5);
      hc06_ptr += 6;
    } else {
      iphc1 |= LOWPAN_IPHC_DAM_00;
      MEMCPY(hc06_ptr, &(((u8_t *)ip6hdr->dest.addr)[0]), 16);
      hc06_ptr += 16;
    }
  } else {
    /* TODO: context lookup */
    if((p_ctx = lowpan_context_addr_lookup_by_prefix((u8_t *)ip6hdr->dest.addr)) != NULL) {
      /* elide the prefix */
      iphc1 |= LOWPAN_IPHC_DAC;
      head[2] |= p_ctx->number;
      /* compession compare with link adress (destination) */

      iphc1 |= lowpan_compress_addr_64(&hc06_ptr,
                   LOWPAN_IPHC_DAM_BIT, (struct in6_addr *)&ip6hdr->dest, ethhdr.dest.addr);
      /* No context found for this address */
    } else if (is_addr_link_local((struct in6_addr *)&ip6hdr->dest)) {
      iphc1 |= lowpan_compress_addr_64(&hc06_ptr,
                   LOWPAN_IPHC_DAM_BIT, (struct in6_addr *)&ip6hdr->dest, ethhdr.dest.addr);
    } else {
      MEMCPY(hc06_ptr, &(((u8_t *)ip6hdr->dest.addr)[0]), 16);
      hc06_ptr += 16;
    }
  }

  nexth = IP6H_NEXTH(ip6hdr);
  pbuf_header(p, -IP6_HLEN);

  /* UDP header compression */
  if (nexth == IP6_NEXTH_UDP) {
    /* identify the UDP header */
    udphdr = (struct udp_hdr *)p->payload;
    if (((udphdr->src & LOWPAN_NHC_UDP_4BIT_MASK) == LOWPAN_NHC_UDP_4BIT_PORT) &&
        ((udphdr->dest & LOWPAN_NHC_UDP_4BIT_MASK) == LOWPAN_NHC_UDP_4BIT_PORT)) {
      /* UDP header: both ports compression to 4 bits */
      *hc06_ptr = LOWPAN_NHC_UDP_CS_P_11;
      *(hc06_ptr + 1) = /* subtraction is faster */
      (u8_t)((udphdr->dest - LOWPAN_NHC_UDP_4BIT_PORT) +
             ((udphdr->src & LOWPAN_NHC_UDP_4BIT_PORT) << 4));
      hc06_ptr += 2;
    
    } else if ((udphdr->dest & LOWPAN_NHC_UDP_8BIT_MASK) == LOWPAN_NHC_UDP_8BIT_PORT) {
      /* UDP header: remove 8 bits of dest */
      *hc06_ptr = LOWPAN_NHC_UDP_CS_P_01;
      MEMCPY(hc06_ptr + 1, &udphdr->src, 2);
      *(hc06_ptr + 3) = (u8_t)(udphdr->dest - LOWPAN_NHC_UDP_8BIT_PORT);
      hc06_ptr += 4;
    
    } else if ((udphdr->src & LOWPAN_NHC_UDP_8BIT_MASK) == LOWPAN_NHC_UDP_8BIT_PORT) {
      /* UDP header: remove 8 bits of source */
      *hc06_ptr = LOWPAN_NHC_UDP_CS_P_10;
      MEMCPY(hc06_ptr + 1, &udphdr->dest, 2);
      *(hc06_ptr + 3) = (u8_t)(udphdr->src - LOWPAN_NHC_UDP_8BIT_PORT);
      hc06_ptr += 4;
    
    } else {
      /* UDP header: can't compress */
      *hc06_ptr = LOWPAN_NHC_UDP_CS_P_00;
      MEMCPY(hc06_ptr + 1, &udphdr->src, 2);
      MEMCPY(hc06_ptr + 3, &udphdr->dest, 2);
      hc06_ptr += 5;
    }

    /* checksum is always inline */
    MEMCPY(hc06_ptr, &udphdr->chksum, 2);
    hc06_ptr += 2;

    pbuf_header(p, -UDP_HLEN);
  }

  head[0] = iphc0;
  head[1] = iphc1;
    
  *len = (u16_t)(hc06_ptr - head);
    
  return p;
#else   /* LWIP_IPV6 */
  *len = 0;
  return p;
#endif  /* LWIP_IPV6 */
}

/**
 * Uncompress IP/UDP header.
 * This funtion is called by the lowpan code to prase the packet to a uncompress
 * ip packet.
 *
 * @param p The pbuf which is ready for uncompressing, it's not include the mac 
 *          frame header.
 * @param ethhdr The ethernet header of the frame.
 * @return NULL Prase failure,
 *         p  The uncompressed packet, include the link layer header.
 */
struct pbuf *
lowpan_header_prase (struct pbuf *p, struct eth_hdr *ethhdr)
{
  u8_t tmp;
#if LWIP_IPV6
  u8_t iphc0, iphc1, *hc06_ptr;
  u8_t checksum_compressed;  /* Now,the udp checksum compression is not support.   */
  u16_t tmp_len;
  u32_t flags;
  struct ip6_hdr ip6hdr;
  struct udp_hdr udphdr;
  struct lowpan_addr_context *p_ctx;
#endif /* LWIP_IPV6 */
  struct pbuf *pfw;

  tmp = *((u8_t *)p->payload);
  /* If the packet is not compressed, then add the link layer header and return.  */
  if ((LOWPAN_DISPATCH_IPV4  == tmp) || 
      (LOWPAN_DISPATCH_ARP   == tmp) ||
      (LOWPAN_DISPATCH_IPV6  == tmp)) {
    pbuf_header(p, -(s16_t)1);
    if (pbuf_header(p, (s16_t)SIZEOF_ETH_HDR)) {
      pfw = pbuf_alloc(PBUF_LINK, p->tot_len, PBUF_RAM);
      if (NULL == pfw) {
        pbuf_free(p);
        return NULL;
      }
      pbuf_copy(pfw, p);
      pbuf_free(p);
      goto prase_done;
     
    } else {
      MEMCPY(p->payload, ethhdr, SIZEOF_ETH_HDR);
      return p;
    }
  } else {
    if ((tmp & (1 << 6)) == 0) { /* not compress ipv6 */
      LWIP_DEBUGF(NETIF_DEBUG, ("lowpan_header_prase: lowpan dispatch unknown!\n"));
      pbuf_free(p);
      return NULL;
    }
  }

#if LWIP_IPV6
  /* The packet must be a compress ipv6 packet. */
  /* identify the compress data start. */
  hc06_ptr = (u8_t *)p->payload;

  iphc0 = *hc06_ptr++;
  iphc1 = *hc06_ptr++;

  /* another if the CID flag is set */
  if (iphc1 & LOWPAN_IPHC_CID) {
    hc06_ptr++;
  }

  /* Traffic Class and Flow Label */
  switch ((iphc0 & LOWPAN_IPHC_TF) >> 3) {
  /**
   * Traffic Class and FLow Label carried in-line
   * ECN + DSCP + 4-bit Pad + Flow Label (4 bytes)
   */
  case 0: /* 00b */
    MEMCPY(&flags, hc06_ptr, 4);
    tmp = *hc06_ptr;
    tmp = ((tmp >> 6) & 0x03) | (tmp << 2);
    flags = ntohl(flags) & 0x000fffff;
    IP6H_VTCFL_SET(&ip6hdr, 6, tmp, flags);
    hc06_ptr += 4;
    break;
    
  /**
   * Flow Label carried in-line
   * ECN + 2-bit Pad + Flow Label (3 bytes), DSCP is elided
   */
  case 1: /* 01b */
    MEMCPY(&flags, hc06_ptr, 4);
    flags = ntohl(flags) & 0x000fffff;
    IP6H_VTCFL_SET(&ip6hdr, 6, 0, flags);
    hc06_ptr += 3;
    break;
  /**
   * Traffic class carried in-line
   * ECN + DSCP (1 byte), Flow Label is elided
   */
  case 2: /* 10b */
    tmp = *hc06_ptr;
    tmp = ((tmp >> 6) & 0x03) | (tmp << 2);
    IP6H_VTCFL_SET(&ip6hdr, 6, tmp, 0);
    hc06_ptr += 1;
    break;
  
  /* Traffic Class and Flow Label are elided */
  case 3: /* 11b */
    IP6H_VTCFL_SET(&ip6hdr, 6, 0, 0);
    break;
  
  default:
    break;
  }

  /* Next Header */
  if ((iphc0 & LOWPAN_IPHC_NH_C) == 0) {
    IP6H_NEXTH_SET(&ip6hdr, *hc06_ptr);
    hc06_ptr += 1;
  }

  /* Hop Limit */
  if ((iphc0 & 0x03) != LOWPAN_IPHC_TTL_I) {
    IP6H_HOPLIM_SET(&ip6hdr, lowpan_ttl_values[iphc0 & 0x03]);
  
  } else {
    IP6H_HOPLIM_SET(&ip6hdr, *hc06_ptr);
    hc06_ptr += 1;
  }

  /* put the source address compression mode SAM in the tmp var */
  tmp = ((iphc1 & LOWPAN_IPHC_SAM) >> LOWPAN_IPHC_SAM_BIT) & 0x03;
  
  /* context based compression */
  if (iphc1 & LOWPAN_IPHC_SAC) {
    u8_t sci = (iphc1 & LOWPAN_IPHC_CID) ?
      ((u8_t *)p->payload)[2] >> 4 : 0;

    /* Source address - check context != NULL only if SAM bits are != 0*/
    if (tmp != 0) {
      p_ctx = lowpan_context_addr_lookup_by_number(sci);
      if(p_ctx == NULL) {
        pbuf_free(p);
        return NULL;
      }
    }
    /* if tmp == 0 we do not have a context and therefore no prefix */
    lowpan_uncompress_addr(&hc06_ptr, (struct in6_addr *)&(ip6hdr.src),
                    tmp != 0 ? p_ctx->prefix : NULL, lowpan_unc_ctxconf[tmp],
                    ethhdr->src.addr);
  } else {
    /* no compression and link local */
    /* Source address uncompression */
    lowpan_uncompress_addr(&hc06_ptr, (struct in6_addr *)&(ip6hdr.src),
                        lowpan_llprefix, lowpan_unc_llconf[tmp], ethhdr->src.addr);
  }

  /* Destination address */
  /* put the destination address compression mode into tmp */
  tmp = ((iphc1 & LOWPAN_IPHC_DAM_11) >> LOWPAN_IPHC_DAM_BIT) & 0x03;

  /* check for Multicast Compression */
  if (iphc1 & LOWPAN_IPHC_M) {
    if (iphc1 & LOWPAN_IPHC_DAC) {
      /* TODO: implement this */
    } else {
      /* non-context based multicast compression - */
      /* DAM_00: 128 bits  */
      /* DAM_01:  48 bits FFXX::00XX:XXXX:XXXX */
      /* DAM_10:  32 bits FFXX::00XX:XXXX */
      /* DAM_11:   8 bits FF02::00XX */
      u8_t prefix[] = {0xff, 0x02};
      if (tmp > 0 && tmp < 3) {
        *hc06_ptr = prefix[1];
        hc06_ptr++;
      }

      lowpan_uncompress_addr(&hc06_ptr, (struct in6_addr *)&(ip6hdr.dest), prefix,
                             lowpan_unc_mxconf[tmp], NULL);
    }
  } else {
    /* no multicast */
    /* Context based */
    if(iphc1 & LOWPAN_IPHC_DAC) {
      u8_t dci = (iphc1 & LOWPAN_IPHC_CID) ?
	((u8_t *)p->payload)[2] & 0x0f : 0;
      p_ctx = lowpan_context_addr_lookup_by_number(dci);

      /* all valid cases below need the context! */
      if(p_ctx == NULL) {
        pbuf_free(p);
        return NULL;
      }
      lowpan_uncompress_addr(&hc06_ptr, (struct in6_addr *)&(ip6hdr.dest), 
                             p_ctx->prefix, lowpan_unc_llconf[tmp], ethhdr->dest.addr);
    } else {
      lowpan_uncompress_addr(&hc06_ptr, (struct in6_addr *)&(ip6hdr.dest), 
                             lowpan_llprefix, lowpan_unc_llconf[tmp], ethhdr->dest.addr);
    }
  }

  tmp_len = (hc06_ptr - (u8_t *)p->payload);
  pbuf_header(p, -(s16_t)(tmp_len));
  
  /* UDP data uncompression */
  if (iphc0 & LOWPAN_IPHC_NH_C) {
    /* The next header is compressed, NHC is following */
    if ((*hc06_ptr & LOWPAN_NHC_UDP_MASK) == LOWPAN_NHC_UDP_ID) {
      IP6H_NEXTH_SET(&ip6hdr, IP6_NEXTH_UDP);
      /* Now,the udp checksum compression is not support.   */
      checksum_compressed = *hc06_ptr & LOWPAN_NHC_UDP_CHECKSUMC;
      
      switch (*hc06_ptr & LOWPAN_NHC_UDP_CS_P_11) {
      
      case LOWPAN_NHC_UDP_CS_P_00:
        /* 1 byte for NHC, 4 byte for ports, 2 bytes chksum */
        MEMCPY(&(udphdr.src), hc06_ptr + 1, 2);
        MEMCPY(&(udphdr.dest), hc06_ptr + 3, 2);
        hc06_ptr += 5;
        break;

      case LOWPAN_NHC_UDP_CS_P_01:
        /* 1 byte for NHC + source 16bit inline, dest = 0xF0 + 8 bit inline */
        MEMCPY(&(udphdr.src), hc06_ptr + 1, 2);
        udphdr.dest = htons(LOWPAN_NHC_UDP_8BIT_PORT + (*(hc06_ptr + 3)));
        hc06_ptr += 4;
        break;

      case LOWPAN_NHC_UDP_CS_P_10:
        /* 1 byte for NHC + source = 0xF0 + 8bit inline, dest = 16 bit inline*/
        udphdr.src = htons(LOWPAN_NHC_UDP_8BIT_PORT + (*(hc06_ptr + 1)));
        MEMCPY(&(udphdr.dest), hc06_ptr + 2, 2);
        hc06_ptr += 4;
        break;

      case LOWPAN_NHC_UDP_CS_P_11:
        /* 1 byte for NHC, 1 byte for ports */
        udphdr.src = htons(LOWPAN_NHC_UDP_4BIT_PORT + ((*(hc06_ptr + 1)) >> 4));
        udphdr.dest = htons(LOWPAN_NHC_UDP_4BIT_PORT + ((*(hc06_ptr + 1)) & 0x0f));
        hc06_ptr += 2;
        break;

      default:
        break;
      }
        
      /* Now,the udp checksum compression is not support.   */
      if (!checksum_compressed) {   /* has_checksum, default  */
        MEMCPY(&(udphdr.chksum), hc06_ptr, 2);
        hc06_ptr += 2;
      }
         
      tmp_len = (hc06_ptr - (u8_t *)p->payload);
      pbuf_header(p, -(s16_t)(tmp_len));
      tmp_len = UDP_HLEN + p->tot_len;
      IP6H_PLEN_SET(&ip6hdr, tmp_len);
      udphdr.len = PP_HTONS(tmp_len);
      pfw = pbuf_alloc(PBUF_RAW, PBUF_LINK_HLEN + IP6_HLEN + UDP_HLEN + p->tot_len, PBUF_RAM);
      if (NULL == pfw) {
        pbuf_free(p);
        return NULL;
      }
      pbuf_header(pfw, -(s16_t)(PBUF_LINK_HLEN + IP6_HLEN + UDP_HLEN));
      pbuf_copy(pfw, p);
      pbuf_free(p);
      pbuf_header(pfw, UDP_HLEN);
      MEMCPY(pfw->payload, &udphdr, UDP_HLEN);
    } else {
      pbuf_free(p);
      return NULL;
    }
  } else {
    IP6H_PLEN_SET(&ip6hdr, p->tot_len);
    pfw = pbuf_alloc(PBUF_RAW, PBUF_LINK_HLEN + IP6_HLEN + p->tot_len, PBUF_RAM);
    if (NULL == pfw) {
        pbuf_free(p);
        return NULL;
    }
    pbuf_header(pfw, -(s16_t)(PBUF_LINK_HLEN + IP6_HLEN));
    pbuf_copy(pfw, p);
    pbuf_free(p);
  }
  
  pbuf_header(pfw, IP6_HLEN);
  MEMCPY(pfw->payload, &ip6hdr, IP6_HLEN);
  
#endif  /* LWIP_IPV6 */ 
prase_done:
  pbuf_header(pfw, (s16_t)SIZEOF_ETH_HDR);
  MEMCPY(pfw->payload, ethhdr, SIZEOF_ETH_HDR);
  return pfw;
}

/* end */
