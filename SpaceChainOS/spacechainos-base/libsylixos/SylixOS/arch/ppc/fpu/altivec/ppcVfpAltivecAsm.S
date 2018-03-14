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
;** 文   件   名: ppcVfpAltivecAsm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2016 年 05 月 04 日
;**
;** 描        述: PowerPC 体系架构 ALTIVEC 支持.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>

#if LW_CFG_CPU_FPU_EN > 0
#include "arch/ppc/arch_float.h"
#define  __SYLIXOS_PPC_HAVE_ALTIVEC 1
#include "arch/ppc/arch_604.h"

    FILE_BEGIN()

    EXPORT_LABEL(ppcVfpAltivecEnable)
    EXPORT_LABEL(ppcVfpAltivecDisable)
    EXPORT_LABEL(ppcVfpAltivecIsEnable)
    EXPORT_LABEL(ppcVfpAltivecSave)
    EXPORT_LABEL(ppcVfpAltivecRestore)

;/*********************************************************************************************************
;  VRSAVE 寄存器
;*********************************************************************************************************/

#define VRSAVE_REG                  256

;/*********************************************************************************************************
;  ALTIVEC 在 MSR 中的使能掩码
;*********************************************************************************************************/

#define ALTIVEC_MASK(src, des) \
    RLWINM  des, src, 0, ARCH_PPC_MSR_BIT_VEC + 1, ARCH_PPC_MSR_BIT_VEC - 1

;/*********************************************************************************************************
;  使能 FPU 宏
;*********************************************************************************************************/

MACRO_DEF(ENABLE_ALTIVEC, Rn)
    MFMSR   \Rn
    ORIS    \Rn , \Rn , HI(ARCH_PPC_MSR_VEC)
    SYNC
    MTMSR   \Rn
    ISYNC
    SYNC
    MACRO_END()

;/*********************************************************************************************************
;  使能 FPU
;*********************************************************************************************************/

FUNC_DEF(ppcVfpAltivecEnable)
    ENABLE_ALTIVEC  R3
    BLR
    FUNC_END()

;/*********************************************************************************************************
;  关闭 FPU
;*********************************************************************************************************/

FUNC_DEF(ppcVfpAltivecDisable)
    MFMSR   R3
    ISYNC
    ALTIVEC_MASK(R3 , R3)
    ISYNC
    MTMSR   R3
    ISYNC
    BLR
    FUNC_END()

;/*********************************************************************************************************
;  判断 FPU 是否使能
;*********************************************************************************************************/

FUNC_DEF(ppcVfpAltivecIsEnable)
    MFMSR   R3
    ISYNC
    RLWINM  R3 , R3 , 0 , ARCH_PPC_MSR_BIT_VEC, ARCH_PPC_MSR_BIT_VEC
    CMPWI   CR0, R3 , 0
    BNE     CR0, __ppcVfpAltivecIsEnable
    BLR
LINE_LABEL(__ppcVfpAltivecIsEnable)
    LI      R3 , 1
    BLR
    FUNC_END()

;/*********************************************************************************************************
;  保存寄存器
;*********************************************************************************************************/

FUNC_DEF(ppcVfpAltivecSave)
    ENABLE_ALTIVEC  R4

    MFSPR   R5  , VRSAVE_REG
    STW     R5  , ALTIVEC_VRSAVE_OFFSET(R3)                     ;/*  Save VRSAVE register                */
    SYNC

    STVX    V0  , 0  , R3
    ADDI    R6  , R3 , ALTIVEC_OFFSET(1)

    ADDI    R7  , R3 , ALTIVEC_VSCR_OFFSET
    SYNC
    MFVSCR  V0
    SYNC
    STVX    V0  , 0 , R7

    LVX     V0  , 0 , R3                                        ;/*  Restore V0                          */

    STVX    V1  , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(2)
    STVX    V2  , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(3)
    STVX    V3  , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(4)
    STVX    V4  , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(5)
    STVX    V5  , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(6)
    STVX    V6  , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(7)
    STVX    V7  , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(8)
    STVX    V8  , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(9)
    STVX    V9  , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(10)
    STVX    V10 , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(11)
    STVX    V11 , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(12)
    STVX    V12 , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(13)
    STVX    V13 , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(14)
    STVX    V14 , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(15)
    STVX    V15 , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(16)
    STVX    V16 , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(17)
    STVX    V17 , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(18)
    STVX    V18 , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(19)
    STVX    V19 , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(20)
    STVX    V20 , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(21)
    STVX    V21 , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(22)
    STVX    V22 , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(23)
    STVX    V23 , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(24)
    STVX    V24 , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(25)
    STVX    V25 , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(26)
    STVX    V26 , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(27)
    STVX    V27 , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(28)
    STVX    V28 , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(29)
    STVX    V29 , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(30)
    STVX    V30 , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(31)
    STVX    V31 , 0  , R6

    ISYNC
    SYNC

    BLR
    FUNC_END()

;/*********************************************************************************************************
;  恢复寄存器
;*********************************************************************************************************/

FUNC_DEF(ppcVfpAltivecRestore)
    ENABLE_ALTIVEC  R4

    LWZ     R5 , ALTIVEC_VRSAVE_OFFSET(R3)                      ;/*  Restore VRSAVE register             */
    MTSPR   VRSAVE_REG , R5
    SYNC

    ADDI    R7  , R3 , ALTIVEC_VSCR_OFFSET
    LVX     V0  , 0  , R7
    MTVSCR  V0                                                  ;/*  Restore VSCR register               */
    SYNC

    LVX     V0  , 0  , R3
    ADDI    R6  , R3 , ALTIVEC_OFFSET(1)
    LVX     V1  , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(2)
    LVX     V2  , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(3)
    LVX     V3  , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(4)
    LVX     V4  , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(5)
    LVX     V5  , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(6)
    LVX     V6  , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(7)
    LVX     V7  , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(8)
    LVX     V8  , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(9)
    LVX     V9  , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(10)
    LVX     V10 , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(11)
    LVX     V11 , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(12)
    LVX     V12 , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(13)
    LVX     V13 , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(14)
    LVX     V14 , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(15)
    LVX     V15 , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(16)
    LVX     V16 , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(17)
    LVX     V17 , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(18)
    LVX     V18 , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(19)
    LVX     V19 , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(20)
    LVX     V20 , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(21)
    LVX     V21 , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(22)
    LVX     V22 , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(23)
    LVX     V23 , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(24)
    LVX     V24 , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(25)
    LVX     V25 , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(26)
    LVX     V26 , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(27)
    LVX     V27 , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(28)
    LVX     V28 , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(29)
    LVX     V29 , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(30)
    LVX     V30 , 0  , R6
    ADDI    R6  , R3 , ALTIVEC_OFFSET(31)
    LVX     V31 , 0  , R6

    ISYNC
    SYNC

    BLR
    FUNC_END()

    FILE_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
