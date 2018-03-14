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
** 文   件   名: _PartitionLib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 01 月 17 日
**
** 描        述: PARTITION 内核操作函数库。

** BUG
2007.04.16  直接使用分配出来的第一个地址，不用再使用下一个字对齐地址。
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: _PartitionAllocate
** 功能描述: PARTITION 分配一个段
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if (LW_CFG_PARTITION_EN > 0) && (LW_CFG_MAX_PARTITIONS > 0)

PVOID  _PartitionAllocate (PLW_CLASS_PARTITION  p_part)
{
    REGISTER PVOID            pvRet;
    REGISTER PLW_LIST_MONO   *ppmonoHeader;
    REGISTER PLW_LIST_MONO    pmonoAllocate;
    
    ppmonoHeader = &p_part->PARTITION_pmonoFreeBlockList;
    
    if (p_part->PARTITION_pmonoFreeBlockList) {                         /*  检查是否存在空闲块          */
        pmonoAllocate = _list_mono_allocate(ppmonoHeader);
        pvRet = (PVOID)pmonoAllocate;
    
        p_part->PARTITION_ulFreeBlockCounter--;                         /*  空闲块减一                  */
        
        return  (pvRet);
    
    } else {
        return  (LW_NULL);
    }
}

/*********************************************************************************************************
** 函数名称: _PartitionFree
** 功能描述: PARTITION 交还一个段
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _PartitionFree (PLW_CLASS_PARTITION  p_part, PVOID  pvFree)
{
    REGISTER PLW_LIST_MONO   *ppmonoHeader;
    REGISTER PLW_LIST_MONO    pmonoFree;
    
    ppmonoHeader = &p_part->PARTITION_pmonoFreeBlockList;
    pmonoFree    = (PLW_LIST_MONO)pvFree;
    
    _list_mono_free(ppmonoHeader, pmonoFree);
    
    p_part->PARTITION_ulFreeBlockCounter++;                             /*  空闲块加一                  */
}

#endif                                                                  /*  (LW_CFG_PARTITION_EN > 0)   */
                                                                        /*  (LW_CFG_MAX_PARTITIONS > 0) */
/*********************************************************************************************************
  END
*********************************************************************************************************/
