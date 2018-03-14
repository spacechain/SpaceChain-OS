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
;** 文   件   名: ppcVfpSpeAsm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2016 年 05 月 04 日
;**
;** 描        述: PowerPC 体系架构 SPE 支持.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>

#if LW_CFG_CPU_FPU_EN > 0
#include "arch/ppc/arch_float.h"
#include "arch/ppc/arch_e500.h"

    FILE_BEGIN()

    EXPORT_LABEL(ppcVfpSpeEnable)
    EXPORT_LABEL(ppcVfpSpeDisable)
    EXPORT_LABEL(ppcVfpSpeIsEnable)
    EXPORT_LABEL(ppcVfpSpeSave)
    EXPORT_LABEL(ppcVfpSpeRestore)

;/*********************************************************************************************************
;  SPE 在 MSR 中的使能掩码
;*********************************************************************************************************/

#define ARCH_PPC_MSR_BIT_SPE    6

#define SPE_MASK(src, des) \
    RLWINM  des, src, 0, ARCH_PPC_MSR_BIT_SPE+1, ARCH_PPC_MSR_BIT_SPE-1

;/*********************************************************************************************************
;  使能 FPU 宏
;*********************************************************************************************************/

MACRO_DEF(ENABLE_SPE, Rn)
    MFMSR           \Rn
    ORIS            \Rn , \Rn , HI(ARCH_PPC_MSR_SPE)
    SYNC
    MTMSR           \Rn
    SYNC
    MACRO_END()

;/*********************************************************************************************************
;  使能 FPU
;*********************************************************************************************************/

FUNC_DEF(ppcVfpSpeEnable)
    ENABLE_SPE      R3
    BLR
    FUNC_END()

;/*********************************************************************************************************
;  关闭 FPU
;*********************************************************************************************************/

FUNC_DEF(ppcVfpSpeDisable)
    MFMSR           R3
    ISYNC
    SPE_MASK(R3 , R3)
    ISYNC
    MTMSR           R3
    ISYNC
    BLR
    FUNC_END()

;/*********************************************************************************************************
;  判断 FPU 是否使能
;*********************************************************************************************************/

FUNC_DEF(ppcVfpSpeIsEnable)
    MFMSR           R3
    ISYNC
    RLWINM          R3 , R3 , 0 , ARCH_PPC_MSR_BIT_SPE , ARCH_PPC_MSR_BIT_SPE
    CMPWI           CR0, R3 , 0
    BNE             CR0, __ppcVfpSpeIsEnable
    BLR
LINE_LABEL(__ppcVfpSpeIsEnable)
    LI              R3 , 1
    BLR
    FUNC_END()

;/*********************************************************************************************************
;  保存寄存器
;*********************************************************************************************************/

FUNC_DEF(ppcVfpSpeSave)
    ENABLE_SPE      R4

    EVSTWWE         R0  , SPE_OFFSET(0)(R3)
    EVSTWWE         R1  , SPE_OFFSET(1)(R3)
    EVSTWWE         R2  , SPE_OFFSET(2)(R3)
    EVSTWWE         R3  , SPE_OFFSET(3)(R3)
    EVSTWWE         R4  , SPE_OFFSET(4)(R3)
    EVSTWWE         R5  , SPE_OFFSET(5)(R3)
    EVSTWWE         R6  , SPE_OFFSET(6)(R3)
    EVSTWWE         R7  , SPE_OFFSET(7)(R3)
    EVSTWWE         R8  , SPE_OFFSET(8)(R3)
    EVSTWWE         R9  , SPE_OFFSET(9)(R3)
    EVSTWWE         R10 , SPE_OFFSET(10)(R3)
    EVSTWWE         R11 , SPE_OFFSET(11)(R3)
    EVSTWWE         R12 , SPE_OFFSET(12)(R3)
    EVSTWWE         R13 , SPE_OFFSET(13)(R3)
    EVSTWWE         R14 , SPE_OFFSET(14)(R3)
    EVSTWWE         R15 , SPE_OFFSET(15)(R3)
    EVSTWWE         R16 , SPE_OFFSET(16)(R3)
    EVSTWWE         R17 , SPE_OFFSET(17)(R3)
    EVSTWWE         R18 , SPE_OFFSET(18)(R3)
    EVSTWWE         R19 , SPE_OFFSET(19)(R3)
    EVSTWWE         R20 , SPE_OFFSET(20)(R3)
    EVSTWWE         R21 , SPE_OFFSET(21)(R3)
    EVSTWWE         R22 , SPE_OFFSET(22)(R3)
    EVSTWWE         R23 , SPE_OFFSET(23)(R3)
    EVSTWWE         R24 , SPE_OFFSET(24)(R3)
    EVSTWWE         R25 , SPE_OFFSET(25)(R3)
    EVSTWWE         R26 , SPE_OFFSET(26)(R3)
    EVSTWWE         R27 , SPE_OFFSET(27)(R3)
    EVSTWWE         R28 , SPE_OFFSET(28)(R3)
    EVSTWWE         R29 , SPE_OFFSET(29)(R3)
    EVSTWWE         R30 , SPE_OFFSET(30)(R3)
    EVSTWWE         R31 , SPE_OFFSET(31)(R3)

    ;/* To avoid corruption of R5 this is done at the end */

    EVXOR           R5  , R5 , R5
    EVMWUMIAA       R5  , R5 , R5
    EVSTDD          R5  , SPE_OFFSET(32)(R3)    ;/*  Offset 32 words into structure for ACC              */

    MFSPR           R5  , SPEFSCR
    STW             R5  , SPE_OFFSET(34)(R3)    ;/*  Offset 34 words into structure for SPEFSCR          */

    SYNC
    BLR

    FUNC_END()

;/*********************************************************************************************************
;  恢复寄存器
;*********************************************************************************************************/

FUNC_DEF(ppcVfpSpeRestore)
    ENABLE_SPE      R4

    LWZ             R5 , SPE_OFFSET(1)(R3)      ;/*  preload data tlb into dummy R5                      */
    B               __ppcVfpSpeRestoreItlbLoad

LINE_LABEL(__ppcVfpSpeRestoreItlbLoadDone)

    LWZ             R5 , SPE_OFFSET(34)(R3)     ;/*  Offset 34 words into structure for SPEFSCR          */
    MTSPR           SPEFSCR , R5

    EVMERGELOHI     R1 , R1 , R1                ;/*  swap SP                                             */
    LWZ             R1 , SPE_OFFSET(1)(R3)      ;/*  load upper word into lower word SP                  */
    EVMERGELOHI     R1 , R1 , R1                ;/*  swap SP back                                        */

    EVLDD           R5 , SPE_OFFSET(32)(R3)     ;/*  Offset 32 words into structure for the accumulator  */
    EVMRA           R5 , R5                     ;/*  R5 corrupted so done before restore of R5           */
    MR              R5 , R3                     ;/*  Move ptr to spe register structure into R5          */
    EVMERGELOHI     R3 , R3 , R3                ;/*  Swap upper and lower words around                   */
    LWZ             R3 , SPE_OFFSET(3)(R5)      ;/*  Load lower word (was upper) to offset for R3        */
    EVMERGELOHI     R3 , R3 , R3                ;/*  Swap R3 back with upper bits now new context value. */
                                                ;/*  Lower bits restored                                 */
    EVMERGELOHI     R4 , R4 , R4                ;/*  Move lower word to upper word to retain lower word  */
    LWZ             R4 , SPE_OFFSET(4)(R3)      ;/*  Load upper word into lower word and swap            */
    EVMERGELOHI     R4 , R4 , R4                ;/*  move new context value into upper word and          */
                                                ;/*  restore lower                                       */

    ;/*
     ;* General approach is to swap upper and lower words
     ;* Load into lower word from SPE context.
     ;* Swap upper and lower to restore upper word and retain lower
     ;* word value
     ;*/

    ;/* Load instructions are done together to make full use of cache */
    EVMERGELOHI     R0  , R0  , R0
    EVMERGELOHI     R2  , R2  , R2
    EVMERGELOHI     R5  , R5  , R5
    EVMERGELOHI     R6  , R6  , R6
    EVMERGELOHI     R7  , R7  , R7
    EVMERGELOHI     R8  , R8  , R8
    EVMERGELOHI     R9  , R9  , R9
    EVMERGELOHI     R10 , R10 , R10
    EVMERGELOHI     R11 , R11 , R11
    EVMERGELOHI     R12 , R12 , R12
    EVMERGELOHI     R13 , R13 , R13
    EVMERGELOHI     R14 , R14 , R14
    EVMERGELOHI     R15 , R15 , R15
    EVMERGELOHI     R16 , R16 , R16
    EVMERGELOHI     R17 , R17 , R17
    EVMERGELOHI     R18 , R18 , R18
    EVMERGELOHI     R19 , R19 , R19
    EVMERGELOHI     R20 , R20 , R20
    EVMERGELOHI     R21 , R21 , R21
    EVMERGELOHI     R22 , R22 , R22
    EVMERGELOHI     R23 , R23 , R23
    EVMERGELOHI     R24 , R24 , R24
    EVMERGELOHI     R25 , R25 , R25
    EVMERGELOHI     R26 , R26 , R26
    EVMERGELOHI     R27 , R27 , R27
    EVMERGELOHI     R28 , R28 , R28
    EVMERGELOHI     R29 , R29 , R29
    EVMERGELOHI     R30 , R30 , R30
    EVMERGELOHI     R31 , R31 , R31

    LWZ             R0  , SPE_OFFSET(0)(R3)
    LWZ             R2  , SPE_OFFSET(2)(R3)
    LWZ             R5  , SPE_OFFSET(5)(R3)
    LWZ             R6  , SPE_OFFSET(6)(R3)
    LWZ             R7  , SPE_OFFSET(7)(R3)
    LWZ             R8  , SPE_OFFSET(8)(R3)
    LWZ             R9  , SPE_OFFSET(9)(R3)
    LWZ             R10 , SPE_OFFSET(10)(R3)
    LWZ             R11 , SPE_OFFSET(11)(R3)
    LWZ             R12 , SPE_OFFSET(12)(R3)
    LWZ             R13 , SPE_OFFSET(13)(R3)
    LWZ             R14 , SPE_OFFSET(14)(R3)
    LWZ             R15 , SPE_OFFSET(15)(R3)
    LWZ             R16 , SPE_OFFSET(16)(R3)
    LWZ             R17 , SPE_OFFSET(17)(R3)
    LWZ             R18 , SPE_OFFSET(18)(R3)
    LWZ             R19 , SPE_OFFSET(19)(R3)
    LWZ             R20 , SPE_OFFSET(20)(R3)
    LWZ             R21 , SPE_OFFSET(21)(R3)
    LWZ             R22 , SPE_OFFSET(22)(R3)
    LWZ             R23 , SPE_OFFSET(23)(R3)
    LWZ             R24 , SPE_OFFSET(24)(R3)
    LWZ             R25 , SPE_OFFSET(25)(R3)
    LWZ             R26 , SPE_OFFSET(26)(R3)
    LWZ             R27 , SPE_OFFSET(27)(R3)
    LWZ             R28 , SPE_OFFSET(28)(R3)
    LWZ             R29 , SPE_OFFSET(29)(R3)
    LWZ             R30 , SPE_OFFSET(30)(R3)
    LWZ             R31 , SPE_OFFSET(31)(R3)

    EVMERGELOHI     R0  , R0  , R0
    EVMERGELOHI     R2  , R2  , R2
    EVMERGELOHI     R5  , R5  , R5
    EVMERGELOHI     R6  , R6  , R6
    EVMERGELOHI     R7  , R7  , R7
    EVMERGELOHI     R8  , R8  , R8
    EVMERGELOHI     R9  , R9  , R9
    EVMERGELOHI     R10 , R10 , R10
    EVMERGELOHI     R11 , R11 , R11
    EVMERGELOHI     R12 , R12 , R12
    EVMERGELOHI     R13 , R13 , R13
    EVMERGELOHI     R14 , R14 , R14
    EVMERGELOHI     R15 , R15 , R15
    EVMERGELOHI     R16 , R16 , R16
    EVMERGELOHI     R17 , R17 , R17
    EVMERGELOHI     R18 , R18 , R18
    EVMERGELOHI     R19 , R19 , R19
    EVMERGELOHI     R20 , R20 , R20
    EVMERGELOHI     R21 , R21 , R21
    EVMERGELOHI     R22 , R22 , R22
    EVMERGELOHI     R23 , R23 , R23
    EVMERGELOHI     R24 , R24 , R24
    EVMERGELOHI     R25 , R25 , R25
    EVMERGELOHI     R26 , R26 , R26
    EVMERGELOHI     R27 , R27 , R27
    EVMERGELOHI     R28 , R28 , R28
    EVMERGELOHI     R29 , R29 , R29
    EVMERGELOHI     R30 , R30 , R30
    EVMERGELOHI     R31 , R31 , R31
    SYNC
    BLR

LINE_LABEL(__ppcVfpSpeRestoreItlbLoad)
    B               __ppcVfpSpeRestoreItlbLoadDone
    BLR
    FUNC_END()

    FILE_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
