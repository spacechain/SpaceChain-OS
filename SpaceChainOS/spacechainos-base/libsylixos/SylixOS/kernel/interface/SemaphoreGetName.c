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
** 文   件   名: SemaphoreGetName.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 07 月 21 日
**
** 描        述: 获得信号量名

** BUG
2007.07.21  加入 _DebugHandle() 功能
2007.09.19  _DebugHandle() 消息有错误.
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock();
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_SemaphoreGetName
** 功能描述: 获得信号量名
** 输　入  : 
**           ulId                   事件句柄
**           pcName                 名字缓冲区
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
#if (LW_CFG_EVENT_EN > 0) && (LW_CFG_MAX_EVENTS > 0)
LW_API
ULONG  API_SemaphoreGetName (LW_OBJECT_HANDLE  ulId, PCHAR  pcName)
{
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_EVENT       pevent;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!pcName) {                                                      /*  名字缓冲区检查              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name buffer invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_PNAME_NULL);
        return  (ERROR_KERNEL_PNAME_NULL);
    }
    if (_Event_Index_Invalid(usIndex)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
#endif
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */

    pevent = &_K_eventBuffer[usIndex];
    
    switch (pevent->EVENT_ucType) {
    
    case LW_TYPE_EVENT_SEMC:
    case LW_TYPE_EVENT_SEMB:
    case LW_TYPE_EVENT_SEMRW:
    case LW_TYPE_EVENT_MUTEX:
        break;
    
    default:
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        _ErrorHandle(ERROR_EVENT_TYPE);
        return  (ERROR_EVENT_TYPE);
    }
    
    lib_strcpy(pcName, pevent->EVENT_cEventName);
    
    __KERNEL_EXIT();                                                    /*  退出内核                    */

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_EVENT_EN > 0)       */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
