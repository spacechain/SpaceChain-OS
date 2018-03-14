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
** 文   件   名: ThreadRestart.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 14 日
**
** 描        述: 线程重新启动函数。

** BUG
2007.04.09  去掉了 pstkButtom 和 pstkLowAddress 变量。
2007.07.19  加入了 _DebugHandle() 功能.
2007.10.22  修改了 _DebugHandle() 一些不准确的地方.
2007.11.13  使用链表库对链表操作进行完全封装.
2007.11.13  使用 join 可能发生致命错误.
2007.12.24  整理注释.
2008.03.29  使用新的等待机制.
2008.03.30  使用新的就绪环操作.
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock();
2008.06.03  加入对 Create Hook 和 Delete Hook 的支持.
2008.06.03  重启自己需要 exce 线程帮助.
2009.05.24  重新调整这个服务, 主要是协程与调试信息部分.
2009.05.28  加入 cleanup 操作.
2009.07.14  本线程被 join 时, 可以重启.
2011.02.24  不再使用古老的 TCB_ulResumeNesting 机制.
2011.02.24  不再使用线程深度睡眠的方法, 而是用僵死状态这种机制.
2011.08.17  调用删除回调后, 需要重新初始化 libc reent 结构.
2012.03.31  任务重启时, 清除记事本.
2012.12.23  更行 API_ThreadRestart() 算法, 
            如果是重启自身, 则将需要回调的函数运行结束后再使用 except 线程重启.
2013.07.18  使用新的获取 TCB 的方法, 确保 SMP 系统安全.
2013.08.23  任务重启回调执行更加安全.
            只能重启相同进程内的任务.
2013.09.22  加入扩展重启接口, 允许任务更改入口函数.
2013.12.11  如果被重启的线程正在请求其他线程改变状态, 则需要退出等待链表.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
** 函数名称: __threadRestart
** 功能描述: 线程重启内部函数。
** 输　入  : ptcb                   重启任务 TCB
**           pfuncThread            线程新的入口 (LW_NULL 表示不改变)
**           pvArg                  参数
**           bIsAlreadyWaitDeath    是否已经为僵死状态
** 输　出  : 重启任务结果
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_THREAD_RESTART_EN > 0

static ULONG  __threadRestart (PLW_CLASS_TCB          ptcb, 
                               PTHREAD_START_ROUTINE  pfuncThread, 
                               PVOID                  pvArg, 
                               BOOL                   bIsAlreadyWaitDeath)
{
             INTREG                iregInterLevel;
             LW_OBJECT_HANDLE      ulId;
    REGISTER PLW_CLASS_PCB         ppcb;
    
    REGISTER PLW_STACK             pstkTop;
    REGISTER PLW_STACK             pstkFristFree;
    
#if LW_CFG_THREAD_NOTE_PAD_EN > 0
    REGISTER UINT8                 ucI;
    REGISTER ULONG                *pulNote;
#endif

#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)
    REGISTER PLW_EVENTSETNODE      pesnPtr;
#endif

    ulId = ptcb->TCB_ulId;

    if (bIsAlreadyWaitDeath == LW_FALSE) {
        _ThreadDeleteWaitDeath(ptcb);                                   /*  将要删除的线程进入僵死状态  */
    }
    
#if LW_CFG_COROUTINE_EN > 0
    _CoroutineFreeAll(ptcb);                                            /*  释放所有的协程              */
    {
        REGISTER PLW_CLASS_COROUTINE  pcrcb = &ptcb->TCB_crcbOrigent;   /*  仅剩下一个原始启动协程      */
        _List_Ring_Add_Ahead(&pcrcb->COROUTINE_ringRoutine,
                             &ptcb->TCB_pringCoroutineHeader);          /*  插入协程表                  */
    }
#endif                                                                  /*  LW_CFG_COROUTINE_EN > 0     */

#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)
    lib_nlreent_init(ulId);                                             /*  reinit libc reent           */
#endif

    __KERNEL_ENTER();                                                   /*  进入内核                    */
    
    ppcb = _GetPcb(ptcb);                                               /*  重新获取 ppcb 防止被修改    */
    
    ptcb->TCB_iDeleteProcStatus = LW_TCB_DELETE_PROC_NONE;              /*  退出虚拟删除过程            */
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */
    
    if (ptcb->TCB_ptcbDeleteWait) {                                     /*  目标线程正在等待其他任务删除*/
        ptcb->TCB_ptcbDeleteWait->TCB_ptcbDeleteMe = (PLW_CLASS_TCB)1;
        ptcb->TCB_ptcbDeleteWait = LW_NULL;
    }
    
#if (LW_CFG_EVENT_EN > 0) && (LW_CFG_MAX_EVENTS > 0)
    if (ptcb->TCB_peventPtr) {                                          /*  等待事件中                  */
        _EventUnQueue(ptcb);                                            /*  解等待连                    */
    }
#endif

#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)
    pesnPtr = ptcb->TCB_pesnPtr;
    if (pesnPtr) {
        _EventSetUnQueue(pesnPtr);                                      /*  解事件集                    */
    }
#endif
    
#if LW_CFG_SMP_EN > 0
    if (ptcb->TCB_ptcbWaitStatus ||
        ptcb->TCB_plineStatusReqHeader) {                               /*  正在请求其他线程改变状态    */
        _ThreadUnwaitStatus(ptcb);
    }
#endif                                                                  /*  LW_CFG_SMP_EN               */
    
#if LW_CFG_THREAD_NOTE_PAD_EN > 0                                       /*  任务记事本                  */
    pulNote = &ptcb->TCB_notepadThreadNotePad.NOTEPAD_ulNotePad[0];
    for (ucI = 0; ucI < LW_CFG_MAX_NOTEPADS; ucI++) {
        *pulNote++ = 0ul;
    }
#endif

    __KERNEL_SPACE_SET2(ptcb, 0);                                       /*  用户空间                    */
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  打开中断                    */

    pstkTop = ptcb->TCB_pstkStackTop;
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */
    
    if (pfuncThread) {
        ptcb->TCB_pthreadStartAddress = pfuncThread;                    /*  设置新的线程入口            */
    }
    
    pstkFristFree = archTaskCtxCreate(_ThreadShell,                     /*  初始化堆栈，SHELL           */
                                      (PVOID)ptcb->TCB_pthreadStartAddress,
                                      pstkTop, 
                                      ptcb->TCB_ulOption);
                                          
    ptcb->TCB_pvArg         = pvArg;
    ptcb->TCB_pstkStackNow  = pstkFristFree;
#if defined(LW_CFG_CPU_ARCH_C6X)
    ptcb->TCB_ulContextType = 0;                                        /*  小上下文                    */
#endif                                                                  /*  LW_CFG_CPU_ARCH_C6X         */
    
    /*
     *  这里不可能是自己重启自己.
     */
    if (!__LW_THREAD_IS_READY(ptcb)) {                                  /*  不在就绪方式就转化为就绪方式*/
        if (ptcb->TCB_usStatus & LW_THREAD_STATUS_DELAY) {
            __DEL_FROM_WAKEUP_LINE(ptcb);                               /*  从等待链中删除              */
            ptcb->TCB_ulDelay = 0ul;
        }
        ptcb->TCB_ucSchedActivate = LW_SCHED_ACT_INTERRUPT;             /*  中断激活方式                */
        __ADD_TO_READY_RING(ptcb, ppcb);                                /*  加入到相对优先级就绪环      */
    }
    
    ptcb->TCB_usStatus = LW_THREAD_STATUS_RDY;
    ptcb->TCB_ulDelay  = 0ul;
    
    ptcb->TCB_ulSuspendNesting = 0ul;
    
#if LW_CFG_SOFTWARE_WATCHDOG_EN > 0
    if (ptcb->TCB_bWatchDogInQ) {
        __DEL_FROM_WATCHDOG_LINE(ptcb);                                 /*  从 watch dog 中删除         */
        ptcb->TCB_ulWatchDog = 0ul;                                     /*  关闭看门狗                  */
    }
#endif
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  退出内核并打开中断          */
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "thread \"%s\" has been restart.\r\n", ptcb->TCB_cThreadName);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_ThreadRestart
** 功能描述: 线程重新启动函数。,这里不改变时间片属性，不能因为重启而获得更多的时间片
** 输　入  : ulId          句柄
**           pvArg         参数
** 输　出  :
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadRestart (LW_OBJECT_HANDLE  ulId, PVOID  pvArg)
{
    return  (API_ThreadRestartEx(ulId, LW_NULL, pvArg));
}
/*********************************************************************************************************
** 函数名称: API_ThreadRestartEx
** 功能描述: 线程重新启动函数。,这里不改变时间片属性，不能因为重启而获得更多的时间片
** 输　入  : ulId              句柄
**           pfuncThread       线程新的入口 (LW_NULL 表示不改变)
**           pvArg             参数
** 输　出  :
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadRestartEx (LW_OBJECT_HANDLE  ulId, PTHREAD_START_ROUTINE  pfuncThread, PVOID  pvArg)
{
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_TCB         ptcbCur;
    REGISTER PLW_CLASS_TCB         ptcb;
             ULONG                 ulError;
             
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
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    
#if LW_CFG_MODULELOADER_EN > 0
    if (ptcbCur->TCB_pvVProcessContext != 
        ptcb->TCB_pvVProcessContext) {                                  /*  不属于一个进程              */
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread not in same process.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */
    
    if (ptcb->TCB_iDeleteProcStatus) {                                  /*  检查是否在删除过程中        */
        if (ptcb == ptcbCur) {
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
    
    if (ptcb->TCB_ulThreadSafeCounter) {                                /*  在安全模式下的线程不能重启  */
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from SAFE mode.\r\n");
        _ErrorHandle(ERROR_THREAD_IN_SAFE);
        return  (ERROR_THREAD_IN_SAFE);
    }
    if (ptcb->TCB_ptcbJoin) {                                           /*  是否已和其他线程合并        */
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread join.\r\n");
        _ErrorHandle(ERROR_THREAD_JOIN);
        return  (ERROR_THREAD_JOIN);
    }
    if (ptcb->TCB_usStatus & LW_THREAD_STATUS_INIT) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread is Initializing.\r\n");
        _ErrorHandle(ERROR_THREAD_JOIN);
        return  (ERROR_THREAD_JOIN);
    }
    
    ptcb->TCB_iDeleteProcStatus = LW_TCB_DELETE_PROC_RESTART;           /*  进入重启过程                */
    
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
    if (ptcb == ptcbCur) {
        _ThreadRestartProcHook(ptcb);                                   /*  处理 hook 相关操作          */
        
        MONITOR_EVT_LONG2(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_RESTART, 
                          ulId, pvArg, LW_NULL);
        
        _excJobAdd((VOIDFUNCPTR)__threadRestart,
                   ptcb, (PVOID)pfuncThread, pvArg, 
                   (PVOID)LW_FALSE, 0, 0);                              /*  使用信号系统的异常处理      */
        for (;;) {
            API_TimeSleep(__ARCH_ULONG_MAX);                            /*  等待被删除                  */
        }
        ulError = ERROR_NONE;
    
    } else {                                                            /*  删除其他任务                */
        _ThreadDeleteWaitDeath(ptcb);                                   /*  将要删除的线程进入僵死状态  */
        
        _ThreadRestartProcHook(ptcb);                                   /*  处理 hook 相关操作          */
        
        MONITOR_EVT_LONG2(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_RESTART, 
                          ulId, pvArg, LW_NULL);
        
        ulError = __threadRestart(ptcb, pfuncThread, pvArg, LW_TRUE);
    }
    
    return  (ulError);
}

#endif                                                                  /*  LW_CFG_THREAD_RESTART_EN    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
