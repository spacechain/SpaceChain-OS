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
** 文   件   名: listEventSet.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 13 日
**
** 描        述: 这是系统 Event Set 链表操作。

** BUG
2007.04.08  加入了对裁剪的宏支持
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: _Allocate_EventSet_Object
** 功能描述: 从空闲EventSet控件池中取出一个空闲EventSet
** 输　入  : 
** 输　出  : 获得的Object地址，失败返回 NULL
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)

PLW_CLASS_EVENTSET  _Allocate_EventSet_Object (VOID)
{
    REGISTER PLW_LIST_MONO       pmonoFree;
    REGISTER PLW_CLASS_EVENTSET  pesFree;
    
    if (_LIST_MONO_IS_EMPTY(_K_resrcEventSet.RESRC_pmonoFreeHeader)) {  /*  检查缓冲区是否为空          */
        return  (LW_NULL);
    }
    
    pmonoFree  = _list_mono_allocate_seq(&_K_resrcEventSet.RESRC_pmonoFreeHeader, 
                                         &_K_resrcEventSet.RESRC_pmonoFreeTail);
                                                                        /*  获得资源                    */
    pesFree    = _LIST_ENTRY(pmonoFree, LW_CLASS_EVENTSET, 
                             EVENTSET_monoResrcList);                   /*  获得资源表容器地址          */
                             
    _K_resrcEventSet.RESRC_uiUsed++;
    if (_K_resrcEventSet.RESRC_uiUsed > _K_resrcEventSet.RESRC_uiMaxUsed) {
        _K_resrcEventSet.RESRC_uiMaxUsed = _K_resrcEventSet.RESRC_uiUsed;
    }
    
    return  (pesFree);
}
/*********************************************************************************************************
** 函数名称: _Free_EventSet_Object
** 功能描述: 将EventSet控制块交还缓冲池
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _Free_EventSet_Object (PLW_CLASS_EVENTSET  pesFree)
{
    REGISTER PLW_LIST_MONO    pmonoFree;
    
    pmonoFree = &pesFree->EVENTSET_monoResrcList;
    
    _list_mono_free_seq(&_K_resrcEventSet.RESRC_pmonoFreeHeader, 
                        &_K_resrcEventSet.RESRC_pmonoFreeTail, 
                        pmonoFree);
    
    _K_resrcEventSet.RESRC_uiUsed--;
}

#endif                                                                  /*  (LW_CFG_EVENTSET_EN > 0)    */
                                                                        /*  (LW_CFG_MAX_EVENTSETS > 0)  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
