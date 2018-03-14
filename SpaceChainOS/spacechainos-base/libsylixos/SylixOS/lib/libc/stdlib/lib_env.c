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
** 文   件   名: lib_env.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 08 月 30 日
**
** 描        述: 系统库.

** BUG:
2012.10.19  lib_getenv() 不再使用 static buffer 形式, 而是直接从 API_TShellVarGet() 返回环境变量的缓冲.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/shell/include/ttiny_shell.h"
/*********************************************************************************************************
** 函数名称: lib_getenv_r
** 功能描述: 获得当前系统环境变量. (可重入)
** 输　入  : pcName    变量名
**           pcBuffer  变量的值
**           iLen      长度
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
int  lib_getenv_r (const char  *pcName, char  *pcBuffer, int  iLen)
{
#if LW_CFG_SHELL_EN > 0
    if (API_TShellVarGetRt(pcName, pcBuffer, iLen) >= 0) {
        return  (ERROR_NONE);
    } else {
        return  (PX_ERROR);
    }
#else
    return  (PX_ERROR);
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
}
/*********************************************************************************************************
** 函数名称: lib_getenv
** 功能描述: 获得当前系统环境变量.
** 输　入  : pcName    变量名
** 输　出  : 变量的值
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
char  *lib_getenv (const char  *pcName)
{
#if LW_CFG_SHELL_EN > 0
    return  (API_TShellVarGet(pcName));
#else
    return  (PX_ERROR);
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
}
/*********************************************************************************************************
** 函数名称: lib_putenv
** 功能描述: 设置当前系统环境变量.
** 输　入  : cString   变量赋值字串
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
int  lib_putenv (char  *cString)
{
#if LW_CFG_SHELL_EN > 0
    return  (API_TShellExec(cString));
#else
    return  (PX_ERROR);
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
}
/*********************************************************************************************************
** 函数名称: lib_setenv
** 功能描述: 设置当前系统环境变量.
** 输　入  : pcName    变量名
**           pcValue   变量的值
**           iRewrite  存在时是否改写
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
int  lib_setenv (const char  *pcName, const char  *pcValue, int  iRewrite)
{
#if LW_CFG_SHELL_EN > 0
    return  (API_TShellVarSet(pcName, pcValue, iRewrite));
#else
    return  (PX_ERROR);
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
}
/*********************************************************************************************************
** 函数名称: lib_unsetenv
** 功能描述: 删除一个指定的系统环境变量.
** 输　入  : pcName    变量名
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
int lib_unsetenv (const char  *pcName)
{
#if LW_CFG_SHELL_EN > 0
    return  (API_TShellVarDelete(pcName));
#else
    return  (PX_ERROR);
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
