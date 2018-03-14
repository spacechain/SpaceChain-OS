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
** 文   件   名: arch_inc.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 05 月 04 日
**
** 描        述: 体系相关头文件.
*********************************************************************************************************/

#ifndef __ARCH_INC_H
#define __ARCH_INC_H

#include "config/cpu/cpu_cfg.h"

#if defined(LW_CFG_CPU_ARCH_ARM)
#include "arm/arch_types.h"
#include "arm/arch_compiler.h"
#include "arm/arch_float.h"
#include "arm/arch_limits.h"
#include "arm/arch_regs.h"
#include "arm/arch_mmu.h"
#include "arm/arch_mpu.h"

#elif defined(LW_CFG_CPU_ARCH_X86)
#include "x86/arch_types.h"
#include "x86/arch_compiler.h"
#include "x86/arch_float.h"
#include "x86/arch_limits.h"
#include "x86/arch_regs.h"
#include "x86/arch_mmu.h"
#include "x86/arch_pc.h"

#elif defined(LW_CFG_CPU_ARCH_MIPS)
#include "mips/arch_types.h"
#include "mips/arch_def.h"
#include "mips/arch_compiler.h"
#include "mips/arch_float.h"
#include "mips/arch_limits.h"
#include "mips/arch_regs.h"
#include "mips/arch_mmu.h"

#elif defined(LW_CFG_CPU_ARCH_PPC)
#include "ppc/arch_types.h"
#include "ppc/arch_def.h"
#include "ppc/arch_compiler.h"
#include "ppc/arch_float.h"
#include "ppc/arch_limits.h"
#include "ppc/arch_regs.h"
#include "ppc/arch_mmu.h"

#elif defined(LW_CFG_CPU_ARCH_C6X)
#include "c6x/arch_types.h"
#include "c6x/arch_compiler.h"
#include "c6x/arch_float.h"
#include "c6x/arch_limits.h"
#include "c6x/arch_regs.h"
#include "c6x/arch_mmu.h"

#elif defined(LW_CFG_CPU_ARCH_SPARC)
#include "sparc/arch_types.h"
#include "sparc/arch_compiler.h"
#include "sparc/arch_def.h"
#include "sparc/arch_float.h"
#include "sparc/arch_limits.h"
#include "sparc/arch_regs.h"
#include "sparc/arch_mmu.h"
#endif                                                                  /*  LW_CFG_CPU_ARCH_ARM         */

#endif                                                                  /*  __ARCH_INC_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
