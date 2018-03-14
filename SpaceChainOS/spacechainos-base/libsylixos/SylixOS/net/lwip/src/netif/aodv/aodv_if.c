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
 
/* 
 * This is an arch independent AODV netif.
 */
#ifdef SYLIXOS
#define  __SYLIXOS_KERNEL
#endif

#include "lwip/opt.h"
#include "lwip/pbuf.h"
#include "lwip/netif.h"
#include "lwip/inet.h"

#include "aodv_proto.h"
#include "aodv_param.h"
#include "aodv_route.h"
#include "aodv_hello.h"
#include "aodv_timer.h"
#include "aodv_timercb.h"
#include "aodv_mcast.h"
#include "aodv_mtunnel.h"
#include "aodv_if.h"

ip_addr_t aodv_ipaddr_any; /* global variable auto clear */
struct netif *aodv_netif[AODV_MAX_NETIF]; /* initialize with NULL */

static int aodv_wait_on_reboot = 1; /* must wait reboot */
static struct aodv_timer wait_on_reboot_timer;
static int expanding_ring_search = 1; /* default enable expanding ring search */
static int aodv_gw = 0; /* we are a gateway ? */

/**
 * setting aodv protocol enable or disable expanding_ring_search.
 */
int aodv_expanding_ring_search_get (void)
{
  return expanding_ring_search;
}

void aodv_expanding_ring_search_set (int enable)
{
  expanding_ring_search = enable;
}

static void aodv_wait_on_reboot_init (void)
{
  aodv_timer_init(&wait_on_reboot_timer, 
                  aodv_wait_on_reboot_timeout,
                  (void *)&aodv_wait_on_reboot);
  aodv_timer_set_timeout(&wait_on_reboot_timer, DELETE_PERIOD);
}

int aodv_wait_on_reboot_get (void)
{
  return (aodv_wait_on_reboot);
}

/**
 * setting if we are a gateway
 */
void aodv_gw_set (int enable)
{
#ifdef SYLIXOS
  KN_SMP_MB();
#endif
  aodv_gw = enable;
#ifdef SYLIXOS
  KN_SMP_MB();
#endif
}

int aodv_gw_get (void)
{
#ifdef SYLIXOS
  KN_SMP_MB();
#endif
  return (aodv_gw);
#ifdef SYLIXOS
  KN_SMP_MB();
#endif
}


/**
 * Add a aodv network interface to the list of lwIP netifs.
 *
 * @param netif a pre-allocated netif structure
 * @param ipaddr IP address for the new netif
 * @param netmask network mask for the new netif
 * @param gw default gateway IP address for the new netif
 * @param state opaque data passed to the new netif
 * @param init callback function that initializes the interface
 * @param input callback function that is called to pass
 * ingress packets up in the protocol layer stack.
 *
 * @return netif, or NULL if failed.
 */
struct netif *aodv_netif_add (struct netif *netif, ip4_addr_t *ipaddr, ip4_addr_t *netmask,
                              ip4_addr_t *gw, void *state, netif_init_fn init, netif_input_fn input)
{
  int i;
  static int aodv_proto_init = 0;
  
  if (aodv_proto_init == 0) {
    aodv_proto_init = 1;
    
    /* Intitialize the local sequence number an rreq_id to zero */
    aodv_host_state.seqno = 1; /* zero is invalid */
    aodv_host_state.rreq_id = 0;
    
    aodv_rt_init(); /* route table init */
#if AODV_MCAST
    aodv_mrt_init(); /* multicast route table init */
#endif
    aodv_hello_start();
    aodv_wait_on_reboot_init();
    aodv_timer_init_svr();
  }
  
  if (netif_add(netif, ipaddr, netmask, gw, state, init, input) == NULL) {
    return NULL;
  }
  
  for (i = 0; i < AODV_MAX_NETIF; i++) {
    if (aodv_netif[i] == NULL) {
      aodv_netif[i] = netif;
      break;
    }
  }
  LWIP_ERROR("i < AODV_MAX_NETIF", (i < AODV_MAX_NETIF), {netif_remove(netif); return NULL;});
  
  aodv_udp_new(i); /* create a udp pcb */
  
#if AODV_MCAST
  aodv_mproxy_new(); /* multicast proxy */
  aodv_mtunnel_new(i); /* create multicast tunnel */
  aodv_igmp_new(i); /* create a igmp pcb */
#endif
  
  netif_set_link_down(netif); /* wait on reboot */
  
  return ERR_OK;
}

/**
 * Remove a network interface from the list of lwIP netifs.
 *
 * @param netif the network interface to remove
 */
void aodv_netif_remove (struct netif *netif)
{
  int i;
  
  if (netif == NULL) {
    return;
  }
  
  for (i = 0; i < AODV_MAX_NETIF; i++) {
    if (aodv_netif[i] == netif) {
      aodv_netif[i] = NULL;
      break;
    }
  }
  LWIP_ERROR("i < AODV_MAX_NETIF", (i < AODV_MAX_NETIF), return;);
  
  aodv_rt_delete_netif(netif);
  
#if AODV_MCAST
  aodv_mrt_delete_netif(netif);
#endif
  
  aodv_udp_remove(i);
  
#if AODV_MCAST
  aodv_mproxy_remove();
  aodv_igmp_remove(i);
  aodv_mtunnel_remove(i);
#endif
  
  netif_remove(netif);
}

/**
 * send a ipv4 unicast packet by aodv netif
 *
 * @param netif The lwIP network interface which the IP packet will be sent on.
 * @param p The pbuf(s) containing the IP packet to be sent.
 * @param ipaddr The IP address of the packet destination.
 *
 * @return err_t
 */
static err_t aodv_netif_output_unicast (struct netif *netif, struct pbuf *p, ip4_addr_t *ipaddr)
{
  u8_t do_send = 0;
  struct in_addr dest_addr;
  struct in_addr src_addr;
  struct aodv_rtnode *rt;
  struct aodv_rtnode *rev_rt;
  struct ip_hdr *iphdr = (struct ip_hdr *)p->payload;
  
  if (p->tot_len < sizeof(struct ip_hdr)) { /* not ip packet */
    return ERR_IF; /* only send ip packet */
  }
  
  dest_addr.s_addr = ipaddr->addr;
  src_addr.s_addr = iphdr->src.addr;
  
  if (!ip4_addr_netcmp(ipaddr, netif_ip4_addr(netif), netif_ip4_netmask(netif))) { /* not in same subnet */
    /* send to gateway */
    if (netif_ip4_gw(netif)->addr != IPADDR_ANY) {
      /* we use setting gateway */
      dest_addr.s_addr = netif_ip4_gw(netif)->addr;
    }
  }
  
  /* update rev_rt */
  if (src_addr.s_addr != netif_ip4_addr(netif)->addr) { /* source not me, this is an forward ip packet output */
    rev_rt = aodv_rt_find(&src_addr);
    if (rev_rt) {
      aodv_rt_update_timeout(rev_rt, ACTIVE_ROUTE_TIMEOUT); /* rev_rt route is valid */
    }
  }
  
__re_find:
  rt = aodv_rt_find(&dest_addr);
  if (rt && (rt->state == AODV_VALID)) { /* valid unicast route */
    if (rt->flags & AODV_RT_INET_DEST) { /* inet "fake" route, we must send to gateway */
      dest_addr.s_addr = rt->next_hop.s_addr; /* gateway addr */
      goto __re_find;
    
    } else if (rt->flags & AODV_RT_UNIDIR) { /* uni-direction route */
      /* In direction route, ONLY can send RREP packet! */
      struct udp_hdr udphdr;
      u16_t iphdr_hlen;
      u8_t type;
      if (IPH_PROTO(iphdr) == IP_PROTO_UDP) { /* UDP packet */
        /* obtain IP header length in number of 32-bit words */
        iphdr_hlen = (u16_t)IPH_HL(iphdr);
        /* calculate IP header length in bytes */
        iphdr_hlen *= 4;
        if (pbuf_copy_partial(p, (void *)&udphdr, UDP_HLEN, iphdr_hlen) == UDP_HLEN) { /* get UDP header */
          if (ntohs(udphdr.dest) == AODV_PORT) { /* AODV packet */
            if (pbuf_copy_partial(p, (void *)&type, 1, (u16_t)(iphdr_hlen + UDP_HLEN)) == 1) {
              if ((type == AODV_RREP) || (type == AODV_RREP_ACK)) { /* RREP packet or RREP-ACK allowed */
                do_send = 1;
              }
            }
          }
        }
      }
    } else { /* can send to next_hop node */
      do_send = 1;
    }
  }
  
  if (do_send) {
    ip4_addr_t ipaddr_next;
    ipaddr_next.addr = rt->next_hop.s_addr; /* use next hop */
    return AODV_PACKET_OUTPUT(netif, p, &ipaddr_next); /* send to next hop */
  
  } else {
    /* Need send RREQ to find route to dest */
    if (rt && (rt->flags & AODV_RT_REPAIR)) {
      src_addr.s_addr = netif_ip4_addr(netif)->addr;
      aodv_rreq_local_repair(rt, &src_addr, p); /* try repair */
    
    } else {
      u8_t flags = 0;
      if ((IPH_PROTO(iphdr) == IP_PROTO_TCP) || (IPH_PROTO(iphdr) == IP_PROTO_ICMP)) {
        flags |= RREQ_GRATUITOUS; /* tcp protocol or ICMP MUST get the whole path */
      }
      aodv_rreq_route_discovery(&dest_addr, flags, p, NULL, NULL); /* discovery will queue one packet */
    }
    return ERR_OK;
  }
}

#if AODV_MCAST
/**
 * send a ipv4 multicast packet by aodv netif
 *
 * @param netif The lwIP network interface which the IP packet will be sent on.
 * @param p The pbuf(s) containing the IP packet to be sent.
 * @param ipaddr The IP address of the packet destination.
 *
 * @return err_t
 */
static err_t aodv_netif_output_multicast (struct netif *netif, struct pbuf *p, ip4_addr_t *ipaddr)
{
  u16_t iphdr_hlen;
  struct ip_hdr *iphdr = (struct ip_hdr *)p->payload;
  
  struct in_addr grp_addr;
  struct igmp_msg igmp;
  
  grp_addr.s_addr = ipaddr->addr;
  
  if (p->tot_len < sizeof(struct ip_hdr)) {
    return ERR_OK;
  }
  
  if (IPH_PROTO(iphdr) == IP_PROTO_IGMP) { /* Intercept IGMP */
    /* obtain IP header length in number of 32-bit words */
    iphdr_hlen = (u16_t)IPH_HL(iphdr);
    /* calculate IP header length in bytes */
    iphdr_hlen *= 4;
  
    pbuf_copy_partial(p, (void *)&igmp, sizeof(struct igmp_msg), iphdr_hlen);
  
    switch (igmp.igmp_msgtype) {
    
    case IGMP_V1_MEMB_REPORT:
    case IGMP_V2_MEMB_REPORT: /* join to a multicast group */
      aodv_mcast_join(&grp_addr, netif);
      break;
      
    case IGMP_LEAVE_GROUP: /* leave from a multicast group */
      aodv_mcast_leave(&grp_addr);
      break;
    }
  
  } else { /* send multicast packet */
    ip4_addr_t rev_addr;
    rev_addr.addr = netif_ip4_addr(netif)->addr; /* rev_ip is me */
    AODV_MPACKET_OUTPUT(netif, p, ipaddr, &rev_addr); /* send multicast packet */
  }
  
  return ERR_OK;
}
#endif

/**
 * send a ipv4 packet by aodv netif
 *
 * @param netif The lwIP network interface which the IP packet will be sent on.
 * @param p The pbuf(s) containing the IP packet to be sent.
 * @param ipaddr The IP address of the packet destination.
 *
 * @return err_t
 */
err_t aodv_netif_output (struct netif *netif, struct pbuf *p, ip4_addr_t *ipaddr)
{
  if (ipaddr->addr == IPADDR_BROADCAST) { /* broadcast packet */
    return AODV_PACKET_OUTPUT(netif, p, ipaddr);
  }
  
  if (ip4_addr_ismulticast(ipaddr)) { /* multicast packet */
#if AODV_MCAST
    return aodv_netif_output_multicast(netif, p, ipaddr);
#else
    return ERR_IF; /* not support multicast */
#endif
  
  } else { /* unicast packet */
    return aodv_netif_output_unicast(netif, p, ipaddr);
  }
}

#if LWIP_IPV6
/**
 * send a ipv6 packet by aodv netif
 *
 * @param netif The lwIP network interface which the IP packet will be sent on.
 * @param p The pbuf(s) containing the IP packet to be sent.
 * @param ip6addr The IP address of the packet destination.
 *
 * @return err_t
 */
err_t aodv_netif_output6 (struct netif *netif, struct pbuf *p, ip6_addr_t *ip6addr)
{
  return ERR_RTE;
}
#endif /* LWIP_IPV6 */
/* end */
