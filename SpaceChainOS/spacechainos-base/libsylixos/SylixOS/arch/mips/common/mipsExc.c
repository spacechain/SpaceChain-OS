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
** 文   件   名: mipsExc.c
**
** 创   建   人: Ryan.Xin (信金龙)
**
** 文件创建日期: 2015 年 09 月 01 日
**
** 描        述: MIPS 体系构架异常处理.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "dtrace.h"
#include "arch/mips/common/cp0/mipsCp0.h"
#include "arch/mips/common/unaligned/mipsUnaligned.h"
#if LW_CFG_VMM_EN > 0
#include "arch/mips/mm/mmu/mipsMmuCommon.h"
#endif
/*********************************************************************************************************
  外部函数声明
*********************************************************************************************************/
extern VOID    archTaskCtxPrint(PVOID  pvBuffer, size_t  stSize, PLW_STACK  pstkTop);
#if LW_CFG_CPU_FPU_EN > 0
extern UINT32  mipsVfp32GetFEXR(VOID);
extern VOID    mipsVfp32ClearFEXR(VOID);
extern int     fpu_emulator_cop1Handler(ARCH_REG_CTX *xcp, ARCH_FPU_CTX *ctx,
                                        int has_fpu, void **fault_addr);
extern int     do_dsemulret(PLW_CLASS_TCB ptcbCur, ARCH_REG_CTX *xcp);
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
/*********************************************************************************************************
  FPU 上下文(用于 FPU 模拟)
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0
ARCH_FPU_CTX   _G_mipsFpuCtx[LW_CFG_MAX_PROCESSORS];
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
/*********************************************************************************************************
  MIPS 异常处理函数类型
*********************************************************************************************************/
typedef VOID  (*MIPS_EXCEPT_HANDLE)(addr_t  ulRetAddr, addr_t  ulAbortAddr);
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
** 函数名称: archCacheErrorHandle
** 功能描述: Cache 错误处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0

VOID  archCacheErrorHandle (addr_t  ulRetAddr)
{
    REGISTER UINT32         uiFiled  = 2 * sizeof(UINT32);
    REGISTER UINT32         uiRegVal;
             PLW_CLASS_TCB  ptcbCur;
             LW_VMM_ABORT   abtInfo;

    uiRegVal = mipsCp0ConfigRead();
    mipsCp0ConfigWrite((uiRegVal & ~M_ConfigK0) | MIPS_UNCACHED);

    _PrintFormat("Cache error exception:\r\n");
    _PrintFormat("cp0_error epc == %lx\r\n", uiFiled, mipsCp0ERRPCRead());

    uiRegVal = mipsCp0CacheErrRead();

    _PrintFormat("cp0_cache error == 0x%08x\r\n", uiRegVal);
    _PrintFormat("Decoded cp0_cache error: %s cache fault in %s reference.\r\n",
                 (uiRegVal & M_CacheLevel) ? "secondary" : "primary",
                 (uiRegVal & M_CacheType)  ? "d-cache"   : "i-cache");

    _PrintFormat("Error bits: %s%s%s%s%s%s%s\r\n",
                 (uiRegVal & M_CacheData) ? "ED " : "",
                 (uiRegVal & M_CacheTag)  ? "ET " : "",
                 (uiRegVal & M_CacheECC)  ? "EE " : "",
                 (uiRegVal & M_CacheBoth) ? "EB " : "",
                 (uiRegVal & M_CacheEI)   ? "EI " : "",
                 (uiRegVal & M_CacheE1)   ? "E1 " : "",
                 (uiRegVal & M_CacheE0)   ? "E0 " : "");

    _PrintFormat("IDX: 0x%08x\r\n", uiRegVal & (M_CacheE0 - 1));

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = 0;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_FATAL_ERROR;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
** 函数名称: archTlbModExceptHandle
** 功能描述: TLB modified 异常处理
** 输　入  : ulRetAddr     返回地址
**           ulAbortAddr   终止地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  archTlbModExceptHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_WRITE;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_PERM;
    API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archTlbLoadExceptHandle
** 功能描述: TLB load or ifetch 异常处理
** 输　入  : ulRetAddr     返回地址
**           ulAbortAddr   终止地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  archTlbLoadExceptHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_READ;
#if LW_CFG_VMM_EN > 0
    abtInfo.VMABT_uiType   = mipsMmuTlbLoadStoreExcHandle(ulAbortAddr);
#else
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_TERMINAL;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
    if (abtInfo.VMABT_uiType) {
        API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
    }
}
/*********************************************************************************************************
** 函数名称: archTlbStoreExceptHandle
** 功能描述: TLB store 异常处理
** 输　入  : ulRetAddr     返回地址
**           ulAbortAddr   终止地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  archTlbStoreExceptHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_WRITE;
#if LW_CFG_VMM_EN > 0
    abtInfo.VMABT_uiType   = mipsMmuTlbLoadStoreExcHandle(ulAbortAddr);
#else
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_TERMINAL;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
    if (abtInfo.VMABT_uiType) {
        API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
    }
}
/*********************************************************************************************************
** 函数名称: archAddrLoadExceptHandle
** 功能描述: Address load or ifetch 异常处理
** 输　入  : ulRetAddr     返回地址
**           ulAbortAddr   终止地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  archAddrLoadExceptHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_READ;
    abtInfo.VMABT_uiType   = mipsUnalignedHandle((ARCH_REG_CTX *)ptcbCur->TCB_pstkStackNow,
                                                 ulAbortAddr);
    if (abtInfo.VMABT_uiType) {
        API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
    }
}
/*********************************************************************************************************
** 函数名称: archAddrStoreExceptHandle
** 功能描述: Address store 异常处理
** 输　入  : ulRetAddr     返回地址
**           ulAbortAddr   终止地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  archAddrStoreExceptHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_WRITE;
    abtInfo.VMABT_uiType   = mipsUnalignedHandle((ARCH_REG_CTX *)ptcbCur->TCB_pstkStackNow,
                                                 ulAbortAddr);
    if (abtInfo.VMABT_uiType) {
        API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
    }
}
/*********************************************************************************************************
** 函数名称: archInstBusExceptHandle
** 功能描述: Instruction Bus 异常处理
** 输　入  : ulRetAddr     返回地址
**           ulAbortAddr   终止地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  archInstBusExceptHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_CPU_EXC_HOOK_EN > 0
    if (bspCpuExcHook(ptcbCur, ulRetAddr, ulAbortAddr, ARCH_BUS_EXCEPTION, 0)) {
        return;
    }
#endif

    abtInfo.VMABT_uiMethod = BUS_ADRERR;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_BUS;
    API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archDataBusExceptHandle
** 功能描述: Data Bus 异常处理
** 输　入  : ulRetAddr     返回地址
**           ulAbortAddr   终止地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  archDataBusExceptHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_CPU_EXC_HOOK_EN > 0
    if (bspCpuExcHook(ptcbCur, ulRetAddr, ulAbortAddr, ARCH_BUS_EXCEPTION, 1)) {
        return;
    }
#endif

    abtInfo.VMABT_uiMethod = BUS_ADRERR;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_BUS;
    API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archSysCallHandle
** 功能描述: System Call 处理
** 输　入  : ulRetAddr     返回地址
**           ulAbortAddr   终止地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  archSysCallHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = 0;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_SYS;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archBreakPointHandle
** 功能描述: Break Point 处理
** 输　入  : ulRetAddr     返回地址
**           ulAbortAddr   终止地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  archBreakPointHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;
#if LW_CFG_GDB_EN > 0
    UINT           uiBpType;
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_CPU_FPU_EN > 0
    if (*(MIPS_INSTRUCTION *)ulRetAddr == BRK_MEMU) {
        if (do_dsemulret(ptcbCur, (ARCH_REG_CTX *)ptcbCur->TCB_pstkStackNow) == ERROR_NONE) {
            return;                                                     /*  FPU 模拟返回                */
        }
    }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

#if LW_CFG_GDB_EN > 0
    uiBpType = archDbgTrapType(ulRetAddr, LW_NULL);                     /*  断点指令探测                */
    if (uiBpType) {
        if (API_DtraceBreakTrap(ulRetAddr, uiBpType) == ERROR_NONE) {
            return;                                                     /*  进入调试接口断点处理        */
        }
    }
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */

    abtInfo.VMABT_uiMethod = 0;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_BREAK;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archTrapInstHandle
** 功能描述: Trap instruction 处理
** 输　入  : ulRetAddr     返回地址
**           ulAbortAddr   终止地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  archTrapInstHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr)
{
    archBreakPointHandle(ulRetAddr, ulRetAddr);
}
/*********************************************************************************************************
** 函数名称: archResvInstHandle
** 功能描述: Reserved instruction 处理
** 输　入  : ulRetAddr     返回地址
**           ulAbortAddr   终止地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  archResvInstHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_UNDEF;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archFloatPointExceptHandle
** 功能描述: 浮点异常处理
** 输　入  : ulRetAddr     返回地址
**           ulAbortAddr   终止地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  archFloatPointExceptHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;
    UINT32         uiFEXR;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_FPE;                       /*  终止类型默认为 FPU 异常     */

#if LW_CFG_CPU_FPU_EN > 0
    uiFEXR = mipsVfp32GetFEXR();                                        /*  获得浮点异常类型            */
    if (uiFEXR & (1 << 17)) {                                           /*  未实现异常                  */
        PVOID           pvFaultAddr;
        INT             iSignal;
        ARCH_FPU_CTX   *pFpuCtx;

        pFpuCtx = &_G_mipsFpuCtx[LW_CPU_GET_CUR_ID()];
        __ARCH_FPU_SAVE(pFpuCtx);                                       /*  保存当前 FPU CTX            */

        iSignal = fpu_emulator_cop1Handler((ARCH_REG_CTX *)ptcbCur->TCB_pstkStackNow,
                                           (ARCH_FPU_CTX *)pFpuCtx,
                                           LW_TRUE, &pvFaultAddr);      /*  FPU 模拟                    */
        switch (iSignal) {

        case 0:                                                         /*  成功模拟                    */
            abtInfo.VMABT_uiType = 0;
            __ARCH_FPU_RESTORE(pFpuCtx);                                /*  恢复当前 FPU CTX            */
            break;

        case SIGILL:                                                    /*  未定义指令                  */
            abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
            abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_UNDEF;
            ulAbortAddr            = ulRetAddr;
            break;

        case SIGBUS:                                                    /*  总线错误                    */
            abtInfo.VMABT_uiMethod = BUS_ADRERR;
            abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_BUS;
            break;

        case SIGSEGV:                                                   /*  地址不合法                  */
            abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_WRITE;
            abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_TERMINAL;
            ulAbortAddr = (ULONG)pvFaultAddr;
            break;

        case SIGFPE:                                                    /*  FPU 错误                    */
        default:
            abtInfo.VMABT_uiMethod = 0;
            break;
        }

    } else if (uiFEXR & (1 << 16)) {                                    /*  非法操作异常                */
        abtInfo.VMABT_uiMethod = FPE_FLTINV;

    } else if (uiFEXR & (1 << 15)) {                                    /*  除零异常                    */
        abtInfo.VMABT_uiMethod = FPE_FLTDIV;

    } else if (uiFEXR & (1 << 14)) {                                    /*  上溢异常                    */
        abtInfo.VMABT_uiMethod = FPE_FLTOVF;

    } else if (uiFEXR & (1 << 13)) {
        abtInfo.VMABT_uiMethod = FPE_FLTUND;                            /*  下溢异常                    */

    } else if (uiFEXR & (1 << 12)) {                                    /*  不精确异常                  */
        abtInfo.VMABT_uiMethod = FPE_FLTRES;

    } else {
        abtInfo.VMABT_uiMethod = 0;
    }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

    if (abtInfo.VMABT_uiType) {
        API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
    }

#if LW_CFG_CPU_FPU_EN > 0
    mipsVfp32ClearFEXR();                                               /*  清除浮点异常                */
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
}
/*********************************************************************************************************
** 函数名称: archCoProcUnusableExceptHandle
** 功能描述: 协处理器不可用异常处理
** 输　入  : ulRetAddr     返回地址
**           ulAbortAddr   终止地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  archCoProcUnusableExceptHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_CPU_FPU_EN > 0
    if (archFpuUndHandle(ptcbCur) == ERROR_NONE) {                      /*  进行 FPU 指令探测           */
        return;
    }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_UNDEF;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archDefaultExceptHandle
** 功能描述: 缺省的异常处理
** 输　入  : ulRetAddr     返回地址
**           ulAbortAddr   终止地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  archDefaultExceptHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = 0;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_TERMINAL;
    API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archMachineCheckExceptHandle
** 功能描述: 机器检查异常处理
** 输　入  : ulRetAddr     返回地址
**           ulAbortAddr   终止地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  archMachineCheckExceptHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = 0;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_FATAL_ERROR;
    API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
  MIPS 异常处理函数数组
*********************************************************************************************************/
static MIPS_EXCEPT_HANDLE   _G_mipsExceptHandle[32] = {
    [EX_INT]      = (MIPS_EXCEPT_HANDLE)bspIntHandle,                   /*  Interrupt                   */
    [EX_MOD]      = archTlbModExceptHandle,                             /*  TLB modified                */
    [EX_TLBL]     = archTlbLoadExceptHandle,                            /*  TLB exception(load or ifetch*/
    [EX_TLBS]     = archTlbStoreExceptHandle,                           /*  TLB exception (store)       */
    [EX_ADEL]     = archAddrLoadExceptHandle,                           /*  Address error(load or ifetch*/
    [EX_ADES]     = archAddrStoreExceptHandle,                          /*  Address error (store)       */
    [EX_IBE]      = archInstBusExceptHandle,                            /*  Instruction Bus Error       */
    [EX_DBE]      = archDataBusExceptHandle,                            /*  Data Bus Error              */
    [EX_SYS]      = archSysCallHandle,                                  /*  Syscall                     */
    [EX_BP]       = archBreakPointHandle,                               /*  Breakpoint                  */
    [EX_TR]       = archTrapInstHandle,                                 /*  Trap instruction            */
    [EX_RI]       = archResvInstHandle,                                 /*  Reserved instruction        */
    [EX_FPE]      = archFloatPointExceptHandle,                         /*  floating point exception    */
    [EX_CPU]      = archCoProcUnusableExceptHandle,                     /*  CoProcessor Unusable        */
    [EX_OV]       = archDefaultExceptHandle,                            /*  Overflow                    */
    [EX_C2E]      = archDefaultExceptHandle,                            /*  COP2 exception              */
    [EX_MDMX]     = archDefaultExceptHandle,                            /*  MDMX exception              */
    [EX_WATCH]    = archDefaultExceptHandle,                            /*  Watch exception             */
    [EX_MCHECK]   = archMachineCheckExceptHandle,                       /*  Machine check exception     */
    [EX_THREAD]   = archDefaultExceptHandle,                            /*  Thread exception            */
    [EX_DSP]      = archDefaultExceptHandle,                            /*  DSP Unusable                */
    [EX_CacheErr] = (MIPS_EXCEPT_HANDLE)archCacheErrorHandle,           /*  Cache error caused re-entry */
                                                                        /*  to Debug Mode               */
};
/*********************************************************************************************************
** 函数名称: archExceptionHandle
** 功能描述: 通用异常处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archExceptionHandle (addr_t  ulRetAddr, UINT32  uiCause, addr_t  ulAbortAddr, UINT32  uiSp)
{
    UINT32              uiExcCode = ((uiCause & M_CauseExcCode) >> S_CauseExcCode);
    ULONG               ulNesting;
    MIPS_EXCEPT_HANDLE  pfuncExceptHandle;

    if (uiExcCode == EX_INT) {                                          /*  优化普通中断处理            */
        bspIntHandle();
        return;
    }

    ulNesting = LW_CPU_GET_CUR_NESTING();
    if (ulNesting > 1) {                                                /*  异常出现嵌套                */
        archTaskCtxPrint(LW_NULL, 0, (PLW_STACK)uiSp);                  /*  直接打印寄存器              */
    }

    pfuncExceptHandle = _G_mipsExceptHandle[uiExcCode];
    if (pfuncExceptHandle == LW_NULL) {
        _BugFormat(LW_TRUE, LW_TRUE, "Unknow exception: %d\r\n", uiExcCode);
    } else {
        pfuncExceptHandle(ulRetAddr, ulAbortAddr);
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
