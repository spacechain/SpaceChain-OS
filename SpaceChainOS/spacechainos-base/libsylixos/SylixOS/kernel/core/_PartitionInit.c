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
** 文   件   名: _PartitionInit.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 01 月 16 日
**
** 描        述: PARTITION 初始化函数库。

** BUG:
2009.07.28  加入对自旋锁的初始化.
2013.11.14  使用对象资源管理器结构管理空闲资源.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: _PartitionInit
** 功能描述: PARTITION 初始化
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _PartitionInit (VOID)
{
#if (LW_CFG_PARTITION_EN > 0) && (LW_CFG_MAX_PARTITIONS > 0)

#if  LW_CFG_MAX_PARTITIONS == 1

    REGISTER PLW_CLASS_PARTITION    p_partTemp1;

    _K_resrcPart.RESRC_pmonoFreeHeader = &_K__partBuffer[0].PARTITION_monoResrcList;
    
    p_partTemp1 = &_K__partBuffer[0];
    
    p_partTemp1->PARTITION_ucType  = LW_PARTITION_UNUSED;
    p_partTemp1->PARTITION_usIndex = 0;
    LW_SPIN_INIT(&p_partTemp1->PARTITION_slLock);
    
    _INIT_LIST_MONO_HEAD(_K_resrcPart.RESRC_pmonoFreeHeader);
    
    _K_resrcPart.RESRC_pmonoFreeTail = _K_resrcPart.RESRC_pmonoFreeHeader;
    
#else
    
    REGISTER ULONG                  ulI;
    REGISTER PLW_LIST_MONO          pmonoTemp1;
    REGISTER PLW_LIST_MONO          pmonoTemp2;
    REGISTER PLW_CLASS_PARTITION    p_partTemp1;
    REGISTER PLW_CLASS_PARTITION    p_partTemp2;
    
    _K_resrcPart.RESRC_pmonoFreeHeader = &_K__partBuffer[0].PARTITION_monoResrcList;
    
    p_partTemp1 = &_K__partBuffer[0];
    p_partTemp2 = &_K__partBuffer[1];
    
    for (ulI = 0; ulI < (LW_CFG_MAX_PARTITIONS - 1); ulI++) {
        p_partTemp1->PARTITION_ucType  = LW_PARTITION_UNUSED;
        p_partTemp1->PARTITION_usIndex = (UINT16)ulI;
        
        pmonoTemp1 = &p_partTemp1->PARTITION_monoResrcList;
        pmonoTemp2 = &p_partTemp2->PARTITION_monoResrcList;
        LW_SPIN_INIT(&p_partTemp1->PARTITION_slLock);
        
        _LIST_MONO_LINK(pmonoTemp1, pmonoTemp2);
        
        p_partTemp1++;
        p_partTemp2++;
    }
    
    p_partTemp1->PARTITION_ucType  = LW_PARTITION_UNUSED;
    p_partTemp1->PARTITION_usIndex = (UINT16)ulI;
    LW_SPIN_INIT(&p_partTemp1->PARTITION_slLock);
    
    pmonoTemp1 = &p_partTemp1->PARTITION_monoResrcList;
    
    _INIT_LIST_MONO_HEAD(pmonoTemp1);
    
    _K_resrcPart.RESRC_pmonoFreeTail = pmonoTemp1;
    
#endif                                                                  /*  LW_CFG_MAX_PARTITIONS == 1  */

    _K_resrcPart.RESRC_uiUsed    = 0;
    _K_resrcPart.RESRC_uiMaxUsed = 0;
    
#endif                                                                  /*  (LW_CFG_PARTITION_EN > 0)   */
                                                                        /*  (LW_CFG_MAX_PARTITIONS > 0) */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
