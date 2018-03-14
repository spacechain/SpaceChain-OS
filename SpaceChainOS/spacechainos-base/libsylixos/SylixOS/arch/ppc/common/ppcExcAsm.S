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
;** 文   件   名: ppcExcAsm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2015 年 12 月 15 日
;**
;** 描        述: PowerPC 体系构架异常处理.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include "ppcContextAsm.h"

    FILE_BEGIN()
    
    IMPORT_LABEL(API_ThreadTcbInter)
    IMPORT_LABEL(API_InterStackBaseGet)
    IMPORT_LABEL(API_InterEnter)
    IMPORT_LABEL(API_InterExit)
    
    IMPORT_LABEL(archMachineCheckExceptionHandle)
    IMPORT_LABEL(archDataStorageExceptionHandle)
    IMPORT_LABEL(archInstructionStorageExceptionHandle)
    IMPORT_LABEL(bspIntHandle)
    IMPORT_LABEL(archAlignmentExceptionHandle)
    IMPORT_LABEL(archProgramExceptionHandle)
    IMPORT_LABEL(archFpuUnavailableExceptionHandle)
    IMPORT_LABEL(archDecrementerInterruptHandle)
    IMPORT_LABEL(archSystemCallHandle)
    IMPORT_LABEL(archTraceHandle)
    IMPORT_LABEL(archFpAssistExceptionHandle)
    
    EXPORT_LABEL(archMachineCheckExceptionEntry)
    EXPORT_LABEL(archDataStorageExceptionEntry)
    EXPORT_LABEL(archInstructionStorageExceptionEntry)
    EXPORT_LABEL(archExternalInterruptEntry)
    EXPORT_LABEL(archAlignmentExceptionEntry)
    EXPORT_LABEL(archProgramExceptionEntry)
    EXPORT_LABEL(archFpuUnavailableExceptionEntry)
    EXPORT_LABEL(archDecrementerInterruptEntry)
    EXPORT_LABEL(archSystemCallEntry)
    EXPORT_LABEL(archTraceEntry)
    EXPORT_LABEL(archFpAssistExceptionEntry)
    
;/*********************************************************************************************************
;  异常入口宏
;*********************************************************************************************************/

MACRO_DEF(EXC_ENTRY, handle)
    EXC_SAVE_REGS                                                       ;/*  所有寄存器入栈              */

    ;/*
    ; *  if (API_InterEnter() == 1) {
    ; *      current stack = SP;
    ; *      SP = interrupt stack;
    ; *  }
    ; */
    SUBI    SP , SP , 16                                                ;/*  开辟 C 函数栈帧             */
    BL      API_InterEnter                                              ;/*  获取中断层数放到返回值 V0   */

    CMPWI   CR0, R3 , 1
    BNE     CR0, 1f

    BL      API_ThreadTcbInter                                          ;/*  get current tcb             */
    ADDI    SP , SP , 16
    STW     SP , 0(R3)                                                  ;/*  current stack = SP          */
    SUBI    SP , SP , 16                                                ;/*  开辟 C 函数栈帧             */

    BL      API_InterStackBaseGet                                       ;/*  获得当前 CPU 中断堆栈栈顶   */
    OR      SP , R3 , R3
    SUBI    SP , SP , 16                                                ;/*  开辟 C 函数栈帧             */

1:
    MFSPR   R3 , SRR0                                                   ;/*  传递返回地址                */
    ISYNC
    BL      \handle                                                     ;/*  调用处理函数                */

    BL      API_InterExit
    ADDI    SP , SP , 16

    RESTORE_REGS                                                        ;/*  所有寄存器出栈              */
    MACRO_END()

;/*********************************************************************************************************
;  机器检查异常入口(Vector 0x200)
;*********************************************************************************************************/

FUNC_DEF(archMachineCheckExceptionEntry)
    EXC_ENTRY   archMachineCheckExceptionHandle
    FUNC_END()

;/*********************************************************************************************************
;  数据存储异常入口(Vector 0x300)
;*********************************************************************************************************/

FUNC_DEF(archDataStorageExceptionEntry)
    EXC_ENTRY   archDataStorageExceptionHandle
    FUNC_END()

;/*********************************************************************************************************
;  指令访问异常入口(Vector 0x400)
;*********************************************************************************************************/

FUNC_DEF(archInstructionStorageExceptionEntry)
    EXC_ENTRY   archInstructionStorageExceptionHandle
    FUNC_END()

;/*********************************************************************************************************
;  外部中断入口(Vector 0x500)
;*********************************************************************************************************/

FUNC_DEF(archExternalInterruptEntry)
    EXC_ENTRY   bspIntHandle
    FUNC_END()

;/*********************************************************************************************************
;  非对齐异常入口(Vector 0x600)
;*********************************************************************************************************/

FUNC_DEF(archAlignmentExceptionEntry)
    EXC_ENTRY   archAlignmentExceptionHandle
    FUNC_END()

;/*********************************************************************************************************
;  程序异常入口(Vector 0x700)
;*********************************************************************************************************/

FUNC_DEF(archProgramExceptionEntry)
    EXC_ENTRY   archProgramExceptionHandle
    FUNC_END()

;/*********************************************************************************************************
;  FPU 不可用异常入口(Vector 0x800)
;*********************************************************************************************************/

FUNC_DEF(archFpuUnavailableExceptionEntry)
    EXC_ENTRY   archFpuUnavailableExceptionHandle
    FUNC_END()

;/*********************************************************************************************************
;  Decrementer 中断入口(Vector 0x900)
;*********************************************************************************************************/

FUNC_DEF(archDecrementerInterruptEntry)
    EXC_ENTRY   archDecrementerInterruptHandle
    FUNC_END()

;/*********************************************************************************************************
;  系统调用入口(Vector 0xC00)
;*********************************************************************************************************/

FUNC_DEF(archSystemCallEntry)
    EXC_ENTRY   archSystemCallHandle
    FUNC_END()

;/*********************************************************************************************************
;  Trace 入口(Vector 0xD00)
;*********************************************************************************************************/

FUNC_DEF(archTraceEntry)
    EXC_ENTRY   archTraceHandle
    FUNC_END()

;/*********************************************************************************************************
;  Floating-Point Assist 异常入口(Vector 0xE00)
;*********************************************************************************************************/

FUNC_DEF(archFpAssistExceptionEntry)
    EXC_ENTRY   archFpAssistExceptionHandle
    FUNC_END()

    FILE_END()

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
