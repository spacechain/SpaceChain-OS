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
#include "lwip/netif.h"
#include "lwip/inet.h"

#include "aodv_route.h"
#include "aodv_proto.h"
#include "aodv_hello.h"
#include "aodv_timercb.h"
#include "aodv_if.h"

/**
 * Add/Update neighbor from a non HELLO AODV control message...
 * 
 * @param source source address
 * @param netif packet receive from
 */
void aodv_neighbor_add (struct in_addr *source, struct netif *netif)
{
#if AODV_DEBUG
  char buffer[INET_ADDRSTRLEN];
#endif

  struct aodv_rtnode *rt = aodv_rt_find(source);
  u32_t seqno = 0;
  
  if (!rt) {
    LWIP_DEBUGF(AODV_DEBUG, ("aodv_neighbor_add: %s new NEIGHBOR!\n", 
                              inet_ntoa_r(*source, buffer, INET_ADDRSTRLEN)));
    rt = aodv_rt_insert(source, source, 1, 0,
	                    ACTIVE_ROUTE_TIMEOUT, AODV_VALID, 
	                    AODV_RT_UNIDIR, netif); /* uni-directional link */
	                    
    LWIP_ERROR("rt != NULL", (rt != NULL), return;);
    
  } else {
    /* Don't update anything if this is a uni-directional link... */
    if (rt->flags & AODV_RT_UNIDIR) {
      return;
    }
    if (rt->dest_seqno != 0) {
      seqno = rt->dest_seqno;
    }
    aodv_rt_update(rt, source, 1, seqno, ACTIVE_ROUTE_TIMEOUT,
                   AODV_VALID, rt->flags);
  }
  
  if (__AODV_TIMER_ISINQ(&rt->hello_timer)) {
    aodv_hello_update_timeout(rt, ALLOWED_HELLO_LOSS * HELLO_INTERVAL);
  }
}

/**
 * make a neighbor link break!
 * 
 * @param rt route entry node
 */
void aodv_neighbor_link_break (struct aodv_rtnode *rt)
{
#if AODV_DEBUG
  char buffer[INET_ADDRSTRLEN];
#endif

  /* If hopcount = 1, this is a direct neighbor and a link break has
     occured. Send a RERR with the incremented sequence number */
  int i;
  struct in_addr rerr_unicast_dest;
  struct pbuf *p = NULL;
  struct aodv_rtnode *rt_u;
  int dest_count = 0;
  
  rerr_unicast_dest.s_addr = INADDR_ANY;
  
  LWIP_ERROR("rt != NULL", (rt != NULL), return;);
  
  if (rt->hcnt != 1) {
    LWIP_DEBUGF(AODV_DEBUG, ("aodv_neighbor_link_break: %s is not a neighbor, hcnt=%d!\n", 
                inet_ntoa_r(rt->dest_addr, buffer, INET_ADDRSTRLEN), rt->hcnt));
    return;
  }
  
  LWIP_DEBUGF(AODV_DEBUG, ("aodv_neighbor_link_break: Link %s down!\n", 
              inet_ntoa_r(rt->dest_addr, buffer, INET_ADDRSTRLEN)));
              
  /* Invalidate the entry of the route that broke or timed out... */
  aodv_rt_invalidate(rt);
  
  /* Create a route error msg, unless the route is to be repaired */
  if (rt->nprec && !(rt->flags & AODV_RT_REPAIR)) {
    
    p = aodv_rerr_create(0, &rt->dest_addr, rt->dest_seqno);
    dest_count = 1;
    LWIP_ERROR("p != NULL", (p != NULL), return;);
    
    LWIP_DEBUGF(AODV_DEBUG, ("aodv_neighbor_link_break: Added %s as unreachable, seqno=%lu\n",
                inet_ntoa_r(rt->dest_addr, buffer, INET_ADDRSTRLEN), rt->dest_seqno));
    
    if (rt->nprec == 1) {
      rerr_unicast_dest = rt->precursors->neighbor;
    }
  }
  
  /* Purge precursor list: */
  if (!(rt->flags & AODV_RT_REPAIR)) {
    aodv_precursor_list_destroy(rt);
  }
  
  /* Check the routing table for entries which have the unreachable
     destination (dest) as next hop. These entries (destinations)
     cannot be reached either since dest is down. They should
     therefore also be included in the RERR. */
  for (i = 0; i < AODV_RT_TABLESIZE; i++) {
    
    for (rt_u  = aodv_rt_tbl.hash_tbl[i];
         rt_u != NULL;
         rt_u  = rt_u->next) {
      
      if ((rt_u->state == AODV_VALID) &&
          (rt_u->next_hop.s_addr == rt->dest_addr.s_addr) &&
          (rt_u->dest_addr.s_addr != rt->dest_addr.s_addr)) {
        
        /* If the link that broke are marked for repair,
           then do the same for all additional unreachable
           destinations. */
        if ((rt->flags & AODV_RT_REPAIR) && rt_u->hcnt <= MAX_REPAIR_TTL) {
          rt_u->flags |= AODV_RT_REPAIR;
          LWIP_DEBUGF(AODV_DEBUG, ("aodv_neighbor_link_break: Marking %s for REPAIR!\n",
                      inet_ntoa_r(rt_u->dest_addr, buffer, INET_ADDRSTRLEN)));
          
          aodv_rt_invalidate(rt_u);
          continue;
        }
        
        aodv_rt_invalidate(rt_u);
        
        if (rt_u->nprec) {
          if (p == NULL) {
            p = aodv_rerr_create(0, &rt_u->dest_addr, rt_u->dest_seqno);
            dest_count = 1;
            LWIP_ERROR("p != NULL", (p != NULL), return;);
          }
          
          if (rt_u->nprec == 1) {
            rerr_unicast_dest = rt_u->precursors->neighbor;
            
            LWIP_DEBUGF(AODV_DEBUG, ("aodv_neighbor_link_break: Added %s as unreachable, seqno=%lu\n",
                inet_ntoa_r(rt_u->dest_addr, buffer, INET_ADDRSTRLEN), rt_u->dest_seqno));
    
          } else {
            /* Decide whether new precursors make this a non unicast
               RERR */
            aodv_rerr_add_udest(p, &rt_u->dest_addr, rt_u->dest_seqno);
            dest_count++;
            
            if (rerr_unicast_dest.s_addr != INADDR_ANY) {
              struct aodv_precursor *pr;
              
              for (pr = rt_u->precursors; pr != NULL; pr = pr->next) {
                if (pr->neighbor.s_addr != rerr_unicast_dest.s_addr) {
                  rerr_unicast_dest.s_addr = INADDR_ANY; /* we need broadcast */
                  break;
                }
              }
            }
            
            LWIP_DEBUGF(AODV_DEBUG, ("aodv_neighbor_link_break: Added %s as unreachable, seqno=%lu\n",
                  inet_ntoa_r(rt_u->dest_addr, buffer, INET_ADDRSTRLEN), rt_u->dest_seqno));
          }
        }
        
        aodv_precursor_list_destroy(rt_u);
      }
    }
  }

  if (p) {
    rt_u = aodv_rt_find(&rerr_unicast_dest);
    
    if (rt_u && (dest_count == 1) && rerr_unicast_dest.s_addr) {
      
      aodv_udp_sendto(p, &rerr_unicast_dest, 1, aodv_rt_netif_index_get(rt_u));
    
    } else if (dest_count > 0) {
    
      struct in_addr dest_broadcast;
      
      dest_broadcast.s_addr = INADDR_BROADCAST;
      
      /* FIXME: Should only transmit RERR on those interfaces
       * which have precursor nodes for the broken route */
      for (i = 0; i < AODV_MAX_NETIF; i++) {
        if (aodv_netif[i]) {
          aodv_udp_sendto(p, &dest_broadcast, 1, i);
        }
      }
    }
    
    pbuf_free(p);
  }
}

#if AODV_MCAST
/**
 * make a neighbor link break!
 * 
 * @param rt route entry node
 */
void aodv_mcast_neighbor_link_break (struct aodv_rtnode *rt)
{
#if AODV_DEBUG
  char buffer1[INET_ADDRSTRLEN];
  char buffer2[INET_ADDRSTRLEN];
#endif

  struct aodv_mrtnode *mrt;
  struct aodv_mrtnexthop *nexthop;
  struct in_addr next_addr;
  int i;
  
  /* determine which, if any, groups the broken node is in */
  for (i = 0; i < AODV_MRT_TABLESIZE; i++) {
    mrt = aodv_mrt_tbl.hash_tbl[i];
    while (mrt) {
      nexthop = aodv_mrt_nexthop_find(mrt, &rt->dest_addr);
      /* only care about activated routes */
      if (nexthop && (nexthop->flags & AODV_MRT_NEXTHOP_ACT)) {
        int nh_dir = (nexthop->flags & AODV_MRT_NEXTHOP_UP);
        
        next_addr = nexthop->addr;
        aodv_mrt_nexthop_remove(mrt, &nexthop->addr);
        
        if (nh_dir) { /* upstream */
          LWIP_DEBUGF(AODV_DEBUG, ("aodv_mcast_neighbor_link_break: We lost upstream %s of group %s!\n", 
                      inet_ntoa_r(next_addr, buffer1, INET_ADDRSTRLEN),
                      inet_ntoa_r(mrt->grp_addr, buffer2, INET_ADDRSTRLEN)));
        
          mrt->flags |= AODV_MRT_BROKEN; /* make broken */
          
          /* we lost our upstream, find a new one */
          aodv_rreq_route_discovery(&mrt->grp_addr, 
                                    (RREQ_JOIN | RREQ_GRP_REBUILD), NULL, NULL, NULL);
        } else {
          LWIP_DEBUGF(AODV_DEBUG, ("aodv_mcast_neighbor_link_break: We lost downstream %s of group %s!\n", 
                      inet_ntoa_r(next_addr, buffer1, INET_ADDRSTRLEN),
                      inet_ntoa_r(mrt->grp_addr, buffer2, INET_ADDRSTRLEN)));
        
          /* we lost our downstream, wait to see if he returns */
          aodv_mrt_nexthop_remove(mrt, &next_addr);
          
          if (!(mrt->flags & AODV_MRT_MEMBER) && (mrt->activated_downstream_cnt == 0)) {
            /* set timeout to prune self if downstream doesn't
               * reconnect */
            mrt->prune_timer.handle = aodv_mcast_prune_dangle; /* timeout callback */
            mrt->prune_timer.arg = &mrt;
            
            /* 20 seconds of rebuild time. */
            aodv_timer_set_timeout(&mrt->prune_timer, PRUNE_TIMEOUT);
          }
        }
      } else if (nexthop && !(nexthop->flags & AODV_MRT_NEXTHOP_ACT)) {
        aodv_mrt_nexthop_remove(mrt, &nexthop->addr);
      }
      
      mrt = mrt->next;
    }
  }
}
#endif /* AODV_MCAST */

/* end */