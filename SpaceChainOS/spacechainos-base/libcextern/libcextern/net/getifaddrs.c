/*	getifaddrs.c for sylixos */

/*
 * Copyright (c) 1995, 1999
 *	Berkeley Software Design, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * THIS SOFTWARE IS PROVIDED BY Berkeley Software Design, Inc. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Berkeley Software Design, Inc. BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	BSDI getifaddrs.c,v 2.12 2013/09/26 15:01:30 dab Exp
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: getifaddrs.c,v 1.11.12.1 2009/05/03 13:17:52 bouyer Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <net/if_dl.h>
#include <net/if_type.h>
#include <netinet6/in6.h>
#include <netpacket/packet.h>

#include <assert.h>
#include <errno.h>
#include <ifaddrs.h>
#include <stdlib.h>
#include <string.h>

#define MAX_IF          LW_CFG_NET_DEV_MAX
#define MAX_IPV6        10

#define ADDR_ROUND_UP(len)  ROUND_UP((len), sizeof(size_t))
#define SIZE_SOADDR         (max(sizeof(struct sockaddr_in6), max(sizeof(struct sockaddr_ll), sizeof(struct sockaddr_dl))))
#define SIZE_ADDR           (ADDR_ROUND_UP(SIZE_SOADDR))
#define ADD_SIZE_ADDR       (SIZE_ADDR * 3)
#define ADD_SIZE            (ADD_SIZE_ADDR + IFNAMSIZ)
#define IFADDR_SIZE         (sizeof(struct ifaddrs) + ADD_SIZE)

static void
__init_ifaddrs (struct ifaddrs  *ifaddr)
{
    ifaddr->ifa_addr    = (struct sockaddr *)((ifaddr) + 1);
    ifaddr->ifa_netmask = (struct sockaddr *)((char *)(ifaddr->ifa_addr)    + SIZE_ADDR);
    ifaddr->ifa_dstaddr = (struct sockaddr *)((char *)(ifaddr->ifa_netmask) + SIZE_ADDR);
    ifaddr->ifa_name    = (char *)           ((char *)(ifaddr->ifa_dstaddr) + SIZE_ADDR);
    ifaddr->ifa_data    = NULL;
}

static void
__init_ifaddrs_l (struct ifaddrs  *ifaddr)
{
    ifaddr->ifa_addr    = (struct sockaddr *)((ifaddr) + 1);
    ifaddr->ifa_netmask = NULL;
    ifaddr->ifa_dstaddr = NULL;
    ifaddr->ifa_name    = (char *)((char *)(ifaddr->ifa_addr) + SIZE_ADDR);
    ifaddr->ifa_data    = NULL;
}

static void
__init_in6 (struct sockaddr_in6 *in6)
{
    in6->sin6_len      = sizeof(struct sockaddr_in6);
    in6->sin6_family   = AF_INET6;
    in6->sin6_port     = 0;
    in6->sin6_flowinfo = 0;
    in6->sin6_scope_id = 0;
}

static void
__init_ll (struct sockaddr_ll *ll)
{
    bzero(ll, sizeof(struct sockaddr_ll));
    ll->sll_len      = sizeof(struct sockaddr_ll);
    ll->sll_family   = AF_PACKET;
    ll->sll_hatype   = ARPHRD_ETHER;
    ll->sll_halen    = IFHWADDRLEN;
}

static void
__init_dl (struct sockaddr_dl *dl)
{
    bzero(dl, sizeof(struct sockaddr_dl));
    dl->sdl_len      = sizeof(struct sockaddr_dl);
    dl->sdl_family   = AF_LINK;
    dl->sdl_nlen     = 4;
    dl->sdl_alen     = IFHWADDRLEN;
}

static void
__calc_mask (struct in6_addr *in6addr, uint32_t prefix)
{
    int i, j;

    memcpy(in6addr, &in6addr_any, sizeof(struct in6_addr));

    for (i = 0; i < 4; i++) {
        if (prefix > 32) {
            prefix -= 32;
            in6addr->un.u32_addr[i] = 0xffffffff;

        } else {
            for (j = 0; j < prefix; j++) {
                in6addr->un.u32_addr[i] >>= 1;
                in6addr->un.u32_addr[i]  |= 0x80000000;
            }
            break;
        }
    }
}

int
getifaddrs(struct ifaddrs **pif)
{
    struct ifaddrs  *ifaddr;
    struct ifconf   conf;

    struct ifreq        req[MAX_IF];
    struct in6_ifreq    req6;
    struct in6_ifr_addr in6ifraddr[MAX_IPV6];

    int err;
    int s;
    int if_num;
    int ipv6_num;
    int i, j;

    _DIAGASSERT(pif != NULL);

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
        return  (-1);
    }

    conf.ifc_len = sizeof(req); /* sizeof bytes */
    conf.ifc_req = req;

    err = ioctl(s, SIOCGIFCONF, &conf);
    if (err < 0) {
        goto    __bad_return;
    }

    if_num = conf.ifc_len / sizeof(struct ifreq);
    if (if_num <= 0) {
        *pif = NULL;
        goto    __suc_return;
    }

    *pif   = NULL;
    ifaddr = NULL;

    for (i = 0; i < if_num; i++) {
        int              netif_index;
        struct ifaddrs  *ifaddr_alloc;

        ifaddr_alloc = (struct ifaddrs *)malloc(IFADDR_SIZE);
        if (ifaddr_alloc == NULL) {
            errno = ENOMEM;
            goto    __bad_return;
        }

        if (ifaddr) {
            ifaddr->ifa_next = ifaddr_alloc;
            ifaddr = ifaddr_alloc;

        } else {
            ifaddr = ifaddr_alloc;
            *pif   = ifaddr_alloc;
        }

        ifaddr->ifa_next = NULL;

        /* get ipv4 address */
        __init_ifaddrs(ifaddr);
        strlcpy(ifaddr->ifa_name, req[i].ifr_name, IFNAMSIZ);
        *(ifaddr->ifa_addr) = req[i].ifr_addr;

        ioctl(s, SIOCGIFFLAGS, &req[i]);
        ifaddr->ifa_flags = req[i].ifr_flags;

        ioctl(s, SIOCGIFNETMASK, &req[i]);
        *(ifaddr->ifa_netmask) = req[i].ifr_netmask;

        if (ifaddr->ifa_flags & IFF_POINTOPOINT) {
            ioctl(s, SIOCGIFDSTADDR, &req[i]);
            *(ifaddr->ifa_dstaddr) = req[i].ifr_dstaddr;

        } else if (ifaddr->ifa_flags & IFF_BROADCAST) {
            ioctl(s, SIOCGIFBRDADDR, &req[i]);
            *(ifaddr->ifa_broadaddr) = req[i].ifr_broadaddr;

        } else {
            bzero(ifaddr->ifa_dstaddr, sizeof(struct sockaddr));
        }

        /* get ipv6 address */
        ioctl(s, SIOCGIFINDEX, &req[i]);
        netif_index          = req[i].ifr_ifindex;
        req6.ifr6_ifindex    = req[i].ifr_ifindex;
        req6.ifr6_len        = sizeof(in6ifraddr);
        req6.ifr6_addr_array = in6ifraddr;

        ioctl(s, SIOCGIFADDR6, &req6);

        ipv6_num = req6.ifr6_len / sizeof(struct in6_ifr_addr);

        for (j = 0; j < ipv6_num; j++) {    /* get ipv6 address */
            struct ifaddrs      *ifaddr6;
            struct sockaddr_in6 *in6;

            ifaddr6 = (struct ifaddrs *)malloc(IFADDR_SIZE);
            if (ifaddr6 == NULL) {
                errno = ENOMEM;
                goto    __bad_return;
            }

            __init_ifaddrs(ifaddr6);
            strcpy(ifaddr6->ifa_name, ifaddr->ifa_name);
            ifaddr6->ifa_flags = ifaddr->ifa_flags;

            in6 = (struct sockaddr_in6 *)ifaddr6->ifa_addr;
            __init_in6(in6);
            in6->sin6_addr = in6ifraddr[j].ifr6a_addr;

            in6 = (struct sockaddr_in6 *)ifaddr6->ifa_netmask;
            __init_in6(in6);
            __calc_mask(&in6->sin6_addr, in6ifraddr[j].ifr6a_prefixlen);

            in6 = (struct sockaddr_in6 *)ifaddr6->ifa_dstaddr;
            __init_in6(in6);
            memcpy(&in6->sin6_addr, &in6addr_any, sizeof(struct in6_addr));

            ifaddr->ifa_next = ifaddr6;
            ifaddr6->ifa_next = NULL;
            ifaddr = ifaddr6;
        }

        if (ioctl(s, SIOCGIFHWADDR, &req[i]) == 0) {    /* get hwaddr */
            struct ifaddrs      *ifaddr_ll;
            struct ifaddrs      *ifaddr_dl;
            struct sockaddr_ll  *ll;
            struct sockaddr_dl  *dl;

            /* setup AF_PACKET */
            ifaddr_ll = (struct ifaddrs *)malloc(IFADDR_SIZE);
            if (ifaddr_ll == NULL) {
                errno = ENOMEM;
                goto    __bad_return;
            }

            __init_ifaddrs_l(ifaddr_ll);
            strcpy(ifaddr_ll->ifa_name, ifaddr->ifa_name);
            ifaddr_ll->ifa_flags = ifaddr->ifa_flags;

            ll = (struct sockaddr_ll *)ifaddr_ll->ifa_addr;
            __init_ll(ll);

            ll->sll_ifindex = netif_index;
            memcpy(ll->sll_addr, req[i].ifr_hwaddr.sa_data, IFHWADDRLEN);

            ifaddr->ifa_next = ifaddr_ll;
            ifaddr_ll->ifa_next = NULL;
            ifaddr = ifaddr_ll;

            /* setup AF_LINK */
            ifaddr_dl = (struct ifaddrs *)malloc(IFADDR_SIZE);
            if (ifaddr_dl == NULL) {
                errno = ENOMEM;
                goto    __bad_return;
            }

            __init_ifaddrs_l(ifaddr_dl);
            strcpy(ifaddr_dl->ifa_name, ifaddr->ifa_name);
            ifaddr_dl->ifa_flags = ifaddr->ifa_flags;

            dl = (struct sockaddr_dl *)ifaddr_dl->ifa_addr;
            __init_dl(dl);

            dl->sdl_index = netif_index;
            memcpy(LLADDR(dl), req[i].ifr_hwaddr.sa_data, IFHWADDRLEN);

            ioctl(s, SIOCGIFTYPE, &req[i]);
            dl->sdl_type = (u_char)req[i].ifr_type;

            ifaddr->ifa_next = ifaddr_dl;
            ifaddr_dl->ifa_next = NULL;
            ifaddr = ifaddr_dl;
        }
    }

__suc_return:
    close(s);
    return  (0);

__bad_return:
    close(s);
    if (*pif) {
        freeifaddrs(*pif);
    }
    return  (-1);
}

void
freeifaddrs(struct ifaddrs *ifp)
{
    struct ifaddrs  *ifaddr;

    while (ifp) {
        ifaddr = ifp;
        ifp = ifaddr->ifa_next;
        free(ifaddr);
    }
}
