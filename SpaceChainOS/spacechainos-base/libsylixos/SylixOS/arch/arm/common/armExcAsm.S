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
;** 文   件   名: armExcAsm.S
;**
;** 创   建   人: Han.Hui (韩辉)
;**
;** 文件创建日期: 2013 年 12 月 07 日
;**
;** 描        述: ARM 体系构架异常处理.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>

#if !defined(__SYLIXOS_ARM_ARCH_M__)

    FILE_BEGIN()

;/*********************************************************************************************************
;  ARM 体系构架相关异常处理函数
;*********************************************************************************************************/

    IMPORT_LABEL(API_ThreadTcbInter)
    IMPORT_LABEL(API_InterStackBaseGet)
    IMPORT_LABEL(API_InterEnter)
    IMPORT_LABEL(API_InterExit)
    
    IMPORT_LABEL(archAbtHandle)
    IMPORT_LABEL(archUndHandle)
    IMPORT_LABEL(archSwiHandle)
    IMPORT_LABEL(bspIntHandle)
    
    EXPORT_LABEL(archIntEntry)
    EXPORT_LABEL(archAbtEntry)
    EXPORT_LABEL(archPreEntry)
    EXPORT_LABEL(archUndEntry)
    EXPORT_LABEL(archSwiEntry)
    
;/*********************************************************************************************************
;  中断入口
;*********************************************************************************************************/

MACRO_DEF(IRQ_ENTRY)
    STMFD   SP!, {R1-R3}                                                ;/*  寄存器压入 IRQ 模式堆栈     */
    MOV     R1 , SP                                                     ;/*  保存 IRQ 堆栈指针           */
    ADD     SP , SP, #12                                                ;/*  调整 IRQ 堆栈指针           */
    SUB     R2 , LR, #4                                                 ;/*  调整用于中断返回的 PC 值    */
    MRS     R3 , SPSR                                                   ;/*  将 SPSR_irq 拷贝到 R3       */
    MSR     CPSR_c, #(DIS_INT | SVC32_MODE)                             ;/*  回到 SVC 模式               */
    
    STMFD   SP!, {R2}                                                   ;/*  任务返回地址压栈            */
    STMFD   SP!, {LR}                                                   ;/*  LR 压栈                     */
    STMFD   SP!, {R4-R12}                                               ;/*  R12 - R4 压栈               */
    
    LDMFD   R1!, {R4-R6}                                                ;/*  将任务寄存器 R1 - R3 从     */
                                                                        ;/*  IRQ 堆栈拷贝到 SVC 堆栈     */
    STMFD   SP!, {R4-R6}
    STMFD   SP!, {R0}                                                   ;/*  R0 压栈                     */
    STMFD   SP!, {R3}                                                   ;/*  CPSR 压栈                   */
    
    ;/*
    ; *  if (API_InterEnter() == 1) {
    ; *      current stack = SP;
    ; *      SP = interrupt stack;
    ; *  }
    ; */
    LDR     R0 , =API_InterEnter
    MOV     LR , PC
    BX      R0
    CMP     R0 , #1
    BNE     1f
    
    LDR     R0 , =API_ThreadTcbInter                                    ;/*  get current tcb             */
    MOV     LR , PC
    BX      R0
    STR     SP , [R0]                                                   ;/*  current stack = SP          */
        
    LDR     R0 , =API_InterStackBaseGet                                 ;/*  获得当前 CPU 中断堆栈栈顶   */
    MOV     LR , PC
    BX      R0
    MOV     SP , R0
    
1:
    LDR     R5 , =bspIntHandle
    MOV     LR , PC
    BX      R5
    
    LDR     R0 , =API_InterExit
    MOV     LR , PC
    BX      R0
    
    LDMFD   SP!, {R4}
    MSR     SPSR_cxsf, R4
    LDMFD   SP!, {R0-R12, LR, PC}^
    MACRO_END()
    
FUNC_DEF(archIntEntry)
    IRQ_ENTRY
    FUNC_END()
    
;/*********************************************************************************************************
;  abort 入口 
;  data_abort     $retoff == 8
;  prefetch_abort $retoff == 4
;*********************************************************************************************************/

MACRO_DEF(ABT_ENTRY retoff)
    STMFD   SP!, {R1-R3}                                                ;/*  寄存器压入 abt 模式堆栈     */
    MOV     R1 , SP                                                     ;/*  保存 abt 堆栈指针           */
    ADD     SP , SP, #12                                                ;/*  调整 abt 堆栈指针           */
    SUB     R2 , LR, #\retoff                                           ;/*  调整用于中断返回的 PC 值    */
    MRS     R3 , SPSR                                                   ;/*  将 SPSR 拷贝到 R3           */
    MSR     CPSR_c, #(DIS_INT | SVC32_MODE)                             ;/*  回到 SVC 模式               */
    
    STMFD   SP!, {R2}                                                   ;/*  任务返回地址压栈            */
    STMFD   SP!, {LR}                                                   ;/*  LR 压栈                     */
    STMFD   SP!, {R4-R12}                                               ;/*  R12 - R4 压栈               */
    
    LDMFD   R1!, {R4-R6}                                                ;/*  将任务寄存器 R1 - R3 从     */
                                                                        ;/*  IRQ 堆栈拷贝到 SVC 堆栈     */
    STMFD   SP!, {R4-R6}
    STMFD   SP!, {R0}                                                   ;/*  R0 压栈                     */
    STMFD   SP!, {R3}                                                   ;/*  CPSR 压栈                   */
    
    ;/*
    ; *  if (API_InterEnter() == 1) {
    ; *      current stack = SP;
    ; *      SP = interrupt stack;
    ; *  }
    ; */
    LDR     R0 , =API_InterEnter
    MOV     LR , PC
    BX      R0
    LDR     R8 , [SP , #(15 * 4)]                                       ;/*  R8 -> LR 地址               */
    CMP     R0 , #1
    BNE     1f
    
    LDR     R0 , =API_ThreadTcbInter                                    ;/*  get current tcb             */
    MOV     LR , PC
    BX      R0
    STR     SP , [R0]                                                   ;/*  current stack = SP          */
    
    LDR     R0 , =API_InterStackBaseGet                                 ;/*  获得当前 CPU 中断堆栈栈顶   */
    MOV     LR , PC
    BX      R0
    MOV     SP , R0
    
1:
    MOV     R0 , R8                                                     ;/*  R8 -> LR 地址               */
    MOV     R1 , #\retoff                                               ;/*  $retoff                     */
    LDR     R5 , =archAbtHandle
    MOV     LR , PC
    BX      R5
    
    LDR     R0 , =API_InterExit
    MOV     LR , PC
    BX      R0
    
    LDMFD   SP!, {R4}
    MSR     SPSR_cxsf, R4
    LDMFD   SP!, {R0-R12, LR, PC}^
    MACRO_END()
    
FUNC_DEF(archAbtEntry)
    ABT_ENTRY 8
    FUNC_END()

FUNC_DEF(archPreEntry)
    ABT_ENTRY 4
    FUNC_END()

;/*********************************************************************************************************
;  undefine 入口
;*********************************************************************************************************/

MACRO_DEF(UND_ENTRY)
    STMFD   SP!, {R1-R3}                                                ;/*  寄存器压入 und 模式堆栈     */
    MOV     R1 , SP                                                     ;/*  保存 und 堆栈指针           */
    ADD     SP , SP, #12                                                ;/*  调整 und 堆栈指针           */
    MRS     R3 , SPSR                                                   ;/*  将 SPSR 拷贝到 R3           */
    TST     R3 , #0x20                                                  ;/*  是否为 Thumb 模式           */
    SUBNE   R2 , LR, #2                                                 ;/*  计算 Thumb 模式返回位置     */
    SUBEQ   R2 , LR, #4                                                 ;/*  计算 ARM 模式返回位置       */
    MSR     CPSR_c, #(DIS_INT | SVC32_MODE)                             ;/*  回到 SVC 模式               */
    
    STMFD   SP!, {R2}                                                   ;/*  任务返回地址压栈            */
    STMFD   SP!, {LR}                                                   ;/*  LR 压栈                     */
    STMFD   SP!, {R4-R12}                                               ;/*  R12 - R4 压栈               */
    
    LDMFD   R1!, {R4-R6}                                                ;/*  将任务寄存器 R1 - R3 从     */
                                                                        ;/*  IRQ 堆栈拷贝到 SVC 堆栈     */
    STMFD   SP!, {R4-R6}
    STMFD   SP!, {R0}                                                   ;/*  R0 压栈                     */
    STMFD   SP!, {R3}                                                   ;/*  CPSR 压栈                   */
    
    ;/*
    ; *  if (API_InterEnter() == 1) {
    ; *      current stack = SP;
    ; *      SP = interrupt stack;
    ; *  }
    ; */
    LDR     R0 , =API_InterEnter
    MOV     LR , PC
    BX      R0
    LDR     R8 , [SP, #(15 * 4)]                                        ;/*  参数为产生未定义指令中断地址*/
    LDR     R9 , [SP]                                                   ;/*  产生异常的 CPSR             */
    CMP     R0 , #1
    BNE     1f
    
    LDR     R0 , =API_ThreadTcbInter                                    ;/*  get current tcb             */
    MOV     LR , PC
    BX      R0
    STR     SP , [R0]                                                   ;/*  current stack = SP          */
    
    LDR     R0 , =API_InterStackBaseGet                                 ;/*  获得当前 CPU 中断堆栈栈顶   */
    MOV     LR , PC
    BX      R0
    MOV     SP , R0
    
1:
    MOV     R0 , R8
    MOV     R1 , R9
    LDR     R5 , =archUndHandle
    MOV     LR , PC
    BX      R5
    
    LDR     R0 , =API_InterExit
    MOV     LR , PC
    BX      R0
    
    LDMFD   SP!, {R4}
    MSR     SPSR_cxsf, R4
    LDMFD   SP!, {R0-R12, LR, PC}^
    MACRO_END()
    
FUNC_DEF(archUndEntry)
    UND_ENTRY
    FUNC_END()
    
;/*********************************************************************************************************
;  swi 入口
;*********************************************************************************************************/

MACRO_DEF(SWI_ENTRY)
    STMFD   SP!, {R0-R12, LR}

    LDR     R0 , [LR, #-4]                                              ;/*  读取 SWI 指令               */
    BIC     R0 , R0, #0xff000000                                        ;/*  获得 SWI 指令号             */
    MOV     R1 , SP                                                     ;/*  获得参数表位置              */

    LDR     R5 , =archSwiHandle
    MOV     LR , PC
    BX      R5

    LDMFD   SP!, {R0-R12, PC}^                                          ;/*  SWI 返回                    */
    MACRO_END()

FUNC_DEF(archSwiEntry)
    SWI_ENTRY
    FUNC_END()

    FILE_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
