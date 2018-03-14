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
#include "aodv_if.h"
#include "aodv_seeklist.h"

#include <string.h>

static struct aodv_rreq_record *record_list;
static struct aodv_blacklist *black_list;

/**
 * create a rreq packet (pbuf)
 *
 * @param flags packet flags
 * @param dest_addr destination address
 * @param dest_seqno destination sequence number 
 * @param orig_addr originator address
 * @return rreq packet
 */
struct pbuf *aodv_rreq_create (u8_t flags, struct in_addr *dest_addr,
                               u32_t dest_seqno, struct in_addr *orig_addr,
                               u8_t grp_hcnt, struct in_addr *grp_addr)
{
  struct pbuf *p;
  struct aodv_rreq *rreq;
  u32_t host_rreq_id;
  u32_t host_seqno;
  u16_t length = RREQ_SIZE;
  
#if AODV_MCAST
  GRP_REBUILD_EXT gre;
  GRP_MERGE_EXT gme;
#endif /* AODV_MCAST */
  
#if AODV_MCAST
  if (!grp_addr) {
    grp_addr = &aodv_addr_any;
  }
#endif /* AODV_MCAST */
  
#if AODV_MCAST
  if (flags & RREQ_GRP_REBUILD) {
    length += GRP_REBUILD_EXT_SIZE;
  }
  if ((flags & RREQ_REPAIR) && (flags & RREQ_JOIN)) {
    length += GRP_MERGE_EXT_SIZE;
  }
#endif /* AODV_MCAST */

  host_rreq_id = htonl(aodv_host_state.rreq_id);
  aodv_host_state.rreq_id++;
  
  SEQNO_INC(aodv_host_state.seqno);
  host_seqno = htonl(aodv_host_state.seqno);
  
  p = pbuf_alloc(PBUF_TRANSPORT, length, PBUF_RAM);
  if (p) {
    rreq = (struct aodv_rreq *)p->payload;
    rreq->type = AODV_RREQ;
    rreq->flags = flags;
    rreq->rsvd = 0;
    rreq->hcnt = 0;
    rreq->rreq_id = host_rreq_id;
    rreq->dest_addr = dest_addr->s_addr;
    rreq->dest_seqno = htonl(dest_seqno);
    rreq->orig_addr = orig_addr->s_addr;
    rreq->orig_seqno = host_seqno;
  } else {
    LWIP_DEBUGF(AODV_DEBUG, ("aodv_rreq_create: can not allocate rreq packet!\n"));
    return NULL;
  }
  
  rreq->flags &= ~RREQ_GRP_REBUILD; /* exclude this special flag */
  
#if AODV_MCAST
  length = RREQ_SIZE;
  if (flags & RREQ_GRP_REBUILD) {
    gre.type = GRE_TYPE;
    gre.length = GRE_LENGTH;
    gre.grp_hcnt = htons((u16_t)grp_hcnt);
    memcpy((char *)rreq + length, &gre, GRP_REBUILD_EXT_SIZE);
    length += GRP_REBUILD_EXT_SIZE;
  }
  if ((flags & RREQ_REPAIR) && (flags & RREQ_JOIN)) {
    /* add merge extension */
    gme.type = GME_TYPE;
    gme.length = GME_LENGTH;
    gme.res = 0;
    gme.grp_addr = grp_addr->s_addr;
    gme.grp_seqno = 0;
    memcpy((char *)rreq + length, &gme, GRP_MERGE_EXT_SIZE);
  }
#endif /* AODV_MCAST */
  
  return p;
} 

/**
 * send a rreq packet
 *
 * @param dest_addr destination address
 * @param dest_seqno destination sequence number 
 * @param ttl ip packet time to live
 * @param flags packet flags
 */
void aodv_rreq_send (struct in_addr *dest_addr, u32_t dest_seqno, u8_t ttl, u8_t flags)
{
  int i;
  struct pbuf *p;
  struct in_addr orig_addr;
  struct in_addr dest_broadcast;
  
  dest_broadcast.s_addr = INADDR_BROADCAST; /* broadcast */
  
  /* TODO: in lwip broadcast packet is always used defaute netif output, 
   * so adov netif must be default one! 
   */
  for (i = 0; i < AODV_MAX_NETIF; i++) { /* broadcast to all aodv netif */
    if (aodv_netif[i]) {
      orig_addr.s_addr = netif_ip4_addr(aodv_netif[i])->addr;
      p = aodv_rreq_create(flags, dest_addr, dest_seqno, &orig_addr, 0, NULL);
      if (p) {
        aodv_udp_sendto(p, &dest_broadcast, ttl, i);
        pbuf_free(p);
      }
    }
  }
}

/**
 * send a rreq packet with grp extern
 *
 * @param dest_addr destination address
 * @param dest_seqno destination sequence number 
 * @param ttl ip packet time to live
 * @param flags packet flags
 */
void aodv_rreq_send_ext (struct in_addr *dest_addr, u32_t dest_seqno, u8_t ttl, u8_t flags, 
                         u8_t grp_hcnt, struct in_addr *grp_addr)
{
  int i;
  struct pbuf *p;
  struct in_addr orig_addr;
  struct in_addr dest_broadcast;
  
  dest_broadcast.s_addr = INADDR_BROADCAST; /* broadcast */
  
  /* TODO: in lwip broadcast packet is always used defaute netif output, 
   * so adov netif must be default one! 
   */
  for (i = 0; i < AODV_MAX_NETIF; i++) { /* broadcast to all aodv netif */
    if (aodv_netif[i]) {
      orig_addr.s_addr = netif_ip4_addr(aodv_netif[i])->addr;
      p = aodv_rreq_create(flags, dest_addr, dest_seqno, &orig_addr, grp_hcnt, grp_addr);
      if (p) {
        aodv_udp_sendto(p, &dest_broadcast, ttl, i);
        pbuf_free(p);
      }
    }
  }
}

/**
 * forward a rreq packet
 *
 * @param p pbuf save the rreq packet
 * @param ttl ip packet time to live
 */
void aodv_rreq_forward (struct pbuf *p, u8_t ttl)
{
#if AODV_DEBUG
  char buffer[INET_ADDRSTRLEN];
  struct in_addr inaddr;
#endif

  int i;
  struct in_addr dest_broadcast;
  struct aodv_rreq *rreq;
  
  LWIP_ERROR("p != NULL", (p != NULL), return;);
  
  dest_broadcast.s_addr = INADDR_BROADCAST;

  rreq = (struct aodv_rreq *)p->payload;
  rreq->hcnt++; /* Increase hopcount to account for intermediate route */

#if AODV_DEBUG
  inaddr.s_addr = (in_addr_t)rreq->orig_addr;
#endif

  /* FORWARD the RREQ if the TTL allows it. */
  LWIP_DEBUGF(AODV_DEBUG, ("aodv_rreq_forward: forwarding RREQ src=%s, rreq_id=%lu\n",
	          inet_ntoa_r(inaddr, buffer, INET_ADDRSTRLEN), ntohl(rreq->rreq_id)));
  
  for (i = 0; i < AODV_MAX_NETIF; i++) { /* broadcast to all aodv netif */
    if (aodv_netif[i]) {
      aodv_udp_sendto(p, &dest_broadcast, ttl, i);
    }
  }
}

#if AODV_MCAST
/**
 * forward a rreq packet
 *
 * @param p pbuf save the rrep packet
 * @param ttl ip packet time to live
 */
void aodv_rreq_forward_unicast (struct pbuf *p, struct in_addr *dest_addr, u8_t ttl)
{
  int i;
  struct in_addr dest_broadcast;
  struct aodv_rreq *rreq;
  
  LWIP_ERROR("p != NULL", (p != NULL), return;);
  
  if (dest_addr) {
    dest_broadcast.s_addr = dest_addr->s_addr;
  } else {
    dest_broadcast.s_addr = INADDR_BROADCAST;
  }
  
  rreq = (struct aodv_rreq *)p->payload;
  
  rreq->hcnt++; /* Increase hopcount to account for intermediate route */
  
  /* TODO: in lwip broadcast packet is always used one netif output, 
   * so adov netif must be default one! 
   */
  for (i = 0; i < AODV_MAX_NETIF; i++) { /* broadcast to all aodv netif */
    if (aodv_netif[i]) {
      aodv_udp_sendto(p, &dest_broadcast, ttl, i);
    }
  }
}
#endif /* AODV_MCAST */

/**
 * process a rreq packet
 *
 * @param p pbuf save the rreq packet
 * @param ip_src source address
 * @param ip_dst destination address
 * @param ttl ip packet time to live
 * @param netif the network interface on which the packet was received
 */
void aodv_rreq_process (struct pbuf *p, 
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

  struct aodv_rreq *rreq;
  struct pbuf *p_rrep;
  
  struct aodv_rtnode *rev_rt, *fwd_rt = NULL;
  
  u32_t rreq_orig_seqno, rreq_dest_seqno;
  u32_t rreq_id, life;
  u8_t  rreq_new_hcnt;
  
  struct in_addr rreq_dest, rreq_orig;
  struct ip4_addr rreq_ip_dest;
  
#if AODV_MCAST
  struct aodv_ext *ext;
  int rreqlen;
  u8_t grb_en = 0, gmg_en = 0;
  GRP_REBUILD_EXT grb_ext;
  GRP_MERGE_EXT gmg_ext;
  struct in_addr grp_addr;
  grp_addr.s_addr = 0;
#endif /* AODV_MCAST */

  LWIP_ERROR("p != NULL", (p != NULL), return;);
  LWIP_ERROR("p->len >= RREQ_SIZE", (p->len >= RREQ_SIZE), return;);
  LWIP_ERROR("p->len == p->tot_len", (p->len == p->tot_len), return;); /* must in a no splite pbuf */
  LWIP_ERROR("ip_src != NULL", (ip_src != NULL), return;);
  LWIP_ERROR("ip_dst != NULL", (ip_dst != NULL), return;);
  LWIP_ERROR("netif != NULL", (netif != NULL), return;);
  
  rreq = (struct aodv_rreq *)p->payload;
#if AODV_MCAST
  rreqlen = (int)p->tot_len;
#endif /* AODV_MCAST */

  rreq_dest.s_addr = rreq->dest_addr;
  rreq_orig.s_addr = rreq->orig_addr;
  rreq_id          = ntohl(rreq->rreq_id);
  rreq_dest_seqno  = ntohl(rreq->dest_seqno);
  rreq_orig_seqno  = ntohl(rreq->orig_seqno);
  rreq_new_hcnt    = (u8_t)(rreq->hcnt + 1);
  
  /* Ignore messages which aim to a create a route to one self */
  if (rreq_orig.s_addr == netif_ip4_addr(netif)->addr) {
    return;
  }
  
  rreq_ip_dest.addr = rreq_dest.s_addr;
  
  LWIP_DEBUGF(AODV_DEBUG, ("aodv_rreq_process: ip_src=%s rreq_orig=%s rreq_dest=%s ttl=%d\n",
              inet_ntoa_r(*ip_src, buffer1, INET_ADDRSTRLEN), 
              inet_ntoa_r(rreq_orig, buffer2, INET_ADDRSTRLEN), 
              inet_ntoa_r(rreq_dest, buffer3, INET_ADDRSTRLEN),
              ttl));
              
  /* Check if the previous hop of the RREQ is in the blacklist set. If
     it is, then ignore the RREQ. */
  if (aodv_rreq_blacklist_find(ip_src)) {
    LWIP_DEBUGF(AODV_DEBUG, ("aodv_rreq_process: prev hop of RREQ blacklisted, ignoring!\n"));
    return;
  }
  
  /* Ignore already processed RREQs in PATH_DISCOVERY_TIME. */
  if (aodv_rreq_record_find(&rreq_orig, rreq_id)) {
    return;
  }
  
  /* Now buffer this RREQ so that we don't process a similar RREQ we
     get within PATH_DISCOVERY_TIME. */
  aodv_rreq_record_insert(&rreq_orig, rreq_id);
  
#if AODV_MCAST
  /* Determine whether there are any RREQ extensions */
  ext = (struct aodv_ext *)((char *)rreq + RREQ_SIZE);
  while (rreqlen > (int)RREQ_SIZE) {
    switch (ext->type) {
    
    case RREQ_GRP_REBUILD_EXT:
      if (ext->length == GRE_LENGTH) {
        memcpy(&grb_ext, ext, sizeof(grb_ext));
        grb_en = 1;
      }
      break;
      
    case RREQ_GRP_MERGE_EXT:
      if (ext->length == GME_LENGTH) {
        memcpy(&gmg_ext, ext, sizeof(gmg_ext));
        memcpy(&grp_addr, &gmg_ext.grp_addr, sizeof(struct in_addr));
        gmg_en = 1;
      }
      break;
    }
    rreqlen -= AODV_EXT_SIZE(ext);
	ext = AODV_EXT_NEXT(ext);
  }
#endif /* AODV_MCAST */
  
  /* The node always creates or updates a REVERSE ROUTE entry to the
     source of the RREQ. */
  rev_rt = aodv_rt_find(&rreq_orig);
  
  /* Calculate the extended minimal life time. */
  life = PATH_DISCOVERY_TIME - 2 * rreq_new_hcnt * NODE_TRAVERSAL_TIME;
  
  if (rev_rt == NULL) {
    LWIP_DEBUGF(AODV_DEBUG, ("aodv_rreq_process: Creating REVERSE route entry, RREQ orig: %s\n",
                inet_ntoa_r(rreq_orig, buffer1, INET_ADDRSTRLEN)));

    rev_rt = aodv_rt_insert(&rreq_orig, ip_src, rreq_new_hcnt,
                            rreq_orig_seqno, life, AODV_VALID, 0, netif);
                            
    LWIP_ERROR("rev_rt != NULL", (rev_rt != NULL), return;);
    
  } else {
    
    /* update condition(s):
     * old entry seqno is zero || rreq seqno > old entry seqno || 
     * (seqno equ && (old entry invalid || short hop))
     */
    if ((rev_rt->dest_seqno == 0) ||
        ((s32_t)rreq_orig_seqno > (s32_t)rev_rt->dest_seqno) ||
        ((rreq_orig_seqno == rev_rt->dest_seqno) &&
         (rev_rt->state == AODV_INVALID || rreq_new_hcnt < rev_rt->hcnt))) {
      
      rev_rt = aodv_rt_update(rev_rt, ip_src, rreq_new_hcnt,
                              rreq_orig_seqno, life, AODV_VALID,
                              rev_rt->flags);
         
    } else if (rev_rt->next_hop.s_addr != ip_src->s_addr) { /* drop the RREQ, because this may be old */
      LWIP_DEBUGF(AODV_DEBUG, ("aodv_rreq_process: Dropping RREQ due to reverse route mismatch!\n"));
      return;
    }
  }
  
#if AODV_MCAST
  if (IN_MULTICAST(ntohl(rreq_dest.s_addr))) { /* multicast RREQ */
    if (grb_en) {
      aodv_mcast_rreq_process(p, ip_src, ip_dst, ttl, netif, &grb_ext);
    } else {
      aodv_mcast_rreq_process(p, ip_src, ip_dst, ttl, netif, NULL);
    }
    return;
  }
#endif /* AODV_MCAST */

  if (aodv_gw_get()) { /* we are a aodv gateway ? */
    if (!ip4_addr_netcmp(&rreq_ip_dest, netif_ip4_addr(netif), netif_ip4_netmask(netif))) { /* RREQ dest NOT in same subnet */
      struct in_addr me;
      struct aodv_ext *ext;
      
      SEQNO_INC(aodv_host_state.seqno);
      
      me.s_addr = netif_ip4_addr(rev_rt->netif)->addr; /* gateway addr is my aodv netif addr */
      
      p_rrep = aodv_rrep_create(0, 0, 0, &me, /* dest is me, extern data is real inet dest */
                                aodv_host_state.seqno, &rev_rt->dest_addr,
                                ACTIVE_ROUTE_TIMEOUT, AODV_EXT_HDR_SIZE + 4, NULL); /* allocate space for extern data */
      if (p_rrep) {
        ext = aodv_rrep_ext_add(p_rrep, RREP_INET_DEST_EXT, RREP_SIZE,
                                sizeof(struct in_addr), (char *)&rreq_dest); /* rreq_dest is inet dest */
        (void)ext;
        aodv_rrep_send(p_rrep, rev_rt, NULL);
        pbuf_free(p_rrep);
      }
      return;
    }
  }
  
  /* Are we the destination of the RREQ?, if so we should immediately send a
     RREP.. */
  if (rreq_dest.s_addr == netif_ip4_addr(netif)->addr) {
    struct in_addr dest_rrep;
    
    dest_rrep.s_addr = netif_ip4_addr(rev_rt->netif)->addr;
  
    /* WE are the RREQ DESTINATION. Update the node's own
       sequence number to the maximum of the current seqno and the
       one in the RREQ. */
    if (rreq_dest_seqno != 0) {
      if ((s32_t)aodv_host_state.seqno < (s32_t)rreq_dest_seqno) {
        aodv_host_state.seqno = rreq_dest_seqno;
        
      } else if (aodv_host_state.seqno == rreq_dest_seqno) {
        SEQNO_INC(aodv_host_state.seqno);
      }
    }
    
#if AODV_MCAST
    if ((rreq->flags & RREQ_JOIN) && (rreq->flags & RREQ_REPAIR) && (gmg_en)) {
      struct aodv_mrtnode *mrt;
      mrt = aodv_mrt_find(&grp_addr);
      if (mrt && (mrt->flags & AODV_MRT_LEADER) && !(mrt->flags & AODV_MRT_REPAIR)) {
        /* recieved a rreq because i'm leader */
        /* make GRPH flag update */
        mrt->grph_update_cnt = 2;
        p_rrep = aodv_rrep_create(RREP_REPAIR, 0, 0, &dest_rrep,
                                  aodv_host_state.seqno, &rev_rt->dest_addr,
                                  MY_ROUTE_TIMEOUT, 0, &grp_addr);
        if (p_rrep) {
          aodv_rrep_send(p_rrep, rev_rt, NULL);
          pbuf_free(p_rrep);
        }
      }
    } else
#endif /* !AODV_MCAST */
    {
      p_rrep = aodv_rrep_create(0, 0, 0, &dest_rrep,
                                aodv_host_state.seqno, &rev_rt->dest_addr,
                                MY_ROUTE_TIMEOUT, 0, NULL);
      if (p_rrep) {
        aodv_rrep_send(p_rrep, rev_rt, NULL);
        pbuf_free(p_rrep);
      }
    }
  } else {
    /* We are an INTERMEDIATE node. - check if we have an active
     * route entry */
    
    fwd_rt = aodv_rt_find(&rreq_dest);
    
    if (fwd_rt && (fwd_rt->state == AODV_VALID) && !(rreq->flags & RREQ_DEST_ONLY)) {
      u32_t lifetime;
      
      /* GENERATE RREP, i.e we have an ACTIVE route entry that is fresh
         enough (our destination sequence number for that route is
         larger than the one in the RREQ). */
      if (fwd_rt->flags & AODV_RT_INET_DEST) {
        goto forward; /* RREP must by gateway self */
      }
      
      /* Respond only if the sequence number is fresh enough... */
      if ((fwd_rt->dest_seqno != 0) &&
          (s32_t)fwd_rt->dest_seqno >= (s32_t)rreq_dest_seqno) {
          
        lifetime = fwd_rt->rt_timer.msec;
          
        p_rrep = aodv_rrep_create(0, 0, fwd_rt->hcnt, &fwd_rt->dest_addr,
                                  fwd_rt->dest_seqno, &rev_rt->dest_addr,
                                  lifetime, 0, NULL);
        if (p_rrep) {
          aodv_rrep_send(p_rrep, rev_rt, fwd_rt);
          pbuf_free(p_rrep);
        }
      
      } else {
        goto forward;
      }
      
      /* If the GRATUITOUS flag is set, we must also unicast a
         gratuitous RREP to the destination. */
      if (rreq->flags & RREQ_GRATUITOUS) {
      
        p_rrep = aodv_rrep_create(0, 0, rev_rt->hcnt, &rev_rt->dest_addr,
                                  rev_rt->dest_seqno, &fwd_rt->dest_addr,
                                  lifetime, 0, NULL);
        if (p_rrep) {
          aodv_rrep_send(p_rrep, fwd_rt, rev_rt);
          pbuf_free(p_rrep);
        }
        
        LWIP_DEBUGF(AODV_DEBUG, ("aodv_rreq_process: Sending G-RREP to %s with rte to %s\n",
              inet_ntoa_r(rreq_dest, buffer1, INET_ADDRSTRLEN), 
              inet_ntoa_r(rreq_orig, buffer2, INET_ADDRSTRLEN)));
      }
      return;
    }
    
forward:
    if (ttl > 1) {
      /* Update the sequence number in case the maintained one is
       * larger */
      if (fwd_rt && !(fwd_rt->flags & AODV_RT_INET_DEST) &&
          (s32_t)fwd_rt->dest_seqno > (s32_t)rreq_dest_seqno) {
        
        rreq->dest_seqno = htonl(fwd_rt->dest_seqno);
      }
      
      ttl--;
      
#if AODV_MCAST
      if ((rreq->flags & RREQ_JOIN) && (rreq->flags & RREQ_REPAIR) && (gmg_en)) {
        struct aodv_mrtnode *mrt;
        struct aodv_mrtnexthop *upstream;
        
        /* if I am part of group, unicast up. */
        /* else regular rreq */
        mrt = aodv_mrt_find(&grp_addr);
        if (mrt && 
            (mrt->flags & AODV_MRT_ROUTER) && 
            (mrt->grp_leader.s_addr == rreq_dest.s_addr)) {
          /* one of the members of the >ip tree */
          /* find activated upstream */
          upstream = aodv_mrt_get_activated_upstream(mrt);
          if (upstream) {
            aodv_rreq_forward_unicast(p, &upstream->addr, ttl);
          } else {
            LWIP_DEBUGF(AODV_DEBUG, ("aodv_rreq_process: no activated upstream, but not leader!\n"));
          }
        } else {
          /* not part of > ip tree, just forward along */
          aodv_rreq_forward(p, ttl);
        }
      } else 
#endif /* AODV_MCAST */
      {
        aodv_rreq_forward(p, ttl);
      }

    } else {
      LWIP_DEBUGF(AODV_DEBUG, ("aodv_rreq_process: RREQ not forwarded - ttl=0\n"));
    }
  }
}

/**
 * Perform route discovery for a unicast destination
 *
 * @param dest_addr destination address
 * @param flags rreq packet flags
 * @param p pbuf queueing
 * @param grp_addr group address
 * @param rev_addr if p is need forward, rev_addr mean who forward this packet to me.
 *
 * only multicast use grp_addr and rev_addr.
 */
void aodv_rreq_route_discovery (struct in_addr *dest_addr, u8_t flags,
                                struct pbuf *p, struct in_addr *grp_addr, struct in_addr *rev_addr)
{
#if AODV_DEBUG
  char buffer[INET_ADDRSTRLEN];
#endif

  struct aodv_seeklist *seek;
  struct aodv_rtnode *rt;
  u32_t dest_seqno;
  u8_t grp_hcnt = 0;
  
  u8_t ttl;
#define TTL_VALUE   ttl

#if AODV_MCAST
  struct aodv_mrtnode *mrt;
#endif /* AODV_MCAST */

  if (!grp_addr) {
    grp_addr = &aodv_addr_any;
  }
  
  LWIP_ERROR("dest_addr != NULL", (dest_addr != NULL), return;);
  
#if AODV_MCAST
  /* for non-merge RREQs, grp_addr should always be zero, so
     seek_list_find_mcast will act like seek_list_find. */
  if (aodv_seeklist_find_mcast(dest_addr, grp_addr)) {
    return;
  }
#else
  if (aodv_seeklist_find(dest_addr)) { /* route discovery are performed */
    return;
  }
#endif /* AODV_MCAST */
  
  /* If we already have a route entry, we use information from it. */
  rt = aodv_rt_find(dest_addr);
  
  grp_hcnt = 0;
  
#if AODV_MCAST
  mrt = NULL;
  if (IN_MULTICAST(ntohl(dest_addr->s_addr))) {
    mrt = aodv_mrt_find(dest_addr);
  }
#endif /* AODV_MCAST */
  
  /* max hops */
  ttl = NET_DIAMETER; /* This is the TTL if we don't use expanding ring search */
  
#if AODV_MCAST
  if ((rt == NULL) && (mrt == NULL))
#else
  if (rt == NULL)
#endif /* AODV_MCAST */
  {
    dest_seqno = 0;
    flags |= RREQ_UNKWN_SEQ; /* unknown dest seqno */
    if (aodv_expanding_ring_search_get()) {
      ttl = TTL_START; /* Successive accumulation */
    }
  } else if (rt) { /* have rt route entry */
    dest_seqno = rt->dest_seqno;
    if (aodv_expanding_ring_search_get()) {
      ttl = (u8_t)(rt->hcnt + TTL_INCREMENT); /* Perhaps his position has not changed */
    }
    /* A routing table entry waiting for a RREP should not be expunged
       before 2 * NET_TRAVERSAL_TIME... */
    if (rt->rt_timer.msec < (2 * NET_TRAVERSAL_TIME)) {
      aodv_rt_update_timeout(rt, 2 * NET_TRAVERSAL_TIME);
    }
  }
#if AODV_MCAST
  else if (mrt) { /* have group route entry */
    if (mrt->grp_seqno == 0) {
      dest_seqno = 0;
      flags |= RREQ_UNKWN_SEQ; /* unknown dest seqno */
    } else {
      dest_seqno = mrt->grp_seqno;
    }
    if (aodv_expanding_ring_search_get()) {
      if (mrt->leader_hcnt == 0xff) {
        ttl = TTL_START;
      } else {
        ttl = (u8_t)(mrt->leader_hcnt + TTL_INCREMENT);
      }
    }
    grp_hcnt = mrt->leader_hcnt;
    
    aodv_mrt_update_timeout(mrt, 2 * NET_TRAVERSAL_TIME);
  }
#endif /* AODV_MCAST */
  
  aodv_rreq_send_ext(dest_addr, dest_seqno, ttl, flags, grp_hcnt, grp_addr);
  
  /* Remember that we are seeking this destination */
  seek = aodv_seeklist_insert(dest_addr, dest_seqno, ttl, flags, p, grp_hcnt, grp_addr, rev_addr);
  LWIP_ERROR("seek != NULL", (seek != NULL), return;);
  
  /* Set a timer for this RREQ */
  if (aodv_expanding_ring_search_get()) {
    aodv_timer_set_timeout(&seek->seek_timer, RING_TRAVERSAL_TIME);
  } else {
    aodv_timer_set_timeout(&seek->seek_timer, NET_TRAVERSAL_TIME);
  }
  
  LWIP_DEBUGF(AODV_DEBUG, ("aodv_rreq_route_discovery: Seeking %s ttl=%d!\n", 
              inet_ntoa_r(*dest_addr, buffer, INET_ADDRSTRLEN), ttl));
}

/**
 * Local repair is very similar to route discovery...
 *
 * @param rt route entry node
 * @param src_addr source address
 * @param p pbuf queueing
 */
void aodv_rreq_local_repair (struct aodv_rtnode *rt, struct in_addr *src_addr,
                             struct pbuf *p)
{
#if AODV_DEBUG
  char buffer[INET_ADDRSTRLEN];
#endif

  struct aodv_seeklist *seek_entry;
  struct aodv_rtnode *src_entry;
  u8_t ttl;
  u8_t flags = 0;

  LWIP_ERROR("rt != NULL", (rt != NULL), return;);
  
  if (aodv_seeklist_find(&rt->dest_addr)) { /* route discovery are performed */
    return;
  }
  
  if (!(rt->flags & AODV_RT_REPAIR)) {
    return;
  }
  
  LWIP_DEBUGF(AODV_DEBUG, ("aodv_rreq_local_repair: REPAIRING route to %s!\n", 
              inet_ntoa_r(*src_addr, buffer, INET_ADDRSTRLEN)));
              
  /* Caclulate the initial ttl to use for the RREQ. MIN_REPAIR_TTL
     mentioned in the draft is the last known hop count to the
     destination. */
  src_entry = aodv_rt_find(src_addr);
  if (src_entry) {
    /* max(rt->hcnt, 0.5 * src_entry->hcnt) + LOCAL_ADD_TTL */
    ttl = (u8_t)(max(rt->hcnt, (src_entry->hcnt / 2)) + LOCAL_ADD_TTL);
  } else {
    ttl = (u8_t)(rt->hcnt + LOCAL_ADD_TTL);
  }
  
  LWIP_DEBUGF(AODV_DEBUG, ("aodv_rreq_local_repair: %s, rreq ttl=%d, dest_hcnt=%d\n",
              inet_ntoa_r(rt->dest_addr, buffer, INET_ADDRSTRLEN), ttl, rt->hcnt));
              
  /* Reset the timeout handler, was probably previously
     local_repair_timeout */
  rt->rt_timer.handle = aodv_route_expire_timeout;

  if (rt->rt_timer.msec < (2 * NET_TRAVERSAL_TIME)) {
    aodv_rt_update_timeout(rt, 2 * NET_TRAVERSAL_TIME);
  }
  
  aodv_rreq_send(&rt->dest_addr, rt->dest_seqno, ttl, flags);
  
  /* Remember that we are seeking this destination and setup the
     timers */
  seek_entry = aodv_seeklist_insert(&rt->dest_addr, rt->dest_seqno,
                                    ttl, flags, p, 0, NULL, NULL);
  LWIP_ERROR("seek_entry != NULL", (seek_entry != NULL), return;);
                                    
  if (aodv_expanding_ring_search_get()) {
    aodv_timer_set_timeout(&seek_entry->seek_timer,
                           2 * ttl * NODE_TRAVERSAL_TIME);
  } else {
    aodv_timer_set_timeout(&seek_entry->seek_timer, NET_TRAVERSAL_TIME);
  }
  
  LWIP_DEBUGF(AODV_DEBUG, ("aodv_rreq_local_repair: Seeking %s ttl=%d\n", 
              inet_ntoa_r(rt->dest_addr, buffer, INET_ADDRSTRLEN), ttl));
}

/**
 * find a received RREQ's data structure
 *
 * @param orig_addr originator address
 * @param rreq_id source address
 * @return a data structure to buffer information
 */
struct aodv_rreq_record *aodv_rreq_record_find (struct in_addr *orig_addr, u32_t rreq_id)
{
  struct aodv_rreq_record *rec;
  
  for (rec = record_list; rec != NULL; rec = rec->next) {
    if ((rec->orig_addr.s_addr == orig_addr->s_addr) && 
        (rec->rreq_id == rreq_id)) {
      return (rec);
    }
  }
  
  return NULL;
}

/**
 * received RREQ's data structure timeout function
 *
 * @param arg data structure
 */
void aodv_rreq_record_timeout (void *arg)
{
  struct aodv_rreq_record *rec = (struct aodv_rreq_record *)arg;
  
  __AODV_LIST_DELETE(rec, record_list);

  mem_free(rec);
}

/**
 * insert a received RREQ's
 *
 * @param orig_addr originator address
 * @param rreq_id source address
 * @return a data structure to buffer information
 */
struct aodv_rreq_record *aodv_rreq_record_insert (struct in_addr *orig_addr, u32_t rreq_id)
{
#if AODV_DEBUG
  char buffer[INET_ADDRSTRLEN];
#endif

  struct aodv_rreq_record *rec;

  /* First check if this rreq packet is already buffered */
  rec = aodv_rreq_record_find(orig_addr, rreq_id);
  if (rec) {
    return (rec);
  }
  
  rec = (struct aodv_rreq_record *)mem_malloc(sizeof(struct aodv_rreq_record));
  
  LWIP_ERROR("rec != NULL", (rec != NULL), return NULL;);
  
  rec->orig_addr = *orig_addr;
  rec->rreq_id = rreq_id;
  
  aodv_timer_init(&rec->rec_timer, aodv_rreq_record_timeout, rec);

  __AODV_LIST_INSERT(rec, record_list);
  
  LWIP_DEBUGF(AODV_DEBUG, ("aodv_rreq_record_insert: Buffering RREQ %s rreq_id=%d time=%u\n", 
              inet_ntoa_r(*orig_addr, buffer, INET_ADDRSTRLEN), rreq_id, PATH_DISCOVERY_TIME));
              
  aodv_timer_set_timeout(&rec->rec_timer, PATH_DISCOVERY_TIME);
  
  return (rec);
}

/**
 * find a blacklist data structure
 *
 * @param dest_addr blacklist address
 * @return a blacklist structure
 */
struct aodv_blacklist *aodv_rreq_blacklist_find (struct in_addr *dest_addr)
{
  struct aodv_blacklist *bl;
  
  for (bl = black_list; bl != NULL; bl = bl->next) {
    if (bl->dest_addr.s_addr == dest_addr->s_addr) {
      return (bl);
    }
  }
  
  return NULL;
}

/**
 * blacklist data structure timeout function
 *
 * @param arg blacklist data structure
 */
void aodv_rreq_blacklist_timeout (void *arg)
{
  struct aodv_blacklist *bl = (struct aodv_blacklist *)arg;
  
  __AODV_LIST_DELETE(bl, black_list);
  
  mem_free(bl);
}

/**
 * insert a blacklist data structure
 *
 * @param dest_addr blacklist address
 * @return a blacklist structure
 */
struct aodv_blacklist *aodv_rreq_blacklist_insert (struct in_addr *dest_addr)
{
  struct aodv_blacklist *bl;
  
  /* First check if this bl is already buffered */
  bl = aodv_rreq_blacklist_find(dest_addr);
  if (bl) {
    return (bl);
  }
  
  bl = (struct aodv_blacklist *)mem_malloc(sizeof(struct aodv_blacklist));
  
  LWIP_ERROR("bl != NULL", (bl != NULL), return NULL;);
  
  bl->dest_addr = *dest_addr;
  
  aodv_timer_init(&bl->bl_timer, aodv_rreq_blacklist_timeout, bl);
  
  __AODV_LIST_INSERT(bl, black_list);
  
  aodv_timer_set_timeout(&bl->bl_timer, BLACKLIST_TIMEOUT);
  
  return (bl);
}
/* end */
