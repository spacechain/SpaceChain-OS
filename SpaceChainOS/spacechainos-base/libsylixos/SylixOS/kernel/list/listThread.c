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
** 文   件   名: listThread.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 13 日
**
** 描        述: 这是系统 Theard 链表操作。

** BUG:
2013.05.07  从今日起, SylixOS TCB 不再存放在任务堆栈中.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: _Allocate_Tcb_Object
** 功能描述: 从空闲 Tcb 控件池中取出一个空闲 TCB
** 输　入  : 
** 输　出  : 获得的 Object 地址，失败返回 NULL
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PLW_CLASS_TCB  _Allocate_Tcb_Object (VOID)
{
    REGISTER PLW_LIST_MONO       pmonoFree;
    REGISTER PLW_CLASS_TCB       ptcbFree;
    
    if (_LIST_MONO_IS_EMPTY(_K_resrcTcb.RESRC_pmonoFreeHeader)) {       /*  检查缓冲池是否为空          */
        return  (LW_NULL);
    }
    
    pmonoFree = _list_mono_allocate_seq(&_K_resrcTcb.RESRC_pmonoFreeHeader, 
                                        &_K_resrcTcb.RESRC_pmonoFreeTail);
                                                                        /*  获得资源                    */
                                                                        
    ptcbFree = _LIST_ENTRY(pmonoFree, LW_CLASS_TCB, TCB_monoResrcList); /*  获得资源表容器地址          */
    
    _K_resrcTcb.RESRC_uiUsed++;
    if (_K_resrcTcb.RESRC_uiUsed > _K_resrcTcb.RESRC_uiMaxUsed) {
        _K_resrcTcb.RESRC_uiMaxUsed = _K_resrcTcb.RESRC_uiUsed;
    }
    
    return  (ptcbFree);
}
/*********************************************************************************************************
** 函数名称: _Free_Tcb_Object
** 功能描述: 将 Tcb 控制块交还缓冲池
** 输　入  : ptcbFree  需要交还的 TCB
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _Free_Tcb_Object (PLW_CLASS_TCB    ptcbFree)
{
    REGISTER PLW_LIST_MONO    pmonoFree;
    
    pmonoFree = &ptcbFree->TCB_monoResrcList;
    
    _list_mono_free_seq(&_K_resrcTcb.RESRC_pmonoFreeHeader, 
                        &_K_resrcTcb.RESRC_pmonoFreeTail, 
                        pmonoFree);
                        
    _K_resrcTcb.RESRC_uiUsed--;
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
