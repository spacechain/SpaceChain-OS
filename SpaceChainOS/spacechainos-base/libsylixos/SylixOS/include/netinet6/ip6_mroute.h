/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: ip6_mroute.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2017 年 12 月 29 日
**
** 描        述: include/netinet6/ip6_mroute.
*********************************************************************************************************/

#ifndef __NETINET6_IP6_MTOUTE_H
#define __NETINET6_IP6_MTOUTE_H

#include "lwip/opt.h"
#include "lwip/def.h"

#if LWIP_IPV6 && LWIP_IPV6_FORWARD && LWIP_IPV6_MLD

#include "lwip/inet.h"
#include "net/if.h"
#include "string.h"

/*********************************************************************************************************
  multicast socket options.
*********************************************************************************************************/

#define MRT6_BASE           200
#define MRT6_INIT           (MRT6_BASE)                         /* initialize mrouted                   */
#define MRT6_DONE           (MRT6_BASE + 1)                     /* shut down mrouted                    */
#define MRT6_ADD_MIF        (MRT6_BASE + 2)                     /* create virtual interface             */
#define MRT6_DEL_MIF        (MRT6_BASE + 3)                     /* delete virtual interface             */
#define MRT6_ADD_MFC        (MRT6_BASE + 4)                     /* insert forwarding cache entry        */
#define MRT6_DEL_MFC        (MRT6_BASE + 5)                     /* delete forwarding cache entry        */
#define MRT6_VERSION        (MRT6_BASE + 6)                     /* get kernel version number            */
#define MRT6_ASSERT         (MRT6_BASE + 7)                     /* enable assert processing             */
#define MRT6_PIM            (MRT6_ASSERT)                       /* enable PIM processing                */
#define MRT6_MAX            (MRT6_PIM)

/*********************************************************************************************************
  Max number of virtual interface.
*********************************************************************************************************/

#define MAXMIFS     32
typedef u_short     mifi_t;
typedef u_long      mifbitmap_t;
#define ALL_MIFS    ((mifi_t)(-1))

#ifndef IF_SETSIZE
#define IF_SETSIZE  256
#endif

typedef u_int32_t   if_mask;
#define NIFBITS     (sizeof(if_mask) * 8)                       /* bits per mask                        */

typedef struct if_set {
    if_mask     ifs_bits[(((IF_SETSIZE) + ((NIFBITS) - 1)) / (NIFBITS))];
} if_set;

#define IF_SET(n, p)    ((p)->ifs_bits[(n) / NIFBITS] |= (1u << ((n) % NIFBITS)))
#define IF_CLR(n, p)    ((p)->ifs_bits[(n) / NIFBITS] &= ~(1u << ((n) % NIFBITS)))
#define IF_ISSET(n, p)  ((p)->ifs_bits[(n) / NIFBITS] & (1u << ((n) % NIFBITS)))
#define IF_COPY(f, t)   bcopy(f, t, sizeof(*(f)))
#define IF_ZERO(p)      bzero(p, sizeof(*(p)))

/*********************************************************************************************************
  Argument structure for MRT6_ADD_MIF and MRT6_DEL_MIF.
*********************************************************************************************************/

struct mif6ctl {
    mifi_t      mif6c_mifi;                                     /* Index of MIF                         */
    u_char      mif6c_flags;                                    /* MIFF_ flags                          */
    u_char      mif6c_pad1;                                     /* vifc_threshold                       */
    u_short     mif6c_pifi;                                     /* the index of the physical IF         */
    u_int       mif6c_pad2;                                     /* max rate (NI)                        */
};

#define MIFF_REGISTER   0x1                                     /* mif represents a register end-point  */

/*********************************************************************************************************
  Cache manipulation structures for mrouted and PIMd
*********************************************************************************************************/

struct mf6cctl {
    struct sockaddr_in6 mf6cc_origin;                           /* Origin of mcast                      */
    struct sockaddr_in6 mf6cc_mcastgrp;                         /* Group in question                    */
    mifi_t              mf6cc_parent;                           /* Where it arrived                     */
    struct if_set       mf6cc_ifset;                            /* Where it is going                    */
};

/*********************************************************************************************************
  The kernel's multicast routing statistics.
*********************************************************************************************************/

struct mrt6stat {
    uint64_t    mrt6s_mfc_lookups;                              /* # forw. cache hash table hits        */
    uint64_t    mrt6s_mfc_misses;                               /* # forw. cache hash table misses      */
    uint64_t    mrt6s_upcalls;                                  /* # calls to multicast routing daemon  */
    uint64_t    mrt6s_no_route;                                 /* no route for packet's origin         */
    uint64_t    mrt6s_bad_tunnel;                               /* malformed tunnel options             */
    uint64_t    mrt6s_cant_tunnel;                              /* no room for tunnel options           */
    uint64_t    mrt6s_wrong_if;                                 /* arrived on wrong interface           */
    uint64_t    mrt6s_upq_ovflw;                                /* upcall Q overflow                    */
    uint64_t    mrt6s_cache_cleanups;                           /* # entries with no upcalls            */
    uint64_t    mrt6s_drop_sel;                                 /* pkts dropped selectively             */
    uint64_t    mrt6s_q_overflow;                               /* pkts dropped - Q overflow            */
    uint64_t    mrt6s_pkt2large;                                /* pkts dropped - size > BKT SIZE       */
    uint64_t    mrt6s_upq_sockfull;                             /* upcalls dropped - socket full        */
};

/*********************************************************************************************************
  To get vif packet counts
*********************************************************************************************************/

struct sioc_mif_req6 {
    mifi_t      mifi;                                           /* Which iface                          */
    u_quad_t    icount;                                         /* In packets                           */
    u_quad_t    ocount;                                         /* Out packets                          */
    u_quad_t    ibytes;                                         /* In bytes                             */
    u_quad_t    obytes;                                         /* Out bytes                            */
};

#define SIOCGETMIFCNT_IN6   (SIOCPROTOPRIVATE)

/*********************************************************************************************************
  Group count retrieval for pim6sd
*********************************************************************************************************/

struct sioc_sg_req6 {
    struct sockaddr_in6 src;
    struct sockaddr_in6 grp;
    u_quad_t            pktcnt;
    u_quad_t            bytecnt;
    u_quad_t            wrong_if;
};

#define SIOCGETSGCNT_IN6    (SIOCPROTOPRIVATE + 1)

/*********************************************************************************************************
  Structure used to communicate from kernel to multicast router.
  We'll overlay the structure onto an MLD header (not an IPv6 heder like igmpmsg{}
  used for IPv4 implementation). This is because this structure will be passed via an
  IPv6 raw socket, on which an application will only receiver the payload i.e the data after
  the IPv6 header and all the extension headers. (See section 3 of RFC 3542)
*********************************************************************************************************/

struct mrt6msg {
#define MRT6MSG_NOCACHE     1
#define MRT6MSG_WRONGMIF    2
#define MRT6MSG_WHOLEPKT    3                                   /* used for use level encap             */
    u_char          im6_mbz;                                    /* must be zero                         */
    u_char          im6_msgtype;                                /* what type of message                 */
    u_short         im6_mif;                                    /* mif rec'd on                         */
    u_int32_t       im6_pad;                                    /* padding for 64 bit arch              */
    struct in6_addr im6_src, im6_dst;
};

#endif                                                          /* LWIP_IPV6 && LWIP_IPV6_FORWARD &&    */
                                                                /* LWIP_IPV6_MLD                        */
#endif                                                          /*  __NETINET6_IP6_MTOUTE_H             */
/*********************************************************************************************************
  END
*********************************************************************************************************/
