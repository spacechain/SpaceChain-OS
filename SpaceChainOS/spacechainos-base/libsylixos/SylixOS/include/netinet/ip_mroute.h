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
** 文   件   名: ip_mroute.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2017 年 12 月 29 日
**
** 描        述: include/netinet/ip_mroute.
*********************************************************************************************************/

#ifndef __NETINET_IP_MTOUTE_H
#define __NETINET_IP_MTOUTE_H

#include "lwip/opt.h"
#include "lwip/def.h"

#if LWIP_IPV4 && IP_FORWARD && LWIP_IGMP

#include "lwip/inet.h"
#include "net/if.h"

/*********************************************************************************************************
  IP_IPPROTO_IP level multicast socket options.
*********************************************************************************************************/

#define MRT_BASE            200
#define MRT_INIT            (MRT_BASE)                          /* initialize mrouted                   */
#define MRT_DONE            (MRT_BASE + 1)                      /* shut down mrouted                    */
#define MRT_ADD_VIF         (MRT_BASE + 2)                      /* create virtual interface             */
#define MRT_DEL_VIF         (MRT_BASE + 3)                      /* delete virtual interface             */
#define MRT_ADD_MFC         (MRT_BASE + 4)                      /* insert forwarding cache entry        */
#define MRT_DEL_MFC         (MRT_BASE + 5)                      /* delete forwarding cache entry        */
#define MRT_VERSION         (MRT_BASE + 6)                      /* get kernel version number            */
#define MRT_ASSERT          (MRT_BASE + 7)                      /* enable assert processing             */
#define MRT_PIM             (MRT_ASSERT)                        /* enable PIM processing                */
#define MRT_API_SUPPORT     (MRT_BASE + 9)                      /* supported MRT API                    */
#define MRT_API_CONFIG      (MRT_BASE + 10)                     /* config MRT API                       */
#define MRT_MAX             (MRT_API_CONFIG)

/*********************************************************************************************************
  Max number of virtual interface.
*********************************************************************************************************/

#define MAXVIFS     32
typedef u_short     vifi_t;
typedef u_long      vifbitmap_t;
#define ALL_VIFS    ((vifi_t)(-1))

#define VIFM_SET(n,m)           ((m) |= (1 << (n)))
#define VIFM_CLR(n,m)           ((m) &= ~(1 << (n)))
#define VIFM_ISSET(n,m)         ((m) & (1 << (n)))
#define VIFM_CLRALL(m)          ((m) = 0)
#define VIFM_COPY(mfrom, mto)   ((mto) = (mfrom))
#define VIFM_SAME(m1, m2)       ((m1) == (m2))

/*********************************************************************************************************
  Structure used for IPv4 multicast routing 
  Argument structure for MRT_ADD_VIF, MRT_DEL_VIF takes a single vifi_t argument. 
*********************************************************************************************************/

struct vifctl {
    vifi_t              vifc_vifi;                              /* the index of the vif to be added     */
    u_char              vifc_flags;                             /* VIFF_ flags defined below            */
    u_char              vifc_threshold;                         /* min ttl required to forward on vif   */
    u_int               vifc_rate_limit;                        /* max rate (NI)                        */
    union {
        struct in_addr  vifc_local_addr;                        /* Local interface address              */
        int             vifc_local_ifindex;                     /* Local interface index                */
    } local;
    struct in_addr      vifc_rmt_addr;                          /* remote address (tunnels only)        */
};

#define vifc_lcl_addr       local.vifc_local_addr
#define vifc_lcl_ifindex    local.vifc_local_ifindex

#define VIFF_TUNNEL         0x1                                 /* vif represents a tunnel end-point    */
#define VIFF_SRCRT          0x2                                 /* tunnel uses IP source routing        */
#define VIFF_REGISTER       0x4                                 /* register vif                         */
#define VIFF_USE_IFINDEX    0x8                                 /* use vifc_lcl_ifindex instead of      */
                                                                /* vifc_lcl_addr to find an interface   */
/*********************************************************************************************************
  Argument structure for MRT_ADD_MFC and MRT_DEL_MFC 
*********************************************************************************************************/

struct mfcctl {
    struct in_addr  mfcc_origin;                                /* ip origin of mcasts                  */
    struct in_addr  mfcc_mcastgrp;                              /* multicast group associated           */
    vifi_t          mfcc_parent;                                /* incoming vif                         */
    u_char          mfcc_ttls[MAXVIFS];                         /* forwarding ttls on vifs              */
};

/*********************************************************************************************************
  The new argument structure for MRT_ADD_MFC and MRT_DEL_MFC overlays
  and extends the old struct mfcctl.
*********************************************************************************************************/

struct mfcctl2 {                                                /* the mfcctl fields                    */
    struct in_addr  mfcc_origin;                                /* ip origin of mcasts                  */
    struct in_addr  mfcc_mcastgrp;                              /* multicast group associated           */
    vifi_t          mfcc_parent;                                /* incoming vif                         */
    u_char          mfcc_ttls[MAXVIFS];                         /* forwarding ttls on vifs              */
                                                                /* extension fields                     */
    u_char          mfcc_flags[MAXVIFS];                        /* the MRT_MFC_FLAGS_* flags            */
    struct in_addr  mfcc_rp;                                    /* the RP address                       */
};

/*********************************************************************************************************
  The advanced-API flags.
  The MRT_MFC_FLAGS_XXX API flags are also used as flags
  for the mfcc_flags field.
*********************************************************************************************************/

#define MRT_MFC_FLAGS_DISABLE_WRONGVIF  (1 << 0)                /* disable WRONGVIF signals             */
#define MRT_MFC_FLAGS_BORDER_VIF        (1 << 1)                /* border vif                           */
#define MRT_MFC_RP                      (1 << 8)                /* enable RP address                    */
#define MRT_MFC_BW_UPCALL               (1 << 9)                /* enable bw upcalls                    */
#define MRT_MFC_FLAGS_ALL               (MRT_MFC_FLAGS_DISABLE_WRONGVIF | \
                                         MRT_MFC_FLAGS_BORDER_VIF)
#define MRT_API_FLAGS_ALL               (MRT_MFC_FLAGS_ALL | \
                                         MRT_MFC_RP | \
                                         MRT_MFC_BW_UPCALL)

/*********************************************************************************************************
  The kernel's multicast routing statistics. 
*********************************************************************************************************/

struct mrtstat {
    u_long      mrts_mfc_lookups;                               /* # forw. cache hash table hits        */
    u_long      mrts_mfc_misses;                                /* # forw. cache hash table misses      */
    u_long      mrts_upcalls;                                   /* # calls to mrouted                   */
    u_long      mrts_no_route;                                  /* no route for packet's origin         */
    u_long      mrts_bad_tunnel;                                /* malformed tunnel options             */
    u_long      mrts_cant_tunnel;                               /* no room for tunnel options           */
    u_long      mrts_wrong_if;                                  /* arrived on wrong interface           */
    u_long      mrts_upq_ovflw;                                 /* upcall Q overflow                    */
    u_long      mrts_cache_cleanups;                            /* # entries with no upcalls            */
    u_long      mrts_drop_sel;                                  /* pkts dropped selectively             */
    u_long      mrts_q_overflow;                                /* pkts dropped - Q overflow            */
    u_long      mrts_pkt2large;                                 /* pkts dropped - size > BKT SIZE       */
    u_long      mrts_upq_sockfull;                              /* upcalls dropped - socket full        */
};

/*********************************************************************************************************
  Argument structure used by mrouted to get vif pkt counts.
*********************************************************************************************************/

struct sioc_vif_req {
    vifi_t      vifi;                                           /* vif number                           */
    u_long      icount;                                         /* Input packet count on vif            */
    u_long      ocount;                                         /* Output packet count on vif           */
    u_long      ibytes;                                         /* Input byte count on vif              */
    u_long      obytes;                                         /* Output byte count on vif             */
};

#define SIOCGETVIFCNT   (SIOCPROTOPRIVATE)

/*********************************************************************************************************
  Argument structure used by mrouted to get src-grp pkt counts.
*********************************************************************************************************/

struct sioc_sg_req {
    struct in_addr  src;
    struct in_addr  grp;
    u_long          pktcnt;
    u_long          bytecnt;
    u_long          wrong_if;
};

#define SIOCGETSGCNT    (SIOCPROTOPRIVATE + 1)

/*********************************************************************************************************
  Struct used to communicate from kernel to multicast router
  note the convenient similarity to an IP packet
*********************************************************************************************************/

struct igmpmsg {
    uint32_t       unused1;
    uint32_t       unused2;
    u_char         im_msgtype;                                  /* what type of message                 */
#define IGMPMSG_NOCACHE     1                                   /* no MFC in the kernel                 */
#define IGMPMSG_WRONGVIF    2                                   /* packet came from wrong interface     */
#define IGMPMSG_WHOLEPKT    3                                   /* PIM pkt for user level encap.        */
    u_char         im_mbz;                                      /* must be zero                         */
    u_char         im_vif;                                      /* vif rec'd on                         */
    u_char         unused3;
    struct in_addr im_src, im_dst;
};

#endif                                                          /* LWIP_IPV4 && IP_FORWARD && LWIP_IGMP */
#endif                                                          /*  __NETINET_IP_MTOUTE_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
