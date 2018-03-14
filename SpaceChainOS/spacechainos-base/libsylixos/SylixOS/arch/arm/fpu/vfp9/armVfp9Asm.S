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
;** 文   件   名: armVfp9Asm.S
;**
;** 创   建   人: Han.Hui (韩辉)
;**
;** 文件创建日期: 2013 年 12 月 07 日
;**
;** 描        述: ARM 体系架构 VFP9 (VFPv2 for ARM9) 支持.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>

#if !defined(__SYLIXOS_ARM_ARCH_M__)

#if LW_CFG_CPU_FPU_EN > 0

    FILE_BEGIN()
    
    EXPORT_LABEL(armVfp9Sid)
    EXPORT_LABEL(armVfp9Enable)
    EXPORT_LABEL(armVfp9Disable)
    EXPORT_LABEL(armVfp9IsEnable)
    EXPORT_LABEL(armVfp9Save16)
    EXPORT_LABEL(armVfp9Restore16)
    EXPORT_LABEL(armVfp9Save32)
    EXPORT_LABEL(armVfp9Restore32)
    
;/*********************************************************************************************************
;  浮点运算器基本操作函数
;*********************************************************************************************************/
    
FUNC_DEF(armVfp9Sid)
    FMRX    R0, FPSID
    BX      LR
    FUNC_END()
    
FUNC_DEF(armVfp9Enable)
    MOV     R1, #0x40000000      ;/*  Set Neon/VFP Enable bit                                            */
    FMXR    FPEXC, R1            ;/*  FPEXC, clear others.                                               */
    BX      LR
    FUNC_END()
    
FUNC_DEF(armVfp9Disable)
    MOV     R1, #0x00000000      ;/*  Clear Neon/VFP Enable bit                                          */
    FMXR    FPEXC, R1            ;/*  FPEXC, clear others.                                               */
    BX      LR
    FUNC_END()
    
FUNC_DEF(armVfp9IsEnable)
    FMRX    R1, FPEXC
    MOV     R0, R1, LSR #30      ;/*  Read Neon/VFP Enable bit                                           */
    AND     R0, R0, #1
    BX      LR
    FUNC_END()
    
;/*********************************************************************************************************
;  VFP-D16
;   +-----------+
;   | freg[31]  |    + 0x98  <-- (r0 + 152)
;   |  ...      |
;   | freg[2]   |    + 0x24
;   | freg[1]   |    + 0x20
;   | freg[0]   |    + 0x1C  <-- (r0 + 28)
;   | mfvfr1    |    + 0x18
;   | mfvfr0    |    + 0x14
;   | fpinst2   |    + 0x10
;   | fpinst    |    + 0x0C
;   | fpexc     |    + 0x08
;   | fpscr     |    + 0x04
;   | fpsid     | <-- cpu_fpu_context ( = r0 )
;   +-----------+
;*********************************************************************************************************/

#define VFP_FPSID       0x00
#define VFP_FPSCR       0x04
#define VFP_FPEXC       0x08
#define VFP_FPINST      0x0C
#define VFP_FPINST2     0x10
#define VFP_MVVFR0      0x14
#define VFP_MVVFR1      0x18
#define VFP_GPR         0x1C

;/*********************************************************************************************************
;  VFP9 保存控制寄存器宏
;*********************************************************************************************************/

MACRO_DEF(VFP9_SAVE_CTRL)
    FMRX    R1, FPSCR
    STR     R1, [R0, #VFP_FPSCR]

    FMRX    R1, FPEXC
    STR     R1, [R0, #VFP_FPEXC]

    FMRX    R2, FPINST
    STR     R2, [R0, #VFP_FPINST]

    TST     R1, #(1 << 28)          ;/* The FPINST2 register does not exist on VFP10rev0, so we test FP2V*/
#if defined(__SYLIXOS_ARM_ARCH_M__)
    ITT     NE
#endif
    FMRXNE  R3, FPINST2             ;/* FP2V: FPINST2 is valid                                           */
    STRNE   R3, [R0, #VFP_FPINST2]

    ADD     R1, R0, #VFP_GPR
    MACRO_END()

;/*********************************************************************************************************
;  VFP9 恢复控制寄存器宏
;*********************************************************************************************************/

MACRO_DEF(VFP9_RESTORE_CTRL)
    LDR     R1, [R0, #VFP_FPEXC]
    FMXR    FPEXC, R1

    LDR     R2, [R0, #VFP_FPINST]
    FMXR    FPINST, R2

    TST     R1, #(1 << 28)          ;/* The FPINST2 register does not exist on VFP10rev0, so we test FP2V*/
#if defined(__SYLIXOS_ARM_ARCH_M__)
    ITT     NE
#endif
    LDRNE   R3, [R0, #VFP_FPINST2]
    FMXRNE  FPINST2, R3             ;/* FP2V: FPINST2 is valid                                           */

    LDR     R1, [R0, #VFP_FPSCR]
    FMXR    FPSCR, R1

    ADD     R1, R0, #VFP_GPR
    MACRO_END()

;/*********************************************************************************************************
;  VFP9 保存/恢复寄存器
;*********************************************************************************************************/

FUNC_DEF(armVfp9Save16)
    VFP9_SAVE_CTRL
    FSTMIAX R1, {d0-d15}
    BX      LR
    FUNC_END()

FUNC_DEF(armVfp9Restore16)
    VFP9_RESTORE_CTRL
    FLDMIAX R1, {d0-d15}
    BX      LR
    FUNC_END()
    
;/*********************************************************************************************************
;  VFP-D32
;   +-----------+
;   | freg[63]  |    + 0x118 <-- (r0 + 280)
;   |  ...      |
;   | freg[31]  |    + 0x98  <-- (r0 + 152)
;   |  ...      |
;   | freg[2]   |    + 0x24
;   | freg[1]   |    + 0x20
;   | freg[0]   |    + 0x1C  <-- (r0 + 28)
;   | mfvfr1    |    + 0x18
;   | mfvfr0    |    + 0x14
;   | fpinst2   |    + 0x10
;   | fpinst    |    + 0x0C
;   | fpexc     |    + 0x08
;   | fpscr     |    + 0x04
;   | fpsid     | <-- cpu_fpu_context ( = r0 )
;   +-----------+
;*********************************************************************************************************/

FUNC_DEF(armVfp9Save32)
    VFP9_SAVE_CTRL
    FSTMIAX R1!, {d0-d15}
    FSTMIAX R1 , {d16-d31}
    BX      LR
    FUNC_END()
    
FUNC_DEF(armVfp9Restore32)
    VFP9_RESTORE_CTRL
    FLDMIAX R1!, {d0-d15}
    FLDMIAX R1 , {d16-d31}
    BX      LR
    FUNC_END()
    
    FILE_END()

#endif
#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
