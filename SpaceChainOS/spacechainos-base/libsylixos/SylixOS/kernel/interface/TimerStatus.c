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
** 文   件   名: TimerStatus.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 01 月 15 日
**
** 描        述: 获得定时器相关状态

** BUG
2007.10.20  加入 _DebugHandle() 功能.
2015.04.07  加入 API_TimerStatusEx() 扩展接口.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_TimerStatusEx
** 功能描述: 获得定时器相关状态扩展接口, 仅提供 POSIX 接口内部使用
** 输　入  : 
**           ulId                        定时器句柄
**           pbTimerRunning              定时器是否在运行
**           pulOption                   定时器选项
**           pulCounter                  定时器当前计数值
**           pulInterval                 间隔时间, 为 0 表示单次运行
**           pclockid                    POSIX 时间类型
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if	((LW_CFG_HTIMER_EN > 0) || (LW_CFG_ITIMER_EN > 0)) && (LW_CFG_MAX_TIMERS > 0)

LW_API  
ULONG  API_TimerStatusEx (LW_OBJECT_HANDLE          ulId,
                          BOOL                     *pbTimerRunning,
                          ULONG                    *pulOption,
                          ULONG                    *pulCounter,
                          ULONG                    *pulInterval,
                          clockid_t                *pclockid)
{
             INTREG                    iregInterLevel;
    REGISTER UINT16                    usIndex;
    REGISTER PLW_CLASS_TIMER           ptmr;
    
    usIndex = _ObjectGetIndex(ulId);
    
#if LW_CFG_ARG_CHK_EN > 0
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
    
    if (pbTimerRunning) {
        if (ptmr->TIMER_ucStatus == LW_TIMER_STATUS_RUNNING) {
            *pbTimerRunning = LW_TRUE;
        } else {
            *pbTimerRunning = LW_FALSE;
        }
    }
    
    if (pulOption) {
        *pulOption = ptmr->TIMER_ulOption;
    }
    
    if (pulCounter) {
        if (ptmr->TIMER_ucStatus == LW_TIMER_STATUS_RUNNING) {
            if (ptmr->TIMER_ucType == LW_TYPE_TIMER_ITIMER) {
                _WakeupStatus(&_K_wuITmr, &ptmr->TIMER_wunTimer, pulCounter);
            
            } else {
                _WakeupStatus(&_K_wuHTmr, &ptmr->TIMER_wunTimer, pulCounter);
            }
        } else {
            *pulCounter = 0ul;
        }
    }
    
    if (pulInterval) {
        *pulInterval = ptmr->TIMER_ulCounterSave;
    }
    
    if (pclockid) {
        *pclockid = ptmr->TIMER_clockid;
    }
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  退出内核并打开中断          */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_TimerStatus
** 功能描述: 获得定时器相关状态
** 输　入  : 
**           ulId                        定时器句柄
**           pbTimerRunning              定时器是否在运行
**           pulOption                   定时器选项
**           pulCounter                  定时器当前计数值
**           pulInterval                 间隔时间, 为 0 表示单次运行
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_TimerStatus (LW_OBJECT_HANDLE          ulId,
                        BOOL                     *pbTimerRunning,
                        ULONG                    *pulOption,
                        ULONG                    *pulCounter,
                        ULONG                    *pulInterval)
{
    return  (API_TimerStatusEx(ulId, pbTimerRunning, pulOption,
                               pulCounter, pulInterval, LW_NULL));
}

#endif                                                                  /*  ((LW_CFG_HTIMER_EN > 0)     */
                                                                        /*  (LW_CFG_ITIMER_EN > 0))     */
                                                                        /*  (LW_CFG_MAX_TIMERS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
