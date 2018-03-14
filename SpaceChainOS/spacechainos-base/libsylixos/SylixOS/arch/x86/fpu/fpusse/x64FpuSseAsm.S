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
;** 文   件   名: x64FpuSseAsm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2017 年 06 月 05 日
;**
;** 描        述: x86-64 体系架构 FPU 支持.
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
    MOV     $0 , %RDX
    FNSTCW  (%RDX)
    MOVW    (%RDX) , %AX
    CMPW    $0x37f , %AX
    JNE     x86FpuSseInitNo487
    MOV     %CR0 , %RAX
    AND     $0xffffffffffffffd9 , %RAX
    OR      $0x0000000000000022 , %RAX
    MOV     %RAX , %CR0                                                 ;/*  NE=1, EM=0, MP=1            */
    JMP     x86FpuSseInitInitRegs

LINE_LABEL(x86FpuSseInitNo487)
    MOV     %CR0 , %RAX
    AND     $0xffffffffffffffd9 , %RAX
    OR      $0x0000000000000004 , %RAX
    MOV     %RAX , %CR0                                                 ;/*  NE=0, EM=1, MP=0            */
    MOV     $-1  , X86_64_RETREG                                        ;/*  Return PX_ERROR             */
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

    MOV     $0 , X86_64_RETREG                                          ;/*  Return ERROR_NONE           */

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
    MOV     %CR0 , %RAX
    OR      $X86_CR0_TS , %RAX                                          ;/*  设置 CR0 的 TS 位           */
    MOV     %RAX , %CR0
    RET
    FUNC_END(x86FpuSseDisable)

;/*********************************************************************************************************
;  判断 FPU 是否使能
;*********************************************************************************************************/

FUNC_DEF(x86FpuSseIsEnable)
    MOV     %CR0 , %RAX
    TEST    $X86_CR0_TS , %RAX
    JZ      __x86FpuSseIsEnable                                         ;/*  CR0 的 TS 位为 0 时为使能   */
    MOV     $0 , X86_64_RETREG
    RET
LINE_LABEL(__x86FpuSseIsEnable)
    MOV     $1 , X86_64_RETREG
    RET
    FUNC_END(x86FpuSseIsEnable)

;/*********************************************************************************************************
;  保存寄存器
;*********************************************************************************************************/

FUNC_DEF(x86FpuSseSave)
    ENABLE_FPU
    FNSAVE  (X86_64_ARG0)
    RET
    FUNC_END(x86FpuSseSave)

;/*********************************************************************************************************
;  恢复寄存器
;*********************************************************************************************************/

FUNC_DEF(x86FpuSseRestore)
    ENABLE_FPU
    PUSH    %RBP
    MOV     %RSP , %RBP

    SUB     $8 , %RSP
    FNCLEX                                                              ;/*  清除异常标志                */
    MOVL    $0x37f , -4(%RBP)
    FLDCW   -4(%RBP)                                                    ;/*  临时屏蔽所有异常            */
    FRSTOR  (X86_64_ARG0)                                               ;/*  新异常掩码被恢复            */

    MOV     %RBP , %RSP
    POP     %RBP

    RET
    FUNC_END(x86FpuSseRestore)

;/*********************************************************************************************************
;  保存寄存器
;*********************************************************************************************************/

FUNC_DEF(x86FpuSseXSave)
    ENABLE_FPU
    FXSAVE  (X86_64_ARG0)
    FNCLEX                                                              ;/*  清除异常标志                */
    RET
    FUNC_END(x86FpuSseXSave)

;/*********************************************************************************************************
;  恢复寄存器
;*********************************************************************************************************/

FUNC_DEF(x86FpuSseXRestore)
    ENABLE_FPU
    FXRSTOR (X86_64_ARG0)
    RET
    FUNC_END(x86FpuSseXRestore)

;/*********************************************************************************************************
;  保存寄存器
;*********************************************************************************************************/

FUNC_DEF(x86FpuSseXExtSave)
    ENABLE_FPU

    MOVL    $0 , %EDX                                       ;/*  EDX:EAX define the state to be saved    */
    MOVL    $7 , %EAX                                       ;/*  7 - Saves FPU, XMY and YMM state        */

    XSAVE   (X86_64_ARG0)

    FNCLEX                                                  ;/*  Clear the exception flags               */
    RET
    FUNC_END(x86FpuSseXExtSave)

;/*********************************************************************************************************
;  恢复寄存器
;*********************************************************************************************************/

FUNC_LABEL(x86FpuSseXExtRestore)
    ENABLE_FPU

    MOVL    $0 , %EDX                                       ;/*  EDX:EAX define the state to be restore  */
    MOVL    $7 , %EAX                                       ;/*  7 - Restore FPU, XMY and YMM state      */

    XRSTOR  (X86_64_ARG0)
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
