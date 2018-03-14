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
** 文   件   名: pim.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2018 年 01 月 02 日
**
** 描        述: include/netinet/pim.
*********************************************************************************************************/

#ifndef __NETINET_PIM_H
#define __NETINET_PIM_H

#include "lwip/opt.h"
#include "lwip/def.h"

#if LWIP_IPV4 && IP_FORWARD && LWIP_IGMP

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
#ifdef _PIM_VT
    u_char  pim_vt;                                             /* PIM version and message type         */
#else                                                           /* ! _PIM_VT                            */
#if defined(BYTE_ORDER) && (BYTE_ORDER == LITTLE_ENDIAN)
    u_char  pim_type:4,     
            pim_ver:4;                                          /* PIM version number; 2 for PIMv2      */
#else
    u_char  pim_ver:4,                                          /* PIM version                          */
            pim_type:4;                                         /* PIM type                             */
#endif
#endif                                                          /* ! _PIM_VT                            */
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

#ifdef _PIM_VT
#define PIM_MAKE_VT(v, t)   (0xff & (((v) << 4) | (0x0f & (t))))
#define PIM_VT_V(x)         (((x) >> 4) & 0x0f)
#define PIM_VT_T(x)         ((x) & 0x0f)
#endif                                                          /* _PIM_VT                              */

/*********************************************************************************************************
  PIM message types
*********************************************************************************************************/

#ifndef PIM_REGISTER
#define PIM_HELLO           0x0                                 /* PIM-SM and PIM-DM                    */
#define PIM_REGISTER        0x1                                 /* PIM-SM only                          */
#define PIM_REGISTER_STOP   0x2                                 /* PIM-SM only                          */
#define PIM_JOIN_PRUNE      0x3                                 /* PIM-SM and PIM-DM                    */
#define PIM_BOOTSTRAP       0x4                                 /* PIM-SM only                          */
#define PIM_ASSERT          0x5                                 /* PIM-SM and PIM-DM                    */
#define PIM_GRAFT           0x6                                 /* PIM-DM only                          */
#define PIM_GRAFT_ACK       0x7                                 /* PIM-DM only                          */
#define PIM_CAND_RP_ADV     0x8                                 /* PIM-SM only                          */
#define PIM_ALL_DF_ELECTION 0xa                                 /* Bidir-PIM-SM only                    */
#endif                                                          /* !PIM_REGISTER                        */

/*********************************************************************************************************
  PIM-Register message flags
*********************************************************************************************************/

#ifndef PIM_NULL_REGISTER
#define PIM_BORDER_REGISTER 0x80000000U                         /* The Border bit (host-order)          */
#define PIM_NULL_REGISTER   0x40000000U                         /* The Null-Register bit (host-order)   */
#endif                                                          /* !PIM_NULL_REGISTER                   */

/*********************************************************************************************************
  All-PIM-Routers IPv4 and IPv6 multicast addresses
*********************************************************************************************************/

#define INADDR_ALLPIM_ROUTERS_GROUP         (UINT32)0xe000000dU /* 224.0.0.13                           */

#ifndef IN6ADDR_LINKLOCAL_ALLPIM_ROUTERS
#define IN6ADDR_LINKLOCAL_ALLPIM_ROUTERS    "ff02::d"
#define IN6ADDR_LINKLOCAL_ALLPIM_ROUTERS_INIT \
        {{{ 0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0d }}}
#endif                                                          /* !IN6ADDR_LINKLOCAL_ALLPIM_ROUTERS    */

/*********************************************************************************************************
  Protocol Independent Multicast (PIM),
  kernel variables and implementation-specific definitions.
 
  Written by George Edmond Eddy (Rusty), ISI, February 1998.
  Modified by Pavlin Radoslavov, USC/ISI, May 1998, Aug 1999, October 2000.
  Modified by Hitoshi Asaeda, WIDE, August 1998.
*********************************************************************************************************/
/*********************************************************************************************************
  PIM statistics kept in the kernel
*********************************************************************************************************/

struct pimstat {
    uint64_t    pims_rcv_total_msgs;                            /* total PIM messages received          */
    uint64_t    pims_rcv_total_bytes;                           /* total PIM bytes received             */
    uint64_t    pims_rcv_tooshort;                              /* rcvd with too few bytes              */
    uint64_t    pims_rcv_badsum;                                /* rcvd with bad checksum               */
    uint64_t    pims_rcv_badversion;                            /* rcvd bad PIM version                 */
    uint64_t    pims_rcv_registers_msgs;                        /* rcvd regs. msgs (data only)          */
    uint64_t    pims_rcv_registers_bytes;                       /* rcvd regs. bytes (data only)         */
    uint64_t    pims_rcv_registers_wrongiif;                    /* rcvd regs. on wrong iif              */
    uint64_t    pims_rcv_badregisters;                          /* rcvd invalid registers               */
    uint64_t    pims_snd_registers_msgs;                        /* sent regs. msgs (data only)          */
    uint64_t    pims_snd_registers_bytes;                       /* sent regs. bytes (data only)         */
};

/*********************************************************************************************************
  Identifiers for PIM sysctl nodes
*********************************************************************************************************/

#define PIMCTL_STATS    1                                       /* statistics (read-only)               */

#endif                                                          /* LWIP_IPV4 && IP_FORWARD && LWIP_IGMP */
#endif                                                          /*  __NETINET_PIM_H                     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
