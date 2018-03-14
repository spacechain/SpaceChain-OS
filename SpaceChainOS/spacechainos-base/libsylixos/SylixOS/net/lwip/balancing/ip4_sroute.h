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

#ifndef __IP4_SROUTE_H
#define __IP4_SROUTE_H

/* tcpip safety call */
#ifndef SRT_LOCK
#define SRT_LOCK() \
        if (lock_tcpip_core) { \
          LOCK_TCPIP_CORE(); \
        }
#define SRT_UNLOCK() \
        if (lock_tcpip_core) { \
          UNLOCK_TCPIP_CORE(); \
        }
#endif

/* source route entry */
struct srt_entry {
  LW_LIST_LINE      srt_list;   /* hash list */
  
  struct ip4_addr   srt_ssrc_hbo;   /* source address start (host byte order) */
  struct ip4_addr   srt_esrc_hbo;   /* source address end (host byte order) */
  struct ip4_addr   srt_sdest_hbo;  /* destination address start (host byte order) */
  struct ip4_addr   srt_edest_hbo;  /* destination address end (host byte order) */
  
  u_long            srt_flags;  /* flags */
  u_short           srt_mode;   /* mode SRT_MODE_EXCLUDE / SRT_MODE_INCLUDE */
  u_short           srt_prio;   /* priority */
  struct netif     *srt_netif;  /* netif */
  char              srt_ifname[IF_NAMESIZE]; /* ifname */
};

/* source route internal functions */
void srt_traversal_entry(VOIDFUNCPTR func, void *arg0, void *arg1, 
                         void *arg2, void *arg3, void *arg4, void *arg5);
struct srt_entry *srt_search_entry(const ip4_addr_t *ipsrc, const ip4_addr_t *ipdest);
struct srt_entry *srt_find_entry(const ip4_addr_t *ipssrc, const ip4_addr_t *ipesrc,
                                 const ip4_addr_t *ipsdest, const ip4_addr_t *ipedest);
int  srt_add_entry(struct srt_entry *sentry);
void srt_delete_entry(struct srt_entry *sentry);
void srt_total_entry(unsigned int *cnt);

/* xchg funcs */
void srt_sentry_to_srtentry(struct srtentry *srtentry, const struct srt_entry *sentry);
void srt_srtentry_to_sentry(struct srt_entry *sentry, const struct srtentry *srtentry);

/* tcpip hooks */
void srt_netif_add_hook(struct netif *netif);
void srt_netif_remove_hook(struct netif *netif);
struct netif *srt_route_search_hook(const ip4_addr_t *ipsrc, const ip4_addr_t *ipdest);
struct netif *srt_route_default_hook(const ip4_addr_t *ipsrc, const ip4_addr_t *ipdest);

#endif /* __IP4_SROUTE_H */
/*
 * end
 */
