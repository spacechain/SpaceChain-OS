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
** 文   件   名: MsgQueueSend.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 01 月 07 日
**
** 描        述: 向消息队列发送消息(简易接口函数)

** BUG
2007.09.19  加入 _DebugHandle() 功能。
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock();
2009.04.08  加入对 SMP 多核的支持.
2010.01.22  修正进入内核的时机.
2013.03.17  加入对自动截断消息时的判断.
2016.07.21  支持发送等待与超时.
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
** 函数名称: API_MsgQueueSend2
** 功能描述: 向消息队列发送消息
** 输　入  : 
**           ulId                   消息队列句柄
**           pvMsgBuffer            消息缓冲区
**           stMsgLen               消息长短
**           ulTimeout              超时时间
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if (LW_CFG_MSGQUEUE_EN > 0) && (LW_CFG_MAX_MSGQUEUES > 0)

LW_API  
ULONG  API_MsgQueueSend2 (LW_OBJECT_HANDLE  ulId,
                          const PVOID       pvMsgBuffer,
                          size_t            stMsgLen,
                          ULONG             ulTimeout)
{
             INTREG                iregInterLevel;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_EVENT       pevent;
    REGISTER PLW_CLASS_MSGQUEUE    pmsgqueue;
    REGISTER PLW_CLASS_TCB         ptcb;
    REGISTER PLW_CLASS_TCB         ptcbCur;
    REGISTER UINT8                 ucPriorityIndex;
    REGISTER PLW_LIST_RING        *ppringList;                          /*  等待队列地址                */
             ULONG                 ulTimeSave;                          /*  系统事件记录                */
             INT                   iSchedRet;
             
             ULONG                 ulEventOption;                       /*  事件创建选项                */
             size_t                stRealLen;
    
    usIndex = _ObjectGetIndex(ulId);
    
__re_send:
#if LW_CFG_ARG_CHK_EN > 0
    if (!pvMsgBuffer) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pvMsgBuffer invalidate.\r\n");
        _ErrorHandle(ERROR_MSGQUEUE_MSG_NULL);
        return  (ERROR_MSGQUEUE_MSG_NULL);
    }
    if (!stMsgLen) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "ulMsgLen invalidate.\r\n");
        _ErrorHandle(ERROR_MSGQUEUE_MSG_LEN);
        return  (ERROR_MSGQUEUE_MSG_LEN);
    }
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
    
    pmsgqueue = (PLW_CLASS_MSGQUEUE)pevent->EVENT_pvPtr;
    
    if (stMsgLen > pmsgqueue->MSGQUEUE_stEachMsgByteSize) {             /*  长度太长                    */
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "ulMsgLen invalidate.\r\n");
        _ErrorHandle(ERROR_MSGQUEUE_MSG_LEN);
        return  (ERROR_MSGQUEUE_MSG_LEN);
    }
    
    if (_EventWaitNum(EVENT_MSG_Q_R, pevent)) {                         /*  有任务在等待消息            */
        BOOL    bSendOk = LW_TRUE;
        
        if (pevent->EVENT_ulOption & LW_OPTION_WAIT_PRIORITY) {         /*  优先级等待队列              */
            _EVENT_DEL_Q_PRIORITY(EVENT_MSG_Q_R, ppringList);           /*  激活优先级等待线程          */
            ptcb = _EventReadyPriorityLowLevel(pevent, LW_NULL, ppringList);
        
        } else {
            _EVENT_DEL_Q_FIFO(EVENT_MSG_Q_R, ppringList);               /*  激活FIFO等待线程            */
            ptcb = _EventReadyFifoLowLevel(pevent, LW_NULL, ppringList);
        }
        
        if ((stMsgLen > ptcb->TCB_stMaxByteSize) && 
            !(ptcb->TCB_ulRecvOption & LW_OPTION_NOERROR)) {            /*  是否允许自动截断            */
            *ptcb->TCB_pstMsgByteSize = 0;
            ptcb->TCB_stMaxByteSize = 0;
            bSendOk = LW_FALSE;
            
        } else {
            stRealLen = (stMsgLen < ptcb->TCB_stMaxByteSize) ?
                        (stMsgLen) : (ptcb->TCB_stMaxByteSize);         /*  确定信息拷贝长短            */
            
            *ptcb->TCB_pstMsgByteSize = stRealLen;                      /*  保存长短                    */
            lib_memcpy(ptcb->TCB_pvMsgQueueMessage,                     /*  传递消息                    */
                       pvMsgBuffer, 
                       stRealLen);
        }
                   
        KN_INT_ENABLE(iregInterLevel);                                  /*  使能中断                    */
        _EventReadyHighLevel(ptcb, LW_THREAD_STATUS_MSGQUEUE);          /*  处理 TCB                    */
        
        MONITOR_EVT_LONG2(MONITOR_EVENT_ID_MSGQ, MONITOR_EVENT_MSGQ_POST, 
                          ulId, ptcb->TCB_ulId, LW_NULL);
        
        __KERNEL_EXIT();                                                /*  退出内核                    */
        
        if (bSendOk == LW_FALSE) {
            goto    __re_send;                                          /*  重新发送                    */
        }
        
        return  (ERROR_NONE);
        
    } else {                                                            /*  没有线程等待                */
        if (pevent->EVENT_ulCounter < pevent->EVENT_ulMaxCounter) {     /*  检查是否还有空间加          */
            pevent->EVENT_ulCounter++;
            _MsgQueueSendMsg(pmsgqueue, pvMsgBuffer, stMsgLen);         /*  保存消息                    */
            __KERNEL_EXIT_IRQ(iregInterLevel);                          /*  退出内核                    */
            return  (ERROR_NONE);
        
        } else {
            if ((ulTimeout == LW_OPTION_NOT_WAIT) || 
                LW_CPU_GET_CUR_NESTING()) {                             /*  不需要等待                  */
                __KERNEL_EXIT_IRQ(iregInterLevel);                      /*  退出内核                    */
                _ErrorHandle(ERROR_MSGQUEUE_FULL);
                return  (ERROR_MSGQUEUE_FULL);
            }
            
            LW_TCB_GET_CUR(ptcbCur);                                    /*  当前任务控制块              */
            
            ptcbCur->TCB_iPendQ         = EVENT_MSG_Q_S;
            ptcbCur->TCB_usStatus      |= LW_THREAD_STATUS_MSGQUEUE;    /*  写状态位，开始等待          */
            ptcbCur->TCB_ucWaitTimeout  = LW_WAIT_TIME_CLEAR;           /*  清空等待时间                */
            
            if (ulTimeout == LW_OPTION_WAIT_INFINITE) {                 /*  是否是无穷等待              */
                ptcbCur->TCB_ulDelay = 0ul;
            } else {
                ptcbCur->TCB_ulDelay = ulTimeout;                       /*  设置超时时间                */
            }
            __KERNEL_TIME_GET_NO_SPINLOCK(ulTimeSave, ULONG);           /*  记录系统时间                */
    
            if (pevent->EVENT_ulOption & LW_OPTION_WAIT_PRIORITY) {
                _EVENT_INDEX_Q_PRIORITY(ptcbCur->TCB_ucPriority, ucPriorityIndex);
                _EVENT_PRIORITY_Q_PTR(EVENT_MSG_Q_S, ppringList, ucPriorityIndex);
                ptcbCur->TCB_ppringPriorityQueue = ppringList;          /*  记录等待队列位置            */
                _EventWaitPriority(pevent, ppringList);                 /*  加入优先级等待表            */
                
            } else {                                                    /*  按 FIFO 等待                */
                _EVENT_FIFO_Q_PTR(EVENT_MSG_Q_S, ppringList);           /*  确定 FIFO 队列的位置        */
                _EventWaitFifo(pevent, ppringList);                     /*  加入 FIFO 等待表            */
            }
            
            KN_INT_ENABLE(iregInterLevel);                              /*  使能中断                    */
            
            ulEventOption = pevent->EVENT_ulOption;
            
            iSchedRet = __KERNEL_EXIT();                                /*  调度器解锁                  */
            if (iSchedRet) {
                if ((iSchedRet == LW_SIGNAL_EINTR) && 
                    (ulEventOption & LW_OPTION_SIGNAL_INTER)) {
                    _ErrorHandle(EINTR);
                    return  (EINTR);
                }
                ulTimeout = _sigTimeoutRecalc(ulTimeSave, ulTimeout);   /*  重新计算超时时间            */
                if (ulTimeout == LW_OPTION_NOT_WAIT) {
                    _ErrorHandle(ERROR_THREAD_WAIT_TIMEOUT);
                    return  (ERROR_THREAD_WAIT_TIMEOUT);
                }
                goto    __re_send;
            }
            
            if (ptcbCur->TCB_ucWaitTimeout == LW_WAIT_TIME_OUT) {
                _ErrorHandle(ERROR_THREAD_WAIT_TIMEOUT);                /*  超时                        */
                return  (ERROR_THREAD_WAIT_TIMEOUT);
                
            } else {
                if (ptcbCur->TCB_ucIsEventDelete == LW_EVENT_EXIST) {   /*  事件是否存在                */
                    goto    __re_send;                                  /*  重新尝试发送                */
                
                } else {
                    _ErrorHandle(ERROR_MSGQUEUE_WAS_DELETED);           /*  已经被删除                  */
                    return  (ERROR_MSGQUEUE_WAS_DELETED);
                }
            }
        }
    }
}
/*********************************************************************************************************
** 函数名称: API_MsgQueueSend
** 功能描述: 向消息队列发送消息
** 输　入  : 
**           ulId                   消息队列句柄
**           pvMsgBuffer            消息缓冲区
**           stMsgLen               消息长短
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_MsgQueueSend (LW_OBJECT_HANDLE  ulId,
                         const PVOID       pvMsgBuffer,
                         size_t            stMsgLen)
{
    return  (API_MsgQueueSend2(ulId, pvMsgBuffer, stMsgLen, LW_OPTION_NOT_WAIT));
}

#endif                                                                  /*  (LW_CFG_MSGQUEUE_EN > 0)    */
                                                                        /*  (LW_CFG_MAX_MSGQUEUES > 0)  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
