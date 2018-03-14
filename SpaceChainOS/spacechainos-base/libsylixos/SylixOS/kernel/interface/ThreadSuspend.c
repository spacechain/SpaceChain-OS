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
** 文   件   名: ThreadSuspend.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 07 月 19 日
**
** 描        述: 这是系统线程挂起函数。

** BUG
2007.07.19  加入 _DebugHandle() 功能
2007.10.28  不能在调度器锁定的情况下锁定自己.
2008.01.20  调度器已经有了重大的改进, 可以在调度器被锁定的情况下调用此 API.
2008.03.30  使用新的就绪环操作.
2010.01.22  支持 SMP.
2013.12.03  使用 _ThreadStatusChange() 做阻塞处理.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_ThreadSuspend
** 功能描述: 挂起线程
** 输　入  : ulId            线程ID
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if LW_CFG_THREAD_SUSPEND_EN > 0

LW_API
ULONG  API_ThreadSuspend (LW_OBJECT_HANDLE    ulId)
{
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_TCB         ptcb;
	         ULONG                 ulError;
	
    usIndex = _ObjectGetIndex(ulId);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  检查 ID 类型有效性          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
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
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_SUSPEND, 
                      ulId, ptcb->TCB_ulSuspendNesting + 1, LW_NULL);
    
    ulError = _ThreadStatusChange(ptcb, LW_TCB_REQ_SUSPEND);
    
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
    _ErrorHandle(ulError);
    return  (ulError);
}

#endif                                                                  /*  LW_CFG_THREAD_SUSPEND_EN    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
