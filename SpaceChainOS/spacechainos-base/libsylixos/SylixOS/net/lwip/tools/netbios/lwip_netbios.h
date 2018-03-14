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
** 文   件   名: lwip_netbios.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 05 月 06 日
**
** 描        述: lwip netbios 简易名字服务器. (相关源码来源于 contrib)
*********************************************************************************************************/

#ifndef __NETBIOS_H
#define __NETBIOS_H

/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_NET_NETBIOS_EN > 0)

LW_API VOID     API_INetNetBiosInit(VOID);
LW_API ULONG    API_INetNetBiosNameSet(CPCHAR  pcLocalName);
LW_API ULONG    API_INetNetBiosNameGet(PCHAR   pcLocalNameBuffer, INT  iMaxLen);

#define inetNetBiosInit         API_INetNetBiosInit
#define inetNetBiosNameSet      API_INetNetBiosNameSet
#define inetNetBiosNameGet      API_INetNetBiosNameGet

#endif                                                                  /*  (LW_CFG_NET_EN > 0)         */
                                                                        /*  (LW_CFG_NET_NETBIOS_EN > 0) */
#endif                                                                  /*  __NETBIOS_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
