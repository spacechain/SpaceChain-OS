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
** 文   件   名: SemaphorePostPend.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 01 月 16 日
**
** 描        述: 在 SMP 系统下解决同步的释放一个信号量并同时开始获取另外一个信号量.

** BUG:
2010.08.03  使用新的获取系统时钟方法.
2011.02.23  加入 LW_OPTION_SIGNAL_INTER 选项, 事件可以选择自己是否可被中断打断.
2012.03.20  减少对 _K_ptcbTCBCur 的引用, 尽量采用局部变量, 减少对当前 CPU ID 获取的次数.
2013.05.05  判断调度器返回值, 决定是重启调用还是退出.
2013.07.18  使用新的获取 TCB 的方法, 确保 SMP 系统安全.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_SemaphorePostBPend
** 功能描述: 释放一个信号量后立即开始等待另外一个信号量(中间无任务切换发生)
** 输　入  : ulIdPost             需要释放的信号量
**           ulId                 需要等待的信号量 (二值信号量)
**           ulTimeout            等待超时时间            
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if ((LW_CFG_SEMB_EN > 0) || (LW_CFG_SEMM_EN > 0)) && (LW_CFG_MAX_EVENTS > 0)

LW_API  
ULONG  API_SemaphorePostBPend (LW_OBJECT_HANDLE  ulIdPost, 
                               LW_OBJECT_HANDLE  ulId,
                               ULONG             ulTimeout)
{
             INTREG                iregInterLevel;
             
             PLW_CLASS_TCB         ptcbCur;
    REGISTER UINT16                usIndex;
    REGISTER ULONG                 ulObjectClass;
             ULONG                 ulError;
    
    REGISTER PLW_CLASS_EVENT       pevent;
    REGISTER UINT8                 ucPriorityIndex;
    REGISTER PLW_LIST_RING        *ppringList;
             ULONG                 ulTimeSave;                          /*  系统事件记录                */
             INT                   iSchedRet;
             
             ULONG                 ulEventOption;                       /*  事件创建选项                */
             
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  当前任务控制块              */

    usIndex       = _ObjectGetIndex(ulIdPost);
    ulObjectClass = _ObjectGetClass(ulIdPost);
    
    if ((ulObjectClass != _OBJECT_SEM_B) &&
        (ulObjectClass != _OBJECT_SEM_C) &&
        (ulObjectClass != _OBJECT_SEM_M)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "ulIdPost invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);                         /*  句柄类型错误                */
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (_Event_Index_Invalid(usIndex)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "ulIdPost invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
#endif

    __KERNEL_ENTER();                                                   /*  进入内核                    */
    ulError = API_SemaphorePost(ulIdPost);                              /*  释放信号量                  */
    if (ulError) {
        __KERNEL_EXIT();
        return  (ulError);
    }
    
    usIndex = _ObjectGetIndex(ulId);
    
__wait_again:
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_SEM_B)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        ulError = ERROR_KERNEL_HANDLE_NULL;
        goto    __wait_over;
    }
    if (_Event_Index_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        ulError = ERROR_KERNEL_HANDLE_NULL;
        goto    __wait_over;
    }
#endif
    pevent = &_K_eventBuffer[usIndex];

    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */
    if (_Event_Type_Invalid(usIndex, LW_TYPE_EVENT_SEMB)) {
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        ulError = ERROR_EVENT_TYPE;
        goto    __wait_over;
    }
    
    if (pevent->EVENT_ulCounter) {                                      /*  事件有效                    */
        pevent->EVENT_ulCounter = LW_FALSE;
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核                    */
        ulError = ERROR_NONE;
        goto    __wait_over;
    }
    
    if (ulTimeout == LW_OPTION_NOT_WAIT) {                              /*  不等待                      */
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核                    */
        ulError = ERROR_THREAD_WAIT_TIMEOUT;                            /*  超时                        */
        goto    __wait_over;
    }
    
    ptcbCur->TCB_iPendQ         = EVENT_SEM_Q;
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
        _EVENT_PRIORITY_Q_PTR(EVENT_SEM_Q, ppringList, ucPriorityIndex);
        ptcbCur->TCB_ppringPriorityQueue = ppringList;                  /*  记录等待队列位置            */
        _EventWaitPriority(pevent, ppringList);                         /*  加入优先级等待表            */
    
    } else {                                                            /*  按 FIFO 等待                */
        _EVENT_FIFO_Q_PTR(EVENT_SEM_Q, ppringList);                     /*  确定 FIFO 队列的位置        */
        _EventWaitFifo(pevent, ppringList);                             /*  加入 FIFO 等待表            */
    }
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  使能中断                    */
    
    ulEventOption = pevent->EVENT_ulOption;
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_SEMB, MONITOR_EVENT_SEM_PEND, 
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
        ulError = ERROR_THREAD_WAIT_TIMEOUT;                            /*  超时                        */
        
    } else {
        if (ptcbCur->TCB_ucIsEventDelete == LW_EVENT_EXIST) {           /*  事件是否存在                */
            ulError = ERROR_NONE;                                       /*  正常                        */
        
        } else {
            ulError = ERROR_EVENT_WAS_DELETED;                          /*  已经被删除                  */
        }
    }
    
__wait_over:
    _ErrorHandle(ulError);
    return  (ulError);
}

#endif                                                                  /*  ((LW_CFG_SEMB_EN > 0) ||    */
                                                                        /*   (LW_CFG_SEMM_EN > 0)) &&   */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
** 函数名称: API_SemaphorePostCPend
** 功能描述: 释放一个信号量后立即开始等待另外一个信号量(中间无任务切换发生)
** 输　入  : ulIdPost             需要释放的信号量
**           ulId                 需要等待的信号量 (计数信号量)
**           ulTimeout            等待超时时间            
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if ((LW_CFG_SEMC_EN > 0) || (LW_CFG_SEMM_EN > 0)) && (LW_CFG_MAX_EVENTS > 0)

LW_API  
ULONG  API_SemaphorePostCPend (LW_OBJECT_HANDLE  ulIdPost, 
                               LW_OBJECT_HANDLE  ulId,
                               ULONG             ulTimeout)
{
             INTREG                iregInterLevel;
             
             PLW_CLASS_TCB         ptcbCur;
    REGISTER UINT16                usIndex;
    REGISTER ULONG                 ulObjectClass;
             ULONG                 ulError;
    
    REGISTER PLW_CLASS_EVENT       pevent;
    REGISTER UINT8                 ucPriorityIndex;
    REGISTER PLW_LIST_RING        *ppringList;
             ULONG                 ulTimeSave;                          /*  系统事件记录                */
             INT                   iSchedRet;
             
             ULONG                 ulEventOption;                       /*  事件创建选项                */
             
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  当前任务控制块              */

    usIndex       = _ObjectGetIndex(ulIdPost);
    ulObjectClass = _ObjectGetClass(ulIdPost);
    
    if ((ulObjectClass != _OBJECT_SEM_B) &&
        (ulObjectClass != _OBJECT_SEM_C) &&
        (ulObjectClass != _OBJECT_SEM_M)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "ulIdPost invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);                         /*  句柄类型错误                */
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (_Event_Index_Invalid(usIndex)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "ulIdPost invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
#endif

    __KERNEL_ENTER();                                                   /*  进入内核                    */
    ulError = API_SemaphorePost(ulIdPost);                              /*  释放信号量                  */
    if (ulError) {
        __KERNEL_EXIT();
        return  (ulError);
    }
    
    usIndex = _ObjectGetIndex(ulId);
    
__wait_again:
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_SEM_C)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        ulError = ERROR_KERNEL_HANDLE_NULL;
        goto    __wait_over;
    }
    if (_Event_Index_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        ulError = ERROR_KERNEL_HANDLE_NULL;
        goto    __wait_over;
    }
#endif
    pevent = &_K_eventBuffer[usIndex];

    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */
    if (_Event_Type_Invalid(usIndex, LW_TYPE_EVENT_SEMC)) {
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        ulError = ERROR_EVENT_TYPE;
        goto    __wait_over;
    }
    
    if (pevent->EVENT_ulCounter) {                                      /*  事件有效                    */
        pevent->EVENT_ulCounter--;
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核                    */
        ulError = ERROR_NONE;
        goto    __wait_over;
    }
    
    if (ulTimeout == LW_OPTION_NOT_WAIT) {                              /*  不等待                      */
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核                    */
        ulError = ERROR_THREAD_WAIT_TIMEOUT;                            /*  超时                        */
        goto    __wait_over;
    }
    
    ptcbCur->TCB_iPendQ         = EVENT_SEM_Q;
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
        _EVENT_PRIORITY_Q_PTR(EVENT_SEM_Q, ppringList, ucPriorityIndex);
        ptcbCur->TCB_ppringPriorityQueue = ppringList;                  /*  记录等待队列位置            */
        _EventWaitPriority(pevent, ppringList);                         /*  加入优先级等待表            */
    
    } else {                                                            /*  按 FIFO 等待                */
        _EVENT_FIFO_Q_PTR(EVENT_SEM_Q, ppringList);                     /*  确定 FIFO 队列的位置        */
        _EventWaitFifo(pevent, ppringList);                             /*  加入 FIFO 等待表            */
    }
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  使能中断                    */

    ulEventOption = pevent->EVENT_ulOption;
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_SEMC, MONITOR_EVENT_SEM_PEND, 
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
        ulError = ERROR_THREAD_WAIT_TIMEOUT;                            /*  超时                        */
        
    } else {
        if (ptcbCur->TCB_ucIsEventDelete == LW_EVENT_EXIST) {           /*  事件是否存在                */
            ulError = ERROR_NONE;                                       /*  正常                        */
        
        } else {
            ulError = ERROR_EVENT_WAS_DELETED;                          /*  已经被删除                  */
        }
    }
    
__wait_over:
    _ErrorHandle(ulError);
    return  (ulError);
}

#endif                                                                  /*  ((LW_CFG_SEMC_EN > 0) ||    */
                                                                        /*   (LW_CFG_SEMM_EN > 0)) &&   */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
