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
 
#include "aodv_list.h"
#include "aodv_route.h"
#include "aodv_param.h"
#include "aodv_timercb.h"
#include "aodv_neighbor.h"
#include "aodv_proto.h"
#include "aodv_if.h"
#include "aodv_seeklist.h"

#include <stddef.h>
#include <string.h>

struct in_addr aodv_addr_any = {INADDR_ANY}; /* aodv INADDR_ANY */

struct aodv_rt aodv_rt_tbl; /* aodv global routing table */

#if AODV_MCAST
struct aodv_mrt aodv_mrt_tbl; /* aodv multicast route table */
#endif /* AODV_MCAST */

/**
 * calculate hash index
 *
 * @param addr the calculate parameter.
 * @return hash index
 */
static int hash_index (struct in_addr *addr)
{
  /* use local byte order */
  return (ntohl(addr->s_addr) & AODV_RT_TABLEMASK);
}

/**
 * initialize global routing table
 */
void aodv_rt_init (void)
{
  int i;
  aodv_rt_tbl.num_entries = 0;
  aodv_rt_tbl.num_active = 0;
  for (i = 0; i < AODV_RT_TABLESIZE; i++) {
    aodv_rt_tbl.hash_tbl[i] = NULL;
  }
}

/**
 * get rt netif index of aodv_netif[]
 *
 * @param rt route entry node
 * @return netif index of aodv_netif[], default is 0
 */
int aodv_rt_netif_index_get (struct aodv_rtnode *rt)
{
  int i;
  
  LWIP_ERROR("rt != NULL", (rt != NULL), return 0;);
  
  for (i = 0; i < AODV_MAX_NETIF; i++) {
    if (aodv_netif[i] == rt->netif) {
      return (i);
    }
  }
  
  return (0);
}

/**
 * insert a routing node
 *
 * @return 1 eliminate node count
 */
static u8_t aodv_rt_eliminate (void)
{
  int i;
  struct aodv_rtnode *rt;
  
  struct aodv_rtnode *rt_del = NULL;
  struct aodv_rtnode *rt_repair = NULL;
  struct aodv_rtnode *rt_val = NULL;
  struct aodv_rtnode *rt_eli = NULL;
  
  u32_t msec_del_max = (u32_t)~0; /* 32bits unsigned max */
  u32_t msec_repair_max = (u32_t)~0;
  u32_t msec_val_max = (u32_t)~0;
  
  for (i = 0; i < AODV_RT_TABLESIZE; i++) {
    for (rt = aodv_rt_tbl.hash_tbl[i]; rt != NULL; rt = rt->next) {
      if ((rt->rt_timer.handle == aodv_route_delete_timeout) &&
          (rt->rt_timer.msec < msec_del_max)) { /* find oldest deleting rt node */
        msec_del_max = rt->rt_timer.msec;
        rt_del = rt;
        
      } else if ((rt->rt_timer.handle == aodv_local_repair_timeout) &&
                 (rt->rt_timer.msec < msec_repair_max) &&
                 (rt_del == NULL)) { /* find oldest repairing rt node */
        msec_repair_max = rt->rt_timer.msec;
        rt_repair = rt;
      
      } else if (rt_del == NULL) { /* find oldest valid rt node */
        if (rt->rt_timer.msec < msec_val_max) {
          msec_val_max = rt->rt_timer.msec;
          rt_val = rt;
        }
      }
    }
  }
  
  if (rt_del) {
    rt_eli = rt_del;
  
  } else if (rt_repair) {
    rt_eli = rt_repair;
  
  } else if (rt_val) {
    rt_eli = rt_val;
  }
  
  if (rt_eli) {
    struct pbuf *p = aodv_rerr_create(0, &rt_eli->dest_addr, rt_eli->dest_seqno);
    if (p) {
      struct in_addr rerr_dest;
      
      if (rt_eli->nprec == 1) { /* only one neighbor */
        rerr_dest = rt_eli->precursors->neighbor;
        aodv_udp_sendto(p, &rerr_dest, 1, aodv_rt_netif_index_get(rt_eli));
      
      } else {
        int i;
        rerr_dest.s_addr = INADDR_BROADCAST;    /* Default destination */
        for (i = 0; i < AODV_MAX_NETIF; i++) { /* send to all aodv netif */
          if (aodv_netif[i]) {
            aodv_udp_sendto(p, &rerr_dest, 1, i);
          }
        }
      }
      pbuf_free(p);
    }
    aodv_rt_delete(rt_eli);
    return (1);
  }
  
  return (0);
}

/**
 * insert a routing node
 *
 * @param dest_addr address of the destination.
 * @param next address of the next hop to the destination
 * @param hops distance (in hops) to the destination
 * @param seqno destination sequence number
 * @param life 
 * @param state the state of this node entry
 * @param flags routing flags
 * @param netif net interface
 * @return route entry node
 */
struct aodv_rtnode *aodv_rt_insert (struct in_addr *dest_addr, struct in_addr *next,
                                    u8_t hops, u32_t seqno, u32_t life, u8_t state,
                                    u16_t flags, struct netif *netif)
{
#if AODV_DEBUG
  char buffer1[INET_ADDRSTRLEN];
  char buffer2[INET_ADDRSTRLEN];
#endif /* AODV_DEBUG */
  
  int hash = hash_index(dest_addr);
  struct aodv_rtnode *rt;
  
  LWIP_ERROR("dest_addr != NULL", (dest_addr != NULL), return NULL;);
  LWIP_ERROR("next != NULL", (next != NULL), return NULL;);
  LWIP_ERROR("netif != NULL", (netif != NULL), return NULL;);
  
  /* Check if we already have an entry for dest_addr */
  for (rt = aodv_rt_tbl.hash_tbl[hash]; rt != NULL; rt = rt->next) {
    if (rt->dest_addr.s_addr == dest_addr->s_addr) {
      LWIP_DEBUGF(AODV_DEBUG, ("aodv_rt_insert: %s already exist in routing table!\n", 
                  inet_ntoa_r(*dest_addr, buffer1, INET_ADDRSTRLEN)));
      return NULL;
    }
  }
  
  rt = (struct aodv_rtnode *)mem_malloc(sizeof(struct aodv_rtnode));
  if (rt == NULL) { /* ENOMEM */
    LWIP_DEBUGF(AODV_DEBUG, ("aodv_rt_insert: Try to eliminate a rt node!\n"));
    aodv_rt_eliminate(); /* try to eliminate a rt node */
    rt = (struct aodv_rtnode *)mem_malloc(sizeof(struct aodv_rtnode));
  }
  
  LWIP_ERROR("rt != NULL", (rt != NULL), return NULL;);
  
  memset(rt, 0, sizeof(struct aodv_rtnode));
  
  rt->netif = netif;
  rt->dest_addr = *dest_addr;
  rt->next_hop = *next;
  rt->dest_seqno = seqno;
  rt->flags = flags;
  rt->state = state;
  rt->hcnt = hops;
  
  aodv_timer_init(&rt->rt_timer, aodv_route_expire_timeout, rt);
  aodv_timer_init(&rt->ack_timer, aodv_rrep_ack_timeout, rt);
  aodv_timer_init(&rt->hello_timer, aodv_hello_timeout, rt);
  
  rt->last_hello_time = 0;
  rt->hello_cnt = 0;
  
  rt->nprec = 0;
  rt->precursors = NULL;
  
  /* Insert into rt list */
  aodv_rt_tbl.num_entries++;
  LWIP_DEBUGF(AODV_DEBUG, ("aodv_rt_insert: Inserting %s in routing table, next hop is %s!\n", 
                            inet_ntoa_r(*dest_addr, buffer1, INET_ADDRSTRLEN), 
                            inet_ntoa_r(*next, buffer2, INET_ADDRSTRLEN)));
  
  __AODV_LIST_INSERT(rt, aodv_rt_tbl.hash_tbl[hash]);

  if (state == AODV_INVALID) {
    if (flags & AODV_RT_REPAIR) {
      rt->rt_timer.handle = aodv_local_repair_timeout;
      life = ACTIVE_ROUTE_TIMEOUT;
    } else {
      rt->rt_timer.handle = aodv_route_delete_timeout;
      life = DELETE_PERIOD;
    }
  } else {
    aodv_rt_tbl.num_active++;
  }
  
  if (rt->flags & AODV_RT_GATEWAY) {
    aodv_rt_update_inet_rt(rt, life);
  }
  
  if (life != 0) {
    aodv_timer_set_timeout(&rt->rt_timer, life);
  }
  
  /* In case there are buffered packets for this destination, we
   * send them on the new route. */
  if ((rt->state == AODV_VALID) && !(rt->flags & AODV_RT_UNIDIR)) {
    struct aodv_seeklist *s = aodv_seeklist_find(dest_addr);
    
    if (s) {
      if (s->p) {
        ip4_addr_t ipaddr_next;
        ipaddr_next.addr = rt->next_hop.s_addr;
        AODV_PACKET_OUTPUT(netif, s->p, &ipaddr_next);
      }
#if AODV_MCAST
      if ((s->flags & RREQ_JOIN) &&
          (s->flags & RREQ_REPAIR)) {
        /* repairing group leader, do not remove */
      } else {
        aodv_seeklist_remove(s);
      }
#else
      aodv_seeklist_remove(s);
#endif /* AODV_MCAST */
    }
  }
  
  return (rt);
}

/**
 * update a routing node
 *
 * @param rt rt route entry node
 * @param next address of the next hop to the destination
 * @param hops distance (in hops) to the destination
 * @param seqno destination sequence number
 * @param lifetime 
 * @param state the state of this node entry
 * @param flags routing flags
 * @return route entry node
 */
struct aodv_rtnode *aodv_rt_update (struct aodv_rtnode *rt, struct in_addr *next,
                                    u8_t hops, u32_t seqno,
                                    u32_t lifetime, u8_t state,
                                    u16_t flags)
{
#if AODV_DEBUG
  char buffer1[INET_ADDRSTRLEN];
  char buffer2[INET_ADDRSTRLEN];
#endif /* AODV_DEBUG */

  if ((rt->state == AODV_INVALID) && (state == AODV_VALID)) { /* make valid! */
    if (rt->flags & AODV_RT_REPAIR) {
      flags &= ~AODV_RT_REPAIR;
    }
  
  } else if ((rt->next_hop.s_addr != INADDR_ANY) &&
             (rt->next_hop.s_addr != next->s_addr)) { /* change next hop */
    LWIP_DEBUGF(AODV_DEBUG, ("aodv_rt_update: rt->next_hop=%s, new_next_hop=%s!\n", 
                            inet_ntoa_r(rt->next_hop, buffer1, INET_ADDRSTRLEN), 
                            inet_ntoa_r(*next, buffer2, INET_ADDRSTRLEN)));
  }
  
  if (hops > 1 && rt->hcnt == 1) {
    rt->last_hello_time = 0;
    rt->hello_cnt = 0;
    aodv_timer_remove(&rt->hello_timer);
    /* Must also do a "link break" when updating a 1 hop
       neighbor in case another routing entry use this as
       next hop... */
    aodv_neighbor_link_break(rt);
  }
  
  if ((rt->state == AODV_INVALID) && (state == AODV_VALID)) {
    aodv_rt_tbl.num_active++;
  }
  
  rt->flags = flags;
  rt->dest_seqno = seqno;
  rt->next_hop = *next;
  rt->hcnt = hops;
  
  if (rt->flags & AODV_RT_GATEWAY) {
    aodv_rt_update_inet_rt(rt, lifetime);
  }
  
  rt->rt_timer.handle = aodv_route_expire_timeout;
  if (!(rt->flags & AODV_RT_INET_DEST)) {
    aodv_rt_update_timeout(rt, lifetime);
  }
  
  /* Finally, mark as VALID */
  rt->state = state;
  
  /* In case there are buffered packets for this destination, we send
   * them on the new route. */
  if ((rt->state == AODV_VALID) && !(rt->flags & AODV_RT_UNIDIR)) {
    struct aodv_seeklist *s = aodv_seeklist_find(&rt->dest_addr);
    
    if (s) {
      if (s->p) {
        ip4_addr_t ipaddr_next;
        ipaddr_next.addr = rt->next_hop.s_addr;
        AODV_PACKET_OUTPUT(rt->netif, s->p, &ipaddr_next);
      }
#if AODV_MCAST
      if ((s->flags & RREQ_JOIN) &&
          (s->flags & RREQ_REPAIR)) {
        /* repairing group leader, do not remove */
      } else {
        aodv_seeklist_remove(s);
      }
#else
      aodv_seeklist_remove(s);
#endif /* AODV_MCAST */
    }
  }

  return (rt);
}

/**
 * update rt_timer timeout
 *
 * @param rt route entry node
 * @param lifetime new lifetime
 * @return route entry node
 */
struct aodv_rtnode *aodv_rt_update_timeout (struct aodv_rtnode *rt, u32_t lifetime)
{
  LWIP_ERROR("rt != NULL", (rt != NULL), return NULL;);
  
  if (rt->state == AODV_VALID) {
    /* Check if the current valid timeout is larger than the new
           one - in that case keep the old one. */
    if (rt->rt_timer.msec < lifetime) {
      aodv_timer_set_timeout(&rt->rt_timer, lifetime);
    }
  } else {
    aodv_timer_set_timeout(&rt->rt_timer, lifetime);
  }
  
  return (rt);
}

/* Update route timeouts in response to an incoming or outgoing data packet. 
 * 
 * @param fwd_rt destination address or subnet
 * @param rev_rt source address
 */
void aodv_rt_update_route_timeouts (struct aodv_rtnode *fwd_rt,
                                    struct aodv_rtnode *rev_rt)
{
  struct aodv_rtnode *next_hop_rt = NULL;
  
  /*
   *  src                                 me                                dest
   *   @    ...        @       <-->       @      <-->       @        ...     @
   *           rev_rt->next_hop                      fwd_rt->next_hop
   */
  
  /* When forwarding a packet, we update the lifetime of the
     destination's routing table entry, as well as the entry for the
     next hop neighbor (if not the same). AODV draft 10, section
     6.2. */
  if (fwd_rt && fwd_rt->state == AODV_VALID) {
  
    if ((fwd_rt->flags & AODV_RT_INET_DEST) ||
        (fwd_rt->hcnt != 1) ||
        (__AODV_TIMER_ISINQ(&fwd_rt->hello_timer))) {
      aodv_rt_update_timeout(fwd_rt, ACTIVE_ROUTE_TIMEOUT);
    }
      
    next_hop_rt = aodv_rt_find(&fwd_rt->next_hop);
      
    if (next_hop_rt && (next_hop_rt->state == AODV_VALID) &&
        (next_hop_rt->dest_addr.s_addr != fwd_rt->dest_addr.s_addr) &&
        (__AODV_TIMER_ISINQ(&fwd_rt->hello_timer))) {
          
      aodv_rt_update_timeout(next_hop_rt, ACTIVE_ROUTE_TIMEOUT);
    }
  }
  
  /* Also update the reverse route and reverse next hop along the
     path back, since routes between originators and the destination
     are expected to be symmetric. */
  if (rev_rt && rev_rt->state == AODV_VALID) {
  
    if ((rev_rt->hcnt != 1) || 
        __AODV_TIMER_ISINQ(&rev_rt->hello_timer)) {
      aodv_rt_update_timeout(rev_rt, ACTIVE_ROUTE_TIMEOUT);
    }
      
    next_hop_rt = aodv_rt_find(&rev_rt->next_hop);
    
    if (next_hop_rt && (next_hop_rt->state == AODV_VALID) &&
        (next_hop_rt->dest_addr.s_addr != rev_rt->dest_addr.s_addr) &&
        (__AODV_TIMER_ISINQ(&rev_rt->hello_timer))) {
        
      aodv_rt_update_timeout(next_hop_rt, ACTIVE_ROUTE_TIMEOUT);
    }
  }
}

/**
 * find a routing node
 *
 * @param dest_addr address of the destination.
 * @return route entry node
 */
struct aodv_rtnode *aodv_rt_find (struct in_addr *dest_addr)
{
  int hash;
  struct aodv_rtnode *find;

  if (aodv_rt_tbl.num_entries == 0) {
    return NULL;
  }
  
  hash = hash_index(dest_addr);
  for (find = aodv_rt_tbl.hash_tbl[hash]; find != NULL; find = find->next) {
    if (find->dest_addr.s_addr == dest_addr->s_addr) {
      return (find);
    }
  }
  
  return NULL;
}

/* find a gateway route entry node
 *
 * @return NULL
 */
struct aodv_rtnode *aodv_rt_find_gateway (void)
{
  struct aodv_rtnode *gw = NULL;
  int i;
  for (i = 0; i < AODV_RT_TABLESIZE; i++) {
    struct aodv_rtnode *rt;
    
    for (rt  = aodv_rt_tbl.hash_tbl[i];
         rt != NULL;
         rt  = rt->next) {
         
      if ((rt->flags & AODV_RT_GATEWAY) && (rt->state == AODV_VALID)) {
        if (!gw || rt->hcnt < gw->hcnt) {
          gw = rt; /* get nearest gateway */
        }
      }
    }
  }
  return (gw);
}

/* update a subnet routing node
 *
 * @return NULL
 */
int aodv_rt_update_inet_rt (struct aodv_rtnode *gw, u32_t life)
{
  int n = 0;
  int i;
  
  LWIP_ERROR("gw != NULL", (gw != NULL), return n;);
  
  for (i = 0; i < AODV_RT_TABLESIZE; i++) {
    struct aodv_rtnode *rt;
    
    for (rt  = aodv_rt_tbl.hash_tbl[i];
         rt != NULL;
         rt  = rt->next) {
    
      if ((rt->flags & AODV_RT_INET_DEST) && (rt->state == AODV_VALID)) {
        aodv_rt_update(rt, &gw->dest_addr, gw->hcnt, 0, life, AODV_VALID, rt->flags);
        n++;
      }
    }
  }

  return n;
}

/**
 * traversal route table
 *
 * @param hook hook function
 */
void aodv_rt_traversal (aodv_rt_hook_handler hook,
                        void *arg0, void *arg1, void *arg2, void *arg3, void *arg4)
{
  int i;
  struct aodv_rtnode *rt;

  for (i = 0; i < AODV_RT_TABLESIZE; i++) {
    
    for (rt  = aodv_rt_tbl.hash_tbl[i];
         rt != NULL;
         rt  = rt->next) {
         
      hook(rt, arg0, arg1, arg2, arg3, arg4);
    }
  }
}

/* Route expiry and Deletion. 
 *
 * @param rt route entry node
 * @return 0: ok -1: an error occur
 */
int aodv_rt_invalidate (struct aodv_rtnode *rt)
{
  LWIP_ERROR("rt != NULL", (rt != NULL), return -1;);
  
  if (rt->state == AODV_INVALID) {
    return -1;
  }
  
  /* Remove any pending, but now obsolete timers. */
  aodv_timer_remove(&rt->rt_timer);
  aodv_timer_remove(&rt->hello_timer);
  aodv_timer_remove(&rt->ack_timer);
  
  /* Mark the route as invalid */
  rt->state = AODV_INVALID;
  rt->hello_cnt = 0;
  aodv_rt_tbl.num_active--;
  
  /* When the lifetime of a route entry expires, increase the sequence
     number for that entry. */
  SEQNO_INC(rt->dest_seqno);
  
  rt->last_hello_time = 0;
  
  /* If this was a gateway, check if any Internet destinations were using
   * it. In that case update them to use a backup gateway or invalide them
   * too. */
  if (rt->flags & AODV_RT_GATEWAY) {
    int i;
    
    for (i = 0; i < AODV_RT_TABLESIZE; i++) {
      struct aodv_rtnode *rt2;
      
      for (rt2  = aodv_rt_tbl.hash_tbl[i];
           rt2 != NULL;
           rt2  = rt2->next) {
           
        if ((rt2->state == AODV_VALID) &&
            (rt2->flags & AODV_RT_INET_DEST) &&
            (rt2->next_hop.s_addr == rt->dest_addr.s_addr)) { /* next hop is this gateway */
          
          aodv_rt_invalidate(rt2);
          aodv_precursor_list_destroy(rt2);
        }
      }
    }
  }
  
  if (rt->flags & AODV_RT_REPAIR) {
    /* Set a timeout for the repair */
    rt->rt_timer.handle = aodv_local_repair_timeout;
    aodv_timer_set_timeout(&rt->rt_timer, ACTIVE_ROUTE_TIMEOUT);
  
  } else {
    /* Schedule a deletion timer */
    rt->rt_timer.handle = aodv_route_delete_timeout;
    aodv_timer_set_timeout(&rt->rt_timer, DELETE_PERIOD);
  }

  return 0;
}

/* delete a rt entry
 * 
 * @param rt route entry node
 */
void aodv_rt_delete (struct aodv_rtnode *rt)
{
  int hash;

  LWIP_ERROR("rt != NULL", (rt != NULL), return;);
  
  hash = hash_index(&rt->dest_addr);
  
  __AODV_LIST_DELETE(rt, aodv_rt_tbl.hash_tbl[hash]);
  
  aodv_precursor_list_destroy(rt);
  
  if (rt->state == AODV_VALID) {
    aodv_rt_tbl.num_active--;
  }
  
  /* Make sure timers are removed... */
  aodv_timer_remove(&rt->rt_timer);
  aodv_timer_remove(&rt->hello_timer);
  aodv_timer_remove(&rt->ack_timer);
  
  aodv_rt_tbl.num_entries--;
  
  mem_free(rt);
}

/* delete a rt entry
 * 
 * @param netif route entry node netif 
 */
void aodv_rt_delete_netif (struct netif *netif)
{
  int i;
    
  for (i = 0; i < AODV_RT_TABLESIZE; i++) {
    struct aodv_rtnode *rt;
    struct aodv_rtnode *rt_del;
    
    rt = aodv_rt_tbl.hash_tbl[i];
    while (rt != NULL) {
      if (rt->netif != netif) {
        rt = rt->next;
      } else {
        rt_del = rt;
        rt = rt->next;
        
        aodv_rt_delete(rt_del);
      }
    }
  }
}

/* Add an neighbor to the active neighbor list. 
 * 
 * @param rt route entry node
 * @param addr neighbor address.
 */
void aodv_precursor_add (struct aodv_rtnode *rt, struct in_addr *addr)
{
#if AODV_DEBUG
  char buffer1[INET_ADDRSTRLEN];
  char buffer2[INET_ADDRSTRLEN];
#endif /* AODV_DEBUG */

  struct aodv_precursor *precursors;

  LWIP_ERROR("rt != NULL", (rt != NULL), return;);
  LWIP_ERROR("addr != NULL", (addr != NULL), return;);
  
  for (precursors  = rt->precursors; 
       precursors != NULL; 
       precursors  = precursors->next) {
    if (precursors->neighbor.s_addr == addr->s_addr) {
      return;
    }
  }
  
  precursors = (struct aodv_precursor *)mem_malloc(sizeof(struct aodv_precursor));
  
  LWIP_ERROR("precursors != NULL", (precursors != NULL), return;);
  
  LWIP_DEBUGF(AODV_DEBUG, ("aodv_precursor_add: Adding precursor %s to rte %s!\n", 
                            inet_ntoa_r(*addr, buffer1, INET_ADDRSTRLEN), 
                            inet_ntoa_r(rt->dest_addr, buffer2, INET_ADDRSTRLEN)));
                            
  precursors->neighbor.s_addr = addr->s_addr;
  
  __AODV_LIST_INSERT(precursors, rt->precursors);
  rt->nprec++;
}

/* Remove a neighbor from the active neighbor list.
 * 
 * @param rt route entry node
 * @param addr neighbor address.
 */
void aodv_precursor_remove (struct aodv_rtnode *rt, struct in_addr *addr)
{
#if AODV_DEBUG
  char buffer1[INET_ADDRSTRLEN];
  char buffer2[INET_ADDRSTRLEN];
#endif /* AODV_DEBUG */

  struct aodv_precursor *precursors;

  LWIP_ERROR("rt != NULL", (rt != NULL), return;);
  LWIP_ERROR("addr != NULL", (addr != NULL), return;);
  
  for (precursors  = rt->precursors; 
       precursors != NULL; 
       precursors  = precursors->next) {
    
    if (precursors->neighbor.s_addr == addr->s_addr) {
      __AODV_LIST_DELETE(precursors, rt->precursors);
      rt->nprec--;
      mem_free(precursors);
      
      LWIP_DEBUGF(AODV_DEBUG, ("aodv_precursor_remove: Removing precursor %s from rte %s!\n", 
                            inet_ntoa_r(*addr, buffer1, INET_ADDRSTRLEN), 
                            inet_ntoa_r(rt->dest_addr, buffer2, INET_ADDRSTRLEN)));
      return;
    }
  }
}

/* Delete all entries from the active neighbor list. 
 *
 * @param rt route entry node
 */
void aodv_precursor_list_destroy (struct aodv_rtnode *rt)
{
  struct aodv_precursor *precursors;
  
  LWIP_ERROR("rt != NULL", (rt != NULL), return;);
  
  while (rt->precursors) {
    precursors = rt->precursors;
    __AODV_LIST_DELETE(precursors, rt->precursors);
    rt->nprec--;
    mem_free(precursors);
  }
}

#if AODV_MCAST
/**
 * initialize multicast routing table
 */
void aodv_mrt_init (void)
{
  int i;
  aodv_mrt_tbl.num_entries = 0;
  for (i = 0; i < AODV_MRT_TABLESIZE; i++) {
    aodv_mrt_tbl.hash_tbl[i] = NULL;
  }
}

/**
 * get mrt netif index of aodv_netif[]
 *
 * @param mrt route entry node
 * @return netif index of aodv_netif[], default is 0
 */
int aodv_mrt_netif_index_get (struct aodv_mrtnode *mrt)
{
  int i;
  
  LWIP_ERROR("mrt != NULL", (mrt != NULL), return 0;);
  
  for (i = 0; i < AODV_MAX_NETIF; i++) {
    if (aodv_netif[i] == mrt->netif) {
      return (i);
    }
  }
  
  return (0);
}

/**
 * insert a multicase routing node
 *
 * @param grp_addr address of the group IP.
 * @param grp_leader address of the group leader IP
 * @param hops distance (in hops) to the group leader
 * @param grp_seqno group sequence number
 * @param life lifetime
 * @param flags multicase routing flags
 * @param netif net interface
 * @return multicase route entry node
 */
struct aodv_mrtnode *aodv_mrt_insert (struct in_addr *grp_addr, struct in_addr *grp_leader,
                                      u8_t hops, u32_t grp_seqno, u32_t life, u16_t flags, struct netif *netif)
{
#if AODV_DEBUG
  char buffer1[INET_ADDRSTRLEN];
  char buffer2[INET_ADDRSTRLEN];
#endif /* AODV_DEBUG */
  int hash;
  struct aodv_mrtnode *mrt;
  
  LWIP_ERROR("grp_addr != NULL", (grp_addr != NULL), return NULL;);
  LWIP_ERROR("netif != NULL", (netif != NULL), return NULL;);
  
  if (!grp_leader) {
    grp_leader = &aodv_addr_any;
  }
  
  hash = hash_index(grp_addr);
  
  /* Check if we already have an entry for grp_addr */
  for (mrt = aodv_mrt_tbl.hash_tbl[hash]; mrt != NULL; mrt = mrt->next) {
    if (mrt->grp_addr.s_addr == grp_addr->s_addr) {
      LWIP_DEBUGF(AODV_DEBUG, ("aodv_mrt_insert: %s already exist in routing table!\n", 
                  inet_ntoa_r(*grp_addr, buffer1, INET_ADDRSTRLEN)));
      return NULL;
    }
  }
  
  mrt = (struct aodv_mrtnode *)mem_malloc(sizeof(struct aodv_mrtnode));
  
  LWIP_ERROR("mrt != NULL", (mrt != NULL), return NULL;);
  
  memset(mrt, 0, sizeof(struct aodv_mrtnode));
  
  mrt->netif = netif;
  mrt->grp_addr = *grp_addr;
  mrt->grp_leader = *grp_leader;
  mrt->grp_seqno = grp_seqno;
  mrt->flags = flags;
  mrt->leader_hcnt = hops;
  
  aodv_timer_init(&mrt->mrt_timer, aodv_mcast_expire_timeout, mrt);
  aodv_timer_init(&mrt->grph_timer, aodv_grph_timeout, mrt);
  aodv_timer_init(&mrt->prune_timer, aodv_mcast_prune_dangle, mrt);
  aodv_timer_init(&mrt->report_up_timer, aodv_mcast_report_up, mrt);
  
  mrt->grph_update_cnt = 0;
  mrt->activated_upstream_cnt = 0;
  mrt->activated_downstream_cnt = 0;
  
  mrt->next_hop = NULL;
  mrt->origlist = NULL;
  mrt->sources = NULL;
  
  /* Insert into mrt list */
  aodv_mrt_tbl.num_entries++;
  LWIP_DEBUGF(AODV_DEBUG, ("aodv_mrt_insert: Inserting %s in multicast routing table, leader is %s!\n", 
                            inet_ntoa_r(*grp_addr, buffer1, INET_ADDRSTRLEN), 
                            inet_ntoa_r(*grp_leader, buffer2, INET_ADDRSTRLEN)));
  
  __AODV_LIST_INSERT(mrt, aodv_mrt_tbl.hash_tbl[hash]);
  
  aodv_timer_set_timeout(&mrt->mrt_timer, life);
  aodv_timer_set_timeout(&mrt->report_up_timer, GROUP_HELLO_INTERVAL);
  
  return (mrt);
}

/**
 * update multicase mrt_timer timeout
 *
 * @param mrt multicase route entry node
 * @param lifetime new lifetime
 * @return multicase route entry node
 */
struct aodv_mrtnode *aodv_mrt_update_timeout (struct aodv_mrtnode *mrt, u32_t lifetime)
{
  LWIP_ERROR("mrt != NULL", (mrt != NULL), return NULL;);
  
  /* Check if the current valid timeout is larger than the ne
     one - in that case keep the old one. */
  if (mrt->mrt_timer.msec < lifetime) {
    aodv_timer_set_timeout(&mrt->mrt_timer, lifetime);
  }
  
  return (mrt);
}

/**
 * find a multicast routing node
 *
 * @param grp_addr address of the group IP.
 * @return multicase route entry node
 */
struct aodv_mrtnode *aodv_mrt_find (struct in_addr *grp_addr)
{
  int hash;
  struct aodv_mrtnode *mfind;

  if (aodv_mrt_tbl.num_entries == 0) {
    return NULL;
  }
  
  hash = hash_index(grp_addr);
  for (mfind = aodv_mrt_tbl.hash_tbl[hash]; mfind != NULL; mfind = mfind->next) {
    if (mfind->grp_addr.s_addr == grp_addr->s_addr) {
      return (mfind);
    }
  }
  
  return NULL;
}

/* delete a mrt entry
 * 
 * @param mrt multicase route entry node
 */
void aodv_mrt_delete (struct aodv_mrtnode *mrt)
{
  int hash;
  
  LWIP_ERROR("mrt != NULL", (mrt != NULL), return;);
  
  hash = hash_index(&mrt->grp_addr);
  
  __AODV_LIST_DELETE(mrt, aodv_mrt_tbl.hash_tbl[hash]);
  
  aodv_mrt_nexthop_remove_all(mrt);
  aodv_mrt_source_remove_all(mrt);
  aodv_mrt_origlist_remove_all(mrt);
  
  /* Make sure timers are removed... */
  aodv_timer_remove(&mrt->mrt_timer);
  aodv_timer_remove(&mrt->grph_timer);
  aodv_timer_remove(&mrt->prune_timer);
  aodv_timer_remove(&mrt->report_up_timer);
  
  aodv_mrt_tbl.num_entries--;
  
  mem_free(mrt);
}

/* delete mrt entry
 * 
 * @param mrt multicase route entry node
 */
void aodv_mrt_delete_netif (struct netif *netif)
{
  int i;
    
  for (i = 0; i < AODV_MRT_TABLESIZE; i++) {
    struct aodv_mrtnode *mrt;
    struct aodv_mrtnode *mrt_del;
    
    mrt = aodv_mrt_tbl.hash_tbl[i];
    while (mrt != NULL) {
      if (mrt->netif != netif) {
        mrt = mrt->next;
      } else {
        mrt_del = mrt;
        mrt = mrt->next;
        
        aodv_mrt_delete(mrt_del);
      }
    }
  }
}

/**
 * traversal multicase route table
 *
 * @param hook hook function
 */
void aodv_mrt_traversal (aodv_rt_hook_handler hook,
                         void *arg0, void *arg1, void *arg2, void *arg3, void *arg4)
{
  int i;
  struct aodv_mrtnode *mrt;

  for (i = 0; i < AODV_MRT_TABLESIZE; i++) {
    
    for (mrt  = aodv_mrt_tbl.hash_tbl[i];
         mrt != NULL;
         mrt  = mrt->next) {
         
      hook(mrt, arg0, arg1, arg2, arg3, arg4);
    }
  }
}

/* set mrt leader
 * 
 * @param mrt multicase route entry node
 * @param leader leader address
 * @param leader_hcnt hops cnt to leader
 */
void aodv_mrt_set_leader (struct aodv_mrtnode *mrt, struct in_addr *leader, u8_t leader_hcnt)
{
#if AODV_DEBUG
  char buffer1[INET_ADDRSTRLEN];
  char buffer2[INET_ADDRSTRLEN];
#endif

  LWIP_ERROR("mrt != NULL", (mrt != NULL), return;);
  
  mrt->grp_leader = *leader;
  mrt->leader_hcnt = leader_hcnt;
  
  LWIP_DEBUGF(AODV_DEBUG, ("aodv_mrt_set_leader: set leader %s of group %s\n", 
              inet_ntoa_r(mrt->grp_leader, buffer1, INET_ADDRSTRLEN),
              inet_ntoa_r(mrt->grp_addr, buffer2, INET_ADDRSTRLEN)));
}

/* set mrt leader
 * 
 * @param mrt multicase route entry node
 * @param ismember is membership ?
 */
void aodv_mrt_set_membership (struct aodv_mrtnode *mrt, u8_t ismember)
{
  LWIP_ERROR("mrt != NULL", (mrt != NULL), return;);
  
  if (ismember) {
    mrt->flags |= AODV_MRT_MEMBER;
  } else {
    mrt->flags &= ~AODV_MRT_MEMBER;
  }
}

/* set mrt leader hops cnt
 * 
 * @param mrt multicase route entry node
 * @param leader_hcnt hops cnt to leader
 */
void aodv_mrt_set_leader_hcnt (struct aodv_mrtnode *mrt, u8_t leader_hcnt)
{
  LWIP_ERROR("mrt != NULL", (mrt != NULL), return;);
  
  mrt->leader_hcnt = leader_hcnt;
}

/* set mrt become a group leader
 * 
 * @param mrt multicase route entry node
 */
void aodv_mrt_become_leader (struct aodv_mrtnode *mrt)
{
#if AODV_DEBUG
  char buffer[INET_ADDRSTRLEN];
#endif

  LWIP_ERROR("mrt != NULL", (mrt != NULL), return;);
  LWIP_ERROR("mrt->netif != NULL", (mrt->netif != NULL), return;);
  
  mrt->grp_leader.s_addr = netif_ip4_addr(mrt->netif)->addr;
  
  if (mrt->flags & AODV_MRT_GATEWAY) {
    mrt->flags |= AODV_MRT_MEMBER;
  }
  
  mrt->flags |= AODV_MRT_LEADER;
  
  mrt->leader_hcnt = 0;
  mrt->grph_update_cnt = 1;
  
  LWIP_DEBUGF(AODV_DEBUG, ("aodv_mrt_become_leader: become leader of group %s\n", 
              inet_ntoa_r(mrt->grp_addr, buffer, INET_ADDRSTRLEN)));
  
  aodv_mrt_start_routing(mrt);
  
  /* send a hello */
  aodv_grph_timeout(mrt);
}

/* set mrt start routing
 * 
 * @param mrt multicase route entry node
 */
void aodv_mrt_start_routing (struct aodv_mrtnode *mrt)
{
  LWIP_ERROR("mrt != NULL", (mrt != NULL), return;);

  mrt->flags |= AODV_MRT_ROUTER;
  
  if (aodv_gw_get()) {
    mrt->flags |= AODV_MRT_GATEWAY;
    if (netif_default) {
      aodv_igmp_sendreply(mrt, netif_default); /* notify multicast router */
    }
  }
}

/* set mrt stop routing
 * 
 * @param mrt multicase route entry node
 */
void aodv_mrt_stop_routing (struct aodv_mrtnode *mrt)
{
  LWIP_ERROR("mrt != NULL", (mrt != NULL), return;);
  
  mrt->flags &= ~AODV_MRT_ROUTER;
}

/* find a next hop in mrt entry
 * 
 * @param mrt Multicase route entry node
 * @param addr Address of next hop
 * @return next hop
 */
struct aodv_mrtnexthop *aodv_mrt_nexthop_find (struct aodv_mrtnode *mrt,
                                               struct in_addr *addr)
{
  struct aodv_mrtnexthop *nexthop;
  
  LWIP_ERROR("mrt != NULL", (mrt != NULL), return NULL;);
  
  nexthop = mrt->next_hop;
  
  while (nexthop) {
    if (nexthop->addr.s_addr == addr->s_addr) {
      return (nexthop);
    }
    nexthop = nexthop->next;
  }
  
  return (NULL);
}

/* add a next hop to mrt entry
 * 
 * @param mrt Multicase route entry node
 * @param addr Address of next hop
 * @param direct Direction
 * @param grp_seqno Group sequence number
 * @return new next hop
 */
struct aodv_mrtnexthop *aodv_mrt_nexthop_add (struct aodv_mrtnode *mrt, 
                                              struct in_addr *addr, 
                                              u8_t direct, 
                                              u32_t grp_seqno)
{
  struct aodv_mrtnexthop *newhop;
  
  LWIP_ERROR("mrt != NULL", (mrt != NULL), return NULL;);
  
  newhop = aodv_mrt_nexthop_find(mrt, addr);
  if (newhop) {
    return (newhop);
  }
  
  newhop = (struct aodv_mrtnexthop *)mem_malloc(sizeof(struct aodv_mrtnexthop));
  
  LWIP_ERROR("newhop != NULL", (newhop != NULL), return NULL;);
  
  newhop->addr = *addr;
  newhop->grp_seqno = grp_seqno;
  newhop->flags = direct; /* not active */
  newhop->grp_hcnt = 0xff; /* start with infinite hcnt */
  
  newhop->next = mrt->next_hop;
  mrt->next_hop = newhop;
  
  return (newhop);
}

/* delete a next hop from mrt entry
 * 
 * @param mrt Multicase route entry node
 * @param addr Address of next hop
 */
void aodv_mrt_nexthop_remove (struct aodv_mrtnode *mrt, struct in_addr *addr)
{
  struct aodv_mrtnexthop *nexthop, *prev = NULL;
  
  LWIP_ERROR("mrt != NULL", (mrt != NULL), return;);
  
  nexthop = mrt->next_hop;
  
  while (nexthop) {
    if (nexthop->addr.s_addr == addr->s_addr) {
      break;
    }
    prev = nexthop;
    nexthop = nexthop->next;
  }
  
  if (!nexthop) {
    return;
  }
  
  /* Deactive */
  if (nexthop->flags & AODV_MRT_NEXTHOP_ACT) {
    nexthop->flags &= ~AODV_MRT_NEXTHOP_ACT;
    if (nexthop->flags & AODV_MRT_NEXTHOP_UP) {
      mrt->activated_upstream_cnt--;
    } else {
      mrt->activated_downstream_cnt--;
    }
  }
  
  if (prev) {
    prev->next = nexthop->next;
  } else {
    mrt->next_hop = nexthop->next;
  }
  
  mem_free(nexthop);
}

/* delete all next hop from mrt entry
 * 
 * @param mrt Multicase route entry node
 */
void aodv_mrt_nexthop_remove_all (struct aodv_mrtnode *mrt)
{
  struct aodv_mrtnexthop *nexthop, *free = NULL;
  
  LWIP_ERROR("mrt != NULL", (mrt != NULL), return;);
  
  nexthop = mrt->next_hop;
  
  while (nexthop) {
    free = nexthop;
    nexthop = nexthop->next;
    mem_free(free);
  }
  
  mrt->next_hop = NULL;
  mrt->activated_upstream_cnt = 0;
  mrt->activated_downstream_cnt = 0;
}
/* active a next hop 
 * 
 * @param mrt Multicase route entry node
 * @param addr Address of next hop
 */
void aodv_mrt_nexthop_active (struct aodv_mrtnode *mrt, struct in_addr *addr)
{
  struct aodv_mrtnexthop *nexthop;
  
  LWIP_ERROR("mrt != NULL", (mrt != NULL), return;);
  
  nexthop = aodv_mrt_nexthop_find(mrt, addr);
  
  if (nexthop && !(nexthop->flags & AODV_MRT_NEXTHOP_ACT)) {
    nexthop->flags |= AODV_MRT_NEXTHOP_ACT;
    if (nexthop->flags & AODV_MRT_NEXTHOP_UP) {
      mrt->activated_upstream_cnt++;
    } else {
      mrt->activated_downstream_cnt++;
    }
  }
}

/* deactive a next hop 
 * 
 * @param mrt Multicase route entry node
 * @param addr Address of next hop
 */
void aodv_mrt_nexthop_deactive (struct aodv_mrtnode *mrt, struct in_addr *addr)
{
  struct aodv_mrtnexthop *nexthop;
  
  LWIP_ERROR("mrt != NULL", (mrt != NULL), return;);
  
  nexthop = aodv_mrt_nexthop_find(mrt, addr);
  
  if (nexthop && (nexthop->flags & AODV_MRT_NEXTHOP_ACT)) {
    nexthop->flags &= ~AODV_MRT_NEXTHOP_ACT;
    if (nexthop->flags & AODV_MRT_NEXTHOP_UP) {
      mrt->activated_upstream_cnt--;
    } else {
      mrt->activated_downstream_cnt--;
    }
  }
}

/* get a next hop witch is upstream (must in active status)
 * 
 * @param mrt Multicase route entry node
 * @return next hop
 */
struct aodv_mrtnexthop *aodv_mrt_get_activated_upstream (struct aodv_mrtnode *mrt)
{
  struct aodv_mrtnexthop *nexthop;
  
  LWIP_ERROR("mrt != NULL", (mrt != NULL), return NULL;);
  
  nexthop = mrt->next_hop;
  
  while (nexthop) {
    if ((nexthop->flags & AODV_MRT_NEXTHOP_ACT) && (nexthop->flags & AODV_MRT_NEXTHOP_UP)) {
      return (nexthop);
    }
    nexthop = nexthop->next;
  }
  
  return (NULL);
}

/* get best next hop witch is upstream
 * 
 * @param mrt Multicase route entry node
 * @return next hop
 */
struct aodv_mrtnexthop *aodv_mrt_get_best_upstream (struct aodv_mrtnode *mrt)
{
  struct aodv_mrtnexthop *nexthop, *besthop = NULL;
  
  LWIP_ERROR("mrt != NULL", (mrt != NULL), return NULL;);
  
  nexthop = mrt->next_hop;
  
  while (nexthop) {
    /* Before qualifying to be a upstream best hop, the next 
       hop must first, in fact be upstream.
       - if that is true, then the current nexthop will take the
       besthop position if either:
         - there is not current besthop
         - sequence number is greater than curr besthop
         - seq no are equal, but we have a lesser hop count
    */ 
    if (nexthop->flags & AODV_MRT_NEXTHOP_UP) {
      if (besthop == NULL) {
        besthop = nexthop;
      
      } else if ((nexthop->grp_seqno > besthop->grp_seqno) ||
                 ((nexthop->grp_seqno == besthop->grp_seqno) && 
                  (nexthop->grp_hcnt < besthop->grp_hcnt))) {
        besthop = nexthop;
      }
    }
    nexthop = nexthop->next;
  }
  
  return (besthop);
}

/* find a source in mrt entry
 * 
 * @param mrt Multicase route entry node
 * @param addr Address of source
 * @return source
 */
struct aodv_mrtsrc *aodv_mrt_source_find (struct aodv_mrtnode *mrt, struct in_addr *src)
{
#if !defined(MRT_ADD_MFC) && !defined(DVMRP_ADD_MRT)

  struct aodv_mrtsrc *mtrsrc;
  
  LWIP_ERROR("mrt != NULL", (mrt != NULL), return NULL;);
  
  mtrsrc = mrt->sources;
  
  while (mtrsrc) {
    if (mtrsrc->src_addr.s_addr == src->s_addr) {
      return (mtrsrc);
    }
    mtrsrc = mtrsrc->next;
  }
#endif /* !defined(MRT_ADD_MFC) && !defined(DVMRP_ADD_MRT) */

  return (NULL);
}

/* add a source to mrt entry
 * 
 * @param addr Address of source
 * @param dest_grp dest group addr
 */
void aodv_mrt_source_add (struct in_addr *src, struct in_addr *dest_grp)
{
#if !defined(MRT_ADD_MFC) && !defined(DVMRP_ADD_MRT)

  struct aodv_mrtnode *mrt = aodv_mrt_find(dest_grp);
  struct aodv_mrtsrc *newsrc;
  
  if (mrt == NULL) {
    return;
  }
  
  newsrc = aodv_mrt_source_find(mrt, src);
  if (newsrc) {
    return;
  }
  
  newsrc = (struct aodv_mrtsrc *)mem_malloc(sizeof(struct aodv_mrtsrc));
  
  LWIP_ERROR("newsrc != NULL", (newsrc != NULL), return;);
  
  newsrc->src_addr = *src;
  newsrc->next = mrt->sources;
  mrt->sources = newsrc;
  
  /* TODO: use setsockopt to add a multicast route */ 
  
#endif /* !defined(MRT_ADD_MFC) && !defined(DVMRP_ADD_MRT) */
}

/* delete all source from mrt entry
 * 
 * @param mrt Multicase route entry node
 */
void aodv_mrt_source_remove_all (struct aodv_mrtnode *mrt)
{
#if !defined(MRT_ADD_MFC) && !defined(DVMRP_ADD_MRT)

  struct aodv_mrtsrc *mtrsrc, *free = NULL;
  
  LWIP_ERROR("mrt != NULL", (mrt != NULL), return;);
  
  mtrsrc = mrt->sources;
  
  while (mtrsrc) {
    free = mtrsrc;
    mtrsrc = mtrsrc->next;
    
    /* TODO: use setsockopt to delete a multicast route */ 
    
    mem_free(free);
  }
  
  mrt->sources = NULL;
#endif /* !defined(MRT_ADD_MFC) && !defined(DVMRP_ADD_MRT) */
}

/* find a orig in mrt entry
 * 
 * @param mrt Multicase route entry node
 * @param orig Address of orig
 * @return mtrorig
 */
struct aodv_mrtorig *aodv_mrt_origlist_find (struct aodv_mrtnode *mrt, struct in_addr *orig)
{
  struct aodv_mrtorig *mtrorig;
  
  LWIP_ERROR("mrt != NULL", (mrt != NULL), return NULL;);
  
  mtrorig = mrt->origlist;
  
  while (mtrorig) {
    if (mtrorig->orig_addr.s_addr == orig->s_addr) {
      return (mtrorig);
    }
    mtrorig = mtrorig->next;
  }
  
  return (NULL);
}

/* add a orig to mrt entry
 * 
 * @param mrt Multicase route entry node
 * @param orig Address of orig
 * @return mtrorig
 */
struct aodv_mrtorig *aodv_mrt_origlist_add (struct aodv_mrtnode *mrt, struct in_addr *orig)
{
  struct aodv_mrtorig *neworig;
  
  LWIP_ERROR("mrt != NULL", (mrt != NULL), return NULL;);
  
  neworig = aodv_mrt_origlist_find(mrt, orig);
  
  if (neworig) {
    return (neworig);
  }
  
  neworig = (struct aodv_mrtorig *)mem_malloc(sizeof(struct aodv_mrtorig));
  
  LWIP_ERROR("neworig != NULL", (neworig != NULL), return NULL;);
  
  neworig->orig_addr = *orig;
  neworig->grp_seqno = 0;
  neworig->grp_hcnt = 0xff; /* max hcnt */
  
  neworig->next = mrt->origlist;
  neworig->up = mrt;
  mrt->origlist = neworig;
  
  /* do timeout stuff */
  aodv_timer_init(&neworig->cleanup_timer, aodv_mrt_origlist_remove, neworig);
  
  aodv_timer_set_timeout(&neworig->cleanup_timer, RREP_WAIT_TIME);
  
  return (neworig);
}

/* remove a orig
 * 
 * @param mrt Multicase route entry node
 * @param orig Address of orig
 * @return mtrorig
 */
void aodv_mrt_origlist_remove (void *arg)
{
  struct aodv_mrtnode *mrt;
  struct aodv_mrtorig *curr, *prev = NULL;
  struct aodv_mrtorig *orig = (struct aodv_mrtorig *)arg;
  
  LWIP_ERROR("orig != NULL", (orig != NULL), return;);
  
  mrt = orig->up;
  
  LWIP_ERROR("mrt != NULL", (mrt != NULL), return;);
  
  aodv_timer_remove(&orig->cleanup_timer);
  
  curr = mrt->origlist;
  
  while (curr && (curr != orig)) {
    prev = curr;
    curr = curr->next;
  }
  
  if (curr == NULL) {
    LWIP_DEBUGF(AODV_DEBUG, ("aodv_mrt_origlist_remove: not exist in multicast routing table!\n"));
    return;
  }
  
  if (prev == NULL) {
    orig->up->origlist = curr->next;
  } else {
    prev->next = curr->next;
  }
  
  mem_free(orig);
}

/* remove a orig
 * 
 * @param mrt Multicase route entry node
 * @param orig Address of orig
 * @return mtrorig
 */
void aodv_mrt_origlist_remove_all (struct aodv_mrtnode *mrt)
{
  struct aodv_mrtorig *orig, *free = NULL;
  
  LWIP_ERROR("mrt != NULL", (mrt != NULL), return;);
  
  orig = mrt->origlist;
  
  while (orig) {
    free = orig;
    orig = orig->next;
    
    aodv_timer_remove(&free->cleanup_timer);
    mem_free(free);
  }
  
  mrt->origlist = NULL;
}

#endif /* AODV_MCAST */

/* end */
