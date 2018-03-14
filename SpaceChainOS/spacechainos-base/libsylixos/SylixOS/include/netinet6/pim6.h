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
** 文   件   名: pim6.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2018 年 01 月 02 日
**
** 描        述: include/netinet6/pim6.
*********************************************************************************************************/

#ifndef __NETINET6_PIM6_H
#define __NETINET6_PIM6_H

#include "lwip/opt.h"
#include "lwip/def.h"

#if LWIP_IPV6 && LWIP_IPV6_FORWARD && LWIP_IPV6_MLD

#include "sys/types.h"
#include "sys/endian.h"
#include "lwip/inet.h"
#include "net/if.h"

/*********************************************************************************************************
  PIM packet header
*********************************************************************************************************/

#ifndef __pim_defined
#define __pim_defined   1
struct pim {
#if defined(BYTE_ORDER) && (BYTE_ORDER == LITTLE_ENDIAN)
    u_char  pim_type:4,     
            pim_ver:4;                                          /* PIM version number; 2 for PIMv2      */
#else
    u_char  pim_ver:4,                                          /* PIM version                          */
            pim_type:4;                                         /* PIM type                             */
#endif
    u_char  pim_rsv;                                            /* Reserved                             */
    u_short pim_cksum;                                          /* IP style check sum                   */
};

#define PIM_REG_MINLEN      (PIM_MINLEN + 20)                   /* PIM Register hdr + inner IPv4 hdr    */
#define PIM6_REG_MINLEN     (PIM_MINLEN + 40)                   /* PIM Register hdr + inner IPv6 hdr    */

/*********************************************************************************************************
  KAME-related name backward compatibility
*********************************************************************************************************/

#define PIM_VERSION         2
#define PIM_MINLEN          8                                   /* PIM message min. length              */
#endif                                                          /* __pim_defined                        */

/*********************************************************************************************************
  Message types
*********************************************************************************************************/

#ifndef PIM_REGISTER
#define PIM_REGISTER        1                                   /* PIM Register type is 1               */
#endif                                                          /* !PIM_REGISTER                        */

/*********************************************************************************************************
  PIM-Register message flags
*********************************************************************************************************/

#ifndef PIM_NULL_REGISTER
#define PIM_NULL_REGISTER   0x40000000U                         /* The Null-Register bit (host-order)   */
#endif                                                          /* !PIM_NULL_REGISTER                   */

/*********************************************************************************************************
  All-PIM-Routers IPv6 multicast addresses
*********************************************************************************************************/

#ifndef IN6ADDR_LINKLOCAL_ALLPIM_ROUTERS
#define IN6ADDR_LINKLOCAL_ALLPIM_ROUTERS    "ff02::d"
#define IN6ADDR_LINKLOCAL_ALLPIM_ROUTERS_INIT \
        {{{ 0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0d }}}
#endif                                                          /* !IN6ADDR_LINKLOCAL_ALLPIM_ROUTERS    */

/*********************************************************************************************************
  PIM statistics kept in the kernel
*********************************************************************************************************/

struct pim6stat {
    uint64_t    pim6s_rcv_total;                                /* total PIM messages received          */
    uint64_t    pim6s_rcv_tooshort;                             /* received with too few bytes          */
    uint64_t    pim6s_rcv_badsum;                               /* received with bad checksum           */
    uint64_t    pim6s_rcv_badversion;                           /* received bad PIM version             */
    uint64_t    pim6s_rcv_registers;                            /* received registers                   */
    uint64_t    pim6s_rcv_badregisters;                         /* received invalid registers           */
    uint64_t    pim6s_snd_registers;                            /* sent registers                       */
};

/*********************************************************************************************************
  Identifiers for PIM sysctl nodes
*********************************************************************************************************/

#define PIM6CTL_STATS    1                                      /* statistics (read-only)               */

#endif                                                          /* LWIP_IPV6 && LWIP_IPV6_FORWARD &&    */
                                                                /* LWIP_IPV6_MLD                        */
#endif                                                          /*  __NETINET6_PIM6_H                   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
