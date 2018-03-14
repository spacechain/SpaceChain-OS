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
** 文   件   名: MsgQueueFlush.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 01 月 09 日
**
** 描        述: 将所有等待消息队列的线程就绪

** BUG
2007.09.19  加入 _DebugHandle() 功能。
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock();
2009.04.08  加入对 SMP 多核的支持.
2016.07.21  加入单独对发送和接收线程的 flush 操作.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_MsgQueueFlush
** 功能描述: 将所有等待消息队列的线程就绪
** 输　入  : 
**           ulId                   消息队列句柄
**           pulThreadUnblockNum    被解锁的线程数量   可以为NULL
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if (LW_CFG_MSGQUEUE_EN > 0) && (LW_CFG_MAX_MSGQUEUES > 0)

/*********************************************************************************************************
** 函数名称: API_MsgQueueFlushSend
** 功能描述: 将所有等待消息队列发送的线程就绪
** 输　入  : 
**           ulId                   消息队列句柄
**           pulThreadUnblockNum    被解锁的线程数量   可以为NULL
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_MsgQueueFlushSend (LW_OBJECT_HANDLE  ulId, ULONG  *pulThreadUnblockNum)
{
             INTREG                iregInterLevel;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_EVENT       pevent;
    REGISTER PLW_CLASS_TCB         ptcb;
    REGISTER PLW_LIST_RING        *ppringList;                          /*  等待队列地址                */
    
    usIndex = _ObjectGetIndex(ulId);
    
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
    
    if (pulThreadUnblockNum) {                                          /*  保存将要解锁的线程数量      */
        *pulThreadUnblockNum = _EventWaitNum(EVENT_MSG_Q_S, pevent);
    }
    
    while (_EventWaitNum(EVENT_MSG_Q_S, pevent)) {                      /*  是否存在正在等待的任务      */
        if (pevent->EVENT_ulOption & LW_OPTION_WAIT_PRIORITY) {         /*  优先级等待队列              */
            _EVENT_DEL_Q_PRIORITY(EVENT_MSG_Q_S, ppringList);           /*  激活优先级等待线程          */
            ptcb = _EventReadyPriorityLowLevel(pevent, LW_NULL, ppringList);
        
        } else {
            _EVENT_DEL_Q_FIFO(EVENT_MSG_Q_S, ppringList);               /*  激活FIFO等待线程            */
            ptcb = _EventReadyFifoLowLevel(pevent, LW_NULL, ppringList);
        }
        
        KN_INT_ENABLE(iregInterLevel);                                  /*  打开中断                    */
        _EventReadyHighLevel(ptcb, LW_THREAD_STATUS_MSGQUEUE);          /*  处理 TCB                    */
        iregInterLevel = KN_INT_DISABLE();                              /*  关闭中断                    */
    }
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  退出内核                    */
    
    MONITOR_EVT_LONG1(MONITOR_EVENT_ID_MSGQ, MONITOR_EVENT_MSGQ_FLUSH, ulId, LW_NULL);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_MsgQueueFlushReceive
** 功能描述: 将所有等待消息队列接收的线程就绪
** 输　入  : 
**           ulId                   消息队列句柄
**           pulThreadUnblockNum    被解锁的线程数量   可以为NULL
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_MsgQueueFlushReceive (LW_OBJECT_HANDLE  ulId, ULONG  *pulThreadUnblockNum)
{
             INTREG                iregInterLevel;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_EVENT       pevent;
    REGISTER PLW_CLASS_TCB         ptcb;
    REGISTER PLW_LIST_RING        *ppringList;                          /*  等待队列地址                */
    
    usIndex = _ObjectGetIndex(ulId);
    
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
    
    if (pulThreadUnblockNum) {                                          /*  保存将要解锁的线程数量      */
        *pulThreadUnblockNum = _EventWaitNum(EVENT_MSG_Q_R, pevent);
    }
    
    while (_EventWaitNum(EVENT_MSG_Q_R, pevent)) {                      /*  是否存在正在等待的任务      */
        if (pevent->EVENT_ulOption & LW_OPTION_WAIT_PRIORITY) {         /*  优先级等待队列              */
            _EVENT_DEL_Q_PRIORITY(EVENT_MSG_Q_R, ppringList);           /*  激活优先级等待线程          */
            ptcb = _EventReadyPriorityLowLevel(pevent, LW_NULL, ppringList);
        
        } else {
            _EVENT_DEL_Q_FIFO(EVENT_MSG_Q_R, ppringList);               /*  激活FIFO等待线程            */
            ptcb = _EventReadyFifoLowLevel(pevent, LW_NULL, ppringList);
        }
        
        *ptcb->TCB_pstMsgByteSize = 0;                                  /*  消息长度为 0                */
        KN_INT_ENABLE(iregInterLevel);                                  /*  打开中断                    */
        _EventReadyHighLevel(ptcb, LW_THREAD_STATUS_MSGQUEUE);          /*  处理 TCB                    */
        iregInterLevel = KN_INT_DISABLE();                              /*  关闭中断                    */
    }
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  退出内核                    */
    
    MONITOR_EVT_LONG1(MONITOR_EVENT_ID_MSGQ, MONITOR_EVENT_MSGQ_FLUSH, ulId, LW_NULL);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_MsgQueueFlush
** 功能描述: 将所有等待消息队列的线程就绪
** 输　入  : 
**           ulId                   消息队列句柄
**           pulThreadUnblockNum    被解锁的线程数量   可以为NULL
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_MsgQueueFlush (LW_OBJECT_HANDLE  ulId, ULONG  *pulThreadUnblockNum)
{
    ULONG   ulError;
    ULONG   ulSend = 0, ulReceive = 0;
    
    ulError = API_MsgQueueFlushSend(ulId, &ulSend);
    if (ulError) {
        return  (ulError);
    }
    
    ulError = API_MsgQueueFlushReceive(ulId, &ulReceive);
    if (ulError) {
        return  (ulError);
    }
    
    if (pulThreadUnblockNum) {
        *pulThreadUnblockNum = ulSend + ulReceive;
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_MSGQUEUE_EN > 0)    */
                                                                        /*  (LW_CFG_MAX_MSGQUEUES > 0)  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
