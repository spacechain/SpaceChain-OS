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
** 文   件   名: SemaphoreRWDelete.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2016 年 07 月 20 日
**
** 描        述: 读写信号量删除.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_SemaphoreRWDelete
** 功能描述: 读写信号量删除, 因为 RW 的所有操作函数均不能在中断中运行，所以这里不用关中断
** 输　入  : 
**           pulId            事件句柄指针
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
#if (LW_CFG_SEMRW_EN > 0) && (LW_CFG_MAX_EVENTS > 0)

LW_API 
ULONG  API_SemaphoreRWDelete (LW_OBJECT_HANDLE  *pulId)
{
             INTREG                iregInterLevel;
    REGISTER ULONG                 ulOptionTemp;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_EVENT       pevent;
    REGISTER PLW_CLASS_TCB         ptcb;
    REGISTER PLW_LIST_RING        *ppringList;                          /*  等待队列地址                */
    
    REGISTER LW_OBJECT_HANDLE      ulId;
    
    ulId = *pulId;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_SEM_RW)) {                        /*  类型是否正确                */
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
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  进入内核                    */
    if (_Event_Type_Invalid(usIndex, LW_TYPE_EVENT_SEMRW)) {
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        _ErrorHandle(ERROR_EVENT_TYPE);
        return  (ERROR_EVENT_TYPE);
    }

    _ObjectCloseId(pulId);                                              /*  清除句柄                    */
    
    pevent->EVENT_ucType = LW_TYPE_EVENT_UNUSED;                        /*  事件类型为空                */
    
    while (_EventWaitNum(EVENT_RW_Q_R, pevent)) {                       /*  是否存在正在等待读的任务    */
        if (pevent->EVENT_ulOption & LW_OPTION_WAIT_PRIORITY) {         /*  优先级等待队列              */
            _EVENT_DEL_Q_PRIORITY(EVENT_RW_Q_R, ppringList);            /*  激活优先级等待线程          */
            ptcb = _EventReadyPriorityLowLevel(pevent, LW_NULL, ppringList);
        
        } else {
            _EVENT_DEL_Q_FIFO(EVENT_RW_Q_R, ppringList);                /*  激活FIFO等待线程            */
            ptcb = _EventReadyFifoLowLevel(pevent, LW_NULL, ppringList);
        }

        KN_INT_ENABLE(iregInterLevel);
        ptcb->TCB_ucIsEventDelete = LW_EVENT_DELETE;                    /*  事件已经被删除              */
        _EventReadyHighLevel(ptcb, LW_THREAD_STATUS_SEM);               /*  处理 TCB                    */
        iregInterLevel = KN_INT_DISABLE();
    }
    
    while (_EventWaitNum(EVENT_RW_Q_W, pevent)) {                       /*  是否存在正在等待写的任务    */
        if (pevent->EVENT_ulOption & LW_OPTION_WAIT_PRIORITY) {         /*  优先级等待队列              */
            _EVENT_DEL_Q_PRIORITY(EVENT_RW_Q_W, ppringList);            /*  激活优先级等待线程          */
            ptcb = _EventReadyPriorityLowLevel(pevent, LW_NULL, ppringList);
        
        } else {
            _EVENT_DEL_Q_FIFO(EVENT_RW_Q_W, ppringList);                /*  激活FIFO等待线程            */
            ptcb = _EventReadyFifoLowLevel(pevent, LW_NULL, ppringList);
        }

        KN_INT_ENABLE(iregInterLevel);
        ptcb->TCB_ucIsEventDelete = LW_EVENT_DELETE;                    /*  事件已经被删除              */
        _EventReadyHighLevel(ptcb, LW_THREAD_STATUS_SEM);               /*  处理 TCB                    */
        iregInterLevel = KN_INT_DISABLE();
    }
    
    ptcb = (PLW_CLASS_TCB)pevent->EVENT_pvTcbOwn;                       /*  获得拥有者 TCB              */
    pevent->EVENT_pvTcbOwn = LW_NULL;
    
    _Free_Event_Object(pevent);                                         /*  交还控制块                  */
    
    ulOptionTemp = pevent->EVENT_ulOption;                              /*  暂存选项                    */
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  退出内核                    */
    
    __LW_OBJECT_DELETE_HOOK(ulId);
    
    MONITOR_EVT_LONG1(MONITOR_EVENT_ID_SEMRW, MONITOR_EVENT_SEM_DELETE, ulId, LW_NULL);
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "semaphore \"%s\" has been delete.\r\n", pevent->EVENT_cEventName);
    
    if (ptcb) {
        if (ulOptionTemp & LW_OPTION_DELETE_SAFE) {                     /*  退出安全模式                */
            _ThreadUnsafeInternalEx(ptcb);
        }
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_SEMRW_EN > 0)       */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
