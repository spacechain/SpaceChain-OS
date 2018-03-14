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
** 文   件   名: listThreadVar.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 13 日
**
** 描        述: 这是系统 Theard Var 资源链表操作。
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: _Allocate_ThreadVar_Object
** 功能描述: 从空闲 ThreadVar 控件池中取出一个空闲 ThreadVar
** 输　入  : 
** 输　出  : 获得的 ThreadVar 地址，失败返回 NULL
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if (LW_CFG_SMP_EN == 0) && (LW_CFG_THREAD_PRIVATE_VARS_EN > 0) && (LW_CFG_MAX_THREAD_GLB_VARS > 0)

PLW_CLASS_THREADVAR  _Allocate_ThreadVar_Object (VOID)
{
    REGISTER PLW_LIST_MONO         pmonoFree;
    REGISTER PLW_CLASS_THREADVAR   pthreadvarFree;
    
    if (_LIST_MONO_IS_EMPTY(_K_resrcThreadVar.RESRC_pmonoFreeHeader)) {
        return  (LW_NULL);
    }
    
    pmonoFree      = _list_mono_allocate_seq(&_K_resrcThreadVar.RESRC_pmonoFreeHeader, 
                                             &_K_resrcThreadVar.RESRC_pmonoFreeTail);
                                                                        /*  获得资源                    */
    pthreadvarFree = _LIST_ENTRY(pmonoFree, LW_CLASS_THREADVAR, 
                                 PRIVATEVAR_monoResrcList);             /*  获得资源表容器地址          */
                                 
    _K_resrcThreadVar.RESRC_uiUsed++;
    if (_K_resrcThreadVar.RESRC_uiUsed > _K_resrcThreadVar.RESRC_uiMaxUsed) {
        _K_resrcThreadVar.RESRC_uiMaxUsed = _K_resrcThreadVar.RESRC_uiUsed;
    }
    
    return  (pthreadvarFree);
}
/*********************************************************************************************************
** 函数名称: _Free_ThreadVar_Object
** 功能描述: 将 ThreadVar 控制块交还缓冲池
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _Free_ThreadVar_Object (PLW_CLASS_THREADVAR    pthreadvarFree)
{
    REGISTER PLW_LIST_MONO    pmonoFree;
    
    pmonoFree = &pthreadvarFree->PRIVATEVAR_monoResrcList;
    
    _list_mono_free_seq(&_K_resrcThreadVar.RESRC_pmonoFreeHeader, 
                        &_K_resrcThreadVar.RESRC_pmonoFreeTail, 
                        pmonoFree);
    
    _K_resrcThreadVar.RESRC_uiUsed--;
}

#endif                                                                  /*  LW_CFG_SMP_EN == 0          */
                                                                        /*  (LW_CFG_THREAD_PRIVATE_VAR..*/
                                                                        /*  (LW_CFG_MAX_THREAD_GLB_VAR..*/
/*********************************************************************************************************
  END
*********************************************************************************************************/
