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
** 文   件   名: TimerDelete.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 01 月 15 日
**
** 描        述: 删除一个定时器

** BUG
2007.07.21  加入 _DebugHandle() 功能。
2007.11.18  ucType 变量的处理时序出现错误.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_TimerDelete
** 功能描述: 删除一个定时器
** 输　入  : 
**           pulId      定时器句柄
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
#if	((LW_CFG_HTIMER_EN > 0) || (LW_CFG_ITIMER_EN > 0)) && (LW_CFG_MAX_TIMERS > 0)

LW_API  
ULONG  API_TimerDelete (LW_OBJECT_HANDLE  *pulId)
{
             INTREG                    iregInterLevel;
    REGISTER UINT16                    usIndex;
    REGISTER PLW_CLASS_TIMER           ptmr;
    
    REGISTER LW_OBJECT_HANDLE          ulId;
    
    ulId = *pulId;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
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
    
    _ObjectCloseId(pulId);
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "timer \"%s\" has been delete.\r\n", ptmr->TIMER_cTmrName);
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */
    
    if (ptmr->TIMER_ucStatus == LW_TIMER_STATUS_STOP) {                 /*  关闭状态下删除              */
        goto    __delete;
    }
    
    ptmr->TIMER_ucStatus = LW_TIMER_STATUS_STOP;
    
    if (ptmr->TIMER_ucType == LW_TYPE_TIMER_ITIMER) {                   /*  从扫描队列删除              */
        _WakeupDel(&_K_wuITmr, &ptmr->TIMER_wunTimer);
    
    } else {
        _WakeupDel(&_K_wuHTmr, &ptmr->TIMER_wunTimer);
    }
    
    ptmr->TIMER_ulCounter = 0ul;                                        /*  清除计数器                  */

__delete:
    ptmr->TIMER_ucType = LW_TYPE_TIMER_UNUSED;                          /*  删除标志                    */
    
    KN_INT_ENABLE(iregInterLevel);
    
    _Free_Timer_Object(ptmr);
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
    __LW_OBJECT_DELETE_HOOK(ulId);
    
    MONITOR_EVT_LONG1(MONITOR_EVENT_ID_TIMER, MONITOR_EVENT_TIMER_DELETE, ulId, LW_NULL);
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  ((LW_CFG_HTIMER_EN > 0)     */
                                                                        /*  (LW_CFG_ITIMER_EN > 0))     */
                                                                        /*  (LW_CFG_MAX_TIMERS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
