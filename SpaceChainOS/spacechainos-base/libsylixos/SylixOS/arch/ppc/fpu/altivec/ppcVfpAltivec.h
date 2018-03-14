/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: ppcVfpAltivec.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 05 月 04 日
**
** 描        述: PowerPC 体系架构 ALTIVEC 支持.
*********************************************************************************************************/

#ifndef __ARCH_PPCVFPALTIVEC_H
#define __ARCH_PPCVFPALTIVEC_H

PPPC_FPU_OP  ppcVfpAltivecPrimaryInit(CPCHAR  pcMachineName, CPCHAR  pcFpuName);
VOID         ppcVfpAltivecSecondaryInit(CPCHAR  pcMachineName, CPCHAR  pcFpuName);

#endif                                                                  /*  __ARCH_PPCVFPALTIVEC_H      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
