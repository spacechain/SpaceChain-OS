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
** 文   件   名: _EventHighLevel.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 19 日
**
** 描        述: 这是系统事件队列高级管理函数.

** BUG
2007.04.08  删除了 _EventMakeThreadReadyFIFO_LowLevel() 和 _EventMakeThreadReadyPRIO_LowLevel() 的 
            ppcb变量，这个变量没有用.
2008.03.30  使用新的就绪环操作.
2008.03.30  加入新的 wake up 机制的处理.
2008.05.18  由于 _EventMakeThreadWait????() 是在关中断中进行的, 所以将时间存在标志的处理放在其中, 这样更为
            安全.
2008.11.30  整理文件, 修改注释.
2009.04.28  _EventMakeThreadReady_HighLevel() 修改开关中断时机.
2010.08.03  添加部分注释.
2012.03.20  减少对 _K_ptcbTCBCur 的引用, 尽量采用局部变量, 减少对当前 CPU ID 获取的次数.
2013.04.01  修正 GCC 4.7.3 引发的新 warning.
2013.07.18  使用新的获取 TCB 的方法, 确保 SMP 系统安全.
2013.09.02  等待类型改为 16 位.
2014.01.14  修改长函数名.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: _EventWaitFifo
** 功能描述: 将一个线程加入 FIFO 事件等待队列,同时设置相关标志位 (进入内核且关中断状态下被调用)
** 输　入  : pevent        事件
**           ppringList    等待链表
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if (LW_CFG_EVENT_EN > 0) && (LW_CFG_MAX_EVENTS > 0)

VOID  _EventWaitFifo (PLW_CLASS_EVENT  pevent, PLW_LIST_RING  *ppringList)
{
    REGISTER PLW_CLASS_PCB    ppcb;
             PLW_CLASS_TCB    ptcbCur;
             
    LW_TCB_GET_CUR(ptcbCur);                                            /*  当前任务控制块              */
    
    ptcbCur->TCB_peventPtr = pevent;
    ppcb = _GetPcb(ptcbCur);
    __DEL_FROM_READY_RING(ptcbCur, ppcb);                               /*  从就绪队列中删除            */
    _AddTCBToEventFifo(ptcbCur, pevent, ppringList);                    /*  加入等待队列                */

    if (ptcbCur->TCB_ulDelay) {
        __ADD_TO_WAKEUP_LINE(ptcbCur);                                  /*  加入等待扫描链              */
    }
    
    ptcbCur->TCB_ucIsEventDelete = LW_EVENT_EXIST;                      /*  事件存在                    */
}
/*********************************************************************************************************
** 函数名称: _EventWaitPriority
** 功能描述: 将一个线程加入优先级事件等待队列,同时设置相关标志位 (进入内核且关中断状态下被调用)
** 输　入  : pevent        事件
**           ppringList    等待链表
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _EventWaitPriority (PLW_CLASS_EVENT  pevent, PLW_LIST_RING  *ppringList)
{
    REGISTER PLW_CLASS_PCB    ppcb;
             PLW_CLASS_TCB    ptcbCur;
    
    LW_TCB_GET_CUR(ptcbCur);                                            /*  当前任务控制块              */
    
    ptcbCur->TCB_peventPtr = pevent;
    ppcb = _GetPcb(ptcbCur);
    __DEL_FROM_READY_RING(ptcbCur, ppcb);                               /*  从就绪队列中删除            */
    _AddTCBToEventPriority(ptcbCur, pevent, ppringList);                /*  加入等待队列                */

    if (ptcbCur->TCB_ulDelay) {
        __ADD_TO_WAKEUP_LINE(ptcbCur);                                  /*  加入等待扫描链              */
    }
    
    ptcbCur->TCB_ucIsEventDelete = LW_EVENT_EXIST;                      /*  事件存在                    */
}
/*********************************************************************************************************
** 函数名称: _EventReadyFifoLowLevel
** 功能描述: 将一个线程从 FIFO 事件等待队列中解锁并就绪, 同时设置相关标志位
** 输　入  : pevent            事件
**           pvMsgBoxMessage   扩展消息
**           ppringList        等待链表
** 输　出  : 激活的任务控制块
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PLW_CLASS_TCB  _EventReadyFifoLowLevel (PLW_CLASS_EVENT  pevent, 
                                        PVOID            pvMsgBoxMessage, 
                                        PLW_LIST_RING   *ppringList)
{
    REGISTER PLW_CLASS_TCB    ptcb;
    
    ptcb = _EventQGetTcbFifo(pevent, ppringList);                       /*  查找需要激活的线程          */
    
    _DelTCBFromEventFifo(ptcb, pevent, ppringList);
    
    ptcb->TCB_pvMsgBoxMessage = pvMsgBoxMessage;                        /*  传递二进制信号量简单消息    */
    
    if (ptcb->TCB_usStatus & LW_THREAD_STATUS_DELAY) {
        __DEL_FROM_WAKEUP_LINE(ptcb);                                   /*  退出等待队列                */
        ptcb->TCB_ulDelay = 0ul;
    }
    
    return  (ptcb);
}
/*********************************************************************************************************
** 函数名称: _EventReadyPriorityLowLevel
** 功能描述: 将一个线程从优先级事件等待队列中解锁并就绪, 同时设置相关标志位
** 输　入  : pevent            事件
**           pvMsgBoxMessage   扩展消息
**           ppringList        等待链表
** 输　出  : 激活的任务控制块
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PLW_CLASS_TCB  _EventReadyPriorityLowLevel (PLW_CLASS_EVENT  pevent, 
                                            PVOID            pvMsgBoxMessage, 
                                            PLW_LIST_RING   *ppringList)
{
    REGISTER PLW_CLASS_TCB    ptcb;
    
    ptcb = _EventQGetTcbPriority(pevent, ppringList);                   /*  查找需要激活的线程          */
    
    _DelTCBFromEventPriority(ptcb, pevent, ppringList);
    
    ptcb->TCB_pvMsgBoxMessage = pvMsgBoxMessage;                        /*  传递二进制信号量简单消息    */

    if (ptcb->TCB_usStatus & LW_THREAD_STATUS_DELAY) {
        __DEL_FROM_WAKEUP_LINE(ptcb);                                   /*  退出等待队列                */
        ptcb->TCB_ulDelay = 0ul;
    }

    return  (ptcb);
}
/*********************************************************************************************************
** 函数名称: _EventReadyHighLevel
** 功能描述: 将一个线程从 FIFO 事件等待队列中解锁并就绪, 同时设置相关标志位, 需要时进入就绪表.
**           此函数在内核锁定状态被调用.
** 输　入  : ptcb          任务控制块
**           usWaitType    等待类型
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                      此函数在开中断时被调用.
*********************************************************************************************************/
VOID  _EventReadyHighLevel (PLW_CLASS_TCB    ptcb, UINT16   usWaitType)
{
             INTREG           iregInterLevel;
    REGISTER PLW_CLASS_PCB    ppcb;
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */
    
    ptcb->TCB_peventPtr = LW_NULL;
    
    if (ptcb->TCB_ucWaitTimeout) {
        ptcb->TCB_ucWaitTimeout = LW_WAIT_TIME_CLEAR;                   /*  清除超时位                  */
    
    } else {                                                            /*  清除相应等待位              */
        ptcb->TCB_usStatus = (UINT16)(ptcb->TCB_usStatus & (~usWaitType));
        if (__LW_THREAD_IS_READY(ptcb)) {                               /*  是否就绪                    */
            ptcb->TCB_ucSchedActivate = LW_SCHED_ACT_INTERRUPT;         /*  中断激活方式                */
            ppcb = _GetPcb(ptcb);
            __ADD_TO_READY_RING(ptcb, ppcb);                            /*  加入到相对优先级就绪环      */
        }
    }
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  开中断                      */
}
/*********************************************************************************************************
** 函数名称: _EventPrioTryBoost
** 功能描述: 互斥信号量提升拥有任务优先级.
**           此函数在内核锁定状态被调用.
** 输　入  : pevent        事件
**           ptcbCur       当前任务控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _EventPrioTryBoost (PLW_CLASS_EVENT  pevent, PLW_CLASS_TCB   ptcbCur)
{
    PLW_CLASS_TCB    ptcbOwner = (PLW_CLASS_TCB)pevent->EVENT_pvTcbOwn;
    
    if (ptcbOwner->TCB_iDeleteProcStatus) {                             /*  任务已被删除或正在被删除    */
        return;
    }
    
    if (LW_PRIO_IS_HIGH(ptcbCur->TCB_ucPriority, 
                        ptcbOwner->TCB_ucPriority)) {                   /*  需要改变优先级              */
        if (pevent->EVENT_ulOption & LW_OPTION_INHERIT_PRIORITY) {      /*  优先级继承                  */
            _SchedSetPrio(ptcbOwner, ptcbCur->TCB_ucPriority);
        
        } else if (LW_PRIO_IS_HIGH(pevent->EVENT_ucCeilingPriority,
                                   ptcbOwner->TCB_ucPriority)) {        /*  优先级天花板                */
            _SchedSetPrio(ptcbOwner, pevent->EVENT_ucCeilingPriority);
        }
    }
}
/*********************************************************************************************************
** 函数名称: _EventPrioTryResume
** 功能描述: 互斥信号量降低当前任务优先级.
**           此函数在内核锁定状态被调用.
** 输　入  : pevent        事件
**           ptcbCur       当前任务控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _EventPrioTryResume (PLW_CLASS_EVENT  pevent, PLW_CLASS_TCB   ptcbCur)
{
    UINT8   ucPriorityOld = (UINT8)pevent->EVENT_ulMaxCounter;
    
    if (!LW_PRIO_IS_EQU(ptcbCur->TCB_ucPriority, ucPriorityOld)) {      /*  产生了优先级变换            */
        _SchedSetPrio(ptcbCur, ucPriorityOld);                          /*  返回优先级                  */
    }
}
/*********************************************************************************************************
** 函数名称: _EventUnQueue
** 功能描述: 将一个线程从事件等待队列中解锁
** 输　入  : ptcb      任务控制块
** 输　出  : 事件控制块
** 全局变量: 
** 调用模块: 
** 注  意  : 如果是优先级等待队列, 则在等待过程中可能有优先级的改变, 所以不能靠当前优先级来判断队列的位置
*********************************************************************************************************/
PLW_CLASS_EVENT  _EventUnQueue (PLW_CLASS_TCB    ptcb)
{
    REGISTER PLW_CLASS_EVENT    pevent;
    REGISTER PLW_LIST_RING     *ppringList;
    
    pevent = ptcb->TCB_peventPtr;
    
    if (pevent->EVENT_ulOption & LW_OPTION_WAIT_PRIORITY) {             /*  优先级队列                  */
        ppringList = ptcb->TCB_ppringPriorityQueue;                     /*  确定等待队列位置            */
        _DelTCBFromEventPriority(ptcb, pevent, ppringList);             /*  从队列中移除                */
        
    } else {                                                            /*  FIFO 队列                   */
        _EVENT_FIFO_Q_PTR(ptcb->TCB_iPendQ, ppringList);
        _DelTCBFromEventFifo(ptcb, pevent, ppringList);                 /*  从队列中移除                */
    }
    
    ptcb->TCB_peventPtr = LW_NULL;                                      /*  清除事件                    */
    
    return  (pevent);
}

#endif                                                                  /*  (LW_CFG_EVENT_EN > 0)       */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
