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
** 文   件   名: ip.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2014 年 05 月 04 日
**
** 描        述: include/netinet/ip.
*********************************************************************************************************/

#ifndef __NETINET_IP_H
#define __NETINET_IP_H

#include "lwip/sockets.h"
#include "sys/types.h"

/*********************************************************************************************************
 * Definitions for internet protocol version 4.
 * Per RFC 791, September 1981.
*********************************************************************************************************/

#define IPVERSION   4

/*********************************************************************************************************
 * Structure of an internet header, naked of options.
*********************************************************************************************************/

struct ip {
#ifdef _IP_VHL
    u_char  ip_vhl;                             /* version << 4 | header length >> 2                    */
#else
#if BYTE_ORDER == LITTLE_ENDIAN
    u_int   ip_hl:4,                            /* header length                                        */
            ip_v:4;                             /* version                                              */
#endif
#if BYTE_ORDER == BIG_ENDIAN
    u_int   ip_v:4,                             /* version                                              */
            ip_hl:4;                            /* header length                                        */
#endif
#endif                                          /* not _IP_VHL                                          */
    u_char  ip_tos;                             /* type of service                                      */
    u_short ip_len;                             /* total length                                         */
    u_short ip_id;                              /* identification                                       */
    u_short ip_off;                             /* fragment offset field                                */
#define IP_RF       0x8000                      /* reserved fragment flag                               */
#define IP_DF       0x4000                      /* dont fragment flag                                   */
#define IP_MF       0x2000                      /* more fragments flag                                  */
#define IP_OFFMASK  0x1fff                      /* mask for fragmenting bits                            */
    u_char  ip_ttl;                             /* time to live                                         */
    u_char  ip_p;                               /* protocol                                             */
    u_short ip_sum;                             /* checksum                                             */
    struct in_addr  ip_src,ip_dst;              /* source and dest address                              */
} __attribute__((__packed__));

#ifdef _IP_VHL
#define IP_MAKE_VHL(v, hl)  ((v) << 4 | (hl))
#define IP_VHL_HL(vhl)      ((vhl) & 0x0f)
#define IP_VHL_V(vhl)       ((vhl) >> 4)
#define IP_VHL_BORING       0x45
#endif

#define IP_MAXPACKET        65535               /* maximum packet size                                  */

/*********************************************************************************************************
 * Definitions for IP type of service (ip_tos)
 * ECN RFC3168 obsoletes RFC2481, and these will be deprecated soon.
*********************************************************************************************************/

#define IPTOS_CE            0x01
#define IPTOS_ECT           0x02

/*********************************************************************************************************
 * ECN (Explicit Congestion Notification) codepoints in RFC3168
 * mapped to the lower 2 bits of the TOS field.
*********************************************************************************************************/

#define IPTOS_ECN_NOTECT    0x00                /* not-ECT                                              */
#define IPTOS_ECN_ECT1      0x01                /* ECN-capable transport (1)                            */
#define IPTOS_ECN_ECT0      0x02                /* ECN-capable transport (0)                            */
#define IPTOS_ECN_CE        0x03                /* congestion experienced                               */
#define IPTOS_ECN_MASK      0x03                /* ECN field mask                                       */

/*********************************************************************************************************
 * Definitions for options.
*********************************************************************************************************/

#define IPOPT_COPIED(o)     ((o)&0x80)
#define IPOPT_CLASS(o)      ((o)&0x60)
#define IPOPT_NUMBER(o)     ((o)&0x1f)

#define IPOPT_CONTROL       0x00
#define IPOPT_RESERVED1     0x20
#define IPOPT_DEBMEAS       0x40
#define IPOPT_RESERVED2     0x60

#define IPOPT_EOL           0                   /* end of option list                                   */
#define IPOPT_NOP           1                   /* no operation                                         */

#define IPOPT_RR            7                   /* record packet route                                  */
#define IPOPT_TS            68                  /* timestamp                                            */
#define IPOPT_SECURITY      130                 /* provide s,c,h,tcc                                    */
#define IPOPT_LSRR          131                 /* loose source route                                   */
#define IPOPT_SATID         136                 /* satnet id                                            */
#define IPOPT_SSRR          137                 /* strict source route                                  */
#define IPOPT_RA            148                 /* router alert                                         */

/*********************************************************************************************************
 * Offsets to fields in options other than EOL and NOP.
*********************************************************************************************************/

#define IPOPT_OPTVAL        0                   /* option ID                                            */
#define IPOPT_OLEN          1                   /* option length                                        */
#define IPOPT_OFFSET        2                   /* offset within option                                 */
#define IPOPT_MINOFF        4                   /* min value of above                                   */

/*********************************************************************************************************
 * Time stamp option structure.
*********************************************************************************************************/

struct ip_timestamp {
    u_char  ipt_code;                           /* IPOPT_TS                                             */
    u_char  ipt_len;                            /* size of structure (variable)                         */
    u_char  ipt_ptr;                            /* index of current entry                               */
#if BYTE_ORDER == LITTLE_ENDIAN
    u_int   ipt_flg:4,                          /* flags, see below                                     */
            ipt_oflw:4;                         /* overflow counter                                     */
#endif
#if BYTE_ORDER == BIG_ENDIAN
    u_int   ipt_oflw:4,                         /* overflow counter                                     */
            ipt_flg:4;                          /* flags, see below                                     */
#endif
    union ipt_timestamp {
        u_int32_t ipt_time[1];
        struct ipt_ta {
            struct in_addr ipt_addr;
            u_int32_t ipt_time;
        } ipt_ta[1];
    } ipt_timestamp;
} __attribute__((__packed__));

/*********************************************************************************************************
 * flag bits for ipt_flg 
*********************************************************************************************************/

#define IPOPT_TS_TSONLY         0               /* timestamps only                                      */
#define IPOPT_TS_TSANDADDR      1               /* timestamps and addresses                             */
#define IPOPT_TS_PRESPEC        3               /* specified modules only                               */

/*********************************************************************************************************
 * bits for security (not byte swapped)
*********************************************************************************************************/

#define IPOPT_SECUR_UNCLASS     0x0000
#define IPOPT_SECUR_CONFID      0xf135
#define IPOPT_SECUR_EFTO        0x789a
#define IPOPT_SECUR_MMMM        0xbc4d
#define IPOPT_SECUR_RESTR       0xaf13
#define IPOPT_SECUR_SECRET      0xd788
#define IPOPT_SECUR_TOPSECRET   0x6bc5

/*********************************************************************************************************
 * Internet implementation parameters.
*********************************************************************************************************/

#define MAXTTL                  255             /* maximum time to live (seconds)                       */
#define IPDEFTTL                64              /* default ttl, from RFC 1340                           */
#define IPFRAGTTL               60              /* time to live for frags, slowhz                       */
#define IPTTLDEC                1               /* subtracted when forwarding                           */

#define IP_MSS                  576             /* default maximum segment size                         */

#endif                                          /*  __NETINET_IP_H                                      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
