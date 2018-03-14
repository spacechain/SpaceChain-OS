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
** 文   件   名: KernelHook.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 14 日
**
** 描        述: 获得内核指定 HOOK

** BUG
2008.03.02  加入了 reboot 的回调函数.
2011.07.29  加入了新的回调函数类型, 包括内核对象和文件的创建与回收.
2012.07.04  合并 API_KernelHookSet() 到这里
2012.09.22  加入诸多电源管理的 HOOK.
2013.03.16  加入进程回调.
2014.08.10  加入系统错误回调.
2015.11.21  修正 API_KernelHookGet() 类型.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_KernelHookGet
** 功能描述: 获得内核指定 HOOK
** 输　入  : ulOpt                         HOOK 类型
** 输　出  : HOOK 功能函数
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
LW_HOOK_FUNC  API_KernelHookGet (ULONG  ulOpt)
{
    INTREG         iregInterLevel;
    LW_HOOK_FUNC   hookfuncPtr;
        
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  进入内核同时关闭中断        */
    
    switch (ulOpt) {
    
    case LW_OPTION_THREAD_CREATE_HOOK:                                  /*  线程建立钩子                */
        hookfuncPtr = _K_hookKernel.HOOK_ThreadCreate;
        break;
    
    case LW_OPTION_THREAD_DELETE_HOOK:                                  /*  线程删除钩子                */
        hookfuncPtr = _K_hookKernel.HOOK_ThreadDelete;
        break;
    
    case LW_OPTION_THREAD_SWAP_HOOK:                                    /*  线程切换钩子                */
        hookfuncPtr = _K_hookKernel.HOOK_ThreadSwap;
        break;
    
    case LW_OPTION_THREAD_TICK_HOOK:                                    /*  系统时钟中断钩子            */
        hookfuncPtr = _K_hookKernel.HOOK_ThreadTick;
        break;
    
    case LW_OPTION_THREAD_INIT_HOOK:                                    /*  线程初始化钩子              */
        hookfuncPtr = _K_hookKernel.HOOK_ThreadInit;
        break;
    
    case LW_OPTION_THREAD_IDLE_HOOK:                                    /*  空闲线程钩子                */
        hookfuncPtr = _K_hookKernel.HOOK_ThreadIdle;
        break;
    
    case LW_OPTION_KERNEL_INITBEGIN:                                    /*  内核初始化开始钩子          */
        hookfuncPtr = _K_hookKernel.HOOK_KernelInitBegin;
        break;
    
    case LW_OPTION_KERNEL_INITEND:                                      /*  内核初始化结束钩子          */
        hookfuncPtr = _K_hookKernel.HOOK_KernelInitEnd;
        break;
    
    case LW_OPTION_KERNEL_REBOOT:                                       /*  内核重新启动钩子            */
        hookfuncPtr = _K_hookKernel.HOOK_KernelReboot;
        break;
        
    case LW_OPTION_WATCHDOG_TIMER:                                      /*  看门狗定时器钩子            */
        hookfuncPtr = _K_hookKernel.HOOK_WatchDogTimer;
        break;
    
    case LW_OPTION_OBJECT_CREATE_HOOK:                                  /*  创建内核对象钩子            */
        hookfuncPtr = _K_hookKernel.HOOK_ObjectCreate;
        break;
    
    case LW_OPTION_OBJECT_DELETE_HOOK:                                  /*  删除内核对象钩子            */
        hookfuncPtr = _K_hookKernel.HOOK_ObjectDelete;
        break;
    
    case LW_OPTION_FD_CREATE_HOOK:                                      /*  文件描述符创建钩子          */
        hookfuncPtr = _K_hookKernel.HOOK_FdCreate;
        break;
    
    case LW_OPTION_FD_DELETE_HOOK:                                      /*  文件描述符删除钩子          */
        hookfuncPtr = _K_hookKernel.HOOK_FdDelete;
        break;
    
    case LW_OPTION_CPU_IDLE_ENTER:                                      /*  CPU 进入空闲模式            */
        hookfuncPtr = _K_hookKernel.HOOK_CpuIdleEnter;
        break;
    
    case LW_OPTION_CPU_IDLE_EXIT:                                       /*  CPU 退出空闲模式            */
        hookfuncPtr = _K_hookKernel.HOOK_CpuIdleExit;
        break;
    
    case LW_OPTION_CPU_INT_ENTER:                                       /*  CPU 进入中断(异常)模式      */
        hookfuncPtr = _K_hookKernel.HOOK_CpuIntEnter;
        break;
    
    case LW_OPTION_CPU_INT_EXIT:                                        /*  CPU 退出中断(异常)模式      */
        hookfuncPtr = _K_hookKernel.HOOK_CpuIntExit;
        break;
        
    case LW_OPTION_STACK_OVERFLOW_HOOK:                                 /*  堆栈溢出                    */
        hookfuncPtr = _K_hookKernel.HOOK_StkOverflow;
        break;
    
    case LW_OPTION_FATAL_ERROR_HOOK:                                    /*  致命错误                    */
        hookfuncPtr = _K_hookKernel.HOOK_FatalError;
        break;
        
    case LW_OPTION_VPROC_CREATE_HOOK:                                   /*  进程建立钩子                */
        hookfuncPtr = _K_hookKernel.HOOK_VpCreate;
        break;
    
    case LW_OPTION_VPROC_DELETE_HOOK:                                   /*  进程删除钩子                */
        hookfuncPtr = _K_hookKernel.HOOK_VpDelete;
        break;
    
    default:
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核同时打开中断        */
        _ErrorHandle(ERROR_KERNEL_OPT_NULL);
        return  (LW_NULL);
    }
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  退出内核同时打开中断        */
    return  (hookfuncPtr);
}
/*********************************************************************************************************
** 函数名称: API_KernelHookSet
** 功能描述: 设置内核指定 HOOK
** 输　入  : 
**           hookfuncPtr                   HOOK 功能函数
**           ulOpt                         HOOK 类型
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_KernelHookSet (LW_HOOK_FUNC  hookfuncPtr, ULONG  ulOpt)
{
    INTREG      iregInterLevel;
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  进入内核同时关闭中断        */
    
    switch (ulOpt) {
    
    case LW_OPTION_THREAD_CREATE_HOOK:                                  /*  线程建立钩子                */
        _K_hookKernel.HOOK_ThreadCreate = hookfuncPtr;
        break;
    
    case LW_OPTION_THREAD_DELETE_HOOK:                                  /*  线程删除钩子                */
        _K_hookKernel.HOOK_ThreadDelete = hookfuncPtr;
        break;
    
    case LW_OPTION_THREAD_SWAP_HOOK:                                    /*  线程切换钩子                */
        _K_hookKernel.HOOK_ThreadSwap = hookfuncPtr;
        break;
    
    case LW_OPTION_THREAD_TICK_HOOK:                                    /*  系统时钟中断钩子            */
        _K_hookKernel.HOOK_ThreadTick = hookfuncPtr;
        break;
    
    case LW_OPTION_THREAD_INIT_HOOK:                                    /*  线程初始化钩子              */
        _K_hookKernel.HOOK_ThreadInit = hookfuncPtr;
        break;
    
    case LW_OPTION_THREAD_IDLE_HOOK:                                    /*  空闲线程钩子                */
        _K_hookKernel.HOOK_ThreadIdle = hookfuncPtr;
        break;
    
    case LW_OPTION_KERNEL_INITBEGIN:                                    /*  内核初始化开始钩子          */
        _K_hookKernel.HOOK_KernelInitBegin = hookfuncPtr;
        break;
    
    case LW_OPTION_KERNEL_INITEND:                                      /*  内核初始化结束钩子          */
        _K_hookKernel.HOOK_KernelInitEnd = hookfuncPtr;
        break;
    
    case LW_OPTION_KERNEL_REBOOT:                                       /*  内核重新启动钩子            */
        _K_hookKernel.HOOK_KernelReboot = hookfuncPtr;
        break;
    
    case LW_OPTION_WATCHDOG_TIMER:                                      /*  看门狗定时器钩子            */
        _K_hookKernel.HOOK_WatchDogTimer = hookfuncPtr;
        break;
    
    case LW_OPTION_OBJECT_CREATE_HOOK:                                  /*  创建内核对象钩子            */
        _K_hookKernel.HOOK_ObjectCreate = hookfuncPtr;
        break;
    
    case LW_OPTION_OBJECT_DELETE_HOOK:                                  /*  删除内核对象钩子            */
        _K_hookKernel.HOOK_ObjectDelete = hookfuncPtr;
        break;
    
    case LW_OPTION_FD_CREATE_HOOK:                                      /*  文件描述符创建钩子          */
        _K_hookKernel.HOOK_FdCreate = hookfuncPtr;
        break;
    
    case LW_OPTION_FD_DELETE_HOOK:                                      /*  文件描述符删除钩子          */
        _K_hookKernel.HOOK_FdDelete = hookfuncPtr;
        break;
    
    case LW_OPTION_CPU_IDLE_ENTER:                                      /*  CPU 进入空闲模式            */
        _K_hookKernel.HOOK_CpuIdleEnter = hookfuncPtr;
        break;
    
    case LW_OPTION_CPU_IDLE_EXIT:                                       /*  CPU 退出空闲模式            */
        _K_hookKernel.HOOK_CpuIdleExit = hookfuncPtr;
        break;
    
    case LW_OPTION_CPU_INT_ENTER:                                       /*  CPU 进入中断(异常)模式      */
        _K_hookKernel.HOOK_CpuIntEnter = hookfuncPtr;
        break;
    
    case LW_OPTION_CPU_INT_EXIT:                                        /*  CPU 退出中断(异常)模式      */
        _K_hookKernel.HOOK_CpuIntExit = hookfuncPtr;
        break;
        
    case LW_OPTION_STACK_OVERFLOW_HOOK:                                 /*  堆栈溢出                    */
        _K_hookKernel.HOOK_StkOverflow = hookfuncPtr;
        break;
    
    case LW_OPTION_FATAL_ERROR_HOOK:                                    /*  致命错误                    */
        _K_hookKernel.HOOK_FatalError = hookfuncPtr;
        break;
        
    case LW_OPTION_VPROC_CREATE_HOOK:                                   /*  进程建立钩子                */
        _K_hookKernel.HOOK_VpCreate = hookfuncPtr;
        break;
    
    case LW_OPTION_VPROC_DELETE_HOOK:                                   /*  进程删除钩子                */
        _K_hookKernel.HOOK_VpDelete = hookfuncPtr;
        break;
    
    default:
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核同时打开中断        */
        _ErrorHandle(ERROR_KERNEL_OPT_NULL);
        return  (ERROR_KERNEL_OPT_NULL);
    }
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  退出内核同时打开中断        */
    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
