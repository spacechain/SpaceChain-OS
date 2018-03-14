/**
 * @file
 * Lwip platform independent net ipv4 source route.
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
#include "ip4_sroute.h"

/* srt_entry to struct srtentry */
void srt_sentry_to_srtentry (struct srtentry *srtentry, const struct srt_entry *sentry)
{
  ip4_addr_t ipssrc, ipesrc;
  ip4_addr_t ipsdest, ipedest;

  srtentry->srt_ssrc.sa_len = sizeof(struct sockaddr_in);
  srtentry->srt_ssrc.sa_family = AF_INET;
  srtentry->srt_esrc.sa_len = sizeof(struct sockaddr_in);
  srtentry->srt_esrc.sa_family = AF_INET;
  srtentry->srt_sdest.sa_len = sizeof(struct sockaddr_in);
  srtentry->srt_sdest.sa_family = AF_INET;
  srtentry->srt_edest.sa_len = sizeof(struct sockaddr_in);
  srtentry->srt_edest.sa_family = AF_INET;
  
  ipssrc.addr  = PP_HTONL(sentry->srt_ssrc_hbo.addr);
  ipesrc.addr  = PP_HTONL(sentry->srt_esrc_hbo.addr);
  ipsdest.addr = PP_HTONL(sentry->srt_sdest_hbo.addr);
  ipedest.addr = PP_HTONL(sentry->srt_edest_hbo.addr);

  inet_addr_from_ip4addr(&((struct sockaddr_in *)&srtentry->srt_ssrc)->sin_addr, &ipssrc);
  inet_addr_from_ip4addr(&((struct sockaddr_in *)&srtentry->srt_esrc)->sin_addr, &ipesrc);
  inet_addr_from_ip4addr(&((struct sockaddr_in *)&srtentry->srt_sdest)->sin_addr, &ipsdest);
  inet_addr_from_ip4addr(&((struct sockaddr_in *)&srtentry->srt_edest)->sin_addr, &ipedest);
  
  lib_strlcpy(srtentry->srt_ifname, sentry->srt_ifname, IF_NAMESIZE);
  
  srtentry->srt_flags = (u_short)sentry->srt_flags;
  srtentry->srt_mode  = sentry->srt_mode;
  srtentry->srt_prio  = sentry->srt_prio;
}

/* struct srtentry to srt_entry */
void srt_srtentry_to_sentry (struct srt_entry *sentry, const struct srtentry *srtentry)
{
  ip4_addr_t ipssrc, ipesrc;
  ip4_addr_t ipsdest, ipedest;

  inet_addr_to_ip4addr(&ipssrc, &((struct sockaddr_in *)&srtentry->srt_ssrc)->sin_addr);
  inet_addr_to_ip4addr(&ipesrc, &((struct sockaddr_in *)&srtentry->srt_esrc)->sin_addr);
  inet_addr_to_ip4addr(&ipsdest, &((struct sockaddr_in *)&srtentry->srt_sdest)->sin_addr);
  inet_addr_to_ip4addr(&ipedest, &((struct sockaddr_in *)&srtentry->srt_edest)->sin_addr);
  
  sentry->srt_ssrc_hbo.addr  = PP_NTOHL(ipssrc.addr);
  sentry->srt_esrc_hbo.addr  = PP_NTOHL(ipesrc.addr);
  sentry->srt_sdest_hbo.addr = PP_NTOHL(ipsdest.addr);
  sentry->srt_edest_hbo.addr = PP_NTOHL(ipedest.addr);
  
  lib_strlcpy(sentry->srt_ifname, srtentry->srt_ifname, IF_NAMESIZE);
  
  sentry->srt_flags = srtentry->srt_flags;
  sentry->srt_mode  = srtentry->srt_mode;
  sentry->srt_prio  = srtentry->srt_prio;
}

#endif /* LW_CFG_NET_ROUTER && LW_CFG_NET_BALANCING */
/*
 * end
 */
