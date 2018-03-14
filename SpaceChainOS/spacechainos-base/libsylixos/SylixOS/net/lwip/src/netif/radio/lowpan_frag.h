/**
 * @file
 * lowpan fragmentation and reassembly, which is used for wireless ieee 802.15.4.
 *
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
#ifndef __LOWPAN_FRAG_H__
#define __LOWPAN_FRAG_H__

#include "lwip/opt.h"
#include "lwip/pbuf.h"
#include "lwip/inet.h"
#include "lwip/ethip6.h"
#include "netif/etharp.h"

#if LWIP_SUPPORT_CUSTOM_PBUF

#ifdef __cplusplus
extern "C" {
#endif

/* The lowpan reassembly timer interval in milliseconds. */
#define LOWPAN_TMR_INTERVAL     1000
/* The lowpan reassembly exceed time. The unit is seconds. */
#define LOWPAN_REASS_MAXAGE     6

#define LOWPAN_REASS_MAX_PBUFS  8
/**
 * LOWPAN_REASS_DEBUG: Enable debugging in lowpan_frag.c for both frag & reass.
*/
#define LOWPAN_REASS_DEBUG      NETIF_DEBUG

/* Lowpan reassembly helper struct.
 * This is exported because memp needs to know the size.
 */
struct lowpan_reassdata {
  struct lowpan_reassdata *next;
  struct pbuf *p;
  struct eth_hdr ethhdr;
  u16_t datagram_len;
  u16_t datagram_tag;
  u8_t timer;
};

void lowpan_reass_init(void);
void lowpan_reass_tmr(void);
struct pbuf *lowpan_reass(struct pbuf *p, struct eth_hdr *ethhdr);

/** A custom pbuf that holds a reference to another pbuf, which is freed
 * when this custom pbuf is freed. This is used to create a custom PBUF_REF
 * that points into the original pbuf. */
#ifndef __LWIP_PBUF_CUSTOM_REF__
#define __LWIP_PBUF_CUSTOM_REF__
struct pbuf_custom_ref {
  /** 'base class' */
  struct pbuf_custom pc;
  /** pointer to the original pbuf that is referenced */
  struct pbuf *original;
};
#endif /* __LWIP_PBUF_CUSTOM_REF__ */

err_t lowpan_frag(struct lowpanif *lowpanif, struct pbuf *p);

#ifdef __cplusplus
}
#endif

#endif /* LWIP_SUPPORT_CUSTOM_PBUF */

#endif  /* __LOWPAN_FRAG_H__ */
