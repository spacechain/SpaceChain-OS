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
** 文   件   名: TimerStart.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 01 月 11 日
**
** 描        述: 启动一个定时器

** BUG
2007.10.20  加入 _DebugHandle() 功能。
2008.12.17  今天很多电视都播出了改革开放 30 年文艺晚会, 深深的感受到了祖国日益强大, 感谢老一辈革命家给了
            我们一个大显拳脚的机会, 身为 80 后的我们应该肩负起使命, 为了中华之崛起!
            调整 API_TimerStart 参数结构.
2013.12.04  如果定时器还在运行, 则需要首先复位定时器.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_TimerStart
** 功能描述: 启动一个定时器
** 输　入  : ulId                        定时器句柄
**           ulCounter                   计数初始值
**           ulOption                    操作选项
**           cbTimerRoutine              回调函数
**           pvArg                       参数
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if	((LW_CFG_HTIMER_EN > 0) || (LW_CFG_ITIMER_EN > 0)) && (LW_CFG_MAX_TIMERS > 0)

LW_API  
ULONG  API_TimerStart (LW_OBJECT_HANDLE         ulId,
                       ULONG                    ulCounter,
                       ULONG                    ulOption,
                       PTIMER_CALLBACK_ROUTINE  cbTimerRoutine,
                       PVOID                    pvArg)
{
    return  (API_TimerStartEx(ulId, ulCounter, ulCounter, ulOption, cbTimerRoutine, pvArg));
}
/*********************************************************************************************************
** 函数名称: API_TimerStartEx
** 功能描述: 启动一个定时器扩展接口
** 输　入  : ulId                        定时器句柄
**           ulInitCounter               计数初始值
**           ulCounter                   重复计数初始值
**           ulOption                    操作选项
**           cbTimerRoutine              回调函数
**           pvArg                       参数
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_TimerStartEx (LW_OBJECT_HANDLE         ulId,
                         ULONG                    ulInitCounter,
                         ULONG                    ulCounter,
                         ULONG                    ulOption,
                         PTIMER_CALLBACK_ROUTINE  cbTimerRoutine,
                         PVOID                    pvArg)
{
             INTREG                    iregInterLevel;
    REGISTER UINT16                    usIndex;
    REGISTER PLW_CLASS_TIMER           ptmr;
    
    usIndex = _ObjectGetIndex(ulId);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!cbTimerRoutine) {                                              /*  回调检查                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "cbTimerRoutine invalidate.\r\n");
        _ErrorHandle(ERROR_TIMER_CALLBACK_NULL);
        return  (ERROR_TIMER_CALLBACK_NULL);
    }
    
    if (!ulInitCounter) {                                               /*  定时时间检查                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "ulInitCounter invalidate.\r\n");
        _ErrorHandle(ERROR_TIMER_TIME);
        return  (ERROR_TIMER_TIME);
    }
    
    if ((ulOption & LW_OPTION_AUTO_RESTART) && !ulCounter) {            /*  定时时间检查                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "ulCounter invalidate.\r\n");
        _ErrorHandle(ERROR_TIMER_TIME);
        return  (ERROR_TIMER_TIME);
    }
    
    if (!_ObjectClassOK(ulId, _OBJECT_TIMER)) {                         /*  对象类型检查                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "timer handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if (_Timer_Index_Invalid(usIndex)) {                                /*  缓冲区索引检查              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "timer handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
#endif

    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (_Timer_Type_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "timer handle invalidate.\r\n");
        _ErrorHandle(ERROR_TIMER_NULL);
        return  (ERROR_TIMER_NULL);
    }

    ptmr = &_K_tmrBuffer[usIndex];
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */
    
    if (ptmr->TIMER_ucStatus == LW_TIMER_STATUS_RUNNING) {              /*  正在工作                    */
        if (ptmr->TIMER_ucType == LW_TYPE_TIMER_ITIMER) {               /*  从扫描队列删除              */
            _WakeupDel(&_K_wuITmr, &ptmr->TIMER_wunTimer);
    
        } else {
            _WakeupDel(&_K_wuHTmr, &ptmr->TIMER_wunTimer);
        }
    }
    
    ptmr->TIMER_ulCounter     = ulInitCounter;
    ptmr->TIMER_ulCounterSave = ulCounter;
    ptmr->TIMER_ulOption      = ulOption;
    ptmr->TIMER_ucStatus      = LW_TIMER_STATUS_RUNNING;                /*  定时器开始运行              */
    ptmr->TIMER_cbRoutine     = cbTimerRoutine;
    ptmr->TIMER_pvArg         = pvArg;
    ptmr->TIMER_u64Overrun    = 0ull;
    
    if (ptmr->TIMER_ucType == LW_TYPE_TIMER_ITIMER) {                   /*  加入扫描队列                */
        _WakeupAdd(&_K_wuITmr, &ptmr->TIMER_wunTimer);
    
    } else {
        _WakeupAdd(&_K_wuHTmr, &ptmr->TIMER_wunTimer);
    }
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  退出内核并打开中断          */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  ((LW_CFG_HTIMER_EN > 0)     */
                                                                        /*  (LW_CFG_ITIMER_EN > 0))     */
                                                                        /*  (LW_CFG_MAX_TIMERS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
