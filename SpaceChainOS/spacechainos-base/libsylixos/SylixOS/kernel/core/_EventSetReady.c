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
** 文   件   名: _EventSetReady.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 20 日
**
** 描        述: 激活等待事件集队列的线程

** BUG
2008.03.29  加入新的 wake up 机制.
2008.03.30  使用新的就绪环操作.
2009.07.03  bIsSched 应该被初始化为 LW_FALSE.
2012.07.04  合并文件.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: _EventSetDeleteReady
** 功能描述: 由于事件集删除，激活等待事件集队列的线程 (进入内核并关中断后被调用)
** 输　入  : pesn      事件组控制块
** 输　出  : 是否调度
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)

BOOL  _EventSetDeleteReady (PLW_CLASS_EVENTSETNODE    pesn)
{
    REGISTER PLW_CLASS_TCB    ptcb;
    REGISTER PLW_CLASS_PCB    ppcb;
    REGISTER BOOL             bIsSched = LW_FALSE;
    
    ptcb = (PLW_CLASS_TCB)pesn->EVENTSETNODE_ptcbMe;
    ptcb->TCB_ucIsEventDelete = LW_EVENT_DELETE;                        /*  事件被删除了                */
    
    if (ptcb->TCB_usStatus & LW_THREAD_STATUS_DELAY) {                  /*  存在于 wake up 表中         */
        __DEL_FROM_WAKEUP_LINE(ptcb);                                   /*  从等待链中删除              */
    }
    
    ptcb->TCB_ulDelay      = 0ul;
    ptcb->TCB_ulEventSets  = 0ul;
    ptcb->TCB_usStatus    &= (~LW_THREAD_STATUS_EVENTSET);
    
    if (__LW_THREAD_IS_READY(ptcb)) {                                   /*  是否就绪                    */
        ptcb->TCB_ucSchedActivate = LW_SCHED_ACT_INTERRUPT;
        ppcb = _GetPcb(ptcb);
        __ADD_TO_READY_RING(ptcb, ppcb);                                /*  加入就绪表                  */
        bIsSched = LW_TRUE;
    }
    
    _EventSetUnQueue(pesn);
    
    return  (bIsSched);
}
/*********************************************************************************************************
** 函数名称: _EventSetThreadReady
** 功能描述: 激活等待事件集队列的线程 (进入内核并关中断后被调用)
** 输　入  : pesn              事件组控制块
**           ulEventsReady     新的事件标志
** 输　出  : 是否调度
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
BOOL  _EventSetThreadReady (PLW_CLASS_EVENTSETNODE    pesn,
                            ULONG                     ulEventsReady)
{
    REGISTER PLW_CLASS_TCB    ptcb;
    REGISTER PLW_CLASS_PCB    ppcb;
    REGISTER BOOL             bIsSched = LW_FALSE;
    
    ptcb = (PLW_CLASS_TCB)pesn->EVENTSETNODE_ptcbMe;
    if (ptcb->TCB_usStatus & LW_THREAD_STATUS_DELAY) {                  /*  存在于 wake up 表中         */
        __DEL_FROM_WAKEUP_LINE(ptcb);                                   /*  从等待链中删除              */
    }
    
    ptcb->TCB_ulDelay      = 0ul;
    ptcb->TCB_ulEventSets  = ulEventsReady;
    ptcb->TCB_usStatus    &= (~LW_THREAD_STATUS_EVENTSET);
    
    if (__LW_THREAD_IS_READY(ptcb)) {                                   /*  是否就绪                    */
        ptcb->TCB_ucSchedActivate = LW_SCHED_ACT_INTERRUPT;
        ppcb = _GetPcb(ptcb);
        __ADD_TO_READY_RING(ptcb, ppcb);                                /*  加入就绪表                  */
        bIsSched = LW_TRUE;
    }
    
    _EventSetUnQueue(pesn);
    
    return  (bIsSched);
}

#endif                                                                  /*  (LW_CFG_EVENTSET_EN > 0)    */
                                                                        /*  (LW_CFG_MAX_EVENTSETS > 0)  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
