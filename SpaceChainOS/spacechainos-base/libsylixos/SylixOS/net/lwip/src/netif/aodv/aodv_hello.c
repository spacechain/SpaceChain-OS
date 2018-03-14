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

#include "aodv_route.h"
#include "aodv_timer.h"
#include "aodv_proto.h"
#include "aodv_hello.h"
#include "aodv_if.h"

#include <string.h>

extern int aodv_wait_on_reboot_get(void);

#define HELLO_DELAY     (HELLO_INTERVAL >> 1)  /* The extra time we should allow an hello
                                                  message to take (due to processing) before
                                                  assuming lost . */
#define HELLO_RAND()    (LWIP_RAND() % HELLO_DELAY)

static struct aodv_timer hello_timer; /* global hello timer */

/**
 * start global hello timer
 * 
 */
void aodv_hello_start (void)
{
  if (__AODV_TIMER_ISINQ(&hello_timer)) {
    return;
  }
  
  LWIP_DEBUGF(AODV_DEBUG, ("aodv_hello_start: Starting to send HELLOs!\n"));
  
  aodv_timer_init(&hello_timer, aodv_hello_send, NULL);

  aodv_timer_set_timeout(&hello_timer, HELLO_INTERVAL + HELLO_RAND());
}

/**
 * stop global hello timer
 * 
 */
void aodv_hello_stop (void)
{
  LWIP_DEBUGF(AODV_DEBUG, ("aodv_hello_stop: No active forwarding routes - stopped sending HELLOs!\n"));

  aodv_timer_remove(&hello_timer);
}

/**
 * reset hello timer
 * 
 */
void aodv_hello_reset (void)
{
  aodv_timer_set_timeout(&hello_timer, HELLO_INTERVAL + HELLO_RAND());
}

/**
 * send a hello packet
 * 
 * @param arg not used!
 */
void aodv_hello_send (void *arg)
{
  int i, j;
  struct pbuf *p;
  struct in_addr addr;
  struct in_addr dest_broadcast;
  
  struct aodv_ext *ext = NULL;
  u16_t ext_size = 0;
  
  if (aodv_wait_on_reboot_get()) {
    goto __reset_timer;
  }
  
  /* RFC3561
   *
   * dest ip    == local ip
   * dest seqno == aodv_host_state.seqno
   * hops       == 0
   * lifetime   == ALLOWED_HELLO_LOSS * HELLO_INTERVAL
   *  
   */
  dest_broadcast.s_addr = INADDR_BROADCAST; /* broadcast */
  
#if AODV_HELLO_NEIGHBOR
  /* extension which contain our neighbor set */
  for (j = 0; j < AODV_RT_TABLESIZE; j++) {
    struct aodv_rtnode *rt;
    for (rt  = aodv_rt_tbl.hash_tbl[j];
         rt != NULL;
         rt  = rt->next) {
      /* If an entry has an active hello timer, we assume
         that we are receiving hello messages from that
         node... */
      if (__AODV_TIMER_ISINQ(&rt->hello_timer)) {
        ext_size += sizeof(struct in_addr);
        if (ext_size >= 252) { /* max save 63 node */
          break;
        }
      }
    }
  }
  
  if (ext_size) { /* have RREP_HELLO_NEIGHBOR_SET_EXT extern */
    ext_size += AODV_EXT_HDR_SIZE;
  }
#else
  (void)j; /* no warning */
#endif /* AODV_HELLO_NEIGHBOR */
  
  /* extension which contain hello interval */
  ext_size += (AODV_EXT_HDR_SIZE + 4);
  
  /* TODO: in lwip broadcast packet is always used defaute netif output, 
   * so adov netif must be default one! 
   */
  for (i = 0; i < AODV_MAX_NETIF; i++) {
    if (aodv_netif[i]) {
      addr.s_addr = netif_ip4_addr(aodv_netif[i])->addr;
      p = aodv_rrep_create(0, 0, 0, &addr, aodv_host_state.seqno, &addr, 
                           ALLOWED_HELLO_LOSS * HELLO_INTERVAL, ext_size, NULL);
      if (p) {
        u32_t interval = htonl(HELLO_INTERVAL);
        struct aodv_rrep *rrep = (struct aodv_rrep *)p->payload;
        
        ext = (struct aodv_ext *)((char *)rrep + RREP_SIZE);
        
#if AODV_HELLO_NEIGHBOR
        ext->type = RREP_HELLO_NEIGHBOR_SET_EXT;
        ext->length = 0;
        
        /* Assemble a RREP extension which contain our neighbor set... */
        for (j = 0; j < AODV_RT_TABLESIZE; j++) {
          struct aodv_rtnode *rt;
          for (rt  = aodv_rt_tbl.hash_tbl[j];
               rt != NULL;
               rt  = rt->next) {
            if (__AODV_TIMER_ISINQ(&rt->hello_timer)) {
              memcpy((AODV_EXT_DATA(ext) + ext->length),
                     &rt->dest_addr, sizeof(struct in_addr));
              ext->length += sizeof(struct in_addr);
              if (ext->length >= 252) { /* max save 63 node */
                break;
              }
            }
          }
        }
        
        /* add hello interval extern */
        if (ext->length != 0) { /* have RREP_HELLO_NEIGHBOR_SET_EXT extern */
          ext = AODV_EXT_NEXT(ext);
        }
#endif /* AODV_HELLO_NEIGHBOR */
        
        ext->type = RREP_HELLO_INTERVAL_EXT;
        ext->length = 4;
        memcpy(AODV_EXT_DATA(ext), &interval, 4);
        
        aodv_udp_sendto(p, &dest_broadcast, 1, i);
        pbuf_free(p);
      }
    }
  }
  
__reset_timer:
  aodv_timer_set_timeout(&hello_timer, HELLO_INTERVAL + HELLO_RAND());
}

/* Process a hello message 
 *
 * @param p hello packet
 * @param netif the network interface on which the packet was received
 */
void aodv_hello_process (struct pbuf *p, struct netif *netif)
{
#if AODV_DEBUG
  char buffer[INET_ADDRSTRLEN];
#endif /* AODV_DEBUG */

  u32_t now = sys_now();
  u32_t hello_seqno, timeout, hello_interval = HELLO_INTERVAL;
  struct in_addr ext_neighbor, hello_dest;
  u8_t state, flags = 0;
  struct aodv_rrep *rrep;
  int rreplen;
  struct aodv_rtnode *rt;
  struct aodv_ext *ext = NULL;
  struct pbuf *psingle = NULL; /* pbuf tot_len == len */

  LWIP_ERROR("p != NULL", (p != NULL), return;);
  LWIP_ERROR("p->len >= RREP_SIZE", (p->len >= RREP_SIZE), return;);
  
  if (p->tot_len != p->len) { /* p is splite buffer */
    psingle = pbuf_alloc(PBUF_RAW, p->tot_len, PBUF_RAM); /* allocate a single buffer */
    LWIP_ERROR("psingle != NULL", (psingle != NULL), return;);
    pbuf_copy(psingle, p);
    rrep = (struct aodv_rrep *)psingle->payload;
  } else {
    rrep = (struct aodv_rrep *)p->payload;
  }
  
  rreplen = p->tot_len;

  hello_dest.s_addr = rrep->dest_addr;
  hello_seqno = ntohl(rrep->dest_seqno);
  
  rt = aodv_rt_find(&hello_dest);
  
  if (rt) {
    flags = (u8_t)rt->flags;
  }
  
  flags |= AODV_RT_UNIDIR; /* uni-directional */
  
  /* Check for hello interval extension: */
  ext = (struct aodv_ext *)((char *)rrep + RREP_SIZE);
  while (rreplen > (int)RREP_SIZE) {
    int i;
    switch (ext->type) {
    
    case RREP_HELLO_INTERVAL_EXT:
      if (ext->length == 4) {
        memcpy(&hello_interval, AODV_EXT_DATA(ext), 4);
        hello_interval = ntohl(hello_interval);
      }
      break;
      
    case RREP_HELLO_NEIGHBOR_SET_EXT:
      for (i = 0; i < ext->length; i += sizeof(struct in_addr)) {
        memcpy(&ext_neighbor, (AODV_EXT_DATA(ext) + i), sizeof(struct in_addr));
        if (ext_neighbor.s_addr == netif_ip4_addr(netif)->addr) {
          flags &= ~AODV_RT_UNIDIR;
        }
      }
      break;
    }
    rreplen -= (u16_t)AODV_EXT_SIZE(ext);
    ext = AODV_EXT_NEXT(ext);
  }
  
  if (psingle) {
    pbuf_free(psingle); /* no use of psingle */
  }
  
  if (AODV_RECV_N_HELLOS > 1) {
    state = AODV_INVALID;
  } else {
    state = AODV_VALID;
  }
  
  timeout = ALLOWED_HELLO_LOSS * hello_interval + ROUTE_TIMEOUT_SLACK;
  
  if (!rt) {
    /* No active or expired route in the routing table. So we add a
       new entry... */
    rt = aodv_rt_insert(&hello_dest, &hello_dest, 1,
                        hello_seqno, timeout, state, flags, netif);

    LWIP_ERROR("rt != NULL", (rt != NULL), return;);
                        
    if (flags & AODV_RT_UNIDIR) {
      LWIP_DEBUGF(AODV_DEBUG, ("aodv_hello_process: %s new NEIGHBOR, link UNI-DIR!\n", 
                                inet_ntoa_r(rt->dest_addr, buffer, INET_ADDRSTRLEN)));
    } else {
      LWIP_DEBUGF(AODV_DEBUG, ("aodv_hello_process: %s new NEIGHBOR!\n", 
                                inet_ntoa_r(rt->dest_addr, buffer, INET_ADDRSTRLEN)));
    }
    rt->hello_cnt = 1;
    rt->last_hello_time = now;
    
  } else {
    if (rt->hello_cnt < AODV_RECV_N_HELLOS) {
      u32_t  diff;
      /* Calculate this time difference with the last hello */
      if (now >= rt->last_hello_time) {
        diff = now - rt->last_hello_time;
      } else {
        diff = now + ((u32_t)(~0) - rt->last_hello_time) + 1;
      }
      /* The time difference in 3 * HELLO_INTERVAL */
      if (diff < (hello_interval * 3)) {
        rt->hello_cnt++;
      } else {
        rt->hello_cnt = 1;
      }
      
      rt->last_hello_time = now;
      return;
    
    } else {
      if ((flags & AODV_RT_UNIDIR) && !(rt->flags & AODV_RT_UNIDIR)) {
        /* this route may not a uni-directional link, it may create by RREQ -> RREP -> RREP-ACK
         * so we do not update this route.
         */
      } else {
        aodv_rt_update(rt, &hello_dest, 1, hello_seqno, 
                       ALLOWED_HELLO_LOSS * HELLO_INTERVAL, AODV_VALID, flags);
      }
    }
  }
  
  aodv_hello_update_timeout(rt, ALLOWED_HELLO_LOSS * (long)hello_interval);
}

/**
 * update route entry node hello timer timeout
 * 
 * @param rt route entry node 
 * @param now last_hello_time
 * @param time new hello timer timeout
 */
void aodv_hello_update_timeout (struct aodv_rtnode *rt, long time)
{
  aodv_timer_set_timeout(&rt->hello_timer, (u32_t)(time + HELLO_DELAY));
  rt->last_hello_time = sys_now();
}
/* end */
