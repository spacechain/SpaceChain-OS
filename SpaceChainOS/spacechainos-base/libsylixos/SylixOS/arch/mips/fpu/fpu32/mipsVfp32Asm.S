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
;** 文   件   名: mipsVfp32Asm.S
;**
;** 创   建   人: Ryan.Xin (信金龙)
;**
;** 文件创建日期: 2015 年 11 月 17 日
;**
;** 描        述: MIPS 体系架构 VFP32 支持.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>

#if LW_CFG_CPU_FPU_EN > 0
#include "arch/mips/arch_float.h"

    FILE_BEGIN()

    EXPORT_LABEL(mipsVfp32Init)
    EXPORT_LABEL(mipsVfp32GetFIR)
    EXPORT_LABEL(mipsVfp32Enable)
    EXPORT_LABEL(mipsVfp32Disable)
    EXPORT_LABEL(mipsVfp32IsEnable)
    EXPORT_LABEL(mipsVfp32Save)
    EXPORT_LABEL(mipsVfp32Restore)
    EXPORT_LABEL(mipsVfp32GetFEXR)
    EXPORT_LABEL(mipsVfp32ClearFEXR)

;/*********************************************************************************************************
;  使能 FPU
;*********************************************************************************************************/

MACRO_DEF(ENABLE_FPU)
    MFC0(T0, CP0_STATUS)
    OR      T0 , M_StatusCU1
    MTC0(T0, CP0_STATUS)
    MACRO_END()

;/*********************************************************************************************************
;  初始化 FPU
;*********************************************************************************************************/

FUNC_DEF(mipsVfp32Init)
    MFC0(T1, CP0_STATUS)

    ENABLE_FPU

    MTC1    ZERO , FP0                                                  ;/* Zero the registers           */
    MTC1    ZERO , FP1
    MTC1    ZERO , FP2
    MTC1    ZERO , FP3
    MTC1    ZERO , FP4
    MTC1    ZERO , FP5
    MTC1    ZERO , FP6
    MTC1    ZERO , FP7
    MTC1    ZERO , FP8
    MTC1    ZERO , FP9
    MTC1    ZERO , FP10
    MTC1    ZERO , FP11
    MTC1    ZERO , FP12
    MTC1    ZERO , FP13
    MTC1    ZERO , FP14
    MTC1    ZERO , FP15
    MTC1    ZERO , FP16
    MTC1    ZERO , FP17
    MTC1    ZERO , FP18
    MTC1    ZERO , FP19
    MTC1    ZERO , FP20
    MTC1    ZERO , FP21
    MTC1    ZERO , FP22
    MTC1    ZERO , FP23
    MTC1    ZERO , FP24
    MTC1    ZERO , FP25
    MTC1    ZERO , FP26
    MTC1    ZERO , FP27
    MTC1    ZERO , FP28
    MTC1    ZERO , FP29
    MTC1    ZERO , FP30
    MTC1    ZERO , FP31

    CFC1(T0,  CP1_STATUS)                                               ;/*  Read to finish float oper   */

    LI      T0 , (1 << 24)                                              ;/*  Set FS bit                  */
    CTC1(T0, CP1_STATUS)

    MTC0(T1, CP0_STATUS)
    JR      RA
    NOP
    FUNC_END(mipsVfp32Init)

;/*********************************************************************************************************
;  使能 FPU
;*********************************************************************************************************/

FUNC_DEF(mipsVfp32Enable)
    ENABLE_FPU
    JR      RA
    NOP
    FUNC_END(mipsVfp32Enable)

;/*********************************************************************************************************
;  禁能 FPU
;*********************************************************************************************************/

FUNC_DEF(mipsVfp32Disable)
    MFC0(T0, CP0_STATUS)
    AND     T0 , ~M_StatusCU1
    MTC0(T0, CP0_STATUS)
    JR      RA
    NOP
    FUNC_END(mipsVfp32Disable)

;/*********************************************************************************************************
;  判断 FPU 是否使能
;*********************************************************************************************************/

FUNC_DEF(mipsVfp32IsEnable)
    MOV     V0 , ZERO                                                   ;/*  FALSE return value          */
    MFC0(T0, CP0_STATUS)
    AND     T0 , M_StatusCU1                                            ;/*  coprocessor 1 enabled       */
    BEQ     T0 , ZERO, 1f
    NOP
    LI      V0 , 1                                                      ;/*  TRUE return value           */
1:
    JR      RA
    NOP
    FUNC_END(mipsVfp32IsEnable)

;/*********************************************************************************************************
;  获得 FIR 寄存器
;*********************************************************************************************************/

FUNC_DEF(mipsVfp32GetFIR)
    MFC0(T1, CP0_STATUS)

    ENABLE_FPU

    CFC1(V0,  CP1_FIR)

    MTC0(T1, CP0_STATUS)
    JR      RA
    NOP
    FUNC_END(mipsVfp32GetFIR)

;/*********************************************************************************************************
;  保存 FPU 寄存器
;*********************************************************************************************************/

FUNC_DEF(mipsVfp32Save)
    ENABLE_FPU

    CFC1(T0,  CP1_STATUS)
    SW      T0   , FPCSR_OFFSET(A0)

    SWC1    FP0  , FP0_OFFSET(A0)
    SWC1    FP1  , FP1_OFFSET(A0)
    SWC1    FP2  , FP2_OFFSET(A0)
    SWC1    FP3  , FP3_OFFSET(A0)
    SWC1    FP4  , FP4_OFFSET(A0)
    SWC1    FP5  , FP5_OFFSET(A0)
    SWC1    FP6  , FP6_OFFSET(A0)
    SWC1    FP7  , FP7_OFFSET(A0)
    SWC1    FP8  , FP8_OFFSET(A0)
    SWC1    FP9  , FP9_OFFSET(A0)
    SWC1    FP10 , FP10_OFFSET(A0)
    SWC1    FP11 , FP11_OFFSET(A0)
    SWC1    FP12 , FP12_OFFSET(A0)
    SWC1    FP13 , FP13_OFFSET(A0)
    SWC1    FP14 , FP14_OFFSET(A0)
    SWC1    FP15 , FP15_OFFSET(A0)
    SWC1    FP16 , FP16_OFFSET(A0)
    SWC1    FP17 , FP17_OFFSET(A0)
    SWC1    FP18 , FP18_OFFSET(A0)
    SWC1    FP19 , FP19_OFFSET(A0)
    SWC1    FP20 , FP20_OFFSET(A0)
    SWC1    FP21 , FP21_OFFSET(A0)
    SWC1    FP22 , FP22_OFFSET(A0)
    SWC1    FP23 , FP23_OFFSET(A0)
    SWC1    FP24 , FP24_OFFSET(A0)
    SWC1    FP25 , FP25_OFFSET(A0)
    SWC1    FP26 , FP26_OFFSET(A0)
    SWC1    FP27 , FP27_OFFSET(A0)
    SWC1    FP28 , FP28_OFFSET(A0)
    SWC1    FP29 , FP29_OFFSET(A0)
    SWC1    FP30 , FP30_OFFSET(A0)
    SWC1    FP31 , FP31_OFFSET(A0)

    JR      RA
    NOP
    FUNC_END(mipsVfp32Save)

;/*********************************************************************************************************
;  恢复 FPU 寄存器
;*********************************************************************************************************/

FUNC_DEF(mipsVfp32Restore)
    ENABLE_FPU

    LWC1    FP0  , FP0_OFFSET(A0)
    LWC1    FP1  , FP1_OFFSET(A0)
    LWC1    FP2  , FP2_OFFSET(A0)
    LWC1    FP3  , FP3_OFFSET(A0)
    LWC1    FP4  , FP4_OFFSET(A0)
    LWC1    FP5  , FP5_OFFSET(A0)
    LWC1    FP6  , FP6_OFFSET(A0)
    LWC1    FP7  , FP7_OFFSET(A0)
    LWC1    FP8  , FP8_OFFSET(A0)
    LWC1    FP9  , FP9_OFFSET(A0)
    LWC1    FP10 , FP10_OFFSET(A0)
    LWC1    FP11 , FP11_OFFSET(A0)
    LWC1    FP12 , FP12_OFFSET(A0)
    LWC1    FP13 , FP13_OFFSET(A0)
    LWC1    FP14 , FP14_OFFSET(A0)
    LWC1    FP15 , FP15_OFFSET(A0)
    LWC1    FP16 , FP16_OFFSET(A0)
    LWC1    FP17 , FP17_OFFSET(A0)
    LWC1    FP18 , FP18_OFFSET(A0)
    LWC1    FP19 , FP19_OFFSET(A0)
    LWC1    FP20 , FP20_OFFSET(A0)
    LWC1    FP21 , FP21_OFFSET(A0)
    LWC1    FP22 , FP22_OFFSET(A0)
    LWC1    FP23 , FP23_OFFSET(A0)
    LWC1    FP24 , FP24_OFFSET(A0)
    LWC1    FP25 , FP25_OFFSET(A0)
    LWC1    FP26 , FP26_OFFSET(A0)
    LWC1    FP27 , FP27_OFFSET(A0)
    LWC1    FP28 , FP28_OFFSET(A0)
    LWC1    FP29 , FP29_OFFSET(A0)
    LWC1    FP30 , FP30_OFFSET(A0)
    LWC1    FP31 , FP31_OFFSET(A0)
    LW      T0   , FPCSR_OFFSET(A0)

    CFC1(T1,  CP1_STATUS)                                               ;/*  Read to finish float oper   */
    CTC1(T0,  CP1_STATUS)                                               ;/*  Restore fpp status reg      */

    JR      RA
    NOP
    FUNC_END(mipsVfp32Restore)

;/*********************************************************************************************************
;  获得 FRXR 寄存器
;*********************************************************************************************************/

FUNC_DEF(mipsVfp32GetFEXR)
    CFC1(V0,  CP1_FEXR)
    JR      RA
    NOP
    FUNC_END(mipsVfp32GetFEXR)

;/*********************************************************************************************************
;  清除浮点异常
;*********************************************************************************************************/

FUNC_DEF(mipsVfp32ClearFEXR)
    CTC1(ZERO, CP1_FEXR)
    JR      RA
    NOP
    FUNC_END(mipsVfp32ClearFEXR)

    FILE_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
