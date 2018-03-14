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
** 文   件   名: CoroutineYield.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 06 月 19 日
**
** 描        述: 这是协程管理库(协程是一个轻量级的并发执行单位). 
                 当前协程主动让出 CPU. 协程调度.
                 
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
** 函数名称: API_CoroutineYield
** 功能描述: 当前协程主动让出 CPU. 协程调度.
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_CoroutineYield (VOID)
{
             INTREG                 iregInterLevel;
             
             PLW_CLASS_CPU          pcpuCur;
             PLW_CLASS_TCB          ptcbCur;
    REGISTER PLW_CLASS_COROUTINE    pcrcbNow;
    REGISTER PLW_CLASS_COROUTINE    pcrcbNext;
    REGISTER PLW_LIST_RING          pringNext;
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return;
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    pringNext = _list_ring_get_next(ptcbCur->TCB_pringCoroutineHeader);
    
    if (ptcbCur->TCB_pringCoroutineHeader == pringNext) {               /*  仅此一个协程                */
        return;
    }
    
    pcrcbNow = _LIST_ENTRY(ptcbCur->TCB_pringCoroutineHeader,
                           LW_CLASS_COROUTINE,
                           COROUTINE_ringRoutine);                      /*  当前协程                    */
    
    pcrcbNext = _LIST_ENTRY(pringNext,
                            LW_CLASS_COROUTINE,
                            COROUTINE_ringRoutine);                     /*  下一个协程                  */
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_COROUTINE, MONITOR_EVENT_COROUTINE_YIELD, 
                      ptcbCur->TCB_ulId, pcrcbNext, LW_NULL);
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */
    
    ptcbCur->TCB_pringCoroutineHeader = pringNext;
    
    pcpuCur                = LW_CPU_GET_CUR();
    pcpuCur->CPU_pcrcbCur  = pcrcbNow;
    pcpuCur->CPU_pcrcbNext = pcrcbNext;
    archCrtCtxSwitch(LW_CPU_GET_CUR());                                 /*  协程切换                    */
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  打开中断                    */
    
    _CoroutineReclaim(ptcbCur);                                         /*  尝试回收已经删除的协程      */
}

#endif                                                                  /*  LW_CFG_COROUTINE_EN > 0     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
