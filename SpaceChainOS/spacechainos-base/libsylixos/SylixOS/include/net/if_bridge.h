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
** 文   件   名: if_bridge.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2017 年 12 月 05 日
**
** 描        述: 桥接网络接口.
*********************************************************************************************************/

#ifndef __IF_BRIDGE_H
#define __IF_BRIDGE_H

#include "sys/types.h"
#include "sys/ioctl.h"

/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_NET_DEV_BRIDGE_EN > 0)

#include "if.h"

#ifdef __cplusplus
extern "C" {
#endif                                                      /*  __cplusplus                             */

/*********************************************************************************************************
 网桥控制参数.
*********************************************************************************************************/
struct net_bridge_ctl {
    int     br_index;                                       /*  only for NETBR_CTL_ADD return           */
    char    br_dev[IFNAMSIZ];                               /*  bridge device name                      */
    char    eth_dev[IFNAMSIZ];                              /*  sub ethernet device name                */
};

#define NETBR_CTL_ADD           _IOWR('b', 0, struct net_bridge_ctl)
#define NETBR_CTL_DELETE        _IOW( 'b', 1, struct net_bridge_ctl)
#define NETBR_CTL_ADD_DEV       _IOW( 'b', 2, struct net_bridge_ctl)
#define NETBR_CTL_DELETE_DEV    _IOW( 'b', 3, struct net_bridge_ctl)
#define NETBR_CTL_ADD_IF        _IOW( 'b', 4, struct net_bridge_ctl)
#define NETBR_CTL_DELETE_IF     _IOW( 'b', 5, struct net_bridge_ctl)
#define NETBR_CTL_CACHE_FLUSH   FIOFLUSH

#define NETBR_CTL_PATH          "/dev/netbr"

#ifdef __cplusplus
}
#endif                                                      /*  __cplusplus                             */

#endif                                                      /*  LW_CFG_NET_EN                           */
                                                            /*  LW_CFG_NET_DEV_BRIDGE_EN                */
#endif                                                      /*  __IF_BRIDGE_H                           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
