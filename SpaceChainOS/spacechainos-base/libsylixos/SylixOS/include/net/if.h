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
** 文   件   名: if.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2011 年 03 月 10 日
**
** 描        述: posix net/if.h
*********************************************************************************************************/

#ifndef __IF_H
#define __IF_H

#include "sys/types.h"

/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0

#include "sys/socket.h"
#include "sys/ioctl.h"

#include "netinet/in.h"
#include "netinet6/in6.h"

/*********************************************************************************************************
  if_nameindex
*********************************************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif                                                          /*  __cplusplus                         */

#define IF_NAMESIZE     16

struct if_nameindex {
    unsigned    if_index;                                       /* Numeric index of interface           */
    char       *if_name;                                        /* Null-terminated name of the          */
                                                                /* interface.                           */
    char        if_name_buf[IF_NAMESIZE];
};

/*********************************************************************************************************
  posix if api
*********************************************************************************************************/

LW_API INT                   if_down(const char *ifname);       /* net interface down                   */
LW_API INT                   if_up(const char *ifname);         /* net interface up                     */
LW_API INT                   if_isup(const char *ifname);       /* net interface is up                  */
LW_API INT                   if_islink(const char *ifname);     /* net interface is on link             */
LW_API INT                   if_get_dhcp(const char *ifname);   /* get net if dhcp flag                 */
LW_API INT                   if_set_dhcp(const char *ifname, int en);   
                                                                /* set net if dhcp flag                 */
LW_API INT                   if_set_dhcp6(const char *ifname, int en, int stateless);
LW_API INT                   if_get_dhcp6(const char *ifname);
                                                                
LW_API unsigned              if_nametoindex(const char *ifname);
LW_API char                 *if_indextoname(unsigned  ifindex, char *ifname);
LW_API struct if_nameindex  *if_nameindex(void);
LW_API void                  if_freenameindex(struct if_nameindex *ptr);

#ifdef __SYLIXOS_KERNEL                                         /* user DO NOT use these API            */
LW_API size_t                if_nameindex_bufsize(void);
LW_API struct if_nameindex  *if_nameindex_rnp(void *buffer, size_t bufsize);
#endif                                                          /* __SYLIXOS_KERNEL                     */

/*********************************************************************************************************
  posix if flag (same as lwip)
*********************************************************************************************************/

#define IFF_UP               0x0001                             /* NETIF_FLAG_UP                        */
#define IFF_BROADCAST        0x0002                             /* NETIF_FLAG_BROADCAST                 */
#define IFF_POINTOPOINT      0x0004                             /* NETIF_FLAG_POINTTOPOINT              */
#define IFF_RUNNING          0x0010                             /* NETIF_FLAG_LINK_UP                   */
#define IFF_MULTICAST        0x0080                             /* NETIF_FLAG_IGMP                      */
#define IFF_LOOPBACK         0x0100
#define IFF_NOARP            0x0200
#define IFF_PROMISC          0x0400
#define IFF_ALLMULTI         0x0800                             /* receive all multicast packets        */

/*********************************************************************************************************
  posix if structures
*********************************************************************************************************/

#ifndef IFNAMSIZ
#define IFNAMSIZ            IF_NAMESIZE
#endif

struct ifreq {
#define IFHWADDRLEN         6
    union {
        char                ifrn_name[IFNAMSIZ];                /* if name, e.g. "en1"                  */
    } ifr_ifrn;
    union {
        struct sockaddr     ifru_addr;
        struct sockaddr     ifru_dstaddr;
        struct sockaddr     ifru_broadaddr;
        struct sockaddr     ifru_netmask;
        struct sockaddr     ifru_hwaddr;
        short               ifru_flags;
        int                 ifru_ifindex;
        int                 ifru_mtu;
        int                 ifru_metric;
        int                 ifru_type;
        void               *ifru_data;
    } ifr_ifru;
};

#define ifr_name            ifr_ifrn.ifrn_name
#define ifr_addr            ifr_ifru.ifru_addr
#define ifr_dstaddr         ifr_ifru.ifru_dstaddr
#define ifr_netmask         ifr_ifru.ifru_netmask
#define ifr_broadaddr       ifr_ifru.ifru_broadaddr
#define ifr_hwaddr          ifr_ifru.ifru_hwaddr
#define ifr_flags           ifr_ifru.ifru_flags
#define ifr_ifindex         ifr_ifru.ifru_ifindex
#define ifr_mtu             ifr_ifru.ifru_mtu
#define ifr_metric          ifr_ifru.ifru_metric
#define ifr_type            ifr_ifru.ifru_type
#define ifr_data            ifr_ifru.ifru_data

struct ifconf {
    int                     ifc_len;                            /* size of buffer in bytes              */
    union {
        char               *ifcu_buf;
        struct ifreq       *ifcu_req;
    } ifc_ifcu;
};

#define ifc_buf             ifc_ifcu.ifcu_buf                   /* buffer address                       */
#define ifc_req             ifc_ifcu.ifcu_req                   /* array of structures                  */

/*********************************************************************************************************
  posix if ioctl
*********************************************************************************************************/

#define SIOCGSIZIFCONF      _IOR('i', 106, int)
#define SIOCGIFCONF         _IOWR('i', 20, struct ifconf)
#define SIOCGIFNUM          _IOR('i',  20, int)

#define SIOCSIFADDR         _IOW('i', 12, struct ifreq)
#define SIOCSIFNETMASK      _IOW('i', 22, struct ifreq)
#define SIOCSIFDSTADDR      _IOW('i', 14, struct ifreq)
#define SIOCSIFBRDADDR      _IOW('i', 19, struct ifreq)
#define SIOCSIFFLAGS        _IOW('i', 16, struct ifreq)

#define SIOCGIFADDR         _IOWR('i', 33, struct ifreq)
#define SIOCGIFNETMASK      _IOWR('i', 37, struct ifreq)
#define SIOCGIFDSTADDR      _IOWR('i', 34, struct ifreq)
#define SIOCGIFBRDADDR      _IOWR('i', 35, struct ifreq)
#define SIOCGIFFLAGS        _IOWR('i', 17, struct ifreq)

#define SIOCGIFMETRIC       _IOWR('i', 23, struct ifreq)
#define SIOCSIFMETRIC       _IOW( 'i', 24, struct ifreq)

#define SIOCGIFTYPE         _IOR('i',  49, struct ifreq)
#define SIOCGIFNAME         _IOWR('i', 50, struct ifreq)
#define SIOCGIFINDEX        _IOWR('i', 51, struct ifreq)

#define SIOCGIFMTU          _IOWR('i', 52, struct ifreq)
#define SIOCSIFMTU          _IOW('i',  53, struct ifreq)

#define SIOCGIFHWADDR       _IOWR('i', 54, struct ifreq)
#define SIOCSIFHWADDR       _IOW('i',  55, struct ifreq)

#define SIOCADDMULTI        _IOW('i', 60, struct ifreq)
#define SIOCDELMULTI        _IOW('i', 61, struct ifreq)

/*********************************************************************************************************
  sylixos if6 structures
*********************************************************************************************************/

struct in6_ifr_addr {
    struct in6_addr      ifr6a_addr;
    uint32_t             ifr6a_prefixlen;
};

struct in6_ifreq {
    int                  ifr6_ifindex;
    int                  ifr6_len;                              /* size of buffer in bytes              */
    struct in6_ifr_addr *ifr6_addr_array;
};

#define ifr6_addr        ifr6_addr_array->ifr6a_addr
#define ifr6_prefixlen   ifr6_addr_array->ifr6a_prefixlen

/*********************************************************************************************************
  sylixos if6 ioctl 
  
  SIOCSIFADDR6, SIOCSIFNETMASK6, SIOCSIFDSTADDR6, SIOCDIFADDR6
  
  ifr6_len must == sizeof(struct in6_ifr_addr))
*********************************************************************************************************/

#define SIOCGSIZIFREQ6      _IOR('i', 106, struct in6_ifreq)    /* size of buffer in bytes              */

#define SIOCSIFADDR6        _IOW('i', 12, struct in6_ifreq)
#define SIOCSIFNETMASK6     _IOW('i', 22, struct in6_ifreq)
#define SIOCSIFDSTADDR6     _IOW('i', 14, struct in6_ifreq)

#define SIOCGIFADDR6        _IOWR('i', 33, struct in6_ifreq)
#define SIOCGIFNETMASK6     _IOWR('i', 37, struct in6_ifreq)
#define SIOCGIFDSTADDR6     _IOWR('i', 34, struct in6_ifreq)

#define SIOCDIFADDR6        _IOW('i', 35, struct in6_ifreq)

/*********************************************************************************************************
  proto private 0x89e0 ~ 0x89ef
*********************************************************************************************************/

#define SIOCPROTOPRIVATE    0x89e0                              /* to 0x89ef                            */

/*********************************************************************************************************
  device private 0x89f0 ~ 0x89ff
*********************************************************************************************************/

#define SIOCDEVPRIVATE      0x89f0                              /* to 89ff                              */

/*********************************************************************************************************
  Structure describing information about an interface
  which may be of interest to management entities.
*********************************************************************************************************/

struct if_data {
                                                                /* generic interface information        */
    u_char  ifi_type;                                           /* ethernet, tokenring, etc             */
    u_char  ifi_physical;                                       /* e.g., AUI, Thinnet, 10base-T, etc    */
    u_char  ifi_addrlen;                                        /* media address length                 */
    u_char  ifi_hdrlen;                                         /* media header length                  */
    u_char  ifi_recvquota;                                      /* polling quota for receive intrs      */
    u_char  ifi_xmitquota;                                      /* polling quota for xmit intrs         */
    u_long  ifi_mtu;                                            /* maximum transmission unit            */
    u_long  ifi_metric;                                         /* routing metric (external only)       */
    u_long  ifi_baudrate;                                       /* linespeed                            */
                                                                /* volatile statistics                  */
    u_long  ifi_ipackets;                                       /* packets received on interface        */
    u_long  ifi_ierrors;                                        /* input errors on interface            */
    u_long  ifi_opackets;                                       /* packets sent on interface            */
    u_long  ifi_oerrors;                                        /* output errors on interface           */
    u_long  ifi_collisions;                                     /* collisions on csma interfaces        */
    u_long  ifi_ibytes;                                         /* total number of octets received      */
    u_long  ifi_obytes;                                         /* total number of octets sent          */
    u_long  ifi_imcasts;                                        /* packets received via multicast       */
    u_long  ifi_omcasts;                                        /* packets sent via multicast           */
    u_long  ifi_iqdrops;                                        /* dropped on input, this interface     */
    u_long  ifi_noproto;                                        /* destined for unsupported protocol    */
    u_long  ifi_recvtiming;                                     /* usec spent receiving when timing     */
    u_long  ifi_xmittiming;                                     /* usec spent xmitting when timing      */
    struct  timeval ifi_lastchange;                             /* time of last administrative change   */
};

/*********************************************************************************************************
  Message format for use in obtaining information about interfaces
  from getkerninfo and the routing socket
*********************************************************************************************************/

struct if_msghdr {
    u_short ifm_msglen;                                         /* to skip over non-understood messages */
    u_char  ifm_version;                                        /* future binary compatibility          */
    u_char  ifm_type;                                           /* message type                         */
    
    int     ifm_addrs;                                          /* like rtm_addrs                       */
    int     ifm_flags;                                          /* value of if_flags                    */
    u_short ifm_index;                                          /* index for associated ifp             */
    struct  if_data ifm_data;                                   /* statistics and other data about if   */
};

/*********************************************************************************************************
  Message format for use in obtaining information about interface addresses
  from getkerninfo and the routing socket
*********************************************************************************************************/

struct ifa_msghdr {
    u_short ifam_msglen;                                        /* to skip over non-understood messages */
    u_char  ifam_version;                                       /* future binary compatibility          */
    u_char  ifam_type;                                          /* message type                         */
    
    int     ifam_addrs;                                         /* like rtm_addrs                       */
    int     ifam_flags;                                         /* value of ifa_flags                   */
    u_short ifam_index;                                         /* index for associated ifp             */
    int     ifam_metric;                                        /* value of ifa_metric                  */
};

/*********************************************************************************************************
  RTM_NEWMADDR & RTM_DELMADDR
*********************************************************************************************************/

struct ifma_msghdr {
    u_short ifmam_msglen;                                       /* to skip over non-understood messages */
    u_char  ifmam_version;                                      /* future binary compatibility          */
    u_char  ifmam_type;                                         /* message type                         */

    int     ifmam_addrs;                                        /* like rtm_addrs                       */
    int     ifmam_flags;                                        /* value of ifa_flags                   */
    u_short ifmam_index;                                        /* index for associated ifp             */
};

/*********************************************************************************************************
  RTM_IFANNOUNCE
*********************************************************************************************************/

struct if_announcemsghdr {
    u_short ifan_msglen;                                        /* to skip over non-understood messages */
    u_char  ifan_version;                                       /* future binary compatibility          */
    u_char  ifan_type;                                          /* message type                         */

    u_short ifan_index;                                         /* index for associated ifp             */
    char    ifan_name[IFNAMSIZ];                                /* if name, e.g. "en0"                  */
    u_short ifan_what;                                          /* what type of announcement            */
};

#define IFAN_ARRIVAL        0                                   /* interface arrival                    */
#define IFAN_DEPARTURE      1                                   /* interface departure                  */

#ifdef __cplusplus
}
#endif                                                          /*  __cplusplus                         */

#endif                                                          /*  LW_CFG_NET_EN                       */
#endif                                                          /*  __IF_H                              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
