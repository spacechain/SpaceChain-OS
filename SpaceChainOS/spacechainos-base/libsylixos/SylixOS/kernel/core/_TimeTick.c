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
** 文   件   名: _TimeTick.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 03 月 29 日
**
** 描        述: 这是系统 TICK 中断函数库.

** BUG
2008.03.30  使用新的就绪环操作.
2009.03.15  使 ppcb 的获取更为优化.
2009.11.20  WatchDogTimerHook 改为 OSTaskWatchDogTimerHook.
2010.01.22  使用内核模式进行保护. 以支持 SMP.
2016.07.21  超时后如果等待事件, 则直接从事件中退链.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: _WatchDogTick
** 功能描述: 扫描看门狗链表, tick 处理函数
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_SOFTWARE_WATCHDOG_EN > 0

VOID  _WatchDogTick (VOID)
{
             INTREG                 iregInterLevel;
    REGISTER PLW_CLASS_TCB          ptcb;
             PLW_CLASS_WAKEUP_NODE  pwun;
             ULONG                  ulCounter = 1;
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  进入内核同时关闭中断        */
    
    __WAKEUP_PASS_FIRST(&_K_wuWatchDog, pwun, ulCounter);
    
    ptcb = _LIST_ENTRY(pwun, LW_CLASS_TCB, TCB_wunWatchDog);
    
    __DEL_FROM_WATCHDOG_LINE(ptcb);                                     /*  从扫描链表中删除            */
    
    KN_INT_ENABLE(iregInterLevel);
    
    bspWdTimerHook(ptcb->TCB_ulId);
    __LW_WATCHDOG_TIMER_HOOK(ptcb->TCB_ulId);                           /*  执行回调函数                */
    
    iregInterLevel = KN_INT_DISABLE();
    
    __WAKEUP_PASS_SECOND();
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  这里允许响应中断            */
    
    iregInterLevel = KN_INT_DISABLE();
    
    __WAKEUP_PASS_END();
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  退出内核同时打开中断        */
}

#endif                                                                  /*  LW_CFG_SOFTWARE_WATCHDOG_EN */
/*********************************************************************************************************
** 函数名称: _ThreadTick
** 功能描述: 扫描等待唤醒链表, tick 处理函数
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _ThreadTick (VOID)
{
             INTREG                 iregInterLevel;
    REGISTER PLW_CLASS_TCB          ptcb;
    REGISTER PLW_CLASS_PCB          ppcb;
             PLW_CLASS_WAKEUP_NODE  pwun;
             ULONG                  ulCounter = 1;
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  进入内核同时关闭中断        */
    
    __WAKEUP_PASS_FIRST(&_K_wuDelay, pwun, ulCounter);
    
    ptcb = _LIST_ENTRY(pwun, LW_CLASS_TCB, TCB_wunDelay);
    
    __DEL_FROM_WAKEUP_LINE(ptcb);                                       /*  从等待链中删除              */
    
    if (ptcb->TCB_usStatus & LW_THREAD_STATUS_PEND_ANY) {               /*  检查是否在等待事件          */
        ptcb->TCB_usStatus &= (~LW_THREAD_STATUS_PEND_ANY);             /*  等待超时清除事件等待位      */
        ptcb->TCB_ucWaitTimeout = LW_WAIT_TIME_OUT;                     /*  等待超时                    */
    
#if (LW_CFG_EVENT_EN > 0) && (LW_CFG_MAX_EVENTS > 0)
        if (ptcb->TCB_peventPtr) {
            _EventUnQueue(ptcb);
        } else 
#endif                                                                  /*  (LW_CFG_EVENT_EN > 0) &&    */
        {
#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)
            if (ptcb->TCB_pesnPtr) {
                _EventSetUnQueue(ptcb->TCB_pesnPtr);
            }
#endif                                                                  /*  (LW_CFG_EVENTSET_EN > 0) && */
        }
    } else {
        ptcb->TCB_ucWaitTimeout = LW_WAIT_TIME_CLEAR;                   /*  没有等待事件                */
    }
    
    if (__LW_THREAD_IS_READY(ptcb)) {                                   /*  检查是否就绪                */
        ptcb->TCB_ucSchedActivate = LW_SCHED_ACT_OTHER;
        ppcb = _GetPcb(ptcb);                                           /*  获得优先级控制块            */
        __ADD_TO_READY_RING(ptcb, ppcb);                                /*  加入就绪环                  */
    }
    
    __WAKEUP_PASS_SECOND();
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  这里允许响应中断            */
    
    iregInterLevel = KN_INT_DISABLE();
    
    __WAKEUP_PASS_END();
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  退出内核同时打开中断        */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
