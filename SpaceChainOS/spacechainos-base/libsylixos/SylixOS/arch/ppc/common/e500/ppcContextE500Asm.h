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
** 文   件   名: ppcContextE500Asm.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 05 月 12 日
**
** 描        述: PowerPC E500 体系构架上下文切换.
*********************************************************************************************************/

#ifndef __ARCH_PPCCONTEXTE500ASM_H
#define __ARCH_PPCCONTEXTE500ASM_H

#include "arch/ppc/arch_regs.h"

/*********************************************************************************************************
  E500 异常上下文保存
  E500 总是使能 MMU, 所以没有 ENABLE_MMU 操作
*********************************************************************************************************/

MACRO_DEF(E500_EXC_SAVE_REGS)
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

/*********************************************************************************************************
  E500 临界输入异常上下文保存
  E500 总是使能 MMU, 所以没有 ENABLE_MMU 操作
*********************************************************************************************************/

MACRO_DEF(E500_CI_EXC_SAVE_REGS)
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

    MFSPR   R0  , CSRR0
    ISYNC
    STW     R0  , XSRR0(SP)                                             /*  保存 CSRR0                  */
    SYNC

    MFSPR   R0  , CSRR1
    ISYNC
    STW     R0  , XSRR1(SP)                                             /*  保存 CSRR1                  */
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
  E500 机器检查异常上下文保存
  E500 总是使能 MMU, 所以没有 ENABLE_MMU 操作
*********************************************************************************************************/

MACRO_DEF(E500_MC_EXC_SAVE_REGS)
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

    MFSPR   R0  , MCSRR0
    ISYNC
    STW     R0  , XSRR0(SP)                                             /*  保存 MCSRR0                 */
    SYNC

    MFSPR   R0  , MCSRR1
    ISYNC
    STW     R0  , XSRR1(SP)                                             /*  保存 MCSRR1                 */
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

#endif                                                                  /* __ARCH_PPCCONTEXTE500ASM_H   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
