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
** 文   件   名: SemaphoreBPostEx.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 07 月 21 日
**
** 描        述: 释放二进制型信号量(高级接口函数，带有简单消息传递功能)

** BUG
2007.07.21  加入 _DebugHandle() 功能。
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock();
2009.04.08  加入对 SMP 多核的支持.
2010.01.22  修正进入内核的时机.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_SemaphoreBPostEx
** 功能描述: 释放二进制型信号量，高级接口函数，带有简单消息传递功能
** 输　入  : 
**           ulId                   事件句柄
**           pvMsgPtr               简单消息指针
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if (LW_CFG_SEMB_EN > 0) && (LW_CFG_MAX_EVENTS > 0)

LW_API  
ULONG  API_SemaphoreBPostEx (LW_OBJECT_HANDLE  ulId, PVOID  pvMsgPtr)
{
             INTREG                iregInterLevel;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_EVENT       pevent;
    REGISTER PLW_CLASS_TCB         ptcb;
    REGISTER PLW_LIST_RING        *ppringList;                          /*  等待队列地址                */
    
    usIndex = _ObjectGetIndex(ulId);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_SEM_B)) {                         /*  类型是否正确                */
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
    if (_Event_Type_Invalid(usIndex, LW_TYPE_EVENT_SEMB)) {
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        _ErrorHandle(ERROR_EVENT_TYPE);
        return  (ERROR_EVENT_TYPE);
    }
    
    if (_EventWaitNum(EVENT_SEM_Q, pevent)) {
        if (pevent->EVENT_ulOption & LW_OPTION_WAIT_PRIORITY) {         /*  优先级等待队列              */
            _EVENT_DEL_Q_PRIORITY(EVENT_SEM_Q, ppringList);             /*  激活优先级等待线程          */
            ptcb = _EventReadyPriorityLowLevel(pevent, pvMsgPtr, ppringList);
        
        } else {
            _EVENT_DEL_Q_FIFO(EVENT_SEM_Q, ppringList);                 /*  激活FIFO等待线程            */
            ptcb = _EventReadyFifoLowLevel(pevent, pvMsgPtr, ppringList);
        }
        
        KN_INT_ENABLE(iregInterLevel);                                  /*  使能中断                    */
        _EventReadyHighLevel(ptcb, LW_THREAD_STATUS_SEM);               /*  处理 TCB                    */
        
        MONITOR_EVT_LONG2(MONITOR_EVENT_ID_SEMB, MONITOR_EVENT_SEM_POST, 
                          ulId, ptcb->TCB_ulId, LW_NULL);
        
        __KERNEL_EXIT();                                                /*  退出内核                    */
        return  (ERROR_NONE);
    
    } else {                                                            /*  没有线程等待                */
        if (pevent->EVENT_ulCounter == LW_FALSE) {                      /*  检查是否还有空间加          */
            pevent->EVENT_ulCounter = (ULONG)LW_TRUE;
            pevent->EVENT_pvPtr     = pvMsgPtr;                         /*  简单消息保存                */
            __KERNEL_EXIT_IRQ(iregInterLevel);                          /*  退出内核                    */
            return  (ERROR_NONE);
        
        } else {                                                        /*  已经满了                    */
            __KERNEL_EXIT_IRQ(iregInterLevel);                          /*  退出内核                    */
            _ErrorHandle(ERROR_EVENT_FULL);
            return  (ERROR_EVENT_FULL);
        }
    }
}
/*********************************************************************************************************
** 函数名称: API_SemaphoreBPostEx2
** 功能描述: 释放二进制型信号量，高级接口函数，带有简单消息传递功能
** 输　入  : 
**           ulId                   事件句柄
**           pvMsgPtr               简单消息指针
**           pulId                  被激活的任务句柄
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_SemaphoreBPostEx2 (LW_OBJECT_HANDLE  ulId, PVOID  pvMsgPtr, LW_OBJECT_HANDLE  *pulId)
{
             INTREG                iregInterLevel;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_EVENT       pevent;
    REGISTER PLW_CLASS_TCB         ptcb;
    REGISTER PLW_LIST_RING        *ppringList;                          /*  等待队列地址                */
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (pulId) {
        *pulId = LW_OBJECT_HANDLE_INVALID;
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_SEM_B)) {                         /*  类型是否正确                */
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
    if (_Event_Type_Invalid(usIndex, LW_TYPE_EVENT_SEMB)) {
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        _ErrorHandle(ERROR_EVENT_TYPE);
        return  (ERROR_EVENT_TYPE);
    }
    
    if (_EventWaitNum(EVENT_SEM_Q, pevent)) {
        if (pevent->EVENT_ulOption & LW_OPTION_WAIT_PRIORITY) {         /*  优先级等待队列              */
            _EVENT_DEL_Q_PRIORITY(EVENT_SEM_Q, ppringList);             /*  激活优先级等待线程          */
            ptcb = _EventReadyPriorityLowLevel(pevent, pvMsgPtr, ppringList);
        
        } else {
            _EVENT_DEL_Q_FIFO(EVENT_SEM_Q, ppringList);                 /*  激活FIFO等待线程            */
            ptcb = _EventReadyFifoLowLevel(pevent, pvMsgPtr, ppringList);
        }
        
        KN_INT_ENABLE(iregInterLevel);                                  /*  使能中断                    */
        _EventReadyHighLevel(ptcb, LW_THREAD_STATUS_SEM);               /*  处理 TCB                    */
        
        if (pulId) {
            *pulId = ptcb->TCB_ulId;                                    /*  记录激活的任务              */
        }
        
        MONITOR_EVT_LONG2(MONITOR_EVENT_ID_SEMB, MONITOR_EVENT_SEM_POST, 
                          ulId, ptcb->TCB_ulId, LW_NULL);
        
        __KERNEL_EXIT();                                                /*  退出内核                    */
        return  (ERROR_NONE);
    
    } else {                                                            /*  没有线程等待                */
        if (pevent->EVENT_ulCounter == LW_FALSE) {                      /*  检查是否还有空间加          */
            pevent->EVENT_ulCounter = (ULONG)LW_TRUE;
            pevent->EVENT_pvPtr     = pvMsgPtr;                         /*  简单消息保存                */
            __KERNEL_EXIT_IRQ(iregInterLevel);                          /*  退出内核                    */
            return  (ERROR_NONE);
        
        } else {                                                        /*  已经满了                    */
            __KERNEL_EXIT_IRQ(iregInterLevel);                          /*  退出内核                    */
            _ErrorHandle(ERROR_EVENT_FULL);
            return  (ERROR_EVENT_FULL);
        }
    }
}

#endif                                                                  /*  (LW_CFG_SEMB_EN > 0)        */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
