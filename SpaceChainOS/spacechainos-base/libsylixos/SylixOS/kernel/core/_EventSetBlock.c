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
** 文   件   名: _EventSetBlock.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 20 日
**
** 描        述: 将线程插入等待事件集队列

** BUG
2008.03.29  加入新的 wake up 机制.
2008.03.30  使用新的就绪环操作.
2012.03.20  减少对 _K_ptcbTCBCur 的引用, 尽量采用局部变量, 减少对当前 CPU ID 获取的次数.
2013.07.18  使用新的获取 TCB 的方法, 确保 SMP 系统安全.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: _EventSetBlock
** 功能描述: 将线程插入等待事件集队列 (进入内核且关中断状态下被调用)
** 输　入  : pes            事件组控制块
**           pesn           事件组等待节点
**           ulEvents       事件组
**           ucWaitType     等待类型
**           ulWaitTime     等待时间
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)

VOID  _EventSetBlock (PLW_CLASS_EVENTSET        pes,
                      PLW_CLASS_EVENTSETNODE    pesn,
                      ULONG                     ulEvents,
                      UINT8                     ucWaitType,
                      ULONG                     ulWaitTime)
{
    REGISTER PLW_CLASS_PCB               ppcb;
             PLW_CLASS_TCB               ptcbCur;
             
    LW_TCB_GET_CUR(ptcbCur);                                            /*  当前任务控制块              */
    
    ptcbCur->TCB_usStatus       |= LW_THREAD_STATUS_EVENTSET;
    ptcbCur->TCB_ucWaitTimeout   = LW_WAIT_TIME_CLEAR;
    ptcbCur->TCB_ucIsEventDelete = LW_EVENT_EXIST;
    
    ptcbCur->TCB_ulDelay = ulWaitTime;
    if (ulWaitTime) {
        __ADD_TO_WAKEUP_LINE(ptcbCur);                                  /*  加入等待扫描链              */
    }
    
#if LW_CFG_THREAD_DEL_EN > 0
    ptcbCur->TCB_pesnPtr = pesn;
#endif
    
    pesn->EVENTSETNODE_pesEventSet = (PVOID)pes;
    pesn->EVENTSETNODE_ulEventSets = ulEvents;
    pesn->EVENTSETNODE_ucWaitType  = ucWaitType;
    pesn->EVENTSETNODE_ptcbMe      = (PVOID)ptcbCur;
    
    _List_Line_Add_Ahead(&pesn->EVENTSETNODE_lineManage, &pes->EVENTSET_plineWaitList);
    
    ppcb = _GetPcb(ptcbCur);
    
    __DEL_FROM_READY_RING(ptcbCur, ppcb);                               /*  从就绪环中删除              */
}

#endif                                                                  /*  (LW_CFG_EVENTSET_EN > 0)    */
                                                                        /*  (LW_CFG_MAX_EVENTSETS > 0)  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
