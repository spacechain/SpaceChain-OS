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
** 文   件   名: listHeap.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 13 日
**
** 描        述: 这是系统 Heap 资源链表操作。
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: _Allocate_Heap_Object
** 功能描述: 从空闲 Heap 控件池中取出一个空闲 Heap
** 输　入  : 
** 输　出  : 获得的 Heap 地址，失败返回 NULL
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PLW_CLASS_HEAP  _Allocate_Heap_Object (VOID)
{
    REGISTER PLW_LIST_MONO    pmonoFree;
    REGISTER PLW_CLASS_HEAP   pheapFree;
    
    if (_LIST_MONO_IS_EMPTY(_K_resrcHeap.RESRC_pmonoFreeHeader)) {
        return  (LW_NULL);
    }
    
    pmonoFree = _list_mono_allocate_seq(&_K_resrcHeap.RESRC_pmonoFreeHeader, 
                                        &_K_resrcHeap.RESRC_pmonoFreeTail);
                                                                        /*  获得资源                   */
    pheapFree = _LIST_ENTRY(pmonoFree, LW_CLASS_HEAP, 
                            HEAP_monoResrcList);                        /*  获得资源表容器地址         */
    
    _K_resrcHeap.RESRC_uiUsed++;
    if (_K_resrcHeap.RESRC_uiUsed > _K_resrcHeap.RESRC_uiMaxUsed) {
        _K_resrcHeap.RESRC_uiMaxUsed = _K_resrcHeap.RESRC_uiUsed;
    }
    
    return  (pheapFree);
}
/*********************************************************************************************************
** 函数名称: _Free_Heap_Object
** 功能描述: 将 Heap 控制块交还缓冲池
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _Free_Heap_Object (PLW_CLASS_HEAP    pheapFree)
{
    REGISTER PLW_LIST_MONO    pmonoFree;
    
    pmonoFree = &pheapFree->HEAP_monoResrcList;
    
    _list_mono_free_seq(&_K_resrcHeap.RESRC_pmonoFreeHeader, 
                        &_K_resrcHeap.RESRC_pmonoFreeTail, 
                        pmonoFree);
    
    _K_resrcHeap.RESRC_uiUsed--;
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
