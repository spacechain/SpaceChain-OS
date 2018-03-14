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
** 文   件   名: route.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2017 年 12 月 07 日
**
** 描        述: bsd route.h
*********************************************************************************************************/

#ifndef __NET_ROUTE_H
#define __NET_ROUTE_H

#include "sys/types.h"
#include "sys/socket.h"

/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_NET_ROUTER > 0)

#include "if.h"

#ifdef __cplusplus
extern "C" {
#endif                                          /*  __cplusplus                                         */

/*********************************************************************************************************
  These numbers are used by reliable protocols for determining
  retransmission behavior and are included in the routing structure.
*********************************************************************************************************/

struct rt_metrics {
    u_long  rmx_locks;                          /* Kernel must leave these values alone                 */
    u_long  rmx_mtu;                            /* MTU for this path                                    */
    u_long  rmx_hopcount;                       /* max hops expected                                    */
    u_long  rmx_expire;                         /* lifetime for route, e.g. redirect                    */
    u_long  rmx_recvpipe;                       /* inbound delay-bandwidth product                      */
    u_long  rmx_sendpipe;                       /* outbound delay-bandwidth product                     */
    u_long  rmx_ssthresh;                       /* outbound gateway buffer limit                        */
    u_long  rmx_rtt;                            /* estimated round trip time                            */
    u_long  rmx_rttvar;                         /* estimated rtt variance                               */
    u_long  rmx_pksent;                         /* packets sent using this route                        */
    u_long  rmx_filler[4];                      /* will be used for T/TCP later                         */
};

/*********************************************************************************************************
  rmx_rtt and rmx_rttvar are stored as microseconds;
  RTTTOPRHZ(rtt) converts to a value suitable for use
  by a protocol slowtimo counter.
*********************************************************************************************************/

#ifndef PR_SLOWHZ                               /* TCP_FAST_INTERVAL & TCP_SLOW_INTERVAL                */
#define PR_SLOWHZ       2L                      /* 2 slow timeouts per second                           */
#define PR_FASTHZ       5L                      /* 5 fast timeouts per second                           */
#endif

/*********************************************************************************************************
  Bitmask values for rtm_inits and rmx_locks.
*********************************************************************************************************/

#define RTV_MTU         0x1                     /* init or lock _mtu                                    */
#define RTV_HOPCOUNT    0x2                     /* init or lock _hopcount                               */
#define RTV_EXPIRE      0x4                     /* init or lock _expire                                 */
#define RTV_RPIPE       0x8                     /* init or lock _recvpipe                               */
#define RTV_SPIPE       0x10                    /* init or lock _sendpipe                               */
#define RTV_SSTHRESH    0x20                    /* init or lock _ssthresh                               */
#define RTV_RTT         0x40                    /* init or lock _rtt                                    */
#define RTV_RTTVAR      0x80                    /* init or lock _rttvar                                 */

/*********************************************************************************************************
  Structures for routing messages.
*********************************************************************************************************/

struct rt_msghdr {
    u_short rtm_msglen;                         /* to skip over non-understood messages                 */
    u_char  rtm_version;                        /* future binary compatibility                          */
    u_char  rtm_type;                           /* message type                                         */
    
    u_short rtm_index;                          /* index for associated ifp                             */
    int     rtm_flags;                          /* flags, incl. kern & message, e.g. DONE               */
    int     rtm_addrs;                          /* bitmask identifying sockaddrs in msg                 */
    pid_t   rtm_pid;                            /* identify sender                                      */
    int     rtm_seq;                            /* for sender to identify action                        */
    int     rtm_errno;                          /* why failed                                           */
    int     rtm_use;                            /* from rtentry                                         */
    u_long  rtm_inits;                          /* which metrics we are initializing                    */
    struct  rt_metrics rtm_rmx;                 /* metrics themselves                                   */
};

#define RTM_VERSION     5                       /* Up the ante and ignore older versions                */

/*********************************************************************************************************
  Bitmask values for rtm_addrs.
*********************************************************************************************************/

#define RTA_DST         0x1                     /* destination sockaddr present                         */
#define RTA_GATEWAY     0x2                     /* gateway sockaddr present                             */
#define RTA_NETMASK     0x4                     /* netmask sockaddr present                             */
#define RTA_GENMASK     0x8                     /* cloning mask sockaddr present                        */
#define RTA_IFP         0x10                    /* interface name sockaddr present                      */
#define RTA_IFA         0x20                    /* interface addr sockaddr present                      */
#define RTA_AUTHOR      0x40                    /* sockaddr for author of redirect                      */
#define RTA_BRD         0x80                    /* for NEWADDR, broadcast or p-p dest addr              */

/*********************************************************************************************************
  Message types.
*********************************************************************************************************/

#define RTM_ADD         0x1                     /* Add Route                                            */
#define RTM_DELETE      0x2                     /* Delete Route                                         */
#define RTM_CHANGE      0x3                     /* Change Metrics or flags                              */
#define RTM_GET         0x4                     /* Report Metrics                                       */
#define RTM_LOSING      0x5                     /* Kernel Suspects Partitioning                         */
#define RTM_REDIRECT    0x6                     /* Told to use different route                          */
#define RTM_MISS        0x7                     /* Lookup failed on this address                        */
#define RTM_LOCK        0x8                     /* fix specified metrics                                */
#define RTM_OLDADD      0x9                     /* caused by SIOCADDRT                                  */
#define RTM_OLDDEL      0xa                     /* caused by SIOCDELRT                                  */
#define RTM_RESOLVE     0xb                     /* req to resolve dst to LL addr                        */
#define RTM_NEWADDR     0xc                     /* address being added to iface                         */
#define RTM_DELADDR     0xd                     /* address being removed from iface                     */
#define RTM_IFINFO      0xe                     /* iface going up/down etc.                             */
#define RTM_NEWMADDR    0xf                     /* mcast group membership being added to if             */
#define RTM_DELMADDR    0x10                    /* mcast group membership being deleted                 */
#define RTM_IFANNOUNCE  0x11                    /* iface arrival/departure                              */
#define RTM_IEEE80211   0x12                    /* IEEE80211 wireless event                             */

/*********************************************************************************************************
  Index offsets for sockaddr array for alternate internal encoding.
*********************************************************************************************************/

#define RTAX_DST        0                       /* destination sockaddr present                         */
#define RTAX_GATEWAY    1                       /* gateway sockaddr present                             */
#define RTAX_NETMASK    2                       /* netmask sockaddr present                             */
#define RTAX_GENMASK    3                       /* cloning mask sockaddr present                        */
#define RTAX_IFP        4                       /* interface name sockaddr present                      */
#define RTAX_IFA        5                       /* interface addr sockaddr present                      */
#define RTAX_AUTHOR     6                       /* sockaddr for author of redirect                      */
#define RTAX_BRD        7                       /* for NEWADDR, broadcast or p-p dest addr              */
#define RTAX_MAX        8                       /* size of array to allocate                            */

struct rt_addrinfo {
    int                 rti_addrs;
    struct sockaddr    *rti_info[RTAX_MAX];
};

/*********************************************************************************************************
  route entry flag
*********************************************************************************************************/

#define RTF_UP          0x1                     /* route usable                                         */
#define RTF_GATEWAY     0x2                     /* destination is a gateway                             */
#define RTF_HOST        0x4                     /* host entry (net otherwise)                           */
#define RTF_REJECT      0x8                     /* host or net unreachable                              */
#define RTF_DYNAMIC     0x10                    /* created dynamically (by redirect)                    */
#define RTF_MODIFIED    0x20                    /* modified dynamically (by redirect)                   */
#define RTF_DONE        0x40                    /* message confirmed                                    */
                                                /* 0x80 unused, was RTF_DELCLONE                        */
#define RTF_CLONING     0x100                   /* generate new routes on use                           */
#define RTF_XRESOLVE    0x200                   /* external daemon resolves name                        */
#define RTF_LLINFO      0x400                   /* generated by link layer (e.g. ARP)                   */
#define RTF_STATIC      0x800                   /* manually added                                       */
#define RTF_BLACKHOLE   0x1000                  /* just discard pkts (during updates)                   */
#define RTF_PROTO2      0x4000                  /* protocol specific routing flag                       */
#define RTF_PROTO1      0x8000                  /* protocol specific routing flag                       */

#define RTF_PRCLONING   0x10000                 /* protocol requires cloning                            */
#define RTF_WASCLONED   0x20000                 /* route generated through cloning                      */
#define RTF_PROTO3      0x40000                 /* protocol specific routing flag                       */
#define RTF_RUNNING     0x80000                 /* net interface online (sylixos)                       */

#define RTF_PINNED      0x100000                /* future use                                           */
#define RTF_LOCAL       0x200000                /* route represents a local address                     */
#define RTF_BROADCAST   0x400000                /* route represents a bcast address                     */
#define RTF_MULTICAST   0x800000                /* route represents a mcast address                     */
                                                /* 0x1000000 and up unassigned                          */

/*********************************************************************************************************
  This structure gets passed by the SIOCADDRT, SIOCDELRT, SIOCCHGRT, SIOCCHGRT calls. 
*********************************************************************************************************/

struct rtentry {
    u_long          rt_pad1;
    struct sockaddr rt_dst;                     /* target address                                       */
    struct sockaddr rt_gateway;                 /* gateway addr (RTF_GATEWAY)                           */
    struct sockaddr rt_genmask;                 /* target network mask (IP)                             */
    char            rt_ifname[IF_NAMESIZE];     /* interface name                                       */
    u_short         rt_flags;
    u_short         rt_refcnt;
    u_long          rt_pad3;
    void           *rt_pad4;
    short           rt_metric;                  /* +1 for binary compatibility!                         */
    void           *rt_dev;                     /* system not use                                       */
    u_long          rt_hopcount;
    u_long          rt_mtu;                     /* per route MTU/Window                                 */
    u_long          rt_window;                  /* Window clamping                                      */
    u_short         rt_irtt;                    /* Initial RTT                                          */
    u_long          rt_pad5[16];                /* for feature                                          */
};

#define SIOCADDRT       _IOW( 'r', 10, struct rtentry)
#define SIOCDELRT       _IOW( 'r', 11, struct rtentry)
#define SIOCCHGRT       _IOW( 'r', 12, struct rtentry)
#define SIOCGETRT       _IOWR('r', 13, struct rtentry)

/*********************************************************************************************************
  This structure gets passed by the SIOCLSTRT calls. 
*********************************************************************************************************/

struct rtentry_list {
    u_long          rtl_bcnt;                   /* struct rtentry buffer count                          */
    u_long          rtl_num;                    /* system return how many route entry in buffer         */
    u_long          rtl_total;                  /* system return total number route entry               */
    struct rtentry *rtl_buf;                    /* route list buffer                                    */
};

#define SIOCLSTRT       _IOWR('r', 14, struct rtentry_list)

/*********************************************************************************************************
  This structure gets passed by the SIOCLSTRTM calls. 
*********************************************************************************************************/

struct rt_msghdr_list {
    size_t              rtml_bsize;             /* struct rtml_buf buffer size in bytes                 */
    size_t              rtml_rsize;             /* system return how many route entry in bytes          */
    size_t              rtml_tsize;             /* system return total route entry in bytes             */
    struct rt_msghdr   *rtml_buf;               /* route rtm_msghdr buffer                              */
};

#define SIOCLSTRTM      _IOWR('r', 15, struct rt_msghdr_list)

/*********************************************************************************************************
  Set tcp mss adjust in ipv4 forward.
*********************************************************************************************************/

#define SIOCGTCPMSSADJ  _IOR('r', 20, int)      /* get ipv4 forward TCP MSS adjust status               */
#define SIOCSTCPMSSADJ  _IOW('r', 20, int)      /* set ipv4 forward TCP MSS adjust status (1: enable)   */

#ifdef __cplusplus
}
#endif                                          /*  __cplusplus                                         */

#endif                                          /*  LW_CFG_NET_EN > 0                                   */
                                                /*  LW_CFG_NET_ROUTER > 0                               */
#endif                                          /*  __NET_ROUTE_H                                       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
