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
;** 文   件   名: ppcExcE500Asm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2016 年 05 月 04 日
;**
;** 描        述: PowerPC E500 体系构架异常处理.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include "arch/ppc/arch_e500.h"
#include "../ppcContextAsm.h"
#include "ppcContextE500Asm.h"

    FILE_BEGIN()

    IMPORT_LABEL(API_ThreadTcbInter)
    IMPORT_LABEL(API_InterStackBaseGet)
    IMPORT_LABEL(API_InterEnter)
    IMPORT_LABEL(API_InterExit)
    IMPORT_LABEL(bspIntHandle)

    IMPORT_LABEL(archE500CriticalInputExceptionHandle)
    IMPORT_LABEL(archE500MachineCheckExceptionHandle)
    IMPORT_LABEL(archE500DataStorageExceptionHandle)
    IMPORT_LABEL(archE500InstructionStorageExceptionHandle)
    IMPORT_LABEL(archE500AlignmentExceptionHandle)
    IMPORT_LABEL(archE500ProgramExceptionHandle)
    IMPORT_LABEL(archE500FpuUnavailableExceptionHandle)
    IMPORT_LABEL(archE500SystemCallHandle)
    IMPORT_LABEL(archE500ApUnavailableExceptionHandle)
    IMPORT_LABEL(archE500DecrementerInterruptHandle)
    IMPORT_LABEL(archE500TimerInterruptHandle)
    IMPORT_LABEL(archE500WatchdogInterruptHandle)
#if LW_CFG_VMM_EN > 0
    IMPORT_LABEL(archE500MmuDataTlbErrorHandle)
    IMPORT_LABEL(archE500MmuInstructionTlbErrorHandle)
#else
    IMPORT_LABEL(archE500DataTLBErrorHandle)
    IMPORT_LABEL(archE500InstructionTLBErrorHandle)
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
    IMPORT_LABEL(archE500DebugExceptionHandle)
    IMPORT_LABEL(archE500SpeUnavailableExceptionHandle)
    IMPORT_LABEL(archE500FpDataExceptionHandle)
    IMPORT_LABEL(archE500FpRoundExceptionHandle)
    IMPORT_LABEL(archE500AltiVecUnavailableExceptionHandle)
    IMPORT_LABEL(archE500AltiVecAssistExceptionHandle)
    IMPORT_LABEL(archE500PerfMonitorExceptionHandle)
    IMPORT_LABEL(archE500DoorbellExceptionHandle)
    IMPORT_LABEL(archE500DoorbellCriticalExceptionHandle)

    EXPORT_LABEL(archE500CriticalInputExceptionEntry)
    EXPORT_LABEL(archE500MachineCheckExceptionEntry)
    EXPORT_LABEL(archE500DataStorageExceptionEntry)
    EXPORT_LABEL(archE500InstructionStorageExceptionEntry)
    EXPORT_LABEL(archE500ExternalInterruptEntry)
    EXPORT_LABEL(archE500AlignmentExceptionEntry)
    EXPORT_LABEL(archE500ProgramExceptionEntry)
    EXPORT_LABEL(archE500FpuUnavailableExceptionEntry)
    EXPORT_LABEL(archE500SystemCallEntry)
    EXPORT_LABEL(archE500ApUnavailableExceptionEntry)
    EXPORT_LABEL(archE500DecrementerInterruptEntry)
    EXPORT_LABEL(archE500TimerInterruptEntry)
    EXPORT_LABEL(archE500WatchdogInterruptEntry)
    EXPORT_LABEL(archE500DataTLBErrorEntry)
    EXPORT_LABEL(archE500InstructionTLBErrorEntry)
    EXPORT_LABEL(archE500DebugExceptionEntry)
    EXPORT_LABEL(archE500SpeUnavailableExceptionEntry)
    EXPORT_LABEL(archE500FpDataExceptionEntry)
    EXPORT_LABEL(archE500FpRoundExceptionEntry)
    EXPORT_LABEL(archE500AltiVecUnavailableExceptionEntry)
    EXPORT_LABEL(archE500AltiVecAssistExceptionEntry)
    EXPORT_LABEL(archE500PerfMonitorExceptionEntry)
    EXPORT_LABEL(archE500DoorbellExceptionEntry)
    EXPORT_LABEL(archE500DoorbellCriticalExceptionEntry)

;/*********************************************************************************************************
;  异常处理程序起始地址需要 16 字节对齐方能填入 IVOR 寄存器
;*********************************************************************************************************/

#ifdef __GNUC__
#define EXCE_DEF(func)  \
        .balign 16;     \
        .type func, %function;  \
func:

#define EXCE_END()
#else                                                                   /*  __GNUC__                    */
#  error "NOT SUPPORT!"
#endif                                                                  /*  !__GNUC__                   */

;/*********************************************************************************************************
;  异常处理程序都放在 .vector 节区
;*********************************************************************************************************/

    SECTION(.vector)

;/*********************************************************************************************************
;  异常入口宏
;  注意: E500 有 3 类异常: 普通异常, 机器检查异常, 临界输入异常(包括调试异常和看门狗中断及门铃临界异常)
;  这 3 类异常使用的寄存器不一样
;  普通异常使用     SRR0   SRR1
;  机器检查异常使用 MCSRR0 MCSRR1
;  临界输入异常使用 CSRR0  CSRR1
;  所以定义了 3 个异常入口宏
;*********************************************************************************************************/

MACRO_DEF(E500_EXC_ENTRY, handle)
    E500_EXC_SAVE_REGS                                                  ;/*  所有寄存器入栈              */

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
;  临界输入异常入口宏
;*********************************************************************************************************/

MACRO_DEF(E500_CI_EXC_ENTRY, handle)
    E500_CI_EXC_SAVE_REGS                                               ;/*  所有寄存器入栈              */

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
    MFSPR   R3 , CSRR0                                                  ;/*  传递返回地址                */
    ISYNC
    BL      \handle                                                     ;/*  调用处理函数                */

    BL      API_InterExit
    ADDI    SP , SP , 16

    RESTORE_REGS                                                        ;/*  所有寄存器出栈              */
    MACRO_END()

;/*********************************************************************************************************
;  机器检查异常入口宏
;*********************************************************************************************************/

MACRO_DEF(E500_MC_EXC_ENTRY, handle)
    E500_MC_EXC_SAVE_REGS                                               ;/*  所有寄存器入栈              */

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
    MFSPR   R3 , MCSRR0                                                 ;/*  传递返回地址                */
    ISYNC
    BL      \handle                                                     ;/*  调用处理函数                */

    BL      API_InterExit
    ADDI    SP , SP , 16

    RESTORE_REGS                                                        ;/*  所有寄存器出栈              */
    MACRO_END()

;/*********************************************************************************************************
;  Embedded Category–Defined IVORs
;*********************************************************************************************************/

;/*********************************************************************************************************
;  临界输入异常入口(Vector IVOR0)
;*********************************************************************************************************/

EXCE_DEF(archE500CriticalInputExceptionEntry)
    E500_CI_EXC_ENTRY   archE500CriticalInputExceptionHandle
    EXCE_END()

;/*********************************************************************************************************
;  机器检查异常入口(Vector IVOR1)
;*********************************************************************************************************/

EXCE_DEF(archE500MachineCheckExceptionEntry)
    E500_MC_EXC_ENTRY   archE500MachineCheckExceptionHandle
    EXCE_END()

;/*********************************************************************************************************
;  数据存储异常入口(Vector IVOR2)
;*********************************************************************************************************/

EXCE_DEF(archE500DataStorageExceptionEntry)
    E500_EXC_ENTRY      archE500DataStorageExceptionHandle
    EXCE_END()

;/*********************************************************************************************************
;  指令访问异常入口(Vector IVOR3)
;*********************************************************************************************************/

EXCE_DEF(archE500InstructionStorageExceptionEntry)
    E500_EXC_ENTRY      archE500InstructionStorageExceptionHandle
    EXCE_END()

;/*********************************************************************************************************
;  外部中断入口(Vector IVOR4)
;*********************************************************************************************************/

EXCE_DEF(archE500ExternalInterruptEntry)
    E500_EXC_ENTRY      bspIntHandle
    EXCE_END()

;/*********************************************************************************************************
;  非对齐异常入口(Vector IVOR5)
;*********************************************************************************************************/

EXCE_DEF(archE500AlignmentExceptionEntry)
    E500_EXC_ENTRY      archE500AlignmentExceptionHandle
    EXCE_END()

;/*********************************************************************************************************
;  程序异常入口(Vector IVOR6)
;*********************************************************************************************************/

EXCE_DEF(archE500ProgramExceptionEntry)
    E500_EXC_ENTRY      archE500ProgramExceptionHandle
    EXCE_END()

;/*********************************************************************************************************
;  FPU 不可用异常入口(Vector IVOR7)
;*********************************************************************************************************/

EXCE_DEF(archE500FpuUnavailableExceptionEntry)
    E500_EXC_ENTRY      archE500FpuUnavailableExceptionHandle
    FUNC_END()

;/*********************************************************************************************************
;  系统调用入口(Vector IVOR8)
;*********************************************************************************************************/

EXCE_DEF(archE500SystemCallEntry)
    E500_EXC_ENTRY      archE500SystemCallHandle
    EXCE_END()

;/*********************************************************************************************************
;  AP 不可用 (not defined by Embedded Implementation Standards)用入口(Vector IVOR9)
;*********************************************************************************************************/

EXCE_DEF(archE500ApUnavailableExceptionEntry)
    E500_EXC_ENTRY      archE500ApUnavailableExceptionHandle
    EXCE_END()

;/*********************************************************************************************************
;  Decrementer 中断入口(Vector IVOR10)
;*********************************************************************************************************/

EXCE_DEF(archE500DecrementerInterruptEntry)
    E500_EXC_ENTRY      archE500DecrementerInterruptHandle
    EXCE_END()

;/*********************************************************************************************************
;  Fixed-interval timer 中断入口(Vector IVOR11)
;*********************************************************************************************************/

EXCE_DEF(archE500TimerInterruptEntry)
    E500_EXC_ENTRY      archE500TimerInterruptHandle
    EXCE_END()

;/*********************************************************************************************************
;  Watchdog timer 中断入口(Vector IVOR12)
;*********************************************************************************************************/

EXCE_DEF(archE500WatchdogInterruptEntry)
    E500_CI_EXC_ENTRY   archE500WatchdogInterruptHandle
    EXCE_END()

;/*********************************************************************************************************
;  数据 TLB 错误异常入口(Vector IVOR13)
;*********************************************************************************************************/

EXCE_DEF(archE500DataTLBErrorEntry)
#if LW_CFG_VMM_EN > 0
    E500_EXC_ENTRY      archE500MmuDataTlbErrorHandle
#else
    E500_EXC_ENTRY      archE500DataTLBErrorHandle
#endif                                                                  /*  LW_CFG_VMM_EN > 0            */
    EXCE_END()

;/*********************************************************************************************************
;  指令 TLB 错误异常入口(Vector IVOR14)
;*********************************************************************************************************/

EXCE_DEF(archE500InstructionTLBErrorEntry)
#if LW_CFG_VMM_EN > 0
    E500_EXC_ENTRY      archE500MmuInstructionTlbErrorHandle
#else
    E500_EXC_ENTRY      archE500InstructionTLBErrorHandle
#endif                                                                  /*  LW_CFG_VMM_EN > 0            */
    EXCE_END()

;/*********************************************************************************************************
;  Debug 异常入口(Vector IVOR15)
;*********************************************************************************************************/

EXCE_DEF(archE500DebugExceptionEntry)
    E500_CI_EXC_ENTRY   archE500DebugExceptionHandle
    EXCE_END()

;/*********************************************************************************************************
;  e500-Specific IVORs
;*********************************************************************************************************/

;/*********************************************************************************************************
;  SPE 不可用异常入口(Vector IVOR32)
;*********************************************************************************************************/

EXCE_DEF(archE500SpeUnavailableExceptionEntry)
    E500_EXC_ENTRY      archE500SpeUnavailableExceptionHandle
    EXCE_END()

;/*********************************************************************************************************
;  SPE floating-point data 异常入口(Vector IVOR33)
;*********************************************************************************************************/

EXCE_DEF(archE500FpDataExceptionEntry)
    E500_EXC_ENTRY      archE500FpDataExceptionHandle
    EXCE_END()

;/*********************************************************************************************************
;  SPE floating-point round 异常入口(Vector IVOR34)
;*********************************************************************************************************/

EXCE_DEF(archE500FpRoundExceptionEntry)
    E500_EXC_ENTRY      archE500FpRoundExceptionHandle
    EXCE_END()

;/*********************************************************************************************************
;  AltiVec 不可用异常入口(Vector IVOR32)
;*********************************************************************************************************/

EXCE_DEF(archE500AltiVecUnavailableExceptionEntry)
    E500_EXC_ENTRY      archE500AltiVecUnavailableExceptionHandle
    EXCE_END()

;/*********************************************************************************************************
;  AltiVec Assist 异常入口(Vector IVOR33)
;*********************************************************************************************************/

EXCE_DEF(archE500AltiVecAssistExceptionEntry)
    E500_EXC_ENTRY      archE500AltiVecAssistExceptionHandle
    EXCE_END()

;/*********************************************************************************************************
;  Performance monitor 异常入口(Vector IVOR35)
;*********************************************************************************************************/

EXCE_DEF(archE500PerfMonitorExceptionEntry)
    E500_EXC_ENTRY      archE500PerfMonitorExceptionHandle
    EXCE_END()

;/*********************************************************************************************************
;  Processor doorbell 异常入口(Vector IVOR36)
;*********************************************************************************************************/

EXCE_DEF(archE500DoorbellExceptionEntry)
    E500_EXC_ENTRY      archE500DoorbellExceptionHandle
    EXCE_END()

;/*********************************************************************************************************
;  Processor doorbell critical 异常入口(Vector IVOR37)
;*********************************************************************************************************/

EXCE_DEF(archE500DoorbellCriticalExceptionEntry)
    E500_CI_EXC_ENTRY   archE500DoorbellCriticalExceptionHandle
    EXCE_END()

    FILE_END()

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
