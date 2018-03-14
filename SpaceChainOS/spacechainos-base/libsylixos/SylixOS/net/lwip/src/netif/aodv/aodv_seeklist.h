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
#ifndef __AODV_SEEKLIST_H
#define __AODV_SEEKLIST_H

#include "lwip/opt.h"

#include "aodv_param.h"

/* This is a list of nodes that route discovery are performed for */
typedef struct aodv_seeklist {
  struct aodv_seeklist *next;
  struct aodv_seeklist *prev;
  
  struct in_addr dest_addr;
  u32_t dest_seqno;
  u8_t flags; /* The flags we are using for resending the RREQ */
  u8_t reqs;
  u8_t ttl;
  u8_t grp_hcnt;
  
  struct aodv_timer seek_timer;
  
  /* we only buffered one packet to conserve memory */
  struct in_addr grp_addr;
  struct in_addr rev_addr; /* who forward multicast packet to me, (May not be original sender, so we must prevent loop)*/
  struct pbuf *p;
} aodv_seeklist_t;

struct aodv_seeklist *aodv_seeklist_insert(struct in_addr *dest_addr,
                                           u32_t dest_seqno,
                                           u8_t ttl, u8_t flags,
                                           struct pbuf *p,
                                           u8_t grp_hcnt,
                                           struct in_addr *grp_addr,
                                           struct in_addr *rev_addr);
int aodv_seeklist_remove(struct aodv_seeklist *seek);
struct aodv_seeklist *aodv_seeklist_find(struct in_addr *dest_addr);

#if AODV_MCAST
int aodv_seeklist_remove_mcast(struct in_addr *dest_addr, struct in_addr *grp_addr);
struct aodv_seeklist *aodv_seeklist_find_mcast(struct in_addr *dest_addr, struct in_addr *grp_addr);
#endif /* AODV_MCAST */

#endif /* __AODV_SEEKLIST_H */
