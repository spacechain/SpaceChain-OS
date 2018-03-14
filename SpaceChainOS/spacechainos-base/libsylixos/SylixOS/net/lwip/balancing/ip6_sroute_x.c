/**
 * @file
 * Lwip platform independent net ipv6 source route.
 * as much as possible compatible with different versions of LwIP
 * Verification using sylixos(tm) real-time operating system
 */

/*
 * Copyright (c) 2006-2017 SylixOS Group.
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
 * Author: Han.hui <hanhui@acoinfo.com>
 *
 */

#define __SYLIXOS_KERNEL
#include "SylixOS.h"

#if LW_CFG_NET_ROUTER > 0 && LW_CFG_NET_BALANCING > 0

#include "lwip/inet.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "net/sroute.h"
#include "ip6_sroute.h"

#if LWIP_IPV6

/* srt6_entry to struct srtentry */
void srt6_sentry_to_srtentry (struct srtentry *srtentry, const struct srt6_entry *sentry6)
{
  ip6_addr_t ip6ssrc, ip6esrc;
  ip6_addr_t ip6sdest, ip6edest;
  
  srtentry->srt_ssrc.sa_len = sizeof(struct sockaddr_in6);
  srtentry->srt_ssrc.sa_family = AF_INET6;
  srtentry->srt_esrc.sa_len = sizeof(struct sockaddr_in6);
  srtentry->srt_esrc.sa_family = AF_INET6;
  srtentry->srt_sdest.sa_len = sizeof(struct sockaddr_in6);
  srtentry->srt_sdest.sa_family = AF_INET6;
  srtentry->srt_edest.sa_len = sizeof(struct sockaddr_in6);
  srtentry->srt_edest.sa_family = AF_INET6;
  
  srt6_addr_hton(&ip6ssrc, &sentry6->srt6_ssrc_hbo);
  srt6_addr_hton(&ip6esrc, &sentry6->srt6_esrc_hbo);
  srt6_addr_hton(&ip6sdest, &sentry6->srt6_sdest_hbo);
  srt6_addr_hton(&ip6edest, &sentry6->srt6_edest_hbo);

  inet6_addr_from_ip6addr(&((struct sockaddr_in6 *)&srtentry->srt_ssrc)->sin6_addr, &ip6ssrc);
  inet6_addr_from_ip6addr(&((struct sockaddr_in6 *)&srtentry->srt_esrc)->sin6_addr, &ip6esrc);
  inet6_addr_from_ip6addr(&((struct sockaddr_in6 *)&srtentry->srt_sdest)->sin6_addr, &ip6sdest);
  inet6_addr_from_ip6addr(&((struct sockaddr_in6 *)&srtentry->srt_edest)->sin6_addr, &ip6edest);
  
  lib_strlcpy(srtentry->srt_ifname, sentry6->srt6_ifname, IF_NAMESIZE);
  
  srtentry->srt_flags = (u_short)sentry6->srt6_flags;
  srtentry->srt_mode  = sentry6->srt6_mode;
  srtentry->srt_prio  = sentry6->srt6_prio;
}

/* struct srtentry to srt6_entry */
void srt6_srtentry_to_sentry (struct srt6_entry *sentry6, const struct srtentry *srtentry)
{
  ip6_addr_t ip6ssrc, ip6esrc;
  ip6_addr_t ip6sdest, ip6edest;

  inet6_addr_to_ip6addr(&ip6ssrc, &((struct sockaddr_in6 *)&srtentry->srt_ssrc)->sin6_addr);
  inet6_addr_to_ip6addr(&ip6esrc, &((struct sockaddr_in6 *)&srtentry->srt_esrc)->sin6_addr);
  inet6_addr_to_ip6addr(&ip6sdest, &((struct sockaddr_in6 *)&srtentry->srt_sdest)->sin6_addr);
  inet6_addr_to_ip6addr(&ip6edest, &((struct sockaddr_in6 *)&srtentry->srt_edest)->sin6_addr);

  srt6_addr_ntoh(&sentry6->srt6_ssrc_hbo, &ip6ssrc);
  srt6_addr_ntoh(&sentry6->srt6_ssrc_hbo, &ip6esrc);
  
  lib_strlcpy(sentry6->srt6_ifname, srtentry->srt_ifname, IF_NAMESIZE);
  
  sentry6->srt6_flags = srtentry->srt_flags;
  sentry6->srt6_mode  = srtentry->srt_mode;
  sentry6->srt6_prio  = srtentry->srt_prio;
}

#endif /* LWIP_IPV6 */
#endif /* LW_CFG_NET_ROUTER && LW_CFG_NET_BALANCING */
/*
 * end
 */
