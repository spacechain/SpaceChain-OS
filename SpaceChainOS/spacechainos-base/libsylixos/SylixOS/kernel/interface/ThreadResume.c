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
** 文   件   名: ThreadResume.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 07 月 19 日
**
** 描        述: 这是系统线程恢复挂起函数。

** BUG
2007.07.19  加入 _DebugHandle() 功能
2007.09.26  去掉了就绪没有阻塞的任务时的错误打印.
2008.03.30  使用新的就绪环操作.
2009.05.26  当目标线程没有被阻塞时, 将 TCB_ulResumeNesting++;
            线程再删除过程中, 不允许调用此函数激活线程.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_ThreadResume
** 功能描述: 恢复挂起线程
** 输　入  : ulId            线程ID
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if LW_CFG_THREAD_SUSPEND_EN > 0

LW_API
ULONG  API_ThreadResume (LW_OBJECT_HANDLE    ulId)
{
             INTREG                iregInterLevel;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_TCB         ptcb;
	REGISTER PLW_CLASS_PCB         ppcb;
	
    usIndex = _ObjectGetIndex(ulId);
	
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  检查 ID 类型有效性          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invaliate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if (_Thread_Index_Invalid(usIndex)) {                               /*  检查线程有效性              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invaliate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
#endif

    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invaliate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    ppcb = _GetPcb(ptcb);
    
    if (ptcb->TCB_iDeleteProcStatus) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread has been deleted.\r\n");
        _ErrorHandle(ERROR_THREAD_OTHER_DELETE);
        return  (ERROR_THREAD_OTHER_DELETE);
    }
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_RESUME,
                      ulId, ptcb->TCB_ulSuspendNesting - 1, LW_NULL);
                      
    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */
    
    if (ptcb->TCB_ulSuspendNesting) {
        ptcb->TCB_ulSuspendNesting--;
    
    } else {
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核并打开中断          */
        return  (ERROR_NONE);
    }
    
    if (!ptcb->TCB_ulSuspendNesting) {                                  /*  最后一层解除挂起            */
        ptcb->TCB_usStatus &= (~LW_THREAD_STATUS_SUSPEND);
        if (__LW_THREAD_IS_READY(ptcb)) {                               /*  就绪 ?                      */
            ptcb->TCB_ucSchedActivate = LW_SCHED_ACT_INTERRUPT;         /*  中断激活方式                */
            __ADD_TO_READY_RING(ptcb, ppcb);                            /*  加入到相对优先级就绪环      */
        }
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核并打开中断          */
        return  (ERROR_NONE);
        
    } else {                                                            /*  有嵌套                      */
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核并打开中断          */
        return  (ERROR_NONE);
    }
}

#endif                                                                  /*  LW_CFG_THREAD_SUSPEND_EN    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
