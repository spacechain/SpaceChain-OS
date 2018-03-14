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
** 文   件   名: PartitionStatus.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 01 月 17 日
**
** 描        述: 查询一个内存分区的状态

** BUG
2008.01.13  加入 _DebugHandle() 功能。
2009.04.08  加入对 SMP 多核的支持.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_PartitionStatus
** 功能描述: 查询一个内存分区的状态
** 输　入  : 
**           ulId                         PARTITION 句柄
**           pulBlockCounter              块数量指针       可以为 NULL
**           pulFreeBlockCounter          空闲块数量指针   可以为 NULL
**           pstBlockByteSize             块大小           可以为 NULL
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if (LW_CFG_PARTITION_EN > 0) && (LW_CFG_MAX_PARTITIONS > 0)

LW_API  
ULONG  API_PartitionStatus (LW_OBJECT_HANDLE    ulId,
                            ULONG              *pulBlockCounter,
                            ULONG              *pulFreeBlockCounter,
                            size_t             *pstBlockByteSize)
{
             INTREG                    iregInterLevel;
    REGISTER PLW_CLASS_PARTITION       p_part;
    REGISTER UINT16                    usIndex;
    
    usIndex = _ObjectGetIndex(ulId);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_PARTITION)) {                     /*  对象类型检查                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "partition handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_Partition_Index_Invalid(usIndex)) {                            /*  缓冲区索引检查              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "partition handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
#endif
    p_part = &_K__partBuffer[usIndex];
    
    LW_SPIN_LOCK_QUICK(&p_part->PARTITION_slLock, &iregInterLevel);     /*  关闭中断同时锁住 spinlock   */
    
    if (_Partition_Type_Invalid(usIndex)) {
        LW_SPIN_UNLOCK_QUICK(&p_part->PARTITION_slLock, 
                             iregInterLevel);                           /*  打开中断, 同时打开 spinlock */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "partition handle invalidate.\r\n");
        _ErrorHandle(ERROR_PARTITION_NULL);
        return  (ERROR_PARTITION_NULL);
    }
    
    if (pulBlockCounter) {
        *pulBlockCounter = p_part->PARTITION_ulBlockCounter;
    }
    
    if (pulFreeBlockCounter) {
        *pulFreeBlockCounter = p_part->PARTITION_ulFreeBlockCounter;
    }
    
    if (pstBlockByteSize) {
        *pstBlockByteSize = p_part->PARTITION_stBlockByteSize;
    }
    
    LW_SPIN_UNLOCK_QUICK(&p_part->PARTITION_slLock, iregInterLevel);    /*  打开中断, 同时打开 spinlock */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_PARTITION_EN > 0)   */
                                                                        /*  (LW_CFG_MAX_PARTITIONS > 0) */
/*********************************************************************************************************
  END
*********************************************************************************************************/
