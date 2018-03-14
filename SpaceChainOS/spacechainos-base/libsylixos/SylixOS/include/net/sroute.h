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
** 文   件   名: sroute.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2017 年 12 月 18 日
**
** 描        述: source router.
*********************************************************************************************************/

#ifndef __NET_SROUTE_H
#define __NET_SROUTE_H

#include "sys/types.h"
#include "sys/socket.h"

/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_NET_ROUTER > 0) && (LW_CFG_NET_BALANCING > 0)

#include "if.h"
#include "route.h"

#ifdef __cplusplus
extern "C" {
#endif                                          /*  __cplusplus                                         */

/*********************************************************************************************************
  This structure gets passed by the SIOCADDSRT, SIOCDELSRT, SIOCCHGSRT, SIOCGETSRT calls. 
  SIOCGETSRT need set srt_ssrc, srt_esrc.
*********************************************************************************************************/

struct srtentry {
    struct sockaddr srt_ssrc;                   /* source address start                                 */
    struct sockaddr srt_esrc;                   /* source address end                                   */
    struct sockaddr srt_sdest;                  /* destination address start                            */
    struct sockaddr srt_edest;                  /* destination address end                              */
    char            srt_ifname[IF_NAMESIZE];    /* interface                                            */
    u_short         srt_flags;
    u_short         srt_mode;
    u_short         srt_prio;
    u_short         srt_pad1;
    u_long          srt_pad2[16];               /* for feature                                          */
};

#define SRT_MODE_EXCLUDE    0                   /* exclude destination                                  */
#define SRT_MODE_INCLUDE    1                   /* include destination                                  */

#define SRT_PRIO_HIGH       0                   /* priority high than route table                       */
#define SRT_PRIO_DEFAULT    1                   /* priority low than route table                        */

#define SIOCADDSRT  _IOW( 'r', 100, struct srtentry)
#define SIOCDELSRT  _IOW( 'r', 101, struct srtentry)
#define SIOCCHGSRT  _IOW( 'r', 102, struct srtentry)
#define SIOCGETSRT  _IOWR('r', 103, struct srtentry)

/*********************************************************************************************************
  This structure gets passed by the SIOCLSTSRT calls. 
*********************************************************************************************************/

struct srtentry_list {
    u_long           srtl_bcnt;                 /* struct rtsentry buffer count                         */
    u_long           srtl_num;                  /* system return how many source route entry in buffer  */
    u_long           srtl_total;                /* system return total number source route entry        */
    struct srtentry *srtl_buf;                  /* source route list buffer                             */
};

#define SIOCLSTSRT  _IOWR('r', 104, struct srtentry_list)

#ifdef __cplusplus
}
#endif                                          /*  __cplusplus                                         */

#endif                                          /*  LW_CFG_NET_EN > 0                                   */
                                                /*  LW_CFG_NET_ROUTER > 0                               */
                                                /*  LW_CFG_NET_BALANCING > 0                            */
#endif                                          /*  __NET_SROUTE_H                                      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
