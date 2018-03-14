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
** 文   件   名: armExc.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 12 月 09 日
**
** 描        述: ARM 体系构架异常处理.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "dtrace.h"
#include "../mm/mmu/armMmuCommon.h"
/*********************************************************************************************************
  ARM 体系构架
*********************************************************************************************************/
#if !defined(__SYLIXOS_ARM_ARCH_M__)
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
** 函数名称: bspCpuExcHook
** 功能描述: 处理器异常回调
** 输　入  : ptcb       异常上下文
**           ulRetAddr  异常返回地址
**           ulExcAddr  异常地址
**           iExcType   异常类型
**           iExcInfo   体系结构相关异常信息
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_CPU_EXC_HOOK_EN > 0

LW_WEAK INT  bspCpuExcHook (PLW_CLASS_TCB   ptcb,
                            addr_t          ulRetAddr,
                            addr_t          ulExcAddr,
                            INT             iExcType, 
                            INT             iExcInfo)
{
    return  (0);
}

#endif                                                                  /*  LW_CFG_CPU_EXC_HOOK_EN      */
/*********************************************************************************************************
** 函数名称: archAbtHandle
** 功能描述: 系统发生 data abort 或者 prefetch abort 异常时会调用此函数
** 输　入  : ulRetAddr     异常返回地址.
**           uiArmExcType  ARM 异常类型
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  archAbtHandle (addr_t  ulRetAddr, UINT32  uiArmExcType)
{
#define ARM_EXC_TYPE_ABT    8
#define ARM_EXC_TYPE_PRE    4
    
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;
    addr_t          ulAbortAddr;
    UINT32          uiRawAbtType;
    
    (VOID)uiRawAbtType;

    LW_TCB_GET_CUR(ptcbCur);
    
    if (uiArmExcType == ARM_EXC_TYPE_ABT) {
        ulAbortAddr  = armGetAbtAddr();
        uiRawAbtType = armGetAbtType(&abtInfo);
        
#if LW_CFG_CPU_EXC_HOOK_EN > 0
        if (bspCpuExcHook(ptcbCur, ulRetAddr, ulAbortAddr, ARCH_DATA_EXCEPTION, uiRawAbtType)) {
            return;
        }
#endif
    
    } else {
        ulAbortAddr  = armGetPreAddr(ulRetAddr);
        uiRawAbtType = armGetPreType(&abtInfo);
        
#if LW_CFG_CPU_EXC_HOOK_EN > 0
        if (bspCpuExcHook(ptcbCur, ulRetAddr, ulAbortAddr, ARCH_INSTRUCTION_EXCEPTION, uiRawAbtType)) {
            return;
        }
#endif
    }

    API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archUndHandle
** 功能描述: archUndEntry 需要调用此函数处理未定义指令
** 输　入  : ulAddr           对应的地址
**           uiCpsr           产生异常时的 CPSR
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  archUndHandle (addr_t  ulAddr, UINT32  uiCpsr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;
    
#if LW_CFG_GDB_EN > 0
    UINT    uiBpType = archDbgTrapType(ulAddr, (PVOID)uiCpsr);          /*  断点指令探测                */
    if (uiBpType) {
        if (API_DtraceBreakTrap(ulAddr, uiBpType) == ERROR_NONE) {      /*  进入调试接口断点处理        */
            return;
        }
    }
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */
    
    LW_TCB_GET_CUR(ptcbCur);
    
#if LW_CFG_CPU_FPU_EN > 0
    if (archFpuUndHandle(ptcbCur) == ERROR_NONE) {                      /*  进行 FPU 指令探测           */
        return;
    }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
    
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_UNDEF;
    abtInfo.VMABT_uiMethod = 0;
    
    API_VmmAbortIsr(ulAddr, ulAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archSwiHandle
** 功能描述: archSwiEntry 需要调用此函数处理软中断
** 输　入  : uiSwiNo       软中断号
**           puiRegs       寄存器组指针, 前 14 项为 R0-R12 LR 后面的项为超过 4 个参数的压栈项.
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
** 注  意  : 以下代码为事例代码, SylixOS 目前未使用.
*********************************************************************************************************/
VOID  archSwiHandle (UINT32  uiSwiNo, UINT32  *puiRegs)
{
#ifdef __SYLIXOS_DEBUG
    UINT32  uiArg[10];
    
    uiArg[0] = puiRegs[0];                                              /*  前四个参数使用 R0-R4 传递   */
    uiArg[1] = puiRegs[1];
    uiArg[2] = puiRegs[2];
    uiArg[3] = puiRegs[3];
    
    uiArg[4] = puiRegs[0 + 14];                                         /*  后面的参数为堆栈传递        */
    uiArg[5] = puiRegs[1 + 14];
    uiArg[6] = puiRegs[2 + 14];
    uiArg[7] = puiRegs[3 + 14];
    uiArg[8] = puiRegs[4 + 14];
    uiArg[9] = puiRegs[5 + 14];
#endif
    
    (VOID)uiSwiNo;
    puiRegs[0] = 0x0;                                                   /*  R0 为返回值                 */
}

#endif                                                                  /*  !__SYLIXOS_ARM_ARCH_M__     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
