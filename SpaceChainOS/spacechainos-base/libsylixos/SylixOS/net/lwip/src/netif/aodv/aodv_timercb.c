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
 * These are timeout functions which are called when timers expire...
 */
#ifdef SYLIXOS
#define  __SYLIXOS_KERNEL
#endif
 
#include "lwip/opt.h"

#include "aodv_list.h"
#include "aodv_timer.h"
#include "aodv_param.h"
#include "aodv_proto.h"
#include "aodv_route.h"
#include "aodv_if.h"
#include "aodv_mcast.h"
#include "aodv_neighbor.h"
#include "aodv_timercb.h"
#include "aodv_seeklist.h"



/* route discovery timeout
 * 
 * @param arg aodv_seeklist
 */
void aodv_route_discovery_timeout (void *arg)
{
#if AODV_DEBUG
  char buffer[INET_ADDRSTRLEN];
#endif /* AODV_DEBUG */

  struct aodv_seeklist *seek_entry = (struct aodv_seeklist *)arg;
  struct aodv_rtnode *rt, *repair_rt;
  
  LWIP_ERROR("seek_entry != NULL", (seek_entry != NULL), return;);
  
#define TTL_VALUE seek_entry->ttl
  
  if (seek_entry->reqs < RREQ_RETRIES) {
    
    if (aodv_expanding_ring_search_get()) {
      
      if (TTL_VALUE < TTL_THRESHOLD) {
        TTL_VALUE += TTL_INCREMENT;
      } else {
        TTL_VALUE = NET_DIAMETER;
        seek_entry->reqs++;
      }
      
      /* Set a new timer for seeking this destination */
      aodv_timer_set_timeout(&seek_entry->seek_timer,
                             RING_TRAVERSAL_TIME);
    } else {
      seek_entry->reqs++;
      aodv_timer_set_timeout(&seek_entry->seek_timer,
                             seek_entry->reqs * 2 *
                             NET_TRAVERSAL_TIME);
    }
    
    /* AODV should use a binary exponential backoff RREP waiting
       time. */
    LWIP_DEBUGF(AODV_DEBUG, ("aodv_route_discovery_timeout: Seeking %s ttl=%d wait=%d\n",
                              inet_ntoa_r(seek_entry->dest_addr, buffer, INET_ADDRSTRLEN), 
                              TTL_VALUE, 2 * TTL_VALUE * NODE_TRAVERSAL_TIME));
                              
    /* A routing table entry waiting for a RREP should not be expunged
       before 2 * NET_TRAVERSAL_TIME... */
    rt = aodv_rt_find(&seek_entry->dest_addr);
    if (rt && rt->rt_timer.msec < (2 * NET_TRAVERSAL_TIME)) {
      aodv_rt_update_timeout(rt, 2 * NET_TRAVERSAL_TIME);
    }
    
    aodv_rreq_send_ext(&seek_entry->dest_addr, seek_entry->dest_seqno,
                       TTL_VALUE, seek_entry->flags,
                       seek_entry->grp_hcnt, &seek_entry->grp_addr);
                   
  } else { /* seek_entry->reqs >= RREQ_RETRIES */
    LWIP_DEBUGF(AODV_DEBUG, ("aodv_route_discovery_timeout: Seeking %s NO ROUTE FOUND!\n",
                              inet_ntoa_r(seek_entry->dest_addr, buffer, INET_ADDRSTRLEN)));
                              
#if AODV_MCAST
    if (aodv_mcast_rreq_retries(seek_entry)) {
      return;
    }
#endif /* AODV_MCAST */
                 
    repair_rt = aodv_rt_find(&seek_entry->dest_addr);
    
    aodv_seeklist_remove(seek_entry);
    
    /* If this route has been in repair, then we should timeout
       the route at this point. */
    if (repair_rt && (repair_rt->flags & AODV_RT_REPAIR)) {
      LWIP_DEBUGF(AODV_DEBUG, ("aodv_route_discovery_timeout: REPAIR for %s failed!\n",
                              inet_ntoa_r(seek_entry->dest_addr, buffer, INET_ADDRSTRLEN)));
      aodv_local_repair_timeout(repair_rt);
    }
  }
}

/* local repair timeout
 * 
 * @param arg route entry node
 */
void aodv_local_repair_timeout (void *arg)
{
#if AODV_DEBUG
  char buffer1[INET_ADDRSTRLEN];
  char buffer2[INET_ADDRSTRLEN];
#endif /* AODV_DEBUG */

  struct aodv_rtnode *rt = (struct aodv_rtnode *)arg;
  struct in_addr rerr_dest;
  struct pbuf *p;
  
  LWIP_ERROR("rt != NULL", (rt != NULL), return;);
  
  rerr_dest.s_addr = INADDR_BROADCAST;    /* Default destination */
  
  /* Unset the REPAIR flag */
  rt->flags &= ~AODV_RT_REPAIR;
  
  /* Route should already be invalidated. */
  if (rt->nprec) {
    p = aodv_rerr_create(0, &rt->dest_addr, rt->dest_seqno);
    if (p) {
      if (rt->nprec == 1) { /* only one neighbor */
        rerr_dest = rt->precursors->neighbor;
        aodv_udp_sendto(p, &rerr_dest, 1, aodv_rt_netif_index_get(rt));
      
      } else {
        int i;
        
        for (i = 0; i < AODV_MAX_NETIF; i++) { /* send to all aodv netif */
          if (aodv_netif[i]) {
            aodv_udp_sendto(p, &rerr_dest, 1, i);
          }
        }
      }
      pbuf_free(p);
    }
    
    LWIP_DEBUGF(AODV_DEBUG, ("aodv_local_repair_timeout: Sending RERR about %s to %s\n",
                inet_ntoa_r(rt->dest_addr, buffer1, INET_ADDRSTRLEN), 
                inet_ntoa_r(rerr_dest, buffer2, INET_ADDRSTRLEN)));
  }
  
  aodv_precursor_list_destroy(rt);
  
  rt->rt_timer.handle = aodv_route_delete_timeout;
  aodv_timer_set_timeout(&rt->rt_timer, DELETE_PERIOD);
  
  LWIP_DEBUGF(AODV_DEBUG, ("aodv_local_repair_timeout: %s removed in %u msecs.\n",
              inet_ntoa_r(rt->dest_addr, buffer1, INET_ADDRSTRLEN), DELETE_PERIOD));
}

/* route expire timeout
 * 
 * @param arg route entry node
 */
void aodv_route_expire_timeout (void *arg)
{
#if AODV_DEBUG
  char buffer[INET_ADDRSTRLEN];
#endif /* AODV_DEBUG */

  struct aodv_rtnode *rt = (struct aodv_rtnode *)arg;
  
  LWIP_ERROR("rt != NULL", (rt != NULL), return;);
  
  LWIP_DEBUGF(AODV_DEBUG, ("aodv_route_expire_timeout: Route %s DOWN, seqno=%d\n",
          inet_ntoa_r(rt->dest_addr, buffer, INET_ADDRSTRLEN), rt->dest_seqno));
          
  if (rt->hcnt == 1) { /* is a neighbor ? */
    aodv_neighbor_link_break(rt);
#if AODV_MCAST
    aodv_mcast_neighbor_link_break(rt);
#endif
    
  } else {
    aodv_rt_invalidate(rt);
    aodv_precursor_list_destroy(rt);
  }
}

/* route delete timeout
 * 
 * @param arg route entry node
 */
void aodv_route_delete_timeout (void *arg)
{
#if AODV_DEBUG
  char buffer[INET_ADDRSTRLEN];
#endif /* AODV_DEBUG */

  struct aodv_rtnode *rt = (struct aodv_rtnode *)arg;
  
  LWIP_ERROR("rt != NULL", (rt != NULL), return;);
  
  LWIP_DEBUGF(AODV_DEBUG, ("aodv_route_delete_timeout: Route %s DELETE\n",
          inet_ntoa_r(rt->dest_addr, buffer, INET_ADDRSTRLEN)));
          
  aodv_rt_delete(rt);
}

/* route hello timeout
 * 
 * @param arg route entry node
 */
void aodv_hello_timeout (void *arg)
{
#if AODV_DEBUG
  char buffer[INET_ADDRSTRLEN];
#endif /* AODV_DEBUG */

  struct aodv_rtnode *rt = (struct aodv_rtnode *)arg;
  
  LWIP_ERROR("rt != NULL", (rt != NULL), return;);
  
  LWIP_DEBUGF(AODV_DEBUG, ("aodv_hello_timeout: LINK/HELLO FAILURE %s last HELLO: %d\n",
          inet_ntoa_r(rt->dest_addr, buffer, INET_ADDRSTRLEN), rt->last_hello_time));
  
  if ((rt->state == AODV_VALID) && !(rt->flags & AODV_RT_UNIDIR)) {
    /* If the we can repair the route, then mark it to be
       repaired.. */
    if (rt->hcnt <= MAX_REPAIR_TTL) {
      rt->flags |= AODV_RT_REPAIR;
      
      LWIP_DEBUGF(AODV_DEBUG, ("aodv_hello_timeout: Marking %s for REPAIR!\n",
                  inet_ntoa_r(rt->dest_addr, buffer, INET_ADDRSTRLEN)));
    }
    
    aodv_neighbor_link_break(rt);
#if AODV_MCAST
    aodv_mcast_neighbor_link_break(rt);
#endif
  }
}

/* rrep ack timeout
 * 
 * @param arg route entry node
 */
void aodv_rrep_ack_timeout (void *arg)
{
#if AODV_DEBUG
  char buffer[INET_ADDRSTRLEN];
#endif /* AODV_DEBUG */

  struct aodv_rtnode *rt = (struct aodv_rtnode *)arg;
  
  LWIP_ERROR("rt != NULL", (rt != NULL), return;);
  
  /* When a RREP transmission fails (i.e. lack of RREP-ACK), add to
     blacklist set... */
  aodv_rreq_blacklist_insert(&rt->dest_addr);
  
  LWIP_DEBUGF(AODV_DEBUG, ("aodv_rrep_ack_timeout: Marking %s to blacklist!\n",
                  inet_ntoa_r(rt->dest_addr, buffer, INET_ADDRSTRLEN)));
}

/* wait on reboot timeout
 * 
 * @param arg reboot flag
 */
void aodv_wait_on_reboot_timeout (void *arg)
{
  int i;

#ifdef SYLIXOS
  KN_SMP_MB();
#endif
  *((int *)arg) = 0;
  for (i = 0; i < AODV_MAX_NETIF; i++) {
    if (aodv_netif[i]) {
      netif_set_link_up(aodv_netif[i]); /* net link up */
    }
  }
#ifdef SYLIXOS
  KN_SMP_MB();
#endif

  LWIP_DEBUGF(AODV_DEBUG, ("aodv_wait_on_reboot_timeout: Wait on reboot over.\n"));
}

#if AODV_MCAST
/* group leader send group hello timeout
 * 
 * @param mrt multicase route entry node
 */
void aodv_grph_timeout (void *arg)
{
  u8_t flags = 0;
  struct aodv_mrtnode *mrt = (struct aodv_mrtnode *)arg;

  LWIP_ERROR("mrt != NULL", (mrt != NULL), return;);
  
  if (mrt->grph_update_cnt > 0) {
    flags = GRPH_UPDATE;
    mrt->grph_update_cnt--;
  }
  
  if (mrt->flags & AODV_MRT_LEADER) {
    if (mrt->grp_seqno == 0) {
      mrt->grp_seqno++;
    } else {
      SEQNO_INC(mrt->grp_seqno);
    }
    aodv_grph_send(flags, 0, &mrt->grp_addr, mrt->grp_seqno);
    aodv_timer_set_timeout(&mrt->grph_timer, GROUP_HELLO_INTERVAL);
  }
}

/* multicase route entry node expire timeout
 * 
 * @param mrt multicase route entry node
 */
void aodv_mcast_expire_timeout (void *arg)
{
#if AODV_DEBUG
  char buffer1[INET_ADDRSTRLEN];
  char buffer2[INET_ADDRSTRLEN];
#endif /* AODV_DEBUG */

  struct aodv_mrtnode *mrt = (struct aodv_mrtnode *)arg;

  LWIP_ERROR("mrt != NULL", (mrt != NULL), return;);
  
  /* we are lost group tree */
  if (!(mrt->flags & AODV_MRT_LEADER)) { /* not leader */
    
    if (mrt->flags & AODV_MRT_ROUTER) { /* I'm router */
      aodv_mrt_nexthop_remove_all(mrt);
      aodv_mrt_stop_routing(mrt);
    }
    
    if (mrt->flags & AODV_MRT_MEMBER) { /* I'm member */
      LWIP_DEBUGF(AODV_DEBUG, ("aodv_mcast_expire_timeout: we are lost group tree, grp:%s leader:%s\n",
                  inet_ntoa_r(mrt->grp_addr, buffer1, INET_ADDRSTRLEN),
                  inet_ntoa_r(mrt->grp_leader, buffer2, INET_ADDRSTRLEN)));
                  
      mrt->flags |= AODV_MRT_BROKEN; /* make broken */
      
      /* we lost our upstream, find a new one */
      aodv_rreq_route_discovery(&mrt->grp_addr, 
                                (RREQ_JOIN | RREQ_GRP_REBUILD), NULL, NULL, NULL);
    }
  }
}

/* prune dangle
 * 
 * @param mrt multicase route entry node
 */
void aodv_mcast_prune_dangle (void *arg)
{
  struct aodv_mrtnexthop  *upstream;
  
  struct aodv_mrtnode *mrt = (struct aodv_mrtnode *)arg;

  LWIP_ERROR("mrt != NULL", (mrt != NULL), return;);
  
  if ((mrt->activated_downstream_cnt == 0) && !(mrt->flags & AODV_MRT_MEMBER) &&
      (!(mrt->flags & AODV_MRT_GATEWAY) || (mrt->activated_upstream_cnt == 0))) {
    /* prune me */
    upstream = aodv_mrt_get_activated_upstream(mrt); 
    if (upstream) {
      aodv_mact_send(MACT_PRUNE, 1, &mrt->grp_addr, &upstream->addr, 1);
      aodv_mrt_nexthop_deactive(mrt, &upstream->addr);
    }
    aodv_mrt_stop_routing(mrt);
  }
}

/* report to upstream
 * 
 * @param mrt multicase route entry node
 */
void aodv_mcast_report_up (void *arg)
{
#define GRP_REPORT_UP_DELAY     1000    /* The extra time we should allow an report up
                                           message to take (due to processing) before
                                           assuming lost . */
#define GRP_REPORT_UP_RAND()    (LWIP_RAND() % GRP_REPORT_UP_DELAY)

  struct aodv_mrtnexthop  *upstream;

  struct aodv_mrtnode *mrt = (struct aodv_mrtnode *)arg;
  
  LWIP_ERROR("mrt != NULL", (mrt != NULL), return;);
  
  /* we are lost group tree */
  if (!(mrt->flags & AODV_MRT_LEADER)) { /* not leader */
    
    if ((mrt->flags & AODV_MRT_ROUTER) ||
        (mrt->flags & AODV_MRT_MEMBER)) { /* I'm router or member */
      
      upstream = aodv_mrt_get_best_upstream(mrt); 
      if (upstream && (upstream->flags & AODV_MRT_NEXTHOP_ACT)) {
        /* we should report upstream */
        aodv_mact_send(MACT_REPORT_UP, 1,  &mrt->grp_addr, &upstream->addr, 1);
      
      } else if (mrt->flags & AODV_MRT_BROKEN) {
        /* discovery will test if this 'join' is in seeklist, this discovery will be discard */
        aodv_rreq_route_discovery(&mrt->grp_addr, 
                                  (RREQ_JOIN | RREQ_GRP_REBUILD), NULL, NULL, NULL);
      }
    }
  }
  
  aodv_timer_set_timeout(&mrt->report_up_timer, GROUP_HELLO_INTERVAL + GRP_REPORT_UP_RAND());
}

#endif /* AODV_MCAST */

/* end */
