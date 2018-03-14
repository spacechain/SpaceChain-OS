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
** 文   件   名: MsgQueueDelete.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 01 月 06 日
**
** 描        述: 删除消息队列

** BUG
2007.09.19  加入 _DebugHandle() 功能。
2007.11.18  整理注释.
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock();
2009.04.08  加入对 SMP 多核的支持.
2011.07.29  加入对象创建/销毁回调.
2016.07.21  删除时需要激活双向队列阻塞线程.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_MsgQueueDelete
** 功能描述: 删除消息队列
** 输　入  : 
**           pulId     事件句柄指针
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
#if (LW_CFG_MSGQUEUE_EN > 0) && (LW_CFG_MAX_MSGQUEUES > 0)

LW_API  
ULONG  API_MsgQueueDelete (LW_OBJECT_HANDLE  *pulId)
{
             INTREG                iregInterLevel;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_EVENT       pevent;
    REGISTER PLW_CLASS_MSGQUEUE    pmsgqueue;
    REGISTER PLW_CLASS_TCB         ptcb;
    REGISTER PLW_LIST_RING        *ppringList;                          /*  等待队列地址                */
    REGISTER PVOID                 pvFreeLowAddr;
    
    REGISTER LW_OBJECT_HANDLE      ulId;
    
    ulId = *pulId;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_MSGQUEUE)) {                      /*  类型是否正确                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "msgqueue handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_Event_Index_Invalid(usIndex)) {                                /*  下标是否正确                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "msgqueue handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
#endif
    pevent = &_K_eventBuffer[usIndex];
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  进入内核                    */
    if (_Event_Type_Invalid(usIndex, LW_TYPE_EVENT_MSGQUEUE)) {
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "msgqueue handle invalidate.\r\n");
        _ErrorHandle(ERROR_MSGQUEUE_TYPE);
        return  (ERROR_MSGQUEUE_TYPE);
    }
    
    _ObjectCloseId(pulId);                                              /*  清除句柄                    */
    
    pevent->EVENT_ucType = LW_TYPE_EVENT_UNUSED;                        /*  事件类型为空                */
    
    while (_EventWaitNum(EVENT_MSG_Q_R, pevent)) {                      /*  是否存在正在等待读的任务    */
        if (pevent->EVENT_ulOption & LW_OPTION_WAIT_PRIORITY) {         /*  优先级等待队列              */
            _EVENT_DEL_Q_PRIORITY(EVENT_MSG_Q_R, ppringList);           /*  激活优先级等待线程          */
            ptcb = _EventReadyPriorityLowLevel(pevent, LW_NULL, ppringList);
        
        } else {
            _EVENT_DEL_Q_FIFO(EVENT_MSG_Q_R, ppringList);               /*  激活FIFO等待线程            */
            ptcb = _EventReadyFifoLowLevel(pevent, LW_NULL, ppringList);
        }
        
        KN_INT_ENABLE(iregInterLevel);                                  /*  打开中断                    */
        ptcb->TCB_ucIsEventDelete = LW_EVENT_DELETE;                    /*  事件已经被删除              */
        _EventReadyHighLevel(ptcb, LW_THREAD_STATUS_MSGQUEUE);          /*  处理 TCB                    */
        iregInterLevel = KN_INT_DISABLE();                              /*  关闭中断                    */
    }
    
    while (_EventWaitNum(EVENT_MSG_Q_S, pevent)) {                      /*  是否存在正在等待写的任务    */
        if (pevent->EVENT_ulOption & LW_OPTION_WAIT_PRIORITY) {         /*  优先级等待队列              */
            _EVENT_DEL_Q_PRIORITY(EVENT_MSG_Q_S, ppringList);           /*  激活优先级等待线程          */
            ptcb = _EventReadyPriorityLowLevel(pevent, LW_NULL, ppringList);
        
        } else {
            _EVENT_DEL_Q_FIFO(EVENT_MSG_Q_S, ppringList);               /*  激活FIFO等待线程            */
            ptcb = _EventReadyFifoLowLevel(pevent, LW_NULL, ppringList);
        }
        
        KN_INT_ENABLE(iregInterLevel);                                  /*  打开中断                    */
        ptcb->TCB_ucIsEventDelete = LW_EVENT_DELETE;                    /*  事件已经被删除              */
        _EventReadyHighLevel(ptcb, LW_THREAD_STATUS_MSGQUEUE);          /*  处理 TCB                    */
        iregInterLevel = KN_INT_DISABLE();                              /*  关闭中断                    */
    }
    
    pevent->EVENT_pvTcbOwn = LW_NULL;

    pmsgqueue = (PLW_CLASS_MSGQUEUE)pevent->EVENT_pvPtr;                /*  获得 msgqueue 控制块        */
    pvFreeLowAddr = (PVOID)pmsgqueue->MSGQUEUE_pucBufferLowAddr;
    
    _Free_Event_Object(pevent);
    _Free_MsgQueue_Object(pmsgqueue);
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  退出内核                    */
    
    __KHEAP_FREE(pvFreeLowAddr);                                        /*  释放内存                    */
    
    __LW_OBJECT_DELETE_HOOK(ulId);
    
    MONITOR_EVT_LONG1(MONITOR_EVENT_ID_MSGQ, MONITOR_EVENT_MSGQ_DELETE, ulId, LW_NULL);
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "msgqueue \"%s\" has been delete.\r\n", pevent->EVENT_cEventName);
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_MSGQUEUE_EN > 0)    */
                                                                        /*  (LW_CFG_MAX_MSGQUEUES > 0)  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
