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
** 文   件   名: _CoroutineLib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 06 月 19 日
**
** 描        述: 这是协程管理库(协程是一个轻量级的并发执行单位). 
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_COROUTINE_EN > 0
/*********************************************************************************************************
** 函数名称: _CoroutineReclaim
** 功能描述: 清除延迟删除的协程.
** 输　入  : ptcb                     线程控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _CoroutineReclaim (PLW_CLASS_TCB  ptcb)
{
             INTREG                 iregInterLevel;
    REGISTER PLW_CLASS_COROUTINE    pcrcbPrev;
    REGISTER PLW_LIST_RING          pringTemp;
    
    pringTemp = _list_ring_get_prev(ptcb->TCB_pringCoroutineHeader);
    
    pcrcbPrev = _LIST_ENTRY(pringTemp, 
                            LW_CLASS_COROUTINE, 
                            COROUTINE_ringRoutine);
    
    if (pcrcbPrev->COROUTINE_ulFlags & LW_COROUTINE_FLAG_DELETE) {
        _ThreadSafeInternal();                                          /*  进入安全模式                */
        
        LW_SPIN_LOCK_QUICK(&ptcb->TCB_slLock, &iregInterLevel);
        _List_Ring_Del(&pcrcbPrev->COROUTINE_ringRoutine,
                       &ptcb->TCB_pringCoroutineHeader);                /*  从协程表中删除              */
        LW_SPIN_UNLOCK_QUICK(&ptcb->TCB_slLock, iregInterLevel);
        
        if (pcrcbPrev->COROUTINE_ulFlags & LW_COROUTINE_FLAG_DYNSTK) {
            _StackFree(ptcb, pcrcbPrev->COROUTINE_pstkStackLowAddr);    /*  释放内存                    */
        }
        
        _ThreadUnsafeInternal();                                        /*  退出安全模式                */
    }
}
/*********************************************************************************************************
** 函数名称: _CoroutineDeleteAll
** 功能描述: 释放指定线程所有的协程空间.
** 输　入  : ptcb                     线程控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _CoroutineFreeAll (PLW_CLASS_TCB  ptcb)
{
    REGISTER PLW_CLASS_COROUTINE    pcrcbTemp;
    REGISTER PLW_LIST_RING          pringTemp;
    
    pringTemp = ptcb->TCB_pringCoroutineHeader;
    
    while (pringTemp) {
        _List_Ring_Del(pringTemp,
                       &ptcb->TCB_pringCoroutineHeader);                /*  从协程表中删除              */
        pcrcbTemp = _LIST_ENTRY(pringTemp, 
                                LW_CLASS_COROUTINE,
                                COROUTINE_ringRoutine);
        if (pcrcbTemp->COROUTINE_ulFlags & LW_COROUTINE_FLAG_DYNSTK) {
            _StackFree(ptcb, pcrcbTemp->COROUTINE_pstkStackLowAddr);
        }
        pringTemp = ptcb->TCB_pringCoroutineHeader;
    }
}

#endif                                                                  /*  LW_CFG_COROUTINE_EN > 0     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
