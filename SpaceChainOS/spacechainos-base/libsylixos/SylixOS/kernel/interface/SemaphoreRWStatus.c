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
** 文   件   名: SemaphoreRWStatus.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2016 年 07 月 20 日
**
** 描        述: 查询读写信号量状态
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_SemaphoreRWStatus
** 功能描述: 查询读写信号量状态
** 输　入  : 
**           ulId                   事件句柄
**           pulRWCount             有多少任务正在并发操作读写锁  可以为NULL
**           pulRPend               当前阻塞读操作的数量          可以为NULL
**           pulWPend               当前阻塞写操作的数量          可以为NULL
**           pulOption              事件选项指针                  可以为NULL
**           pulThreadBlockNum      被解锁的线程数量              可以为NULL
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if (LW_CFG_SEMRW_EN > 0) && (LW_CFG_MAX_EVENTS > 0)

LW_API 
ULONG  API_SemaphoreRWStatus (LW_OBJECT_HANDLE   ulId,
                              ULONG             *pulRWCount,
                              ULONG             *pulRPend,
                              ULONG             *pulWPend,
                              ULONG             *pulOption,
                              LW_OBJECT_HANDLE  *pulOwnerId)
{
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_EVENT       pevent;
             
    usIndex = _ObjectGetIndex(ulId);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_SEM_RW)) {
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
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (_Event_Type_Invalid(usIndex, LW_TYPE_EVENT_SEMRW)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        _ErrorHandle(ERROR_EVENT_TYPE);
        return  (ERROR_EVENT_TYPE);
    }
    
    if (pulRWCount) {
        *pulRWCount = pevent->EVENT_ulCounter;
    }
    
    if (pulRPend) {
        *pulRPend = (ULONG)_EventWaitNum(EVENT_RW_Q_R, pevent);
    }
    
    if (pulWPend) {
        *pulWPend = (ULONG)_EventWaitNum(EVENT_RW_Q_W, pevent);
    }
    
    if (pulOption) {
        *pulOption = pevent->EVENT_ulOption;
    }
    
    if (pevent->EVENT_iStatus == EVENT_RW_STATUS_W) {
        if (pulOwnerId) {
            *pulOwnerId = ((PLW_CLASS_TCB)(pevent->EVENT_pvTcbOwn))->TCB_ulId;
        }
    } else {
        if (pulOwnerId) {
            *pulOwnerId = 0ul;
        }
    }
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_SEMRW_EN > 0)       */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
