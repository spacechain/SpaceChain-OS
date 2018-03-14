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

#include "aodv_proto.h"
#include "aodv_param.h"
#include "aodv_route.h"
#include "aodv_mcast.h"
#include "aodv_timer.h"
#include "aodv_if.h"
#include "aodv_hello.h"
#include "aodv_neighbor.h"
#include "aodv_seeklist.h"

#include <string.h>

extern int aodv_wait_on_reboot_get(void);

/**
 * create a rrep packet (pbuf)
 *
 * @param flags packet flags
 * @param prefix same routing prefix
 * @param hcnt The number of hops from the Originator IP Address to the Destination IP Address
 * @param dest_addr destination address
 * @param dest_seqno destination sequence number 
 * @param orig_addr originator address
 * @param life The time in milliseconds for which nodes receiving the RREP consider the route to be valid.
 * @param ext_size extern data size
 * @param grp_addr group addr
 * @return rrep packet
 */
struct pbuf *aodv_rrep_create (u8_t flags, u8_t prefix, u8_t hcnt, struct in_addr *dest_addr,
                               u32_t dest_seqno, struct in_addr *orig_addr, u32_t life, u16_t ext_size,
                               struct in_addr *grp_addr)
{
  struct pbuf *p;
  struct aodv_rrep *rrep;
  u16_t length = (u16_t)(RREP_SIZE + ext_size);
  
#if AODV_MCAST
  GRP_MERGE_EXT gme;
  
  if (flags & RREP_REPAIR) {
    length += GRP_MERGE_EXT_SIZE;
  }
#endif /* AODV_MCAST */

#if AODV_MCAST
  if (!grp_addr) {
    grp_addr = &aodv_addr_any;
  }
#endif /* AODV_MCAST */
  
  p = pbuf_alloc(PBUF_TRANSPORT, length, PBUF_RAM);
  if (p) {
    rrep = (struct aodv_rrep *)p->payload;
    rrep->type = AODV_RREP;
    rrep->flags = flags;
    rrep->hcnt = hcnt;
    rrep->dest_addr = dest_addr->s_addr;
    rrep->dest_seqno = htonl(dest_seqno);
    rrep->orig_addr = orig_addr->s_addr;
    rrep->lifetime = htonl(life);
    RREP_FREFIX_SET(rrep, prefix);
  } else {
    LWIP_DEBUGF(AODV_DEBUG, ("aodv_rrep_create: can not allocate rrep packet!\n"));
    return NULL;
  }
  
#if AODV_MCAST
  if (flags & RREP_REPAIR) {
    /* add group merge extension */
    gme.type = GME_TYPE;
    gme.length = GME_LENGTH;
    gme.res = 0;
    gme.grp_addr = grp_addr->s_addr;
    gme.grp_seqno = 0;
    memcpy((char *)rrep + RREP_SIZE + ext_size, &gme, GRP_MERGE_EXT_SIZE); /* add group merge msg at last! */
  }
#endif /* AODV_MCAST */
  
  return p;
}

/**
 * add a rrep extern data (pbuf must have enough space)
 *
 * @param p rrep packet
 * @param type extern data type
 * @param offset offset bytes of rrep packet
 * @param len data len
 * @param data extern data
 * @return rrep extern data
 */
struct aodv_ext *aodv_rrep_ext_add (struct pbuf *p, u8_t type, u16_t offset, u8_t len, char *data)
{
  struct aodv_ext *ext = NULL;
  struct aodv_rrep *rrep = (struct aodv_rrep *)p->payload;
  
  if (offset < RREP_SIZE) {
    return NULL;
  }
  
  ext = (struct aodv_ext *)((char *)rrep + offset);
  
  ext->type = type;
  ext->length = len;
  
  memcpy(AODV_EXT_DATA(ext), data, len);
  
  return ext;
}

/**
 * create a rrep_ack packet (pbuf)
 *
 * @return rrep_ack packet
 */
struct pbuf *aodv_rrep_ack_create (void)
{
  struct pbuf *p;
  struct aodv_rrep_ack *rrep_ack;
  
  p = pbuf_alloc(PBUF_TRANSPORT, RREP_ACK_SIZE, PBUF_RAM);
  if (p) {
    rrep_ack = (struct aodv_rrep_ack *)p->payload;
    rrep_ack->type = AODV_RREP_ACK;
    rrep_ack->rsvd = 0;
  } else {
    LWIP_DEBUGF(AODV_DEBUG, ("aodv_rrep_ack_create: can not allocate rrep_ack packet!\n"));
  }
  
  return p;
}

/**
 * process a rrep_ack packet
 * 
 * @param p pbuf save the rrep packet
 * @param ip_src source address
 * @param ip_dest destnation address
 */
void aodv_rrep_ack_process (struct pbuf *p, struct in_addr *ip_src, struct in_addr *ip_dest)
{
#if AODV_DEBUG
  char buffer[INET_ADDRSTRLEN];
#endif

  struct aodv_rtnode *rt;
  
  LWIP_ERROR("p != NULL", (p != NULL), return;);
  LWIP_ERROR("p->len >= RREP_ACK_SIZE", (p->len >= RREP_ACK_SIZE), return;);
  LWIP_ERROR("p->len == p->tot_len", (p->len == p->tot_len), return;); /* must in a no splite pbuf */
  
  rt = aodv_rt_find(ip_src);
  
  if (rt == NULL) {
    LWIP_DEBUGF(AODV_DEBUG, ("aodv_rrep_ack_process: No RREP_ACK expected for %s!\n", 
                inet_ntoa_r(*ip_src, buffer, INET_ADDRSTRLEN)));
    return;
  }
  
  if (rt->flags & AODV_RT_UNIDIR) {
    rt->flags &= ~AODV_RT_UNIDIR; /* can receive rrep-ack, so this route not a uni-directional */
  }
  
  LWIP_DEBUGF(AODV_DEBUG, ("aodv_rrep_ack_process: Received RREP_ACK from %s!\n", 
              inet_ntoa_r(*ip_src, buffer, INET_ADDRSTRLEN)));
                
  /* Remove unexpired timer for this RREP_ACK */
  aodv_timer_remove(&rt->ack_timer);
}

/**
 * send rrep packet
 * 
 * @param p pbuf save the rrep packet
 * @param rev_rt recv node
 * @param fwd_rt forward node
 */
void aodv_rrep_send (struct pbuf *p, 
                     struct aodv_rtnode *rev_rt,
                     struct aodv_rtnode *fwd_rt)
{
#if AODV_DEBUG
  char buffer1[INET_ADDRSTRLEN];
  char buffer2[INET_ADDRSTRLEN];
  char buffer3[INET_ADDRSTRLEN];
#endif

  struct aodv_rrep *rrep;
  struct aodv_rtnode *neighbor;
  
#if AODV_DEBUG
  struct in_addr dest;
#endif

  LWIP_ERROR("p != NULL", (p != NULL), return;);
  LWIP_ERROR("rev_rt != NULL", (rev_rt != NULL), return;);
  
  rrep = (struct aodv_rrep *)p->payload;
  
#if AODV_DEBUG
  dest.s_addr = rrep->dest_addr;
#endif

  if (aodv_wait_on_reboot_get()) {
    return;
  }
  
  /* Check if we should request a RREP-ACK */
  if (((rev_rt->state == AODV_VALID) && (rev_rt->flags & AODV_RT_UNIDIR)) ||
      (rev_rt->hcnt == 1)) {
    neighbor = aodv_rt_find(&rev_rt->next_hop);
    
    if ((neighbor && neighbor->state == AODV_VALID) && 
        (!__AODV_TIMER_ISINQ(&neighbor->ack_timer))) {
      
      /* If the node we received a RREQ for is a neighbor we are
         probably facing a unidirectional link... Better request a
         RREP-ack */
      rrep->flags |= RREP_ACK;
      
      LWIP_DEBUGF(AODV_DEBUG, ("aodv_rrep_send: Link to %s is unidirectional!\n",
                  inet_ntoa_r(neighbor->dest_addr, buffer1, INET_ADDRSTRLEN)));
    }
  }
  
  LWIP_DEBUGF(AODV_DEBUG, ("aodv_rrep_send: Sending RREP to next hop %s about %s->%s\n",
              inet_ntoa_r(rev_rt->next_hop, buffer1, INET_ADDRSTRLEN),
              inet_ntoa_r(rev_rt->dest_addr, buffer2, INET_ADDRSTRLEN),
              inet_ntoa_r(dest, buffer3, INET_ADDRSTRLEN)));
              
  /* we must first send this packet, because the following call aodv_neighbor_link_break() will make this route
   * invalid!
   */
  aodv_udp_sendto(p, &rev_rt->next_hop, (u8_t)(~0), aodv_rt_netif_index_get(rev_rt));
  
  if (rrep->flags & RREP_ACK) {
    neighbor->flags |= AODV_RT_UNIDIR;
    
    /* Must remove any pending hello timeouts when we set the
       RT_UNIDIR flag, else the route may expire after we begin to
       ignore hellos... */
    aodv_timer_remove(&neighbor->hello_timer);
    aodv_neighbor_link_break(neighbor);
    
    aodv_timer_set_timeout(&neighbor->ack_timer, NEXT_HOP_WAIT); /* wait RREP-ACK */
  }
  
  /* Update precursor lists */
  if (fwd_rt) {
    aodv_precursor_add(fwd_rt, &rev_rt->next_hop);
    aodv_precursor_add(rev_rt, &fwd_rt->next_hop);
  }
  
#ifdef AODV_OPTIMIZED_HELLOS /* do not use this NOW */
  /* reset hello timer counter */
  aodv_hello_reset();
#endif
}

/**
 * forward rrep_ack packet
 * 
 * @param p pbuf save the rrep packet
 * @param rev_rt recv node
 * @param fwd_rt forwad node
 * @param ttl ip packet time to live
 */
void aodv_rrep_forward (struct pbuf *p, 
                        struct aodv_rtnode *rev_rt, 
                        struct aodv_rtnode *fwd_rt, 
                        u8_t ttl)
{
#if AODV_DEBUG
  char buffer[INET_ADDRSTRLEN];
#endif

  struct aodv_rrep *rrep;

  LWIP_ERROR("p != NULL", (p != NULL), return;);
  LWIP_ERROR("p->len >= RREP_SIZE", (p->len >= RREP_SIZE), return;);
  LWIP_ERROR("rev_rt != NULL", (rev_rt != NULL), return;);
  LWIP_ERROR("fwd_rt != NULL", (fwd_rt != NULL), return;);
  
  rrep = (struct aodv_rrep *)p->payload;
  
  LWIP_DEBUGF(AODV_DEBUG, ("aodv_rrep_forward: Forwarding RREP to %s\n", 
              inet_ntoa_r(rev_rt->next_hop, buffer, INET_ADDRSTRLEN)));
              
  /* Here we should do a check if we should request a RREP_ACK,
     i.e we suspect a unidirectional link.. But how? */
  if (0) {
    struct aodv_rtnode *neighbor;

    /* If the source of the RREP is not a neighbor we must find the
       neighbor (link) entry which is the next hop towards the RREP
       source... */
    if (rev_rt->dest_addr.s_addr != rev_rt->next_hop.s_addr) {
      neighbor = aodv_rt_find(&rev_rt->next_hop);
    } else {
      neighbor = rev_rt;
    }

    if (neighbor && (!__AODV_TIMER_ISINQ(&neighbor->ack_timer))) {
      /* If the node we received a RREQ for is a neighbor we are
         probably facing a unidirectional link... Better request a
         RREP-ack */
      rrep->flags |= RREP_ACK;
      neighbor->flags |= AODV_RT_UNIDIR;

      aodv_timer_set_timeout(&neighbor->ack_timer, NEXT_HOP_WAIT);
    }
  }

  rrep->hcnt = fwd_rt->hcnt;    /* Update the hopcount */
  
  aodv_udp_sendto(p, &rev_rt->next_hop, ttl, aodv_rt_netif_index_get(rev_rt));

  aodv_precursor_add(fwd_rt, &rev_rt->next_hop);
  aodv_precursor_add(rev_rt, &fwd_rt->next_hop);

  aodv_rt_update_timeout(rev_rt, ACTIVE_ROUTE_TIMEOUT);
}

/**
 * process a rrep packet
 * 
 * @param p pbuf save the rrep packet
 * @param ip_src source address
 * @param ip_dst destination address
 * @param ttl ip packet time to live
 * @param netif the network interface on which the packet was received
 */
void aodv_rrep_process (struct pbuf *p, 
                        struct in_addr *ip_src,
                        struct in_addr *ip_dst, 
                        u8_t ttl,
                        struct netif *netif)
{
#if AODV_DEBUG
  char buffer1[INET_ADDRSTRLEN];
  char buffer2[INET_ADDRSTRLEN];
  char buffer3[INET_ADDRSTRLEN];
#endif

  struct aodv_rrep *rrep;

  u32_t rrep_lifetime, rrep_seqno;
  u8_t rrep_new_hcnt;
  u16_t pre_repair_hcnt = 0, pre_repair_flags = 0;
  struct aodv_rtnode *fwd_rt, *rev_rt;
  
  u8_t rt_flags = 0;
  struct in_addr rrep_dest, rrep_orig;
  
  int inet_rrep = 0;
  struct in_addr inet_dest_addr;
  struct aodv_ext *ext;
  int rreplen;
  
#if AODV_MCAST
  u8_t gmg_en = 0;
  GRP_MERGE_EXT  gmg_ext;
  struct in_addr grp_addr = aodv_addr_any;
#endif /* AODV_MCAST */
  
  LWIP_ERROR("p != NULL", (p != NULL), return;);
  LWIP_ERROR("p->len >= RREP_SIZE", (p->len >= RREP_SIZE), return;);
  LWIP_ERROR("p->len == p->tot_len", (p->len == p->tot_len), return;); /* must in a no splite pbuf */
  LWIP_ERROR("ip_src != NULL", (ip_src != NULL), return;);
  LWIP_ERROR("ip_dst != NULL", (ip_dst != NULL), return;);
  LWIP_ERROR("netif != NULL", (netif != NULL), return;);
  
  rrep = (struct aodv_rrep *)p->payload;
  rreplen = p->tot_len;
  
  /* Convert to correct byte order on affeected fields: */
  rrep_dest.s_addr = rrep->dest_addr;
  rrep_orig.s_addr = rrep->orig_addr;
  rrep_seqno       = ntohl(rrep->dest_seqno);
  rrep_lifetime    = ntohl(rrep->lifetime);
  
  /* Increment RREP hop count to account for intermediate node... */
  rrep_new_hcnt = (u8_t)(rrep->hcnt + 1);
  
  /* Ignore messages which aim to a create a route to one self */
  if (rrep_dest.s_addr == netif_ip4_addr(netif)->addr) {
    return;
  }
  
  LWIP_DEBUGF(AODV_DEBUG, ("aodv_rrep_process: from %s about %s->%s %s\n",
              inet_ntoa_r(*ip_src, buffer1, INET_ADDRSTRLEN), 
              inet_ntoa_r(rrep_orig, buffer2, INET_ADDRSTRLEN), 
              inet_ntoa_r(rrep_dest, buffer3, INET_ADDRSTRLEN),
              (rrep->flags & RREP_ACK) ? "need-ack" : ""));
              
  /* Determine whether there are any extensions */
  ext = (struct aodv_ext *)((char *)rrep + RREP_SIZE);
  while (rreplen > (int)RREP_SIZE) { /* have extern data */
    switch (ext->type) {
    
    case RREP_INET_DEST_EXT: /* real inet address */
      if (ext->length == sizeof(u32_t)) {
        /* Destination address in RREP is the gateway address, while the
         * extension holds the real destination */
        memcpy(&inet_dest_addr, AODV_EXT_DATA(ext), ext->length);
        rt_flags |= AODV_RT_GATEWAY;
        inet_rrep = 1;
      }
      break;
      
#if AODV_MCAST
    case RREP_GRP_MERGE_EXT:
      if (ext->length == GME_LENGTH) {
        memcpy(&gmg_ext, ext, sizeof(gmg_ext));
        memcpy(&grp_addr, &gmg_ext.grp_addr, sizeof(struct in_addr));
        gmg_en = 1;
      }
      break;
#endif /* AODV_MCAST */
    }
    rreplen -= (u16_t)AODV_EXT_SIZE(ext);
    ext = AODV_EXT_NEXT(ext);
  }
  
#if AODV_MCAST
  if (IN_MULTICAST(ntohl(rrep_dest.s_addr))) {
    aodv_mcast_rrep_process(p, ip_src, ip_dst, ttl, netif);
    return;
  }
#endif /* AODV_MCAST */

  /* check if we should make a foward route */
  fwd_rt = aodv_rt_find(&rrep_dest);
  rev_rt = aodv_rt_find(&rrep_orig);
  
  if (!fwd_rt) {
    /* We didn't have an existing entry, so we insert a new one. */
    fwd_rt = aodv_rt_insert(&rrep_dest, ip_src, rrep_new_hcnt, rrep_seqno,
                            rrep_lifetime, AODV_VALID, rt_flags, netif);
                            
    LWIP_ERROR("fwd_rt != NULL", (fwd_rt != NULL), return;);
                            
  } else if ((fwd_rt->dest_seqno == 0) ||
             ((s32_t)rrep_seqno > (s32_t)fwd_rt->dest_seqno) ||
             ((rrep_seqno == fwd_rt->dest_seqno) &&
              (fwd_rt->state == AODV_INVALID || fwd_rt->flags & AODV_RT_UNIDIR ||
               (rrep_new_hcnt < fwd_rt->hcnt)))) {
    
    /* update condition(s):
     * fwd entry seqno is zero || rrep seqno > old entry seqno || 
     * (seqno equ && (fwd entry invalid || fwd entry is unidirection || short hop))
     */
    pre_repair_hcnt = fwd_rt->hcnt;
    pre_repair_flags = fwd_rt->flags;
    
    /* if rrep_new_hcnt == 1 && ip_dst == me, then this packet is response for my rreq.
     * so, this link is not a uni-direction
     */
    if ((rrep_new_hcnt == 1) && (ip_dst->s_addr == netif_ip4_addr(netif)->addr)) {
      fwd_rt->flags &= ~AODV_RT_UNIDIR;
    }
    
    fwd_rt = aodv_rt_update(fwd_rt, ip_src, rrep_new_hcnt, rrep_seqno,
                            rrep_lifetime, AODV_VALID,
                            (u16_t)(rt_flags | fwd_rt->flags));
  } else {
    if (fwd_rt->hcnt > 1) {
      LWIP_DEBUGF(AODV_DEBUG, ("aodv_rrep_process: Dropping RREP, fwd_rt->hcnt=%d fwd_rt->seqno=%ld\n",
                  fwd_rt->hcnt, fwd_rt->dest_seqno));
    }
    return;
  }
  
  /* If the RREP_ACK flag is set we must send a RREP
     acknowledgement to the destination that replied... */
  if (rrep->flags & RREP_ACK) {
    struct pbuf *p_ack = aodv_rrep_ack_create();
    
    if (p_ack) {
      aodv_udp_sendto(p_ack, &fwd_rt->next_hop, (u8_t)(~0), aodv_rt_netif_index_get(fwd_rt));
    
      pbuf_free(p_ack);
    }
    
    /* Remove RREP_ACK flag... */
    rrep->flags &= ~RREP_ACK;
  }
  
  /* Check if this RREP was for us (i.e. we previously made a RREQ
     for this host). */
  if (rrep_orig.s_addr == netif_ip4_addr(netif)->addr) {
    
    if (inet_rrep) { /* next hop is subnet router */
      struct aodv_rtnode *inet_rt;
      
      inet_rt = aodv_rt_find(&inet_dest_addr);

      /* Add a "fake" route indicating that this is an Internet
       * destination, thus should be encapsulated and routed through a
       * gateway... */
      if (!inet_rt) {
        
        aodv_rt_insert(&inet_dest_addr, &rrep_dest, rrep_new_hcnt, 0,
                      rrep_lifetime, AODV_VALID, AODV_RT_INET_DEST, netif);
      
      } else if (inet_rt->state == AODV_INVALID || rrep_new_hcnt < inet_rt->hcnt) {
        
        aodv_rt_update(inet_rt, &rrep_dest, rrep_new_hcnt, 0,
                       rrep_lifetime, AODV_VALID, 
                       (u16_t)(AODV_RT_INET_DEST | inet_rt->flags));
      
      } else {
        LWIP_DEBUGF(AODV_DEBUG, ("aodv_rrep_process: INET Response, but no update %s\n",
                    inet_ntoa_r(inet_dest_addr, buffer1, INET_ADDRSTRLEN)));
      }
    }
    
    /* If the route was previously in repair, a NO DELETE RERR should be
       sent to the source of the route, so that it may choose to reinitiate
       route discovery for the destination. Fixed a bug here that caused the
       repair flag to be unset and the RERR never being sent. Thanks to
       McWood <hjw_5@hotmail.com> for discovering this. */
    if (pre_repair_flags & AODV_RT_REPAIR) {
      if (fwd_rt->hcnt > pre_repair_hcnt) {
        struct pbuf *p_rerr;
        u8_t rerr_flags = 0;
        struct in_addr dest;

        dest.s_addr = INADDR_BROADCAST;
        rerr_flags |= RERR_NODELETE;
        
        p_rerr = aodv_rerr_create(rerr_flags, &fwd_rt->dest_addr,
                                  fwd_rt->dest_seqno);
        if (p_rerr) {
          if (fwd_rt->nprec) {
            aodv_udp_sendto(p_rerr, &dest, 1, aodv_rt_netif_index_get(fwd_rt));
          }
          pbuf_free(p_rerr);
        }
      }
    } else {
#if AODV_MCAST
      struct aodv_mrtnode *mrt;
      
      if ((gmg_en) && 
          ((mrt = aodv_mrt_find(&grp_addr)) != NULL) && 
          (rrep->flags & RREP_REPAIR)) {
        struct aodv_mrtnexthop *upstream;
        
        /* make ip_src upstream */
        upstream = aodv_mrt_get_activated_upstream(mrt);
        if (upstream) {
          aodv_mrt_nexthop_deactive(mrt, &upstream->addr);
        }
        
        upstream = aodv_mrt_nexthop_find(mrt, ip_src);
        if (upstream == NULL) {
          upstream = aodv_mrt_nexthop_add(mrt, ip_src, AODV_MRT_NEXTHOP_UP, 0);
          LWIP_ERROR("upstream != NULL", (upstream != NULL), return;);
        }
        
        if (upstream->flags & AODV_MRT_NEXTHOP_ACT) {
          aodv_mrt_nexthop_deactive(mrt, &upstream->addr);
        }
        upstream->flags |= AODV_MRT_NEXTHOP_UP;
        
        aodv_mrt_nexthop_active(mrt, &upstream->addr);
        
        /* send mact */
        aodv_mact_send(MACT_JOIN, 1, &grp_addr, ip_src, 1);
        
        /* route is repaired */
        /* stop being leader */
        mrt->flags &= ~(AODV_MRT_REPAIR | AODV_MRT_LEADER);
        
        aodv_seeklist_remove_mcast(&rrep_dest, &grp_addr);
      }
#endif /* AODV_MCAST */
    }
  } else {
    /* --- Here we FORWARD the RREP on the REVERSE route --- */
    if (rev_rt && rev_rt->state == AODV_VALID) {
      ttl--;
#if AODV_MCAST
      if (rrep->flags & RREP_REPAIR) {
        /* make old activated upstream a downstream? or let mact? */
        struct aodv_mrtnode *mrt;
        
        if ((gmg_en) && 
            ((mrt = aodv_mrt_find(&grp_addr)) != NULL)) {
          struct aodv_mrtnexthop *upstream;
          
          /* make ip_src upstream */
          upstream = aodv_mrt_get_activated_upstream(mrt);
          if (upstream) {
            aodv_mrt_nexthop_deactive(mrt, &upstream->addr);
          }
          
          upstream = aodv_mrt_nexthop_find(mrt, ip_src);
          if (upstream == NULL) {
            upstream = aodv_mrt_nexthop_add(mrt, ip_src, AODV_MRT_NEXTHOP_UP, 0);
            LWIP_ERROR("upstream != NULL", (upstream != NULL), goto __forward;);
          }
          
          if (upstream->flags & AODV_MRT_NEXTHOP_ACT) {
            aodv_mrt_nexthop_deactive(mrt, &upstream->addr);
          }
          
          upstream->flags |= AODV_MRT_NEXTHOP_UP;
          
          aodv_mrt_start_routing(mrt);
          
          aodv_mrt_nexthop_active(mrt, &upstream->addr);
          
          /* send mact */
          aodv_mact_send(MACT_JOIN, 1, &grp_addr, ip_src, 1);
        }
      }
__forward:
#endif /* AODV_MCAST */
      aodv_rrep_forward(p, rev_rt, fwd_rt, ttl);
    } else {
      LWIP_DEBUGF(AODV_DEBUG, ("aodv_rrep_process: Could not forward RREP - NO ROUTE!\n"));
    }
  }
  
#ifdef AODV_OPTIMIZED_HELLOS /* do not use this NOW */
  /* reset hello timer counter */
  aodv_hello_reset();
#endif
}
/* end */
