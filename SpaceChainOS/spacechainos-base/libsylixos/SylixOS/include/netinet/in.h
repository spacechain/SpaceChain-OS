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
** 文   件   名: in.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2010 年 09 月 03 日
**
** 描        述: include/netinet/in .
*********************************************************************************************************/

#ifndef __NETINET_IN_H
#define __NETINET_IN_H

#include "netinet6/in6.h"

/*********************************************************************************************************
 Local port number conventions:
 Ports < IPPORT_RESERVED are reserved for
 privileged processes (e.g. root).
 Ports > IPPORT_USERRESERVED are reserved
 for servers, not necessarily privileged.
*********************************************************************************************************/

#define IPPORT_RESERVED         1024
#define IPPORT_USERRESERVED     5000

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN         16
#endif

/*********************************************************************************************************
  IP protocol
*********************************************************************************************************/

#define IPPROTO_IPIP    4                                       /* for compatibility                    */
#define IPPROTO_IGMP    2                                       /* IGMP                                 */
#define IPPROTO_RSVP    46                                      /* resource reservation                 */
#define IPPROTO_ENCAP   98                                      /* encapsulation header                 */
#define IPPROTO_PIM     103                                     /* Protocol Independent Mcast           */

/*********************************************************************************************************
  Defaults and limits for options
*********************************************************************************************************/

#define IP_DEFAULT_MULTICAST_TTL    1                           /* normally limit mcasts to 1 hop       */
#define IP_DEFAULT_MULTICAST_LOOP   1                           /* normally hear sends if a member      */
#define IP_MAX_MEMBERSHIPS          20                          /* per socket                           */

/*********************************************************************************************************
  SylixOS Multicast
*********************************************************************************************************/

#define IN_LOCAL_GROUP(i)           (((UINT32)(i) & 0xffffff00) == 0xe0000000)

#define INADDR_UNSPEC_GROUP         (UINT32)0xe0000000          /*  224.0.0.0                           */
#define INADDR_ALLHOSTS_GROUP       (UINT32)0xe0000001          /*  224.0.0.1                           */
#define INADDR_ALLRTRS_GROUP        (UINT32)0xe0000002          /*  224.0.0.2                           */
#define INADDR_NEW_ALLRTRS_GROUP    (UINT32)0xe0000016          /*  224.0.0.22                          */
#define INADDR_MAX_LOCAL_GROUP      (UINT32)0xe00000ff          /*  224.0.0.255                         */

#define IP_RSVP_ON          15                                  /* enable RSVP in kernel                */
#define IP_RSVP_OFF         16                                  /* disable RSVP in kernel               */
#define IP_RSVP_VIF_ON      17                                  /* set RSVP per-vif socket              */
#define IP_RSVP_VIF_OFF     18                                  /* unset RSVP per-vif socket            */

/*********************************************************************************************************
  SylixOS add
*********************************************************************************************************/

#define inet_network(cp)    ntohl(inet_addr(cp))

#endif                                                          /*  __NETINET_IN_H                      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
