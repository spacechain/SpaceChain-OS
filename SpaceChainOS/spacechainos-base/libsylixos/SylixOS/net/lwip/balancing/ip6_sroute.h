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

#ifndef __IP6_SROUTE_H
#define __IP6_SROUTE_H

#if LWIP_IPV6

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

/* ipv6 address host net byte order convert */
#define srt6_addr_hton(ipaddr1, ipaddr2) \
        do { \
          (ipaddr1)->addr[0] = PP_HTONL((ipaddr2)->addr[0]); \
          (ipaddr1)->addr[1] = PP_HTONL((ipaddr2)->addr[1]); \
          (ipaddr1)->addr[2] = PP_HTONL((ipaddr2)->addr[2]); \
          (ipaddr1)->addr[3] = PP_HTONL((ipaddr2)->addr[3]); \
        } while (0)
#define srt6_addr_ntoh(ipaddr1, ipaddr2)    srt6_addr_hton(ipaddr1, ipaddr2)

/* source route entry */
struct srt6_entry {
  LW_LIST_LINE      srt6_list;   /* hash list */
  
  struct ip6_addr   srt6_ssrc_hbo;   /* source address start */
  struct ip6_addr   srt6_esrc_hbo;   /* source address end */
  struct ip6_addr   srt6_sdest_hbo;  /* destination address start (host byte order) */
  struct ip6_addr   srt6_edest_hbo;  /* destination address end (host byte order) */
  
  u_long            srt6_flags;  /* flags */
  u_short           srt6_mode;   /* mode SRT_MODE_EXCLUDE / SRT_MODE_INCLUDE */
  u_short           srt6_prio;   /* priority */
  struct netif     *srt6_netif;  /* netif */
  char              srt6_ifname[IF_NAMESIZE]; /* ifname */
};

/* source route internal functions */
void srt6_traversal_entry(VOIDFUNCPTR func, void *arg0, void *arg1, 
                          void *arg2, void *arg3, void *arg4, void *arg5);
struct srt6_entry *srt6_search_entry(const ip6_addr_t *ip6src, const ip6_addr_t *ip6dest);
struct srt6_entry *srt6_find_entry(const ip6_addr_t *ip6ssrc, const ip6_addr_t *ip6esrc,
                                   const ip6_addr_t *ip6sdest, const ip6_addr_t *ip6edest);
int  srt6_add_entry(struct srt6_entry *sentry6);
void srt6_delete_entry(struct srt6_entry *sentry6);
void srt6_total_entry(unsigned int *cnt);

/* xchg funcs */
void srt6_sentry_to_srtentry(struct srtentry *srtentry, const struct srt6_entry *sentry6);
void srt6_srtentry_to_sentry(struct srt6_entry *sentry6, const struct srtentry *srtentry);

/* tcpip hooks */
void srt6_netif_add_hook(struct netif *netif);
void srt6_netif_remove_hook(struct netif *netif);
struct netif *srt6_route_search_hook(const ip6_addr_t *ip6src, const ip6_addr_t *ip6dest);
struct netif *srt6_route_default_hook(const ip6_addr_t *ip6src, const ip6_addr_t *ip6dest);

#endif /* LWIP_IPV6 */
#endif /* __IP6_SROUTE_H */
/*
 * end
 */
