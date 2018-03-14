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
** 文   件   名: Lw_Api_Shell.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 14 日
**
** 描        述: 这是系统提供给 C / C++ 用户的内核应用程序接口层。
                 为了适应不同语言习惯的人，这里使用了很多重复功能.
*********************************************************************************************************/

#ifndef __LW_API_SHELL_H
#define __LW_API_SHELL_H

/*********************************************************************************************************
    API
*********************************************************************************************************/

#define Lw_TShell_Init                  API_TShellInit
#define Lw_TShell_Startup               API_TShellStartup
#define Lw_TShell_SetStackSize          API_TShellSetStackSize
#define Lw_TShell_Create                API_TShellCreate
#define Lw_TShell_CreateEx              API_TShellCreateEx

#define Lw_TShell_GetUserName           API_TShellGetUserName
#define Lw_TShell_GetUserHome           API_TShellGetUserHome
#define Lw_TShell_GetGrpName            API_TShellGetGrpName
#define Lw_TShell_FlushCache            API_TShellFlushCache

#define Lw_TShell_KeywordAdd            API_TShellKeywordAdd
#define Lw_TShell_KeywordAddEx          API_TShellKeywordAddEx
#define Lw_TShell_FormatAdd             API_TShellFormatAdd
#define Lw_TShell_HelpAdd               API_TShellHelpAdd
#define Lw_TShell_Exec                  API_TShellExec
#define Lw_TShell_ExecBg                API_TShellExecBg

#define Lw_TShell_VarHookSet            API_TShellVarHookSet
#define Lw_TShell_VarGetRt              API_TShellVarGetRt
#define Lw_TShell_VarGet                API_TShellVarGet
#define Lw_TShell_VarSet                API_TShellVarSet
#define Lw_TShell_VarDelete             API_TShellVarDelete
#define Lw_TShell_VarGetNum             API_TShellVarGetNum
#define Lw_TShell_VarDup                API_TShellVarDup

#endif                                                                  /*  __LW_API_SHELL_H            */
/*********************************************************************************************************
    OBJECT
*********************************************************************************************************/
