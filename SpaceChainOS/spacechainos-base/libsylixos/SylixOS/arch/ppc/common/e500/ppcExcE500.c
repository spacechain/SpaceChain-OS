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
** 文   件   名: ppcExcE500.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 05 月 04 日
**
** 描        述: PowerPC E500 体系构架异常处理.
**
** 注        意: 目前没有实现 E.HV(Embedded.Hypervisor) 和 E.HV.LRAT(Embedded.Hypervisor.LRAT) 相关的
**               异常处理, 未来支持硬件虚拟化时需要加入相关的异常处理.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "dtrace.h"
#include "../ppcSpr.h"
#include "ppcSprE500.h"
#define  __SYLIXOS_PPC_E500__
#define  __SYLIXOS_PPC_E500MC__
#include "arch/ppc/arch_e500.h"
#if LW_CFG_VMM_EN > 0
#include "arch/ppc/mm/mmu/e500/ppcMmuE500.h"
#endif
/*********************************************************************************************************
  汇编函数声明
*********************************************************************************************************/
extern VOID  archE500CriticalInputExceptionEntry(VOID);
extern VOID  archE500MachineCheckExceptionEntry(VOID);
extern VOID  archE500DataStorageExceptionEntry(VOID);
extern VOID  archE500InstructionStorageExceptionEntry(VOID);
extern VOID  archE500ExternalInterruptEntry(VOID);
extern VOID  archE500AlignmentExceptionEntry(VOID);
extern VOID  archE500ProgramExceptionEntry(VOID);
extern VOID  archE500FpuUnavailableExceptionEntry(VOID);
extern VOID  archE500SystemCallEntry(VOID);
extern VOID  archE500ApUnavailableExceptionEntry(VOID);
extern VOID  archE500DecrementerInterruptEntry(VOID);
extern VOID  archE500TimerInterruptEntry(VOID);
extern VOID  archE500WatchdogInterruptEntry(VOID);
extern VOID  archE500DataTLBErrorEntry(VOID);
extern VOID  archE500InstructionTLBErrorEntry(VOID);
extern VOID  archE500DebugExceptionEntry(VOID);
extern VOID  archE500SpeUnavailableExceptionEntry(VOID);
extern VOID  archE500FpDataExceptionEntry(VOID);
extern VOID  archE500FpRoundExceptionEntry(VOID);
extern VOID  archE500AltiVecUnavailableExceptionEntry(VOID);
extern VOID  archE500AltiVecAssistExceptionEntry(VOID);
extern VOID  archE500PerfMonitorExceptionEntry(VOID);
extern VOID  archE500DoorbellExceptionEntry(VOID);
extern VOID  archE500DoorbellCriticalExceptionEntry(VOID);
/*********************************************************************************************************
** 函数名称: archE500CriticalInputExceptionHandle
** 功能描述: 临界输入异常处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
VOID  archE500CriticalInputExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archE500MachineCheckExceptionHandle
** 功能描述: 机器检查异常处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
VOID  archE500MachineCheckExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

#if LW_CFG_CPU_EXC_HOOK_EN > 0
    addr_t          ulAbortAddr = ppcE500GetMCAR();
#endif

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_CPU_EXC_HOOK_EN > 0
    if (bspCpuExcHook(ptcbCur, ulRetAddr, ulAbortAddr, ARCH_MACHINE_EXCEPTION, 0)) {
        return;
    }
#endif

    _DebugHandle(__ERRORMESSAGE_LEVEL, "Machine error detected!\r\n");
    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_FATAL_ERROR;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archE500DataStorageExceptionHandle
** 功能描述: 数据存储异常处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
VOID  archE500DataStorageExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    addr_t          ulAbortAddr;
    LW_VMM_ABORT    abtInfo;

    ulAbortAddr          = ppcE500GetDEAR();
    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_VMM_EN > 0
    UINT32  uiESR = ppcE500GetESR();

    if (uiESR & ARCH_PPC_ESR_BO) {                                      /*  a byte-ordering exception   */
        abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;

    } else if (uiESR & ARCH_PPC_ESR_DLK) {                              /*  a DSI occurs because dcbtls,*/
                                                                        /*  dcbtstls, or dcblc is       */
                                                                        /*  executed in user mode       */
        abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;

    } else if (uiESR & ARCH_PPC_ESR_ST) {                               /*  a store or store-class cache*/
                                                                        /*  management instruction      */
        abtInfo.VMABT_uiType   = ppcE500MmuDataStorageAbortType(ulAbortAddr, LW_TRUE);
        abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_WRITE;

    } else {
        abtInfo.VMABT_uiType   = ppcE500MmuDataStorageAbortType(ulAbortAddr, LW_FALSE);
        abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_READ;
    }
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

    if (abtInfo.VMABT_uiType) {
        API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
    }
}
/*********************************************************************************************************
** 函数名称: archE500InstructionStorageExceptionHandle
** 功能描述: 指令访问异常处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
VOID  archE500InstructionStorageExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    addr_t          ulAbortAddr;
    LW_VMM_ABORT    abtInfo;

    ulAbortAddr          = ulRetAddr;
    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_VMM_EN > 0
    UINT32  uiESR = ppcE500GetESR();
    if (uiESR & ARCH_PPC_ESR_BO) {                                      /*  the instruction fetch caused*/
                                                                        /*  a byte-ordering exception   */
        abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;

    } else {
        abtInfo.VMABT_uiType   = ppcE500MmuInstStorageAbortType(ulAbortAddr);
        abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
    }
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

    if (abtInfo.VMABT_uiType) {
        API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
    }
}
/*********************************************************************************************************
** 函数名称: archE500AlignmentExceptionHandle
** 功能描述: 非对齐异常处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
VOID  archE500AlignmentExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    addr_t          ulAbortAddr;
    LW_VMM_ABORT    abtInfo;

    ulAbortAddr            = ppcE500GetDEAR();
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_BUS;
    abtInfo.VMABT_uiMethod = BUS_ADRALN;

    LW_TCB_GET_CUR(ptcbCur);

    API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archE500ProgramExceptionHandle
** 功能描述: 程序异常处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
VOID  archE500ProgramExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

#if LW_CFG_GDB_EN > 0
    UINT    uiBpType = archDbgTrapType(ulRetAddr, (PVOID)LW_NULL);      /*  断点指令探测                */
    if (uiBpType) {
        if (API_DtraceBreakTrap(ulRetAddr, uiBpType) == ERROR_NONE) {   /*  进入调试接口断点处理        */
            return;
        }
    }
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_UNDEF;
    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archE500FpuUnavailableExceptionHandle
** 功能描述: FPU 不可用异常处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
VOID  archE500FpuUnavailableExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_CPU_FPU_EN > 0
    if (archFpuUndHandle(ptcbCur) == ERROR_NONE) {                      /*  进行 FPU 指令探测           */
        return;
    }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_FPE;
    abtInfo.VMABT_uiMethod = FPE_FLTINV;                                /*  FPU 不可用                  */
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archE500ApUnavailableExceptionHandle
** 功能描述: AP 不可用异常处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
VOID  archE500ApUnavailableExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_CPU_FPU_EN > 0
    if (archFpuUndHandle(ptcbCur) == ERROR_NONE) {                      /*  进行 FPU 指令探测           */
        return;
    }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_FPE;
    abtInfo.VMABT_uiMethod = FPE_FLTINV;                                /*  FPU 不可用                  */
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archE500SystemCallHandle
** 功能描述: 系统调用处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
VOID  archE500SystemCallHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_SYS;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archE500DataTLBErrorHandle
** 功能描述: 数据访问 TLB 错误异常处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
#if LW_CFG_VMM_EN == 0

VOID  archE500DataTLBErrorHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    addr_t          ulAbortAddr;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    ulAbortAddr = ppcE500GetDEAR();

    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;
    API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
}

#endif                                                                 /*  LW_CFG_VMM_EN > 0            */
/*********************************************************************************************************
** 函数名称: archE500InstructionTLBErrorHandle
** 功能描述: 指令访问 TLB 错误异常处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
#if LW_CFG_VMM_EN == 0

VOID  archE500InstructionTLBErrorHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}

#endif                                                                 /*  LW_CFG_VMM_EN > 0            */
/*********************************************************************************************************
** 函数名称: archE500DebugExceptionHandle
** 功能描述: 调试异常处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
VOID  archE500DebugExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archE500SpeUnavailableExceptionHandle
** 功能描述: SPE 不可用异常处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
VOID  archE500SpeUnavailableExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_CPU_FPU_EN > 0
    if (archFpuUndHandle(ptcbCur) == ERROR_NONE) {                      /*  进行 FPU 指令探测           */
        return;
    }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_FPE;
    abtInfo.VMABT_uiMethod = FPE_FLTINV;                                /*  FPU 不可用                  */
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archE500FpDataExceptionHandle
** 功能描述: SPE floating-point data 异常处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
VOID  archE500FpDataExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    /*
     * SPEFSCR[FINVE] = 1 and either SPEFSCR[FINVH, FINV] = 1
     * SPEFSCR[FDBZE] = 1 and either SPEFSCR[FDBZH, FDBZ] = 1
     * SPEFSCR[FUNFE] = 1 and either SPEFSCR[FUNFH, FUNF] = 1
     * SPEFSCR[FOVFE] = 1 and either SPEFSCR[FOVFH, FOVF] = 1
     */
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_FPE;
    abtInfo.VMABT_uiMethod = FPE_FLTUND;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archE500FpRoundExceptionHandle
** 功能描述: SPE floating-point round 异常处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
VOID  archE500FpRoundExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    /*
     * SPEFSCR[FINXE] = 1 and any of the SPEFSCR[FGH, FXH, FG, FX] bits = 1
     * SPEFSCR[FRMC]  = 0b10 (+∞)
     * SPEFSCR[FRMC]  = 0b11 (-∞)
     */
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_FPE;
    abtInfo.VMABT_uiMethod = FPE_FLTOVF;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archE500AltiVecUnavailableExceptionHandle
** 功能描述: AltiVec 不可用异常处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
VOID  archE500AltiVecUnavailableExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_CPU_FPU_EN > 0
    if (archFpuUndHandle(ptcbCur) == ERROR_NONE) {                      /*  进行 FPU 指令探测           */
        return;
    }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_FPE;
    abtInfo.VMABT_uiMethod = FPE_FLTINV;                                /*  FPU 不可用                  */
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archE500AltiVecAssistExceptionHandle
** 功能描述: AltiVec Assist 异常处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
VOID  archE500AltiVecAssistExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_FPE;
    abtInfo.VMABT_uiMethod = FPE_FLTINV;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archE500PerfMonitorExceptionHandle
** 功能描述: Performance monitor 异常处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
LW_WEAK VOID  archE500PerfMonitorExceptionHandle (addr_t  ulRetAddr)
{
}
/*********************************************************************************************************
** 函数名称: archE500DoorbellExceptionHandle
** 功能描述: Processor doorbell 异常处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
LW_WEAK VOID  archE500DoorbellExceptionHandle (addr_t  ulRetAddr)
{
}
/*********************************************************************************************************
** 函数名称: archE500DoorbellCriticalExceptionHandle
** 功能描述: Processor doorbell critical 异常处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
LW_WEAK VOID  archE500DoorbellCriticalExceptionHandle (addr_t  ulRetAddr)
{
}
/*********************************************************************************************************
** 函数名称: archE500DecrementerInterruptAck
** 功能描述: Decrementer 中断 ACK
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archE500DecrementerInterruptAck (VOID)
{
    UINT32  uiTSR;

    uiTSR  = ppcE500GetTSR();
    uiTSR |= ARCH_PPC_TSR_DIS_U << 16;
    ppcE500SetTSR(uiTSR);
}
/*********************************************************************************************************
** 函数名称: archE500DecrementerInterruptHandle
** 功能描述: Decrementer 中断处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
LW_WEAK VOID  archE500DecrementerInterruptHandle (VOID)
{
    archE500DecrementerInterruptAck();
}
/*********************************************************************************************************
** 函数名称: archE500DecrementerInterruptEnable
** 功能描述: Decrementer 中断使能
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archE500DecrementerInterruptEnable (VOID)
{
    UINT32  uiTCR;

    uiTCR  = ppcE500GetTCR();
    uiTCR |= ARCH_PPC_TCR_DIE_U << 16;
    ppcE500SetTCR(uiTCR);
}
/*********************************************************************************************************
** 函数名称: archE500DecrementerInterruptDisable
** 功能描述: Decrementer 中断禁能
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archE500DecrementerInterruptDisable (VOID)
{
    UINT32  uiTCR;

    uiTCR  = ppcE500GetTCR();
    uiTCR &= ~(ARCH_PPC_TCR_DIE_U << 16);
    ppcE500SetTCR(uiTCR);
}
/*********************************************************************************************************
** 函数名称: archE500DecrementerInterruptIsEnable
** 功能描述: 判断 Decrementer 中断是否使能
** 输　入  : NONE
** 输　出  : LW_TRUE OR LW_FALSE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
BOOL  archE500DecrementerInterruptIsEnable (VOID)
{
    UINT32  uiTCR;

    uiTCR = ppcE500GetTCR();
    if (uiTCR & (ARCH_PPC_TCR_DIE_U << 16)) {
        return  (LW_TRUE);
    } else {
        return  (LW_FALSE);
    }
}
/*********************************************************************************************************
** 函数名称: archE500DecrementerAutoReloadEnable
** 功能描述: Decrementer 中断使能
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archE500DecrementerAutoReloadEnable (VOID)
{
    UINT32  uiTCR;

    uiTCR  = ppcE500GetTCR();
    uiTCR |= ARCH_PPC_TCR_ARE_U << 16;
    ppcE500SetTCR(uiTCR);
}
/*********************************************************************************************************
** 函数名称: archE500DecrementerAutoReloadDisable
** 功能描述: Decrementer 中断禁能
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archE500DecrementerAutoReloadDisable (VOID)
{
    UINT32  uiTCR;

    uiTCR  = ppcE500GetTCR();
    uiTCR &= ~(ARCH_PPC_TCR_ARE_U << 16);
    ppcE500SetTCR(uiTCR);
}
/*********************************************************************************************************
** 函数名称: archE500TimerInterruptAck
** 功能描述: 固定间隔定时器中断 ACK
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archE500TimerInterruptAck (VOID)
{
    UINT32  uiTSR;

    uiTSR  = ppcE500GetTSR();
    uiTSR |= ARCH_PPC_TSR_FIS_U << 16;
    ppcE500SetTSR(uiTSR);
}
/*********************************************************************************************************
** 函数名称: archE500TimerInterruptHandle
** 功能描述: 固定间隔定时器中断处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
LW_WEAK VOID  archE500TimerInterruptHandle (VOID)
{
    archE500TimerInterruptAck();
}
/*********************************************************************************************************
** 函数名称: archE500TimerInterruptEnable
** 功能描述: 固定间隔定时器中断使能
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archE500TimerInterruptEnable (VOID)
{
    UINT32  uiTCR;

    uiTCR  = ppcE500GetTCR();
    uiTCR |= ARCH_PPC_TCR_FIE_U << 16;
    ppcE500SetTCR(uiTCR);
}
/*********************************************************************************************************
** 函数名称: archE500TimerInterruptDisable
** 功能描述: 固定间隔定时器中断禁能
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archE500TimerInterruptDisable (VOID)
{
    UINT32  uiTCR;

    uiTCR  = ppcE500GetTCR();
    uiTCR &= ~(ARCH_PPC_TCR_FIE_U << 16);
    ppcE500SetTCR(uiTCR);
}
/*********************************************************************************************************
** 函数名称: archE500TimerInterruptIsEnable
** 功能描述: 判断固定间隔定时器中断是否使能
** 输　入  : NONE
** 输　出  : LW_TRUE OR LW_FALSE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
BOOL  archE500TimerInterruptIsEnable (VOID)
{
    UINT32  uiTCR;

    uiTCR = ppcE500GetTCR();
    if (uiTCR & (ARCH_PPC_TCR_FIE_U << 16)) {
        return  (LW_TRUE);
    } else {
        return  (LW_FALSE);
    }
}
/*********************************************************************************************************
** 函数名称: archE500WatchdogInterruptAck
** 功能描述: 看门狗定时器中断 ACK
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archE500WatchdogInterruptAck (VOID)
{
    UINT32  uiTSR;

    uiTSR  = ppcE500GetTSR();
    uiTSR |= ARCH_PPC_TSR_WIS_U << 16;
    ppcE500SetTSR(uiTSR);
}
/*********************************************************************************************************
** 函数名称: archE500WatchdogInterruptHandle
** 功能描述: 看门狗定时器中断处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
LW_WEAK VOID  archE500WatchdogInterruptHandle (VOID)
{
    archE500WatchdogInterruptAck();
}
/*********************************************************************************************************
** 函数名称: archE500WatchdogInterruptEnable
** 功能描述: 看门狗定时器中断使能
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archE500WatchdogInterruptEnable (VOID)
{
    UINT32  uiTCR;

    uiTCR  = ppcE500GetTCR();
    uiTCR |= ARCH_PPC_TCR_WIE_U << 16;
    ppcE500SetTCR(uiTCR);
}
/*********************************************************************************************************
** 函数名称: archE500WatchdogInterruptDisable
** 功能描述: 看门狗定时器中断禁能
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archE500WatchdogInterruptDisable (VOID)
{
    UINT32  uiTCR;

    uiTCR  = ppcE500GetTCR();
    uiTCR &= ~(ARCH_PPC_TCR_WIE_U << 16);
    ppcE500SetTCR(uiTCR);
}
/*********************************************************************************************************
** 函数名称: archE500WatchdogInterruptIsEnable
** 功能描述: 判断看门狗定时器中断是否使能
** 输　入  : NONE
** 输　出  : LW_TRUE OR LW_FALSE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
BOOL  archE500WatchdogInterruptIsEnable (VOID)
{
    UINT32  uiTCR;

    uiTCR = ppcE500GetTCR();
    if (uiTCR & (ARCH_PPC_TCR_WIE_U << 16)) {
        return  (LW_TRUE);
    } else {
        return  (LW_FALSE);
    }
}
/*********************************************************************************************************
** 函数名称: archE500VectorInit
** 功能描述: 初始化 E500 异常向量表
** 输　入  : pcMachineName         机器名
**           ulVectorBase          Interrupt Vector Prefix
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archE500VectorInit (CPCHAR  pcMachineName, addr_t  ulVectorBase)
{
#define tostring(rn)    #rn
#define mtspr(rn, v)    asm volatile("mtspr " tostring(rn) ",%0" : : "r" (v))

    archE500WatchdogInterruptDisable();                                 /*   关闭看门狗中断             */
    archE500DecrementerInterruptDisable();                              /*   关闭 DEC 中断              */
    archE500TimerInterruptDisable();                                    /*   关闭固定间隔定时器中断     */

    mtspr(IVPR, ulVectorBase);

    mtspr(IVOR0,  (addr_t)archE500CriticalInputExceptionEntry - ulVectorBase);
    mtspr(IVOR1,  (addr_t)archE500MachineCheckExceptionEntry  - ulVectorBase);
    mtspr(IVOR2,  (addr_t)archE500DataStorageExceptionEntry - ulVectorBase);
    mtspr(IVOR3,  (addr_t)archE500InstructionStorageExceptionEntry - ulVectorBase);
    mtspr(IVOR4,  (addr_t)archE500ExternalInterruptEntry - ulVectorBase);
    mtspr(IVOR5,  (addr_t)archE500AlignmentExceptionEntry - ulVectorBase);
    mtspr(IVOR6,  (addr_t)archE500ProgramExceptionEntry - ulVectorBase);
    mtspr(IVOR8,  (addr_t)archE500SystemCallEntry - ulVectorBase);
    mtspr(IVOR9,  (addr_t)archE500ApUnavailableExceptionEntry - ulVectorBase);
    mtspr(IVOR10, (addr_t)archE500DecrementerInterruptEntry - ulVectorBase);
    mtspr(IVOR11, (addr_t)archE500TimerInterruptEntry - ulVectorBase);
    mtspr(IVOR12, (addr_t)archE500WatchdogInterruptEntry - ulVectorBase);
    mtspr(IVOR13, (addr_t)archE500DataTLBErrorEntry - ulVectorBase);
    mtspr(IVOR14, (addr_t)archE500InstructionTLBErrorEntry - ulVectorBase);
    mtspr(IVOR15, (addr_t)archE500DebugExceptionEntry - ulVectorBase);
    mtspr(IVOR35, (addr_t)archE500PerfMonitorExceptionEntry - ulVectorBase);

    if ((lib_strcmp(pcMachineName, PPC_MACHINE_E500)   == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_E500V1) == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_E500V2) == 0)) {         /*  E500v1 v2 使用 SPE          */
        mtspr(IVOR32, (addr_t)archE500SpeUnavailableExceptionEntry - ulVectorBase);
        mtspr(IVOR33, (addr_t)archE500FpDataExceptionEntry - ulVectorBase);
        mtspr(IVOR34, (addr_t)archE500FpRoundExceptionEntry - ulVectorBase);

    } else {                                                            /*  E500mc E5500 等后续使用 FPU */
        mtspr(IVOR7, (addr_t)archE500FpuUnavailableExceptionEntry - ulVectorBase);

        if (lib_strcmp(pcMachineName, PPC_MACHINE_E6500) == 0) {        /*  E6500 有 AltiVec            */
            mtspr(IVOR32, (addr_t)archE500AltiVecUnavailableExceptionEntry - ulVectorBase);
            mtspr(IVOR33, (addr_t)archE500AltiVecAssistExceptionEntry - ulVectorBase);
        }

        mtspr(IVOR36, (addr_t)archE500DoorbellExceptionEntry - ulVectorBase);
        mtspr(IVOR37, (addr_t)archE500DoorbellCriticalExceptionEntry - ulVectorBase);
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
