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
** 文   件   名: listEvent.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 13 日
**
** 描        述: 这是系统 Event 链表操作。

** BUG
2007.04.08  加入了对裁剪的宏支持
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: _Allocate_Event_Object
** 功能描述: 从空闲Event控件池中取出一个空闲Event
** 输　入  : 
** 输　出  : 获得的Object地址，失败返回 NULL
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if (LW_CFG_EVENT_EN > 0) && (LW_CFG_MAX_EVENTS > 0)

PLW_CLASS_EVENT  _Allocate_Event_Object (VOID)
{
    REGISTER PLW_LIST_MONO    pmonoFree;
    REGISTER PLW_CLASS_EVENT  peventFree;
    
    if (_LIST_MONO_IS_EMPTY(_K_resrcEvent.RESRC_pmonoFreeHeader)) {     /*  检查缓冲区是否为空          */
        return  (LW_NULL);
    }
    
    pmonoFree  = _list_mono_allocate_seq(&_K_resrcEvent.RESRC_pmonoFreeHeader, 
                                         &_K_resrcEvent.RESRC_pmonoFreeTail);
                                                                        /*  获得资源                    */
    peventFree = _LIST_ENTRY(pmonoFree, LW_CLASS_EVENT, 
                             EVENT_monoResrcList);                      /*  获得资源表容器地址          */
    
    _K_resrcEvent.RESRC_uiUsed++;
    if (_K_resrcEvent.RESRC_uiUsed > _K_resrcEvent.RESRC_uiMaxUsed) {
        _K_resrcEvent.RESRC_uiMaxUsed = _K_resrcEvent.RESRC_uiUsed;
    }
    
    return  (peventFree);
}
/*********************************************************************************************************
** 函数名称: _Free_Event_Object
** 功能描述: 将Event控制块交还缓冲池
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _Free_Event_Object (PLW_CLASS_EVENT  pevent)
{
    REGISTER PLW_LIST_MONO    pmonoFree;
    
    pmonoFree = &pevent->EVENT_monoResrcList;
    
    _list_mono_free_seq(&_K_resrcEvent.RESRC_pmonoFreeHeader, 
                        &_K_resrcEvent.RESRC_pmonoFreeTail, 
                        pmonoFree);
                        
    _K_resrcEvent.RESRC_uiUsed--;
}

#endif                                                                  /*  (LW_CFG_EVENT_EN > 0)       */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
