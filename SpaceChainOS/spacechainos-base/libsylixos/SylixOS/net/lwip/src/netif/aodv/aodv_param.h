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
#ifndef __AODV_PARAM_H
#define __AODV_PARAM_H

#ifndef max
#define max(a, b)                       ((a) > (b) ? (a) : (b))
#endif

/* from RFC3561 */
#define ACTIVE_ROUTE_TIMEOUT            3000
#define ALLOWED_HELLO_LOSS              3  /* rfc3561 2 is recommend, but in wireless 3 is better */
#define BLACKLIST_TIMEOUT               (RREQ_RETRIES * NET_TRAVERSAL_TIME)
#define DELETE_PERIOD                   (5 * max(ACTIVE_ROUTE_TIMEOUT, HELLO_INTERVAL))
#define HELLO_INTERVAL                  1000
#define LOCAL_ADD_TTL                   2
#define MAX_REPAIR_TTL                  (3 * NET_DIAMETER / 10)
#define MIN_REPAIR_TTL                  1
#define MY_ROUTE_TIMEOUT                (2 * ACTIVE_ROUTE_TIMEOUT)
#define NET_DIAMETER                    35 /* max hops */
#define NET_TRAVERSAL_TIME              (2 * NODE_TRAVERSAL_TIME * NET_DIAMETER)
#define NEXT_HOP_WAIT                   (NODE_TRAVERSAL_TIME + 10)
#define NODE_TRAVERSAL_TIME             40
#define PATH_DISCOVERY_TIME             (2 * NET_TRAVERSAL_TIME)
#define RERR_RATELIMIT                  10
#define RING_TRAVERSAL_TIME             (2 * NODE_TRAVERSAL_TIME * (TTL_VALUE + TIMEOUT_BUFFER))
#define RREQ_RETRIES                    2
#define RREQ_RATELIMIT                  10
#define TIMEOUT_BUFFER                  2
#define TTL_START                       2  /* must at least == 2 */
#define TTL_INCREMENT                   2
#define TTL_THRESHOLD                   7

/* multicast support */
#if LWIP_IGMP
#define AODV_MCAST                      1
#else
#define AODV_MCAST                      0
#endif /* LWIP_IGMP */

#if AODV_MCAST
#define GROUP_HELLO_INTERVAL            5000
#define RREP_WAIT_TIME                  (3 * NODE_TRAVERSAL_TIME * NET_DIAMETER)
#define PRUNE_TIMEOUT                   (2 * RREP_WAIT_TIME)
#endif /* AODV_MCAST */

/* timer */
#define AODV_TIMER_PRECISION            40

/* interface */
#define AODV_MAX_NETIF                  1

/* receive n hellos considered a neighbor*/
#define AODV_RECV_N_HELLOS              2  /* must bigger than 1 */
#define AODV_HELLO_NEIGHBOR             1  /* hello packet has neighbor extern data */

/* Whether or not enable the debug. */
#define AODV_DEBUG                      LWIP_DBG_OFF /* set LWIP_DBG_ON if you want print debug message */

#endif /* __AODV_PARAM_H */
