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
** 文   件   名: flowctl.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2017 年 12 月 18 日
**
** 描        述: SylixOS 路由器流量控制.
*********************************************************************************************************/

#ifndef __INC_NET_FLOWCTL_H
#define __INC_NET_FLOWCTL_H

#include "sys/types.h"
#include "sys/socket.h"

/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_NET_FLOWCTL_EN > 0)

#include "if.h"

#ifdef __cplusplus
extern "C" {
#endif                                          /*  __cplusplus                                         */

/*********************************************************************************************************
  fc_type
*********************************************************************************************************/

#define FCT_IP      1                           /* ip flowctl                                           */
#define FCT_IF      2                           /* net interface flowctl                                */

/*********************************************************************************************************
  This structure gets passed by the SIOCADDFC, SIOCDELFC, SIOCCHGFC, SIOCGETFC calls. 
  SIOCGETFC need set fc_type, fc_proto, fc_start, fc_end, fc_sport, fc_eport, fc_ifname.
*********************************************************************************************************/

struct fcentry {
    u_char          fc_type;                    /* FCT_*                                                */
    u_char          fc_enable;                  /* 1: enable 0: disable                                 */
    u_char          fc_proto;                   /* IPPROTO_ICMP / TCP / UDP / ICMPV6 ... 0: all proto   */
    u_char          fc_pad1;
    struct sockaddr fc_start;                   /* flowctl start address                                */
    struct sockaddr fc_end;                     /* flowctl end address                                  */
    u_short         fc_sport;                   /* flowctl start port (NET byte order)                  */
    u_short         fc_eport;                   /* flowctl end port (NET byte order)                    */
    char            fc_ifname[IF_NAMESIZE];     /* LAN interface name                                   */
    u_int64_t       fc_uprate;                  /* LAN up stream bits rate                              */
    u_int64_t       fc_downrate;                /* LAN down stream bits rate                            */
    size_t          fc_bufsize;                 /* buffer size (16K ~ 128M Bytes)                       */
    u_long          fc_pad2[16];                /* for feature                                          */
};

#define SIOCADDFC       _IOW( 'f', 10, struct fcentry)
#define SIOCDELFC       _IOW( 'f', 11, struct fcentry)
#define SIOCCHGFC       _IOW( 'f', 12, struct fcentry)
#define SIOCGETFC       _IOWR('f', 13, struct fcentry)

/*********************************************************************************************************
  This structure gets passed by the SIOCLSTFC calls. 
*********************************************************************************************************/

struct fcentry_list {
    u_char          fcl_type;                   /* FCT_*                                                */
    u_long          fcl_bcnt;                   /* struct fcentry buffer count                          */
    u_long          fcl_num;                    /* system return how many flowctl entry in buffer       */
    u_long          fcl_total;                  /* system return total number flowctl entry             */
    struct fcentry *fcl_buf;                    /* flowctl list buffer                                  */
};

#define SIOCLSTFC       _IOWR('f', 14, struct fcentry_list)

#ifdef __cplusplus
}
#endif                                          /*  __cplusplus                                         */

#endif                                          /*  LW_CFG_NET_EN > 0                                   */
                                                /*  LW_CFG_NET_FLOWCTL_EN > 0                           */
#endif                                          /*  __INC_NET_FLOWCTL_H                                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
