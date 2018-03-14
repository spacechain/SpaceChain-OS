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
 
#include "lwip/opt.h"
#include "lwip/pbuf.h"
#include "lwip/inet.h"
#include "lwip/raw.h"
#include "lwip/inet_chksum.h"

#include <stddef.h>

#include "aodv_proto.h"
#include "aodv_hello.h"
#include "aodv_neighbor.h"
#include "aodv_route.h"
#include "aodv_seeklist.h"
#include "aodv_mcast.h"
#include "aodv_if.h"

extern ip_addr_t aodv_ipaddr_any;

struct aodv_state aodv_host_state; /* initialize with zero */
static struct udp_pcb *aodv_udp; /* initialize with NULL */

#if AODV_MCAST && LWIP_RAW
static struct raw_pcb *aodv_raw_igmp; /* igmp raw */
static struct raw_pcb *aodv_raw_mproxy; /* multicast proxy */
#endif /* AODV_MCAST && LWIP_RAW */

extern int aodv_wait_on_reboot_get(void);

#define ADOV_GET_TYPE(p)    (*(u8_t *)(p)->payload)

/**
 * aodv udp pcb recv callback
 * 
 * @param arg callback argument.
 * @param pcb udp pcb
 * @param p recv pbuf list
 * @param addr remote address
 * @param port remote port
 */
static void aodv_udp_recv (void *arg, struct udp_pcb *pcb, struct pbuf *p,
                           const ip_addr_t *addr, u16_t port)
{
#if AODV_DEBUG
  char buffer[INET_ADDRSTRLEN];
#endif

  struct in_addr ip_src, ip_dest;
  
  const struct ip_hdr *iphdr;
  ip4_addr_t *current_ip_dest;
  struct netif *inp;
  u8_t ttl;
  
  
  if (p == NULL) {
    return;
  }
  
  LWIP_ERROR("pcb != NULL", (pcb != NULL), goto __out;);
  LWIP_ERROR("addr != NULL", (addr != NULL), goto __out;);
  
  iphdr = ip4_current_header();
  inp = ip_current_netif();
  
  ttl = IPH_TTL(iphdr);
  
  current_ip_dest = ip4_current_dest_addr();
  ip_src.s_addr = ip_2_ip4(addr)->addr; /* source ip */
  ip_dest.s_addr = current_ip_dest->addr; /* destination ip */
  
  if ((ADOV_GET_TYPE(p) == AODV_RREP) && (ttl == 1) && 
      (ip_dest.s_addr == INADDR_BROADCAST)) {
    
    /* hello packet */
    LWIP_ERROR("p->len >= RREP_SIZE", (p->len >= RREP_SIZE), goto __out;);
    aodv_hello_process(p, inp);
    pbuf_free(p); /* free recv pbuf */
    return;
  }
  
  /* Make sure we add/update neighbors */
  aodv_neighbor_add(&ip_src, inp);
  
  switch (ADOV_GET_TYPE(p)) {
  
  case AODV_RREQ:
    LWIP_ERROR("p->len >= RREQ_SIZE", (p->len >= RREQ_SIZE), goto __out;);
    aodv_rreq_process(p, &ip_src, &ip_dest, ttl, inp);
    break;
    
  case AODV_RREP:
    LWIP_ERROR("p->len >= RREP_SIZE", (p->len >= RREP_SIZE), goto __out;);
    aodv_rrep_process(p, &ip_src, &ip_dest, ttl, inp);
    break;
    
  case AODV_RERR:
    LWIP_ERROR("p->len >= RERR_SIZE", (p->len >= RERR_SIZE), goto __out;);
    aodv_rerr_process(p, &ip_src, &ip_dest);
    break;
  
  case AODV_RREP_ACK:
    LWIP_ERROR("p->len >= RREP_ACK_SIZE", (p->len >= RREP_ACK_SIZE), goto __out;);
    aodv_rrep_ack_process(p, &ip_src, &ip_dest);
    break;
    
#if AODV_MCAST
  case AODV_MACT:
    LWIP_ERROR("p->len >= MACT_SIZE", (p->len >= MACT_SIZE), goto __out;);
    aodv_mact_process(p);
    break;
    
  case AODV_GRPH:
    LWIP_ERROR("p->len >= GRPH_SIZE", (p->len >= GRPH_SIZE), goto __out;);
    aodv_grph_process(p, &ip_src, ttl, inp);
    break;
#endif /* AODV_MCAST */
    
  default:
    LWIP_DEBUGF(AODV_DEBUG, ("aodv_recv: unknown packet %s!\n", ipaddr_ntoa_r(addr, buffer, INET_ADDRSTRLEN)));
    break;
  }
  
__out:
  pbuf_free(p); /* free recv pbuf */
}

/**
 * create a aodv udp pcb
 *
 * @param aodv_if_index aodv netif index
 */
void aodv_udp_new (int aodv_if_index)
{
  LWIP_ERROR("aodv_if_index < AODV_MAX_NETIF", (aodv_if_index < AODV_MAX_NETIF), return;);
  LWIP_ERROR("aodv_netif[aodv_if_index] != NULL", (aodv_netif[aodv_if_index] != NULL), return;);
  
  /* lwip only support one aodv netif, because can not bind socket to netif
   * so here has only one udp_pcb.
   */
  if (aodv_udp) {
    return;
  }
  
  aodv_udp = udp_new();
  if (aodv_udp) {
    aodv_udp->so_options |= SOF_BROADCAST;
    if (udp_bind(aodv_udp, &aodv_ipaddr_any, AODV_PORT) != ERR_OK) {
      LWIP_DEBUGF(AODV_DEBUG, ("aodv_udp_new: can not bind port %d!\n", AODV_PORT));
      return;
    }
    udp_recv(aodv_udp, aodv_udp_recv, NULL);
  } else {
    LWIP_DEBUGF(AODV_DEBUG, ("aodv_udp_new: can not create udp_pcb!\n"));
  }
}

/**
 * remove a aodv udp pcb
 *
 * @param aodv_if_index aodv netif index
 */
void aodv_udp_remove (int aodv_if_index)
{
  LWIP_ERROR("aodv_if_index < AODV_MAX_NETIF", (aodv_if_index < AODV_MAX_NETIF), return;);

  if (aodv_udp) {
    udp_remove(aodv_udp);
    aodv_udp = NULL;
  }
}

/**
 * send a aodv packet
 * 
 * @param p aodv packet
 * @param dest_addr destination address
 * @param ttl ip packet time to live
 * @param aodv_if_index aodv netif index
 * @return send packet is ok or not.
 */
err_t aodv_udp_sendto (struct pbuf *p, struct in_addr *dest_addr, u8_t ttl, int aodv_if_index)
{
  ip_addr_t  addr;
  struct netif *netif;
  
  LWIP_ERROR("p != NULL", (p != NULL), return ERR_BUF;);
  LWIP_ERROR("dest_addr != NULL", (dest_addr != NULL), return ERR_ARG;);
  LWIP_ERROR("aodv_if_index < AODV_MAX_NETIF", (aodv_if_index < AODV_MAX_NETIF), return ERR_ARG;);
  
  if (aodv_wait_on_reboot_get() && (ADOV_GET_TYPE(p) == AODV_RREP)) { /* must pass wait reboot time */
    return ERR_RTE;
  }
  
  IP_SET_TYPE_VAL(addr, IPADDR_TYPE_V4);
  ip_2_ip4(&addr)->addr = dest_addr->s_addr;

  aodv_udp->ttl = ttl;
  netif = aodv_netif[aodv_if_index];
  
  LWIP_ERROR("netif != NULL", (netif != NULL), return ERR_IF;);
  
  return udp_sendto_if(aodv_udp, p, &addr, AODV_PORT, netif);
}

#if AODV_MCAST

/**
 * aodv multicast proxy pcb recv callback
 * 
 * @param arg callback argument.
 * @param pcb raw pcb
 * @param p recv pbuf list
 * @param addr remote address
 */
static u8_t aodv_mproxy_recv (void *arg, struct raw_pcb *pcb, struct pbuf *p, const ip_addr_t *addr)
{
#if LWIP_RAW
  struct in_addr dest_addr;
  struct ip_hdr *iphdr;
  struct netif *netif = ip_current_netif();
  int i;
  
  if (aodv_gw_get() == 0) { /* only gateway can be multicast proxy */
    return (0);
  }
  
  iphdr = (struct ip_hdr *)p->payload;
#if LWIP_IPV6
  if (IPH_V(iphdr) == 6) {
    return (0);
  }
#endif /* LWIP_IPV6 */

  if (p->tot_len < sizeof(struct ip_hdr)) {
    return (0);
  }
  
  dest_addr.s_addr = ip4_current_dest_addr()->addr;
  
  if ((dest_addr.s_addr & ~(netif_ip4_netmask(netif)->addr)) ==
      (INADDR_BROADCAST & ~(netif_ip4_netmask(netif)->addr))) { /* broadcast */
    return (0);
  }
  
  if (IN_MULTICAST(ntohl(dest_addr.s_addr))) {
    ip4_addr_t  grp_ip, rev_ip;
    
    if (netif != netif_default) { /* remote send multicast into me(gateway) */
      return (0);
    }
    
    for (i = 0; i < AODV_MAX_NETIF; i++) { /* must not a aodv netif */
      if (netif == aodv_netif[i]) {
        return (0);
      }
    }
    
    /* we can do proxy, and send this multicast packet to aodv subnet */
    grp_ip.addr = dest_addr.s_addr;
    rev_ip.addr = ip4_current_src_addr()->addr;
    AODV_MPACKET_OUTPUT(netif, p, &grp_ip, &rev_ip);
  
  } else {
    struct aodv_rtnode *rev_rt = aodv_rt_find(&dest_addr);
    
    if (rev_rt && (rev_rt->state == AODV_VALID)) {
      aodv_rt_update_timeout(rev_rt, ACTIVE_ROUTE_TIMEOUT);
    }
  }
#endif /* LWIP_RAW */
  return (0);
}

/**
 * create aodv multicast proxy pcb
 *
 */
void aodv_mproxy_new (void)
{
#if LWIP_RAW
  /* lwip only support one aodv netif, because can not bind socket to netif
   * so here has only one udp_pcb.
   */
  if (aodv_raw_mproxy) {
    return;
  }
  
  aodv_raw_mproxy = raw_new(IP_PROTO_UDP); /* udp packet */
  if (aodv_raw_mproxy) {
    if (raw_bind(aodv_raw_mproxy, &aodv_ipaddr_any) != ERR_OK) {
      LWIP_DEBUGF(AODV_DEBUG, ("aodv_mproxy_new: can not bind!\n"));
      return;
    }
    raw_recv(aodv_raw_mproxy, aodv_mproxy_recv, NULL);
  } else {
    LWIP_DEBUGF(AODV_DEBUG, ("aodv_mproxy_new: can not create raw_pcb!\n"));
  }
#else
  LWIP_ERROR("MUST #define LWIP_RAW 1", (0), return;);
#endif /* LWIP_RAW */
}

/**
 * remove aodv multicast proxy pcb
 *
 */
void aodv_mproxy_remove (void)
{
#if LWIP_RAW
  if (aodv_raw_mproxy) {
    raw_remove(aodv_raw_mproxy);
    aodv_raw_mproxy = NULL;
  }
#endif /* LWIP_RAW */
}

/**
 * aodv raw pcb recv callback
 * 
 * @param arg callback argument.
 * @param pcb raw pcb
 * @param p recv pbuf list
 * @param addr remote address
 */
static u8_t aodv_igmp_recv (void *arg, struct raw_pcb *pcb, struct pbuf *p, const ip_addr_t *addr)
{
#if LWIP_RAW
  struct in_addr grp_addr;
  u16_t iphdr_hlen;
  struct igmp_msg igmp;
  struct ip_hdr *iphdr;
  
  iphdr = (struct ip_hdr *)p->payload;
#if LWIP_IPV6
  if (IPH_V(iphdr) == 6) {
    return (0);
  }
#endif /* LWIP_IPV6 */

  if (p->tot_len < sizeof(struct ip_hdr)) {
    return (0);
  }

  /* obtain IP header length in number of 32-bit words */
  iphdr_hlen = (u16_t)IPH_HL(iphdr);
  /* calculate IP header length in bytes */
  iphdr_hlen *= 4;
  
  pbuf_copy_partial(p, (void *)&igmp, sizeof(struct igmp_msg), iphdr_hlen);
  
  grp_addr.s_addr = igmp.igmp_group_address.addr;
  
  switch (igmp.igmp_msgtype) {
  
  case IGMP_V1_MEMB_REPORT:
  case IGMP_V2_MEMB_REPORT: /* join to a multicast group */
    aodv_mcast_join(&grp_addr, ip_current_netif());
    break;
    
  case IGMP_LEAVE_GROUP: /* leave from a multicast group */
    aodv_mcast_leave(&grp_addr);
    break;
    
  case IGMP_MEMB_QUERY:
    aodv_mcast_query(ip_current_netif());
    break;
  }
#endif /* LWIP_RAW */
  return (0); /* do not eaten! */
}

/**
 * create a aodv raw pcb
 *
 * @param aodv_if_index aodv netif index
 */
void aodv_igmp_new (int aodv_if_index)
{
#if LWIP_RAW
  LWIP_ERROR("aodv_if_index < AODV_MAX_NETIF", (aodv_if_index < AODV_MAX_NETIF), return;);
  LWIP_ERROR("aodv_netif[aodv_if_index] != NULL", (aodv_netif[aodv_if_index] != NULL), return;);
  
  /* lwip only support one aodv netif, because can not bind socket to netif
   * so here has only one udp_pcb.
   */
  if (aodv_raw_igmp) {
    return;
  }
  
  aodv_raw_igmp = raw_new(IP_PROTO_IGMP); /* igmp packet */
  if (aodv_raw_igmp) {
    if (raw_bind(aodv_raw_igmp, &aodv_ipaddr_any) != ERR_OK) {
      LWIP_DEBUGF(AODV_DEBUG, ("aodv_igmp_new: can not bind!\n"));
      return;
    }
    raw_recv(aodv_raw_igmp, aodv_igmp_recv, NULL);
  } else {
    LWIP_DEBUGF(AODV_DEBUG, ("aodv_igmp_new: can not create raw_pcb!\n"));
  }
#else
  LWIP_ERROR("MUST #define LWIP_RAW 1", (0), return;);
#endif /* LWIP_RAW */
}

/**
 * remove a aodv igmp pcb
 *
 * @param aodv_if_index aodv netif index
 */
void aodv_igmp_remove (int aodv_if_index)
{
#if LWIP_RAW
  LWIP_ERROR("aodv_if_index < AODV_MAX_NETIF", (aodv_if_index < AODV_MAX_NETIF), return;);

  if (aodv_raw_igmp) {
    raw_remove(aodv_raw_igmp);
    aodv_raw_igmp = NULL;
  }
#endif /* LWIP_RAW */
}

/**
 * send a aodv igmp membership packet
 *
 * @param aodv_if_index aodv netif index
 */
void aodv_igmp_sendreply (struct aodv_mrtnode *mrt, struct netif *netif)
{
  struct pbuf *p;
  struct igmp_msg *igmp;
  ip_addr_t dest;

  if (!mrt->next_hop || !(mrt->flags & AODV_MRT_ROUTER)) {
    /* Nobody listening, so don't send */
    return;
  }
  
  p = pbuf_alloc(PBUF_TRANSPORT, sizeof(struct igmp_msg), PBUF_RAM);
  if (p == NULL) {
    LWIP_DEBUGF(AODV_DEBUG, ("aodv_igmp_sendreply: can not allocate igmp packet!\n"));
    return;
  }
  
  igmp = (struct igmp_msg *)p->payload;
  igmp->igmp_msgtype = IGMP_V1_MEMB_REPORT;
  igmp->igmp_maxresp = 0;
  igmp->igmp_group_address.addr = mrt->grp_addr.s_addr;
  
  igmp->igmp_checksum = 0;
  igmp->igmp_checksum = inet_chksum(igmp, sizeof(struct igmp_msg));
  
  IP_SET_TYPE_VAL(dest, IPADDR_TYPE_V4);
  ip_2_ip4(&dest)->addr = mrt->grp_addr.s_addr;
  
  aodv_raw_igmp->ttl = 1;
  ip_2_ip4(&aodv_raw_igmp->local_ip)->addr = mrt->next_hop->addr.s_addr;
  
  raw_sendto(aodv_raw_igmp, p, &dest);
  
  pbuf_free(p);
}

#endif /* AODV_MCAST */

/* end */
