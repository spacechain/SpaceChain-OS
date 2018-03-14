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
#include "aodv_param.h"
#include "aodv_route.h"
#include "aodv_timer.h"
#include "aodv_timercb.h"
#include "aodv_seeklist.h"

#include "string.h"

static struct aodv_seeklist *seek_list; /* The seek list is a linked list of destinations we are seeking (with RREQ's). */

/**
 * insert seek point into list
 *
 * @param dest_addr address of the destination.
 * @param dest_seqno destination sequence number
 * @param ttl ip ttl 
 * @param flags The flags we are using for resending the RREQ
 * @param p packet buffer (seeklist will copy this packet and save)
 * @param grp_hcnt grp hop cnt
 * @param grp_addr group address
 * @param rev_addr if p is need forward, rev_addr mean who forward this packet to me.
 * @return seeklist entry node
 */
struct aodv_seeklist *aodv_seeklist_insert (struct in_addr *dest_addr,
                                            u32_t dest_seqno,
                                            u8_t ttl, u8_t flags,
                                            struct pbuf *p,
                                            u8_t grp_hcnt,
                                            struct in_addr *grp_addr,
                                            struct in_addr *rev_addr)
{
  struct aodv_seeklist *newseek;
  struct pbuf *psave;
  
  newseek = (struct aodv_seeklist *)mem_malloc(sizeof(struct aodv_seeklist));
  if (!newseek) {
    LWIP_DEBUGF(AODV_DEBUG, ("aodv_seeklist_insert: mem_malloc failed!\n"));
    return NULL;
  }
  
  newseek->dest_addr = *dest_addr;
  newseek->dest_seqno = dest_seqno;
  newseek->flags = flags;
  newseek->reqs = 0;
  newseek->ttl = ttl;
  newseek->grp_hcnt = grp_hcnt;
  
  if (grp_addr) {
    newseek->grp_addr = *grp_addr;
  } else {
    newseek->grp_addr.s_addr = INADDR_ANY;
  }
  
  if (rev_addr) {
    newseek->rev_addr = *rev_addr;
  } else {
    newseek->rev_addr.s_addr = INADDR_ANY;
  }
  
  if (p) {
    psave = pbuf_alloc(PBUF_LINK, p->tot_len, PBUF_RAM); /* add room for link layer header */
    if (psave != NULL) {
      if (pbuf_copy(psave, p) != ERR_OK) {
        pbuf_free(psave);
        mem_free(newseek);
        LWIP_DEBUGF(AODV_DEBUG, ("aodv_seeklist_insert: pbuf_copy() failed!\n"));
        return NULL;
      }
    }
    newseek->p = psave;
  } else {
    newseek->p = NULL; /* no packet to be send */
  }
  
  aodv_timer_init(&newseek->seek_timer, &aodv_route_discovery_timeout, newseek);

  __AODV_LIST_INSERT(newseek, seek_list);
  
  return (newseek);
}

/**
 * remove a seek point from list
 *
 * @param seek seek point
 * @return 1: remove ok, 0: remove failed
 */
int aodv_seeklist_remove (struct aodv_seeklist *seek)
{
  LWIP_ERROR("seek != NULL", (seek != NULL), return 0;);
  
  __AODV_LIST_DELETE(seek, seek_list);
  
  aodv_timer_remove(&seek->seek_timer);
  
  if (seek->p) {
    pbuf_free(seek->p);
  }
  
  mem_free(seek);
  
  return 1;
}

/**
 * find a seek point in list
 *
 * @param dest_addr address of the destination.
 * @return seek point
 */
struct aodv_seeklist *aodv_seeklist_find (struct in_addr *dest_addr)
{
  struct aodv_seeklist *s;
  
  for (s = seek_list; s != NULL; s = s->next) {
    if (s->dest_addr.s_addr == dest_addr->s_addr) {
      return (s);
    }
  }
  
  return NULL;
}

#if AODV_MCAST
/**
 * remove a seek point from list (mach grp address)
 *
 * @param dest_addr  dest addr
 * @param grp_addr  group addr
 * @return 1: remove ok, 0: remove failed
 */
int aodv_seeklist_remove_mcast (struct in_addr *dest_addr, struct in_addr *grp_addr)
{
  struct aodv_seeklist *s;
  
  LWIP_ERROR("dest_addr != NULL", (dest_addr != NULL), return 0;);
  LWIP_ERROR("grp_addr != NULL", (grp_addr != NULL), return 0;);
  
  for (s = seek_list; s != NULL; s = s->next) {
    if ((s->dest_addr.s_addr == dest_addr->s_addr) && 
        (s->grp_addr.s_addr  == grp_addr->s_addr)) {
      return aodv_seeklist_remove(s);
    }
  }
  
  return 0;
}

/**
 * find a seek point in list
 *
 * @param dest_addr address of the destination.
 * @return seek point
 */
struct aodv_seeklist *aodv_seeklist_find_mcast (struct in_addr *dest_addr, struct in_addr *grp_addr)
{
  struct aodv_seeklist *s;
  
  for (s = seek_list; s != NULL; s = s->next) {
    if ((s->dest_addr.s_addr == dest_addr->s_addr) && 
        (s->grp_addr.s_addr  == grp_addr->s_addr)) {
      return (s);
    }
  }
  
  return NULL;
}

#endif /* AODV_MCAST */

/* end */
