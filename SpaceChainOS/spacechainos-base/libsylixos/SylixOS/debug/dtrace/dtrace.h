/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: dtrace.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2011 年 11 月 18 日
**
** 描        述: SylixOS 调试跟踪器, GDB server 可以使用此调试进程.
*********************************************************************************************************/

#ifndef __DTRACE_H
#define __DTRACE_H

#include "signal.h"

/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_GDB_EN > 0

/*********************************************************************************************************
  是否使用硬件单步
*********************************************************************************************************/

#ifdef LW_CFG_CPU_ARCH_X86
#define LW_DTRACE_HW_ISTEP  1                                           /*  支持硬件单步断点            */
#endif                                                                  /*  LW_CFG_CPU_ARCH_X86         */

/*********************************************************************************************************
  创建 dtrace 的类型
*********************************************************************************************************/

#define LW_DTRACE_THREAD    0                                           /*  仅暂停线程                  */
#define LW_DTRACE_PROCESS   1                                           /*  仅暂停进程                  */
#define LW_DTRACE_SYSTEM    2                                           /*  暂停整个系统 (当前未支持)   */

/*********************************************************************************************************
  创建 dtrace flags
*********************************************************************************************************/

#define LW_DTRACE_F_DEF     0x00
#define LW_DTRACE_F_KBP     0x01                                        /*  内核断点使能                */

/*********************************************************************************************************
  LW_DTRACE_MSG 类型 (LW_TRAP_RETRY 为内部使用)
*********************************************************************************************************/

#define LW_TRAP_INVAL       0                                           /*  无效                        */
#define LW_TRAP_RETRY       1                                           /*  虚假断点                    */

#define LW_TRAP_BRKPT       2                                           /*  程序断点                    */
#define LW_TRAP_ABORT       3                                           /*  程序异常终止                */
#define LW_TRAP_ISTEP       4                                           /*  硬件指令单步断点            */
#define LW_TRAP_QUIT        5                                           /*  进程退出                    */
#define LW_TRAP_WATCH       LW_TRAP_BRKPT                               /*  观察点 (暂不支持)           */

/*********************************************************************************************************
  断点消息
*********************************************************************************************************/

typedef struct {
    addr_t              DTM_ulAddr;                                     /*  断点地址                    */
    UINT                DTM_uiType;                                     /*  停止类型                    */
    LW_OBJECT_HANDLE    DTM_ulThread;                                   /*  执行到断点的线程            */
} LW_DTRACE_MSG;
typedef LW_DTRACE_MSG  *PLW_DTRACE_MSG;

/*********************************************************************************************************
  API
*********************************************************************************************************/

LW_API PVOID    API_DtraceCreate(UINT  uiType, UINT  uiFlag, LW_OBJECT_HANDLE  ulDbger);
LW_API ULONG    API_DtraceDelete(PVOID  pvDtrace);
LW_API BOOL     API_DtraceIsValid(VOID);
LW_API ULONG    API_DtraceSetPid(PVOID  pvDtrace, pid_t  pid);
LW_API ULONG    API_DtraceGetRegs(PVOID  pvDtrace, 
                                  LW_OBJECT_HANDLE  ulThread, 
                                  ARCH_REG_CTX  *pregctx,
                                  ARCH_REG_T *pregSp);
LW_API ULONG    API_DtraceSetRegs(PVOID  pvDtrace, 
                                  LW_OBJECT_HANDLE  ulThread, 
                                  const ARCH_REG_CTX  *pregctx);
                                  
#if LW_CFG_CPU_FPU_EN > 0
LW_API ULONG    API_DtraceGetFpuRegs(PVOID  pvDtrace, 
                                    LW_OBJECT_HANDLE  ulThread, 
                                    ARCH_FPU_CTX  *pfpuctx);
LW_API ULONG    API_DtraceSetFpuRegs(PVOID  pvDtrace, 
                                     LW_OBJECT_HANDLE  ulThread, 
                                     const ARCH_FPU_CTX  *pfpuctx);
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
                                     
LW_API ULONG    API_DtraceGetMems(PVOID  pvDtrace, addr_t  ulAddr, PVOID  pvBuffer, size_t  stSize);
LW_API ULONG    API_DtraceSetMems(PVOID  pvDtrace, addr_t  ulAddr, const PVOID  pvBuffer, size_t  stSize);
LW_API ULONG    API_DtraceBreakpointInsert(PVOID  pvDtrace, addr_t  ulAddr, size_t stSize, ULONG  *pulIns);
LW_API ULONG    API_DtraceBreakpointRemove(PVOID  pvDtrace, addr_t  ulAddr, size_t stSize, ULONG  ulIns);
LW_API ULONG    API_DtraceWatchpointInsert(PVOID  pvDtrace, addr_t  ulAddr, size_t stSize);
LW_API ULONG    API_DtraceWatchpointRemove(PVOID  pvDtrace, addr_t  ulAddr, size_t stSize);
LW_API ULONG    API_DtraceStopThread(PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread);
LW_API ULONG    API_DtraceContinueThread(PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread);
LW_API ULONG    API_DtraceStopProcess(PVOID  pvDtrace);
LW_API ULONG    API_DtraceContinueProcess(PVOID  pvDtrace);
LW_API ULONG    API_DtraceProcessThread(PVOID  pvDtrace, LW_OBJECT_HANDLE ulThread[], 
                                        UINT   uiTableNum, UINT *puiThreadNum);
LW_API ULONG    API_DtraceGetBreakInfo(PVOID  pvDtrace, PLW_DTRACE_MSG  pdtm, LW_OBJECT_HANDLE  ulThread);
LW_API ULONG    API_DtraceDelBreakInfo(PVOID             pvDtrace, 
                                       LW_OBJECT_HANDLE  ulThread, 
                                       addr_t            ulBreakAddr,
                                       BOOL              bContinue);
LW_API ULONG    API_DtraceThreadExtraInfo(PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread,
                                          PCHAR  pcExtraInfo, size_t  stSize);
                                          
#ifndef LW_DTRACE_HW_ISTEP
LW_API ULONG    API_DtraceThreadStepSet(PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread, addr_t  ulAddr);
LW_API ULONG    API_DtraceThreadStepGet(PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread, addr_t  *pulAddr);
LW_API VOID     API_DtraceSchedHook(LW_OBJECT_HANDLE  ulThreadOld, LW_OBJECT_HANDLE  ulThreadNew);
#else
LW_API ULONG    API_DtraceThreadStepSet(PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread, BOOL bEnable);
#endif                                                                  /*  !LW_DTRACE_HW_ISTEP         */

/*********************************************************************************************************
  API (SylixOS internal use only!)
*********************************************************************************************************/

#ifdef __SYLIXOS_KERNEL
LW_API INT      API_DtraceBreakTrap(addr_t  ulAddr, UINT  uiBpType);
LW_API INT      API_DtraceAbortTrap(addr_t  ulAddr);
LW_API INT      API_DtraceChildSig(pid_t pid, struct sigevent *psigevent, struct siginfo *psiginfo);
#endif                                                                  /*  __SYLIXOS_KERNEL            */

#endif                                                                  /*  LW_CFG_GDB_EN > 0           */
#endif                                                                  /*  __DTRACE_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
