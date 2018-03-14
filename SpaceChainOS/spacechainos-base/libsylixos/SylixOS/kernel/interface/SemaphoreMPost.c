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
** 文   件   名: SemaphoreMPost.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 07 月 21 日
**
** 描        述: 释放互斥信号量, 不可在中断中操作

** BUG
2007.07.21  加入 _DebugHandle() 功能。
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock();
2008.08.26  对 MUTEX 信号量作出改进, 当单线程连续调用 pend 操作时, 不会阻塞, 
            但只有连续释放调用的次数后, 才会释放.
2009.04.08  加入对 SMP 多核的支持.
2012.03.20  减少对 _K_ptcbTCBCur 的引用, 尽量采用局部变量, 减少对当前 CPU ID 获取的次数.
2013.07.18  使用新的获取 TCB 的方法, 确保 SMP 系统安全.
2013.12.11  系统没有启动时 post 操作不工作.
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
** 函数名称: API_SemaphoreMPost
** 功能描述: 释放互斥信号量, 不可在中断中操作
** 输　入  : 
**           ulId                   事件句柄
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if (LW_CFG_SEMM_EN > 0) && (LW_CFG_MAX_EVENTS > 0)

LW_API  
ULONG  API_SemaphoreMPost (LW_OBJECT_HANDLE  ulId)
{
             INTREG                iregInterLevel;
             
             PLW_CLASS_TCB         ptcbCur;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_EVENT       pevent;
    REGISTER PLW_CLASS_TCB         ptcb;
    REGISTER PLW_LIST_RING        *ppringList;                          /*  等待队列地址                */
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  当前任务控制块              */
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_SEM_M)) {                         /*  类型是否正确                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_Event_Index_Invalid(usIndex)) {                                /*  下标是否正确                */
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
    
    if (ptcbCur != (PLW_CLASS_TCB)pevent->EVENT_pvTcbOwn) {             /*  是否是拥有者                */
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _ErrorHandle(ERROR_EVENT_NOT_OWN);                              /*  没有事件所有权              */
        return  (ERROR_EVENT_NOT_OWN);
    }
    
    if (pevent->EVENT_pvPtr) {                                          /*  检测是否进行了递归调用      */
        pevent->EVENT_pvPtr = (PVOID)((ULONG)pevent->EVENT_pvPtr - 1);  /*  临时计数器--                */
        __KERNEL_EXIT();                                                /*  退出内核                    */
        return  (ERROR_NONE);
    }
    
    iregInterLevel = KN_INT_DISABLE();
    
    if (_EventWaitNum(EVENT_SEM_Q, pevent)) {
        if (pevent->EVENT_ulOption & LW_OPTION_WAIT_PRIORITY) {         /*  优先级等待队列              */
            _EVENT_DEL_Q_PRIORITY(EVENT_SEM_Q, ppringList);             /*  激活优先级等待线程          */
            ptcb = _EventReadyPriorityLowLevel(pevent, LW_NULL, ppringList);
        
        } else {
            _EVENT_DEL_Q_FIFO(EVENT_SEM_Q, ppringList);                 /*  激活FIFO等待线程            */
            ptcb = _EventReadyFifoLowLevel(pevent, LW_NULL, ppringList);
        }
        
        KN_INT_ENABLE(iregInterLevel);
        
        _EventPrioTryResume(pevent, ptcbCur);                           /*  尝试返回之前的优先级        */
        
        pevent->EVENT_ulMaxCounter = (ULONG)ptcb->TCB_ucPriority;
        pevent->EVENT_pvTcbOwn     = (PVOID)ptcb;                       /*  保存线程信息                */

        _EventReadyHighLevel(ptcb, LW_THREAD_STATUS_SEM);               /*  处理 TCB                    */
        
        MONITOR_EVT_LONG2(MONITOR_EVENT_ID_SEMM, MONITOR_EVENT_SEM_POST, 
                          ulId, ptcb->TCB_ulId, LW_NULL);
        
        __KERNEL_EXIT();                                                /*  退出内核                    */
        
        if (pevent->EVENT_ulOption & LW_OPTION_DELETE_SAFE) {           /*  退出安全模式                */
            _ThreadUnsafeInternal();
        }
        return  (ERROR_NONE);
    
    } else {                                                            /*  没有线程等待                */
        KN_INT_ENABLE(iregInterLevel);
        
        if (pevent->EVENT_ulCounter == LW_FALSE) {                      /*  检查是否还有空间加          */
            pevent->EVENT_ulCounter = (ULONG)LW_TRUE;
            
            pevent->EVENT_ulMaxCounter = LW_PRIO_LOWEST;                /*  清空保存信息                */
            pevent->EVENT_pvTcbOwn     = LW_NULL;
            
            __KERNEL_EXIT();                                            /*  退出内核                    */
            
            if (pevent->EVENT_ulOption & LW_OPTION_DELETE_SAFE) {       /*  退出安全模式                */
                _ThreadUnsafeInternal();
            }
            return  (ERROR_NONE);
        
        } else {                                                        /*  已经满了                    */
            __KERNEL_EXIT();                                            /*  退出内核                    */
            _ErrorHandle(ERROR_EVENT_FULL);
            return  (ERROR_EVENT_FULL);
        }
    }
}

#endif                                                                  /*  (LW_CFG_SEMM_EN > 0)        */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
