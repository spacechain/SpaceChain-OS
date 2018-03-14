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
** 文   件   名: SemaphoreMPend.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 07 月 21 日
**
** 描        述: 等待互斥信号量

** BUG
2007.05.07  当使用优先级天花板时，_ThreadMove() 前，应该判断 Mutex 拥有者的优先级是否与天花板相同。
2007.07.21  加入 _DebugHandle() 功能。
2007.10.28  不能在调度器被关闭的情况下调用.
2008.01.20  调度器已经有了重大的改进, 可以在调度器被锁定的情况下调用此 API.
2008.03.30  由于使用了分离的关闭调度器方式, 所以对线程属性块的操作必须放在关闭中断的情况下.
2008.05.03  加入信号令调度器返回 restart 进行重新阻塞的处理.
2008.05.18  去掉对 LW_EVENT_EXIST, 放在了其他地方.
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock();
2008.08.26  昨天过生日了, 正式 25 岁了, 有点老了啊! 
            对 MUTEX 信号量作出改进, 当单线程连续调用 pend 操作时, 不会阻塞, 
            但只有连续释放调用的次数后, 才会释放.
2009.04.08  加入对 SMP 多核的支持.
2009.05.28  自从加入多核支持, 关闭中断时间延长了, 今天进行一些优化.
2009.06.05  上个月做的优化有一处问题调试了很久, 就是超时退出时应该关中断.
2009.10.11  将TCB_ulWakeTimer赋值和ulTimeSave赋值提前. 放在等待类型判断分支前面.
2010.08.03  使用新的获取系统时钟方法.
2011.02.23  加入 LW_OPTION_SIGNAL_INTER 选项, 事件可以选择自己是否可被中断打断.
2012.03.20  减少对 _K_ptcbTCBCur 的引用, 尽量采用局部变量, 减少对当前 CPU ID 获取的次数.
2012.12.08  不仅支持递归, 还支持不检查和检查.
2013.05.05  判断调度器返回值, 决定是重启调用还是退出.
2013.07.18  使用新的获取 TCB 的方法, 确保 SMP 系统安全.
2013.12.11  系统没有启动时 pend 操作不工作.
2014.05.29  修复超时后瞬间激活时没有设置任务安全属性错误.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  注意:
       MUTEX 不同于 COUNTING, BINERY, 一个线程必须成对使用.
       
       API_SemaphoreMPend();
       ... (do something as fast as possible...)
       API_SemaphoreMPost();
*********************************************************************************************************/
/*********************************************************************************************************
ulTimeout 取值：
    
    LW_OPTION_NOT_WAIT                       不进行等待
    LW_OPTION_WAIT_A_TICK                    等待一个系统时钟
    LW_OPTION_WAIT_A_SECOND                  等待一秒
    LW_OPTION_WAIT_INFINITE                  永远等待，直到发生为止
*********************************************************************************************************/
/*********************************************************************************************************
** 函数名称: API_SemaphoreMPend
** 功能描述: 等待互斥信号量            由于 mutext post 操作不能在中断中进行，可大大缩短关中断时间
** 输　入  : 
**           ulId            事件句柄
**           ulTimeout       等待时间
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
#if (LW_CFG_SEMM_EN > 0) && (LW_CFG_MAX_EVENTS > 0)

LW_API  
ULONG  API_SemaphoreMPend (LW_OBJECT_HANDLE  ulId, ULONG  ulTimeout)
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
    if (!_ObjectClassOK(ulId, _OBJECT_SEM_M)) {
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
    if (_Event_Type_Invalid(usIndex, LW_TYPE_EVENT_MUTEX)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        _ErrorHandle(ERROR_EVENT_TYPE);
        return  (ERROR_EVENT_TYPE);
    }
    
    if (pevent->EVENT_ulCounter) {                                      /*  事件有效                    */
        pevent->EVENT_ulCounter    = LW_FALSE;
        pevent->EVENT_ulMaxCounter = (ULONG)ptcbCur->TCB_ucPriority;
        pevent->EVENT_pvTcbOwn     = (PVOID)ptcbCur;                    /*  保存线程信息                */
        __KERNEL_EXIT();                                                /*  退出内核                    */
        if (pevent->EVENT_ulOption & LW_OPTION_DELETE_SAFE) {           /*  安全模式设定                */
            _ThreadSafeInternal();
        }
        return  (ERROR_NONE);
    }
    
    if (!(pevent->EVENT_ulOption & LW_OPTION_NORMAL)) {                 /*  需要递归支持或递归检查      */
        if (pevent->EVENT_pvTcbOwn == (PVOID)ptcbCur) {                 /*  是否是自己连续调用          */
            if (pevent->EVENT_ulOption & LW_OPTION_ERRORCHECK) {
                __KERNEL_EXIT();                                        /*  退出内核                    */
                _ErrorHandle(EDEADLK);                                  /*  退出                        */
                return  (EDEADLK);
            }
                                                                        /*  临时计数器++                */
            pevent->EVENT_pvPtr = (PVOID)((ULONG)pevent->EVENT_pvPtr + 1);
            __KERNEL_EXIT();                                            /*  退出内核                    */
            return  (ERROR_NONE);
        }
    }

    if (ulTimeout == LW_OPTION_NOT_WAIT) {                              /*  不等待                      */
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _ErrorHandle(ERROR_THREAD_WAIT_TIMEOUT);                        /*  超时                        */
        return  (ERROR_THREAD_WAIT_TIMEOUT);
    }
    
    _EventPrioTryBoost(pevent, ptcbCur);                                /*  尝试提升所属任务优先级      */
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */
    
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
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_SEMM, MONITOR_EVENT_SEM_PEND, 
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

#endif                                                                  /*  (LW_CFG_SEMM_EN > 0)        */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
