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
** 文   件   名: _ITimerThread.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 18 日
**
** 描        述: 这是系统普通定时器周期服务线程。

** BUG
2007.05.11  删除了错误的注释信息
2007.11.13  使用链表库对链表操作进行完全封装.
2008.10.17  改使用精度单调调度进行延迟.
2011.11.29  调用回调时, 不能锁定内核.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: _ITimerThread
** 功能描述: 这是系统普通定时器周期服务线程。
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if	(LW_CFG_ITIMER_EN > 0) && (LW_CFG_MAX_TIMERS > 0)

PVOID  _ITimerThread (PVOID  pvArg)
{
             INTREG                     iregInterLevel;
    REGISTER PLW_CLASS_TIMER            ptmr;
             PTIMER_CALLBACK_ROUTINE    pfuncRoutine;
             PVOID                      pvRoutineArg;
    
#if (LW_CFG_RMS_EN > 0) && (LW_CFG_MAX_RMSS > 0)
             LW_OBJECT_HANDLE           ulRms = API_RmsCreate("rms_timer", 
                                                               LW_OPTION_OBJECT_GLOBAL, LW_NULL);
#endif
    
    (VOID)pvArg;
    
    for (;;) {
        PLW_CLASS_WAKEUP_NODE  pwun;
        ULONG                  ulCounter = LW_ITIMER_RATE;
        
#if (LW_CFG_RMS_EN > 0) && (LW_CFG_MAX_RMSS > 0)
        API_RmsPeriod(ulRms, LW_ITIMER_RATE);                           /*  使用 RMS 进行周期运行       */
#else
        API_TimeSleep(LW_ITIMER_RATE);                                  /*  等待一个扫描周期            */
#endif

        iregInterLevel = __KERNEL_ENTER_IRQ();                          /*  进入内核同时关闭中断        */
        
        __WAKEUP_PASS_FIRST(&_K_wuITmr, pwun, ulCounter);
        
        ptmr = _LIST_ENTRY(pwun, LW_CLASS_TIMER, TIMER_wunTimer);
        
        _WakeupDel(&_K_wuITmr, pwun);
        
        if (ptmr->TIMER_ulOption & LW_OPTION_AUTO_RESTART) {
            ptmr->TIMER_ulCounter = ptmr->TIMER_ulCounterSave;
            _WakeupAdd(&_K_wuITmr, pwun);
            
        } else {
            ptmr->TIMER_ucStatus = LW_TIMER_STATUS_STOP;                /*  填写停止标志位              */
        }
        
        pfuncRoutine = ptmr->TIMER_cbRoutine;
        pvRoutineArg = ptmr->TIMER_pvArg;
        
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核同时打开中断        */
        
        LW_SOFUNC_PREPARE(pfuncRoutine);
        pfuncRoutine(pvRoutineArg);
        
        iregInterLevel = __KERNEL_ENTER_IRQ();                          /*  进入内核同时关闭中断        */
        
        __WAKEUP_PASS_SECOND();
        
        KN_INT_ENABLE(iregInterLevel);                                  /*  这里允许响应中断            */
    
        iregInterLevel = KN_INT_DISABLE();
        
        __WAKEUP_PASS_END();
        
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核同时打开中断        */
    }
    
    return  (LW_NULL);
}

#endif                                                                  /*  (LW_CFG_ITIMER_EN > 0)      */
                                                                        /*  (LW_CFG_MAX_TIMERS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
