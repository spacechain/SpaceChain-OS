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
** 文   件   名: listTimer.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 13 日
**
** 描        述: 这是系统定时器资源链表操作。
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: _Allocate_Timer_Object
** 功能描述: 从空闲 Timer 控件池中取出一个空闲 Timer
** 输　入  : 
** 输　出  : 获得的 Timer 地址，失败返回 NULL
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if	((LW_CFG_HTIMER_EN > 0) || (LW_CFG_ITIMER_EN > 0)) && (LW_CFG_MAX_TIMERS)

PLW_CLASS_TIMER  _Allocate_Timer_Object (VOID)
{
    REGISTER PLW_LIST_MONO         pmonoFree;
    REGISTER PLW_CLASS_TIMER       ptmrFree;
    
    if (_LIST_MONO_IS_EMPTY(_K_resrcTmr.RESRC_pmonoFreeHeader)) {
        return  (LW_NULL);
    }
    
    pmonoFree = _list_mono_allocate_seq(&_K_resrcTmr.RESRC_pmonoFreeHeader, 
                                        &_K_resrcTmr.RESRC_pmonoFreeTail);
                                                                        /*  获得资源                    */
    ptmrFree  = _LIST_ENTRY(pmonoFree, LW_CLASS_TIMER, 
                            TIMER_monoResrcList);                       /*  获得资源表容器地址          */
    
    _K_resrcTmr.RESRC_uiUsed++;
    if (_K_resrcTmr.RESRC_uiUsed > _K_resrcTmr.RESRC_uiMaxUsed) {
        _K_resrcTmr.RESRC_uiMaxUsed = _K_resrcTmr.RESRC_uiUsed;
    }
    
    return  (ptmrFree);
}
/*********************************************************************************************************
** 函数名称: _Free_Timer_Object
** 功能描述: 将 Timer 控制块交还缓冲池
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _Free_Timer_Object (PLW_CLASS_TIMER    ptmrFree)
{
    REGISTER PLW_LIST_MONO    pmonoFree;
    
    pmonoFree = &ptmrFree->TIMER_monoResrcList;
    
    _list_mono_free_seq(&_K_resrcTmr.RESRC_pmonoFreeHeader, 
                        &_K_resrcTmr.RESRC_pmonoFreeTail, 
                        pmonoFree);
    
    _K_resrcTmr.RESRC_uiUsed--;
}

#endif                                                                  /*  ((LW_CFG_HTIMER_EN > 0)     */
                                                                        /*  (LW_CFG_ITIMER_EN > 0))     */
                                                                        /*  (LW_CFG_MAX_TIMERS)         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
