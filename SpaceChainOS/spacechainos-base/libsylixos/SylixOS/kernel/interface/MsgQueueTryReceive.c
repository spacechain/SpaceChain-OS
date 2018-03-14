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
** 文   件   名: MsgQueueTryReceive.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 01 月 07 日
**
** 描        述: 无等待获得消息队列消息

** BUG
2007.09.19  加入 _DebugHandle() 功能。
2009.04.08  加入对 SMP 多核的支持.
2009.06.25  pulMsgLen 可以为 NULL,
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_MsgQueueTryReceive
** 功能描述: 无等待获得消息队列消息
** 输　入  : 
**           ulId            消息队列句柄
**           pvMsgBuffer     消息缓冲区
**           stMaxByteSize   消息缓冲区大小
**           pstMsgLen       消息长度
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if (LW_CFG_MSGQUEUE_EN > 0) && (LW_CFG_MAX_MSGQUEUES > 0)

LW_API  
ULONG  API_MsgQueueTryReceive (LW_OBJECT_HANDLE    ulId,
                               PVOID               pvMsgBuffer,
                               size_t              stMaxByteSize,
                               size_t             *pstMsgLen)
{
             INTREG                iregInterLevel;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_EVENT       pevent;
    REGISTER PLW_CLASS_TCB         ptcb;
    REGISTER PLW_CLASS_MSGQUEUE    pmsgqueue;
    REGISTER PLW_LIST_RING        *ppringList;
            
             size_t                stMsgLenTemp;
             
    usIndex = _ObjectGetIndex(ulId);
    
    if (pstMsgLen == LW_NULL) {
        pstMsgLen =  &stMsgLenTemp;                                     /*  临时变量记录消息长短        */
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!pvMsgBuffer) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pvMsgBuffer invalidate.\r\n");
        _ErrorHandle(ERROR_MSGQUEUE_MSG_NULL);
        return  (ERROR_MSGQUEUE_MSG_NULL);
    }
    if (!_ObjectClassOK(ulId, _OBJECT_MSGQUEUE)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "msgqueue handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_Event_Index_Invalid(usIndex)) {
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
    
    pmsgqueue = (PLW_CLASS_MSGQUEUE)pevent->EVENT_pvPtr;
    
    if (pevent->EVENT_ulCounter) {                                      /*  事件有效                    */
        pevent->EVENT_ulCounter--;
        
        _MsgQueueGetMsg(pmsgqueue, pvMsgBuffer, 
                        stMaxByteSize, pstMsgLen);                      /*  获得消息                    */
        
        if (_EventWaitNum(EVENT_MSG_Q_S, pevent)) {                     /*  有任务在等待写消息          */
            if (pevent->EVENT_ulOption & LW_OPTION_WAIT_PRIORITY) {     /*  优先级等待队列              */
                _EVENT_DEL_Q_PRIORITY(EVENT_MSG_Q_S, ppringList);       /*  激活优先级等待线程          */
                ptcb = _EventReadyPriorityLowLevel(pevent, LW_NULL, ppringList);
            
            } else {
                _EVENT_DEL_Q_FIFO(EVENT_MSG_Q_S, ppringList);           /*  激活FIFO等待线程            */
                ptcb = _EventReadyFifoLowLevel(pevent, LW_NULL, ppringList);
            }
            
            KN_INT_ENABLE(iregInterLevel);                              /*  使能中断                    */
            _EventReadyHighLevel(ptcb, LW_THREAD_STATUS_MSGQUEUE);      /*  处理 TCB                    */
            __KERNEL_EXIT();                                            /*  退出内核                    */
        
        } else {
            __KERNEL_EXIT_IRQ(iregInterLevel);                          /*  退出内核                    */
        }
        return  (ERROR_NONE);
    
    } else {                                                            /*  事件无效                    */
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核                    */
        _ErrorHandle(ERROR_THREAD_WAIT_TIMEOUT);                        /*  没有事件发生                */
        return  (ERROR_THREAD_WAIT_TIMEOUT);
    }
}

#endif                                                                  /*  (LW_CFG_MSGQUEUE_EN > 0)    */
                                                                        /*  (LW_CFG_MAX_MSGQUEUES > 0)  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
