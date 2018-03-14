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
** 文   件   名: armExcV7M.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2017 年 11 月 14 日
**
** 描        述: ARMv7M 体系构架异常处理.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  ARMv7M 体系构架
*********************************************************************************************************/
#if defined(__SYLIXOS_ARM_ARCH_M__)
#include "armSvcV7M.h"
/*********************************************************************************************************
  向量使能与禁能锁
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
#define VECTOR_OP_LOCK()    LW_SPIN_LOCK_IGNIRQ(&_K_slVectorTable)
#define VECTOR_OP_UNLOCK()  LW_SPIN_UNLOCK_IGNIRQ(&_K_slVectorTable)
#else
#define VECTOR_OP_LOCK()
#define VECTOR_OP_UNLOCK()
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
** 函数名称: archIntHandle
** 功能描述: bspIntHandle 需要调用此函数处理中断 (关闭中断情况被调用)
** 输　入  : ulVector         中断向量
**           bPreemptive      中断是否可抢占
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
LW_WEAK VOID  archIntHandle (ULONG  ulVector, BOOL  bPreemptive)
{
    REGISTER irqreturn_t irqret;

    if (_Inter_Vector_Invalid(ulVector)) {
        return;                                                         /*  向量号不正确                */
    }

    if (LW_IVEC_GET_FLAG(ulVector) & LW_IRQ_FLAG_PREEMPTIVE) {
        bPreemptive = LW_TRUE;
    }

    if (bPreemptive) {
        VECTOR_OP_LOCK();
        __ARCH_INT_VECTOR_DISABLE(ulVector);                            /*  屏蔽 vector 中断            */
        VECTOR_OP_UNLOCK();
        KN_INT_ENABLE_FORCE();                                          /*  允许中断                    */
    }

    irqret = API_InterVectorIsr(ulVector);                              /*  调用中断服务程序            */

    KN_INT_DISABLE();                                                   /*  禁能中断                    */

    if (bPreemptive) {
        if (irqret != LW_IRQ_HANDLED_DISV) {
            VECTOR_OP_LOCK();
            __ARCH_INT_VECTOR_ENABLE(ulVector);                         /*  允许 vector 中断            */
            VECTOR_OP_UNLOCK();
        }

    } else if (irqret == LW_IRQ_HANDLED_DISV) {
        VECTOR_OP_LOCK();
        __ARCH_INT_VECTOR_DISABLE(ulVector);                            /*  屏蔽 vector 中断            */
        VECTOR_OP_UNLOCK();
    }
}
/*********************************************************************************************************
  Structure type to access the System Control Block (SCB)
*********************************************************************************************************/
typedef struct {
    UINT32      CPUID;          /*  0x000 (R/ )  CPUID Base Register                                    */
    UINT32      ICSR;           /*  0x004 (R/W)  Interrupt Control and State Register                   */
    UINT32      VTOR;           /*  0x008 (R/W)  Vector Table Offset Register                           */
    UINT32      AIRCR;          /*  0x00C (R/W)  Application Interrupt and Reset Control Register       */
    UINT32      SCR;            /*  0x010 (R/W)  System Control Register                                */
    UINT32      CCR;            /*  0x014 (R/W)  Configuration Control Register                         */
    UINT8       SHP[12];        /*  0x018 (R/W)  System Handlers Priority Registers (4-7, 8-11, 12-15)  */
    UINT32      SHCSR;          /*  0x024 (R/W)  System Handler Control and State Register              */
    UINT32      CFSR;           /*  0x028 (R/W)  Configurable Fault Status Register                     */
    UINT32      HFSR;           /*  0x02C (R/W)  HardFault Status Register                              */
    UINT32      DFSR;           /*  0x030 (R/W)  Debug Fault Status Register                            */
    UINT32      MMFAR;          /*  0x034 (R/W)  MemManage Fault Address Register                       */
    UINT32      BFAR;           /*  0x038 (R/W)  BusFault Address Register                              */
    UINT32      AFSR;           /*  0x03C (R/W)  Auxiliary Fault Status Register                        */
    UINT32      PFR[2];         /*  0x040 (R/ )  Processor Feature Register                             */
    UINT32      DFR;            /*  0x048 (R/ )  Debug Feature Register                                 */
    UINT32      ADR;            /*  0x04C (R/ )  Auxiliary Feature Register                             */
    UINT32      MMFR[4];        /*  0x050 (R/ )  Memory Model Feature Register                          */
    UINT32      ISAR[5];        /*  0x060 (R/ )  Instruction Set Attributes Register                    */
    UINT32      RESERVED0[5];
    UINT32      CPACR;          /*  0x088 (R/W)  Coprocessor Access Control Register                    */
} SCB_Type;
/*********************************************************************************************************
  Memory mapping of Cortex-M Hardware
*********************************************************************************************************/
#define SCS_BASE            (0xe000e000)                        /*  System Control Space Base Address   */
#define SCB_BASE            (SCS_BASE + 0x0d00)                 /*  System Control Block Base Address   */
#define SCB                 ((SCB_Type *)SCB_BASE)
/*********************************************************************************************************
  数据类型定义
*********************************************************************************************************/
typedef enum {
    UNALIGN_TRP = 0x00000008,
    DIV_0_TRP   = 0x00000010,
} CONFIG_CTRL_BITS;

typedef enum {
    BUSFAULTENA = 0x00020000,
    USGFAULTENA = 0x00040000,
} SYS_HANDLER_CSR_BITS;

typedef enum {
    IBUSERR     = 0x00000100,
    PRECISERR   = 0x00000200,
    IMPRECISERR = 0x00000400,
    UNSTKERR    = 0x00000800,
    STKERR      = 0x00001000,
    BFARVALID   = 0x00008000,
    UNDEFINSTR  = 0x00010000,
    INVSTATE    = 0x00020000,
    INVPC       = 0x00040000,
    NOCP        = 0x00080000,
    UNALIGNED   = 0x01000000,
    DIVBYZERO   = 0x02000000,
} LOCAL_FAULT_STATUS_BITS;

typedef enum {
    VECTTBL     = 0x00000002,
    FORCED      = 0x40000000,
} HARD_FAULT_STATUS_BITS;

typedef enum {
    HARDFAULT   = 3,
    BUSFAULT    = 5,
    USAGEFAULT  = 6,
} INTERRUPTS;

typedef struct {
    CHAR   *pcName;
    INT     iTestBit;
    INT     iHandler;
} TRAPS;

static TRAPS  _G_traps[] = {
    {"Vector Read error",        VECTTBL,     HARDFAULT},
    {"uCode stack push error",   STKERR,      BUSFAULT},
    {"uCode stack pop error",    UNSTKERR,    BUSFAULT},
    {"Escalated to Hard Fault",  FORCED,      HARDFAULT},
    {"Pre-fetch error",          IBUSERR,     BUSFAULT},
    {"Precise data bus error",   PRECISERR,   BUSFAULT},
    {"Imprecise data bus error", IMPRECISERR, BUSFAULT},
    {"No Coprocessor",           NOCP,        USAGEFAULT},
    {"Undefined Instruction",    UNDEFINSTR,  USAGEFAULT},
    {"Invalid ISA state",        INVSTATE,    USAGEFAULT},
    {"Return to invalid PC",     INVPC,       USAGEFAULT},
    {"Illegal unaligned access", UNALIGNED,   USAGEFAULT},
    {"Divide By 0",              DIVBYZERO,   USAGEFAULT},
    {NULL}
};
/*********************************************************************************************************
** 函数名称: armv7mTrapsInit
** 功能描述: The function initializes Bus fault and Usage fault exceptions,
**           forbids unaligned data access and division by 0.
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  armv7mTrapsInit (VOID)
{
    write32(read32((addr_t)&SCB->SHCSR) | USGFAULTENA | BUSFAULTENA,
            (addr_t)&SCB->SHCSR);

    write32(UNALIGN_TRP | DIV_0_TRP | read32((addr_t)&SCB->CCR),
            (addr_t)&SCB->CCR);
}
/*********************************************************************************************************
** 函数名称: armv7mSvcHandle
** 功能描述: SVC 处理
** 输　入  : uiVector  中断向量
**           pregctx   上下文
** 输　出  : 上下文
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ARCH_REG_CTX  *armv7mSvcHandle (UINT32  uiVector, ARCH_REG_CTX  *pregctx)
{
    UINT32          uiCmd = pregctx->REG_uiR1;
    PLW_CLASS_CPU   pcpuCur;

    switch (uiCmd) {

    case SVC_archTaskCtxStart:
        pcpuCur = (PLW_CLASS_CPU)pregctx->REG_uiR0;
        return  ((ARCH_REG_CTX *)(pcpuCur->CPU_ptcbTCBCur->TCB_pstkStackNow));

    case SVC_archTaskCtxSwitch:
        pcpuCur = (PLW_CLASS_CPU)pregctx->REG_uiR0;
        pcpuCur->CPU_ptcbTCBCur->TCB_pstkStackNow = (PLW_STACK)pregctx;
        _SchedSwp(pcpuCur);
        return  ((ARCH_REG_CTX *)(pcpuCur->CPU_ptcbTCBCur->TCB_pstkStackNow));

#if LW_CFG_COROUTINE_EN > 0
    case SVC_archCrtCtxSwitch:
        pcpuCur = (PLW_CLASS_CPU)pregctx->REG_uiR0;
        pcpuCur->CPU_pcrcbCur->COROUTINE_pstkStackNow = (PLW_STACK)pregctx;
        _SchedCrSwp(pcpuCur);
        return  ((ARCH_REG_CTX *)(pcpuCur->CPU_pcrcbCur->COROUTINE_pstkStackNow));
#endif

    case SVC_archSigCtxLoad:
        return  ((ARCH_REG_CTX *)pregctx->REG_uiR0);

    default:
        _BugHandle(LW_TRUE, LW_TRUE, "unknown SVC command!\r\n");
        break;
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: armv7mIntHandle
** 功能描述: 中断处理
** 输　入  : uiVector  中断向量
**           pregctx   上下文
** 输　出  : 上下文
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ARCH_REG_CTX  *armv7mIntHandle (UINT32  uiVector, ARCH_REG_CTX  *pregctx)
{
    if (LW_CPU_GET_CUR_NESTING() == 1) {
        PLW_CLASS_TCB   ptcbCur;

        LW_TCB_GET_CUR(ptcbCur);
        ptcbCur->TCB_pstkStackNow = (PLW_STACK)pregctx;
    }

    archIntHandle((ULONG)uiVector, LW_FALSE);

    API_InterExit();

    return  (pregctx);
}
/*********************************************************************************************************
** 函数名称: armv7mNMIIntHandle
** 功能描述: NMI 处理
** 输　入  : uiVector  中断向量
**           pregctx   上下文
** 输　出  : 上下文
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK ARCH_REG_CTX  *armv7mNMIIntHandle (UINT32  uiVector, ARCH_REG_CTX  *pregctx)
{
    return  (armv7mIntHandle(uiVector, pregctx));
}
/*********************************************************************************************************
** 函数名称: armv7mSysTickIntHandle
** 功能描述: SysTick 中断处理
** 输　入  : uiVector  中断向量
**           pregctx   上下文
** 输　出  : 上下文
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ARCH_REG_CTX  *armv7mSysTickIntHandle (UINT32  uiVector, ARCH_REG_CTX  *pregctx)
{
    return  (armv7mIntHandle(uiVector, pregctx));
}
/*********************************************************************************************************
** 函数名称: armv7mFaultPrintInfo
** 功能描述: The function prints information about the reason of the exception
** 输　入  : in        IPSR, the number of the exception
**           addr      address caused the interrupt, or current pc
**           hstatus   status register for hard fault
**           lstatus   status register for local fault
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  armv7mFaultPrintInfo (INTERRUPTS  in,
                                   addr_t      ulAddr,
                                   UINT32      uiHStatus,
                                   UINT32      uiLStatus)
{
    INT  i;

    _PrintFormat("\r\n\r\nfault %d at 0x%08lx [hstatus=0x%08lx, lstatus=0x%08lx]\r\n",
                (INT)in, ulAddr, uiHStatus, uiLStatus);

    for (i = 0; _G_traps[i].pcName != NULL; ++i) {
        if ((_G_traps[i].iHandler == HARDFAULT ? uiHStatus : uiLStatus) & _G_traps[i].iTestBit) {
            _PrintFormat("%s\r\n", _G_traps[i].pcName);
        }
    }
    _PrintFormat("\r\n");
}
/*********************************************************************************************************
** 函数名称: armv7mFaultCommonHandle
** 功能描述: Common routine for high-level exception handlers.
** 输　入  : pregctx   上下文
**           ptcbCur   当前任务控制块
**           in        异常号
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  armv7mFaultCommonHandle (ARCH_REG_CTX   *pregctx,
                                      PLW_CLASS_TCB   ptcbCur,
                                      INTERRUPTS      in)
{
    LW_VMM_ABORT    abtInfo;
    UINT32          uiHStatus;
    UINT32          uiLStatus;
    addr_t          ulAddr;

    uiHStatus = read32((addr_t)&SCB->HFSR);
    uiLStatus = read32((addr_t)&SCB->CFSR);

    if (uiLStatus & BFARVALID && (in == BUSFAULT ||
        (in == HARDFAULT && uiHStatus & FORCED))) {
        ulAddr = read32((addr_t)&SCB->BFAR);

    } else {
        ulAddr = pregctx->REG_uiPc;
    }

    write32(uiHStatus, (addr_t)&SCB->HFSR);
    write32(uiLStatus, (addr_t)&SCB->CFSR);

    armv7mFaultPrintInfo(in, ulAddr, uiHStatus, uiLStatus);

#if LW_CFG_CORTEX_M_FAULT_REBOOT > 0
    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_FATAL_ERROR;
#else
    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;
#endif

    abtInfo.VMABT_uiMethod = 0;

    API_VmmAbortIsr(pregctx->REG_uiPc, pregctx->REG_uiPc, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: armv7mHardFaultHandle
** 功能描述: Hard Fault 处理
** 输　入  : uiVector  中断向量
**           pregctx   上下文
** 输　出  : 上下文
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ARCH_REG_CTX  *armv7mHardFaultHandle (UINT32  uiVector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB   ptcbCur;

    LW_TCB_GET_CUR(ptcbCur);

    if (LW_CPU_GET_CUR_NESTING() == 1) {
        ptcbCur->TCB_pstkStackNow = (PLW_STACK)pregctx;
    }

    armv7mFaultCommonHandle(pregctx, ptcbCur, HARDFAULT);

    API_InterExit();

    return  ((ARCH_REG_CTX *)ptcbCur->TCB_pstkStackNow);
}
/*********************************************************************************************************
** 函数名称: armv7mBusFaultHandle
** 功能描述: Bus Fault 处理
** 输　入  : uiVector  中断向量
**           pregctx   上下文
** 输　出  : 上下文
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ARCH_REG_CTX  *armv7mBusFaultHandle (UINT32  uiVector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB   ptcbCur;

    LW_TCB_GET_CUR(ptcbCur);

    if (LW_CPU_GET_CUR_NESTING() == 1) {
        ptcbCur->TCB_pstkStackNow = (PLW_STACK)pregctx;
    }

    armv7mFaultCommonHandle(pregctx, ptcbCur, BUSFAULT);

    API_InterExit();

    return  ((ARCH_REG_CTX *)ptcbCur->TCB_pstkStackNow);
}
/*********************************************************************************************************
** 函数名称: armv7mUsageFaultHandle
** 功能描述: Usage Fault 处理
** 输　入  : uiVector  中断向量
**           pregctx   上下文
** 输　出  : 上下文
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ARCH_REG_CTX  *armv7mUsageFaultHandle (UINT32  uiVector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB   ptcbCur;

    LW_TCB_GET_CUR(ptcbCur);

    if (LW_CPU_GET_CUR_NESTING() == 1) {
        ptcbCur->TCB_pstkStackNow = (PLW_STACK)pregctx;
    }

#if LW_CFG_CPU_FPU_EN > 0
    if (archFpuUndHandle(ptcbCur) == ERROR_NONE) {                      /*  进行 FPU 指令探测           */
        API_InterExit();
        return  (pregctx);
    }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

    armv7mFaultCommonHandle(pregctx, ptcbCur, USAGEFAULT);

    API_InterExit();

    return  ((ARCH_REG_CTX *)ptcbCur->TCB_pstkStackNow);
}
/*********************************************************************************************************
** 函数名称: armv7mMemFaultHandle
** 功能描述: Mem Fault 处理
** 输　入  : uiVector  中断向量
**           pregctx   上下文
** 输　出  : 上下文
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ARCH_REG_CTX  *armv7mMemFaultHandle (UINT32  uiVector, ARCH_REG_CTX  *pregctx)
{
    UINT32          uiLStatus;
    addr_t          ulAddr;
    LW_VMM_ABORT    abtInfo;
    PLW_CLASS_TCB   ptcbCur;

    LW_TCB_GET_CUR(ptcbCur);

    if (LW_CPU_GET_CUR_NESTING() == 1) {
        ptcbCur->TCB_pstkStackNow = (PLW_STACK)pregctx;
    }

    /*
     * Read the fault status register from the MPU hardware
     */
    uiLStatus = read32((addr_t)&SCB->CFSR);

    /*
     * Clean up the memory fault status register for a next exception
     */
    write32(uiLStatus, (addr_t)&SCB->CFSR);

    if ((uiLStatus & 0xF0) == 0x80) {
        /*
         * Did we get a valid address in the memory fault address register?
         * If so, this is a data access failure (can't tell read or write).
         */
        ulAddr = read32((addr_t)&SCB->MMFAR);

        abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_PERM;
        abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_WRITE;

    } else if (uiLStatus & 0x1) {
        /*
         * If there is no valid address, did we get an instuction access
         * failure? If so, this is a code access failure.
         */
        ulAddr = pregctx->REG_uiPc;

        abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_PERM;
        abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;

    } else {
        /*
         * Some error bit set in the memory fault address register.
         * This must be MUNSTKERR due to a stacking error, which
         * implies that we have gone beyond the low stack boundary
         * (or somehow SP got incorrect).
         * There is no recovery from that since no registers
         * were saved on the stack on this exception, that is,
         * we have no PC saved to return to user mode.
         */
        ulAddr = (addr_t)-1;
        abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_PERM;
        abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
    }

#if LW_CFG_CORTEX_M_FAULT_REBOOT > 0
    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_FATAL_ERROR;
#endif

    API_VmmAbortIsr(pregctx->REG_uiPc, ulAddr, &abtInfo, ptcbCur);

    API_InterExit();

    return  ((ARCH_REG_CTX *)ptcbCur->TCB_pstkStackNow);
}
/*********************************************************************************************************
** 函数名称: armv7mDebugMonitorHandle
** 功能描述: Debug Monitor 处理
** 输　入  : uiVector  中断向量
**           pregctx   上下文
** 输　出  : 上下文
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ARCH_REG_CTX  *armv7mDebugMonitorHandle (UINT32  uiVector, ARCH_REG_CTX  *pregctx)
{
    LW_VMM_ABORT    abtInfo;
    PLW_CLASS_TCB   ptcbCur;

    LW_TCB_GET_CUR(ptcbCur);

    if (LW_CPU_GET_CUR_NESTING() == 1) {
        ptcbCur->TCB_pstkStackNow = (PLW_STACK)pregctx;
    }

#if LW_CFG_CORTEX_M_FAULT_REBOOT > 0
    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_FATAL_ERROR;
#else
    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_BREAK;
#endif

    abtInfo.VMABT_uiMethod = 0;

    API_VmmAbortIsr(pregctx->REG_uiPc, pregctx->REG_uiPc, &abtInfo, ptcbCur);

    API_InterExit();

    return  ((ARCH_REG_CTX *)ptcbCur->TCB_pstkStackNow);
}
/*********************************************************************************************************
** 函数名称: armv7mPendSVHandle
** 功能描述: PendSV 处理
** 输　入  : uiVector  中断向量
**           pregctx   上下文
** 输　出  : 上下文
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ARCH_REG_CTX  *armv7mPendSVHandle (UINT32  uiVector, ARCH_REG_CTX  *pregctx)
{
    LW_VMM_ABORT    abtInfo;
    PLW_CLASS_TCB   ptcbCur;

    LW_TCB_GET_CUR(ptcbCur);

    if (LW_CPU_GET_CUR_NESTING() == 1) {
        ptcbCur->TCB_pstkStackNow = (PLW_STACK)pregctx;
    }

#if LW_CFG_CORTEX_M_FAULT_REBOOT > 0
    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_FATAL_ERROR;
#else
    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_UNDEF;
#endif

    abtInfo.VMABT_uiMethod = 0;

    API_VmmAbortIsr(pregctx->REG_uiPc, pregctx->REG_uiPc, &abtInfo, ptcbCur);

    API_InterExit();

    return  ((ARCH_REG_CTX *)ptcbCur->TCB_pstkStackNow);
}
/*********************************************************************************************************
** 函数名称: armv7mReservedIntHandle
** 功能描述: Reserved 中断处理
** 输　入  : uiVector  中断向量
**           pregctx   上下文
** 输　出  : 上下文
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ARCH_REG_CTX  *armv7mReservedIntHandle (UINT32  uiVector, ARCH_REG_CTX  *pregctx)
{
    LW_VMM_ABORT    abtInfo;
    PLW_CLASS_TCB   ptcbCur;

    LW_TCB_GET_CUR(ptcbCur);

    if (LW_CPU_GET_CUR_NESTING() == 1) {
        ptcbCur->TCB_pstkStackNow = (PLW_STACK)pregctx;
    }

#if LW_CFG_CORTEX_M_FAULT_REBOOT > 0
    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_FATAL_ERROR;
#else
    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;
#endif

    abtInfo.VMABT_uiMethod = 0;

    API_VmmAbortIsr(pregctx->REG_uiPc, pregctx->REG_uiPc, &abtInfo, ptcbCur);

    API_InterExit();

    return  ((ARCH_REG_CTX *)ptcbCur->TCB_pstkStackNow);
}

#endif                                                                  /*  __SYLIXOS_ARM_ARCH_M__      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
