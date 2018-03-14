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
** 文   件   名: _EventQueue.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 19 日
**
** 描        述: 这是系统事件队列管理函数。

** BUG
2008.11.30  整理文件, 修改注释.
2009.04.28  整理文件, 修改注释.
2014.01.14  修改长函数名.
2016.07.29  修正 _AddTCBToEventPriority() 插入错误.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: _AddTCBToEventFifo
** 功能描述: 将一个线程加入 FIFO 事件等待队列
** 输　入  : ptcb           任务控制块
**           pevent         事件控制块
**           ppringList     等待队列
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if (LW_CFG_EVENT_EN > 0) && (LW_CFG_MAX_EVENTS > 0)

VOID  _AddTCBToEventFifo (PLW_CLASS_TCB    ptcb, 
                          PLW_CLASS_EVENT  pevent, 
                          PLW_LIST_RING   *ppringList)
{
    _List_Ring_Add_Ahead(&ptcb->TCB_ringEvent, ppringList);             /*  加入到 FIFO 队列头          */
    
    pevent->EVENT_wqWaitQ[ptcb->TCB_iPendQ].WQ_usNum++;                 /*  等待事件的个数++            */
}
/*********************************************************************************************************
** 函数名称: _DelTCBFromEventFifo
** 功能描述: 从 FIFO 事件等待队列中删除一个线程
** 输　入  : ptcb           任务控制块
**           pevent         事件控制块
**           ppringList     等待队列
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _DelTCBFromEventFifo (PLW_CLASS_TCB      ptcb, 
                            PLW_CLASS_EVENT    pevent, 
                            PLW_LIST_RING     *ppringList)
{
    _List_Ring_Del(&ptcb->TCB_ringEvent, ppringList);
    
    pevent->EVENT_wqWaitQ[ptcb->TCB_iPendQ].WQ_usNum--;                 /*  等待事件的个数--            */
}
/*********************************************************************************************************
** 函数名称: _AddTCBToEventPriority
** 功能描述: 将一个线程加入 PRIORITY 事件等待队列
** 输　入  : ptcb           任务控制块
**           pevent         事件控制块
**           ppringList     等待队列
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _AddTCBToEventPriority (PLW_CLASS_TCB    ptcb, 
                              PLW_CLASS_EVENT  pevent, 
                              PLW_LIST_RING   *ppringList)
{
             PLW_LIST_RING    pringList;
    REGISTER PLW_LIST_RING    pringListHeader;
    REGISTER PLW_CLASS_TCB    ptcbTemp;
    REGISTER UINT8            ucPriority;                               /*  优先级暂存变量              */
    
    ucPriority      = ptcb->TCB_ucPriority;                             /*  优先级                      */
    pringListHeader = *ppringList;
    
    if (pringListHeader == LW_NULL) {                                   /*  没有等待任务                */
        _List_Ring_Add_Ahead(&ptcb->TCB_ringEvent, ppringList);         /*  直接添加                    */
        
    } else {
        ptcbTemp = _LIST_ENTRY(pringListHeader, LW_CLASS_TCB, TCB_ringEvent);
        if (LW_PRIO_IS_HIGH(ucPriority, ptcbTemp->TCB_ucPriority)) {
            _List_Ring_Add_Ahead(&ptcb->TCB_ringEvent, ppringList);     /*  加入到队列头                */
        
        } else {
            pringList = _list_ring_get_next(pringListHeader);
            while (pringList != pringListHeader) {
                ptcbTemp = _LIST_ENTRY(pringList, LW_CLASS_TCB, TCB_ringEvent);
                if (LW_PRIO_IS_HIGH(ucPriority, ptcbTemp->TCB_ucPriority)) {
                    break;
                }
                pringList = _list_ring_get_next(pringList);
            }
            _List_Ring_Add_Last(&ptcb->TCB_ringEvent, &pringList);
        }
    }
    
    pevent->EVENT_wqWaitQ[ptcb->TCB_iPendQ].WQ_usNum++;                 /*  等待事件的个数++            */
}
/*********************************************************************************************************
** 函数名称: _DelTCBFromEventPriority
** 功能描述: 从FIFO事件等待队列中删除一个线程
** 输　入  : ptcb           任务控制块
**           pevent         事件控制块
**           ppringList     等待队列
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _DelTCBFromEventPriority (PLW_CLASS_TCB      ptcb, 
                                PLW_CLASS_EVENT    pevent, 
                                PLW_LIST_RING     *ppringList)
{    
    _List_Ring_Del(&ptcb->TCB_ringEvent, ppringList);
    
    pevent->EVENT_wqWaitQ[ptcb->TCB_iPendQ].WQ_usNum--;                 /*  等待事件的个数--            */
}

#endif                                                                  /*  (LW_CFG_EVENT_EN > 0)       */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
