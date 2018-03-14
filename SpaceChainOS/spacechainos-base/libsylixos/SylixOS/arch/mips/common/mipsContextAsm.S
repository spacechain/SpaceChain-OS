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
;** 文   件   名: mipsContextAsm.S
;**
;** 创   建   人: Ryan.Xin (信金龙)
;**
;** 文件创建日期: 2015 年 09 月 01 日
;**
;** 描        述: MIPS 体系构架上下文切换.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/kernel/kernel_cfg.h>
#include <config/mp/mp_cfg.h>
#include "mipsContextAsm.h"

    FILE_BEGIN()

    IMPORT_LABEL(_SchedSwp)
#if LW_CFG_SMP_EN > 0
    IMPORT_LABEL(_SchedSafeStack)
#endif
#if LW_CFG_COROUTINE_EN > 0
    IMPORT_LABEL(_SchedCrSwp)
#endif

    EXPORT_LABEL(archGetGP)
    EXPORT_LABEL(archTaskCtxStart)
    EXPORT_LABEL(archTaskCtxSwitch)
#if LW_CFG_COROUTINE_EN > 0
    EXPORT_LABEL(archCrtCtxSwitch)
#endif
    EXPORT_LABEL(archIntCtxLoad)
    EXPORT_LABEL(archSigCtxLoad)

;/*********************************************************************************************************
;  获得 _gp
;*********************************************************************************************************/

FUNC_DEF(archGetGP)
    LA      V0 , _gp
    JR      RA
    NOP
    FUNC_END(archGetGP)

;/*********************************************************************************************************
;  当前 CPU 开始运行多任务执行
;  参数为当前 CPU 控制块, 即 A0 为当前 CPU 控制块指针
;*********************************************************************************************************/

FUNC_DEF(archTaskCtxStart)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile

    LW      T0 , 0(A0)                                                  ;/*  获取该任务堆栈栈顶地址      */
    LW      SP , 0(T0)                                                  ;/*  堆栈指针赋值                */

    RESTORE_REGS                                                        ;/*  所有寄存器出栈              */

    .set    pop
    FUNC_END(archTaskCtxStart)

;/*********************************************************************************************************
;  线程切换
;  参数为当前 CPU 控制块, 即 A0 为当前 CPU 控制块指针
;*********************************************************************************************************/

FUNC_DEF(archTaskCtxSwitch)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile

    SAVE_REGS                                                           ;/*  所有寄存器入栈              */

    LW      T0 , 0(A0)                                                  ;/*  stack current = SP          */
    SW      SP , 0(T0)

#if LW_CFG_SMP_EN > 0
    SUBU    SP , 32
    SW      A0 , 28(SP)                                                 ;/*  当前 CPU 信息入栈           */
    JAL     _SchedSafeStack                                             ;/*  _SchedSafeStack();          */
    NOP
    LW      A0 , 28(SP)                                                 ;/*  当前 CPU 信息出栈           */
    MOV     SP , V0
#endif

    SUBU    SP , 32
    SW      A0 , 28(SP)                                                 ;/*  当前 CPU 信息入栈           */
    JAL     _SchedSwp                                                   ;/*  _SchedSwp();                */
    NOP
    LW      A0 , 28(SP)                                                 ;/*  当前 CPU 信息出栈           */
    ADDU    SP , 32

    LW      T0 , 0(A0)                                                  ;/*  SP = stack current          */
    LW      SP , 0(T0)

    RESTORE_REGS                                                        ;/*  所有寄存器出栈              */

    .set    pop
    FUNC_END(archTaskCtxSwitch)

;/*********************************************************************************************************
;  协程切换
;  参数为当前 CPU 控制块, 即 A0 为当前 CPU 控制块指针
;*********************************************************************************************************/

#if LW_CFG_COROUTINE_EN > 0
FUNC_DEF(archCrtCtxSwitch)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile

    SAVE_REGS                                                           ;/*  所有寄存器入栈              */

    LW      T0 , 8(A0)                                                  ;/*  stack current = SP          */
    SW      SP , 0(T0)

    SUBU    SP , 32
    SW      A0 , 28(SP)                                                 ;/*  当前 CPU 信息入栈           */
    JAL     _SchedCrSwp                                                 ;/*  _SchedCrSwp();              */
    NOP
    LW      A0 , 28(SP)                                                 ;/*  当前 CPU 信息出栈           */
    ADDU    SP , 32

    LW      T0 , 8(A0)                                                  ;/*  SP = stack current          */
    LW      SP , 0(T0)

    RESTORE_REGS                                                        ;/*  所有寄存器出栈              */

    .set    pop
    FUNC_END(archCrtCtxSwitch)
#endif

;/*********************************************************************************************************
;  中断返回时, 线程装载
;  参数为当前 CPU 控制块, 即 A0 为当前 CPU 控制块指针
;*********************************************************************************************************/

FUNC_DEF(archIntCtxLoad)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile

    LW      T0 , 0(A0)                                                  ;/*  SP = stack current          */
    LW      SP , 0(T0)

    RESTORE_REGS                                                        ;/*  所有寄存器出栈              */

    .set    pop
    FUNC_END(archIntCtxLoad)

;/*********************************************************************************************************
;  信号上下文返回
;  参数为需要返回的堆栈栈顶位置
;*********************************************************************************************************/

FUNC_DEF(archSigCtxLoad)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile

    MOV     SP , A0                                                     ;/*  返回的堆栈指针              */

    RESTORE_REGS                                                        ;/*  所有寄存器出栈              */

    .set    pop
    FUNC_END(archSigCtxLoad)

    FILE_END()

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
