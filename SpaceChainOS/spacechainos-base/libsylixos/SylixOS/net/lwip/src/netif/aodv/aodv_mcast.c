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
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/inet.h"

#include "aodv_list.h"
#include "aodv_proto.h"
#include "aodv_param.h"
#include "aodv_timercb.h"
#include "aodv_route.h"
#include "aodv_mcast.h"
#include "aodv_seeklist.h"
#include "aodv_if.h"

/* aodv multicast support */
#if AODV_MCAST

extern int aodv_wait_on_reboot_get(void);

/**
 * try send a packet to multicast group
 *
 * @param grp_addr group addr
 * @param netif adov netif
 * @param p packet
 */
void aodv_mcast_try_send (struct in_addr *grp_addr, struct netif *netif, struct pbuf *p)
{
  struct aodv_mrtnode *mrt;
  
  mrt = aodv_mrt_find(grp_addr);
  if (mrt == NULL) {
    mrt = aodv_mrt_insert(grp_addr, NULL, 255 /* max ttl */, 0, 
                          (GROUP_HELLO_INTERVAL * ALLOWED_HELLO_LOSS), 0, netif);
    LWIP_ERROR("mrt != NULL", (mrt != NULL), return;);
  }
  
  /* we must to be a membership of this group (not only send message) */
  aodv_mrt_set_membership(mrt, 1);
  
  if (!(mrt->flags & AODV_MRT_ROUTER)) {
    /* no route to group, so let's find one. */
    aodv_rreq_route_discovery(grp_addr, RREQ_JOIN, p, NULL, NULL);
  }
}

/**
 * join to a multicast group
 *
 * @param grp_addr group addr
 * @param netif adov netif
 */
void aodv_mcast_join (struct in_addr *grp_addr, struct netif *netif)
{
  struct aodv_mrtnode *mrt;
  
  mrt = aodv_mrt_find(grp_addr);
  if (mrt == NULL) {
    mrt = aodv_mrt_insert(grp_addr, NULL, 255 /* max ttl */, 0, 
                          (GROUP_HELLO_INTERVAL * ALLOWED_HELLO_LOSS), 0, netif);
    LWIP_ERROR("mrt != NULL", (mrt != NULL), return;);
  }
  
  /* `join` must as a membership */
  /* we must to be a membership of this group (not only send message) */
  aodv_mrt_set_membership(mrt, 1);
  
  if (!(mrt->flags & AODV_MRT_ROUTER)) {
    /* no route to group, so let's find one. */
    aodv_rreq_route_discovery(grp_addr, RREQ_JOIN, NULL, NULL, NULL);
  }
}

/**
 * leave from a multicast group
 *
 * @param grp_addr group addr
 */
void aodv_mcast_leave (struct in_addr *grp_addr)
{
  struct aodv_mrtnode *mrt;
  struct aodv_seeklist *s;
  
  mrt = aodv_mrt_find(grp_addr);
  
  /* don't leave if not in the group */
  if (mrt == NULL || !(mrt->flags & AODV_MRT_ROUTER)) {
    return;
  }
  
  /* leave group */
  mrt->flags &= ~AODV_MRT_MEMBER;
  
  /* If gateway mode is enabled, make sure the gateway flag is set */
  if (aodv_gw_get()) {
    mrt->flags |= AODV_MRT_GATEWAY;
  }
  
  if (mrt->flags & AODV_MRT_GATEWAY) {
    /* Only prune if no up or down stream */
    if ((mrt->activated_downstream_cnt == 0) &&
        (mrt->activated_upstream_cnt == 0)) {

      mrt->flags &= ~AODV_MRT_LEADER;
      mrt->flags &= ~AODV_MRT_GATEWAY;

      aodv_mrt_stop_routing(mrt);
      
      /* remove any pending RREQs (or other packets) for this group */
      s = aodv_seeklist_find(grp_addr);
      if (s) {
        aodv_seeklist_remove(s);
      }
      
      /* flush all nexthops */
      aodv_mrt_nexthop_remove_all(mrt);
    }
    
  } else if (mrt->flags & AODV_MRT_LEADER) {
    if (mrt->activated_downstream_cnt > 0) {
      /* do have downstreams, so make one new leader */
      aodv_mact_make_downstream_leader(mrt);
    }
    
    mrt->flags &= ~AODV_MRT_LEADER;
    
    aodv_mrt_stop_routing(mrt);
  
  } else if (mrt->activated_downstream_cnt == 0) {
    /* no downs, prune self and invalidate route */
    struct aodv_mrtnexthop *upstream;
    
    upstream = aodv_mrt_get_activated_upstream(mrt);
    if (upstream) {
      aodv_mact_send(MACT_PRUNE, 1, grp_addr, &upstream->addr, 1);
      aodv_mrt_nexthop_deactive(mrt, &upstream->addr);
    }
    aodv_mrt_stop_routing(mrt);

    /* remove any pending RREQs (or other packets) for this group */
    s = aodv_seeklist_find(grp_addr);
    if (s) {
      aodv_seeklist_remove(s);
    }
    
    /* flush all nexthops */
    aodv_mrt_nexthop_remove_all(mrt);
  }
}

/**
 * query multicast group
 *
 * @param netif aodv send netif
 */
void aodv_mcast_query (struct netif *netif)
{
  struct aodv_mrtnode *mrt;
  int n;
  
  /* Iterate through all the multicast routing entries */
  for (n = 0; n < AODV_MRT_TABLESIZE; n++) {
    for (mrt  = aodv_mrt_tbl.hash_tbl[n];
         mrt != NULL;
         mrt  = mrt->next) {
      aodv_igmp_sendreply(mrt, netif); /* notify back to query-er */
    }
  }
}

/**
 * send a rrep packet (destination address is a multicast address)
 *
 * @param rev_rt recv node
 * @param fwd_rt forward node
 */
static void aodv_mcast_rrep_send (struct aodv_rtnode  *rev_rt,
                                  struct aodv_mrtnode *fwd_rt)
{
#if AODV_DEBUG
  char buffer1[INET_ADDRSTRLEN];
  char buffer2[INET_ADDRSTRLEN];
  char buffer3[INET_ADDRSTRLEN];
#endif

  struct pbuf *p;
  
  LWIP_ERROR("rev_rt != NULL", (rev_rt != NULL), return;);
  LWIP_ERROR("fwd_rt != NULL", (fwd_rt != NULL), return;);
  
  if (aodv_wait_on_reboot_get()) {
    return;
  }

  LWIP_DEBUGF(AODV_DEBUG, ("aodv_mcast_rrep_send: Sending RREP to %s about %s->%s\n",
              inet_ntoa_r(rev_rt->next_hop, buffer1, INET_ADDRSTRLEN),
              inet_ntoa_r(rev_rt->dest_addr, buffer2, INET_ADDRSTRLEN),
              inet_ntoa_r(fwd_rt->grp_addr, buffer3, INET_ADDRSTRLEN)));
  
  p = aodv_rrep_create(0, 0, 0, &fwd_rt->grp_addr, fwd_rt->grp_seqno, &rev_rt->dest_addr,
                       ACTIVE_ROUTE_TIMEOUT, 0, NULL);
  if (p) {
    aodv_udp_sendto(p, &rev_rt->next_hop, (u8_t)(~0), aodv_rt_netif_index_get(rev_rt));
    pbuf_free(p);
  }
}

/**
 * forward a rrep packet (destination address is a multicast address)
 *
 * @param p pbuf save the rrep packet
 * @param rev_rt recv node
 * @param fwd_rt forwad node
 * @param ttl ip packet time to live
 */
static void aodv_mcast_rrep_forward (struct pbuf *p,
                                     struct aodv_rtnode  *rev_rt,
                                     struct aodv_mrtnode *fwd_rt,
                                     u8_t ttl)
{
#if AODV_DEBUG
  char buffer[INET_ADDRSTRLEN];
#endif

  struct aodv_mrtnexthop *upstream;
  struct aodv_rrep *rrep;
  
  LWIP_ERROR("p != NULL", (p != NULL), return;);
  LWIP_ERROR("p->len >= RREP_SIZE", (p->len >= RREP_SIZE), return;);
  LWIP_ERROR("rev_rt != NULL", (rev_rt != NULL), return;);
  LWIP_ERROR("fwd_rt != NULL", (fwd_rt != NULL), return;);
  
  rrep = (struct aodv_rrep *)p->payload;
  
  upstream = aodv_mrt_get_best_upstream(fwd_rt);
  
  (void)upstream;

  LWIP_DEBUGF(AODV_DEBUG, ("aodv_mcast_rrep_forward: Forwarding RREP to %s\n", 
              inet_ntoa_r(rev_rt->next_hop, buffer, INET_ADDRSTRLEN)));
  
  /* Update the hopcount */
  if (!(fwd_rt->flags & AODV_MRT_ROUTER)) {
    rrep->hcnt++;
  }
  
  aodv_udp_sendto(p, &rev_rt->next_hop, ttl, aodv_rt_netif_index_get(rev_rt));
}
 
/**
 * process a rrep packet (destination address is a multicast address)
 *
 * @param p pbuf save the rreq packet
 * @param ip_src source address
 * @param ip_dst destination address
 * @param ttl ip packet time to live
 * @param netif the network interface on which the packet was received
 */
void aodv_mcast_rrep_process (struct pbuf *p, 
                              struct in_addr *ip_src,
                              struct in_addr *ip_dst, 
                              u8_t ttl,
                              struct netif *netif)
{
  struct aodv_rrep *rrep;

  u32_t rrep_seqno;
  struct in_addr rrep_dest, rrep_orig;
  struct aodv_mrtnexthop *upstream;
  struct aodv_mrtnode *mrt;
  struct aodv_mrtorig *orig;
  
  /* aodv_rrep_process has already check arguments */
  rrep = (struct aodv_rrep *)p->payload;
  
  /* Convert to correct byte order on affeected fields: */
  rrep_dest.s_addr = rrep->dest_addr;
  rrep_orig.s_addr = rrep->orig_addr;
  rrep_seqno       = ntohl(rrep->dest_seqno);
  
  mrt = aodv_mrt_find(&rrep_dest);
  
  LWIP_ERROR("mrt != NULL", (mrt != NULL), return;);
  
  upstream = aodv_mrt_nexthop_find(mrt, ip_src);
  if (upstream == NULL) {
     upstream = aodv_mrt_nexthop_add(mrt, ip_src, AODV_MRT_NEXTHOP_UP, rrep_seqno);
     LWIP_ERROR("upstream != NULL", (upstream != NULL), return;);
  
  } else {
    /* update seqno */
    upstream->grp_seqno = rrep_seqno;
  }
  
  /* add the originator to the originator list if not there already */
  orig = aodv_mrt_origlist_find(mrt, &rrep_orig);
  if (orig == NULL) {
    orig = aodv_mrt_origlist_add(mrt, &rrep_orig);
    LWIP_ERROR("orig != NULL", (orig != NULL), return;);
  }

  LWIP_DEBUGF(AODV_DEBUG, ("aodv_mcast_rrep_process: Originator seqno: %d hcnt: %d\n", 
              orig->grp_seqno, orig->grp_hcnt));
  
  /* rrep is propagated if the prior rreq had a lower
       sequence number or the sequence numbers are equal and the
       incoming rrep has a smaller hopcount */ 
  if ((orig->grp_seqno < rrep_seqno) ||
       ((orig->grp_seqno == rrep_seqno) &&
        ( orig->grp_hcnt > rrep->hcnt))) {
    mrt->grp_seqno = rrep_seqno;
    orig->grp_seqno = rrep_seqno;
    orig->grp_hcnt = rrep->hcnt;
    
    if (!(upstream->flags & AODV_MRT_NEXTHOP_ACT)) {
      upstream->flags |= AODV_MRT_NEXTHOP_UP;
      upstream->grp_hcnt = rrep->hcnt;
    }
    
    if (rrep_orig.s_addr != netif_ip4_addr(netif)->addr) {
      struct aodv_rtnode *rev_rt = aodv_rt_find(&rrep_orig);
      if (rev_rt) {
        ttl--;
        aodv_mcast_rrep_forward(p, rev_rt, mrt, ttl);
      }
    }
  } else {
    LWIP_DEBUGF(AODV_DEBUG, ("aodv_mcast_rrep_process: mrt->grp_seqno: %d\trrep_seqno: %d\n", 
                mrt->grp_seqno, rrep_seqno));
  }
}

/**
 * process a rreq packet (destination address is a multicast address)
 *
 * @param p pbuf save the rreq packet
 * @param ip_src source address
 * @param ip_dst destination address
 * @param ttl ip packet time to live
 * @param netif the network interface on which the packet was received
 * @param grb_ext RREQ REBUILD extern
 */
void aodv_mcast_rreq_process (struct pbuf *p, 
                              struct in_addr *ip_src,
                              struct in_addr *ip_dst, 
                              u8_t ttl,
                              struct netif *netif,
                              GRP_REBUILD_EXT *grb_ext)
{
  struct aodv_rreq *rreq;
  
  struct aodv_rtnode *rev_rt;
  struct aodv_mrtnode *mrt;
  struct aodv_mrtnexthop *nh;
  struct aodv_mrtorig *orig;
  
  u32_t rreq_dest_seqno;
  
  struct in_addr rreq_dest, rreq_orig;
  
  /* aodv_rrep_process has already check arguments */
  rreq = (struct aodv_rreq *)p->payload;
  
  rreq_dest.s_addr = rreq->dest_addr;
  rreq_orig.s_addr = rreq->orig_addr;
  rreq_dest_seqno  = ntohl(rreq->dest_seqno);
  
  mrt = aodv_mrt_find(&rreq_dest);
  if (mrt == NULL) {
    /* no route found */
    mrt = aodv_mrt_insert(&rreq_dest, NULL, 255 /* max ttl */, rreq_dest_seqno, 
                          (GROUP_HELLO_INTERVAL * ALLOWED_HELLO_LOSS), 0, netif);
    LWIP_ERROR("mrt != NULL", (mrt != NULL), return;);
  }
  
  /* add nexthop downstream */
  nh = aodv_mrt_nexthop_find(mrt, &rreq_orig);
  if (nh == NULL) {
    nh = aodv_mrt_nexthop_add(mrt, ip_src, 0 /* DOWNSTREAM */, rreq_dest_seqno);
    LWIP_ERROR("nh != NULL", (nh != NULL), return;);
  }
  
  /* add the originator to the originator list if not there already */
  orig = aodv_mrt_origlist_find(mrt, &rreq_orig);
  if (orig == NULL) {
    orig = aodv_mrt_origlist_add(mrt, &rreq_orig);
    LWIP_ERROR("orig != NULL", (orig != NULL), return;);
  }

  LWIP_DEBUGF(AODV_DEBUG, ("aodv_mcast_rreq_process: Originator seqno: %d hcnt: %d\n", orig->grp_seqno, orig->grp_hcnt));
  
  if (rreq->flags & RREQ_JOIN) {
    /* a Join request */
    LWIP_DEBUGF(AODV_DEBUG, ("aodv_mcast_rreq_process: [j] Join.\n"));
    
    if (!(mrt->flags & AODV_MRT_ROUTER)) {
      LWIP_DEBUGF(AODV_DEBUG, ("aodv_mcast_rreq_process: [j] Found a mrt, but not on the tree.\n"));
    
    } else if (mrt->grp_seqno < rreq_dest_seqno) {
      LWIP_DEBUGF(AODV_DEBUG, ("aodv_mcast_rreq_process: [j] Found a mrt, but it is old. %d < %d\n", mrt->grp_seqno, rreq_dest_seqno));
      mrt->grp_seqno = rreq_dest_seqno;
    
    } else {
      if ((grb_ext == NULL) || (mrt->leader_hcnt <= grb_ext->grp_hcnt)) {
        LWIP_DEBUGF(AODV_DEBUG, ("aodv_mcast_rreq_process: [j] Should send RREP!\n"));
        /* send rrep */
        rev_rt = aodv_rt_find(&rreq_orig);
        
        LWIP_ERROR("rev_rt != NULL", (rev_rt != NULL), return;);
        
        orig->grp_seqno = mrt->grp_seqno;
        orig->grp_hcnt = 0;
        aodv_mcast_rrep_send(rev_rt, mrt);

        /* done! get out */
        return; 
      } else {
        LWIP_DEBUGF(AODV_DEBUG, ("aodv_mcast_rreq_process: had repair extension, bad hopcount, do nothing.\n"));
      }
    }
  } else {
    /* non-Join request */
    LWIP_DEBUGF(AODV_DEBUG, ("aodv_mcast_rreq_process: [] non-Join.\n"));

    if (mrt->grp_seqno < rreq_dest_seqno) {
      LWIP_DEBUGF(AODV_DEBUG, ("aodv_mcast_rreq_process: [] Found a mrt, but it is old."));
      /* update sequence number in our table */
      mrt->grp_seqno = rreq_dest_seqno;
    
    } else if (mrt->flags & AODV_MRT_ROUTER) {
      LWIP_DEBUGF(AODV_DEBUG, ("aodv_mcast_rreq_process: [] Should send RREP!"));

      /* send rrep */
      rev_rt = aodv_rt_find(&rreq_orig);
      
      LWIP_ERROR("rev_rt != NULL", (rev_rt != NULL), return;);
      
      orig->grp_seqno = mrt->grp_seqno;
      orig->grp_hcnt = 0;
      aodv_mcast_rrep_send(rev_rt, mrt);

      /* we're done! */
      return;
    }
  }
  
  /* if we get this far, no RREP has been sent. */

  /* now, forward RREQ */
  /* if rreq has unknown seqno and our table has a known one
     (not zero), then update packet seqno field. or if ours
     is just bigger.
     CHECK: this may not be the way to do this */
  if ((mrt->grp_seqno > rreq_dest_seqno) || 
       ((rreq->flags & RREQ_UNKWN_SEQ) && mrt->grp_seqno)) {
    rreq->flags &= ~RREQ_UNKWN_SEQ;
    rreq->dest_seqno = htonl(mrt->grp_seqno);
  }
  
#define aodv_mcast_rreq_forward aodv_rreq_forward
  
  /* send it back out */
  ttl--;
  aodv_mcast_rreq_forward(p, ttl);
}

/**
 * rreq packet retries
 *
 * @param seek_entry seeklist entry node
 * @return 1: already remove seek entry
 */
int aodv_mcast_rreq_retries (struct aodv_seeklist *seek_entry)
{
  struct in_addr dest_addr;
  struct aodv_seeklist *s;

  if ((seek_entry->flags & RREQ_JOIN) && !(seek_entry->flags & RREQ_REPAIR)) {
    struct aodv_mrtnode *group_rec;
    struct aodv_mrtnexthop *upstream;
    
    /* try to join the group */
    group_rec = aodv_mrt_find(&seek_entry->dest_addr);
    if (group_rec == NULL) {
      return (0);
    }
    
    upstream = aodv_mact_activate_best_upstream(group_rec);
    if (upstream) {
      if (group_rec->flags & AODV_MRT_BROKEN) {
        /* we were repairing route */
        group_rec->flags &= ~AODV_MRT_BROKEN;
      
      } else if (group_rec->flags & AODV_MRT_GATEWAY) {
        /* Gateway joining, not a member */
      
      } else {
        /* we were able to select an upstream */
        group_rec->flags |= AODV_MRT_MEMBER;
      }
    } else {
      if (!(group_rec->flags & AODV_MRT_MEMBER)) { /* only trying to send a packet to a group address */
        s = aodv_seeklist_find(&seek_entry->dest_addr);
        if (s) {
          aodv_seeklist_remove(s);
        }
        return (1);
      }
      
      /* have to become leader ourselves */
      if ((group_rec->flags & AODV_MRT_BROKEN) && 
          !(group_rec->flags & AODV_MRT_MEMBER)) {
          
        group_rec->flags &= ~AODV_MRT_BROKEN;
        aodv_mact_make_downstream_leader(group_rec);
        
      } else {
        if (group_rec->flags & AODV_MRT_BROKEN) {
          group_rec->flags &= ~AODV_MRT_BROKEN;
        }
        aodv_mrt_become_leader(group_rec);
      }
    }
    
    dest_addr = seek_entry->dest_addr;
    
    s = aodv_seeklist_find(&seek_entry->dest_addr);
    if (s) {
      if (s->p) {
        ip4_addr_t grp_ip;
        ip4_addr_t rev_ip;
        grp_ip.addr = dest_addr.s_addr;
        rev_ip.addr = s->rev_addr.s_addr;
        AODV_MPACKET_OUTPUT(group_rec->netif, s->p, &grp_ip, &rev_ip);
      }
      aodv_seeklist_remove(s);
    }
    return (1);
    
  } else if ((seek_entry->flags & RREQ_JOIN) &&
             (seek_entry->flags & RREQ_REPAIR)) {
             
    struct aodv_mrtnode *mrt = aodv_mrt_find(&seek_entry->grp_addr);
    if (mrt) {
      mrt->flags &= ~AODV_MRT_REPAIR;
    }
  }
  
  return (0);
}

/**
 * create a grph packet
 *
 * @param p pbuf save the grph packet
 * @param hcnt hops
 * @param grp_leader leader address
 * @param grp_addr group address
 * @param grp_seqno group seq number
 * @return grph packet
 */
struct pbuf *aodv_grph_create (u8_t flags, u8_t hcnt, struct in_addr *grp_leader, struct in_addr *grp_addr, u32_t grp_seqno)
{
  struct pbuf *p;
  struct aodv_grph *grph;
  
  p = pbuf_alloc(PBUF_TRANSPORT, GRPH_SIZE, PBUF_RAM);
  if (p) {
    grph = (struct aodv_grph *)p->payload;
    grph->type = AODV_GRPH;
    grph->flags = flags;
    grph->rsvd = 0;
    grph->hcnt = hcnt;
    grph->grp_leader = grp_leader->s_addr;
    grph->grp_addr = grp_addr->s_addr;
    grph->grp_seqno = htonl(grp_seqno);
  } else {
    LWIP_DEBUGF(AODV_DEBUG, ("aodv_grph_create: can not allocate grph packet!\n"));
  }
  
  return p;
}

/**
 * send a grph packet
 *
 * @param p pbuf save the grph packet
 * @param grp_addr group address
 * @param grp_seqno group seq number
 */
void aodv_grph_send (u8_t flags, u8_t hcnt, struct in_addr *grp_addr, u32_t grp_seqno)
{
  int i;
  struct pbuf *p;
  struct in_addr addr;
  struct in_addr dest_broadcast;
  u8_t ttl = NET_DIAMETER; /* all net must can recv */
  
  dest_broadcast.s_addr = INADDR_BROADCAST; /* broadcast */
  
  for (i = 0; i < AODV_MAX_NETIF; i++) {
    if (aodv_netif[i]) {
      addr.s_addr = netif_ip4_addr(aodv_netif[i])->addr;
      p = aodv_grph_create(flags, hcnt, &addr, grp_addr, grp_seqno);
      if (p) {
        aodv_udp_sendto(p, &dest_broadcast, ttl, i);
        pbuf_free(p);
      }
    }
  }
}

/**
 * forward a grph packet
 *
 * @param p pbuf save the grph packet
 * @param ttl ip packet time to live
 */
void aodv_grph_forward (struct pbuf *p, u8_t ttl)
{
  int i;
  struct aodv_grph *grph;
  struct in_addr dest_broadcast;
  
  LWIP_ERROR("p != NULL", (p != NULL), return;);
  LWIP_ERROR("p->len >= GRPH_SIZE", (p->len >= GRPH_SIZE), return;);
  
  grph = (struct aodv_grph *)p->payload;
  grph->hcnt++;
  
  dest_broadcast.s_addr = INADDR_BROADCAST; /* broadcast */
  
  for (i = 0; i < AODV_MAX_NETIF; i++) {
    if (aodv_netif[i]) {
      aodv_udp_sendto(p, &dest_broadcast, ttl, i);
    }
  }
}

/**
 * forward a grph packet (unicast)
 *
 * @param mrt multicase route entry node
 * @param p pbuf save the grph packet
 * @param ttl ip packet time to live
 */
void aodv_grph_forward_ucast (struct aodv_mrtnode *mrt, struct pbuf *p, u8_t ttl)
{
  struct aodv_grph *grph;
  struct aodv_mrtnexthop *upstream;
  struct aodv_rtnode *rt;
  
  LWIP_ERROR("p != NULL", (p != NULL), return;);
  LWIP_ERROR("p->len >= GRPH_SIZE", (p->len >= GRPH_SIZE), return;);
  LWIP_ERROR("mrt != NULL", (mrt != NULL), return;);
  
  grph = (struct aodv_grph *)p->payload;
  
  upstream = aodv_mrt_get_activated_upstream(mrt);
  
  if (upstream != NULL) {
    rt = aodv_rt_find(&upstream->addr);
    if (rt && (rt->state == AODV_VALID)) {
      grph->hcnt++;
      aodv_udp_sendto(p, &upstream->addr, ttl, aodv_rt_netif_index_get(rt));
    }
  }
}

/**
 * process a grph packet
 *
 * @param p pbuf save the grph packet
 * @param src_addr source address
 * @param ttl ip packet time to live
 * @param netif packet input netif
 */
void aodv_grph_process (struct pbuf *p, struct in_addr *src_addr, u8_t ttl, struct netif *netif)
{
#if AODV_DEBUG
  char buffer1[INET_ADDRSTRLEN];
  char buffer2[INET_ADDRSTRLEN];
#endif

  struct aodv_grph *grph;
  struct aodv_mrtnode *mrt;
  struct aodv_mrtnexthop *next_hop;
  struct in_addr grp_addr;
  struct in_addr grp_leader;
  u32_t grp_seqno;
  u8_t do_forward = 0, do_forward_ucast = 0;
  u8_t new_hcnt;
  
  LWIP_ERROR("p != NULL", (p != NULL), return;);
  LWIP_ERROR("p->len >= GRPH_SIZE", (p->len >= GRPH_SIZE), return;);
  LWIP_ERROR("p->len == p->tot_len", (p->len == p->tot_len), return;); /* must in a no splite pbuf */
  LWIP_ERROR("src_addr != NULL", (src_addr != NULL), return;);
  
  grph = (struct aodv_grph *)p->payload;
  
  grp_addr.s_addr = grph->grp_addr;
  grp_leader.s_addr = grph->grp_leader;
  grp_seqno = ntohl(grph->grp_seqno);
  new_hcnt = (u8_t)(grph->hcnt + 1);
  
  mrt = aodv_mrt_find(&grp_addr);
  if (mrt == NULL) {
    mrt = aodv_mrt_insert(&grp_addr, &grp_leader, new_hcnt, grp_seqno, 
                          (GROUP_HELLO_INTERVAL * ALLOWED_HELLO_LOSS), 0, netif);
    LWIP_ERROR("mrt != NULL", (mrt != NULL), return;);
    do_forward = 1;
  }
  
  next_hop = aodv_mrt_get_best_upstream(mrt);
  if (next_hop && 
      (next_hop->flags & AODV_MRT_NEXTHOP_ACT) && 
      (next_hop->addr.s_addr == src_addr->s_addr)) { /* only best upstream's grp hello */
    aodv_mrt_update_timeout(mrt, GROUP_HELLO_INTERVAL * ALLOWED_HELLO_LOSS); /* update timeout */
  }
  
  if (mrt->flags & AODV_MRT_ROUTER) {
    struct aodv_mrtnexthop *upstream = aodv_mrt_get_activated_upstream(mrt);
    /* group leader doens't need to process grph */
    /* nor do I care about offmtree packets if I'm on the tree */
    if (mrt->flags & AODV_MRT_LEADER) {
      if (mrt->grp_leader.s_addr != grp_leader.s_addr) {
        /* initiate tree merge */
        /* Check IP's */
        if ((mrt->grp_leader.s_addr < grp_leader.s_addr) &&
            (!(mrt->flags & AODV_MRT_REPAIR))) {
          /* my IP is less than his ;( */
          /* initiate route discovery with all the flags */
          LWIP_DEBUGF(AODV_DEBUG, ("aodv_grph_process: give up leader %s of group %s\n", 
                      inet_ntoa_r(mrt->grp_leader, buffer1, INET_ADDRSTRLEN),
                      inet_ntoa_r(mrt->grp_addr, buffer2, INET_ADDRSTRLEN)));
          
          mrt->flags |= AODV_MRT_REPAIR;
          aodv_rreq_route_discovery(&grp_leader, 
                        RREQ_JOIN | RREQ_REPAIR | RREQ_DEST_ONLY,
                        NULL, &mrt->grp_addr, NULL);
        }
      }
    } else if (!(grph->flags & GRPH_OFFMTREE)) {
      if ((upstream != NULL) && (src_addr->s_addr == upstream->addr.s_addr)) {
        mrt->leader_hcnt = new_hcnt;
        mrt->grp_seqno = grp_seqno;
        
        if ((grph->flags & GRPH_UPDATE) || (mrt->grp_leader.s_addr != grp_leader.s_addr)) {
          aodv_mrt_set_leader(mrt, &grp_leader, new_hcnt);
        }
        do_forward = 1;
      }
    } 
    
    if (!(mrt->flags & AODV_MRT_LEADER) && (mrt->grp_leader.s_addr != grp_leader.s_addr)) {
      /* if you get a grph for a different leader.... */
      /* unicast it to your upstream */
      do_forward_ucast = 1;
    }
  } else {
    /* if not on the tree, you can be less discriminating about 
       your information */
    grph->flags |= GRPH_OFFMTREE;
    
    /* potential method for forwarding grph */
    if ((grp_seqno > mrt->grp_seqno) || 
        ((grp_seqno == mrt->grp_seqno) && (new_hcnt < mrt->leader_hcnt))) {
      do_forward = 1;
    }
    
    /* and finally update our sequence number */
    mrt->grp_seqno = grp_seqno;
  }
  
  if (do_forward && (ttl > 1)) {
    ttl--;
    aodv_grph_forward(p, ttl);
  }
  
  if (do_forward_ucast && (ttl > 1)) {
    ttl--;
    aodv_grph_forward_ucast(mrt, p, ttl);
  }
  
  if (aodv_gw_get() && !(mrt->flags & AODV_MRT_ROUTER)) {
    /* In Gateway mode, we want a piece of every group */
    mrt->flags |= AODV_MRT_GATEWAY;
    aodv_rreq_route_discovery(&grp_addr, RREQ_JOIN, NULL, NULL, NULL);
  }
}

/**
 * create a mact packet
 *
 * @param flags mack packet flags
 * @param hcnt The number of hops
 * @param src_addr source address
 * @param grp_addr group IP address
 * @return mact packet
 */
struct pbuf *aodv_mact_create (u8_t flags, u8_t hcnt, struct in_addr *src_addr, struct in_addr *grp_addr)
{
  struct pbuf *p;
  struct aodv_mact *mact;
  u32_t host_seqno;
  
  SEQNO_INC(aodv_host_state.seqno);
  host_seqno = htonl(aodv_host_state.seqno);
  
  p = pbuf_alloc(PBUF_TRANSPORT, MACT_SIZE, PBUF_RAM);
  if (p) {
    mact = (struct aodv_mact *)p->payload;
    mact->type = AODV_MACT;
    mact->flags = flags;
    mact->rsvd = 0;
    mact->hcnt = hcnt;
    mact->grp_addr = grp_addr->s_addr;
    mact->orig_addr = src_addr->s_addr;
    mact->orig_seqno = host_seqno;
  } else {
    LWIP_DEBUGF(AODV_DEBUG, ("aodv_mact_create: can not allocate grph packet!\n"));
  }
  
  return p;
}

/**
 * send a mact packet
 *
 * @param flags mack packet flags
 * @param hcnt The number of hops
 * @param grp_addr group IP address
 * @param dest_addr destination address
 * @param ttl ip packet time to live
 */
void aodv_mact_send (u8_t flags, u8_t hcnt, struct in_addr *grp_addr, struct in_addr *dest_addr, u8_t ttl)
{
  struct pbuf *p;
  struct aodv_rtnode *rt;
  struct in_addr src_addr;
  
  LWIP_ERROR("grp_addr != NULL", (grp_addr != NULL), return;);
  LWIP_ERROR("dest_addr != NULL", (dest_addr != NULL), return;);
  
  rt = aodv_rt_find(dest_addr);
  
  LWIP_ERROR("rt != NULL", (rt != NULL), return;);
  
  src_addr.s_addr = netif_ip4_addr(rt->netif)->addr;
  
  p = aodv_mact_create(flags, hcnt, &src_addr, grp_addr);
  if (p) {
    aodv_udp_sendto(p, dest_addr, ttl, aodv_rt_netif_index_get(rt));
    pbuf_free(p);
  }
}

/**
 * send a mact packet
 *
 * @param flags mack packet flags
 * @param hcnt The number of hops
 * @param grp_addr group IP address
 * @param ttl ip packet time to live
 */
void aodv_mact_mcast (u8_t flags, u8_t hcnt, struct in_addr *grp_addr, u8_t ttl)
{
  struct pbuf *p;
  struct aodv_rtnode *rt;
  struct aodv_mrtnode *mrt;
  struct aodv_mrtnexthop *nexthop;
  struct in_addr src_addr;
  /* prevent sending packet twice on an iface */
  static u8_t ifmask[AODV_MAX_NETIF];
  int i;
  
  LWIP_ERROR("grp_addr != NULL", (grp_addr != NULL), return;);
  
  for (i = 0; i < AODV_MAX_NETIF; i++) {
    ifmask[i] = 0;
  }
  
  mrt = aodv_mrt_find(grp_addr);
  nexthop = mrt->next_hop;
  
  /* an attempt to send multicast packets right.
   * destination is group address
   * only send one per interface you are currently multicasting to group on
   */
  while (nexthop != NULL) {
    rt = aodv_rt_find(&nexthop->addr);
    if (rt) {
      i = aodv_rt_netif_index_get(rt);
      
      if (!ifmask[i]) {
        src_addr.s_addr = netif_ip4_addr(rt->netif)->addr;
        
        p = aodv_mact_create(flags, hcnt, &src_addr, grp_addr);
        if (p) {
          aodv_udp_sendto(p, grp_addr, ttl, aodv_rt_netif_index_get(rt));
          pbuf_free(p);
          ifmask[i] = 1;
        }
      }
    }
    nexthop = nexthop->next;
  }
}

/**
 * process a mact packet
 *
 * @param p pbuf save the mact packet
 */
void aodv_mact_process (struct pbuf *p)
{
#if AODV_DEBUG
  char buffer1[INET_ADDRSTRLEN];
  char buffer2[INET_ADDRSTRLEN];
#endif /* AODV_DEBUG */

  struct aodv_mact *mact;
  struct in_addr src_addr;
  struct in_addr grp_addr;
  u32_t src_seqno;
  struct aodv_mrtnode *mrt;
  
  LWIP_ERROR("p != NULL", (p != NULL), return;);
  LWIP_ERROR("p->len >= MACT_SIZE", (p->len >= MACT_SIZE), return;);
  LWIP_ERROR("p->len == p->tot_len", (p->len == p->tot_len), return;); /* must in a no splite pbuf */
  
  mact = (struct aodv_mact *)p->payload;
  
  src_addr.s_addr = mact->orig_addr;
  grp_addr.s_addr = mact->grp_addr;
  src_seqno = ntohl(mact->orig_seqno);
  
  mrt = aodv_mrt_find(&grp_addr);
  if (!mrt) {
    return;
  }
  
  if (mact->flags & MACT_JOIN) {
    /* sender wants to join specified group with us as next hop.
     * if we are already in group, then fine, else need to
     * pseudo-join ourselves.
     */
    struct aodv_mrtnexthop *nh;
    
    nh = aodv_mrt_nexthop_find(mrt, &src_addr);
    if (!nh) {
      nh = aodv_mrt_nexthop_add(mrt, &src_addr, 0, src_seqno); /* down stream */
      LWIP_ERROR("nh != NULL", (nh != NULL), return;);
    }
    
    if (nh->flags & AODV_MRT_NEXTHOP_ACT) {
      aodv_mrt_nexthop_deactive(mrt, &nh->addr);
    }
    
    nh->flags &= ~AODV_MRT_NEXTHOP_UP; /* down stream */
    
    aodv_mrt_nexthop_active(mrt, &src_addr);
    
    LWIP_DEBUGF(AODV_DEBUG, ("aodv_mact_process: activated downstream %s\n", 
                inet_ntoa_r(src_addr, buffer1, INET_ADDRSTRLEN)));
    
    /* check if we are not leader and yet have no upstream */
    if (!(mrt->flags & AODV_MRT_LEADER) && (mrt->activated_upstream_cnt == 0)) {
      /* not leader and no upstream, so activate one */
      aodv_mact_activate_best_upstream(mrt);
    }
    
  } else if (mact->flags & MACT_PRUNE) {
    struct aodv_mrtnexthop *nexthop;
    
    nexthop = aodv_mrt_get_activated_upstream(mrt);
    
    if ((nexthop != NULL) && (nexthop->addr.s_addr == src_addr.s_addr)) {
      LWIP_DEBUGF(AODV_DEBUG, ("aodv_mact_process: Upstream sent prune %s\n",
                  inet_ntoa_r(src_addr, buffer1, INET_ADDRSTRLEN)));
    
      /* deactivate said upstream in our records */
      aodv_mrt_nexthop_deactive(mrt, &nexthop->addr);
      /* upstream sent us prune */
      /* if member, become leader, else leave and make a downstream */
      if ((mrt->flags & AODV_MRT_MEMBER) || 
           ((mrt->flags & AODV_MRT_GATEWAY) && (mrt->activated_downstream_cnt > 0))) {
        aodv_mrt_become_leader(mrt);
      } else {
        aodv_mact_make_downstream_leader(mrt);
      }
    } else if (!(mrt->flags & AODV_MRT_MEMBER) &&
                (!(mrt->flags & AODV_MRT_GATEWAY) || (mrt->activated_upstream_cnt == 0)) &&
                (mrt->activated_downstream_cnt == 1)) {
      /* if no other downstreams and not listening ourselves, prune self */
      struct aodv_mrtnexthop *upstream;
      
      mrt->flags &= ~AODV_MRT_GATEWAY;
      mrt->flags &= ~AODV_MRT_LEADER;
      
      upstream = aodv_mrt_get_activated_upstream(mrt);
      
      LWIP_DEBUGF(AODV_DEBUG, ("aodv_mact_process: Downstream sent prune, prune self.\n"));
      
      if (upstream) {
        aodv_mact_send(MACT_PRUNE, 1, &grp_addr, &upstream->addr, 1);
        aodv_mrt_nexthop_deactive(mrt, &upstream->addr);
      }
      aodv_mrt_stop_routing(mrt);
    
    } else {
      LWIP_DEBUGF(AODV_DEBUG, ("aodv_mact_process: Downstream sent prune, stay on tree.\n"));
    }
    
    /* remove them */
    aodv_mrt_nexthop_deactive(mrt, &src_addr);
        
  } else if (mact->flags & MACT_GRPLEADR) {
    /* GR/OUP LEADER
     * 
     * my upstream may send it to me.
     * either I become leader or I promote a child
     */
    /* node who sent it has me marked as its upstream now */
    struct aodv_mrtnexthop *node;
    
    node = aodv_mrt_get_activated_upstream(mrt);
    if (node) {
      node->flags &= ~AODV_MRT_NEXTHOP_UP; /* down stream */
      mrt->activated_upstream_cnt--;
      mrt->activated_downstream_cnt++;
    }
    
    if (mrt->flags & AODV_MRT_MEMBER) {
      /* declare myself leader */
      /* TODO: might need to set U flag on first one */
      aodv_mrt_become_leader(mrt);
    } else {
      /* send a notice to a downstream making it leader */
      aodv_mact_make_downstream_leader(mrt);
    }
    
  } else if (mact->flags & MACT_REPORT_UP) {
    /* downstream report upstream. 
     * downstream will periodically send this packet to best upstream, 
     * if 'src_addr' not in upstream's nexthop, 'src_addr' MUST a UNI-DIR node,
     * we can send to 'src_addr' but 'src_addr' can not send to me,
     * so we will notify 'src_addr' Re-Discover the group 
     */
    struct aodv_mrtnexthop *nh;
    
    nh = aodv_mrt_nexthop_find(mrt, &src_addr);
    if (!nh || !(nh->flags & AODV_MRT_NEXTHOP_ACT)) {
      aodv_mact_send(MACT_REDISCOVER, 1, &grp_addr, &src_addr, 1);
    }
  
  } else if (mact->flags & MACT_REDISCOVER) {
    /* upstream report downstream that we need Re-Discover the group */
    if (!(mrt->flags & AODV_MRT_LEADER)) { /* not leader */
    
      if (mrt->flags & AODV_MRT_ROUTER) { /* I'm router */
        aodv_mrt_nexthop_remove_all(mrt);
        aodv_mrt_stop_routing(mrt);
      }
    
      if (mrt->flags & AODV_MRT_MEMBER) { /* I'm member */
        LWIP_DEBUGF(AODV_DEBUG, ("aodv_mact_process: [MACT_REDISCOVER] we are lost group tree, grp:%s leader:%s\n",
                    inet_ntoa_r(mrt->grp_addr, buffer1, INET_ADDRSTRLEN),
                    inet_ntoa_r(mrt->grp_leader, buffer2, INET_ADDRSTRLEN)));
                  
        mrt->flags |= AODV_MRT_BROKEN; /* make broken */
      
        /* we lost our upstream, find a new one */
        aodv_rreq_route_discovery(&mrt->grp_addr, 
                                  (RREQ_JOIN | RREQ_GRP_REBUILD), NULL, NULL, NULL);
      }
    }
  }
}

/**
 * activate best upstream
 *
 * @param mrt route entry node
 */
struct aodv_mrtnexthop *aodv_mact_activate_best_upstream (struct aodv_mrtnode *mrt)
{
  struct aodv_mrtnexthop *besthop;
  struct aodv_seeklist *s;
  
  besthop = aodv_mrt_get_best_upstream(mrt);
  
  /* and activate it */
  if (besthop) {
    aodv_mrt_start_routing(mrt);
    
    aodv_mrt_nexthop_active(mrt, &besthop->addr);
    
    aodv_mact_send(MACT_JOIN, 1, &mrt->grp_addr, &besthop->addr, 1);
    
    s = aodv_seeklist_find(&mrt->grp_addr);
    
    if (s) {
      if (s->p) {
        ip4_addr_t grp_ip;
        ip4_addr_t rev_ip;
        grp_ip.addr = mrt->grp_addr.s_addr;
        rev_ip.addr = s->rev_addr.s_addr;
        AODV_MPACKET_OUTPUT(mrt->netif, s->p, &grp_ip, &rev_ip);
      }
      
      aodv_seeklist_remove(s);
    }
  }
  
  return besthop;
}

/**
 * make downstream leader
 *
 * @param mrt route entry node
 */
void aodv_mact_make_downstream_leader (struct aodv_mrtnode *mrt)
{
  struct aodv_mrtnexthop *downstream;
  struct aodv_seeklist *s;
  
  if (mrt->activated_downstream_cnt == 0) {
    aodv_mrt_stop_routing(mrt);
    mrt->flags &= ~AODV_MRT_GATEWAY;
    return;
  }
  
  downstream = mrt->next_hop;
  while ((downstream->flags & AODV_MRT_NEXTHOP_UP) || 
         !(downstream->flags & AODV_MRT_NEXTHOP_ACT)) {
    downstream = downstream->next;
  }
  
  if (mrt->activated_downstream_cnt == 1) {
    /* one child, make him leader */
    aodv_mact_send(MACT_PRUNE, 0, &mrt->grp_addr, &downstream->addr, 1);
    aodv_mrt_nexthop_deactive(mrt, &downstream->addr);
    aodv_mrt_stop_routing(mrt);
    
    /* remove any pending RREQs (or other packets) for this group */
    
    s = aodv_seeklist_find(&mrt->grp_addr);
    if (s) {
      aodv_seeklist_remove(s);
    }
    
    /* flush all nexthops */
    aodv_mrt_nexthop_remove_all(mrt);
  
  } else {
    /* send a 'G' mact to an arbitrary child, then make it upstream */
    aodv_mact_send(MACT_GRPLEADR, 0, &mrt->grp_addr, &downstream->addr, 1);
    downstream->flags |= AODV_MRT_NEXTHOP_UP;
    mrt->activated_downstream_cnt--;
    mrt->activated_upstream_cnt++;
  }
}
#endif /* AODV_MCAST */

/* end */