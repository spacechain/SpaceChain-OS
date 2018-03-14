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
;** 文   件   名: ppcVfpAsm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2015 年 12 月 21 日
;**
;** 描        述: PowerPC 体系架构 VPU 支持.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>

#if LW_CFG_CPU_FPU_EN > 0
#include "arch/ppc/arch_float.h"

    FILE_BEGIN()

    EXPORT_LABEL(ppcVfpEnable)
    EXPORT_LABEL(ppcVfpDisable)
    EXPORT_LABEL(ppcVfpIsEnable)
    EXPORT_LABEL(ppcVfpSave)
    EXPORT_LABEL(ppcVfpRestore)

;/*********************************************************************************************************
;  FPU 在 MSR 中的使能掩码
;*********************************************************************************************************/

#define ARCH_PPC_MSR_BIT_FP     18

#define FPU_MASK(src, des) \
    RLWINM  des, src, 0, ARCH_PPC_MSR_BIT_FP+1, ARCH_PPC_MSR_BIT_FP-1

;/*********************************************************************************************************
;  使能 FPU 宏
;*********************************************************************************************************/

MACRO_DEF(ENABLE_VFP, Rn)
    MFMSR   \Rn                                                         ;/*  读取 MSR                    */
    ISYNC
    ORI     \Rn , \Rn , ARCH_PPC_MSR_FP                                 ;/*  设置 FP 位                  */
    ISYNC
    MTMSR   \Rn                                                         ;/*  设置 MSR                    */
    ISYNC

#ifdef  ARCH_PPC_CCR0_BIT_DAPUIB_U
    ;/*
    ; * 确保复用处理器 (包括 FP) 被使能，通过清除 CCR0[DAPUIB] 位来完成
    ; */
    MFSPR   \Rn , CCR0
    ISYNC
    RLWINM  \Rn , \Rn , 0 , ARCH_PPC_CCR0_BIT_DAPUIB_U+1 , ARCH_PPC_CCR0_BIT_DAPUIB_U-1
    ISYNC
    MTSPR   CCR0 , \Rn
    ISYNC
#endif                                                                  /*  ARCH_PPC_CCR0_BIT_DAPUIB_U  */
    MACRO_END()

;/*********************************************************************************************************
;  使能 FPU
;*********************************************************************************************************/

FUNC_DEF(ppcVfpEnable)
    ENABLE_VFP  R3
    BLR
    FUNC_END()

;/*********************************************************************************************************
;  关闭 FPU
;*********************************************************************************************************/

FUNC_DEF(ppcVfpDisable)
    MFMSR   R3
    ISYNC
    FPU_MASK(R3 , R3)
    ISYNC
    MTMSR   R3
    ISYNC
    BLR
    FUNC_END()

;/*********************************************************************************************************
;  判断 FPU 是否使能
;*********************************************************************************************************/

FUNC_DEF(ppcVfpIsEnable)
    MFMSR   R3
    ISYNC
    RLWINM  R3 , R3 , 0 , ARCH_PPC_MSR_BIT_FP , ARCH_PPC_MSR_BIT_FP
    CMPWI   CR0, R3 , 0
    BNE     CR0, __ppcVfpIsEnable
    BLR
LINE_LABEL(__ppcVfpIsEnable)
    LI      R3 , 1
    BLR
    FUNC_END()

;/*********************************************************************************************************
;  保存寄存器
;*********************************************************************************************************/

FUNC_DEF(ppcVfpSave)
    ENABLE_VFP  R4

    STFD    FPR0  ,  XFPR(0)(R3)                                        ;/*  保存 FPR0                   */
    STFD    FPR1  ,  XFPR(1)(R3)                                        ;/*  保存 FPR1                   */
    STFD    FPR2  ,  XFPR(2)(R3)                                        ;/*  保存 FPR2                   */
    STFD    FPR3  ,  XFPR(3)(R3)                                        ;/*  保存 FPR3                   */
    STFD    FPR4  ,  XFPR(4)(R3)                                        ;/*  保存 FPR4                   */
    STFD    FPR5  ,  XFPR(5)(R3)                                        ;/*  保存 FPR5                   */
    STFD    FPR6  ,  XFPR(6)(R3)                                        ;/*  保存 FPR6                   */
    STFD    FPR7  ,  XFPR(7)(R3)                                        ;/*  保存 FPR7                   */
    STFD    FPR8  ,  XFPR(8)(R3)                                        ;/*  保存 FPR8                   */
    STFD    FPR9  ,  XFPR(9)(R3)                                        ;/*  保存 FPR9                   */
    STFD    FPR10 , XFPR(10)(R3)                                        ;/*  保存 FPR10                  */
    STFD    FPR11 , XFPR(11)(R3)                                        ;/*  保存 FPR11                  */
    STFD    FPR12 , XFPR(12)(R3)                                        ;/*  保存 FPR12                  */
    STFD    FPR13 , XFPR(13)(R3)                                        ;/*  保存 FPR13                  */
    STFD    FPR14 , XFPR(14)(R3)                                        ;/*  保存 FPR14                  */
    STFD    FPR15 , XFPR(15)(R3)                                        ;/*  保存 FPR15                  */
    STFD    FPR16 , XFPR(16)(R3)                                        ;/*  保存 FPR16                  */
    STFD    FPR17 , XFPR(17)(R3)                                        ;/*  保存 FPR17                  */
    STFD    FPR18 , XFPR(18)(R3)                                        ;/*  保存 FPR18                  */
    STFD    FPR19 , XFPR(19)(R3)                                        ;/*  保存 FPR19                  */
    STFD    FPR20 , XFPR(20)(R3)                                        ;/*  保存 FPR20                  */
    STFD    FPR21 , XFPR(21)(R3)                                        ;/*  保存 FPR21                  */
    STFD    FPR22 , XFPR(22)(R3)                                        ;/*  保存 FPR22                  */
    STFD    FPR23 , XFPR(23)(R3)                                        ;/*  保存 FPR23                  */
    STFD    FPR24 , XFPR(24)(R3)                                        ;/*  保存 FPR24                  */
    STFD    FPR25 , XFPR(25)(R3)                                        ;/*  保存 FPR25                  */
    STFD    FPR26 , XFPR(26)(R3)                                        ;/*  保存 FPR26                  */
    STFD    FPR27 , XFPR(27)(R3)                                        ;/*  保存 FPR27                  */
    STFD    FPR28 , XFPR(28)(R3)                                        ;/*  保存 FPR28                  */
    STFD    FPR29 , XFPR(29)(R3)                                        ;/*  保存 FPR29                  */
    STFD    FPR30 , XFPR(30)(R3)                                        ;/*  保存 FPR30                  */
    STFD    FPR31 , XFPR(31)(R3)                                        ;/*  保存 FPR31                  */

    SYNC

    MFFS    FPR28                                                       ;/*  使用 FPR28 去保存 FPSCR     */
    ISYNC
    STFD    FPR28 , XFPSCR(R3)                                          ;/*  用 double 保存 FPSCR        */
    LWZ     R4    , XFPSCR_COPY(R3)                                     ;/*  获得真实的 FPSCR            */
    STW     R4    , XFPSCR(R3)                                          ;/*  保存到属于它的位置          */

    LFD     FPR28 , XFPR(28)(R3)                                        ;/*  恢复 FPR28                  */
    SYNC

    BLR
    FUNC_END()

;/*********************************************************************************************************
;  恢复寄存器
;*********************************************************************************************************/

FUNC_DEF(ppcVfpRestore)
    ENABLE_VFP  R4

    LWZ     R4    , XFPSCR(R3)                                          ;/*  读取已经保存的 FPSCR        */
    STW     R4    , XFPSCR_COPY(R3)                                     ;/*  拷贝它到 FpscrCopy          */
    LFD     FPR1  , XFPSCR(R3)                                          ;/*  用 double 加载 FPSCR        */
    SYNC
    MTFSF   255   , FPR1                                                ;/*  恢复 FPSCR                  */
    ISYNC

    LFD     FPR0  ,  XFPR(0)(R3)                                        ;/*  恢复 FPR0                   */
    LFD     FPR1  ,  XFPR(1)(R3)                                        ;/*  恢复 FPR1                   */
    LFD     FPR2  ,  XFPR(2)(R3)                                        ;/*  恢复 FPR2                   */
    LFD     FPR3  ,  XFPR(3)(R3)                                        ;/*  恢复 FPR3                   */
    LFD     FPR4  ,  XFPR(4)(R3)                                        ;/*  恢复 FPR4                   */
    LFD     FPR5  ,  XFPR(5)(R3)                                        ;/*  恢复 FPR5                   */
    LFD     FPR6  ,  XFPR(6)(R3)                                        ;/*  恢复 FPR6                   */
    LFD     FPR7  ,  XFPR(7)(R3)                                        ;/*  恢复 FPR7                   */
    LFD     FPR8  ,  XFPR(8)(R3)                                        ;/*  恢复 FPR8                   */
    LFD     FPR9  ,  XFPR(9)(R3)                                        ;/*  恢复 FPR9                   */
    LFD     FPR10 , XFPR(10)(R3)                                        ;/*  恢复 FPR10                  */
    LFD     FPR11 , XFPR(11)(R3)                                        ;/*  恢复 FPR11                  */
    LFD     FPR12 , XFPR(12)(R3)                                        ;/*  恢复 FPR12                  */
    LFD     FPR13 , XFPR(13)(R3)                                        ;/*  恢复 FPR13                  */
    LFD     FPR14 , XFPR(14)(R3)                                        ;/*  恢复 FPR14                  */
    LFD     FPR15 , XFPR(15)(R3)                                        ;/*  恢复 FPR15                  */
    LFD     FPR16 , XFPR(16)(R3)                                        ;/*  恢复 FPR16                  */
    LFD     FPR17 , XFPR(17)(R3)                                        ;/*  恢复 FPR17                  */
    LFD     FPR18 , XFPR(18)(R3)                                        ;/*  恢复 FPR18                  */
    LFD     FPR19 , XFPR(19)(R3)                                        ;/*  恢复 FPR19                  */
    LFD     FPR20 , XFPR(20)(R3)                                        ;/*  恢复 FPR20                  */
    LFD     FPR21 , XFPR(21)(R3)                                        ;/*  恢复 FPR21                  */
    LFD     FPR22 , XFPR(22)(R3)                                        ;/*  恢复 FPR22                  */
    LFD     FPR23 , XFPR(23)(R3)                                        ;/*  恢复 FPR23                  */
    LFD     FPR24 , XFPR(24)(R3)                                        ;/*  恢复 FPR24                  */
    LFD     FPR25 , XFPR(25)(R3)                                        ;/*  恢复 FPR25                  */
    LFD     FPR26 , XFPR(26)(R3)                                        ;/*  恢复 FPR26                  */
    LFD     FPR27 , XFPR(27)(R3)                                        ;/*  恢复 FPR27                  */
    LFD     FPR28 , XFPR(28)(R3)                                        ;/*  恢复 FPR28                  */
    LFD     FPR29 , XFPR(29)(R3)                                        ;/*  恢复 FPR29                  */
    LFD     FPR30 , XFPR(30)(R3)                                        ;/*  恢复 FPR30                  */
    LFD     FPR31 , XFPR(31)(R3)                                        ;/*  恢复 FPR31                  */
    SYNC

    BLR
    FUNC_END()

    FILE_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
