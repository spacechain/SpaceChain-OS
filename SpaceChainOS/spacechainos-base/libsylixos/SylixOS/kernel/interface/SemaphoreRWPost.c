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
** 文   件   名: SemaphoreRWPost.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2016 年 07 月 20 日
**
** 描        述: 释放读写信号量, 不可在中断中操作.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_SemaphoreRWPost
** 功能描述: 释放读写信号量, 不可在中断中操作
** 输　入  : 
**           ulId                   事件句柄
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if (LW_CFG_SEMRW_EN > 0) && (LW_CFG_MAX_EVENTS > 0)

LW_API  
ULONG  API_SemaphoreRWPost (LW_OBJECT_HANDLE  ulId)
{
             INTREG                iregInterLevel;
             
             PLW_CLASS_TCB         ptcbCur;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_EVENT       pevent;
    REGISTER PLW_CLASS_TCB         ptcb;
    REGISTER PLW_LIST_RING        *ppringList;                          /*  等待队列地址                */
             BOOL                  bIgnWrite = LW_FALSE;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  当前任务控制块              */
    
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
    
    if (!LW_SYS_STATUS_IS_RUNNING()) {                                  /*  系统还没有启动              */
        return  (ERROR_NONE);
    }
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (_Event_Type_Invalid(usIndex, LW_TYPE_EVENT_SEMRW)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        _ErrorHandle(ERROR_EVENT_TYPE);
        return  (ERROR_EVENT_TYPE);
    }
    
    if (pevent->EVENT_ulCounter == 0) {                                 /*  没有加锁                    */
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _ErrorHandle(ERROR_EVENT_NOT_OWN);                              /*  没有事件所有权              */
        return  (ERROR_EVENT_NOT_OWN);
    }
    
    if (pevent->EVENT_iStatus == EVENT_RW_STATUS_R) {                   /*  当前为读操作                */
        if (pevent->EVENT_ulCounter > 1) {                              /*  还有其他读操作正在执行      */
            pevent->EVENT_ulCounter--;
            __KERNEL_EXIT();                                            /*  退出内核                    */
            if (pevent->EVENT_ulOption & LW_OPTION_DELETE_SAFE) {       /*  退出安全模式                */
                _ThreadUnsafeInternal();
            }
            return  (ERROR_NONE);
        
        } else {                                                        /*  不存在其他操作者            */
            if (!(pevent->EVENT_ulOption & LW_OPTION_RW_PREFER_WRITER)) {
                if (_EventWaitNum(EVENT_RW_Q_R, pevent)) {
                    bIgnWrite = LW_TRUE;                                /*  忽略激活写者, 优先激活读者  */
                }
            }
            goto    __release_pend;
        }
        
    } else {                                                            /*  写操作                      */
        if (pevent->EVENT_pvTcbOwn != (PVOID)ptcbCur) {
            __KERNEL_EXIT();                                            /*  退出内核                    */
            _ErrorHandle(ERROR_EVENT_NOT_OWN);                          /*  没有事件所有权              */
            return  (ERROR_EVENT_NOT_OWN);
        }
        
        if (pevent->EVENT_pvPtr) {                                      /*  检测是否进行了连续调用      */
            pevent->EVENT_pvPtr = (PVOID)((ULONG)pevent->EVENT_pvPtr - 1);
            __KERNEL_EXIT();                                            /*  退出内核                    */
            return  (ERROR_NONE);
        }
        
__release_pend:
        iregInterLevel = KN_INT_DISABLE();                              /*  关闭中断                    */

        if (_EventWaitNum(EVENT_RW_Q_W, pevent) && !bIgnWrite) {        /*  存在等待写的任务            */
            if (pevent->EVENT_ulOption & LW_OPTION_WAIT_PRIORITY) {
                _EVENT_DEL_Q_PRIORITY(EVENT_RW_Q_W, ppringList);        /*  激活优先级等待线程          */
                ptcb = _EventReadyPriorityLowLevel(pevent, LW_NULL, ppringList);
            
            } else {
                _EVENT_DEL_Q_FIFO(EVENT_RW_Q_W, ppringList);            /*  激活FIFO等待线程            */
                ptcb = _EventReadyFifoLowLevel(pevent, LW_NULL, ppringList);
            }
            
            KN_INT_ENABLE(iregInterLevel);
            _EventReadyHighLevel(ptcb, LW_THREAD_STATUS_SEM);           /*  处理 TCB                    */
            
            pevent->EVENT_pvTcbOwn = (PVOID)ptcb;
            pevent->EVENT_iStatus  = EVENT_RW_STATUS_W;
            
            MONITOR_EVT_LONG2(MONITOR_EVENT_ID_SEMRW, MONITOR_EVENT_SEM_POST, 
                              ulId, ptcb->TCB_ulId, LW_NULL);
            
            __KERNEL_EXIT();                                            /*  退出内核                    */
            
            if (pevent->EVENT_ulOption & LW_OPTION_DELETE_SAFE) {       /*  退出安全模式                */
                _ThreadUnsafeInternal();
            }
            return  (ERROR_NONE);
        
        } else if (_EventWaitNum(EVENT_RW_Q_R, pevent)) {               /*  存在等待读的任务            */
            pevent->EVENT_ulCounter--;
            
            while (_EventWaitNum(EVENT_RW_Q_R, pevent)) {               /*  激活全部读任务              */
                if (pevent->EVENT_ulOption & LW_OPTION_WAIT_PRIORITY) {
                    _EVENT_DEL_Q_PRIORITY(EVENT_RW_Q_R, ppringList);    /*  激活优先级等待线程          */
                    ptcb = _EventReadyPriorityLowLevel(pevent, LW_NULL, ppringList);
                    
                } else {
                    _EVENT_DEL_Q_FIFO(EVENT_RW_Q_R, ppringList);        /*  激活FIFO等待线程            */
                    ptcb = _EventReadyFifoLowLevel(pevent, LW_NULL, ppringList);
                }
                
                pevent->EVENT_ulCounter++;                              /*  增加使用者计数              */
                
                KN_INT_ENABLE(iregInterLevel);                          /*  打开中断                    */
                _EventReadyHighLevel(ptcb, LW_THREAD_STATUS_SEM);       /*  处理 TCB                    */
                
                MONITOR_EVT_LONG2(MONITOR_EVENT_ID_SEMRW, MONITOR_EVENT_SEM_POST, 
                                  ulId, ptcb->TCB_ulId, LW_NULL);
                
                iregInterLevel = KN_INT_DISABLE();                      /*  关闭中断                    */
            }
            
            KN_INT_ENABLE(iregInterLevel);
            
            pevent->EVENT_pvTcbOwn = LW_NULL;
            pevent->EVENT_iStatus  = EVENT_RW_STATUS_R;                 /*  恢复为读状态                */
            
            __KERNEL_EXIT();                                            /*  退出内核                    */
            
            if (pevent->EVENT_ulOption & LW_OPTION_DELETE_SAFE) {       /*  退出安全模式                */
                _ThreadUnsafeInternal();
            }
            return  (ERROR_NONE);
        
        } else {
            KN_INT_ENABLE(iregInterLevel);
        
            pevent->EVENT_ulCounter--;
            pevent->EVENT_pvTcbOwn = LW_NULL;
            pevent->EVENT_iStatus  = EVENT_RW_STATUS_R;                 /*  恢复为读状态                */
            __KERNEL_EXIT();                                            /*  退出内核                    */
            
            if (pevent->EVENT_ulOption & LW_OPTION_DELETE_SAFE) {       /*  退出安全模式                */
                _ThreadUnsafeInternal();
            }
            return  (ERROR_NONE);
        }
    }
}

#endif                                                                  /*  (LW_CFG_SEMRW_EN > 0)       */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
