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
** 文   件   名: CoroutineDelete.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 06 月 19 日
**
** 描        述: 这是协程管理库(协程是一个轻量级的并发执行单位). 
                 在当前线程中删除一个协程. (不能删除其他线程中的协程, 否这会引起系统出现严重问题)
** BUG:
2013.07.18  使用新的获取 TCB 的方法, 确保 SMP 系统安全.
2013.12.14  使用任务自旋锁确保任务协程链表操作安全性.
2016.12.27  使用协程延迟删除方法, 确保堆栈安全.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_COROUTINE_EN > 0
/*********************************************************************************************************
** 函数名称: API_CoroutineExit
** 功能描述: 在当前线程正在执行的协程删除
** 输　入  : NONE
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG   API_CoroutineExit (VOID)
{
             INTREG                 iregInterLevel;
             
             PLW_CLASS_CPU          pcpuCur;
             PLW_CLASS_TCB          ptcbCur;
    REGISTER PLW_CLASS_COROUTINE    pcrcbExit;
    REGISTER PLW_CLASS_COROUTINE    pcrcbNext;
    REGISTER PLW_LIST_RING          pringNext;
    
    if (!LW_SYS_STATUS_IS_RUNNING()) {                                  /*  系统必须已经启动            */
        _ErrorHandle(ERROR_KERNEL_NOT_RUNNING);
        return  (ERROR_KERNEL_NOT_RUNNING);
    }
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    pcrcbExit = _LIST_ENTRY(ptcbCur->TCB_pringCoroutineHeader, 
                            LW_CLASS_COROUTINE, 
                            COROUTINE_ringRoutine);                     /*  获得当前协程                */
    
    if (&pcrcbExit->COROUTINE_ringRoutine == 
        _list_ring_get_next(&pcrcbExit->COROUTINE_ringRoutine)) {       /*  仅有这一个协程              */
#if LW_CFG_THREAD_DEL_EN > 0
        API_ThreadExit(LW_NULL);
#endif                                                                  /*  LW_CFG_THREAD_DEL_EN > 0    */
        return  (ERROR_NONE);
    }
    
    pringNext = _list_ring_get_next(&pcrcbExit->COROUTINE_ringRoutine);

    pcrcbNext = _LIST_ENTRY(pringNext, LW_CLASS_COROUTINE, 
                            COROUTINE_ringRoutine);                     /*  获得下一个协程              */

    pcrcbExit->COROUTINE_ulFlags |= LW_COROUTINE_FLAG_DELETE;           /*  当前协程请求被删除          */
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_COROUTINE, MONITOR_EVENT_COROUTINE_DELETE, 
                      ptcbCur->TCB_ulId, pcrcbExit, LW_NULL);
                      
    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */
    
    ptcbCur->TCB_pringCoroutineHeader = pringNext;                      /*  选择下一个协程              */
    
    pcpuCur                = LW_CPU_GET_CUR();
    pcpuCur->CPU_pcrcbCur  = pcrcbExit;
    pcpuCur->CPU_pcrcbNext = pcrcbNext;
    archCrtCtxSwitch(LW_CPU_GET_CUR());                                 /*  协程切换                    */
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  打开中断                    */

    return  (ERROR_NONE);                                               /*  理论上是无法运行到这里的    */
}
/*********************************************************************************************************
** 函数名称: API_CoroutineDelete
** 功能描述: 删除一个指定的协程.
** 输　入  : pvCrcb                        协程句柄
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG   API_CoroutineDelete (PVOID  pvCrcb)
{
             INTREG                 iregInterLevel;

    REGISTER PLW_CLASS_COROUTINE    pcrcbDel = (PLW_CLASS_COROUTINE)pvCrcb;
    REGISTER PLW_CLASS_COROUTINE    pcrcbNow;
             PLW_CLASS_TCB          ptcbCur;

    if (!LW_SYS_STATUS_IS_RUNNING()) {                                  /*  系统必须已经启动            */
        _ErrorHandle(ERROR_KERNEL_NOT_RUNNING);
        return  (ERROR_KERNEL_NOT_RUNNING);
    }
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
    if (!pcrcbDel) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "coroutine handle invalidate.\r\n");
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  当前任务控制块              */
    
    if (pcrcbDel->COROUTINE_ulThread != ptcbCur->TCB_ulId) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "coroutine handle invalidate.\r\n");
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    pcrcbNow = _LIST_ENTRY(ptcbCur->TCB_pringCoroutineHeader, 
                           LW_CLASS_COROUTINE, 
                           COROUTINE_ringRoutine);                      /*  获得当前协程                */
    
    if (pcrcbNow == pcrcbDel) {                                         /*  删除当前协程                */
        return  (API_CoroutineExit());
    }
    
    _ThreadSafeInternal();                                              /*  进入安全模式                */

    LW_SPIN_LOCK_QUICK(&ptcbCur->TCB_slLock, &iregInterLevel);
    _List_Ring_Del(&pcrcbDel->COROUTINE_ringRoutine,
                   &ptcbCur->TCB_pringCoroutineHeader);                 /*  从协程表中删除              */
    LW_SPIN_UNLOCK_QUICK(&ptcbCur->TCB_slLock, iregInterLevel);
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_COROUTINE, MONITOR_EVENT_COROUTINE_DELETE, 
                      ptcbCur->TCB_ulId, pcrcbDel, LW_NULL);
    
    if (pcrcbDel->COROUTINE_ulFlags & LW_COROUTINE_FLAG_DYNSTK) {
        _StackFree(ptcbCur, pcrcbDel->COROUTINE_pstkStackLowAddr);      /*  释放内存                    */
    }
    
    _ThreadUnsafeInternal();                                            /*  退出安全模式                */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_COROUTINE_EN > 0     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
