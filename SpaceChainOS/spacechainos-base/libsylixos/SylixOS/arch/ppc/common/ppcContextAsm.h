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
** 文   件   名: ppcContextAsm.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2015 年 12 月 17 日
**
** 描        述: PowerPC 体系构架上下文切换.
*********************************************************************************************************/

#ifndef __ARCH_PPCCONTEXTASM_H
#define __ARCH_PPCCONTEXTASM_H

#include <config/kernel/vmm_cfg.h>
#include "arch/ppc/arch_regs.h"

/*********************************************************************************************************
  上下文恢复
*********************************************************************************************************/

MACRO_DEF(RESTORE_REGS)
    LWZ     R2  ,  XR2(SP)                                              /*  恢复 R2 - R31               */
    LWZ     R3  ,  XR3(SP)
    LWZ     R4  ,  XR4(SP)
    LWZ     R5  ,  XR5(SP)
    LWZ     R6  ,  XR6(SP)
    LWZ     R7  ,  XR7(SP)
    LWZ     R8  ,  XR8(SP)
    LWZ     R9  ,  XR9(SP)
    LWZ     R10 , XR10(SP)
    LWZ     R11 , XR11(SP)
    LWZ     R12 , XR12(SP)
    LWZ     R13 , XR13(SP)
    LWZ     R14 , XR14(SP)
    LWZ     R15 , XR15(SP)
    LWZ     R16 , XR16(SP)
    LWZ     R17 , XR17(SP)
    LWZ     R18 , XR18(SP)
    LWZ     R19 , XR19(SP)
    LWZ     R20 , XR20(SP)
    LWZ     R21 , XR21(SP)
    LWZ     R22 , XR22(SP)
    LWZ     R23 , XR23(SP)
    LWZ     R24 , XR24(SP)
    LWZ     R25 , XR25(SP)
    LWZ     R26 , XR26(SP)
    LWZ     R27 , XR27(SP)
    LWZ     R28 , XR28(SP)
    LWZ     R29 , XR29(SP)
    LWZ     R30 , XR30(SP)
    LWZ     R31 , XR31(SP)

    LWZ     R0  , XCR(SP)                                               /*  恢复 CR                     */
    SYNC
    MTCR    R0
    ISYNC

    LWZ     R0  , XXER(SP)                                              /*  恢复 XER                    */
    SYNC
    MTXER   R0
    ISYNC

    LWZ     R0  , XCTR(SP)                                              /*  恢复 CTR                    */
    SYNC
    MTCTR   R0
    ISYNC

    LWZ     R0  , XLR(SP)                                               /*  恢复 LR                     */
    SYNC
    MTLR    R0
    ISYNC

    LWZ     R0  , XSRR1(SP)                                             /*  恢复 SRR1                   */
    SYNC
    MTSPR   SRR1, R0
    ISYNC

    LWZ     R0  , XSRR0(SP)                                             /*  恢复 SRR0                   */
    SYNC
    MTSPR   SRR0, R0
    ISYNC

    LWZ     R0  , XR0(SP)                                               /*  恢复 R0                     */
    SYNC

    ADDI    SP  , SP , STACK_FRAME_SIZE                                 /*  调整 SP                     */

    RFI                                                                 /*  从 SRR0 返回，同时 MSR=SRR1 */
    MACRO_END()

/*********************************************************************************************************
  上下文保存
*********************************************************************************************************/

MACRO_DEF(SAVE_REGS)
    STWU    SP  , -STACK_FRAME_SIZE(SP)                                 /*  保存并调整 SP               */

    STW     R0  ,  XR0(SP)                                              /*  保存 R0, R2 - R31           */
    STW     R2  ,  XR2(SP)
    STW     R3  ,  XR3(SP)
    STW     R4  ,  XR4(SP)
    STW     R5  ,  XR5(SP)
    STW     R6  ,  XR6(SP)
    STW     R7  ,  XR7(SP)
    STW     R8  ,  XR8(SP)
    STW     R9  ,  XR9(SP)
    STW     R10 , XR10(SP)
    STW     R11 , XR11(SP)
    STW     R12 , XR12(SP)
    STW     R13 , XR13(SP)
    STW     R14 , XR14(SP)
    STW     R15 , XR15(SP)
    STW     R16 , XR16(SP)
    STW     R17 , XR17(SP)
    STW     R18 , XR18(SP)
    STW     R19 , XR19(SP)
    STW     R20 , XR20(SP)
    STW     R21 , XR21(SP)
    STW     R22 , XR22(SP)
    STW     R23 , XR23(SP)
    STW     R24 , XR24(SP)
    STW     R25 , XR25(SP)
    STW     R26 , XR26(SP)
    STW     R27 , XR27(SP)
    STW     R28 , XR28(SP)
    STW     R29 , XR29(SP)
    STW     R30 , XR30(SP)
    STW     R31 , XR31(SP)

    MFMSR   R0
    ISYNC
    STW     R0  , XMSR(SP)                                              /*  保存 MSR                    */
    SYNC

    MFLR    R0                                                          /*  LR 代替 SRR0 被保存         */
    ISYNC
    STW     R0  , XSRR0(SP)
    SYNC

    MFMSR   R0                                                          /*  MSR 代替 SRR1 被保存        */
    ISYNC
    STW     R0  , XSRR1(SP)
    SYNC

    MFLR    R0
    ISYNC
    STW     R0  , XLR(SP)                                               /*  保存 LR                     */
    SYNC

    MFCTR   R0
    ISYNC
    STW     R0  , XCTR(SP)                                              /*  保存 CTR                    */
    SYNC

    MFXER   R0
    ISYNC
    STW     R0  , XXER(SP)                                              /*  保存 XER                    */
    SYNC

    MFCR    R0
    ISYNC
    STW     R0  , XCR(SP)                                               /*  保存 CR                     */
    SYNC
    MACRO_END()

/*********************************************************************************************************
  使能 MMU
*********************************************************************************************************/

#if LW_CFG_VMM_EN > 0
MACRO_DEF(ENABLE_MMU)
    MTSPR   SPRG0 , R3                                                  /*  SPRG0 暂存 R3               */
    ISYNC

    MFMSR   R3
    ISYNC
    ORI     R3 , R3 , ARCH_PPC_MSR_DR | ARCH_PPC_MSR_IR                 /*  使能 DR 与 IR 位            */
    ISYNC
    MTMSR   R3
    ISYNC

    MFSPR   R3 , SPRG0                                                  /*  恢复 R3                     */
    ISYNC
    MACRO_END()
#else
MACRO_DEF(ENABLE_MMU)
    MACRO_END()
#endif

/*********************************************************************************************************
  异常上下文保存
*********************************************************************************************************/

MACRO_DEF(EXC_SAVE_REGS)
    ENABLE_MMU                                                          /*  使能 MMU                    */

    STWU    SP  , -STACK_FRAME_SIZE(SP)                                 /*  保存并调整 SP               */

    STW     R0  ,  XR0(SP)                                              /*  保存 R0, R2 - R31           */
    STW     R2  ,  XR2(SP)
    STW     R3  ,  XR3(SP)
    STW     R4  ,  XR4(SP)
    STW     R5  ,  XR5(SP)
    STW     R6  ,  XR6(SP)
    STW     R7  ,  XR7(SP)
    STW     R8  ,  XR8(SP)
    STW     R9  ,  XR9(SP)
    STW     R10 , XR10(SP)
    STW     R11 , XR11(SP)
    STW     R12 , XR12(SP)
    STW     R13 , XR13(SP)
    STW     R14 , XR14(SP)
    STW     R15 , XR15(SP)
    STW     R16 , XR16(SP)
    STW     R17 , XR17(SP)
    STW     R18 , XR18(SP)
    STW     R19 , XR19(SP)
    STW     R20 , XR20(SP)
    STW     R21 , XR21(SP)
    STW     R22 , XR22(SP)
    STW     R23 , XR23(SP)
    STW     R24 , XR24(SP)
    STW     R25 , XR25(SP)
    STW     R26 , XR26(SP)
    STW     R27 , XR27(SP)
    STW     R28 , XR28(SP)
    STW     R29 , XR29(SP)
    STW     R30 , XR30(SP)
    STW     R31 , XR31(SP)

    MFMSR   R0
    ISYNC
    STW     R0  , XMSR(SP)                                              /*  保存 MSR                    */
    SYNC

    MFSPR   R0  , SRR0
    ISYNC
    STW     R0  , XSRR0(SP)                                             /*  保存 SRR0                   */
    SYNC

    MFSPR   R0  , SRR1
    ISYNC
    STW     R0  , XSRR1(SP)                                             /*  保存 SRR1                   */
    SYNC

    MFLR    R0
    ISYNC
    STW     R0  , XLR(SP)                                               /*  保存 LR                     */
    SYNC

    MFCTR   R0
    ISYNC
    STW     R0  , XCTR(SP)                                              /*  保存 CTR                    */
    SYNC

    MFXER   R0
    ISYNC
    STW     R0  , XXER(SP)                                              /*  保存 XER                    */
    SYNC

    MFCR    R0
    ISYNC
    STW     R0  , XCR(SP)                                               /*  保存 CR                     */
    SYNC
    MACRO_END()

#endif                                                                  /*  __ARCH_PPCCONTEXTASM_H      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
