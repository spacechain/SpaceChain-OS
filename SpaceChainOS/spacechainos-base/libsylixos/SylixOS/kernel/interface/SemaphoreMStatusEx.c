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
** 文   件   名: SemaphoreMStatusEx.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 03 月 04 日
**
** 描        述: 查询互斥信号量状态:高级接口

** BUG
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock();
2009.02.05  修正了 pulOwnerId BUG.
2009.04.08  加入对 SMP 多核的支持.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_SemaphoreMStatusEx
** 功能描述: 查询互斥信号量状态
** 输　入  : 
**           ulId                   事件句柄
**           pbValue                事件计数值         可以为NULL
**           pulOption              事件选项指针       可以为NULL
**           pulThreadBlockNum      被解锁的线程数量   可以为NULL
**           pulOwnerId             信号量的拥有者     可以为NULL
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if (LW_CFG_SEMM_EN > 0) && (LW_CFG_MAX_EVENTS > 0)

LW_API  
ULONG  API_SemaphoreMStatusEx (LW_OBJECT_HANDLE   ulId,
                               BOOL              *pbValue,
                               ULONG             *pulOption,
                               ULONG             *pulThreadBlockNum,
                               LW_OBJECT_HANDLE  *pulOwnerId)
{
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_EVENT       pevent;
    
    usIndex = _ObjectGetIndex(ulId);
    
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
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (_Event_Type_Invalid(usIndex, LW_TYPE_EVENT_MUTEX)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        _ErrorHandle(ERROR_EVENT_TYPE);
        return  (ERROR_EVENT_TYPE);
    }
    
    if (pbValue) {
        *pbValue = (BOOL)pevent->EVENT_ulCounter;
    }
    if (pulOption) {
        *pulOption = pevent->EVENT_ulOption;
    }
    if (pulThreadBlockNum) {
        *pulThreadBlockNum = _EventWaitNum(EVENT_SEM_Q, pevent);
    }
    if (!pevent->EVENT_ulCounter) {
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

#endif                                                                  /*  (LW_CFG_SEMM_EN > 0)        */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
