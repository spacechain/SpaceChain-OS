;/*********************************************************************************************************
;**
;**                                    中国软件开源组织
;**
;**                                   嵌入式实时操作系统
;**
;**                                       SylixOS(TM)
;**
;**                               Copyright  All Rights Reserved
;**
;**--------------文件信息--------------------------------------------------------------------------------
;**
;** 文   件   名: armVfp11Asm.S
;**
;** 创   建   人: Han.Hui (韩辉)
;**
;** 文件创建日期: 2013 年 12 月 07 日
;**
;** 描        述: ARM 体系架构 VFP11 (VFPv2 for ARM11) 支持.
;**
;** BUG:
;2014.12.29  由于 armVfp11HwInit 需要操作 NSACR 寄存器, 有些处理器需要做特殊处理, 所以这里改为弱符号.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>

#if !defined(__SYLIXOS_ARM_ARCH_M__)

#if LW_CFG_CPU_FPU_EN > 0

    FILE_BEGIN()
    
    EXPORT_LABEL(armVfp11Mvfr0)
    EXPORT_LABEL(armVfp11NonSecEn)
    EXPORT_LABEL(armVfp11HwInit)
    
    WEAK(armVfp11HwInit)

;/*********************************************************************************************************
;  浮点运算器基本操作函数
;*********************************************************************************************************/

FUNC_DEF(armVfp11Mvfr0)
    FMRX    R0, MVFR0
    BX      LR
    FUNC_END()

FUNC_DEF(armVfp11NonSecEn)
#if LW_CFG_ARM_CP15 > 0
                                        ;/*  NSACR: Non-secure Access Control Register                   */
    MRC     p15, 0, R0, c1, c1, 2       ;/*  R0 = NSACR                                                  */
    ORR     R0, R0, #(0x3 << 10)        ;/*  Enable full access for cp10,cp11                            */
    MCR     p15, 0, R0, c1, c1, 2       ;/*  NSACR = R0                                                  */
    ARM_DSB()
    ARM_ISB()
#endif
    BX      LR
    FUNC_END()

FUNC_DEF(armVfp11HwInit)
#if LW_CFG_ARM_CP15 > 0
                                        ;/*  CPACR: Coprocessor Access Control Register                  */
    MRC     p15, 0, R0, c1, c0, 2       ;/*  R0 = CPACR                                                  */
    ORR     R0, R0, #(0xf << 20)        ;/*  Enable full access for cp10,cp11                            */
    MCR     p15, 0, R0, c1, c0, 2       ;/*  CPACR = R0                                                  */

#if __SYLIXOS_ARM_ARCH__ < 8
    MOV     R0, #0
    MCR     p15, 0, R0, c7, c5, 4       ;/*  Flush prefetch buffer                                       */
#endif
    ARM_DSB()
    ARM_ISB()
#endif
    BX      LR
    FUNC_END()
    
    FILE_END()

#endif
#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
