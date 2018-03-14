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
** 文   件   名: SemaphoreRWPend.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2016 年 07 月 20 日
**
** 描        述: 等待读写信号量.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
ulTimeout 取值：
    
    LW_OPTION_NOT_WAIT                       不进行等待
    LW_OPTION_WAIT_A_TICK                    等待一个系统时钟
    LW_OPTION_WAIT_A_SECOND                  等待一秒
    LW_OPTION_WAIT_INFINITE                  永远等待，直到发生为止
*********************************************************************************************************/
/*********************************************************************************************************
** 函数名称: API_SemaphoreRWPendR
** 功能描述: 等待读写信号量读, 由于 rw post 操作不能在中断中进行，可大大缩短关中断时间
** 输　入  : 
**           ulId            事件句柄
**           ulTimeout       等待时间
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
#if (LW_CFG_SEMRW_EN > 0) && (LW_CFG_MAX_EVENTS > 0)

LW_API  
ULONG  API_SemaphoreRWPendR (LW_OBJECT_HANDLE  ulId, ULONG  ulTimeout)
{
             INTREG                iregInterLevel;
             
             PLW_CLASS_TCB         ptcbCur;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_EVENT       pevent;
    REGISTER UINT8                 ucPriorityIndex;
    REGISTER PLW_LIST_RING        *ppringList;
             ULONG                 ulTimeSave;                          /*  系统事件记录                */
             INT                   iSchedRet;
             
             ULONG                 ulEventOption;                       /*  事件创建选项                */
             
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  当前任务控制块              */
    
__wait_again:
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_SEM_RW)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_Event_Index_Invalid(usIndex)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
#endif
    pevent = &_K_eventBuffer[usIndex];
    
    if (!LW_SYS_STATUS_IS_RUNNING()) {                                  /*  系统还没有启动              */
        return  (ERROR_NONE);
    }
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (_Event_Type_Invalid(usIndex, LW_TYPE_EVENT_SEMRW)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        _ErrorHandle(ERROR_EVENT_TYPE);
        return  (ERROR_EVENT_TYPE);
    }
    
    if ((pevent->EVENT_iStatus == EVENT_RW_STATUS_R) &&
        (_EventWaitNum(EVENT_RW_Q_W, pevent) == 0)) {                   /*  当前为读状态并且没有写请求  */
        pevent->EVENT_ulCounter++;                                      /*  操作数++                    */
        __KERNEL_EXIT();                                                /*  退出内核                    */
        if (pevent->EVENT_ulOption & LW_OPTION_DELETE_SAFE) {           /*  安全模式设定                */
            _ThreadSafeInternal();
        }
        return  (ERROR_NONE);
    }
    
    if (ulTimeout == LW_OPTION_NOT_WAIT) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _ErrorHandle(ERROR_THREAD_WAIT_TIMEOUT);                        /*  超时                        */
        return  (ERROR_THREAD_WAIT_TIMEOUT);
    }

    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */
    
    ptcbCur->TCB_iPendQ         = EVENT_RW_Q_R;
    ptcbCur->TCB_usStatus      |= LW_THREAD_STATUS_SEM;                 /*  写状态位，开始等待          */
    ptcbCur->TCB_ucWaitTimeout  = LW_WAIT_TIME_CLEAR;                   /*  清空等待时间                */
    
    if (ulTimeout == LW_OPTION_WAIT_INFINITE) {                         /*  是否是无穷等待              */
        ptcbCur->TCB_ulDelay = 0ul;
    } else {
        ptcbCur->TCB_ulDelay = ulTimeout;                               /*  设置超时时间                */
    }
    __KERNEL_TIME_GET_NO_SPINLOCK(ulTimeSave, ULONG);                   /*  记录系统时间                */
    
    if (pevent->EVENT_ulOption & LW_OPTION_WAIT_PRIORITY) {             /*  按优先级等待                */
        _EVENT_INDEX_Q_PRIORITY(ptcbCur->TCB_ucPriority, ucPriorityIndex);
        _EVENT_PRIORITY_Q_PTR(EVENT_RW_Q_R, ppringList, ucPriorityIndex);
        ptcbCur->TCB_ppringPriorityQueue = ppringList;                  /*  记录等待队列位置            */
        _EventWaitPriority(pevent, ppringList);                         /*  加入优先级等待表            */
        
    } else {                                                            /*  按 FIFO 等待                */
        _EVENT_FIFO_Q_PTR(EVENT_RW_Q_R, ppringList);                    /*  确定 FIFO 队列的位置        */
        _EventWaitFifo(pevent, ppringList);                             /*  加入 FIFO 等待表            */
    }
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  使能中断                    */

    ulEventOption = pevent->EVENT_ulOption;
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_SEMRW, MONITOR_EVENT_SEM_PEND, 
                      ulId, ulTimeout, LW_NULL);
    
    iSchedRet = __KERNEL_EXIT();                                        /*  调度器解锁                  */
    if (iSchedRet) {
        if ((iSchedRet == LW_SIGNAL_EINTR) && 
            (ulEventOption & LW_OPTION_SIGNAL_INTER)) {
            _ErrorHandle(EINTR);
            return  (EINTR);
        }
        ulTimeout = _sigTimeoutRecalc(ulTimeSave, ulTimeout);           /*  重新计算超时时间            */
        if (ulTimeout == LW_OPTION_NOT_WAIT) {
            _ErrorHandle(ERROR_THREAD_WAIT_TIMEOUT);
            return  (ERROR_THREAD_WAIT_TIMEOUT);
        }
        goto    __wait_again;
    }
    
    if (ptcbCur->TCB_ucWaitTimeout == LW_WAIT_TIME_OUT) {               /*  等待超时                    */
        _ErrorHandle(ERROR_THREAD_WAIT_TIMEOUT);                        /*  超时                        */
        return  (ERROR_THREAD_WAIT_TIMEOUT);
        
    } else {
        if (ptcbCur->TCB_ucIsEventDelete == LW_EVENT_EXIST) {           /*  事件是否存在                */
            if (pevent->EVENT_ulOption & LW_OPTION_DELETE_SAFE) {       /*  安全模式设定                */
                _ThreadSafeInternal();
            }
            return  (ERROR_NONE);
        
        } else {
            _ErrorHandle(ERROR_EVENT_WAS_DELETED);                      /*  已经被删除                  */
            return  (ERROR_EVENT_WAS_DELETED);
        }
    }
}
/*********************************************************************************************************
** 函数名称: API_SemaphoreRWPendW
** 功能描述: 等待读写信号量写, 由于 rw post 操作不能在中断中进行，可大大缩短关中断时间
** 输　入  : 
**           ulId            事件句柄
**           ulTimeout       等待时间
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API  
ULONG  API_SemaphoreRWPendW (LW_OBJECT_HANDLE  ulId, ULONG  ulTimeout)
{
             INTREG                iregInterLevel;
             
             PLW_CLASS_TCB         ptcbCur;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_EVENT       pevent;
    REGISTER UINT8                 ucPriorityIndex;
    REGISTER PLW_LIST_RING        *ppringList;
             ULONG                 ulTimeSave;                          /*  系统事件记录                */
             INT                   iSchedRet;
             
             ULONG                 ulEventOption;                       /*  事件创建选项                */
             
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  当前任务控制块              */
    
__wait_again:
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_SEM_RW)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_Event_Index_Invalid(usIndex)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
#endif
    pevent = &_K_eventBuffer[usIndex];
    
    if (!LW_SYS_STATUS_IS_RUNNING()) {                                  /*  系统还没有启动              */
        return  (ERROR_NONE);
    }
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (_Event_Type_Invalid(usIndex, LW_TYPE_EVENT_SEMRW)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        _ErrorHandle(ERROR_EVENT_TYPE);
        return  (ERROR_EVENT_TYPE);
    }
    
    if (pevent->EVENT_ulCounter == 0) {                                 /*  当前没有任何操作            */
        pevent->EVENT_ulCounter++;
        pevent->EVENT_iStatus  = EVENT_RW_STATUS_W;
        pevent->EVENT_pvTcbOwn = (PVOID)ptcbCur;                        /*  保存线程信息                */
        __KERNEL_EXIT();                                                /*  退出内核                    */
        if (pevent->EVENT_ulOption & LW_OPTION_DELETE_SAFE) {           /*  安全模式设定                */
            _ThreadSafeInternal();
        }
        return  (ERROR_NONE);
    }
    
    if (pevent->EVENT_pvTcbOwn == (PVOID)ptcbCur) {                     /*  写递归                      */
        pevent->EVENT_pvPtr = (PVOID)((ULONG)pevent->EVENT_pvPtr + 1);  /*  临时计数器++                */
        __KERNEL_EXIT();                                                /*  退出内核                    */
        return  (ERROR_NONE);
    }

    if (ulTimeout == LW_OPTION_NOT_WAIT) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _ErrorHandle(ERROR_THREAD_WAIT_TIMEOUT);                        /*  超时                        */
        return  (ERROR_THREAD_WAIT_TIMEOUT);
    }
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */
    
    ptcbCur->TCB_iPendQ         = EVENT_RW_Q_W;
    ptcbCur->TCB_usStatus      |= LW_THREAD_STATUS_SEM;                 /*  写状态位，开始等待          */
    ptcbCur->TCB_ucWaitTimeout  = LW_WAIT_TIME_CLEAR;                   /*  清空等待时间                */
    
    if (ulTimeout == LW_OPTION_WAIT_INFINITE) {                         /*  是否是无穷等待              */
        ptcbCur->TCB_ulDelay = 0ul;
    } else {
        ptcbCur->TCB_ulDelay = ulTimeout;                               /*  设置超时时间                */
    }
    __KERNEL_TIME_GET_NO_SPINLOCK(ulTimeSave, ULONG);                   /*  记录系统时间                */
    
    if (pevent->EVENT_ulOption & LW_OPTION_WAIT_PRIORITY) {             /*  按优先级等待                */
        _EVENT_INDEX_Q_PRIORITY(ptcbCur->TCB_ucPriority, ucPriorityIndex);
        _EVENT_PRIORITY_Q_PTR(EVENT_RW_Q_W, ppringList, ucPriorityIndex);
        ptcbCur->TCB_ppringPriorityQueue = ppringList;                  /*  记录等待队列位置            */
        _EventWaitPriority(pevent, ppringList);                         /*  加入优先级等待表            */
        
    } else {                                                            /*  按 FIFO 等待                */
        _EVENT_FIFO_Q_PTR(EVENT_RW_Q_W, ppringList);                    /*  确定 FIFO 队列的位置        */
        _EventWaitFifo(pevent, ppringList);                             /*  加入 FIFO 等待表            */
    }
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  使能中断                    */

    ulEventOption = pevent->EVENT_ulOption;
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_SEMRW, MONITOR_EVENT_SEM_PEND, 
                      ulId, ulTimeout, LW_NULL);
    
    iSchedRet = __KERNEL_EXIT();                                        /*  调度器解锁                  */
    if (iSchedRet) {
        if ((iSchedRet == LW_SIGNAL_EINTR) && 
            (ulEventOption & LW_OPTION_SIGNAL_INTER)) {
            _ErrorHandle(EINTR);
            return  (EINTR);
        }
        ulTimeout = _sigTimeoutRecalc(ulTimeSave, ulTimeout);           /*  重新计算超时时间            */
        if (ulTimeout == LW_OPTION_NOT_WAIT) {
            _ErrorHandle(ERROR_THREAD_WAIT_TIMEOUT);
            return  (ERROR_THREAD_WAIT_TIMEOUT);
        }
        goto    __wait_again;
    }
    
    if (ptcbCur->TCB_ucWaitTimeout == LW_WAIT_TIME_OUT) {               /*  等待超时                    */
        _ErrorHandle(ERROR_THREAD_WAIT_TIMEOUT);                        /*  超时                        */
        return  (ERROR_THREAD_WAIT_TIMEOUT);
        
    } else {
        if (ptcbCur->TCB_ucIsEventDelete == LW_EVENT_EXIST) {           /*  事件是否存在                */
            if (pevent->EVENT_ulOption & LW_OPTION_DELETE_SAFE) {       /*  安全模式设定                */
                _ThreadSafeInternal();
            }
            return  (ERROR_NONE);
        
        } else {
            _ErrorHandle(ERROR_EVENT_WAS_DELETED);                      /*  已经被删除                  */
            return  (ERROR_EVENT_WAS_DELETED);
        }
    }
}

#endif                                                                  /*  (LW_CFG_SEMRW_EN > 0)       */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
