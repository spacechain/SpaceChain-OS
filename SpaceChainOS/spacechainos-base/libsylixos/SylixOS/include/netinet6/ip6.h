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
** 文   件   名: ip6.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2014 年 05 月 04 日
**
** 描        述: include/netinet6/ip6.
*********************************************************************************************************/

#ifndef __NETINET6_IP6_H
#define __NETINET6_IP6_H

#include "lwip/sockets.h"
#include "sys/types.h"

/*********************************************************************************************************
 * Definition for internet protocol version 6.
 * RFC 2460
*********************************************************************************************************/

struct ip6_hdr {
    union {
        struct ip6_hdrctl {
            u_int32_t ip6_un1_flow;             /* 20 bits of flow-ID                                   */
            u_int16_t ip6_un1_plen;             /* payload length                                       */
            u_int8_t  ip6_un1_nxt;              /* next header                                          */
            u_int8_t  ip6_un1_hlim;             /* hop limit                                            */
        } ip6_un1;
        u_int8_t ip6_un2_vfc;                   /* 4 bits version, top 4 bits class                     */
    } ip6_ctlun;
    struct in6_addr ip6_src;                    /* source address                                       */
    struct in6_addr ip6_dst;                    /* destination address                                  */
} __attribute__((__packed__));

#define ip6_vfc     ip6_ctlun.ip6_un2_vfc
#define ip6_flow    ip6_ctlun.ip6_un1.ip6_un1_flow
#define ip6_plen    ip6_ctlun.ip6_un1.ip6_un1_plen
#define ip6_nxt     ip6_ctlun.ip6_un1.ip6_un1_nxt
#define ip6_hlim    ip6_ctlun.ip6_un1.ip6_un1_hlim
#define ip6_hops    ip6_ctlun.ip6_un1.ip6_un1_hlim

#define IPV6_VERSION        0x60
#define IPV6_VERSION_MASK   0xf0

#if BYTE_ORDER == BIG_ENDIAN
#define IPV6_FLOWINFO_MASK  0x0fffffff          /* flow info (28 bits)                                  */
#define IPV6_FLOWLABEL_MASK 0x000fffff          /* flow label (20 bits)                                 */
#else
#if BYTE_ORDER == LITTLE_ENDIAN
#define IPV6_FLOWINFO_MASK  0xffffff0f          /* flow info (28 bits)                                  */
#define IPV6_FLOWLABEL_MASK 0xffff0f00          /* flow label (20 bits)                                 */
#endif                                          /* LITTLE_ENDIAN                                        */
#endif

/*********************************************************************************************************
  ECN bits proposed by Sally Floyd 
*********************************************************************************************************/

#define IP6TOS_CE   0x01                        /* congestion experienced                               */
#define IP6TOS_ECT  0x02                        /* ECN-capable transport                                */

/*********************************************************************************************************
  Extension Headers
*********************************************************************************************************/

struct ip6_ext {
    u_int8_t ip6e_nxt;
    u_int8_t ip6e_len;
} __attribute__((__packed__));

/*********************************************************************************************************
  Hop-by-Hop options header 
  XXX should we pad it to force alignment on an 8-byte boundar
*********************************************************************************************************/

struct ip6_hbh {
    u_int8_t ip6h_nxt;                          /* next header                                          */
    u_int8_t ip6h_len;                          /* length in units of 8 octets                          */
    /* 
     * followed by options 
     */
} __attribute__((__packed__));

/*********************************************************************************************************
  Destination options header
  XXX should we pad it to force alignment on an 8-byte boundary? 
*********************************************************************************************************/

struct ip6_dest {
    u_int8_t ip6d_nxt;                          /* next header                                          */
    u_int8_t ip6d_len;                          /* length in units of 8 octets                          */
    /* 
     * followed by options 
     */
} __attribute__((__packed__));

/*********************************************************************************************************
  Option types and related macros 
*********************************************************************************************************/

#define IP6OPT_PAD1             0x00            /* 00 0 00000                                           */
#define IP6OPT_PADN             0x01            /* 00 0 00001                                           */
#define IP6OPT_JUMBO            0xC2            /* 11 0 00010 = 194                                     */
#define IP6OPT_NSAP_ADDR        0xC3            /* 11 0 00011                                           */
#define IP6OPT_TUNNEL_LIMIT     0x04            /* 00 0 00100                                           */
#define IP6OPT_RTALERT          0x05            /* 00 0 00101 (KAME definition)                         */

#define IP6OPT_RTALERT_LEN      4
#define IP6OPT_RTALERT_MLD      0               /* Datagram contains an MLD message                     */
#define IP6OPT_RTALERT_RSVP     1               /* Datagram contains an RSVP message                    */
#define IP6OPT_RTALERT_ACTNET   2               /* contains an Active Networks msg                      */
#define IP6OPT_MINLEN           2

#define IP6OPT_BINDING_UPDATE   0xc6            /* 11 0 00110                                           */
#define IP6OPT_BINDING_ACK      0x07            /* 00 0 00111                                           */
#define IP6OPT_BINDING_REQ      0x08            /* 00 0 01000                                           */
#define IP6OPT_HOME_ADDRESS     0xc9            /* 11 0 01001                                           */
#define IP6OPT_EID              0x8a            /* 10 0 01010                                           */

#define IP6OPT_TYPE(o)          ((o) & 0xC0)
#define IP6OPT_TYPE_SKIP        0x00
#define IP6OPT_TYPE_DISCARD     0x40
#define IP6OPT_TYPE_FORCEICMP   0x80
#define IP6OPT_TYPE_ICMP        0xC0

#define IP6OPT_MUTABLE          0x20

#define IP6OPT_JUMBO_LEN        6

/*********************************************************************************************************
  Routing header 
*********************************************************************************************************/

struct ip6_rthdr {
    u_int8_t  ip6r_nxt;                         /* next header                                          */
    u_int8_t  ip6r_len;                         /* length in units of 8 octets                          */
    u_int8_t  ip6r_type;                        /* routing type                                         */
    u_int8_t  ip6r_segleft;                     /* segments left                                        */
    /* 
     * followed by routing type specific data 
     */
} __attribute__((__packed__));

/*********************************************************************************************************
  Type 0 Routing header
*********************************************************************************************************/

struct ip6_rthdr0 {
    u_int8_t  ip6r0_nxt;                        /* next header                                          */
    u_int8_t  ip6r0_len;                        /* length in units of 8 octets                          */
    u_int8_t  ip6r0_type;                       /* always zero                                          */
    u_int8_t  ip6r0_segleft;                    /* segments left                                        */
    u_int8_t  ip6r0_reserved;                   /* reserved field                                       */
    u_int8_t  ip6r0_slmap[3];                   /* strict/loose bit map                                 */
    struct in6_addr  ip6r0_addr[1];             /* up to 23 addresses                                   */
} __attribute__((__packed__));

/*********************************************************************************************************
  Fragment header
*********************************************************************************************************/

struct ip6_frag {
    u_int8_t  ip6f_nxt;                         /* next header                                          */
    u_int8_t  ip6f_reserved;                    /* reserved field                                       */
    u_int16_t ip6f_offlg;                       /* offset, reserved, and flag                           */
    u_int32_t ip6f_ident;                       /* identification                                       */
} __attribute__((__packed__));

#if BYTE_ORDER == BIG_ENDIAN
#define IP6F_OFF_MASK           0xfff8          /* mask out offset from _offlg                          */
#define IP6F_RESERVED_MASK      0x0006          /* reserved bits in ip6f_offlg                          */
#define IP6F_MORE_FRAG          0x0001          /* more-fragments flag                                  */
#else                                           /* BYTE_ORDER == LITTLE_ENDIAN                          */
#define IP6F_OFF_MASK           0xf8ff          /* mask out offset from _offlg                          */
#define IP6F_RESERVED_MASK      0x0600          /* reserved bits in ip6f_offlg                          */
#define IP6F_MORE_FRAG          0x0100          /* more-fragments flag                                  */
#endif                                          /* BYTE_ORDER == LITTLE_ENDIAN                          */

/*********************************************************************************************************
  Internet implementation parameters.
*********************************************************************************************************/

#define IPV6_MAXHLIM            255             /* maximun hoplimit                                     */
#define IPV6_DEFHLIM            64              /* default hlim                                         */
#define IPV6_FRAGTTL            120             /* ttl for fragment packets, in slowtimo tick           */
#define IPV6_HLIMDEC            1               /* subtracted when forwaeding                           */

#define IPV6_MMTU               1280            /* minimal MTU and reassembly. 1024 + 256               */
#define IPV6_MAXPACKET          65535           /* ip6 max packet size without Jumbo payload            */

#endif                                          /*  __NETINET6_IP6_H                                    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
