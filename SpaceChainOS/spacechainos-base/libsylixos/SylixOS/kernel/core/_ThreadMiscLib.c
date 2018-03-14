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
** 文   件   名: _ThreadMiscLib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 18 日
**
** 描        述: 线程改变优先级。

** BUG
2008.03.30  使用新的就绪环操作.
2012.07.04  合并 _ThreadDeleteWaitDeath() 到这里.
2012.08.24  加入 _ThreadUserGet() 函数.
2012.10.23  加入 _ThreadContinue() 函数.
2013.07.18  加入 _ThreadStop() 函数.
2013.08.27  加入内核事件监控器.
2013.12.02  将状态改变函数移动到 _ThreadStatus.c 文件中.
2014.05.13  _ThreadContinue() 支持强制恢复.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  进程相关
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
** 函数名称: _ThreadUserGet
** 功能描述: 获取线程用户信息
** 输　入  : ulId          线程 ID
**           puid          uid
**           pgid          gid
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  _ThreadUserGet (LW_HANDLE  ulId, uid_t  *puid, gid_t  *pgid)
{
    REGISTER UINT16         usIndex;
    REGISTER PLW_CLASS_TCB  ptcb;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  检查 ID 类型有效性          */
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if (_Thread_Index_Invalid(usIndex)) {                               /*  检查线程有效性              */
        return  (ERROR_THREAD_NULL);
    }
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        return  (ERROR_THREAD_NULL);
    }
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    
    if (puid) {
        *puid = ptcb->TCB_uid;
    }
    if (pgid) {
        *pgid = ptcb->TCB_gid;
    }
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _ThreadDeleteWaitDeath
** 功能描述: 将将要删除的线程进入僵死状态.
** 输　入  : ptcbDel         将要删除的线程控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _ThreadDeleteWaitDeath (PLW_CLASS_TCB  ptcbDel)
{
    __KERNEL_ENTER();
    _ThreadStatusChange(ptcbDel, LW_TCB_REQ_WDEATH);
    __KERNEL_EXIT();                                                    /*  等待对方状态转换完毕        */
}
/*********************************************************************************************************
** 函数名称: _ThreadSched
** 功能描述: 线程解锁后尝试进行调度
** 输　入  : ptcbCur       当前任务控制块
** 输　出  : 调度器返回值
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _ThreadSched (PLW_CLASS_TCB  ptcbCur)
{
    LW_KERNEL_JOB_EXEC();                                               /*  尝试执行异步工作队列        */
    
    return  (__KERNEL_SCHED());                                         /*  尝试一次调度                */
}
/*********************************************************************************************************
** 函数名称: _ThreadLock
** 功能描述: 线程锁定当前 CPU 调度
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _ThreadLock (VOID)
{
    PLW_CLASS_TCB   ptcbCur;
    
    if (!LW_SYS_STATUS_IS_RUNNING()) {                                  /*  系统必须已经启动            */
        return;
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    if (__THREAD_LOCK_GET(ptcbCur) != __ARCH_ULONG_MAX) {
        __THREAD_LOCK_INC(ptcbCur);
    }
    KN_SMP_MB();
}
/*********************************************************************************************************
** 函数名称: _ThreadUnlock
** 功能描述: 线程解锁当前 CPU 调度
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _ThreadUnlock (VOID)
{
    INTREG          iregInterLevel;
    PLW_CLASS_CPU   pcpuCur;
    PLW_CLASS_TCB   ptcbCur;
    BOOL            bTrySched = LW_FALSE;
   
    if (!LW_SYS_STATUS_IS_RUNNING()) {                                  /*  系统必须已经启动            */
        return;
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    KN_SMP_MB();
    if (__THREAD_LOCK_GET(ptcbCur)) {
        __THREAD_LOCK_DEC(ptcbCur);                                     /*  解锁任务                    */
    }
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  关中断, 禁止 CPU 调度       */
    
    pcpuCur = LW_CPU_GET_CUR();
    if (__COULD_SCHED(pcpuCur, 0)) {
        bTrySched = LW_TRUE;                                            /*  需要尝试调度                */
    }
    
    KN_INT_ENABLE(iregInterLevel);

    if (bTrySched) {
        _ThreadSched(ptcbCur);                                          /*  尝试调度                    */
    }
}
/*********************************************************************************************************
** 函数名称: _ThreadStop
** 功能描述: 停止线程 (进入内核后被调用)
** 输　入  : ptcb          线程控制块
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
** 注  意  : ptcb != current tcb
*********************************************************************************************************/
ULONG  _ThreadStop (PLW_CLASS_TCB  ptcb)
{
    MONITOR_EVT_LONG1(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_STOP, 
                      ptcb->TCB_ulId, LW_NULL);

    return  (_ThreadStatusChange(ptcb, LW_TCB_REQ_STOP));
}
/*********************************************************************************************************
** 函数名称: _ThreadContinue
** 功能描述: 被 _ThreadStop() 的线程继续执行 (进入内核后被调用)
** 输　入  : ptcb          线程控制块
**           bForce        如果产生递归是否强制恢复.
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
** 注  意  : ptcb != current tcb
*********************************************************************************************************/
ULONG  _ThreadContinue (PLW_CLASS_TCB  ptcb, BOOL  bForce)
{
             INTREG         iregInterLevel;
    REGISTER PLW_CLASS_PCB  ppcb;
    
    MONITOR_EVT_LONG1(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_CONT, 
                      ptcb->TCB_ulId, LW_NULL);
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */
    
    if (!(ptcb->TCB_usStatus & LW_THREAD_STATUS_STOP)) {                /*  已经退出了 STOP 状态        */
        KN_INT_ENABLE(iregInterLevel);                                  /*  打开中断                    */
        return  (ERROR_NONE);
    }
    
    if (ptcb->TCB_ulStopNesting) {
        ptcb->TCB_ulStopNesting--;
        if ((ptcb->TCB_ulStopNesting == 0ul) || bForce) {
            ptcb->TCB_ulStopNesting = 0ul;
            ptcb->TCB_usStatus &= (~LW_THREAD_STATUS_STOP);
            if (__LW_THREAD_IS_READY(ptcb)) {                           /*  就绪 ?                      */
                ptcb->TCB_ucSchedActivate = LW_SCHED_ACT_INTERRUPT;     /*  中断激活方式                */
                ppcb = _GetPcb(ptcb);
                __ADD_TO_READY_RING(ptcb, ppcb);                        /*  加入到相对优先级就绪环      */
            }
        }
    }
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  打开中断                    */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _ThreadDebugUnpendSem
** 功能描述: 线程不等待任何调试关键信号量 (进入内核后被调用)
** 输　入  : ptcb      线程控制块指针
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 调试器停止本线程前会调用此函数, 防止调试器释放的资源被已经 stop 的线程获取.
*********************************************************************************************************/
#if LW_CFG_GDB_EN > 0

VOID  _ThreadDebugUnpendSem (PLW_CLASS_TCB  ptcb)
{
    INTREG          iregInterLevel;
    PLW_CLASS_PCB   ppcb;

    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */
    
    if (__LW_THREAD_IS_READY(ptcb)) {                                   /*  处于就绪状态, 直接退出      */
        KN_INT_ENABLE(iregInterLevel);                                  /*  打开中断                    */
        return;
    }
    
    ppcb = _GetPcb(ptcb);                                               /*  获得优先级控制块            */
    if (ptcb->TCB_usStatus & LW_THREAD_STATUS_SEM) {
        if (ptcb->TCB_peventPtr &&
            (ptcb->TCB_peventPtr->EVENT_ulOption & 
             LW_OPTION_OBJECT_DEBUG_UNPEND)) {                          /*  调试关键资源                */
            if (ptcb->TCB_usStatus & LW_THREAD_STATUS_DELAY) {
                __DEL_FROM_WAKEUP_LINE(ptcb);                           /*  从等待链中删除              */
                ptcb->TCB_ulDelay = 0ul;
            }
            
            ptcb->TCB_usStatus &= (~LW_THREAD_STATUS_SEM);              /*  等待超时清除事件等待位      */
            ptcb->TCB_ucWaitTimeout = LW_WAIT_TIME_OUT;                 /*  等待超时                    */
            
            _EventUnQueue(ptcb);
            
            if (__LW_THREAD_IS_READY(ptcb)) {
                ptcb->TCB_ucSchedActivate = LW_SCHED_ACT_OTHER;
                __ADD_TO_READY_RING(ptcb, ppcb);                        /*  加入就绪环                  */
            }
            
            if (ptcb->TCB_iSchedRet == ERROR_NONE) {
                ptcb->TCB_iSchedRet =  LW_SIGNAL_RESTART;               /*  下次调度需要重新等待        */
            }
        }
    }
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  打开中断                    */
}

#endif                                                                  /*  LW_CFG_GDB_EN > 0           */
/*********************************************************************************************************
** 函数名称: _ThreadMakeMain
** 功能描述: 将指定线程转化为进程内主线程
** 输　入  : ulId          线程 ID
**           pvVProc       进程控制块
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0

ULONG  _ThreadMakeMain (LW_HANDLE  ulId, PVOID   pvVProc)
{
             INTREG         iregInterLevel;
    REGISTER UINT16         usIndex;
    REGISTER PLW_CLASS_TCB  ptcb;
             BOOL           bVpAdd;
             LW_LD_VPROC   *pvproc = (LW_LD_VPROC *)pvVProc;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  检查 ID 类型有效性          */
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if (_Thread_Index_Invalid(usIndex)) {                               /*  检查线程有效性              */
        return  (ERROR_THREAD_NULL);
    }
    
    if (_Thread_Invalid(usIndex)) {
        return  (ERROR_THREAD_NULL);
    }
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  进入内核关闭中断            */
    ptcb = _K_ptcbTCBIdTable[usIndex];
    if (ptcb->TCB_ulOption & LW_OPTION_OBJECT_GLOBAL) {
        ptcb->TCB_ulOption &= (~LW_OPTION_OBJECT_GLOBAL);
        bVpAdd = LW_TRUE;
    
    } else {
        bVpAdd = LW_FALSE;
    }
    
    ptcb->TCB_pvVProcessContext = pvVProc;
    pvproc->VP_ulMainThread     = ulId;
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  进入内核打开中断            */
    
    if (bVpAdd) {
        vprocThreadAdd(ptcb->TCB_pvVProcessContext, ptcb);
    }
    
    __resHandleMakeLocal(ulId);
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
** 函数名称: _ThreadTraversal
** 功能描述: 遍历所有的线程 (必须在锁定内核的前提下调用)
** 输　入  : pfunc             回调函数
**           pvArg[0 ~ 5]      遍历函数参数
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _ThreadTraversal (VOIDFUNCPTR    pfunc, 
                        PVOID          pvArg0,
                        PVOID          pvArg1,
                        PVOID          pvArg2,
                        PVOID          pvArg3,
                        PVOID          pvArg4,
                        PVOID          pvArg5)
{
    REGISTER PLW_CLASS_TCB         ptcb;
             PLW_LIST_LINE         plineList;
             
    for (plineList  = _K_plineTCBHeader;
         plineList != LW_NULL;
         plineList  = _list_line_get_next(plineList)) {
         
        ptcb = _LIST_ENTRY(plineList, LW_CLASS_TCB, TCB_lineManage);
        pfunc(ptcb, pvArg0, pvArg1, pvArg2, pvArg3, pvArg4, pvArg5);
    }
}
/*********************************************************************************************************
** 函数名称: _ThreadRestartProcHook
** 功能描述: 线程 Rstart 时对 hook 的处理
** 输　入  : ptcb      任务控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _ThreadRestartProcHook (PLW_CLASS_TCB  ptcb)
{
    LW_OBJECT_HANDLE  ulId = ptcb->TCB_ulId;

    _TCBCleanupPopExt(ptcb);                                            /*  cleanup 操作                */
    bspTaskDeleteHook(ulId, LW_NULL, ptcb);                             /*  用户钩子函数                */
    __LW_THREAD_DELETE_HOOK(ulId, LW_NULL, ptcb);
    bspTCBInitHook(ulId, ptcb);                                         /*  调用钩子函数                */
    __LW_THREAD_INIT_HOOK(ulId, ptcb);
    bspTaskCreateHook(ulId);                                            /*  调用钩子函数                */
    __LW_THREAD_CREATE_HOOK(ulId, ptcb->TCB_ulOption);
}
/*********************************************************************************************************
** 函数名称: _ThreadDeleteProcHook
** 功能描述: 线程 Delete 时对 hook 的处理
** 输　入  : ptcb          任务控制块
**           pvRetVal      任务返回值
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _ThreadDeleteProcHook (PLW_CLASS_TCB  ptcb, PVOID  pvRetVal)
{
    LW_OBJECT_HANDLE  ulId = ptcb->TCB_ulId;

    _TCBCleanupPopExt(ptcb);
    bspTaskDeleteHook(ulId, pvRetVal, ptcb);                            /*  用户钩子函数                */
    __LW_THREAD_DELETE_HOOK(ulId, pvRetVal, ptcb);
    _TCBDestroyExt(ptcb);                                               /*  销毁 TCB 扩展区             */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
