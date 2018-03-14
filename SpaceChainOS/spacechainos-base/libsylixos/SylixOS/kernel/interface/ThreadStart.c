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
** 文   件   名: ThreadStart.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 07 月 19 日
**
** 描        述: 这是系统运行一个经过初始化的线程。

** BUG
2007.07.19  加入 _DebugHandle() 功能
2008.03.30  使用新的就绪环操作.
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock();
2012.03.30  加入 API_ThreadStartEx() 可以在启动线程时, join 线程, (原子操作)
2013.08.28  加入内核事件监控器.
2013.09.17  使用 POSIX 规定的取消点动作.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  s_internal.h 中也有相关定义
*********************************************************************************************************/
#if LW_CFG_THREAD_CANCEL_EN > 0
#define __THREAD_CANCEL_POINT()         API_ThreadTestCancel()
#else
#define __THREAD_CANCEL_POINT()
#endif                                                                  /*  LW_CFG_THREAD_CANCEL_EN     */
/*********************************************************************************************************
** 函数名称: API_ThreadStartEx
** 功能描述: 启动线程
** 输　入  : ulId            线程ID
**           bJoin           是否合并线程
**           ppvRetValAddr   存放线程返回值的地址
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API
ULONG  API_ThreadStartEx (LW_OBJECT_HANDLE  ulId, BOOL  bJoin, PVOID  *ppvRetValAddr)
{
             INTREG                iregInterLevel;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_TCB         ptcb;
	REGISTER PLW_CLASS_PCB         ppcb;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }

#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
#endif

    __THREAD_CANCEL_POINT();                                            /*  测试取消点                  */

    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    ppcb = _GetPcb(ptcb);
    
    MONITOR_EVT_LONG1(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_START, ulId, LW_NULL);
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */
    if (ptcb->TCB_usStatus & LW_THREAD_STATUS_INIT) {                   /*  线程是否为初始化状态        */
        ptcb->TCB_usStatus &= (~LW_THREAD_STATUS_INIT);                 /*  清除标志位                  */
        
        if (__LW_THREAD_IS_READY(ptcb)) {                               /*  就绪                        */
            _DebugFormat(__LOGMESSAGE_LEVEL, "thread \"%s\" has been start.\r\n",
                         ptcb->TCB_cThreadName);
            ptcb->TCB_ucSchedActivate = LW_SCHED_ACT_OTHER;
            __ADD_TO_READY_RING(ptcb, ppcb);                            /*  加入到相对优先级就绪环      */

            KN_INT_ENABLE(iregInterLevel);                              /*  打开中断                    */
            
            if (bJoin) {
                _ThreadJoin(ptcb, ppvRetValAddr);                       /*  合并                        */
            }
            
            __KERNEL_EXIT();                                            /*  退出内核                    */
            return  (ERROR_NONE);

        } else {
            __KERNEL_EXIT_IRQ(iregInterLevel);                          /*  退出内核并打开中断          */
            _ErrorHandle(ERROR_THREAD_NOT_READY);
            return  (ERROR_THREAD_NOT_READY);
        }
    } else {
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核并打开中断          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread not has this opt.\r\n");
        _ErrorHandle(ERROR_THREAD_NOT_INIT);
        return  (ERROR_THREAD_NOT_INIT);
    }
}
/*********************************************************************************************************
** 函数名称: API_ThreadStart
** 功能描述: 启动线程
** 输　入  : ulId            线程ID
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API
ULONG  API_ThreadStart (LW_OBJECT_HANDLE    ulId)
{
    return  (API_ThreadStartEx(ulId, LW_FALSE, LW_NULL));
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
