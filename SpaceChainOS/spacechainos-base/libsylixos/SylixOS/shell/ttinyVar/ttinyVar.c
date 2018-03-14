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
** 文   件   名: ttinyVar.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 08 月 06 日
**
** 描        述: 一个超小型的 shell 系统的变量管理器.

** BUG:
2012.10.23  加入变量删除接口.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/shell/include/ttiny_shell.h"
/*********************************************************************************************************
  应用级 API
*********************************************************************************************************/
#include "../SylixOS/api/Lw_Api_Kernel.h"
#include "../SylixOS/api/Lw_Api_System.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0
#include "../SylixOS/shell/hashLib/hashHorner.h"
#include "../SylixOS/shell/ttinyShell/ttinyShell.h"
#include "../SylixOS/shell/ttinyShell/ttinyShellLib.h"
#include "ttinyVarLib.h"
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
VOID   (*_G_pfuncTSVarHook)() = LW_NULL;                                /*  变量改变回调                */
/*********************************************************************************************************
** 函数名称: API_TShellVarHookSet
** 功能描述: 当变量改变时, 调用的用户回调
** 输　入  : pfuncTSVarHook    新的回调函数
** 输　出  : 以前的回调函数.
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID   (*API_TShellVarHookSet(VOID  (*pfuncTSVarHook)()))()
{
    VOID  (*pfuncTSVarOldHook)() = _G_pfuncTSVarHook;
    
    _G_pfuncTSVarHook = pfuncTSVarHook;
    
    return  (pfuncTSVarOldHook);
}
/*********************************************************************************************************
** 函数名称: API_TShellVarGetRt
** 功能描述: 获得一个变量的值
** 输　入  : pcVarName     变量名
**           pcVarValue    变量的值
** 输　出  : 变量值长度 or ERROR 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_TShellVarGetRt (CPCHAR  pcVarName, 
                         PCHAR   pcVarValue,
                         INT     iMaxLen)
{
    if (!pcVarName || !pcVarValue || !iMaxLen) {                        /*  参数检查                    */
        _ErrorHandle(ERROR_TSHELL_EPARAM);
        return  (PX_ERROR);
    }
    
    return  (__tshellVarGetRt(pcVarName, pcVarValue, iMaxLen));
}
/*********************************************************************************************************
** 函数名称: API_TShellVarGet
** 功能描述: 获得一个变量的值
** 输　入  : pcVarName     变量名
** 输　出  : 变量的值
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
PCHAR   API_TShellVarGet (CPCHAR  pcVarName)
{
    REGISTER ULONG      ulError;
             PCHAR      pcVarValue;
    
    if (!pcVarName) {                                                   /*  参数检查                    */
        _ErrorHandle(ERROR_TSHELL_EPARAM);
        return  (LW_NULL);
    }
    
    ulError = __tshellVarGet(pcVarName, &pcVarValue);
    if (ulError) {
        return  (LW_NULL);
    } else {
        return  (pcVarValue);
    }
}
/*********************************************************************************************************
** 函数名称: API_TShellVarSet
** 功能描述: 设置一个变量的值
** 输　入  : pcVarName     变量名
**           pcVarValue    变量的值
**           iIsOverwrite  是否覆盖
**                         如果 iIsOverwrite 不为0，而该变量原已有内容，则原内容会被改为参数 pcVarValue 
**                         所指的变量内容：如果 iIsOverwrite 为0，且该环境变量已有内容，
**                         则参数 pcVarValue 会被忽略。
** 输　出  : 执行成功则返回0，有错误发生时返回-1。
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT   API_TShellVarSet (CPCHAR  pcVarName, CPCHAR  pcVarValue, INT  iIsOverwrite)
{
    REGISTER ULONG      ulError;
    
    if (!pcVarName) {                                                   /*  参数检查                    */
        _ErrorHandle(ERROR_TSHELL_EPARAM);
        return  (PX_ERROR);
    }
    
    ulError = __tshellVarSet(pcVarName, pcVarValue, iIsOverwrite);
    if (ulError == ERROR_TSHELL_EVAR) {
        ulError =  __tshellVarAdd(pcVarName, pcVarValue, 
                                  lib_strlen(pcVarName));
    }
    
    if (ulError) {
        return  (PX_ERROR);
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: API_TShellVarDelete
** 功能描述: 删除一个变量
** 输　入  : pcVarName     变量名
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_TShellVarDelete (CPCHAR  pcVarName)
{
    REGISTER ULONG      ulError;
    
    if (!pcVarName) {                                                   /*  参数检查                    */
        _ErrorHandle(ERROR_TSHELL_EPARAM);
        return  (PX_ERROR);
    }
    
    ulError = __tshellVarDeleteByName(pcVarName);
    if (ulError) {
        return  (PX_ERROR);
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: API_TShellVarGetNum
** 功能描述: 获得变量个数
** 输　入  : NONE
** 输　出  : 变量个数
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT   API_TShellVarGetNum (VOID)
{
    return  (__tshellVarNum());
}
/*********************************************************************************************************
** 函数名称: API_TShellVarDup
** 功能描述: dup shell 变量
** 输　入  : pfuncMalloc       内存分配函数
**           ppcEvn            dup 目标
**           ulMax             最大个数
** 输　出  : dup 个数
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT   API_TShellVarDup (PVOID (*pfuncMalloc)(size_t stSize), PCHAR  ppcEvn[], ULONG  ulMax)
{
    if (!pfuncMalloc || !ppcEvn || !ulMax) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    return  (__tshellVarDup(pfuncMalloc, ppcEvn, ulMax));
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
