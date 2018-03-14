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
** 文   件   名: ttinyShellColor.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2014 年 07 月 08 日
**
** 描        述: tty 颜色系统.
*********************************************************************************************************/

#ifndef __TTINYSHELLCOLOR_H
#define __TTINYSHELLCOLOR_H

/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0

/*********************************************************************************************************
  颜色定义
*********************************************************************************************************/

#define LW_TSHELL_COLOR_NONE          "\033[m"
#define LW_TSHELL_COLOR_RED           "\033[0;31m"
#define LW_TSHELL_COLOR_LIGHT_RED     "\033[1;31m"
#define LW_TSHELL_COLOR_GREEN         "\033[0;32m"
#define LW_TSHELL_COLOR_LIGHT_GREEN   "\033[1;32m"
#define LW_TSHELL_COLOR_BLUE          "\033[0;34m"
#define LW_TSHELL_COLOR_LIGHT_BLUE    "\033[1;34m"
#define LW_TSHELL_COLOR_DARY_GRAY     "\033[1;30m"
#define LW_TSHELL_COLOR_CYAN          "\033[0;36m"
#define LW_TSHELL_COLOR_LIGHT_CYAN    "\033[1;36m"
#define LW_TSHELL_COLOR_PURPLE        "\033[0;35m"
#define LW_TSHELL_COLOR_LIGHT_PURPLE  "\033[1;35m"
#define LW_TSHELL_COLOR_BROWN         "\033[0;33m"
#define LW_TSHELL_COLOR_YELLOW        "\033[1;33m"
#define LW_TSHELL_COLOR_LIGHT_GRAY    "\033[0;37m"
#define LW_TSHELL_COLOR_WHITE         "\033[1;37m"

LW_API  VOID  API_TShellColorRefresh(VOID);
LW_API  VOID  API_TShellColorStart(CPCHAR  pcName, CPCHAR  pcLink, mode_t  mode, INT  iFd);
LW_API  VOID  API_TShellColorStart2(CPCHAR  pcColor, INT  iFd);
LW_API  VOID  API_TShellColorEnd(INT  iFd);

#define tshellColorRefresh  API_TShellColorRefresh
#define tshellColorStart    API_TShellColorStart
#define tshellColorStart2   API_TShellColorStart2
#define tshellColorEnd      API_TShellColorEnd

#ifdef __SYLIXOS_KERNEL
VOID    __tshellColorInit(VOID);
#endif                                                                  /*  __SYLIXOS_KERNEL            */

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
#endif                                                                  /*  __TTINYSHELLCOLOR_H         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
