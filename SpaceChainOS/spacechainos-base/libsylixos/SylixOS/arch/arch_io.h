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
** 文   件   名: arch_io.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 12 月 09 日
**
** 描        述: SylixOS 体系构架 I/O 访问接口.
*********************************************************************************************************/

#ifndef __ARCH_IO_H
#define __ARCH_IO_H

#include "config/cpu/cpu_cfg.h"

#if defined(LW_CFG_CPU_ARCH_ARM)
#include "./arm/arm_io.h"

#elif defined(LW_CFG_CPU_ARCH_X86)
#include "./x86/x86_io.h"

#elif defined(LW_CFG_CPU_ARCH_MIPS)
#include "./mips/mips_io.h"

#elif defined(LW_CFG_CPU_ARCH_PPC)
#include "./ppc/ppc_io.h"

#elif defined(LW_CFG_CPU_ARCH_C6X)
#include "./c6x/c6x_io.h"

#elif defined(LW_CFG_CPU_ARCH_SPARC)
#include "./sparc/sparc_io.h"
#endif                                                                  /*  LW_CFG_CPU_ARCH_ARM         */

#endif                                                                  /*  __ARCH_IO_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
