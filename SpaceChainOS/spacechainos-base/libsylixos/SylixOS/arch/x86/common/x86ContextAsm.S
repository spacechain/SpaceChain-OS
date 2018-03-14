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
;** 文   件   名: x86ContextAsm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2016 年 07 月 04 日
;**
;** 描        述: x86 体系构架上下文切换.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/kernel/kernel_cfg.h>
#include <config/mp/mp_cfg.h>
#include "x86ContextAsm.h"

    FILE_BEGIN()

    IMPORT_LABEL(_SchedSwp)
#if LW_CFG_SMP_EN > 0
    IMPORT_LABEL(_SchedSafeStack)
#endif
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
;  archResumePc
;*********************************************************************************************************/

FUNC_DEF(archResumePc)
    RET
    FUNC_END(archResumePc)

;/*********************************************************************************************************
;  当前 CPU 开始运行多任务执行
;  参数为当前 CPU 控制块, 即 X86_SP_ARG1 为当前 CPU 控制块指针
;  根据 ABI，%EDI 由被调用函数保存，所以使用 %EDI 保存当前 CPU 控制块指针
;*********************************************************************************************************/

FUNC_DEF(archTaskCtxStart)
    MOVL    X86_SP_ARG1(%ESP) , %EDI                                    ;/*  EDI = 当前 CPU 控制块指针   */

    MOVL    0(%EDI) , %EAX                                              ;/*  EAX = 当前 任务 控制块指针  */
    MOVL    0(%EAX) , %ESP                                              ;/*  修改 ESP                    */

    RESTORE_REGS                                                        ;/*  所有寄存器出栈              */
    FUNC_END(archTaskCtxStart)

;/*********************************************************************************************************
;  线程切换 
;  参数为当前 CPU 控制块, 即 X86_SP_ARG1 为当前 CPU 控制块指针
;*********************************************************************************************************/

FUNC_DEF(archTaskCtxSwitch)
    SAVE_REGS                                                           ;/*  所有寄存器入栈              */

    MOVL    (X86_SP_ARG1 + ARCH_REG_CTX_WORD_SIZE * 4)(%ESP) , %EDI     ;/*  EDI = 当前 CPU 控制块指针   */
    MOVL    0(%EDI) , %EAX                                              ;/*  EAX = 当前 任务 控制块指针  */
    MOVL    %ESP , 0(%EAX)                                              ;/*  保存 ESP                    */

#if LW_CFG_SMP_EN > 0
    PUSHL   %EDI                                                        ;/*  参数 = 当前 CPU 控制块指针  */
    CALL    _SchedSafeStack                                             ;/*  _SchedSafeStack();          */
    MOVL    %EAX , %ESP
#endif

    PUSHL   %EDI                                                        ;/*  参数 = 当前 CPU 控制块指针  */
    CALL    _SchedSwp                                                   ;/*  _SchedSwp();                */
    ADDL    $4 , %ESP

    MOVL    0(%EDI) , %EAX                                              ;/*  EAX = 当前 任务 控制块指针  */
    MOVL    0(%EAX) , %ESP                                              ;/*  修改 ESP                    */

    RESTORE_REGS                                                        ;/*  所有寄存器出栈              */
    FUNC_END(archTaskCtxSwitch)

;/*********************************************************************************************************
;  协程切换 
;  参数为当前 CPU 控制块, 即 X86_SP_ARG1 为当前 CPU 控制块指针
;*********************************************************************************************************/

#if LW_CFG_COROUTINE_EN > 0
FUNC_DEF(archCrtCtxSwitch)
    SAVE_REGS                                                           ;/*  所有寄存器入栈              */

    MOVL    (X86_SP_ARG1 + ARCH_REG_CTX_WORD_SIZE * 4)(%ESP) , %EDI     ;/*  EDI = 当前 CPU 控制块指针   */
    MOVL    8(%EDI) , %EAX                                              ;/*  EAX = 当前 任务 控制块指针  */
    MOVL    %ESP , 0(%EAX)                                              ;/*  保存 ESP                    */

    PUSHL   %EDI                                                        ;/*  参数 = 当前 CPU 控制块指针  */
    CALL    _SchedCrSwp                                                 ;/*  _SchedCrSwp();              */
    ADDL    $4 , %ESP

    MOVL    8(%EDI) , %EAX                                              ;/*  EAX = 当前 任务 控制块指针  */
    MOVL    0(%EAX) , %ESP                                              ;/*  修改 ESP                    */

    RESTORE_REGS                                                        ;/*  所有寄存器出栈              */
    FUNC_END(archCrtCtxSwitch)
#endif

;/*********************************************************************************************************
;  中断返回时, 线程装载 
;  参数为当前 CPU 控制块, 即 X86_SP_ARG1 为当前 CPU 控制块指针
;*********************************************************************************************************/

FUNC_DEF(archIntCtxLoad)
    MOVL    (X86_SP_ARG1)(%ESP) , %EDI                                  ;/*  EDI = 当前 CPU 控制块指针   */
    MOVL    0(%EDI) , %EAX                                              ;/*  EAX = 当前 任务 控制块指针  */
    MOVL    0(%EAX) , %ESP                                              ;/*  修改 ESP                    */

    RESTORE_REGS                                                        ;/*  所有寄存器出栈              */
    FUNC_END(archIntCtxLoad)

;/*********************************************************************************************************
;  信号上下文返回
;  参数为需要返回的堆栈栈顶位置
;*********************************************************************************************************/

FUNC_DEF(archSigCtxLoad)
    MOVL    X86_SP_ARG1(%ESP) , %ESP                                    ;/*  ESP = 传入的参数            */

    RESTORE_REGS                                                        ;/*  所有寄存器出栈              */
    FUNC_END(archSigCtxLoad)

    FILE_END()

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
