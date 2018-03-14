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
;** 文   件   名: sparcVfpAsm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2017 年 09 月 29 日
;**
;** 描        述: SPARC 体系架构 VPU 支持.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>

#if LW_CFG_CPU_FPU_EN > 0
#include "arch/sparc/arch_float.h"

    FILE_BEGIN()

    EXPORT_LABEL(sparcVfpEnable)
    EXPORT_LABEL(sparcVfpDisable)
    EXPORT_LABEL(sparcVfpIsEnable)
    EXPORT_LABEL(sparcVfpSave)
    EXPORT_LABEL(sparcVfpRestore)

;/*********************************************************************************************************
;  使能 FPU 宏
;*********************************************************************************************************/

MACRO_DEF(ENABLE_VFP)
    RD      %psr , %o1
    SET     PSR_EF , %o2
    OR      %o1 , %o2 , %o1
    WR      %o1 , %psr
    NOP
    NOP
    NOP
    MACRO_END()

;/*********************************************************************************************************
;  使能 FPU
;*********************************************************************************************************/

FUNC_DEF(sparcVfpEnable)
    ENABLE_VFP
    RETL
    NOP
    FUNC_END(sparcVfpEnable)

;/*********************************************************************************************************
;  关闭 FPU
;*********************************************************************************************************/

FUNC_DEF(sparcVfpDisable)
    RD      %psr , %o1
    SET     PSR_EF , %o2
    ANDN    %o1 , %o2 , %o1
    WR      %o1 , %psr
    NOP
    RETL
    NOP
    FUNC_END(sparcVfpDisable)

;/*********************************************************************************************************
;  判断 FPU 是否使能
;*********************************************************************************************************/

FUNC_DEF(sparcVfpIsEnable)
    RD      %psr , %o1
    SET     PSR_EF , %o2
    ANDCC   %o1 , %o2 , %g0
    BNZ     __sparcVfpIsEnable
    NOP
    RETL
    MOV     %g0 , %o0
LINE_LABEL(__sparcVfpIsEnable)
    RETL
    MOV     1 , %o0
    FUNC_END(sparcVfpIsEnable)

;/*********************************************************************************************************
;  保存寄存器
;*********************************************************************************************************/

FUNC_DEF(sparcVfpSave)
    ENABLE_VFP
    STD     %f0  , [%o0 + FO_F1_OFFSET]
    STD     %f2  , [%o0 + F2_F3_OFFSET]
    STD     %f4  , [%o0 + F4_F5_OFFSET]
    STD     %f6  , [%o0 + F6_F7_OFFSET]
    STD     %f8  , [%o0 + F8_F9_OFFSET]
    STD     %f10 , [%o0 + F1O_F11_OFFSET]
    STD     %f12 , [%o0 + F12_F13_OFFSET]
    STD     %f14 , [%o0 + F14_F15_OFFSET]
    STD     %f16 , [%o0 + F16_F17_OFFSET]
    STD     %f18 , [%o0 + F18_F19_OFFSET]
    STD     %f20 , [%o0 + F2O_F21_OFFSET]
    STD     %f22 , [%o0 + F22_F23_OFFSET]
    STD     %f24 , [%o0 + F24_F25_OFFSET]
    STD     %f26 , [%o0 + F26_F27_OFFSET]
    STD     %f28 , [%o0 + F28_F29_OFFSET]
    STD     %f30 , [%o0 + F3O_F31_OFFSET]
    ST      %fsr , [%o0 + FSR_OFFSET]
    RETL
    NOP
    FUNC_END(sparcVfpSave)

;/*********************************************************************************************************
;  恢复寄存器
;*********************************************************************************************************/

FUNC_DEF(sparcVfpRestore)
    ENABLE_VFP
    LDD     [%o0 + FO_F1_OFFSET]   , %f0
    LDD     [%o0 + F2_F3_OFFSET]   , %f2
    LDD     [%o0 + F4_F5_OFFSET]   , %f4
    LDD     [%o0 + F6_F7_OFFSET]   , %f6
    LDD     [%o0 + F8_F9_OFFSET]   , %f8
    LDD     [%o0 + F1O_F11_OFFSET] , %f10
    LDD     [%o0 + F12_F13_OFFSET] , %f12
    LDD     [%o0 + F14_F15_OFFSET] , %f14
    LDD     [%o0 + F16_F17_OFFSET] , %f16
    LDD     [%o0 + F18_F19_OFFSET] , %f18
    LDD     [%o0 + F2O_F21_OFFSET] , %f20
    LDD     [%o0 + F22_F23_OFFSET] , %f22
    LDD     [%o0 + F24_F25_OFFSET] , %f24
    LDD     [%o0 + F26_F27_OFFSET] , %f26
    LDD     [%o0 + F28_F29_OFFSET] , %f28
    LDD     [%o0 + F3O_F31_OFFSET] , %f30
    LD      [%o0 + FSR_OFFSET]     , %fsr
    RETL
    NOP
    FUNC_END(sparcVfpRestore)

    FILE_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
