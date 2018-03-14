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
** 文   件   名: ttinyVar.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 08 月 06 日
**
** 描        述: 一个超小型的 shell 系统的变量管理器.
*********************************************************************************************************/

#ifndef __TTINYVAR_H
#define __TTINYVAR_H

/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0

/*********************************************************************************************************
  API
*********************************************************************************************************/
LW_API VOID       (*API_TShellVarHookSet(VOID  (*pfuncTSVarHook)()))();

LW_API INT          API_TShellVarGetRt(CPCHAR     pcVarName, 
                                       PCHAR      pcVarValue,
                                       INT        iMaxLen);
LW_API PCHAR        API_TShellVarGet(CPCHAR       pcVarName);
LW_API INT          API_TShellVarSet(CPCHAR       pcVarName, CPCHAR       pcVarValue, INT  iIsOverwrite);
LW_API INT          API_TShellVarDelete(CPCHAR  pcVarName);
LW_API INT          API_TShellVarGetNum(VOID);
LW_API INT          API_TShellVarDup(PVOID (*pfuncMalloc)(size_t stSize), PCHAR  ppcEvn[], ULONG  ulMax);

#define tshellVarHookSet        API_TShellVarHookSet
#define tshellVarGetRt          API_TShellVarGetRt
#define tshellVarGet            API_TShellVarGet
#define tshellVarSet            API_TShellVarSet
#define tshellVarDelete         API_TShellVarDelete
#define tshellVarGetNum         API_TShellVarGetNum
#define tshellVarDup            API_TShellVarDup

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
#endif                                                                  /*  __TTINYVAR_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
