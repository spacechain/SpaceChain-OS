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
** 文   件   名: _ThreadJoinLib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 18 日
**
** 描        述: 线程合并和解锁CORE函数库

** BUG
2007.11.13  使用链表库对链表操作进行完全封装.
2007.11.13  加入 TCB_ptcbJoin 信息记录与判断.
2008.03.30  使用新的就绪环操作.
2010.08.03  将外部不使用的函数改为 static 类型, 
            这里的函数都是在内核锁定模式下调用的, 所以只需关闭中断就可避免 SMP 抢占.
2012.03.20  减少对 _K_ptcbTCBCur 的引用, 尽量采用局部变量, 减少对当前 CPU ID 获取的次数.
2014.12.03  不再使用 suspend 操作, 转而使用 LW_THREAD_STATUS_JOIN.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: _ThreadJoinWait
** 功能描述: 线程合并后阻塞自己 (在进入内核并关中断后被调用)
** 输　入  : ptcbCur   当前任务控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _ThreadJoinWait (PLW_CLASS_TCB  ptcbCur)
{
    REGISTER PLW_CLASS_PCB  ppcbMe;
             
    ppcbMe = _GetPcb(ptcbCur);
    __DEL_FROM_READY_RING(ptcbCur, ppcbMe);                             /*  从就绪环中删除              */
    ptcbCur->TCB_usStatus |= LW_THREAD_STATUS_JOIN;                     /*  设置为 join 状态            */
}
/*********************************************************************************************************
** 函数名称: _ThreadJoinWakeup
** 功能描述: 就绪其他线程 (在进入内核后被调用)
** 输　入  : ptcb      任务控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _ThreadJoinWakeup (PLW_CLASS_TCB  ptcb)
{
    REGISTER PLW_CLASS_PCB  ppcb;
    
    ptcb->TCB_usStatus &= (~LW_THREAD_STATUS_JOIN);
    if (__LW_THREAD_IS_READY(ptcb)) {
       ptcb->TCB_ucSchedActivate = LW_SCHED_ACT_INTERRUPT;              /*  中断激活方式                */
       ppcb = _GetPcb(ptcb);
       __ADD_TO_READY_RING(ptcb, ppcb);                                 /*  加入就绪环                  */
    }
}
/*********************************************************************************************************
** 函数名称: _ThreadJoin
** 功能描述: 将当前线程合并到其他线程 (在进入内核后被调用)
** 输　入  : ptcbDes          要合并的线程
**           ppvRetValSave    目的线程结束时的返回值存放地址
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _ThreadJoin (PLW_CLASS_TCB  ptcbDes, PVOID  *ppvRetValSave)
{
    INTREG                iregInterLevel;
    PLW_CLASS_TCB         ptcbCur;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  当前任务控制块              */
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_JOIN, 
                      ptcbCur->TCB_ulId, ptcbDes->TCB_ulId, LW_NULL);
    
    ptcbCur->TCB_ppvJoinRetValSave = ppvRetValSave;                     /*  保存存放返回值的地址        */
                                                                        /*  加入等待队列                */
    _List_Line_Add_Ahead(&ptcbCur->TCB_lineJoin, &ptcbDes->TCB_plineJoinHeader);
    
    ptcbCur->TCB_ptcbJoin = ptcbDes;                                    /*  记录目标线程                */
    
    iregInterLevel = KN_INT_DISABLE();
    _ThreadJoinWait(ptcbCur);                                           /*  阻塞自己等待对方激活        */
    KN_INT_ENABLE(iregInterLevel);
}
/*********************************************************************************************************
** 函数名称: _ThreadDisjoin
** 功能描述: 指定线程解除合并的线程 (在进入内核后被调用)
** 输　入  : ptcbDes          合并的目标线程
**           ptcbDisjoin      需要解除合并的线程
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
** 注  意  : 此函数不操作就绪表, 外部需要自行操作就绪表.
*********************************************************************************************************/
VOID  _ThreadDisjoin (PLW_CLASS_TCB  ptcbDes, PLW_CLASS_TCB  ptcbDisjoin)
{
    INTREG  iregInterLevel;
    
    if (ptcbDisjoin->TCB_ppvJoinRetValSave) {                           /*  等待返回值                  */
        *ptcbDisjoin->TCB_ppvJoinRetValSave = LW_NULL;                  /*  保存返回值                  */
    }
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_DETACH, 
                      ptcbDisjoin->TCB_ulId, LW_NULL, LW_NULL);
                      
    _List_Line_Del(&ptcbDisjoin->TCB_lineJoin, &ptcbDes->TCB_plineJoinHeader);
    
    ptcbDisjoin->TCB_ptcbJoin = LW_NULL;                                /*  清除记录的等待线程 tcb      */
    
    iregInterLevel = KN_INT_DISABLE();
    ptcbDisjoin->TCB_usStatus &= (~LW_THREAD_STATUS_JOIN);              /*  仅修改状态即可              */
    KN_INT_ENABLE(iregInterLevel);
}
/*********************************************************************************************************
** 函数名称: _ThreadDisjoinWakeup
** 功能描述: 指定线程解除合并的线程 (在进入内核后被调用)
** 输　入  : ptcbDes          合并的目标线程
**           ptcbWakeup       需要解除合并并唤醒的线程
**           pvArg            返回值
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _ThreadDisjoinWakeup (PLW_CLASS_TCB  ptcbDes, PLW_CLASS_TCB  ptcbWakeup, PVOID  pvArg)
{
    INTREG  iregInterLevel;
    
    if (ptcbWakeup->TCB_ppvJoinRetValSave) {                            /*  等待返回值                  */
        *ptcbWakeup->TCB_ppvJoinRetValSave = pvArg;                     /*  保存返回值                  */
    }
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_DETACH, 
                      ptcbWakeup->TCB_ulId, pvArg, LW_NULL);
                      
    _List_Line_Del(&ptcbWakeup->TCB_lineJoin, &ptcbDes->TCB_plineJoinHeader);
    
    ptcbWakeup->TCB_ptcbJoin = LW_NULL;                                 /*  清除记录的等待线程 tcb      */
    
    iregInterLevel = KN_INT_DISABLE();
    _ThreadJoinWakeup(ptcbWakeup);                                      /*  释放合并的线程              */
    KN_INT_ENABLE(iregInterLevel);
}
/*********************************************************************************************************
** 函数名称: _ThreadDisjoinWakeupAll
** 功能描述: 指定线程解除合并的所有线程 (在进入内核后被调用)
** 输　入  : ptcbDes          合并的目标线程
**           pvArg            返回值
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _ThreadDisjoinWakeupAll (PLW_CLASS_TCB  ptcbDes, PVOID  pvArg)
{
    REGISTER PLW_CLASS_TCB  ptcbJoin;
    
    while (ptcbDes->TCB_plineJoinHeader) {
        ptcbJoin = _LIST_ENTRY(ptcbDes->TCB_plineJoinHeader, 
                               LW_CLASS_TCB, TCB_lineJoin);
        _ThreadDisjoinWakeup(ptcbDes, ptcbJoin, pvArg);                 /*  释放并唤醒退出合并的线程    */
    }
}
/*********************************************************************************************************
** 函数名称: _ThreadDetach
** 功能描述: 指定线程解除合并的所有其他线程并不允许其他线程合并自己 (在进入内核后被调用)
** 输　入  : ptcbDes          合并的目标线程
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _ThreadDetach (PLW_CLASS_TCB  ptcbDes)
{
    if (ptcbDes->TCB_plineJoinHeader) {                                 /*  已经有线程合并              */
        _ThreadDisjoinWakeupAll(ptcbDes, LW_NULL);                      /*  激活                        */
    }
    
    ptcbDes->TCB_bDetachFlag = LW_TRUE;                                 /*  严禁合并自己                */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
