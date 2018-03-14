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
** 文   件   名: ttinyString.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 07 月 27 日
**
** 描        述: 一个超小型的 shell 系统, 字符串基本处理函数.
*********************************************************************************************************/

#ifndef __TTINYSTRING_H
#define __TTINYSTRING_H

/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0

ULONG  __tshellStrConvertVar(CPCHAR       pcCmd, PCHAR  pcCmdOut);
ULONG  __tshellStrDelCRLF(CPCHAR       pcCmd);
ULONG  __tshellStrFormat(CPCHAR       pcCmd, PCHAR  pcCmdOut);
ULONG  __tshellStrKeyword(CPCHAR       pcCmd, PCHAR  pcBuffer, PCHAR  *ppcParam);
ULONG  __tshellStrGetToken(CPCHAR       pcCmd, PCHAR  *ppcNext);
ULONG  __tshellStrDelTransChar(CPCHAR       pcCmd, PCHAR  pcCmdOut);

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
#endif                                                                  /*  __TTINYSTRING_H             */
/*********************************************************************************************************
  END
*********************************************************************************************************/
