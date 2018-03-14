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
** 文   件   名: SemaphoreCStatusEx.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 03 月 04 日
**
** 描        述: 查询计数型信号量状态:高级接口.

** BUG:
2009.04.08  加入对 SMP 多核的支持.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_SemaphoreCStatusEx
** 功能描述: 查询计数型信号量状态
** 输　入  : 
**           ulId                   事件句柄
**           pulCounter             事件计数值         可以为NULL
**           pulOption              事件选项指针       可以为NULL
**           pulThreadBlockNum      被锁定的线程数量   可以为NULL
**           pulMaxCounter          最大计数值         可以为NULL
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if (LW_CFG_SEMC_EN > 0) && (LW_CFG_MAX_EVENTS > 0)

LW_API  
ULONG  API_SemaphoreCStatusEx (LW_OBJECT_HANDLE   ulId,
                               ULONG             *pulCounter,
                               ULONG             *pulOption,
                               ULONG             *pulThreadBlockNum,
                               ULONG             *pulMaxCounter)
{
             INTREG                iregInterLevel;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_EVENT       pevent;
    
    usIndex = _ObjectGetIndex(ulId);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_SEM_C)) {                         /*  类型是否正确                */
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
    if (_Event_Type_Invalid(usIndex, LW_TYPE_EVENT_SEMC)) {
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        _ErrorHandle(ERROR_EVENT_TYPE);
        return  (ERROR_EVENT_TYPE);
    }
    
    if (pulCounter) {
        *pulCounter = pevent->EVENT_ulCounter;                          /*  当前计数器值                */
    }
    if (pulOption) {
        *pulOption  = pevent->EVENT_ulOption;                           /*  选项                        */
    }
    if (pulThreadBlockNum) {
        *pulThreadBlockNum = _EventWaitNum(EVENT_SEM_Q, pevent);
    }
    
    if (pulMaxCounter) {
        *pulMaxCounter = pevent->EVENT_ulMaxCounter;                    /*  最大计数值                  */
    }
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  退出内核                    */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_SEMC_EN > 0)        */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
