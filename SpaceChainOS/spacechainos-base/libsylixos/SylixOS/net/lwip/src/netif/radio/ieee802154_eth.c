/**
 * @file
 * IEEE 802.15.4 interface receive/transmit ethernet format
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

#include "lwip/opt.h"
#include "lwip/pbuf.h"
#include "netif/etharp.h"

#include "lowpan_if.h"
#include "lowpan_compress.h"

#include "string.h"

/* Ethernet address boardcast adress*/
#define is_eth_addr_boardcast(a)    \
        ((a.addr[0] == 0xff) &&     \
         (a.addr[1] == 0xff) &&     \
         (a.addr[2] == 0xff) &&     \
         (a.addr[3] == 0xff) &&     \
         (a.addr[4] == 0xff) &&     \
         (a.addr[5] == 0xff))

/* Ethernet address multicast address */
#define is_eth_addr_mcast(a)        \
        ((a.addr[0]  & 0x01) &&     \
         (a.addr[0] != 0xff))

/**
 * if IEEE 802.15.4 use 8 bytes long address, then the address must like the following:
 * 
 *  xx:xx:xx:ff:fe:xx:xx:xx  -> to ethernet ->  xx:xx:xx:xx:xx:xx
 *
 * if IEEE 802.15.4 use 2 bytes short address:
 *
 *  xx:xx -> to ethernet -> 00:00:00:00:xx:xx
 *
 * short address broadcast:
 *
 *  ff:ff -> to ethernet -> ff:ff:ff:ff:ff:ff
 */

/*
 * ieee802154 short address change to ethernet address
 */
static void short_addr_to_eth_addr (u8_t shortaddr[], u8_t ethaddr[], u8_t swp)
{
  if ((shortaddr[0] == 0xff) && (shortaddr[1] == 0xff)) { /* broadcast */
    ethaddr[2] = 0xff;
    ethaddr[3] = 0xff;
    ethaddr[4] = 0xff;
    ethaddr[5] = 0xff;
  } else {
    ethaddr[2] = 0;
    ethaddr[3] = 0;
    ethaddr[4] = 0;
    ethaddr[5] = 0;
  }
  
  if (swp) {
    ethaddr[0] = shortaddr[1];
    ethaddr[1] = shortaddr[0];
  } else {
    ethaddr[0] = shortaddr[0];
    ethaddr[1] = shortaddr[1];
  }
}

/*
 * ieee802154 long address change to ethernet address
 */
static void long_addr_to_eth_addr (u8_t longaddr[], u8_t ethaddr[], u8_t swp)
{
  if (swp) {
    ethaddr[0] = longaddr[7];
    ethaddr[1] = longaddr[6];
    ethaddr[2] = longaddr[5];
    ethaddr[3] = longaddr[2];
    ethaddr[4] = longaddr[1];
    ethaddr[5] = longaddr[0];
  } else {
    ethaddr[0] = longaddr[0];
    ethaddr[1] = longaddr[1];
    ethaddr[2] = longaddr[2];
    ethaddr[3] = longaddr[5];
    ethaddr[4] = longaddr[6];
    ethaddr[5] = longaddr[7];
  }
}

/*
 * ethernet address change to ieee802154 short address
 */
static void eth_addr_to_short_addr (u8_t shortaddr[], u8_t ethaddr[])
{
  shortaddr[0] = ethaddr[0];
  shortaddr[1] = ethaddr[1];
}

/*
 * ethernet address change to ieee802154 long address
 */
static void eth_addr_to_long_addr (u8_t longaddr[], u8_t ethaddr[])
{
  longaddr[0] = ethaddr[0];
  longaddr[1] = ethaddr[1];
  longaddr[2] = ethaddr[2];
  longaddr[3] = 0xff;
  longaddr[4] = 0xfe;
  longaddr[5] = ethaddr[3];
  longaddr[6] = ethaddr[4];
  longaddr[7] = ethaddr[5];
}

/**
 * According to a pbuf which include a ieee 802.15.4 frame, 
 * Abstract the ethernet frame header.
 * The ip packet in the frame is compressed. So we can analyse the ip type.
 * The ip type can be ipv4_ip¡¢ipv4_arp or ipv6.
 *
 * @param p The ieee 802.15.4 frame buffer.
 * @param ethhdr The convert result of the ethernet header.
 * @param isbc is a boardcast packet ?
 * @return The length of the ieee 802.15.4 frame header before abstracting.
 */
u8_t ieee802154_to_eth (struct pbuf *p, struct eth_hdr *ethhdr, u8_t *isbc)
{
  u8_t *head;
  u8_t type;
  ieee802154_frame_fcf_t fcf;
  
  head = (u8_t *)p->payload;
  
  /* decode the FCF */
  fcf.frame_type        =  head[0] & 7;
  fcf.security_enabled  = (head[0] >> 3) & 1;
  fcf.frame_pending     = (head[0] >> 4) & 1;
  fcf.ack_required      = (head[0] >> 5) & 1;
  fcf.panid_compression = (head[0] >> 6) & 1;

  fcf.dest_addr_mode = (head[1] >> 2) & 3;
  fcf.frame_version  = (head[1] >> 4) & 3;
  fcf.src_addr_mode  = (head[1] >> 6) & 3;
  
  /* The index is point to the dest address, bypass the Sequence number and PAN ID. */
  head += 5;
  
  if (isbc) {
    *isbc = 0;
  }
  
  /* 802.15.4 raw address must swp */
  if (fcf.dest_addr_mode == IEEE802154_LONGADDRMODE) {
    long_addr_to_eth_addr(head, ethhdr->dest.addr, 1);
    head += 8;
  
  } else {
    short_addr_to_eth_addr(head, ethhdr->dest.addr, 1);
    if (isbc && (head[0] == 0xff) && (head[1] == 0xff)) {
      *isbc = 1;
    }
    head += 2;
  }
  
  if (fcf.panid_compression == 0) {
    head += 2;
  }
  
  if (fcf.src_addr_mode == IEEE802154_LONGADDRMODE) {
    long_addr_to_eth_addr(head, ethhdr->src.addr, 1);
    head += 8;
  
  } else {
    short_addr_to_eth_addr(head, ethhdr->src.addr, 1);
    head += 2;
  }

  /* Pad the ethernet type */
  type = *head;
  
  /* If the type is IPv6 */
  if (type & 0xc0) {
    ethhdr->type = PP_HTONS(ETHTYPE_IPV6);
  
  } else {
    if(LOWPAN_DISPATCH_IPV4 == type) { /* If he packet is IPv4*/
      ethhdr->type = PP_HTONS(ETHTYPE_IP);
      
    } else if (LOWPAN_DISPATCH_ARP == type) { /* If the Packet is arp*/
      ethhdr->type = PP_HTONS(ETHTYPE_ARP);
    
    } else {
      ethhdr->type = PP_HTONS(ETHTYPE_IP); /* ? */
    }
  }
  
  return (head - (u8_t *)p->payload);
}

/**
 * Convert a ethernet header to a ieee 802.15.4 header.
 *
 * @param ethhdr The ethernet header which is to converted.
 * @param lowpan_if The point of lowpan network struct.
 * @param buf The convert result of ieee 802.15.4 header.
 * @param buf_len buf size.
 * @param isbc is a boardcast packet ?
 * @return 0 Convert failure,
 *         others return the header length of the ieee 802.15.4 frame.
 */
u8_t ieee802154_from_eth (struct eth_hdr *ethhdr, struct lowpanif *lowpanif, void *buf, u8_t buf_len, u8_t *isbc)
{
  ieee802154_frame_t frame;
  u8_t hdr_len;
  
  frame.fcf.frame_type = IEEE802154_DATAFRAME;
  frame.fcf.security_enabled = 0;
  frame.fcf.frame_pending = 0;
  frame.fcf.reserved = 0;
  
  if (is_eth_addr_mcast(ethhdr->dest)) {
    memset(ethhdr->dest.addr, 0xff, ETHARP_HWADDR_LEN); /* multicast must use boardcast address */
  }
  
  /* If the ethernet address is boradcast or multicast */
  if (is_eth_addr_boardcast(ethhdr->dest)) {
    frame.fcf.ack_required = 0;
    frame.fcf.dest_addr_mode = IEEE802154_SHORTADDRMODE;
    if (isbc) {
      *isbc = 1;
    }
  } else {
    frame.fcf.ack_required = 1; /* need destination ack */
    frame.fcf.dest_addr_mode = IEEE802154_LONGADDRMODE;
    if (isbc) {
      *isbc = 0;
    }
  }
  
  /* Source address alwwys use long adde mode. */
  frame.fcf.src_addr_mode = IEEE802154_LONGADDRMODE;
  
  /* Set the frame version */
  frame.fcf.frame_version = IEEE802154_IEEE802154_2003;
  
  /* Set PANID */
  frame.dest_pid = lowpanif->panid;
  frame.src_pid = lowpanif->panid;
  
  /* Set address (do not need swp, becasue ieee802154_frame_create_hdr will do this) */
  if (frame.fcf.dest_addr_mode == IEEE802154_LONGADDRMODE) {
    eth_addr_to_long_addr(frame.dest_addr, ethhdr->dest.addr);
  
  } else {
    eth_addr_to_short_addr(frame.dest_addr, ethhdr->dest.addr);
  }
  
  if (frame.fcf.src_addr_mode == IEEE802154_LONGADDRMODE) {
    eth_addr_to_long_addr(frame.src_addr, ethhdr->src.addr);
    
  } else {
    eth_addr_to_short_addr(frame.src_addr, ethhdr->src.addr);
  }
  
  /* Not set now */
  frame.payload = NULL;
  frame.payload_len = 0;
  
  hdr_len = ieee802154_frame_create_hdr(&frame, (u8_t *)buf, buf_len);
  
  return hdr_len;
}

/**
 * Get a ieee 802.15.4 frame header length.
 *
 * @param ctl The fcf in the ieee 802.15.4 frame header.
 * @return ieee802154 header length
 */
u8_t ieee802154_hdr_len (u8_t *pfcf)
{
  u8_t len = 3;
  ieee802154_frame_fcf_t fcf;
  
  /* decode the FCF */
  fcf.frame_type        =  pfcf[0] & 7;
  fcf.security_enabled  = (pfcf[0] >> 3) & 1;
  fcf.frame_pending     = (pfcf[0] >> 4) & 1;
  fcf.ack_required      = (pfcf[0] >> 5) & 1;
  fcf.panid_compression = (pfcf[0] >> 6) & 1;

  fcf.dest_addr_mode = (pfcf[1] >> 2) & 3;
  fcf.frame_version  = (pfcf[1] >> 4) & 3;
  fcf.src_addr_mode  = (pfcf[1] >> 6) & 3;
  
  /* If the pan id is compressed, then the length add 2, else add 4. */
  if (fcf.panid_compression == 1) {
    len += 2;
  } else {
    len += 4;
  }
  
  switch (fcf.dest_addr_mode) {
  
  case IEEE802154_SHORTADDRMODE:
    len += 2;
    break;
    
  case IEEE802154_LONGADDRMODE:
    len += 8;
    break;
  }
  
  switch (fcf.src_addr_mode) {
  
  case IEEE802154_SHORTADDRMODE:
    len += 2;
    break;
    
  case IEEE802154_LONGADDRMODE:
    len += 8;
    break;
  }
  
  return len;
}

/**
 * Set sequence number of an ieee 802.15.4 frame.
 *
 * @param p An ieee 802.15.4 frame.
 * @return none
 */
void ieee802154_seqno_set (struct pbuf *p, u16_t seqno)
{
  ((u8_t *)p->payload)[2] = seqno;
}
/* end */
