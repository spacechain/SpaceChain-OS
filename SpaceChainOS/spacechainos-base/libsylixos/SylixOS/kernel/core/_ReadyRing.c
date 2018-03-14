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
** 文   件   名: _ReadyRing.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 17 日
**
** 描        述: 这是系统就绪环操作函数

** BUG
2007.12.22 整理注释.
2009.04.28 仅在就绪但未运行时加入就绪环.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: _AddTCBToReadyRing
** 功能描述: 将一个任务加入就绪环
** 输　入  : ptcb        需要加入的任务
**           ppcb        优先级控制块
**           bIsHeader   是否插入头部
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _AddTCBToReadyRing (PLW_CLASS_TCB  ptcb, PLW_CLASS_PCB  ppcb, BOOL  bIsHeader)
{
    if (bIsHeader) {
        _List_Ring_Add_Ahead(&ptcb->TCB_ringReady, &ppcb->PCB_pringReadyHeader);
        return;
    
    } else if (ptcb->TCB_ucSchedPolicy == LW_OPTION_SCHED_FIFO) {
        _List_Ring_Add_Last(&ptcb->TCB_ringReady, &ppcb->PCB_pringReadyHeader);
        return;
    }
    
    switch (ptcb->TCB_ucSchedActivateMode) {
    
    case LW_OPTION_RESPOND_IMMIEDIA:
        _List_Ring_Add_Front(&ptcb->TCB_ringReady, &ppcb->PCB_pringReadyHeader);
        break;
        
    case LW_OPTION_RESPOND_STANDARD:
        _List_Ring_Add_Last(&ptcb->TCB_ringReady, &ppcb->PCB_pringReadyHeader);
        break;
        
    default:
        if (ptcb->TCB_ucSchedActivate == LW_SCHED_ACT_OTHER) {
            _List_Ring_Add_Last(&ptcb->TCB_ringReady, &ppcb->PCB_pringReadyHeader);
        
        } else {
            _List_Ring_Add_Front(&ptcb->TCB_ringReady, &ppcb->PCB_pringReadyHeader);
        }
        break;
    }
    
    return;
}
/*********************************************************************************************************
** 函数名称: _DelTCBFromReadyRing
** 功能描述: 将一个任务从就绪环中解除
** 输　入  : ptcb        需要删除的任务
**           ppcb        优先级控制块
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _DelTCBFromReadyRing (PLW_CLASS_TCB  ptcb, PLW_CLASS_PCB  ppcb)
{
    _List_Ring_Del(&ptcb->TCB_ringReady, &ppcb->PCB_pringReadyHeader);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
