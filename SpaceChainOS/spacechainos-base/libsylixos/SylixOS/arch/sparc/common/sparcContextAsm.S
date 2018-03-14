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
;** 文   件   名: sparcContextAsm.S
;**
;** 创   建   人: Xu.Guizhou (徐贵洲)
;**
;** 文件创建日期: 2017 年 05 月 15 日
;**
;** 描        述: SPARC 体系构架上下文切换.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/kernel/kernel_cfg.h>
#include <config/mp/mp_cfg.h>
#include "sparcContextAsm.h"

    FILE_BEGIN()

    IMPORT_LABEL(_SchedSwp)
    IMPORT_LABEL(_G_ulIntSafeStack)
#if LW_CFG_COROUTINE_EN > 0
    IMPORT_LABEL(_SchedCrSwp)
#endif

    EXPORT_LABEL(archTaskCtxStart)
    EXPORT_LABEL(archTaskCtxSwitch)
#if LW_CFG_COROUTINE_EN > 0
    EXPORT_LABEL(archCrtCtxSwitch)
#endif
    EXPORT_LABEL(archIntCtxLoad)
    EXPORT_LABEL(archSigCtxLoad)

;/*********************************************************************************************************
;  线程切换
;  参数为当前 CPU 控制块, 即 %o0 为当前 CPU 控制块指针
;*********************************************************************************************************/

FUNC_DEF(archTaskCtxSwitch)
    SAVE_REGS                                                           ;/*  保存上下文                  */

    LD      [%o0], %l0                                                  ;/*  %l0 = 当前 TCB 指针         */
    ST      %g1  , [%l0]                                                ;/*  保存当前的栈指针到当前 TCB  */

    ;/*
    ; * %g1 = 当前 CPU 指针, 保证 flush 窗口后, 还能通过 %g1 获取当前 CPU 指针
    ; */
    MOV     %o0  , %g1

    FLUSH_WINDOWS                                                       ;/*  flush 窗口                  */

    ;/*
    ; * 切换到中断安全栈
    ; */
    SET     _G_ulIntSafeStack , %l0                                     ;/*  %l0 = _G_ulIntSafeStack     */

#if LW_CFG_SMP_EN > 0
    READ_CPUID(%l1)                                                     ;/*  %l1 = CPU_ID                */
    SLL     %l1   , 2   , %l1                                           ;/*  %l1 = CPU_ID * 4            */
    ADD     %l1   , %l0 , %l0                                           ;/*  %l0 = &safestack[CPU_ID]    */
#endif

    LD      [%l0] , %l1                                                 ;/*  %l1 = 中断安全栈            */
    SUB     %l1   , ASM_STACK_FRAME_SIZE , %sp                          ;/*  开辟栈空间                  */

    ;/*
    ; * 调用 C 函数 _SchedSwp(%o0 为当前 CPU 控制块指针)
    ; * %l0 暂存当前 CPU 控制块指针(%l0 是 assumed by caller to be preserved across a procedure call)
    ; */
    MOV     %g1 , %o0
    MOV     %g1 , %l0
    CALL    _SchedSwp
    NOP

    LD      [%l0] , %o0                                                 ;/*  %o0 = 当前 TCB 指针         */
    LD      [%o0] , %g1                                                 ;/*  %g1 = 当前 TCB 的栈指针     */
    RESTORE_REGS
    FUNC_END(archTaskCtxSwitch)

;/*********************************************************************************************************
;  协程切换
;  参数为当前 CPU 控制块, 即 %o0 为当前 CPU 控制块指针
;*********************************************************************************************************/

#if LW_CFG_COROUTINE_EN > 0
FUNC_DEF(archCrtCtxSwitch)
    SAVE_REGS                                                           ;/*  保存上下文                  */

    LD      [%o0 + 8], %l0                                              ;/*  %l0 = 当前 crcb 指针        */
    ST      %g1  , [%l0]                                                ;/*  保存当前的栈指针到当前 crcb */

    ;/*
    ; * %g1 = 当前 CPU 指针, 保证 flush 窗口后, 还能通过 %g1 获取当前 CPU 指针
    ; */
    MOV     %o0  , %g1

    FLUSH_WINDOWS                                                       ;/*  flush 窗口                  */

    ;/*
    ; * 切换到中断安全栈
    ; */
    SET     _G_ulIntSafeStack , %l0                                     ;/*  %l0 = _G_ulIntSafeStack     */

#if LW_CFG_SMP_EN > 0
    READ_CPUID(%l1)                                                     ;/*  %l1 = CPU_ID                */
    SLL     %l1   , 2   , %l1                                           ;/*  %l1 = CPU_ID * 4            */
    ADD     %l1   , %l0 , %l0                                           ;/*  %l0 = &safestack[CPU_ID]    */
#endif

    LD      [%l0] , %l1                                                 ;/*  %l1 = 中断安全栈            */
    SUB     %l1   , ASM_STACK_FRAME_SIZE , %sp                          ;/*  开辟栈空间                  */

    ;/*
    ; * 调用 C 函数 _SchedCrSwp(%o0 为当前 CPU 控制块指针)
    ; * %l0 暂存当前 CPU 控制块指针(%l0 是 assumed by caller to be preserved across a procedure call)
    ; */
    MOV     %g1 , %o0
    MOV     %g1 , %l0
    CALL    _SchedCrSwp
    NOP

    LD      [%l0 + 8] , %o0                                             ;/*  %o0 = 当前 crcb 指针        */
    LD      [%o0] , %g1                                                 ;/*  %g1 = 当前 crcb 的栈指针    */
    RESTORE_REGS
    FUNC_END(archCrtCtxSwitch)
#endif

;/*********************************************************************************************************
;  当前 CPU 开始运行多任务执行
;  参数为当前 CPU 控制块, 即 %o0 为当前 CPU 控制块指针
;*********************************************************************************************************/

FUNC_DEF(archTaskCtxStart)
    LD      [%o0] , %o0                                                 ;/*  %o0 = 当前 TCB 指针         */
    LD      [%o0] , %g1                                                 ;/*  %g1 = 当前 TCB 的栈指针     */
    RESTORE_REGS
    FUNC_END(archTaskCtxStart)

;/*********************************************************************************************************
;  中断返回时, 线程装载
;  参数为当前 CPU 控制块, 即 %o0 为当前 CPU 控制块指针
;*********************************************************************************************************/

FUNC_DEF(archIntCtxLoad)
    LD      [%o0] , %o0                                                 ;/*  %o0 = 当前 TCB 指针         */
    LD      [%o0] , %g1                                                 ;/*  %g1 = 当前 TCB 的栈指针     */
    RESTORE_REGS
    FUNC_END(archIntCtxLoad)

;/*********************************************************************************************************
;  信号上下文返回
;  参数为需要返回的堆栈栈顶位置, 即 %o0 为堆栈栈顶位置
;*********************************************************************************************************/

FUNC_DEF(archSigCtxLoad)
    MOV     %o0 , %g1                                                   ;/*  %g1 = 堆栈栈顶位置          */
    RESTORE_REGS
    FUNC_END(archSigCtxLoad)

    FILE_END()

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
