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
** 文   件   名: KernelReboot.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 03 月 02 日
**
** 描        述: 内核重新启动函数.

** BUG:
2009.05.05  需要禁能 CACHE 以适应 bootloader 的工作.
2009.05.31  必须在 except 线程执行, 而且需要锁住内核.
2009.06.24  API_KernelRebootEx() 启动地址中的局部变量可能会被一些粗心的 BSP CACHE 函数改写, 这里设置全局
            变量保护.
2011.03.08  加入 c++ 运行时卸载函数.
2012.03.26  加入 __LOGMESSAGE_LEVEL 信息打印.
2012.11.09  加入系统重新启动类型定义, 当遇到严重错误时不调用回调并立即重新启动.
2013.12.03  多核系统重启时, 需要将 LW_NCPUS - 1 个 idle 线程优先级提到最高, 抢占其他核的 CPU 然后实际上
            成为单核系统执行复位重启的操作.
2014.07.21  重启时, 自动关闭其他 CPU.
2016.12.27  重启带有超时强制重启操作.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  系统函数声明
*********************************************************************************************************/
INT  _excJobAdd(VOIDFUNCPTR  pfunc, 
                PVOID        pvArg0,
                PVOID        pvArg1,
                PVOID        pvArg2,
                PVOID        pvArg3,
                PVOID        pvArg4,
                PVOID        pvArg5);
VOID _cppRtUninit(VOID);
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static INT              _K_iRebootType;                                 /*  重启类型                    */
static addr_t           _K_ulRebootStartAddress;                        /*  重新引导地址                */
static LW_HOOK_FUNC     _K_pfuncTickHookOld;                            /*  记录的 TICK HOOK            */
static ULONG            _K_ulTickTimeout;                               /*  重启超时                    */
/*********************************************************************************************************
** 函数名称: __makeOtherDown
** 功能描述: 关闭其他 CPU. (LW_NCPUS 必须大于 1)
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
#if LW_CFG_SMP_CPU_DOWN_EN > 0

static VOID  __makeOtherDown (VOID)
{
    ULONG   i;
    BOOL    bNeedWait;
    
    LW_CPU_FOREACH_ACTIVE_EXCEPT (i, 0) {                               /*  除 0 以外的其他 CPU         */
        API_CpuDown(i);
    }
    
    do {
        bNeedWait = LW_FALSE;
        LW_CPU_FOREACH_EXCEPT (i, 0) {
            if (API_CpuIsUp(i)) {                                       /*  确保除 0 核外, 其他 CPU 全关*/
                bNeedWait = LW_TRUE;
            }
        }
        LW_SPINLOCK_DELAY();
    } while (bNeedWait);
}

#else

static VOID  __makeOtherDown (VOID)
{
    ULONG           i;
    PLW_CLASS_TCB   ptcbIdle;
    BOOL            bRunning;
    
    LW_CPU_FOREACH_EXCEPT (i, 0) {                                      /*  除 0 以外的其他 CPU         */
        ptcbIdle = _K_ptcbIdle[i];
        if (!LW_PRIO_IS_EQU(ptcbIdle->TCB_ucPriority, LW_PRIO_HIGHEST)) {
            __KERNEL_ENTER();                                           /*  进入内核                    */
            _SchedSetPrio(ptcbIdle, LW_PRIO_HIGHEST);
            __KERNEL_EXIT();                                            /*  退出内核                    */
            
            do {
                API_TimeSleep(LW_OPTION_ONE_TICK);
                __KERNEL_ENTER();                                       /*  进入内核                    */
                bRunning = __LW_THREAD_IS_RUNNING(ptcbIdle);
                __KERNEL_EXIT();                                        /*  退出内核                    */
            } while (!bRunning);
        }
    }
}

#endif                                                                  /*  LW_CFG_SMP_CPU_DOWN_EN      */
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
** 函数名称: __kernelRebootToIsr
** 功能描述: REBOOT 重启超时处理
** 输　入  : i64Tick   当前 tick
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __kernelRebootToIsr (INT64   i64Tick)
{
    if (_K_ulTickTimeout) {
        _K_ulTickTimeout--;
        if (_K_ulTickTimeout == 0) {
            if (_K_iRebootType == LW_REBOOT_SHUTDOWN) {
                archReboot(LW_REBOOT_SHUTDOWN, _K_ulRebootStartAddress);
            } else {
                archReboot(LW_REBOOT_FORCE, _K_ulRebootStartAddress);
            }
        }
    }

    if (_K_pfuncTickHookOld) {
        _K_pfuncTickHookOld(i64Tick);
    }
}
/*********************************************************************************************************
** 函数名称: __kernelRebootToSet
** 功能描述: 设置 REBOOT 超时处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __kernelRebootToSet (VOID)
{
    if (_K_pfuncTickHookOld == LW_NULL) {
        _K_pfuncTickHookOld =  API_KernelHookGet(LW_OPTION_THREAD_TICK_HOOK);
        API_KernelHookSet(__kernelRebootToIsr, LW_OPTION_THREAD_TICK_HOOK);
        _K_ulTickTimeout = LW_REBOOT_TO_SEC * LW_TICK_HZ;
    }
}
/*********************************************************************************************************
** 函数名称: API_KernelReboot
** 功能描述: 内核重新启动函数
** 输　入  : iRebootType        重启类型 
                                LW_REBOOT_FORCE
                                LW_REBOOT_WARM
                                LW_REBOOT_COLD
                                LW_REBOOT_SHUTDOWN
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID   API_KernelReboot (INT  iRebootType)
{
    API_KernelRebootEx(iRebootType, 0ull);
}
/*********************************************************************************************************
** 函数名称: API_KernelRebootEx
** 功能描述: 内核重新启动函数
** 输　入  : iRebootType        重启类型 
                                LW_REBOOT_FORCE
                                LW_REBOOT_WARM
                                LW_REBOOT_COLD
                                LW_REBOOT_SHUTDOWN
**           ulStartAddress     启动地址
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID   API_KernelRebootEx (INT  iRebootType, addr_t  ulStartAddress)
{
#if LW_CFG_DEVICE_EN > 0
    extern  VOID  sync(VOID);
#endif
    
    INTREG  iregInterLevel;
    ULONG   ulI;
    
    if (iRebootType == LW_REBOOT_FORCE) {
        archReboot(iRebootType, ulStartAddress);                        /*  调用体系架构重启操作        */
        _BugHandle(LW_TRUE, LW_TRUE, "kernel reboot error!\r\n");       /*  不会运行到这里              */
    }

    if (LW_CPU_GET_CUR_NESTING() || (API_ThreadIdSelf() != API_KernelGetExc())) {
        _excJobAdd(API_KernelRebootEx, (PVOID)iRebootType, (PVOID)ulStartAddress, 0, 0, 0, 0);
        return;
    }

    _DebugHandle(__PRINTMESSAGE_LEVEL, "kernel rebooting...\r\n");
    
    _K_iRebootType          = iRebootType;
    _K_ulRebootStartAddress = ulStartAddress;                           /*  记录局部变量, 防止 XXX      */
    
    __kernelRebootToSet();                                              /*  设置重启超时处理            */
    
#if LW_CFG_DEVICE_EN > 0
    sync();                                                             /*  执行回写操作                */
#endif

#if LW_CFG_SMP_EN > 0
    if (LW_NCPUS > 1) {
        __makeOtherDown();                                              /*  将其他 CPU 设置为 idle 模式 */
    }
#endif                                                                  /*  LW_CFG_SMP_EN               */
    
    if (iRebootType != LW_REBOOT_FORCE) {
        _cppRtUninit();                                                 /*  卸载 C++ 运行时             */
        __LW_KERNEL_REBOOT_HOOK(iRebootType);                           /*  调用回调函数                */
    }
    
    for (ulI = 0; ulI < LW_CFG_MAX_INTER_SRC; ulI++) {
        API_InterVectorDisable(ulI);                                    /*  关闭所有中断                */
    }
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  进入内核同时关闭中断        */

#if LW_CFG_CACHE_EN > 0
    if (LW_NCPUS <= 1) {
        API_CacheDisable(DATA_CACHE);                                   /*  禁能 CACHE                  */
        API_CacheDisable(INSTRUCTION_CACHE);
    }
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

#if LW_CFG_VMM_EN > 0
    if (LW_NCPUS <= 1) {
        API_VmmMmuDisable();                                            /*  关闭 MMU                    */
    }
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

    if (iRebootType == LW_REBOOT_SHUTDOWN) {
        _DebugHandle(__PRINTMESSAGE_LEVEL, "you can power off now.\r\n");
    
    } else {
        _DebugHandle(__PRINTMESSAGE_LEVEL, "you can reboot now.\r\n");
    }
    
    archReboot(iRebootType, _K_ulRebootStartAddress);                   /*  调用体系架构重启操作        */
    
    _BugHandle(LW_TRUE, LW_TRUE, "kernel reboot error!\r\n");           /*  不会运行到这里              */
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  退出内核同时打开中断        */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
