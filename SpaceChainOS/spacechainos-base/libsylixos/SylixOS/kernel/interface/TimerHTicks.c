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
** 文   件   名: TimerHTicks.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 01 月 11 日
**
** 描        述: 这是高速定时器周期中断服务函数

** BUG
2007.11.13  使用链表库对链表操作进行完全封装.
2013.12.04  调用回调时, 需要打开内核锁.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_TimerHTicks
** 功能描述: 这是高速定时器周期中断服务函数
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if	(LW_CFG_HTIMER_EN > 0) && (LW_CFG_MAX_TIMERS > 0)

LW_API
VOID  API_TimerHTicks (VOID)
{
             INTREG                     iregInterLevel;
    REGISTER PLW_CLASS_TIMER            ptmr;
             PLW_CLASS_WAKEUP_NODE      pwun;
             PTIMER_CALLBACK_ROUTINE    pfuncRoutine;
             PVOID                      pvRoutineArg;
             ULONG                      ulCounter = 1;
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  进入内核同时关闭中断        */
    
    __WAKEUP_PASS_FIRST(&_K_wuHTmr, pwun, ulCounter);
    
    ptmr = _LIST_ENTRY(pwun, LW_CLASS_TIMER, TIMER_wunTimer);
    
    _WakeupDel(&_K_wuHTmr, pwun);
    
    if (ptmr->TIMER_ulOption & LW_OPTION_AUTO_RESTART) {
        ptmr->TIMER_ulCounter = ptmr->TIMER_ulCounterSave;
        _WakeupAdd(&_K_wuHTmr, pwun);
        
    } else {
        ptmr->TIMER_ucStatus = LW_TIMER_STATUS_STOP;                    /*  填写停止标志位              */
    }
    
    pfuncRoutine = ptmr->TIMER_cbRoutine;
    pvRoutineArg = ptmr->TIMER_pvArg;
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  退出内核同时打开中断        */
    
    LW_SOFUNC_PREPARE(pfuncRoutine);
    pfuncRoutine(pvRoutineArg);
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  进入内核同时关闭中断        */
    
    __WAKEUP_PASS_SECOND();
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  这里允许响应中断            */
    
    iregInterLevel = KN_INT_DISABLE();
    
    __WAKEUP_PASS_END();
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  退出内核同时打开中断        */
}

#endif                                                                  /*  (LW_CFG_HTIMER_EN > 0)      */
                                                                        /*  (LW_CFG_MAX_TIMERS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
