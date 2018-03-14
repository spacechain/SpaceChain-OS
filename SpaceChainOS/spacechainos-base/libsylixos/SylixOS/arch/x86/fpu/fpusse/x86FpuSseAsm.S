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
;** 文   件   名: x86FpuSseAsm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2016 年 08 月 05 日
;**
;** 描        述: x86 体系架构 FPU 支持.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>

#if LW_CFG_CPU_FPU_EN > 0

    FILE_BEGIN()

    EXPORT_LABEL(x86FpuSseInit)

    EXPORT_LABEL(x86FpuSseEnable)
    EXPORT_LABEL(x86FpuSseDisable)
    EXPORT_LABEL(x86FpuSseIsEnable)

    EXPORT_LABEL(x86FpuSseSave)
    EXPORT_LABEL(x86FpuSseRestore)

    EXPORT_LABEL(x86FpuSseXSave)
    EXPORT_LABEL(x86FpuSseXRestore)

    EXPORT_LABEL(x86FpuSseXExtSave)
    EXPORT_LABEL(x86FpuSseXExtRestore)
    EXPORT_LABEL(x86FpuSseEnableYMMState)

;/*********************************************************************************************************
;  初始化 FPU
;  i386 使用 387 协处理器, 后面的 CPU 都使用 487 协处理器, NS486 无 FPU, 调用该函数时, 上层已经确定有 FPU
;*********************************************************************************************************/

FUNC_DEF(x86FpuSseInit)
    CLTS
    FNINIT
    MOVL    $0 , %EDX
    FNSTCW  (%EDX)
    MOVW    (%EDX) , %AX
    CMPW    $0x37f , %AX
    JNE     x86FpuSseInitNo487
    MOVL    %CR0 , %EAX
    ANDL    $0xffffffd9 , %EAX
    ORL     $0x00000022 , %EAX
    MOVL    %EAX , %CR0                                                 ;/*  NE=1, EM=0, MP=1            */
    JMP     x86FpuSseInitInitRegs

LINE_LABEL(x86FpuSseInitNo487)
    MOVL    %CR0 , %EAX
    ANDL    $0xffffffd9 , %EAX
    ORL     $0x00000004 , %EAX
    MOVL    %EAX , %CR0                                                 ;/*  NE=0, EM=1, MP=0            */
    MOVL    $-1  , %EAX                                                 ;/*  Return PX_ERROR             */
    JMP     x86FpuSseInitDone

LINE_LABEL(x86FpuSseInitInitRegs)
    ;/*
    ; * FNINIT 不初始化以下数据寄存器(在寄存器堆栈), 所以下面初始化它们
    ; */
    FLDZ
    FLDZ
    FLDZ
    FLDZ
    FLDZ
    FLDZ
    FLDZ
    FLDZ
    FNINIT

    XORL    %EAX , %EAX                                                 ;/*  Return ERROR_NONE           */

LINE_LABEL(x86FpuSseInitDone)
    RET
    FUNC_END(x86FpuSseInit)

;/*********************************************************************************************************
;  使能 FPU
;*********************************************************************************************************/

MACRO_DEF(ENABLE_FPU)
    CLTS
    MACRO_END()

;/*********************************************************************************************************
;  使能 FPU
;*********************************************************************************************************/

FUNC_DEF(x86FpuSseEnable)
    ENABLE_FPU
    RET
    FUNC_END(x86FpuSseEnable)

;/*********************************************************************************************************
;  关闭 FPU
;*********************************************************************************************************/

FUNC_DEF(x86FpuSseDisable)
    MOVL    %CR0 , %EAX
    ORL     $X86_CR0_TS , %EAX                                          ;/*  设置 CR0 的 TS 位           */
    MOVL    %EAX , %CR0
    RET
    FUNC_END(x86FpuSseDisable)

;/*********************************************************************************************************
;  判断 FPU 是否使能
;*********************************************************************************************************/

FUNC_DEF(x86FpuSseIsEnable)
    MOVL    %CR0 , %EAX
    TESTL   $X86_CR0_TS , %EAX
    JZ      __x86FpuSseIsEnable                                         ;/*  CR0 的 TS 位为 0 时为使能   */
    MOVL    $0 , %EAX
    RET
LINE_LABEL(__x86FpuSseIsEnable)
    MOVL    $1 , %EAX
    RET
    FUNC_END(x86FpuSseIsEnable)

;/*********************************************************************************************************
;  保存寄存器
;*********************************************************************************************************/

FUNC_DEF(x86FpuSseSave)
    ENABLE_FPU
    MOVL    X86_SP_ARG1(%ESP) , %EAX
    FNSAVE  (%EAX)
    RET
    FUNC_END(x86FpuSseSave)

;/*********************************************************************************************************
;  恢复寄存器
;*********************************************************************************************************/

FUNC_DEF(x86FpuSseRestore)
    ENABLE_FPU
    PUSHL   %EBP
    MOVL    %ESP , %EBP

    SUBL    $4 , %ESP
    FNCLEX                                                              ;/*  清除异常标志                */
    MOVL    $0x37f , -4(%EBP)
    FLDCW   -4(%EBP)                                                    ;/*  临时屏蔽所有异常            */
    MOVL    X86_FP_ARG1(%EBP) , %EAX
    FRSTOR  (%EAX)                                                      ;/*  新异常掩码被恢复            */

    MOVL    %EBP , %ESP
    POPL    %EBP
    RET
    FUNC_END(x86FpuSseRestore)

;/*********************************************************************************************************
;  保存寄存器
;*********************************************************************************************************/

FUNC_DEF(x86FpuSseXSave)
    ENABLE_FPU
    MOVL    X86_SP_ARG1(%ESP) , %EAX
    FXSAVE  (%EAX)
    FNCLEX                                                              ;/*  清除异常标志                */
    RET
    FUNC_END(x86FpuSseXSave)

;/*********************************************************************************************************
;  恢复寄存器
;*********************************************************************************************************/

FUNC_DEF(x86FpuSseXRestore)
    ENABLE_FPU
    MOVL    X86_SP_ARG1(%ESP) , %EAX
    FXRSTOR (%EAX)
    RET
    FUNC_END(x86FpuSseXRestore)

;/*********************************************************************************************************
;  保存寄存器
;*********************************************************************************************************/

FUNC_DEF(x86FpuSseXExtSave)
    ENABLE_FPU
    MOVL    X86_SP_ARG1(%ESP) , %ECX                        ;/*  Where to save registers                 */

    MOVL    $0 , %EDX                                       ;/*  EDX:EAX define the state to be saved    */
    MOVL    $7 , %EAX                                       ;/*  7 - Saves FPU, XMY and YMM state        */

    XSAVE   (%ECX)

    FNCLEX                                                  ;/*  Clear the exception flags               */
    RET
    FUNC_END(x86FpuSseXExtSave)

;/*********************************************************************************************************
;  恢复寄存器
;*********************************************************************************************************/

FUNC_LABEL(x86FpuSseXExtRestore)
    ENABLE_FPU
    MOVL    X86_SP_ARG1(%ESP) , %ECX                        ;/*  From where to restore registers         */

    MOVL    $0 , %EDX                                       ;/*  EDX:EAX define the state to be restore  */
    MOVL    $7 , %EAX                                       ;/*  7 - Restore FPU, XMY and YMM state      */

    XRSTOR  (%ECX)
    RET
FUNC_END(x86FpuSseXExtRestore)

;/*********************************************************************************************************
;  使能 YMM 状态
;*********************************************************************************************************/

FUNC_DEF(x86FpuSseEnableYMMState)
    MOVL    $0 , %ECX                                       ;/*  Select XCR0                             */
    MOVL    $0 , %EDX                                       ;/*  EDX:EAX define the state to be enable   */
    MOVL    $7 , %EAX                                       ;/*  7 - Enable FPU, XMY and YMM state       */
    XSETBV
    RET
    FUNC_END(x86FpuSseEnableYMMState)

    FILE_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
