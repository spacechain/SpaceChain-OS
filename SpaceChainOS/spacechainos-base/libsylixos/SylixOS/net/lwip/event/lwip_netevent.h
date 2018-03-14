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
** 文   件   名: lwip_netevent.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2014 年 06 月 28 日
**
** 描        述: 网络事件接口.
*********************************************************************************************************/

#ifndef __LWIP_NETEVENT_H
#define __LWIP_NETEVENT_H

/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0

#include "net/if.h"

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

/*********************************************************************************************************
  网络事件文件
*********************************************************************************************************/

#define NET_EVENT_DEV_PATH              "/dev/netevent"                 /*  网络事件设备路径            */
#define NET_EVENT_DEV_MAX_MSGSIZE       (4 + IFNAMSIZ + (4 * 4))        /*  最大消息长度                */

/*********************************************************************************************************
  网络事件类型
*********************************************************************************************************/
/*********************************************************************************************************
  通用事件类型
*********************************************************************************************************/

#define NET_EVENT_STD           0                                       /*  网卡标准事件                */

#define NET_EVENT_ADD           (NET_EVENT_STD + 0)                     /*  网卡添加                    */
#define NET_EVENT_REMOVE        (NET_EVENT_STD + 1)                     /*  网卡删除                    */

#define NET_EVENT_UP            (NET_EVENT_STD + 2)                     /*  网卡使能                    */
#define NET_EVENT_DOWN          (NET_EVENT_STD + 3)                     /*  网卡禁能                    */

#define NET_EVENT_LINK          (NET_EVENT_STD + 4)                     /*  网卡已连接                  */
#define NET_EVENT_UNLINK        (NET_EVENT_STD + 5)                     /*  网卡断开连接                */

#define NET_EVENT_ADDR          (NET_EVENT_STD + 6)                     /*  网卡地址变化                */
#define NET_EVENT_ADDR_CONFLICT (NET_EVENT_STD + 9)                     /*  网卡地址冲突                */
#define NET_EVENT_AUTH_FAIL     (NET_EVENT_STD + 7)                     /*  网卡认证失败                */
#define NET_EVENT_AUTH_TO       (NET_EVENT_STD + 8)                     /*  网卡认证超时                */

/*********************************************************************************************************
  PPP 事件类型
*********************************************************************************************************/

#define NET_EVENT_PPP           100

#define NET_EVENT_PPP_DEAD      (NET_EVENT_PPP + 0)                     /*  连接停止                    */
#define NET_EVENT_PPP_INIT      (NET_EVENT_PPP + 1)                     /*  进入初始化过程              */
#define NET_EVENT_PPP_AUTH      (NET_EVENT_PPP + 2)                     /*  进入用户认证                */
#define NET_EVENT_PPP_RUN       (NET_EVENT_PPP + 3)                     /*  网络连通                    */
#define NET_EVENT_PPP_DISCONN   (NET_EVENT_PPP + 4)                     /*  进入连接中断                */

/*********************************************************************************************************
  wireless 事件类型
*********************************************************************************************************/

#define NET_EVENT_WL            200                                     /*  无线网卡事件                */

#define NET_EVENT_WL_QUAL       (NET_EVENT_WL + 0)                      /*  网卡无线环境变化(信号强度等)*/
#define NET_EVENT_WL_SCAN       (NET_EVENT_WL + 1)                      /*  无线网卡 AP 扫描结束        */
#define NET_EVENT_WL_EXT        (NET_EVENT_WL + 50)                     /*  用户自定义无线事件          */
#define NET_EVENT_WL_EXT2       (NET_EVENT_WL + 51)                     /*  用户自定义无线事件2         */

/*********************************************************************************************************
  内核 API
*********************************************************************************************************/

#ifdef __SYLIXOS_KERNEL
VOID  netEventIfAdd(struct netif *pnetif);
VOID  netEventIfRemove(struct netif *pnetif);
VOID  netEventIfUp(struct netif *pnetif);
VOID  netEventIfDown(struct netif *pnetif);
VOID  netEventIfLink(struct netif *pnetif);
VOID  netEventIfUnlink(struct netif *pnetif);
VOID  netEventIfAddr(struct netif *pnetif);
VOID  netEventIfAddrConflict(struct netif *pnetif, UINT8 ucHw[], UINT uiHwLen);
VOID  netEventIfAuthFail(struct netif *pnetif);
VOID  netEventIfAuthTo(struct netif *pnetif);
VOID  netEventIfPppExt(struct netif *pnetif, UINT32  uiEvent);
VOID  netEventIfWlQual(struct netif *pnetif);
VOID  netEventIfWlScan(struct netif *pnetif);
VOID  netEventIfWlExt(struct netif *pnetif, 
                      UINT32        uiEvent, 
                      UINT32        uiArg0,
                      UINT32        uiArg1,
                      UINT32        uiArg2,
                      UINT32        uiArg3);
VOID  netEventIfWlExt2(struct netif *pnetif,
                       PVOID         pvEvent,
                       UINT32        uiArg);
#endif                                                                  /*  __SYLIXOS_KERNEL            */

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
#endif                                                                  /*  __LWIP_NETEVENT_H           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
