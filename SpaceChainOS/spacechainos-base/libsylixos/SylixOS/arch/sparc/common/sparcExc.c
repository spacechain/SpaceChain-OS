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
** 文   件   名: sparcExc.c
**
** 创   建   人: Xu.Guizhou (徐贵洲)
**
** 文件创建日期: 2017 年 05 月 15 日
**
** 描        述: SPARC 体系构架异常处理.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "dtrace.h"
#if LW_CFG_VMM_EN > 0
#include "arch/sparc/mm/mmu/sparcMmu.h"
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  外部引用变量声明
*********************************************************************************************************/
extern LW_CLASS_CPU _K_cpuTable[];                                      /*  CPU 表                      */
extern LW_STACK     _K_stkInterruptStack[LW_CFG_MAX_PROCESSORS][LW_CFG_INT_STK_SIZE / sizeof(LW_STACK)];
/*********************************************************************************************************
  外部函数声明
*********************************************************************************************************/
extern UINT sparcUnalignedHandle(ARCH_REG_CTX  *regs, addr_t  *pulAbortAddr, UINT  *puiMethod);
/*********************************************************************************************************
  中断相关
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
addr_t  _G_ulIntSafeStack[LW_CFG_MAX_PROCESSORS];
addr_t  _G_ulIntNesting[LW_CFG_MAX_PROCESSORS];
addr_t  _G_ulCpu[LW_CFG_MAX_PROCESSORS];
#else
addr_t  _G_ulIntSafeStack[1];
addr_t  _G_ulIntNesting[1];
addr_t  _G_ulCpu[1];
#endif
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
    REGISTER irqreturn_t  irqret;

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
** 函数名称: archInstAccessErrHandle
** 功能描述: A peremptory error exception occurred on an instruction access
**           (for example, a parity error on an instruction cache access).
** 输　入  : ulRetAddr     返回地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archInstAccessErrHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_FATAL_ERROR;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archDataStoreErrHandle
** 功能描述: A peremptory error exception occurred on a data store to memory
**           (for example, a bus parity error on a store from a store buffer).
** 输　入  : ulRetAddr     返回地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archDataStoreErrHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_FATAL_ERROR;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archDataAccessErrHandle
** 功能描述: A peremptory error exception occurred on a load/store data access from/to memory
**           (for example, a parity error on a data cache access, or an uncorrect-able ECC memory error).
** 输　入  : ulRetAddr     返回地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archDataAccessErrHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_FATAL_ERROR;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archInstAccessExcHandle
** 功能描述: A blocking error exception occurred on an instruction access
**           (for example, an MMU indicated that the page was invalid or read-protected).
** 输　入  : ulRetAddr     返回地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archInstAccessExcHandle (addr_t  ulRetAddr)
{
#if LW_CFG_VMM_EN > 0
    PLW_CLASS_TCB   ptcbCur;
    addr_t          ulAbortAddr;
    UINT32          ulFaultStatus;
    UINT32          uiAT;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    ulFaultStatus = sparcMmuGetFaultStatus();
    if (ulFaultStatus & FSTAT_FAV_MASK) {
        ulAbortAddr = sparcMmuGetFaultAddr();                           /*  某些实现为错误页面地址      */

    } else {
        ulAbortAddr = (addr_t)-1;
    }

    uiAT = ((ulFaultStatus & FSTAT_AT_MASK) >> FSTAT_AT_SHIFT);
    if (uiAT <= FSTAT_AT_LSD) {
        abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_READ;

    } else if (uiAT <= FSTAT_AT_LXSI) {
        abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;

    } else {
        abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_WRITE;
    }

    switch ((ulFaultStatus & FSTAT_FT_MASK) >> FSTAT_FT_SHIFT) {

    case FSTAT_FT_NONE:
    case FSTAT_FT_RESV:
        break;

    case FSTAT_FT_INVADDR:
        abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_MAP;
        break;

    case FSTAT_FT_PROT:
        abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_PERM;
        break;

    case FSTAT_FT_PRIV:
        abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_PERM;
        break;

    case FSTAT_FT_TRANS:
        abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_MAP;
        break;

    case FSTAT_FT_ACCESSBUS:
        abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_BUS;
        break;

    case FSTAT_FT_INTERNAL:
        abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;
        break;
    }

    if (abtInfo.VMABT_uiType) {
        API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
    }
#else
    archInstAccessErrHandle(ulRetAddr);
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** 函数名称: archDataAccessExcHandle
** 功能描述: A blocking error exception occurred on a load/store data access.
**           (for example, an MMU indicated that the page was invalid or write-protected).
** 输　入  : ulRetAddr     返回地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archDataAccessExcHandle (addr_t  ulRetAddr)
{
    archInstAccessExcHandle(ulRetAddr);
}
/*********************************************************************************************************
** 函数名称: archDataAccessMmuMissHandle
** 功能描述: A miss in an MMU occurred on a load/store access from/to memory.
**           For example, a PDC or TLB did not contain a translation for the virtual adddress.
** 输　入  : ulRetAddr     返回地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK VOID  archDataAccessMmuMissHandle (addr_t  ulRetAddr)
{
}
/*********************************************************************************************************
** 函数名称: archInstAccessMmuMissHandle
** 功能描述: A miss in an MMU occurred on an instruction access from memory.
**           For example, a PDC or TLB did not contain a translation for the virtual adddress.
** 输　入  : ulRetAddr     返回地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK VOID  archInstAccessMmuMissHandle (addr_t  ulRetAddr)
{
}
/*********************************************************************************************************
** 函数名称: archRRegAccessErrHandle
** 功能描述: A peremptory error exception occurred on an r register access
**           (for example, a parity error on an r register read).
** 输　入  : ulRetAddr     返回地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archRRegAccessErrHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_BUS;
    abtInfo.VMABT_uiMethod = BUS_OBJERR;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archPrivInstHandle
** 功能描述: An attempt was made to execute a privileged instruction while S = 0.
** 输　入  : ulRetAddr     返回地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archPrivInstHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_UNDEF;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archIllegalInstHandle
** 功能描述: An attempt was made to execute an instruction with an unimplemented opcode,
**           or an UNIMP instruction
** 输　入  : ulRetAddr     返回地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archIllegalInstHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_UNDEF;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archFpDisableHandle
** 功能描述: An attempt was made to execute an FPop, FBfcc,
**           or a floating-point load/store instruction while EF = 0 or an FPU was not present.
** 输　入  : ulRetAddr     返回地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archFpDisableHandle (addr_t  ulRetAddr)
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
** 函数名称: archCpDisableHandle
** 功能描述: An attempt was made to execute an CPop, CBccc,
**           or a coprocessor load/store instruction while EC = 0 or a coprocessor was not present.
** 输　入  : ulRetAddr     返回地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archCpDisableHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_UNDEF;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archUnimplFlushHandle
** 功能描述: An attempt was made to execute a FLUSH instruction,
**           the semantics of which are not fully implemented in hardware.
** 输　入  : ulRetAddr     返回地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archUnimplFlushHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_UNDEF;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archWatchPointDectectHandle
** 功能描述: An instruction fetch memory address or load/store data memory address
**           matched the contents of a pre-loaded implementation-dependent “watch-point” register.
**           Whether a SPARC processor generates watchpoint_detected exceptions is
**           implementation-dependent.
** 输　入  : ulRetAddr     返回地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK VOID  archWatchPointDectectHandle (addr_t  ulRetAddr)
{
}
/*********************************************************************************************************
** 函数名称: archMemAddrNoAlignHandle
** 功能描述: A load/store instruction would have generated a memory address that was not
**           properly aligned according to the instruction, or a JMPL or RETT instruction
**           would have generated a non-word-aligned address.
** 输　入  : ulRetAddr     返回地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archMemAddrNoAlignHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;
    addr_t         ulAbortAddr;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType = sparcUnalignedHandle((ARCH_REG_CTX *)ptcbCur->TCB_pstkStackNow,
                                                &ulAbortAddr, &abtInfo.VMABT_uiMethod);
    if (abtInfo.VMABT_uiType) {
        API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
    }
}
/*********************************************************************************************************
** 函数名称: archFpExcHandle
** 功能描述: an FPop instruction generated an IEEE_754_exception and its correspond-ing
**           trap enable mask (TEM) bit was 1, or the FPop was unimplemented,
**           or the FPop did not complete, or there was a sequence or hardware error in the FPU.
**           The type of floating-point exception is encoded in the FSR’s ftt field.
** 输　入  : ulRetAddr     返回地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archFpExcHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;
    UINT32          uiFSR;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_FPE;
    abtInfo.VMABT_uiMethod = FPE_FLTINV;

    uiFSR = *(UINT32 *)((addr_t)ptcbCur->TCB_pvStackFP + FSR_OFFSET);
    if ((uiFSR & 0x1c000) == (1 << 14)) {
        if (uiFSR & 0x10) {
            abtInfo.VMABT_uiMethod = FPE_FLTINV;

        } else if (uiFSR & 0x08) {
            abtInfo.VMABT_uiMethod = FPE_FLTOVF;

        } else if (uiFSR & 0x04) {
            abtInfo.VMABT_uiMethod = FPE_FLTUND;

        } else if (uiFSR & 0x02) {
            abtInfo.VMABT_uiMethod = FPE_FLTDIV;

        } else if (uiFSR & 0x01) {
            abtInfo.VMABT_uiMethod = FPE_FLTRES;
        }
    }
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archCpExcHandle
** 功能描述: A coprocessor instruction generated an exception.
** 输　入  : ulRetAddr     返回地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archCpExcHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_UNDEF;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archTagOverFlowHandle
** 功能描述: A TADDccTV or TSUBccTV instruction was executed, and either arith-metic
**           overflow occurred or at least one of the tag bits of the operands was nonzero.
** 输　入  : ulRetAddr     返回地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archTagOverFlowHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archDivZeroHandle
** 功能描述: An integer divide instruction attempted to divide by zero.
** 输　入  : ulRetAddr     返回地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archDivZeroHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_FPE;
    abtInfo.VMABT_uiMethod = FPE_FLTDIV;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archTrapInstHandle
** 功能描述: A Ticc instruction was executed and the trap condition evaluated to true.
** 输　入  : ulRetAddr     返回地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archTrapInstHandle (addr_t  ulRetAddr)
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

    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_SYS;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archTrapInit
** 功能描述: 初始化异常处理系统
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archTrapInit (VOID)
{
    ULONG  i;
    ULONG  ulMaxProcessors = 1;

    /*
     *  重要：
     *  保存每个 CPU 的中断栈地址和每个 CPU 的中断嵌套计数变量指针，
     *  这些信息将在中断处理入口使用，免除调用 C 函数来获取这些信息；
     *  中断处理函数需要使用此栈构造 C 函数运行环境，
     *  否则需要用临时栈构造 C 函数运行环境，再调用 C 函数获取栈地址，最后再切换一次栈
     *  注意：SPARC 切换栈消耗极其大
     */
#if LW_CFG_SMP_EN > 0
    ulMaxProcessors = LW_CFG_MAX_PROCESSORS;
#endif

    for (i = 0; i < ulMaxProcessors; i++) {
        _G_ulCpu[i]          = (addr_t)&_K_cpuTable[i];
        _G_ulIntNesting[i]   = (addr_t)&_K_cpuTable[i].CPU_ulInterNesting;

#if CPU_STK_GROWTH == 0
        _G_ulIntSafeStack[i] = (addr_t)&_K_stkInterruptStack[i][0];
        _G_ulIntSafeStack[i] = ROUND_UP(_G_ulIntSafeStack[i], 16);
#else
        _G_ulIntSafeStack[i] = (addr_t)&_K_stkInterruptStack[i][(LW_CFG_INT_STK_SIZE / sizeof(LW_STACK)) - 1];
        _G_ulIntSafeStack[i] = ROUND_DOWN(_G_ulIntSafeStack[i], 16);
#endif                                                                  /*  CPU_STK_GROWTH              */
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
