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
** 文   件   名: if_arp.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2014 年 05 月 09 日
**
** 描        述: ARP.
*********************************************************************************************************/

#ifndef __IF_ARP_H
#define __IF_ARP_H

#include "sys/types.h"
#include "sys/ioctl.h"

/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0

#ifdef __cplusplus
extern "C" {
#endif                                                      /*  __cplusplus                             */

/*********************************************************************************************************
 * Address Resolution Protocol.
 *
 * See RFC 826 for protocol description.  ARP packets are variable
 * in size; the arphdr structure defines the fixed-length portion.
 * Protocol type values are the same as those for 10 Mb/s Ethernet.
 * It is followed by the variable-sized fields ar_sha, arp_spa,
 * arp_tha and arp_tpa in that order, according to the lengths
 * specified.  Field names used correspond to RFC 826.
*********************************************************************************************************/

struct arphdr {
    u_short     ar_hrd;                                     /* format of hardware address               */
#define ARPHRD_ETHER        1                               /* ethernet hardware format                 */
#define ARPHRD_IEEE802      6                               /* token-ring hardware format               */
#define ARPHRD_FRELAY       15                              /* frame relay hardware format              */
    u_short     ar_pro;                                     /* format of protocol address               */
    u_char      ar_hln;                                     /* length of hardware address               */
    u_char      ar_pln;                                     /* length of protocol address               */
    u_short     ar_op;                                      /* one of: */
#define ARPOP_REQUEST       1                               /* request to resolve address               */
#define ARPOP_REPLY         2                               /* response to previous request             */
#define ARPOP_REVREQUEST    3                               /* request protocol address given hardware  */
#define ARPOP_REVREPLY      4                               /* response giving protocol address         */
#define ARPOP_INVREQUEST    8                               /* request to identify peer                 */
#define ARPOP_INVREPLY      9                               /* response identifying peer                */

/*********************************************************************************************************
 * The remaining fields are variable in size,
 * according to the sizes above.
*********************************************************************************************************/

#ifdef COMMENT_ONLY
    u_char    ar_sha[];                                     /* sender hardware address                  */
    u_char    ar_spa[];                                     /* sender protocol address                  */
    u_char    ar_tha[];                                     /* target hardware address                  */
    u_char    ar_tpa[];                                     /* target protocol address                  */
#endif
};

/*********************************************************************************************************
 * ARP ioctl request
*********************************************************************************************************/

struct arpreq {
    struct sockaddr    arp_pa;                              /* protocol address                         */
    struct sockaddr    arp_ha;                              /* hardware address                         */
    int                arp_flags;                           /* flags                                    */
    struct sockaddr    arp_netmask;                         /* netmask (only for proxy arps)            */
    char               arp_dev[16];
};

/*********************************************************************************************************
 * arp_flags and at_flags field values 
*********************************************************************************************************/

#define ATF_INUSE           0x01                            /* entry in use                             */
#define ATF_COM             0x02                            /* completed entry (enaddr valid)           */
#define ATF_PERM            0x04                            /* permanent entry                          */
#define ATF_PUBL            0x08                            /* publish entry (respond for other host)   */
#define ATF_USETRAILERS     0x10                            /* has requested trailers                   */

/*********************************************************************************************************
 * arp ioctl command
*********************************************************************************************************/

#define SIOCSARP            _IOW('i', 30, struct arpreq)    /* set arp entry                            */
#define SIOCGARP            _IOWR('i',38, struct arpreq)    /* get arp entry                            */
#define SIOCDARP            _IOW('i', 32, struct arpreq)    /* delete arp entry                         */

#ifdef __cplusplus
}
#endif                                                      /*  __cplusplus                             */

#endif                                                      /*  LW_CFG_NET_EN                           */
#endif                                                      /*  __IF_ARP_H                              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
