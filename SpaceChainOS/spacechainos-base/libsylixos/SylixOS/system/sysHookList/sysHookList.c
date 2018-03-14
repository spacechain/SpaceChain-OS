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
** 文   件   名: HookList.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 04 月 20 日
**
** 描        述: 系统钩子函数函数, 

** BUG
2007.08.22  API_SystemHookAdd 出错误时，没有释放掉内存。
2007.08.22  API_SystemHookDelete 在操作关键性链表没有关闭中断。
2007.09.21  加入 _DebugHandle() 功能。
2007.11.13  使用链表库对链表操作进行完全封装.
2007.11.18  整理注释.
2008.03.02  加入系统重新启动回调.
2008.03.10  加入安全处理机制.
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock(); 
2009.12.09  修改注释.
2010.08.03  每个回调控制块使用独立的 spinlock.
2012.09.22  加入诸多电源管理的 HOOK.
2012.09.23  初始化时并不设置系统回调, 而是当用户第一次调用 hook add 操作时再安装.
2012.12.08  加入资源回收的功能.
2013.03.16  加入进程回调.
2013.05.02  这里已经加入资源管理, 进程允许安装回调.
2014.08.10  加入系统错误回调.
2015.04.07  优化回调删除操作.
2015.05.16  使用全新的 hook 接口.
*********************************************************************************************************/
/*********************************************************************************************************
注意：
      用户最好不要使用内核提供的 hook 功能，内核的 hook 功能是为系统级 hook 服务的，系统的 hook 有动态链表
      的功能，一个系统 hook 功能可以添加多个函数
      
      API_SystemHookDelete() 调用的时机非常重要，不能在 hook 扫描链扫描时调用，可能会发生扫描链断裂的情况
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "sysHookList.h"
/*********************************************************************************************************
  进程相关
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
** 函数名称: API_SystemHookAdd
** 功能描述: 添加一个系统 hook 功能函数
** 输　入  : hookfunc   HOOK 功能函数
**           ulOpt      HOOK 类型
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_SystemHookAdd (LW_HOOK_FUNC  hookfunc, ULONG  ulOpt)
{
    PLW_FUNC_NODE    pfuncnode;
    INT              iAddRet;
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
             
#if LW_CFG_ARG_CHK_EN > 0
    if (!hookfunc) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "hookfuncPtr invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HOOK_NULL);
        return  (ERROR_KERNEL_HOOK_NULL);
    }
#endif
    
    pfuncnode = (PLW_FUNC_NODE)__SHEAP_ALLOC(sizeof(LW_FUNC_NODE));     /*  申请控制块内存              */
    if (!pfuncnode) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);                          /*  缺少内存                    */
        return  (ERROR_SYSTEM_LOW_MEMORY);
    }
    
    pfuncnode->FUNCNODE_hookfunc = hookfunc;
        
    switch (ulOpt) {
    
    case LW_OPTION_THREAD_CREATE_HOOK:                                  /*  线程建立钩子                */
        iAddRet = HOOK_F_ADD(HOOK_T_CREATE, pfuncnode);
        if (iAddRet == ERROR_NONE) {
            _K_hookKernel.HOOK_ThreadCreate = HOOK_T_CREATE->HOOKCB_pfuncCall;
        }
        break;
        
    case LW_OPTION_THREAD_DELETE_HOOK:                                  /*  线程删除钩子                */
        iAddRet = HOOK_F_ADD(HOOK_T_DELETE, pfuncnode);
        if (iAddRet == ERROR_NONE) {
            _K_hookKernel.HOOK_ThreadDelete = HOOK_T_DELETE->HOOKCB_pfuncCall;
        }
        break;
        
    case LW_OPTION_THREAD_SWAP_HOOK:                                    /*  线程切换钩子                */
        iAddRet = HOOK_F_ADD(HOOK_T_SWAP, pfuncnode);
        if (iAddRet == ERROR_NONE) {
            _K_hookKernel.HOOK_ThreadSwap = HOOK_T_SWAP->HOOKCB_pfuncCall;
        }
        break;
        
    case LW_OPTION_THREAD_TICK_HOOK:                                    /*  系统时钟中断钩子            */
        iAddRet = HOOK_F_ADD(HOOK_T_TICK, pfuncnode);
        if (iAddRet == ERROR_NONE) {
            _K_hookKernel.HOOK_ThreadTick = HOOK_T_TICK->HOOKCB_pfuncCall;
        }
        break;
        
    case LW_OPTION_THREAD_INIT_HOOK:                                    /*  线程初始化钩子              */
        iAddRet = HOOK_F_ADD(HOOK_T_INIT, pfuncnode);
        if (iAddRet == ERROR_NONE) {
            _K_hookKernel.HOOK_ThreadInit = HOOK_T_INIT->HOOKCB_pfuncCall;
        }
        break;
        
    case LW_OPTION_THREAD_IDLE_HOOK:                                    /*  空闲线程钩子                */
        if (LW_SYS_STATUS_IS_RUNNING()) {
            __SHEAP_FREE(pfuncnode);                                    /*  释放内存                    */
            _DebugHandle(__ERRORMESSAGE_LEVEL, "can not add idle hook in running status.\r\n");
            _ErrorHandle(ERROR_KERNEL_RUNNING);
            return  (ERROR_KERNEL_RUNNING);
        }
        iAddRet = HOOK_F_ADD(HOOK_T_IDLE, pfuncnode);
        if (iAddRet == ERROR_NONE) {
            _K_hookKernel.HOOK_ThreadIdle = HOOK_T_IDLE->HOOKCB_pfuncCall;
        }
        break;
        
    case LW_OPTION_KERNEL_INITBEGIN:                                    /*  内核初始化开始钩子          */
        iAddRet = HOOK_F_ADD(HOOK_T_INITBEGIN, pfuncnode);
        if (iAddRet == ERROR_NONE) {
            _K_hookKernel.HOOK_KernelInitBegin = HOOK_T_INITBEGIN->HOOKCB_pfuncCall;
        }
        break;
        
    case LW_OPTION_KERNEL_INITEND:                                      /*  内核初始化结束钩子          */
        iAddRet = HOOK_F_ADD(HOOK_T_INITEND, pfuncnode);
        if (iAddRet == ERROR_NONE) {
            _K_hookKernel.HOOK_KernelInitEnd = HOOK_T_INITEND->HOOKCB_pfuncCall;
        }
        break;
        
    case LW_OPTION_KERNEL_REBOOT:                                       /*  内核重新启动                */
        iAddRet = HOOK_F_ADD(HOOK_T_REBOOT, pfuncnode);
        if (iAddRet == ERROR_NONE) {
            _K_hookKernel.HOOK_KernelReboot = HOOK_T_REBOOT->HOOKCB_pfuncCall;
        }
        break;
        
    case LW_OPTION_WATCHDOG_TIMER:                                      /*  看门狗定时器钩子            */
        iAddRet = HOOK_F_ADD(HOOK_T_WATCHDOG, pfuncnode);
        if (iAddRet == ERROR_NONE) {
            _K_hookKernel.HOOK_WatchDogTimer = HOOK_T_WATCHDOG->HOOKCB_pfuncCall;
        }
        break;
        
    case LW_OPTION_OBJECT_CREATE_HOOK:                                  /*  创建内核对象钩子            */
        iAddRet = HOOK_F_ADD(HOOK_T_OBJCREATE, pfuncnode);
        if (iAddRet == ERROR_NONE) {
            _K_hookKernel.HOOK_ObjectCreate = HOOK_T_OBJCREATE->HOOKCB_pfuncCall;
        }
        break;
        
    case LW_OPTION_OBJECT_DELETE_HOOK:                                  /*  删除内核对象钩子            */
        iAddRet = HOOK_F_ADD(HOOK_T_OBJDELETE, pfuncnode);
        if (iAddRet == ERROR_NONE) {
            _K_hookKernel.HOOK_ObjectDelete = HOOK_T_OBJDELETE->HOOKCB_pfuncCall;
        }
        break;
    
    case LW_OPTION_FD_CREATE_HOOK:                                      /*  文件描述符创建钩子          */
        iAddRet = HOOK_F_ADD(HOOK_T_FDCREATE, pfuncnode);
        if (iAddRet == ERROR_NONE) {
            _K_hookKernel.HOOK_FdCreate = HOOK_T_FDCREATE->HOOKCB_pfuncCall;
        }
        break;
    
    case LW_OPTION_FD_DELETE_HOOK:                                      /*  文件描述符删除钩子          */
        iAddRet = HOOK_F_ADD(HOOK_T_FDDELETE, pfuncnode);
        if (iAddRet == ERROR_NONE) {
            _K_hookKernel.HOOK_FdDelete = HOOK_T_FDDELETE->HOOKCB_pfuncCall;
        }
        break;
    
    case LW_OPTION_CPU_IDLE_ENTER:                                      /*  CPU 进入空闲模式            */
        iAddRet = HOOK_F_ADD(HOOK_T_IDLEENTER, pfuncnode);
        if (iAddRet == ERROR_NONE) {
            _K_hookKernel.HOOK_CpuIdleEnter = HOOK_T_IDLEENTER->HOOKCB_pfuncCall;
        }
        break;
    
    case LW_OPTION_CPU_IDLE_EXIT:                                       /*  CPU 退出空闲模式            */
        iAddRet = HOOK_F_ADD(HOOK_T_IDLEEXIT, pfuncnode);
        if (iAddRet == ERROR_NONE) {
            _K_hookKernel.HOOK_CpuIdleExit = HOOK_T_IDLEEXIT->HOOKCB_pfuncCall;
        }
        break;
    
    case LW_OPTION_CPU_INT_ENTER:                                       /*  CPU 进入中断(异常)模式      */
        iAddRet = HOOK_F_ADD(HOOK_T_INTENTER, pfuncnode);
        if (iAddRet == ERROR_NONE) {
            _K_hookKernel.HOOK_CpuIntEnter = HOOK_T_INTENTER->HOOKCB_pfuncCall;
        }
        break;
    
    case LW_OPTION_CPU_INT_EXIT:                                        /*  CPU 退出中断(异常)模式      */
        iAddRet = HOOK_F_ADD(HOOK_T_INTEXIT, pfuncnode);
        if (iAddRet == ERROR_NONE) {
            _K_hookKernel.HOOK_CpuIntExit = HOOK_T_INTEXIT->HOOKCB_pfuncCall;
        }
        break;
        
    case LW_OPTION_STACK_OVERFLOW_HOOK:                                 /*  堆栈溢出                    */
        iAddRet = HOOK_F_ADD(HOOK_T_STKOF, pfuncnode);
        if (iAddRet == ERROR_NONE) {
            _K_hookKernel.HOOK_StkOverflow = HOOK_T_STKOF->HOOKCB_pfuncCall;
        }
        break;
        
    case LW_OPTION_FATAL_ERROR_HOOK:                                    /*  致命错误                    */
        iAddRet = HOOK_F_ADD(HOOK_T_FATALERR, pfuncnode);
        if (iAddRet == ERROR_NONE) {
            _K_hookKernel.HOOK_FatalError = HOOK_T_FATALERR->HOOKCB_pfuncCall;
        }
        break;
        
    case LW_OPTION_VPROC_CREATE_HOOK:                                   /*  进程建立钩子                */
        iAddRet = HOOK_F_ADD(HOOK_T_VPCREATE, pfuncnode);
        if (iAddRet == ERROR_NONE) {
            _K_hookKernel.HOOK_VpCreate = HOOK_T_VPCREATE->HOOKCB_pfuncCall;
        }
        break;
        
    case LW_OPTION_VPROC_DELETE_HOOK:                                   /*  进程删除钩子                */
        iAddRet = HOOK_F_ADD(HOOK_T_VPDELETE, pfuncnode);
        if (iAddRet == ERROR_NONE) {
            _K_hookKernel.HOOK_VpDelete = HOOK_T_VPDELETE->HOOKCB_pfuncCall;
        }
        break;
    
    default:
        __SHEAP_FREE(pfuncnode);                                        /*  释放内存                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "option invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_OPT_NULL);
        return  (ERROR_KERNEL_OPT_NULL);
    }
    
    if (iAddRet) {
        __SHEAP_FREE(pfuncnode);                                        /*  释放内存                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "hook table full.\r\n");
        _ErrorHandle(ERROR_KERNEL_HOOK_FULL);
        return  (ERROR_KERNEL_HOOK_FULL);
    }
    
#if LW_CFG_MODULELOADER_EN > 0
    if (__PROC_GET_PID_CUR() && vprocFindProc((PVOID)hookfunc)) {
        __resAddRawHook(&pfuncnode->FUNCNODE_resraw, (VOIDFUNCPTR)API_SystemHookDelete, 
                        (PVOID)pfuncnode->FUNCNODE_hookfunc, (PVOID)ulOpt, 0, 0, 0, 0);
    } else {
        pfuncnode->FUNCNODE_resraw.RESRAW_bIsInstall = LW_FALSE;        /*  不需要回收操作             */
    }
#endif
    
    return  (iAddRet);
}
/*********************************************************************************************************
** 函数名称: API_SystemHookDelete
** 功能描述: 删除一个系统 hook 功能函数
** 输　入  : hookfunc  HOOK 功能函数
**           ulOpt     HOOK 类型
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_SystemHookDelete (LW_HOOK_FUNC  hookfunc, ULONG  ulOpt)
{
    PLW_FUNC_NODE   pfuncnode;
    BOOL            bEmpty;
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!hookfunc) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "hookfuncPtr invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HOOK_NULL);
        return  (ERROR_KERNEL_HOOK_NULL);
    }
#endif
    
    switch (ulOpt) {
    
    case LW_OPTION_THREAD_CREATE_HOOK:                                  /*  线程建立钩子                */
        pfuncnode = HOOK_F_DEL(HOOK_T_CREATE, hookfunc, &bEmpty);
        if (bEmpty) {
            _K_hookKernel.HOOK_ThreadCreate = LW_NULL;
        }
        break;
        
    case LW_OPTION_THREAD_DELETE_HOOK:                                  /*  线程删除钩子                */
        pfuncnode = HOOK_F_DEL(HOOK_T_DELETE, hookfunc, &bEmpty);
        if (bEmpty) {
            _K_hookKernel.HOOK_ThreadDelete = LW_NULL;
        }
        break;
        
    case LW_OPTION_THREAD_SWAP_HOOK:                                    /*  线程切换钩子                */
        pfuncnode = HOOK_F_DEL(HOOK_T_SWAP, hookfunc, &bEmpty);
        if (bEmpty) {
            _K_hookKernel.HOOK_ThreadSwap = LW_NULL;
        }
        break;
        
    case LW_OPTION_THREAD_TICK_HOOK:                                    /*  系统时钟中断钩子            */
        pfuncnode = HOOK_F_DEL(HOOK_T_TICK, hookfunc, &bEmpty);
        if (bEmpty) {
            _K_hookKernel.HOOK_ThreadTick = LW_NULL;
        }
        break;
        
    case LW_OPTION_THREAD_INIT_HOOK:                                    /*  线程初始化钩子              */
        pfuncnode = HOOK_F_DEL(HOOK_T_INIT, hookfunc, &bEmpty);
        if (bEmpty) {
            _K_hookKernel.HOOK_ThreadInit = LW_NULL;
        }
        break;
        
    case LW_OPTION_THREAD_IDLE_HOOK:                                    /*  空闲线程钩子                */
        pfuncnode = HOOK_F_DEL(HOOK_T_IDLE, hookfunc, &bEmpty);
        if (bEmpty) {
            _K_hookKernel.HOOK_ThreadIdle = LW_NULL;
        }
        break;
        
    case LW_OPTION_KERNEL_INITBEGIN:                                    /*  内核初始化开始钩子          */
        pfuncnode = HOOK_F_DEL(HOOK_T_INITBEGIN, hookfunc, &bEmpty);
        if (bEmpty) {
            _K_hookKernel.HOOK_KernelInitBegin = LW_NULL;
        }
        break;
        
    case LW_OPTION_KERNEL_INITEND:                                      /*  内核初始化结束钩子          */
        pfuncnode = HOOK_F_DEL(HOOK_T_INITEND, hookfunc, &bEmpty);
        if (bEmpty) {
            _K_hookKernel.HOOK_KernelInitEnd = LW_NULL;
        }
        break;
        
    case LW_OPTION_KERNEL_REBOOT:                                       /*  内核重新启动                */
        pfuncnode = HOOK_F_DEL(HOOK_T_REBOOT, hookfunc, &bEmpty);
        if (bEmpty) {
            _K_hookKernel.HOOK_KernelReboot = LW_NULL;
        }
        break;
        
    case LW_OPTION_WATCHDOG_TIMER:                                      /*  看门狗定时器钩子            */
        pfuncnode = HOOK_F_DEL(HOOK_T_WATCHDOG, hookfunc, &bEmpty);
        if (bEmpty) {
            _K_hookKernel.HOOK_WatchDogTimer = LW_NULL;
        }
        break;
        
    case LW_OPTION_OBJECT_CREATE_HOOK:                                  /*  创建内核对象钩子            */
        pfuncnode = HOOK_F_DEL(HOOK_T_OBJCREATE, hookfunc, &bEmpty);
        if (bEmpty) {
            _K_hookKernel.HOOK_ObjectCreate = LW_NULL;
        }
        break;
    
    case LW_OPTION_OBJECT_DELETE_HOOK:                                  /*  删除内核对象钩子            */
        pfuncnode = HOOK_F_DEL(HOOK_T_OBJDELETE, hookfunc, &bEmpty);
        if (bEmpty) {
            _K_hookKernel.HOOK_ObjectDelete = LW_NULL;
        }
        break;
    
    case LW_OPTION_FD_CREATE_HOOK:                                      /*  文件描述符创建钩子          */
        pfuncnode = HOOK_F_DEL(HOOK_T_FDCREATE, hookfunc, &bEmpty);
        if (bEmpty) {
            _K_hookKernel.HOOK_FdCreate = LW_NULL;
        }
        break;
    
    case LW_OPTION_FD_DELETE_HOOK:                                      /*  文件描述符删除钩子          */
        pfuncnode = HOOK_F_DEL(HOOK_T_FDDELETE, hookfunc, &bEmpty);
        if (bEmpty) {
            _K_hookKernel.HOOK_FdDelete = LW_NULL;
        }
        break;
        
    case LW_OPTION_CPU_IDLE_ENTER:                                      /*  CPU 进入空闲模式            */
        pfuncnode = HOOK_F_DEL(HOOK_T_IDLEENTER, hookfunc, &bEmpty);
        if (bEmpty) {
            _K_hookKernel.HOOK_CpuIdleEnter = LW_NULL;
        }
        break;
    
    case LW_OPTION_CPU_IDLE_EXIT:                                       /*  CPU 退出空闲模式            */
        pfuncnode = HOOK_F_DEL(HOOK_T_IDLEEXIT, hookfunc, &bEmpty);
        if (bEmpty) {
            _K_hookKernel.HOOK_CpuIdleExit = LW_NULL;
        }
        break;
    
    case LW_OPTION_CPU_INT_ENTER:                                       /*  CPU 进入中断(异常)模式      */
        pfuncnode = HOOK_F_DEL(HOOK_T_INTENTER, hookfunc, &bEmpty);
        if (bEmpty) {
            _K_hookKernel.HOOK_CpuIntEnter = LW_NULL;
        }
        break;
    
    case LW_OPTION_CPU_INT_EXIT:                                        /*  CPU 退出中断(异常)模式      */
        pfuncnode = HOOK_F_DEL(HOOK_T_INTEXIT, hookfunc, &bEmpty);
        if (bEmpty) {
            _K_hookKernel.HOOK_CpuIntExit = LW_NULL;
        }
        break;
        
    case LW_OPTION_STACK_OVERFLOW_HOOK:                                 /*  堆栈溢出                    */
        pfuncnode = HOOK_F_DEL(HOOK_T_STKOF, hookfunc, &bEmpty);
        if (bEmpty) {
            _K_hookKernel.HOOK_StkOverflow = LW_NULL;
        }
        break;
    
    case LW_OPTION_FATAL_ERROR_HOOK:                                    /*  致命错误                    */
        pfuncnode = HOOK_F_DEL(HOOK_T_FATALERR, hookfunc, &bEmpty);
        if (bEmpty) {
            _K_hookKernel.HOOK_FatalError = LW_NULL;
        }
        break;
            
    case LW_OPTION_VPROC_CREATE_HOOK:                                   /*  进程建立钩子                */
        pfuncnode = HOOK_F_DEL(HOOK_T_VPCREATE, hookfunc, &bEmpty);
        if (bEmpty) {
            _K_hookKernel.HOOK_VpCreate = LW_NULL;
        }
        break;
        
    case LW_OPTION_VPROC_DELETE_HOOK:                                   /*  进程删除钩子                */
        pfuncnode = HOOK_F_DEL(HOOK_T_VPDELETE, hookfunc, &bEmpty);
        if (bEmpty) {
            _K_hookKernel.HOOK_VpDelete = LW_NULL;
        }
        break;
    
    default:
        _DebugHandle(__ERRORMESSAGE_LEVEL, "option invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_OPT_NULL);
        return  (ERROR_KERNEL_OPT_NULL);
    }
    
    if (pfuncnode) {
        __resDelRawHook(&pfuncnode->FUNCNODE_resraw);
        __SHEAP_FREE(pfuncnode);
        return  (ERROR_NONE);
    
    } else {
        _ErrorHandle(ERROR_KERNEL_HOOK_NULL);
        return  (ERROR_KERNEL_HOOK_NULL);
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
