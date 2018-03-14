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
** 文   件   名: CoroutineResume.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 06 月 19 日
**
** 描        述: 这是协程管理库(协程是一个轻量级的并发执行单位). 
                 当前协程主动让出 CPU. 并调度指定的协程.
                 注意! 调用的协程一定是当前线程创建的协程.
                 
** BUG:
2010.08.03  添加注释.
2013.07.18  使用新的获取 TCB 的方法, 确保 SMP 系统安全.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_COROUTINE_EN > 0
/*********************************************************************************************************
** 函数名称: API_CoroutineResume
** 功能描述: 当前协程主动让出 CPU. 并调度指定的协程.
** 输　入  : pvCrcb            需要调度的协程句柄
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_CoroutineResume (PVOID  pvCrcb)
{
             INTREG                 iregInterLevel;
             
             PLW_CLASS_CPU          pcpuCur;
             PLW_CLASS_TCB          ptcbCur;
    REGISTER PLW_CLASS_COROUTINE    pcrcbNow;
    REGISTER PLW_CLASS_COROUTINE    pcrcbNext = (PLW_CLASS_COROUTINE)pvCrcb;

    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }

    if (!pcrcbNext) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "coroutine handle invalidate.\r\n");
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    if (pcrcbNext->COROUTINE_ulThread != ptcbCur->TCB_ulId) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "coroutine handle invalidate.\r\n");
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    pcrcbNow = _LIST_ENTRY(ptcbCur->TCB_pringCoroutineHeader,
                           LW_CLASS_COROUTINE,
                           COROUTINE_ringRoutine);                      /*  当前协程                    */
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_COROUTINE, MONITOR_EVENT_COROUTINE_RESUME, 
                      ptcbCur->TCB_ulId, pcrcbNext, LW_NULL);
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */
    
    ptcbCur->TCB_pringCoroutineHeader = &pcrcbNext->COROUTINE_ringRoutine;
    
    pcpuCur = LW_CPU_GET_CUR();
    pcpuCur->CPU_pcrcbCur  = pcrcbNow;
    pcpuCur->CPU_pcrcbNext = pcrcbNext;
    archCrtCtxSwitch(LW_CPU_GET_CUR());                                 /*  协程切换                    */
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  打开中断                    */
    
    _CoroutineReclaim(ptcbCur);                                         /*  尝试回收已经删除的协程      */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_COROUTINE_EN > 0     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
