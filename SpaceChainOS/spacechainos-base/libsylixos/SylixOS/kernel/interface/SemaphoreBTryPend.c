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
** 文   件   名: SemaphoreBTryPend.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 07 月 21 日
**
** 描        述: 无阻塞的等待二进制型信号量

** BUG
2007.07.21  加入 _DebugHandle() 功能。
2009.04.08  加入对 SMP 多核的支持.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
       用来弥补 Pend API 不能在中断中运行的问题，在正常模式时，相当于 Pend 参数 LW_OPTION_NOT_WAIT
*********************************************************************************************************/
/*********************************************************************************************************
** 函数名称: API_SemaphoreBTryPend
** 功能描述: 无阻塞的等待二进制型信号量
** 输　入  : 
**           ulId            事件句柄
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if (LW_CFG_SEMB_EN > 0) && (LW_CFG_MAX_EVENTS > 0)

LW_API  
ULONG  API_SemaphoreBTryPend (LW_OBJECT_HANDLE  ulId)
{
             INTREG                iregInterLevel;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_EVENT       pevent;
    
    usIndex = _ObjectGetIndex(ulId);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_SEM_B)) {
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
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  进入内核                    */
    if (_Event_Type_Invalid(usIndex, LW_TYPE_EVENT_SEMB)) {
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        _ErrorHandle(ERROR_EVENT_TYPE);
        return  (ERROR_EVENT_TYPE);
    }
    
    if (pevent->EVENT_ulCounter) {                                      /*  事件有效                    */
        pevent->EVENT_ulCounter = LW_FALSE;
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核                    */
        return  (ERROR_NONE);
    
    } else {                                                            /*  事件无效                    */
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核                    */
        _ErrorHandle(ERROR_THREAD_WAIT_TIMEOUT);                        /*  没有事件发生                */
        return  (ERROR_THREAD_WAIT_TIMEOUT);
    }
}

#endif                                                                  /*  (LW_CFG_SEMB_EN > 0)        */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
