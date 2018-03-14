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
** 文   件   名: ThreadDelete.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 07 月 18 日
**
** 描        述: 线程删除函数。

** BUG
2007.01.08  内存最后释放，减少中断屏蔽时间
2007.07.18  加入 _DebugHandle() 功能
2007.11.13  唤醒 join 线程函数入口线程控制块错误.
2007.11.18  整理注释.
2007.12.22  改动了删除流程, 首先调用回调(开调度器,开中断的情况), 将被删除任务深度挂起. 
2007.12.24  操作系统在没有进入多任务模式时, 不能删除线程.
2008.01.04  对开关中断的时机进行了修改, 在执行完 HOOK 后, 重新获取 PCB, 即使修改了优先级, 也不会出现错误.
2008.03.29  加入对新的 wake up 和 watch dog 机制的处理.
2008.03.30  使用新的就绪环操作.
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock();
2008.06.21  加入协程的空间释放函数.
2009.02.03  加入销毁 TCB 扩展区.
2009.05.08  加入 _CleanupPopTCBExt().
2009.05.24  将 TCB_bIsInDeleteProc 处理提前.
2011.02.24  不再使用线程深度睡眠的方法, 而是用僵死状态这种机制.
2011.08.05  加入 exit 函数, 不再使用 #define exit API_ThreadExit
2012.12.08  加入资源管理器静态回调.
2012.12.23  更行 API_ThreadDelete() 算法, 
            如果是删除自身, 则将需要回调的函数运行结束后再使用 except 线程删除.
2013.01.15  _exit() 在 1.0.0.rc37 后成为系统 api. 补丁不再提供此函数.
2013.05.07  TCB 不再放在堆栈中, 而是从 TCB 池中分配.
2013.07.18  使用新的获取 TCB 的方法, 确保 SMP 系统安全.
2013.08.23  任务删除回调执行更加安全.
            只能删除相同进程内的任务. 其他进程的任务需要使用信号删除.
2013.08.28  加入内核事件监控器.
2013.09.10  允许在安全模式下删除自己, 线程将会在退出安全模式后自动删除.
2013.09.21  加入对删除进程主线程的特殊处理.
2013.12.11  如果被删除的线程正在请求其他线程改变状态, 则需要退出等待链表.
2014.01.01  允许删除正在等待删除其他任务的任务.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  进程相关
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
** 函数名称: __threadDelete
** 功能描述: 线程删除内部函数。
** 输　入  : 
**           ptcbDel                删除任务 TCB
**           bIsInSafe              是否在安全模式下被删除
**           ulRetVal               返回值   (返回给 JOIN 的线程)
**           bIsAlreadyWaitDeath    是否已经为僵死状态
** 输　出  : 删除任务结果
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_THREAD_DEL_EN > 0

ULONG  __threadDelete (PLW_CLASS_TCB  ptcbDel, BOOL  bIsInSafe, 
                       PVOID  pvRetVal, BOOL  bIsAlreadyWaitDeath)
{
             INTREG                iregInterLevel;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_PCB         ppcbDel;
    REGISTER LW_OBJECT_HANDLE      ulId;
    REGISTER PLW_STACK             pstkFree;                            /*  要释放的内存地址            */
             PVOID                 pvVProc;
             
#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)
    REGISTER PLW_EVENTSETNODE      pesnPtr;
#endif

    ulId    = ptcbDel->TCB_ulId;
    usIndex = _ObjectGetIndex(ulId);
    
    if (bIsAlreadyWaitDeath == LW_FALSE) {
        _ThreadDeleteWaitDeath(ptcbDel);                                /*  将要删除的线程进入僵死状态  */
    }
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    
    ppcbDel = _GetPcb(ptcbDel);
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */
    
    if (ptcbDel->TCB_ptcbDeleteWait) {                                  /*  目标线程正在等待其他任务删除*/
        ptcbDel->TCB_ptcbDeleteWait->TCB_ptcbDeleteMe = (PLW_CLASS_TCB)1;
        ptcbDel->TCB_ptcbDeleteWait = LW_NULL;
    }
    
#if (LW_CFG_EVENT_EN > 0) && (LW_CFG_MAX_EVENTS > 0)
    if (ptcbDel->TCB_peventPtr) {                                       /*  等待事件中                  */
        _EventUnQueue(ptcbDel);                                         /*  解等待连                    */
    }
#endif

#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)
    pesnPtr = ptcbDel->TCB_pesnPtr;
    if (pesnPtr) {
        _EventSetUnQueue(pesnPtr);                                      /*  解事件集                    */
    }
#endif

#if LW_CFG_SMP_EN > 0
    if (ptcbDel->TCB_ptcbWaitStatus ||
        ptcbDel->TCB_plineStatusReqHeader) {                            /*  正在请求其他线程改变状态    */
        _ThreadUnwaitStatus(ptcbDel);
    }
#endif                                                                  /*  LW_CFG_SMP_EN               */
    
    if (__LW_THREAD_IS_READY(ptcbDel)) {                                /*  是否就绪                    */
        __DEL_FROM_READY_RING(ptcbDel, ppcbDel);                        /*  从就绪队列中删除            */
        
    } else {
        if (ptcbDel->TCB_usStatus & LW_THREAD_STATUS_DELAY) {
            __DEL_FROM_WAKEUP_LINE(ptcbDel);                            /*  从等待链中删除              */
            ptcbDel->TCB_ulDelay = 0ul;
        }
        ptcbDel->TCB_usStatus = LW_THREAD_STATUS_RDY;                   /*  防止 Tick 中断激活          */
    }
    
#if LW_CFG_SOFTWARE_WATCHDOG_EN > 0
    if (ptcbDel->TCB_bWatchDogInQ) {
        __DEL_FROM_WATCHDOG_LINE(ptcbDel);                              /*  从 watch dog 中删除         */
        ptcbDel->TCB_ulWatchDog = 0ul;
    }
#endif
    KN_INT_ENABLE(iregInterLevel);
    
    pstkFree = ptcbDel->TCB_pstkStackLowAddr;                           /*  记录地址                    */
    
#if (LW_CFG_SMP_EN == 0) && (LW_CFG_THREAD_PRIVATE_VARS_EN > 0) && (LW_CFG_MAX_THREAD_GLB_VARS > 0)
    _ThreadVarDelete(ptcbDel);                                          /*  删除并恢复私有化的全局变量  */
#endif
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */ 
    
    _K_usThreadCounter--;
    _K_ptcbTCBIdTable[usIndex] = LW_NULL;                               /*  TCB 表清0                   */
    
    _List_Line_Del(&ptcbDel->TCB_lineManage, &_K_plineTCBHeader);       /*  从管理练表中删除            */
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  打开中断                    */
    
    if (ptcbDel->TCB_ptcbJoin) {
        _ThreadDisjoin(ptcbDel->TCB_ptcbJoin, ptcbDel);                 /*  退出 join 状态, 不操作就绪表*/
    }
    
    _ThreadDisjoinWakeupAll(ptcbDel, pvRetVal);                         /*  DETACH                      */
    
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
#if LW_CFG_COROUTINE_EN > 0
    _CoroutineFreeAll(ptcbDel);                                         /*  删除协程内存空间            */
#endif                                                                  /*  LW_CFG_COROUTINE_EN > 0     */
    
    if (bIsInSafe) {
        _DebugFormat(__ERRORMESSAGE_LEVEL, 
                     "thread \"%s\" has been delete in SAFE mode.\r\n",
                     ptcbDel->TCB_cThreadName);
    } else {
        _DebugFormat(__LOGMESSAGE_LEVEL, 
                     "thread \"%s\" has been delete.\r\n",
                     ptcbDel->TCB_cThreadName);
    }
    
    pvVProc = ptcbDel->TCB_pvVProcessContext;                           /*  进程信息                    */
    
    if (ptcbDel->TCB_ucStackAutoAllocFlag) {                            /*  是否是自动分配              */
        _StackFree(ptcbDel, pstkFree);                                  /*  释放堆栈空间                */
    }
    
    _TCBDestroy(ptcbDel);                                               /*  销毁 TCB                    */
    
    __KERNEL_MODE_PROC(
        _Free_Tcb_Object(ptcbDel);                                      /*  释放 ID                     */
    );
    
    __LW_OBJECT_DELETE_HOOK(ulId);

#if LW_CFG_MODULELOADER_EN > 0
    __resThreadDelHook(pvVProc, ulId);                                  /*  资源管理器内置静态回调      */
#else
    (VOID)pvVProc;
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_ThreadDelete
** 功能描述: 线程删除函数。
** 输　入  : 
**           pulId         句柄
**           pvRetVal      返回值   (返回给 JOIN 的线程)
** 输　出  :
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadDelete (LW_OBJECT_HANDLE  *pulId, PVOID  pvRetVal)
{
             INTREG                iregInterLevel;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_TCB         ptcbCur;
    REGISTER PLW_CLASS_TCB         ptcbDel;
    REGISTER LW_OBJECT_HANDLE      ulId;
             ULONG                 ulError;
             
#if LW_CFG_MODULELOADER_EN > 0
             LW_LD_VPROC          *pvprocCur;
             LW_LD_VPROC          *pvprocDel;
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */
    
    ulId = *pulId;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
    if (!LW_SYS_STATUS_IS_RUNNING()) {                                  /*  系统没有启动不能调用        */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system not running.\r\n");
        _ErrorHandle(ERROR_KERNEL_NOT_RUNNING);
        return  (ERROR_KERNEL_NOT_RUNNING);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  检查 ID 类型有效性          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if (_Thread_Index_Invalid(usIndex)) {                               /*  检查线程有效性              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
#endif

    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
    
    ptcbDel = _K_ptcbTCBIdTable[usIndex];
    
#if LW_CFG_MODULELOADER_EN > 0
    pvprocCur = __LW_VP_GET_TCB_PROC(ptcbCur);
    pvprocDel = __LW_VP_GET_TCB_PROC(ptcbDel);
    if (pvprocCur != pvprocDel) {                                       /*  不属于一个进程              */
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread not in same process.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
    
    if (pvprocDel && (pvprocDel->VP_ulMainThread == ulId)) {
        if (ptcbCur != ptcbDel) {                                       /*  主线程只能自己删除自己      */
            __KERNEL_EXIT();                                            /*  退出内核                    */
            _DebugHandle(__ERRORMESSAGE_LEVEL, "can not delete main thread.\r\n");
            _ErrorHandle(ERROR_THREAD_NULL);
            return  (ERROR_THREAD_NULL);
        }
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */

    if (ptcbDel->TCB_iDeleteProcStatus) {                               /*  检查是否在删除过程中        */
        if (ptcbDel == ptcbCur) {
            __KERNEL_EXIT();                                            /*  退出内核                    */
            for (;;) {
                API_TimeSleep(__ARCH_ULONG_MAX);                        /*  等待被删除                  */
            }
            return  (ERROR_NONE);                                       /*  永远运行不到这里            */
        
        } else {
            __KERNEL_EXIT();                                            /*  退出内核                    */
            _ErrorHandle(ERROR_THREAD_OTHER_DELETE);
            return  (ERROR_THREAD_OTHER_DELETE);
        }
    }
    
    if (ptcbDel->TCB_ulThreadSafeCounter) {                             /*  在安全模式下                */
        if (ptcbDel->TCB_ptcbDeleteMe) {                                /*  已经有线程删除它了          */
            __KERNEL_EXIT();                                            /*  退出内核                    */
            _DebugHandle(__ERRORMESSAGE_LEVEL, "another thread kill this thread now.\r\n");
            _ErrorHandle(ERROR_THREAD_OTHER_DELETE);
            return  (ERROR_THREAD_OTHER_DELETE);
        }
        
        iregInterLevel = KN_INT_DISABLE();                              /*  关闭中断                    */
        
        ptcbDel->TCB_ptcbDeleteMe = ptcbCur;                            /*  申请删除线程                */
        ptcbDel->TCB_pvRetValue   = pvRetVal;                           /*  记录返回值                  */
        
        if (ptcbDel != ptcbCur) {
            ptcbCur->TCB_ptcbDeleteWait = ptcbDel;                      /*  记录自己等待删除的任务      */
            _ThreadSafeSuspend(ptcbCur);                                /*  阻塞自己等待对方删除        */
        }
        
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核并打开中断          */
        return  (ERROR_NONE);
    }
    
#if LW_CFG_MODULELOADER_EN > 0
    if (pvprocDel && (pvprocDel->VP_ulMainThread == ulId)) {            /*  主线程自己删除自己          */
        if (pvprocDel->VP_iStatus != __LW_VP_EXIT) {
            __KERNEL_EXIT();                                            /*  退出内核                    */
            vprocExit(pvprocDel, ulId, (INT)pvRetVal);                  /*  进程结束                    */
            return  (ERROR_NONE);                                       /*  不会运行到这里              */
        }
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */
    
    ptcbDel->TCB_iDeleteProcStatus = LW_TCB_DELETE_PROC_DEL;            /*  进入删除过程                */
    
    _ObjectCloseId(pulId);                                              /*  关闭 ID                     */
    
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
    if (ptcbDel == ptcbCur) {
        _ThreadDeleteProcHook(ptcbDel, pvRetVal);                       /*  处理 hook 相关操作          */
        
        MONITOR_EVT_LONG2(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_DELETE, 
                          ptcbDel->TCB_ulId, pvRetVal, LW_NULL);
        
        _excJobAdd((VOIDFUNCPTR)__threadDelete,
                   ptcbDel, (PVOID)LW_FALSE, pvRetVal, 
                   (PVOID)LW_FALSE, 0, 0);                              /*  使用信号系统的异常处理      */
        for (;;) {
            API_TimeSleep(__ARCH_ULONG_MAX);                            /*  等待被删除                  */
        }
        ulError = ERROR_NONE;
    
    } else {                                                            /*  删除其他任务                */
        _ThreadDeleteWaitDeath(ptcbDel);                                /*  将要删除的线程进入僵死状态  */
        
        _ThreadDeleteProcHook(ptcbDel, pvRetVal);                       /*  处理 hook 相关操作          */
    
        MONITOR_EVT_LONG2(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_DELETE, 
                          ptcbDel->TCB_ulId, pvRetVal, LW_NULL);
    
        ulError = __threadDelete(ptcbDel, LW_FALSE, pvRetVal, LW_TRUE);
    }
    
    return  (ulError);
}
/*********************************************************************************************************
** 函数名称: API_ThreadExit
** 功能描述: 线程自行退出。
** 输　入  : 
**           pvRetVal       返回值   (返回给 JOIN 的线程)
** 输　出  :
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadExit (PVOID  pvRetVal)
{
    LW_OBJECT_HANDLE    ulId = API_ThreadIdSelf();
    
    return  (API_ThreadDelete(&ulId, pvRetVal));
}
/*********************************************************************************************************
** 函数名称: exit
** 功能描述: 内核线程或者进程退出
** 输　入  : 
**           iCode         返回值
** 输　出  :
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API  
void  exit (int  iCode)
{
    LW_OBJECT_HANDLE    ulId = API_ThreadIdSelf();
    
#if LW_CFG_MODULELOADER_EN > 0
    LW_LD_VPROC        *pvprocCur = __LW_VP_GET_CUR_PROC();
    
    if (pvprocCur) {
        if (pvprocCur->VP_ulMainThread == ulId) {
            vprocExit(__LW_VP_GET_CUR_PROC(), ulId, iCode);             /*  进程结束                    */
        }
#if LW_CFG_SIGNAL_EN > 0
          else {
            union sigval    sigvalue;
            sigvalue.sival_int = iCode;
            sigqueue(pvprocCur->VP_ulMainThread, SIGTERM, sigvalue);    /*  给主线程发信号退出          */
        }
#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

    API_ThreadForceDelete(&ulId, (PVOID)iCode);
    
    for (;;) {
        API_TimeSleep(__ARCH_ULONG_MAX);
    }
}
/*********************************************************************************************************
** 函数名称: _exit
** 功能描述: 内核线程或者进程退出, 不执行 atexit 安装的函数
** 输　入  : 
**           iCode         返回值
** 输　出  :
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API  
void  _exit (int  iCode)
{
    LW_OBJECT_HANDLE    ulId   = API_ThreadIdSelf();
    
#if LW_CFG_MODULELOADER_EN > 0
    LW_LD_VPROC        *pvprocCur = __LW_VP_GET_CUR_PROC();
    
    if (pvprocCur) {
        pvprocCur->VP_bRunAtExit = LW_FALSE;                            /*  不执行 atexit 安装函数      */
        if (pvprocCur->VP_ulMainThread == ulId) {
            vprocExit(__LW_VP_GET_CUR_PROC(), ulId, iCode);             /*  进程结束                    */
        }
#if LW_CFG_SIGNAL_EN > 0
          else {
            union sigval    sigvalue;
            sigvalue.sival_int = iCode;
            sigqueue(pvprocCur->VP_ulMainThread, SIGTERM, sigvalue);    /*  给主线程发信号退出          */
        }
#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

    API_ThreadForceDelete(&ulId, (PVOID)iCode);
    
    for (;;) {
        API_TimeSleep(__ARCH_ULONG_MAX);
    }
}

#ifdef __GNUC__
weak_alias(_exit, _Exit);
#else
void  _Exit (int  iCode)
{
    _exit(iCode);
}
#endif                                                                  /*  __GNUC__                    */
/*********************************************************************************************************
** 函数名称: atexit
** 功能描述: 进程退出时执行的操作. (当使用多进程模式时, 将会使用 vp patch 重定向)
** 输　入  : 
**           iCode         返回值   (返回给 JOIN 的线程)
** 输　出  :
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API  
int  atexit (void (*func)(void))
{
#if (LW_CFG_MODULELOADER_EN > 0) && (LW_CFG_MODULELOADER_ATEXIT_EN > 0)
    return  (API_ModuleAtExit(func));

#else
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
}

#endif                                                                  /*  LW_CFG_THREAD_DEL_EN        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
