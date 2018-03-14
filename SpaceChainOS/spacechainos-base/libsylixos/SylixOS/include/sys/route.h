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
** 文   件   名: route.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 01 月 15 日
**
** 描        述: SylixOS 简易路由接口.
*********************************************************************************************************/

#ifndef __SYS_ROUTE_H
#define __SYS_ROUTE_H

#include "sys/types.h"
#include "sys/socket.h"

/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_NET_ROUTER > 0)

#include "net/if.h"
#include "net/route.h"

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

LW_API int  route_add(const struct rtentry *prtentry);
LW_API int  route_delete(const struct rtentry *prtentry);
LW_API int  route_change(const struct rtentry *prtentry);
LW_API int  route_get(struct rtentry *prtentry);
LW_API int  route_list(INT  iFamily, struct rtentry_list *prtentrylist);

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_NET_ROUTER > 0       */
#endif                                                                  /*  __SYS_ROUTE_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
