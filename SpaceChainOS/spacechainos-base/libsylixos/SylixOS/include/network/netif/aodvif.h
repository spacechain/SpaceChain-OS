/**
 * @file
 * The Ad hoc On Demand Distance Vector (AODV) routing protocol 
 *
 * Verification using sylixos(tm) real-time operating system
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
 
/* 
 * This is an arch independent AODV netif.
 */
#ifndef __AODVIF_H
#define __AODVIF_H

#include "lwip/pbuf.h"
#include "lwip/netif.h"
#include "netif/etharp.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * NOTIC: 
 * If you use aodv in SylixOS real-time operating system, 
 * aodv netif not must have to be a defaut netif, if in case, 
 * you must use route_add() api add a 255.255.255.255 (broadcast host) route,
 * so udp broadcast packet can transmit on aodv netif.
 *
 * If you have more than two netif: 
 * that nodes in a aodv subnet required access to the external networks, this node can be a gateway.
 * first add a external netif like eth-net interface, and set it to default netif.
 * then add aodv netif, and use route_add() api add a 255.255.255.255 (broadcast host) route to aodv netif
 * then call aodv_gw_set(1) to be a aodv gateway.
 *
 * If you use aodv NOT in SylixOS, you MUST use netif_set_default() set aodv netif as a default netif.
 * so udp broadcast packet can transmit on aodv netif. aodv can NOT to be getway mode!
 * 
 */

/* aodv netif api */
struct netif *aodv_netif_add(struct netif *netif, ip4_addr_t *ipaddr, ip4_addr_t *netmask,
                             ip4_addr_t *gw, void *state, netif_init_fn init, netif_input_fn input);
void aodv_netif_remove(struct netif *netif);

/* aodv netif output function */
err_t aodv_netif_output(struct netif *netif, struct pbuf *q, ip4_addr_t *ipaddr);

#if LWIP_IPV6
err_t aodv_netif_output6(struct netif *netif, struct pbuf *q, ip6_addr_t *ip6addr);
#endif

/* expand setting */
int aodv_expanding_ring_search_get(void);
void aodv_expanding_ring_search_set(int enable);
void aodv_gw_set(int enable);
int aodv_gw_get(void);

#ifdef __cplusplus
}
#endif

#endif /* __AODVIF_H */
