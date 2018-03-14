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
;** 文   件   名: armContextAsm.S
;**
;** 创   建   人: Han.Hui (韩辉)
;**
;** 文件创建日期: 2013 年 12 月 07 日
;**
;** 描        述: ARM 体系构架上下文切换.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/kernel/kernel_cfg.h>
#include <config/mp/mp_cfg.h>

#if !defined(__SYLIXOS_ARM_ARCH_M__)

    FILE_BEGIN()

;/*********************************************************************************************************
;  ARM 体系构架相关异常处理函数
;*********************************************************************************************************/

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
;  当前 CPU 开始运行多任务执行
;  参数为当前 CPU 控制块, 即 R0 为当前 CPU 控制块指针
;*********************************************************************************************************/

FUNC_DEF(archTaskCtxStart)
    LDR     R4 , [R0]                                                   ;/*  获取该任务堆栈栈顶地址      */
    LDR     SP , [R4]                                                   ;/*  堆栈指针赋值                */
    
    LDR     R4 , [SP], #4                                               ;/*  CPSR 出栈                   */
    MSR     SPSR_cxsf, R4
    LDMFD   SP!, {R0-R12, LR, PC}^                                      ;/*  包括 PC 的所有寄存器出栈,   */
                                                                        ;/*  同时更新 CPSR               */
    FUNC_END()

;/*********************************************************************************************************
;  线程切换 
;  参数为当前 CPU 控制块, 即 R0 为当前 CPU 控制块指针
;*********************************************************************************************************/

FUNC_DEF(archTaskCtxSwitch)
    STMFD   SP!, {LR}                                                   ;/*  任务返回 PC 值入栈          */
    STMFD   SP!, {LR}
    STMFD   SP!, {R0-R12}                                               ;/*  Push registers              */
    MRS     R4 , CPSR                                                   ;/*  CPSR 入栈                   */
    STMFD   SP!, {R4}

    MOV     R4 , R0                                                     ;/*  stack current = SP          */
    LDR     R5 , [R4]
    STR     SP , [R5]

#if LW_CFG_SMP_EN > 0
    MOV     R9 , R0
    LDR     R1 , =_SchedSafeStack                                       ;/*  _SchedSafeStack();          */
    MOV     LR , PC
    BX      R1
    MOV     SP , R0
    MOV     R0 , R9
#endif

    STMFD   SP!, {R0}                                                   ;/*  当前 CPU 信息入栈           */
    LDR     R1 , =_SchedSwp                                             ;/*  _SchedSwp();                */
    MOV     LR , PC
    BX      R1
    LDMFD   SP!, {R0}                                                   ;/*  当前 CPU 信息出栈           */

    MOV     R4 , R0                                                     ;/*  SP = stack current          */
    LDR     R5 , [R4]
    LDR     SP , [R5]

    LDMFD   SP!, {R4}                                                   ;/*  CPSR 出栈                   */
    MSR     SPSR_cxsf, R4
    LDMFD   SP!, {R0-R12, LR, PC}^                                      ;/*  包括 PC 的所有寄存器出栈,   */
                                                                        ;/*  同时更新 CPSR               */
    FUNC_END()

;/*********************************************************************************************************
;  协程切换 
;  参数为当前 CPU 控制块, 即 R0 为当前 CPU 控制块指针
;*********************************************************************************************************/

#if LW_CFG_COROUTINE_EN > 0
FUNC_DEF(archCrtCtxSwitch)
    STMFD   SP!, {LR}                                                   ;/*  任务返回 PC 值入栈          */
    STMFD   SP!, {LR}
    STMFD   SP!, {R0-R12}                                               ;/*  Push registers              */
    MRS     R4 , CPSR                                                   ;/*  CPSR 入栈                   */
    STMFD   SP!, {R4}

    ADD     R4 , R0 , #8                                                ;/*  get current crcb            */
    LDR     R5 , [R4]                                                   ;/*  stack current = SP          */
    STR     SP , [R5]

    STMFD   SP!, {R0}                                                   ;/*  当前 CPU 信息入栈           */
    LDR     R1 , =_SchedCrSwp                                           ;/*  _SchedCrSwp();              */
    MOV     LR , PC
    BX      R1
    LDMFD   SP!, {R0}                                                   ;/*  当前 CPU 信息出栈           */

    ADD     R4 , R0 , #8                                                ;/*  get current crcb            */
    LDR     R5 , [R4]                                                   ;/*  SP = stack next crcb        */
    LDR     SP , [R5]

    LDMFD   SP!, {R4}                                                   ;/*  CPSR 出栈                   */
    MSR     SPSR_cxsf, R4
    LDMFD   SP!, {R0-R12, LR, PC}^                                      ;/*  包括 PC 的所有寄存器出栈,   */
                                                                        ;/*  同时更新 CPSR               */
    FUNC_END()
#endif

;/*********************************************************************************************************
;  中断返回时, 线程装载 
;  参数为当前 CPU 控制块, 即 R0 为当前 CPU 控制块指针
;*********************************************************************************************************/

FUNC_DEF(archIntCtxLoad)
    MOV     R4 , R0                                                     ;/*  SP = stack current          */
    LDR     R5 , [R4]
    LDR     SP , [R5]

    LDMFD   SP!, {R4}                                                   ;/*  CPSR 出栈                   */
    MSR     SPSR_cxsf, R4
    LDMFD   SP!, {R0-R12, LR, PC}^                                      ;/*  包括 PC 的所有寄存器出栈,   */
                                                                        ;/*  同时更新 CPSR               */
    FUNC_END()

;/*********************************************************************************************************
;  信号上下文返回
;  参数为需要返回的堆栈栈顶位置
;*********************************************************************************************************/

FUNC_DEF(archSigCtxLoad)
    MOV     SP , R0                                                     ;/*  返回的堆栈指针              */
    
    LDMFD   SP!, {R4}                                                   ;/*  CPSR 出栈                   */
    MSR     SPSR_cxsf, R4
    LDMFD   SP!, {R0-R12,LR,PC}^                                        ;/*  包括 PC 的所有寄存器出栈,   */
                                                                        ;/*  同时更新 CPSR               */
    FUNC_END()

    FILE_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
