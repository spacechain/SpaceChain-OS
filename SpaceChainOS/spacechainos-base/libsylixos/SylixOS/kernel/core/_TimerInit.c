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
** 文   件   名: _TimerInit.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 01 月 10 日
**
** 描        述: 定时器初始化函数库。
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: _TimerInit
** 功能描述: 定时器初始化
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _TimerInit (VOID)
{
#if	((LW_CFG_HTIMER_EN > 0) || (LW_CFG_ITIMER_EN > 0)) && (LW_CFG_MAX_TIMERS > 0)
    
#if  LW_CFG_MAX_TIMERS == 1

    REGISTER PLW_CLASS_TIMER        ptmrTemp1;
    
    _K_resrcTmr.RESRC_pmonoFreeHeader = &_K_tmrBuffer[0].TIMER_monoResrcList;
    
    ptmrTemp1 = &_K_tmrBuffer[0];
    
    ptmrTemp1->TIMER_ucType  = LW_TYPE_TIMER_UNUSED;
    ptmrTemp1->TIMER_usIndex = 0;
    LW_SPIN_INIT(&ptmrTemp1->TIMER_slLock);

    _INIT_LIST_MONO_HEAD(_K_resrcTmr.RESRC_pmonoFreeHeader);
    
    _K_resrcTmr.RESRC_pmonoFreeTail = _K_resrcTmr.RESRC_pmonoFreeHeader;
#else

    REGISTER ULONG                  ulI;
    REGISTER PLW_LIST_MONO          pmonoTemp1;
    REGISTER PLW_LIST_MONO          pmonoTemp2;
    REGISTER PLW_CLASS_TIMER        ptmrTemp1;
    REGISTER PLW_CLASS_TIMER        ptmrTemp2;
    
    _K_resrcTmr.RESRC_pmonoFreeHeader = &_K_tmrBuffer[0].TIMER_monoResrcList;
    
    ptmrTemp1 = &_K_tmrBuffer[0];                                       /*  指向缓冲池首地址            */
    ptmrTemp2 = &_K_tmrBuffer[1];
    
    for (ulI = 0; ulI < (LW_CFG_MAX_TIMERS - 1); ulI++) {
        ptmrTemp1->TIMER_ucType  = LW_TYPE_TIMER_UNUSED;
        ptmrTemp1->TIMER_usIndex = (UINT16)ulI;
        
        pmonoTemp1 = &ptmrTemp1->TIMER_monoResrcList;
        pmonoTemp2 = &ptmrTemp2->TIMER_monoResrcList;
        LW_SPIN_INIT(&ptmrTemp1->TIMER_slLock);
        
        _LIST_MONO_LINK(pmonoTemp1, pmonoTemp2);
        
        ptmrTemp1++;
        ptmrTemp2++;
    }
    
    ptmrTemp1->TIMER_ucType  = LW_TYPE_TIMER_UNUSED;
    ptmrTemp1->TIMER_usIndex = (UINT16)ulI;
    LW_SPIN_INIT(&ptmrTemp1->TIMER_slLock);
    
    pmonoTemp1 = &ptmrTemp1->TIMER_monoResrcList;
    
    _INIT_LIST_MONO_HEAD(pmonoTemp1);
    
    _K_resrcTmr.RESRC_pmonoFreeTail = pmonoTemp1;
#endif                                                                  /*  LW_CFG_MAX_TIMERS == 1      */

    _K_resrcTmr.RESRC_uiUsed    = 0;
    _K_resrcTmr.RESRC_uiMaxUsed = 0;

#if (LW_CFG_HTIMER_EN > 0)
    __WAKEUP_INIT(&_K_wuHTmr);
#endif

#if (LW_CFG_ITIMER_EN > 0)
    __WAKEUP_INIT(&_K_wuITmr);
#endif

#endif                                                                  /*  ((LW_CFG_HTIMER_EN > 0)     */
                                                                        /*  (LW_CFG_ITIMER_EN > 0))     */
                                                                        /*  (LW_CFG_MAX_TIMERS > 0)     */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
