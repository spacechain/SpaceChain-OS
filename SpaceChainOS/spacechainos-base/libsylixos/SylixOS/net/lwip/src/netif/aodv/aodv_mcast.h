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

#ifndef __AODV_MCAST_H
#define __AODV_MCAST_H

#include "aodv_param.h"

#if AODV_MCAST

#include "aodv_seeklist.h"

void aodv_mcast_try_send(struct in_addr *grp_addr, struct netif *netif, struct pbuf *p);
void aodv_mcast_join(struct in_addr *grp_addr, struct netif *netif);
void aodv_mcast_leave(struct in_addr *grp_addr);
void aodv_mcast_query(struct netif *netif);
void aodv_mcast_rrep_process(struct pbuf *p, 
                             struct in_addr *ip_src,
                             struct in_addr *ip_dst, 
                             u8_t ttl,
                             struct netif *netif);
void aodv_mcast_rreq_process(struct pbuf *p, 
                             struct in_addr *ip_src,
                             struct in_addr *ip_dst, 
                             u8_t ttl,
                             struct netif *netif,
                             GRP_REBUILD_EXT *grp_ext);
int aodv_mcast_rreq_retries(struct aodv_seeklist *seek_entry);

#endif /* AODV_MCAST */

#endif /* __AODV_MCAST_H */
