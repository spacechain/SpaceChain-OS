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
** 文   件   名: MsgQueueStatus.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 01 月 06 日
**
** 描        述: 查询消息队列状态

** BUG
2007.09.19  加入 _DebugHandle() 功能。
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock();
2009.04.08  加入对 SMP 多核的支持.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_MsgQueueStatus
** 功能描述: 查询消息队列状态
** 输　入  : 
**           ulId                   消息队列句柄
**           pulMaxMsgNum           消息队列消息总数量        可以为NULL
**           pulCounter             消息队列消息数量          可以为NULL
**           pstMsgLen              消息队列最近一条消息大小  可以为NULL
**           pulOption              消息队列选项指针          可以为NULL
**           pulThreadBlockNum      被解锁的线程数量          可以为NULL
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if (LW_CFG_MSGQUEUE_EN > 0) && (LW_CFG_MAX_MSGQUEUES > 0)

LW_API  
ULONG  API_MsgQueueStatus (LW_OBJECT_HANDLE   ulId,
                           ULONG             *pulMaxMsgNum,
                           ULONG             *pulCounter,
                           size_t            *pstMsgLen,
                           ULONG             *pulOption,
                           ULONG             *pulThreadBlockNum)
{
             INTREG                iregInterLevel;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_EVENT       pevent;
    
    REGISTER PLW_CLASS_MSGQUEUE    pmsgqueue;
    
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
    
    pmsgqueue = (PLW_CLASS_MSGQUEUE)pevent->EVENT_pvPtr;
    
    if (pulMaxMsgNum) {
        *pulMaxMsgNum = pevent->EVENT_ulMaxCounter;                     /*  最大的消息数量              */
    }
    
    if (pulCounter) {
        *pulCounter = pevent->EVENT_ulCounter;                          /*  获得消息数量                */
    }
    
    if (pevent->EVENT_ulCounter) {
        if (pstMsgLen) {
            _MsgQueueGetMsgLen(pmsgqueue, pstMsgLen);                   /*  获得最近的消息长度          */
        }
    } else {
        if (pstMsgLen) {
            *pstMsgLen = 0;
        }
    }
    
    if (pulOption) {
        *pulOption  = pevent->EVENT_ulOption;                           /*  OPTION 选项                 */
    }
    
    if (pulThreadBlockNum) {
        *pulThreadBlockNum  = _EventWaitNum(EVENT_MSG_Q_R, pevent);     /*  线程等待数量                */
        *pulThreadBlockNum += _EventWaitNum(EVENT_MSG_Q_S, pevent);     /*  线程等待数量                */
    }
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  退出内核                    */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_MSGQUEUE_EN > 0      */
                                                                        /*  LW_CFG_MAX_MSGQUEUES > 0    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
