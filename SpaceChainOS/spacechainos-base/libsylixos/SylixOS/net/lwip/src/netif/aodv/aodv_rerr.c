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
#include "aodv_if.h"

/**
 * create a rerr packet (pbuf)
 *
 * @param flags packet flags
 * @param dest_addr destination address
 * @param dest_seqno destination sequence number 
 * @return rerr packet
 */
struct pbuf *aodv_rerr_create (u8_t flags, struct in_addr *dest_addr,
                               u32_t dest_seqno)
{
  struct pbuf *p;
  struct aodv_rerr *rerr;
  
  p = pbuf_alloc(PBUF_TRANSPORT, RERR_SIZE, PBUF_RAM);
  if (p) {
    rerr = (struct aodv_rerr *)p->payload;
    rerr->type = AODV_RERR;
    rerr->flags = flags;
    rerr->rsvd = 0;
    rerr->dest_count = 1;
    rerr->dest_addr = dest_addr->s_addr;
    rerr->dest_seqno = htonl(dest_seqno);
  } else {
    LWIP_DEBUGF(AODV_DEBUG, ("aodv_rerr_create: can not allocate rerr packet!\n"));
  }
  
  return p;
}

/**
 * add a rerr udest in rerr packet (pbuf)
 *
 * @param p rerr packet flags
 * @param udest destination address
 * @param udest_seqno destination sequence number 
 */
void aodv_rerr_add_udest (struct pbuf *p, struct in_addr *udest,
                          u32_t udest_seqno)
{
  struct aodv_rerr *rerr;
  struct aodv_rerr_udest *rerr_udest;
  struct pbuf *p_udest;
  
  LWIP_ERROR("p != NULL", (p != NULL), return;);

  p_udest = pbuf_alloc(PBUF_RAW, RERR_UDEST_SIZE, PBUF_RAM);
  
  LWIP_ERROR("p_udest != NULL", (p_udest != NULL), return;);
  
  rerr_udest = (struct aodv_rerr_udest *)p_udest->payload;
  rerr_udest->dest_addr = udest->s_addr;
  rerr_udest->dest_seqno = htonl(udest_seqno);
  
  rerr = (struct aodv_rerr *)p->payload;
  rerr->dest_count++;
  
  pbuf_cat(p, p_udest);
}

/**
 * rerr packet process
 *
 * @param p rerr packet flags
 * @param ip_src source address
 * @param ip_dst destination address
 */
void aodv_rerr_process (struct pbuf *p, struct in_addr *ip_src, struct in_addr *ip_dst)
{
#if AODV_DEBUG
  char buffer[INET_ADDRSTRLEN];
#endif

  int i, len, dest_count, offset;
  struct aodv_rtnode *rt;
  u32_t rerr_dest_seqno;
  struct in_addr udest_addr, rerr_unicast_dest;
  
  struct pbuf *p_new = NULL;
  int dest_count_new = 0;
  struct aodv_rerr *rerr;
  struct aodv_rerr_udest udest;
  
  rerr_unicast_dest.s_addr = INADDR_ANY;
  
  LWIP_ERROR("p->len >= RERR_SIZE", (p->len >= RERR_SIZE), return;);
  LWIP_ERROR("p->len == p->tot_len", (p->len == p->tot_len), return;); /* must in a no splite pbuf */
  
  rerr = (struct aodv_rerr *)p->payload;
  
  len  = p->tot_len;
  len -= RERR_SIZE;
  dest_count = len / RERR_UDEST_SIZE;
  
  offset = RERR_SIZE;
  
  /* Check which destinations that are unreachable.  */
  for (i = 0; i < dest_count; i++) {
    pbuf_copy_partial(p, (void *)&udest, RERR_SIZE, (u16_t)offset);
    
    udest_addr.s_addr = udest.dest_addr;
    rerr_dest_seqno = ntohl(udest.dest_seqno);
    
    LWIP_DEBUGF(AODV_DEBUG, ("aodv_rerr_process: unreachable dest=%s seqno=%lu\n",
          inet_ntoa_r(udest_addr, buffer, INET_ADDRSTRLEN), rerr_dest_seqno));
          
    rt = aodv_rt_find(&udest_addr);
    
    if (rt && rt->state == AODV_VALID && rt->next_hop.s_addr == ip_src->s_addr) {
      /* Checking sequence numbers here is an out of draft
       * addition to AODV-UU. It is here because it makes a lot
       * of sense... */
      if (0 && ((s32_t) rt->dest_seqno > (s32_t)rerr_dest_seqno)) {
        LWIP_DEBUGF(AODV_DEBUG, ("aodv_rerr_process: Udest ignored because of seqno!\n"));
        offset += RERR_UDEST_SIZE;
        continue;
      }
      
      LWIP_DEBUGF(AODV_DEBUG, ("aodv_rerr_process: removing rte %s - WAS IN RERR!\n",
          inet_ntoa_r(udest_addr, buffer, INET_ADDRSTRLEN)));
          
      /* Invalidate route: */
      if ((rerr->flags & RERR_NODELETE) == 0) {
        aodv_rt_invalidate(rt);
      }

      /* (a) updates the corresponding destination sequence number
             with the Destination Sequence Number in the packet, and */
      rt->dest_seqno = rerr_dest_seqno;
      
      /* (d) check precursor list for emptiness. If not empty, include
             the destination as an unreachable destination in the
             RERR... */
      if (rt->nprec && !(rt->flags & AODV_RT_REPAIR)) {
        if (p_new == NULL) {
          u8_t flags = 0;
          
          if (rerr->flags & RERR_NODELETE) {
            flags |= RERR_NODELETE;
          }
          
          p_new = aodv_rerr_create(flags, &rt->dest_addr, rt->dest_seqno);
          dest_count_new = 1;
          LWIP_ERROR("p_new != NULL", (p_new != NULL), return;);
          
          LWIP_DEBUGF(AODV_DEBUG, ("aodv_rerr_process: Added %s as unreachable, seqno=%lu\n",
              inet_ntoa_r(rt->dest_addr, buffer, INET_ADDRSTRLEN), rt->dest_seqno));
              
          if (rt->nprec == 1) {
            rerr_unicast_dest = rt->precursors->neighbor;
          }
          
        } else {
          /* Decide whether new precursors make this a non unicast RERR */
          aodv_rerr_add_udest(p_new, &rt->dest_addr, rt->dest_seqno);
          dest_count_new++;
          
          LWIP_DEBUGF(AODV_DEBUG, ("aodv_rerr_process: Added %s as unreachable, seqno=%lu\n",
              inet_ntoa_r(rt->dest_addr, buffer, INET_ADDRSTRLEN), rt->dest_seqno));
              
          if (rerr_unicast_dest.s_addr != INADDR_ANY) {
            struct aodv_precursor *pr;
            
            for (pr = rt->precursors; pr != NULL; pr = pr->next) {
              if (pr->neighbor.s_addr != rerr_unicast_dest.s_addr) {
                rerr_unicast_dest.s_addr = INADDR_ANY; /* we need broadcast */
                break;
              }
            }
          }
        }
      } else {
        LWIP_DEBUGF(AODV_DEBUG, 
              ("aodv_rerr_process: Not sending RERR, no precursors or route in RT_REPAIR!\n"));
      }
      
      /* We should delete the precursor list for all unreachable
         destinations. */
      if (rt->state == AODV_INVALID) {
        aodv_precursor_list_destroy(rt);
      }
    } else {
      LWIP_DEBUGF(AODV_DEBUG, ("aodv_rerr_process: Ignoring UDEST %s\n", 
                                inet_ntoa_r(udest_addr, buffer, INET_ADDRSTRLEN)));
    }
    
    offset += RERR_UDEST_SIZE; /* continue */
  }
  
  /* If a RERR was created, then send it now... */
  if (p_new) {
    rt = aodv_rt_find(&rerr_unicast_dest);
    
    if (rt && dest_count_new == 1 && rerr_unicast_dest.s_addr) {
    
      aodv_udp_sendto(p_new, &rerr_unicast_dest, 1, aodv_rt_netif_index_get(rt));
    
    } else if (dest_count_new > 0) {
    
      struct in_addr dest_broadcast;
      
      dest_broadcast.s_addr = INADDR_BROADCAST;
      
      for (i = 0; i < AODV_MAX_NETIF; i++) {
        if (aodv_netif[i]) {
          aodv_udp_sendto(p_new, &dest_broadcast, 1, i);
        }
      }
    }
    
    pbuf_free(p_new);
  }
}
/* end */
