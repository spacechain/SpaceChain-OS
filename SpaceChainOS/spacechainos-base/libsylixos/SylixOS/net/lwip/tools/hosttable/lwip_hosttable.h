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
** 文   件   名: lwip_hosttable.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 08 月 19 日
**
** 描        述: lwip 动态 DNS 本地主机表.
*********************************************************************************************************/

#ifndef __LWIP_HOSTTABLE_H
#define __LWIP_HOSTTABLE_H

#include "lwip/inet.h"
/*********************************************************************************************************
  互联网静态地址转换表操作
*********************************************************************************************************/

LW_API INT          API_INetHostTableGetItem(CPCHAR  pcHost, struct in_addr  *pinaddr);
LW_API INT          API_INetHostTableAddItem(CPCHAR  pcHost, struct in_addr  inaddr);
LW_API INT          API_INetHostTableDelItem(CPCHAR  pcHost);

#define inetHostTableGetItem        API_INetHostTableGetItem
#define inetHostTableAddItem        API_INetHostTableAddItem
#define inetHostTableDelItem        API_INetHostTableDelItem

#endif                                                                  /*  __LWIP_HOSTTABLE_H          */
/*********************************************************************************************************
  END
*********************************************************************************************************/
