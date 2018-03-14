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
** 文   件   名: ThreadSetPriority.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 07 月 19 日
**
** 描        述: 设置线程优先级。

** BUG
2007.05.11  没有检查优先级的有效性
2007.07.19  加入 _DebugHandle() 功能
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock();
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_ThreadSetPriority
** 功能描述: 设置线程优先级。
** 输　入  : 
**           ulId            线程ID
**           ucPriority      优先级
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
#if LW_CFG_THREAD_CHANGE_PRIO_EN > 0

LW_API
ULONG  API_ThreadSetPriority (LW_OBJECT_HANDLE    ulId, UINT8    ucPriority)
{
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_TCB         ptcb;
	
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  检查 ID 类型有效性          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if (_PriorityCheck(ucPriority)) {                                   /*  优先级错误                  */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread priority invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_PRIORITY_WRONG);
        return  (ERROR_THREAD_PRIORITY_WRONG);
    }
    
    if (_Thread_Index_Invalid(usIndex)) {                               /*  检查线程有效性              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
#endif

    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    
    if (LW_PRIO_IS_EQU(ptcb->TCB_ucPriority, ucPriority)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        return  (ERROR_NONE);
    }
    
    _SchedSetPrio(ptcb, ucPriority);
    
    __KERNEL_EXIT();                                                    /*  退出内核                    */

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_THREAD_CHANGE_PRIO_EN*/
/*********************************************************************************************************
  END
*********************************************************************************************************/
