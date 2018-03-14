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
** 文   件   名: lwip_loginbl.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2017 年 01 月 09 日
**
** 描        述: 网络登录黑名单管理.
*********************************************************************************************************/

#ifndef __LWIP_LOGINBL_H
#define __LWIP_LOGINBL_H

#include "sys/socket.h"

/*********************************************************************************************************
  API
*********************************************************************************************************/
LW_API INT  API_LoginBlAdd(const struct sockaddr *addr, UINT  uiRep, UINT  uiSec);
LW_API INT  API_LoginBlDelete(const struct sockaddr *addr);
LW_API VOID API_LoginBlShow(VOID);

LW_API INT  API_LoginWlAdd(const struct sockaddr *addr);
LW_API INT  API_LoginWlDelete(const struct sockaddr *addr);
LW_API VOID API_LoginWlShow(VOID);

#define netLoginBlAdd       API_LoginBlAdd
#define netLoginBlDelete    API_LoginBlDelete
#define netLoginBlShow      API_LoginBlShow

#define netLoginWlAdd       API_LoginWlAdd
#define netLoginWlDelete    API_LoginWlDelete
#define netLoginWlShow      API_LoginWlShow

#endif                                                                  /*  __LWIP_LOGINBL_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
