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
** 文   件   名: TimerCreate.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 01 月 11 日
**
** 描        述: 建立一个定时器

** BUG
2007.10.20  加入 _DebugHandle() 功能。
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock();
2008.05.31  使用 __KERNEL_MODE_...().
2008.06.09  初始化为非 posix 定时器.
2008.12.15  更新 _INIT_LIST_LINE_HEAD 为 _LIST_LINE_INIT_IN_CODE 与其他文件统一.
2011.02.26  使用 TIMER_u64Overrun 记录 overrun 个数.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_TimerCreate
** 功能描述: 建立一个定时器
** 输　入  : 
**           pcName                        名字
**           ulOption                      定时器类型：高速、普通
**           pulId                         Id 号
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
#if	((LW_CFG_HTIMER_EN > 0) || (LW_CFG_ITIMER_EN > 0)) && (LW_CFG_MAX_TIMERS > 0)

LW_API  
LW_OBJECT_HANDLE  API_TimerCreate (CPCHAR             pcName,
                                   ULONG              ulOption,
                                   LW_OBJECT_ID      *pulId)
{
    REGISTER PLW_CLASS_TIMER       ptmr;
    REGISTER ULONG                 ulIdTemp;
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    if (_Object_Name_Invalid(pcName)) {                                 /*  检查名字有效性              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name too long.\r\n");
        _ErrorHandle(ERROR_KERNEL_PNAME_TOO_LONG);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    __KERNEL_MODE_PROC(
        ptmr = _Allocate_Timer_Object();                                /*  获得一个定时器控制块        */
    );
    
    if (!ptmr) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "there is no ID to buide a timer.\r\n");
        _ErrorHandle(ERROR_TIMER_FULL);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    if (pcName) {                                                       /*  拷贝名字                    */
        lib_strcpy(ptmr->TIMER_cTmrName, pcName);
    } else {
        ptmr->TIMER_cTmrName[0] = PX_EOS;                               /*  清空名字                    */
    }
    
    if (ulOption & LW_OPTION_ITIMER) {                                  /*  应用级定时器                */
        ptmr->TIMER_ucType = LW_TYPE_TIMER_ITIMER;                      /*  定时器类型                  */
    } else {
        ptmr->TIMER_ucType = LW_TYPE_TIMER_HTIMER;
    }
    
    __WAKEUP_NODE_INIT(&ptmr->TIMER_wunTimer);

    ptmr->TIMER_ulCounterSave = 0ul;                                    /*  定时器计数值保留值          */
    ptmr->TIMER_ulOption      = 0ul;                                    /*  定时器操作选项              */
    ptmr->TIMER_ucStatus      = LW_TIMER_STATUS_STOP;                   /*  定时器状态                  */
    ptmr->TIMER_cbRoutine     = LW_NULL;                                /*  执行函数                    */
    ptmr->TIMER_pvArg         = LW_NULL;
    
    ptmr->TIMER_ulThreadId    = 0ul;                                    /*  没有线程拥有                */
    ptmr->TIMER_u64Overrun    = 0ull;
    ptmr->TIMER_clockid       = CLOCK_REALTIME;

#if LW_CFG_TIMERFD_EN > 0
    ptmr->TIMER_pvTimerfd     = LW_NULL;
#endif                                                                  /*  LW_CFG_TIMERFD_EN > 0       */

    ulIdTemp = _MakeObjectId(_OBJECT_TIMER, 
                             LW_CFG_PROCESSOR_NUMBER, 
                             ptmr->TIMER_usIndex);                      /*  构建对象 id                 */
    
    if (pulId) {
        *pulId = ulIdTemp;
    }

#if LW_CFG_PTIMER_AUTO_DEL_EN > 0
    ptmr->TIMER_ulTimer = ulIdTemp;
#endif                                                                  /*  LW_CFG_PTIMER_AUTO_DEL_EN   */
    
    __LW_OBJECT_CREATE_HOOK(ulIdTemp, ulOption);
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_TIMER, MONITOR_EVENT_TIMER_CREATE, 
                      ulIdTemp, ulOption, pcName);
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "timer \"%s\" has been create.\r\n", (pcName ? pcName : ""));
    
    return  (ulIdTemp);
}

#endif                                                                  /*  ((LW_CFG_HTIMER_EN > 0)     */
                                                                        /*  (LW_CFG_ITIMER_EN > 0))     */
                                                                        /*  (LW_CFG_MAX_TIMERS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
