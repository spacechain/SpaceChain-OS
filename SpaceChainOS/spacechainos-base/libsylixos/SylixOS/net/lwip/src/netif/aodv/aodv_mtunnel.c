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
#include "lwip/inet_chksum.h"

#include "aodv_proto.h"
#include "aodv_hello.h"
#include "aodv_neighbor.h"
#include "aodv_route.h"
#include "aodv_seeklist.h"
#include "aodv_mcast.h"
#include "aodv_if.h"

#if AODV_MCAST

extern ip_addr_t aodv_ipaddr_any;

static struct udp_pcb *aodv_mtunnel; /* initialize with NULL */

/**
 * aodv mtunnel forward
 * 
 * @param mrt multicast route entry
 * @param grp_addr group address
 * @param rev_addr rev_hop address
 * @param orig_addr multicast packet productor
 * @param p pbuf need forward
 */
static void aodv_mtunnel_forward (struct aodv_mrtnode *mrt, 
                                  struct in_addr *grp_addr, 
                                  struct in_addr *rev_addr, 
                                  struct in_addr *orig_addr,
                                  struct pbuf *p)
{
  struct aodv_mrtnexthop *rev_hop;
  struct aodv_mrtnexthop *up_hop;
  struct aodv_mrtnexthop *down;
  
  struct pbuf *pfw;
  ip_addr_t dst_ip;
  u16_t orig_len;
  
  if (pbuf_header(p, PBUF_LINK_HLEN + PBUF_IP_HLEN + PBUF_TRANSPORT_HLEN)) { /* pbuf is have enough header space? */
    pfw = pbuf_alloc(PBUF_TRANSPORT, p->tot_len, PBUF_RAM);
    LWIP_ERROR("pfw != NULL", (pfw != NULL), return;);
    pbuf_copy(pfw, p);
  
  } else {
    pbuf_header(p, -(PBUF_LINK_HLEN + PBUF_IP_HLEN + PBUF_TRANSPORT_HLEN));
    pfw = p;
  }
  
  orig_len = pfw->tot_len; /* orig packet len */
  
  /* if this packet rececive from downstream, it first forward to upstearm */
  rev_hop = aodv_mrt_nexthop_find(mrt, rev_addr);
  if (!rev_hop || (rev_hop->flags & AODV_MRT_NEXTHOP_UP) == 0) { /* revt invalid or is not a upstream */
    /* first forward to up stream except rev_addr */
    up_hop = aodv_mrt_get_best_upstream(mrt);
    if (up_hop && (up_hop->flags & AODV_MRT_NEXTHOP_ACT) && (up_hop != rev_hop)) {
      IP_SET_TYPE_VAL(dst_ip, IPADDR_TYPE_V4);
      ip_2_ip4(&dst_ip)->addr = up_hop->addr.s_addr;
      udp_sendto(aodv_mtunnel, pfw, &dst_ip, AODV_MTUNNEL);
      if (pfw->tot_len != orig_len) {
        /* udp_sendto() change the pfw buf, pfw->tot_len now is (orig_len + iphdr + udphdr) 
         * so we want to make pfw to origenal stat.
         */
        pbuf_header(pfw, (s16_t)(orig_len - pfw->tot_len)); /* pfw->payload point to packet (NOT point to ip hdr) */
      }
    }
  }
  
  /* send to all active downstream node except rev_addr */
  for (down = mrt->next_hop;
       down != NULL;
       down = down->next) {
    /* active and downstream */
    if ((down->flags & AODV_MRT_NEXTHOP_ACT) && 
        !(down->flags & AODV_MRT_NEXTHOP_UP)) {
      if (down != rev_hop) { /* prevent loop */
        IP_SET_TYPE_VAL(dst_ip, IPADDR_TYPE_V4);
        ip_2_ip4(&dst_ip)->addr = down->addr.s_addr;
        udp_sendto(aodv_mtunnel, pfw, &dst_ip, AODV_MTUNNEL);
        if (pfw->tot_len != orig_len) {
          pbuf_header(pfw, (s16_t)(orig_len - pfw->tot_len)); /* pfw->payload point to packet (NOT point to ip hdr) */
        }
      }
    }     
  }
  
  if (pfw != p) {
    pbuf_free(pfw);
  }
}

/**
 * aodv mtunnel pcb recv callback
 * 
 * @param arg callback argument.
 * @param pcb udp pcb
 * @param p recv pbuf list
 * @param addr remote address
 * @param port remote port
 */
static void aodv_mtunnel_recv (void *arg, struct udp_pcb *pcb, struct pbuf *p,
                               const ip_addr_t *addr, u16_t port)
{
  struct in_addr grp_addr, rev_addr, orig_addr;
  struct ip_hdr *iphdr;
  struct netif *netif = ip_current_netif();
  
  iphdr = (struct ip_hdr *)p->payload; /* the packet payload is a ip multicast packet */
#if LWIP_IPV6
  if (IPH_V(iphdr) == 6) {
    pbuf_free(p);
    return;
  }
#endif /* LWIP_IPV6 */
  
  if (p->tot_len < sizeof(struct ip_hdr)) {
    pbuf_free(p);
    return;
  }
  
  grp_addr.s_addr = iphdr->dest.addr; /* grp address */
  rev_addr.s_addr = ip4_current_src_addr()->addr; /* who send this packet to me (May not be original sender, so we must prevent loop) */
  orig_addr.s_addr = iphdr->src.addr; /* grp packet original sender */
  
  if (IN_MULTICAST(ntohl(grp_addr.s_addr))) {
    u16_t iphdr_hlen;
    struct aodv_mrtnode *mrt = aodv_mrt_find(&grp_addr);
    
    if ((mrt == NULL) || !(mrt->flags & AODV_MRT_ROUTER)) {
      /* we got a packet for which we have no entry */
      goto input;
    }
    
    if (mrt->flags & AODV_MRT_BROKEN) {
      aodv_rreq_route_discovery(&grp_addr, (RREQ_JOIN | RREQ_GRP_REBUILD), p, NULL, &rev_addr);
    
    } else if (iphdr->_ttl > 1) { /* can to forward */
      ip4_addr_t  grp_ip;
      
      grp_ip.addr = grp_addr.s_addr;
      
      /* route is valid, packet wants out, send the packet along its
       * way */
      aodv_mrt_source_add(&rev_addr, &grp_addr);
      
      /* we are a gateway, forward to remote multicast router, use netif_default */
      /* obtain IP header length in number of 32-bit words */
      iphdr_hlen = (u16_t)IPH_HL(iphdr);
      /* calculate IP header length in bytes */
      iphdr_hlen *= 4;
      
      iphdr->_ttl--;
      iphdr->_chksum = 0;
      iphdr->_chksum = inet_chksum(iphdr, iphdr_hlen);
      
      /* We are in gateway mode */
      if (aodv_gw_get()) {
        if (netif != mrt->netif) {  /* remote(tunnel) -> gateway_me(tunnel) -> aodv_node(tunnel) */
          aodv_mtunnel_forward(mrt, &grp_addr, &rev_addr, &orig_addr, p); /* forward to aodv subnet(tunnel) */
          
        } else if (netif_default && (netif != netif_default)) { /* aodv_node(tunnel) -> gateway_me(tunnel) -> remote */
          /* aodv netif not SylixOS default gateway netif */
          /* because pbuf is recv from mtunnel, so it must have room for link layer header, forward it to gateway! */
          netif_default->output(netif_default, p, &grp_ip); /* send multicast packet to default gateway netif */
        }
      } else { /* We are only aodv node mode */
        aodv_mtunnel_forward(mrt, &grp_addr, &rev_addr, &orig_addr, p); /* forward to aodv subnet(tunnel) */
      }
    }
    
input:
    /* local input a multicast packet */
    ip_input(p, netif); /* input packet (pbuf_free(p) by lwip) */
  
  } else {
    pbuf_free(p);
  }
}

/**
 * create a aodv multicast tunnel pcb
 *
 * @param aodv_if_index aodv netif index
 */
void aodv_mtunnel_new (int aodv_if_index)
{
  LWIP_ERROR("aodv_if_index < AODV_MAX_NETIF", (aodv_if_index < AODV_MAX_NETIF), return;);
  LWIP_ERROR("aodv_netif[aodv_if_index] != NULL", (aodv_netif[aodv_if_index] != NULL), return;);
  
  /* lwip only support one aodv netif, because can not bind socket to netif
   * so here has only one udp_pcb.
   */
  if (aodv_mtunnel) {
    return;
  }
  
  aodv_mtunnel = udp_new();
  if (aodv_mtunnel) {
    if (udp_bind(aodv_mtunnel, &aodv_ipaddr_any, AODV_MTUNNEL) != ERR_OK) {
      LWIP_DEBUGF(AODV_DEBUG, ("aodv_mtunnel_new: can not bind port %d!\n", AODV_MTUNNEL));
      return;
    }
    udp_recv(aodv_mtunnel, aodv_mtunnel_recv, NULL);
  } else {
    LWIP_DEBUGF(AODV_DEBUG, ("aodv_mtunnel_new: can not create udp_pcb!\n"));
  }
}

/**
 * remove a aodv mtunnel pcb
 *
 * @param aodv_if_index aodv netif index
 */
void aodv_mtunnel_remove (int aodv_if_index)
{
  LWIP_ERROR("aodv_if_index < AODV_MAX_NETIF", (aodv_if_index < AODV_MAX_NETIF), return;);

  if (aodv_mtunnel) {
    udp_remove(aodv_mtunnel);
    aodv_mtunnel = NULL;
  }
}

/**
 * send a multicast packet by mtunnel
 * 
 * @param netif aodv packet netif
 * @param p aodv packet
 * @param grp_ip destination group address
 * @param rev_ip how forward to me
 * @return send packet is ok or not.
 */
err_t aodv_mtunnel_output (struct netif *netif, struct pbuf *p, ip4_addr_t *grp_ip, ip4_addr_t *rev_ip)
{
  struct in_addr grp_addr;
  struct in_addr rev_addr;
  struct in_addr orig_addr;
  struct aodv_mrtnode *mrt;
  struct ip_hdr *iphdr = (struct ip_hdr *)p->payload;
  
  grp_addr.s_addr = grp_ip->addr;
  rev_addr.s_addr = rev_ip->addr;
  orig_addr.s_addr = iphdr->src.addr;
  
  mrt = aodv_mrt_find(&grp_addr);
  if (mrt && (mrt->activated_upstream_cnt || mrt->activated_downstream_cnt)) { /* we can do forward */
    aodv_mtunnel_forward(mrt, &grp_addr, &rev_addr, &orig_addr, p);
  
  } else if (mrt && (mrt->flags & AODV_MRT_LEADER)) { /* we are the leader and no member */
    return ERR_OK; /* currently do nothing! */
  
  } else { /* we must try send to this gourp */
    aodv_mcast_try_send(&grp_addr, netif, p);
  }
  
  return ERR_OK;
}

#endif /* AODV_MCAST */

/* end */
