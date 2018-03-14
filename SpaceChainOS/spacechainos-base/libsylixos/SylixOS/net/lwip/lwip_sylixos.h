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
** 文   件   名: lwip_sylixos.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 05 月 06 日
**
** 描        述: sylixos inet (lwip)
*********************************************************************************************************/

#ifndef __LWIP_SYLIXOS_H
#define __LWIP_SYLIXOS_H

/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0

/*********************************************************************************************************
  API_NetJobDeleteEx() 第一个参数为 LW_NETJOB_Q_ALL 表示对所有队列均有效
*********************************************************************************************************/

#define LW_NETJOB_Q_ALL     ((UINT)-1)

/*********************************************************************************************************
  网络初始化及网卡工作队列.
*********************************************************************************************************/

LW_API VOID         API_NetInit(VOID);                                  /*  安装网络组件                */

LW_API VOID         API_NetSnmpInit(VOID);                              /*  启动 SNMP                   */

LW_API INT          API_NetJobAdd(VOIDFUNCPTR  pfunc, 
                                  PVOID        pvArg0,
                                  PVOID        pvArg1,
                                  PVOID        pvArg2,
                                  PVOID        pvArg3,
                                  PVOID        pvArg4,
                                  PVOID        pvArg5);                 /*  net job add                 */
LW_API INT          API_NetJobAddEx(UINT         uiQ,
                                    VOIDFUNCPTR  pfunc, 
                                    PVOID        pvArg0,
                                    PVOID        pvArg1,
                                    PVOID        pvArg2,
                                    PVOID        pvArg3,
                                    PVOID        pvArg4,
                                    PVOID        pvArg5);               /*  net job add                 */
                                  
LW_API VOID         API_NetJobDelete(UINT         uiMatchArgNum,
                                     VOIDFUNCPTR  pfunc, 
                                     PVOID        pvArg0,
                                     PVOID        pvArg1,
                                     PVOID        pvArg2,
                                     PVOID        pvArg3,
                                     PVOID        pvArg4,
                                     PVOID        pvArg5);              /*  net job delete              */
LW_API VOID         API_NetJobDeleteEx(UINT         uiQ,
                                       UINT         uiMatchArgNum,
                                       VOIDFUNCPTR  pfunc, 
                                       PVOID        pvArg0,
                                       PVOID        pvArg1,
                                       PVOID        pvArg2,
                                       PVOID        pvArg3,
                                       PVOID        pvArg4,
                                       PVOID        pvArg5);            /*  net job delete              */

LW_API size_t       API_NetJobGetLost(VOID);

#define netInit             API_NetInit
#define netSnmpInit         API_NetSnmpInit

#define netJobAdd           API_NetJobAdd
#define netJobAddEx         API_NetJobAddEx
#define netJobDelete        API_NetJobDelete
#define netJobDeleteEx      API_NetJobDeleteEx
#define netJobGetLost       API_NetJobGetLost

#endif                                                                  /*  LW_CFG_NET_EN               */
#endif                                                                  /*  __LWIP_SYLIXOS_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
