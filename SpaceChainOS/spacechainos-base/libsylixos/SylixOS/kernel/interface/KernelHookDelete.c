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
** 文   件   名: KernelHookDelete.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 14 日
**
** 描        述: 删除内核指定 HOOK

** BUG
2007.10.28  修改注释.
2010.08.03  支持 SMP 多核.
2012.09.22  加入诸多电源管理的 HOOK.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_KernelHookDelete
** 功能描述: 删除内核指定 HOOK
** 输　入  : 
**           ulOpt                         HOOK 类型
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_KernelHookDelete (ULONG  ulOpt)
{
    INTREG  iregInterLevel;
        
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  进入内核同时关闭中断        */
    
    switch (ulOpt) {
    
    case LW_OPTION_THREAD_CREATE_HOOK:                                  /*  线程建立钩子                */
        _K_hookKernel.HOOK_ThreadCreate = LW_NULL;
        break;
    
    case LW_OPTION_THREAD_DELETE_HOOK:                                  /*  线程删除钩子                */
        _K_hookKernel.HOOK_ThreadDelete = LW_NULL;
        break;
    
    case LW_OPTION_THREAD_SWAP_HOOK:                                    /*  线程切换钩子                */
        _K_hookKernel.HOOK_ThreadSwap = LW_NULL;
        break;
    
    case LW_OPTION_THREAD_TICK_HOOK:                                    /*  系统时钟中断钩子            */
        _K_hookKernel.HOOK_ThreadTick = LW_NULL;
        break;
    
    case LW_OPTION_THREAD_INIT_HOOK:                                    /*  线程初始化钩子              */
        _K_hookKernel.HOOK_ThreadInit = LW_NULL;
        break;
    
    case LW_OPTION_THREAD_IDLE_HOOK:                                    /*  空闲线程钩子                */
        _K_hookKernel.HOOK_ThreadIdle = LW_NULL;
        break;
    
    case LW_OPTION_KERNEL_INITBEGIN:                                    /*  内核初始化开始钩子          */
        _K_hookKernel.HOOK_KernelInitBegin = LW_NULL;
        break;
    
    case LW_OPTION_KERNEL_INITEND:                                      /*  内核初始化结束钩子          */
        _K_hookKernel.HOOK_KernelInitEnd = LW_NULL;
        break;
    
    case LW_OPTION_WATCHDOG_TIMER:                                      /*  看门狗定时器钩子            */
        _K_hookKernel.HOOK_WatchDogTimer = LW_NULL;
        break;
        
    case LW_OPTION_OBJECT_CREATE_HOOK:                                  /*  创建内核对象钩子            */
        _K_hookKernel.HOOK_ObjectCreate = LW_NULL;
        break;
    
    case LW_OPTION_OBJECT_DELETE_HOOK:                                  /*  删除内核对象钩子            */
        _K_hookKernel.HOOK_ObjectDelete = LW_NULL;
        break;
    
    case LW_OPTION_FD_CREATE_HOOK:                                      /*  文件描述符创建钩子          */
        _K_hookKernel.HOOK_FdCreate = LW_NULL;
        break;
    
    case LW_OPTION_FD_DELETE_HOOK:                                      /*  文件描述符删除钩子          */
        _K_hookKernel.HOOK_FdDelete = LW_NULL;
        break;
        
    case LW_OPTION_CPU_IDLE_ENTER:                                      /*  CPU 进入空闲模式            */
        _K_hookKernel.HOOK_CpuIdleEnter = LW_NULL;
        break;
    
    case LW_OPTION_CPU_IDLE_EXIT:                                       /*  CPU 退出空闲模式            */
        _K_hookKernel.HOOK_CpuIdleExit = LW_NULL;
        break;
    
    case LW_OPTION_CPU_INT_ENTER:                                       /*  CPU 进入中断(异常)模式      */
        _K_hookKernel.HOOK_CpuIntEnter = LW_NULL;
        break;
    
    case LW_OPTION_CPU_INT_EXIT:                                        /*  CPU 退出中断(异常)模式      */
        _K_hookKernel.HOOK_CpuIntExit = LW_NULL;
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
