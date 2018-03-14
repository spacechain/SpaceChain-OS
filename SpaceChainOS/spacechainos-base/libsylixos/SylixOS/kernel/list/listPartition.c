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
** 文   件   名: listPartition.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 01 月 16 日
**
** 描        述: 这是系统PARTITION资源链表操作。

** BUG:
2009-04-09  使用资源顺序分配.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: _Allocate_Partition_Object
** 功能描述: 从空闲 Partition 控件池中取出一个空闲 Partition
** 输　入  : 
** 输　出  : 获得的 Partition 地址，失败返回 NULL
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if	(LW_CFG_PARTITION_EN > 0) && (LW_CFG_MAX_PARTITIONS > 0)

PLW_CLASS_PARTITION  _Allocate_Partition_Object (VOID)
{
    REGISTER PLW_LIST_MONO             pmonoFree;
    REGISTER PLW_CLASS_PARTITION       p_partFree;
    
    if (_LIST_MONO_IS_EMPTY(_K_resrcPart.RESRC_pmonoFreeHeader)) {
        return  (LW_NULL);
    }
    
    pmonoFree   = _list_mono_allocate_seq(&_K_resrcPart.RESRC_pmonoFreeHeader, 
                                          &_K_resrcPart.RESRC_pmonoFreeTail);
                                                                        /*  获得资源                    */
    p_partFree  = _LIST_ENTRY(pmonoFree, 
                              LW_CLASS_PARTITION, 
                              PARTITION_monoResrcList);                 /*  获得资源表容器地址          */
                              
    _K_resrcPart.RESRC_uiUsed++;
    if (_K_resrcPart.RESRC_uiUsed > _K_resrcPart.RESRC_uiMaxUsed) {
        _K_resrcPart.RESRC_uiMaxUsed = _K_resrcPart.RESRC_uiUsed;
    }
    
    return  (p_partFree);
}
/*********************************************************************************************************
** 函数名称: _Free_Partition_Object
** 功能描述: 将 Partition 控制块交还缓冲池
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _Free_Partition_Object (PLW_CLASS_PARTITION    p_partFree)
{
    REGISTER PLW_LIST_MONO    pmonoFree;
    
    pmonoFree = &p_partFree->PARTITION_monoResrcList;
    
    _list_mono_free_seq(&_K_resrcPart.RESRC_pmonoFreeHeader, 
                        &_K_resrcPart.RESRC_pmonoFreeTail, 
                        pmonoFree);
                        
    _K_resrcPart.RESRC_uiUsed--;
}

#endif                                                                  /*  (LW_CFG_PARTITION_EN > 0)   */
                                                                        /*  (LW_CFG_MAX_PARTITIONS > 0) */
/*********************************************************************************************************
  END
*********************************************************************************************************/
