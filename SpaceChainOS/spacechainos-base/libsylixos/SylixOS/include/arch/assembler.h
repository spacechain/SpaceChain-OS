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
** 文   件   名: assembler.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 06 月 14 日
**
** 描        述: 体系结构汇编相关.
*********************************************************************************************************/

#ifndef __ARCH_ASSEMBLER_H
#define __ARCH_ASSEMBLER_H

#include "config/cpu/cpu_cfg.h"

#if defined(LW_CFG_CPU_ARCH_ARM)
#include "arm/asm/assembler.h"

#elif defined(LW_CFG_CPU_ARCH_X86)
#include "x86/asm/assembler.h"

#elif defined(LW_CFG_CPU_ARCH_MIPS)
#include "mips/asm/assembler.h"

#elif defined(LW_CFG_CPU_ARCH_PPC)
#include "ppc/asm/assembler.h"

#elif defined(LW_CFG_CPU_ARCH_C6X)
#include "c6x/asm/assembler.h"

#elif defined(LW_CFG_CPU_ARCH_SPARC)
#include "sparc/asm/assembler.h"
#endif                                                                  /*  LW_CFG_CPU_ARCH_ARM         */

#endif                                                                  /*  __ARCH_ASSEMBLER_H          */
/*********************************************************************************************************
  END
*********************************************************************************************************/
