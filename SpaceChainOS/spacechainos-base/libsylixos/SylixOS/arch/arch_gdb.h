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
** 文   件   名: arch_gdb.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2014 年 05 月 22 日
**
** 描        述: SylixOS 体系构架 GDB 调试接口.
*********************************************************************************************************/

#ifndef __ARCH_GDB_H
#define __ARCH_GDB_H

#include "config/cpu/cpu_cfg.h"

#if defined(LW_CFG_CPU_ARCH_ARM)
#include "./arm/arm_gdb.h"

#elif defined(LW_CFG_CPU_ARCH_X86)
#include "./x86/x86_gdb.h"

#elif defined(LW_CFG_CPU_ARCH_MIPS)
#include "./mips/mips_gdb.h"

#elif defined(LW_CFG_CPU_ARCH_PPC)
#include "./ppc/ppc_gdb.h"

#elif defined(LW_CFG_CPU_ARCH_C6X)
#include "./c6x/c6x_gdb.h"

#elif defined(LW_CFG_CPU_ARCH_SPARC)
#include "./sparc/sparc_gdb.h"
#endif                                                                  /*  LW_CFG_CPU_ARCH_ARM         */

#endif                                                                  /*  __ARCH_GDB_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
