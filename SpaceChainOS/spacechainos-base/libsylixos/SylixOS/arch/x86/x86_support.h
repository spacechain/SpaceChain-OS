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
** 文   件   名: x86_support.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 06 月 25 日
**
** 描        述: x86 体系构架支持.
*********************************************************************************************************/

#ifndef __ARCH_X86_SUPPORT_H
#define __ARCH_X86_SUPPORT_H

#define __LW_SCHEDULER_BUG_TRACE_EN                                     /*  测试多核调度器              */
#define __LW_KERNLOCK_BUG_TRACE_EN                                      /*  测试内核锁                  */

/*********************************************************************************************************
  汇编相关头文件
*********************************************************************************************************/

#include "arch/assembler.h"

/*********************************************************************************************************
  存储器定义 (CPU 栈区操作)
*********************************************************************************************************/

#define CPU_STK_GROWTH              1                                   /*  1：入栈从高地址向低地址     */
                                                                        /*  0：入栈从低地址向高地址     */
/*********************************************************************************************************
  arch 已经提供的接口如下:
*********************************************************************************************************/

#define __ARCH_KERNEL_PARAM         archKernelParam

VOID    archKernelParam(CPCHAR  pcParam);

/*********************************************************************************************************
  x86 处理器断言
*********************************************************************************************************/

VOID    archAssert(INT  iCond, CPCHAR  pcFunc, CPCHAR  pcFile, INT  iLine);

/*********************************************************************************************************
  x86 处理器线程上下文相关接口
*********************************************************************************************************/

PLW_STACK       archTaskCtxCreate(PTHREAD_START_ROUTINE  pfuncTask,
                                  PVOID                  pvArg,
                                  PLW_STACK              pstkTop, 
                                  ULONG                  ulOpt);
VOID            archTaskCtxSetFp(PLW_STACK  pstkDest, PLW_STACK  pstkSrc);
ARCH_REG_CTX   *archTaskRegsGet(PLW_STACK  pstkTop, ARCH_REG_T *pregSp);
VOID            archTaskRegsSet(PLW_STACK  pstkTop, const ARCH_REG_CTX  *pregctx);

#if LW_CFG_DEVICE_EN > 0
VOID        archTaskCtxShow(INT  iFd, PLW_STACK  pstkTop);
#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
VOID        archTaskCtxPrint(PVOID  pvBuffer, size_t  stSize, PLW_STACK  pstkTop);

VOID        archTaskCtxStart(PLW_CLASS_CPU  pcpuSw);
VOID        archTaskCtxSwitch(PLW_CLASS_CPU  pcpuSw);

#if LW_CFG_COROUTINE_EN > 0
VOID        archCrtCtxSwitch(PLW_CLASS_CPU  pcpuSw);
#endif                                                                  /*  LW_CFG_COROUTINE_EN > 0     */

VOID        archIntCtxLoad(PLW_CLASS_CPU  pcpuSw);
VOID        archSigCtxLoad(PVOID  pvStack);

/*********************************************************************************************************
  x86 处理器调试接口
*********************************************************************************************************/

#if LW_CFG_GDB_EN > 0
#define X86_DBG_TRAP_BP     (PVOID)0
#define X86_DBG_TRAP_STEP   (PVOID)1

VOID    archDbgBpInsert(addr_t   ulAddr, size_t stSize, ULONG  *pulIns, BOOL  bLocal);
VOID    archDbgAbInsert(addr_t   ulAddr, ULONG  *pulIns);
VOID    archDbgBpRemove(addr_t   ulAddr, size_t stSize, ULONG   ulIns, BOOL  bLocal);
VOID    archDbgBpPrefetch(addr_t ulAddr);
UINT    archDbgTrapType(addr_t   ulAddr, PVOID   pvArch);
VOID    archDbgBpAdjust(PVOID  pvDtrace, PVOID   pvtm);
VOID    archDbgSetStepMode(ARCH_REG_CTX  *pregctx, BOOL  bEnable);
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */

/*********************************************************************************************************
  x86 处理器异常接口
*********************************************************************************************************/

VOID    archIntHandle(ULONG  ulVector, BOOL  bPreemptive);              /*  bspIntHandle 需要调用此函数 */

/*********************************************************************************************************
  x86 通用库
*********************************************************************************************************/

INT     archFindLsb(UINT32 ui32);
INT     archFindMsb(UINT32 ui32);

/*********************************************************************************************************
  x86 处理器标准底层库
*********************************************************************************************************/

#define	KN_INT_DISABLE()            archIntDisable()
#define	KN_INT_ENABLE(intLevel)     archIntEnable(intLevel)
#define KN_INT_ENABLE_FORCE()       archIntEnableForce()

INTREG  archIntDisable(VOID);
VOID    archIntEnable(INTREG  iregInterLevel);
VOID    archIntEnableForce(VOID);

VOID    archPageCopy(PVOID pvTo, PVOID pvFrom);

#define KN_COPY_PAGE(to, from)      archPageCopy(to, from)

VOID    archReboot(INT  iRebootType, addr_t  ulStartAddress);

/*********************************************************************************************************
  x86 处理器 BogoMIPS 循环
*********************************************************************************************************/

VOID    archBogoMipsLoop(ULONG  ulLoop);

#define __ARCH_BOGOMIPS_LOOP            archBogoMipsLoop
#define __ARCH_BOGOMIPS_INS_PER_LOOP    2

/*********************************************************************************************************
  x86 处理器 CACHE 操作
*********************************************************************************************************/

#define X86_MACHINE_PC      "x86"                                       /*  x86                         */

#if LW_CFG_CACHE_EN > 0
VOID    archCacheReset(CPCHAR     pcMachineName);
VOID    archCacheInit(CACHE_MODE  uiInstruction, CACHE_MODE  uiData, CPCHAR  pcMachineName);

#define __ARCH_CACHE_INIT   archCacheInit
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

/*********************************************************************************************************
  x86 处理器 MMU 操作 (x86Mmu*() 可供给 bsp 使用)
*********************************************************************************************************/

#if LW_CFG_VMM_EN > 0
VOID    archMmuInit(CPCHAR  pcMachineName);

#define __ARCH_MMU_INIT     archMmuInit
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

/*********************************************************************************************************
  x86 处理器多核自旋锁操作
*********************************************************************************************************/

#if LW_CFG_SMP_EN > 0
VOID    archSpinInit(spinlock_t  *psl);
VOID    archSpinDelay(VOID);
VOID    archSpinNotify(VOID);

#define __ARCH_SPIN_INIT    archSpinInit
#define __ARCH_SPIN_DELAY   archSpinDelay
#define __ARCH_SPIN_NOTIFY  archSpinNotify

INT     archSpinLock(spinlock_t  *psl);
INT     archSpinTryLock(spinlock_t  *psl);
INT     archSpinUnlock(spinlock_t  *psl);

#define __ARCH_SPIN_LOCK    archSpinLock
#define __ARCH_SPIN_TRYLOCK archSpinTryLock
#define __ARCH_SPIN_UNLOCK  archSpinUnlock

INT     archSpinLockRaw(spinlock_t  *psl);
INT     archSpinTryLockRaw(spinlock_t  *psl);
INT     archSpinUnlockRaw(spinlock_t  *psl);

#define __ARCH_SPIN_LOCK_RAW    archSpinLockRaw
#define __ARCH_SPIN_TRYLOCK_RAW archSpinTryLockRaw
#define __ARCH_SPIN_UNLOCK_RAW  archSpinUnlockRaw

ULONG   archMpCur(VOID);
VOID    archMpInt(ULONG  ulCPUId);
#endif                                                                  /*  LW_CFG_SMP_EN               */

/*********************************************************************************************************
  x86 内存屏障
*********************************************************************************************************/

#ifdef __GNUC__
#define KN_BARRIER()    __asm__ __volatile__ ("" : : : "memory")

#if LW_CFG_CPU_X86_NO_BARRIER > 0
#define KN_MB()         __asm__ __volatile__ ("nop" : : : "memory")
#define KN_RMB()        __asm__ __volatile__ ("nop" : : : "memory")
#define KN_WMB()        __asm__ __volatile__ ("nop" : : : "memory")
#else
#define KN_MB()         __asm__ __volatile__ ("mfence" : : : "memory")
#define KN_RMB()        __asm__ __volatile__ ("lfence" : : : "memory")
#define KN_WMB()        __asm__ __volatile__ ("sfence" : : : "memory")
#endif                                                                  /*  LW_CFG_CPU_X86_NO_BARRIER   */

#else                                                                   /*  __GNUC__                    */
#define KN_BARRIER()

#define KN_MB()
#define KN_RMB()
#define KN_WMB()
#endif                                                                  /*  !__GNUC__                   */

#if LW_CFG_SMP_EN > 0
#define KN_SMP_MB()     KN_MB()
#define KN_SMP_RMB()    KN_RMB()
#define KN_SMP_WMB()    KN_WMB()
#else
#define KN_SMP_MB()     KN_BARRIER()
#define KN_SMP_RMB()    KN_BARRIER()
#define KN_SMP_WMB()    KN_BARRIER()
#endif                                                                  /*  LW_CFG_SMP_EN               */

/*********************************************************************************************************
  x86 处理器浮点运算器
*********************************************************************************************************/

#define X86_FPU_NONE        "none"
#define X86_FPU_AUTO        "auto"

#if LW_CFG_CPU_FPU_EN > 0
VOID    archFpuPrimaryInit(CPCHAR  pcMachineName, CPCHAR  pcFpuName);
#if LW_CFG_SMP_EN > 0
VOID    archFpuSecondaryInit(CPCHAR  pcMachineName, CPCHAR  pcFpuName);
#endif                                                                  /*  LW_CFG_SMP_EN               */

VOID    archFpuCtxInit(PVOID pvFpuCtx);
VOID    archFpuEnable(VOID);
VOID    archFpuDisable(VOID);
VOID    archFpuSave(PVOID pvFpuCtx);
VOID    archFpuRestore(PVOID pvFpuCtx);

#define __ARCH_FPU_CTX_INIT     archFpuCtxInit
#define __ARCH_FPU_ENABLE       archFpuEnable
#define __ARCH_FPU_DISABLE      archFpuDisable
#define __ARCH_FPU_SAVE         archFpuSave
#define __ARCH_FPU_RESTORE      archFpuRestore

#if LW_CFG_DEVICE_EN > 0
VOID    archFpuCtxShow(INT  iFd, PVOID pvFpuCtx);
#define __ARCH_FPU_CTX_SHOW     archFpuCtxShow
#endif                                                                  /*  #if LW_CFG_DEVICE_EN        */

INT     archFpuUndHandle(PLW_CLASS_TCB  ptcbCur);
#endif                                                                  /*  LW_CFG_CPU_FPU_EN           */

/*********************************************************************************************************
  bsp 需要提供的接口如下:
*********************************************************************************************************/
/*********************************************************************************************************
  x86 处理器中断向量判读
*********************************************************************************************************/

VOID    bspIntInit(VOID);
VOID    bspIntHandle(ULONG  ulVector);

VOID    bspIntVectorEnable(ULONG  ulVector);
VOID    bspIntVectorDisable(ULONG  ulVector);
BOOL    bspIntVectorIsEnable(ULONG  ulVector);

#define __ARCH_INT_VECTOR_ENABLE    bspIntVectorEnable
#define __ARCH_INT_VECTOR_DISABLE   bspIntVectorDisable
#define __ARCH_INT_VECTOR_ISENABLE  bspIntVectorIsEnable

#if LW_CFG_INTER_PRIO > 0
ULONG   bspIntVectorSetPriority(ULONG  ulVector, UINT  uiPrio);
ULONG   bspIntVectorGetPriority(ULONG  ulVector, UINT *puiPrio);

#define __ARCH_INT_VECTOR_SETPRIO   bspIntVectorSetPriority
#define __ARCH_INT_VECTOR_GETPRIO   bspIntVectorGetPriority
#endif                                                                  /*  LW_CFG_INTER_PRIO > 0       */

#if LW_CFG_INTER_TARGET > 0
ULONG   bspIntVectorSetTarget(ULONG  ulVector, size_t  stSize, const PLW_CLASS_CPUSET  pcpuset);
ULONG   bspIntVectorGetTarget(ULONG  ulVector, size_t  stSize, PLW_CLASS_CPUSET  pcpuset);

#define __ARCH_INT_VECTOR_SETTARGET bspIntVectorSetTarget
#define __ARCH_INT_VECTOR_GETTARGET bspIntVectorGetTarget
#endif                                                                  /*  LW_CFG_INTER_TARGET > 0     */

#define X86_INT_MODE_PIC            0                                   /*  PIC Mode                    */
#define X86_INT_MODE_VIRTUAL_WIRE   1                                   /*  Virtual Wire Mode           */
#define X86_INT_MODE_SYMMETRIC_IO   2                                   /*  Symmetric I/O Mode          */
UINT    bspIntModeGet(VOID);

/*********************************************************************************************************
  CPU 定时器时钟
*********************************************************************************************************/

VOID    bspTickInit(VOID);
VOID    bspDelayUs(ULONG ulUs);
VOID    bspDelayNs(ULONG ulNs);
VOID    bspDelay720Ns(VOID);

/*********************************************************************************************************
  TICK 高精度时间修正
*********************************************************************************************************/

#if LW_CFG_TIME_HIGH_RESOLUTION_EN > 0
VOID    bspTickHighResolution(struct timespec *ptv);
#endif                                                                  /*  LW_CFG_TIME_HIGH_...        */

/*********************************************************************************************************
  内核关键位置回调函数
*********************************************************************************************************/

VOID    bspTaskCreateHook(LW_OBJECT_ID  ulId);
VOID    bspTaskDeleteHook(LW_OBJECT_ID  ulId, PVOID  pvReturnVal, PLW_CLASS_TCB  ptcb);
VOID    bspTaskSwapHook(LW_OBJECT_HANDLE  hOldThread, LW_OBJECT_HANDLE  hNewThread);
VOID    bspTaskIdleHook(VOID);
VOID    bspTickHook(INT64  i64Tick);
VOID    bspWdTimerHook(LW_OBJECT_ID  ulId);
VOID    bspTCBInitHook(LW_OBJECT_ID  ulId, PLW_CLASS_TCB   ptcb);
VOID    bspKernelInitHook(VOID);

/*********************************************************************************************************
  系统重启操作 (ulStartAddress 参数用于调试, BSP 可忽略)
*********************************************************************************************************/

VOID    bspReboot(INT  iRebootType, addr_t  ulStartAddress);

/*********************************************************************************************************
  系统关键信息打印 (打印信息不可依赖任何操作系统 api)
*********************************************************************************************************/

VOID    bspDebugMsg(CPCHAR pcMsg);

/*********************************************************************************************************
  BSP 信息
*********************************************************************************************************/

CPCHAR  bspInfoCpu(VOID);
CPCHAR  bspInfoCache(VOID);
CPCHAR  bspInfoPacket(VOID);
CPCHAR  bspInfoVersion(VOID);
ULONG   bspInfoHwcap(VOID);
addr_t  bspInfoRomBase(VOID);
size_t  bspInfoRomSize(VOID);
addr_t  bspInfoRamBase(VOID);
size_t  bspInfoRamSize(VOID);

/*********************************************************************************************************
  x86 处理器 CACHE 操作
*********************************************************************************************************/

/*********************************************************************************************************
  x86 处理器 MMU 操作
*********************************************************************************************************/

#if LW_CFG_VMM_EN > 0
ULONG   bspMmuPgdMaxNum(VOID);
#if LW_CFG_CPU_WORD_LENGHT == 64
ULONG   bspMmuPmdMaxNum(VOID);
ULONG   bspMmuPtsMaxNum(VOID);
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT == 64*/
ULONG   bspMmuPteMaxNum(VOID);
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

/*********************************************************************************************************
  x86 处理器多核操作
*********************************************************************************************************/

#if LW_CFG_SMP_EN > 0
VOID    bspMpInt(ULONG  ulCPUId);
VOID    bspCpuUp(ULONG  ulCPUId);                                       /*  启动一个 CPU                */
VOID    bspCpuUpDone(VOID);

VOID    bspSecondaryCpusUp(VOID);                                       /*  启动所有的 Secondary Cpu    */
INT     bspSecondaryInit(VOID);                                         /*  Secondary Cpu 启动后的初始化*/

#if LW_CFG_SMP_CPU_DOWN_EN > 0
VOID    bspCpuDown(ULONG  ulCPUId);                                     /*  停止一个 CPU                */
#endif                                                                  /*  LW_CFG_SMP_CPU_DOWN_EN > 0  */

#if LW_CFG_CPU_ARCH_SMT > 0
ULONG   bspCpuLogic2Physical(ULONG  ulCPUId);
#endif
#endif                                                                  /*  LW_CFG_SMP_EN               */

VOID    bspCpuIpiVectorInstall(VOID);                                   /*  安装 IPI 向量               */

/*********************************************************************************************************
  x86 处理器 CPU 功耗 (升降主频)
*********************************************************************************************************/

#if LW_CFG_POWERM_EN > 0
VOID    bspSuspend(VOID);                                               /*  系统休眠                    */
VOID    bspCpuPowerSet(UINT  uiPowerLevel);                             /*  设置 CPU 主频等级           */
VOID    bspCpuPowerGet(UINT *puiPowerLevel);                            /*  获取 CPU 主频等级           */
#endif                                                                  /*  LW_CFG_POWERM_EN > 0        */

/*********************************************************************************************************
  x86 体系结构的初始化函数
*********************************************************************************************************/

VOID    x86ExceptIrqInit(VOID);

INT     x86GdtInit(VOID);
INT     x86GdtSecondaryInit(VOID);

INT     x86TssInit(VOID);
INT     x86TssSecondaryInit(VOID);

INT     x86IdtInit(VOID);
INT     x86IdtSecondaryInit(VOID);
INT     x86IdtSetHandler(UINT8    ucX86Vector,
                         addr_t   ulHandlerAddr,
                         INT      iLowestPriviledge);

VOID    x86CpuIdProbe(VOID);
VOID    x86CpuIdShow(VOID);

VOID    x86CpuTopologyShow(VOID);
INT     x86MpApicDataShow(VOID);
VOID    x86MpBiosShow(VOID);
VOID    x86MpBiosIoIntMapShow(VOID);
VOID    x86MpBiosLocalIntMapShow(VOID);
VOID    x86MpBiosLocalIntMapShow(VOID);

BOOL    x86AcpiAvailable(VOID);
INT     x86AcpiInit(VOID);
VOID    x86AcpiMpTableShow(VOID);

/*********************************************************************************************************
  x86_64 特殊操作
*********************************************************************************************************/

#if LW_CFG_CPU_WORD_LENGHT == 64
ULONG  *x64BootstrapPageTbl(VOID);
#endif

/*********************************************************************************************************
  x86 指令宏定义
*********************************************************************************************************/

#ifdef __GNUC__
#if LW_CFG_CPU_X86_NO_PAUSE > 0
#define X86_PAUSE()
#else
#define X86_PAUSE()                 __asm__ __volatile__ ("pause" : : : "memory")
#endif                                                                  /*  LW_CFG_CPU_X86_NO_PAUSE     */
#if LW_CFG_CPU_X86_NO_HLT > 0
#define X86_HLT()
#else
#define X86_HLT()                   __asm__ __volatile__ ("hlt" : : : "memory")
#endif                                                                  /*  LW_CFG_CPU_X86_NO_HLT       */
#define X86_WBINVD()                __asm__ __volatile__ ("wbinvd" : : : "memory")
#else
#define X86_PAUSE()
#define X86_HLT()
#define X86_WBINVD()
#endif

/*********************************************************************************************************
  Trusted computing
*********************************************************************************************************/

#if LW_CFG_TRUSTED_COMPUTING_EN > 0
VOID    bspTrustedModuleInit(VOID);
INT     bspTrustedModuleCheck(const PCHAR  pcPath);
VOID    bspTrustedModuleLoad(PVOID  pvModule);
VOID    bspTrustedModuleUnload(PVOID  pvModule);
#endif                                                                  /*  LW_CFG_TRUSTED_COMPUTING_EN */

#endif                                                                  /*  __ARCH_X86_SUPPORT_H        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
