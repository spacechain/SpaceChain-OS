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
** 文   件   名: ppcSprE500.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 05 月 05 日
**
** 描        述: PowerPC E500 体系构架特殊功能寄存器接口.
*********************************************************************************************************/

#ifndef __ARCH_PPCSPRE500_H
#define __ARCH_PPCSPRE500_H

UINT32  ppcE500GetDEAR(VOID);
UINT32  ppcE500GetESR(VOID);

UINT32  ppcE500GetHID0(VOID);
VOID    ppcE500SetHID0(UINT32  uiValue);

UINT32  ppcE500GetHID1(VOID);
VOID    ppcE500SetHID1(UINT32  uiValue);

UINT32  ppcE500GetTCR(VOID);
VOID    ppcE500SetTCR(UINT32  uiValue);

UINT32  ppcE500GetTSR(VOID);
VOID    ppcE500SetTSR(UINT32  uiValue);

UINT32  ppcE500GetDECAR(VOID);
VOID    ppcE500SetDECAR(UINT32  uiValue);

UINT32  ppcE500GetMCAR(VOID);

#endif                                                                  /*  __ARCH_PPCSPRE500_H         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
