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
** 文   件   名: PartitionDelete.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 01 月 17 日
**
** 描        述: 删除一个内存分区

** BUG
2008.01.13  加入 _DebugHandle() 功能。
2009.04.08  加入对 SMP 多核的支持.
2011.07.29  加入对象创建/销毁回调.
2012.12.06  加入强制删除的 API 用于资源回收.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_PartitionDeleteEx
** 功能描述: 删除一个内存分区
** 输　入  : 
**           pulId                         PARTITION 句柄指针
**           bForce                        是否强制删除
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
#if (LW_CFG_PARTITION_EN > 0) && (LW_CFG_MAX_PARTITIONS > 0)

LW_API  
ULONG  API_PartitionDeleteEx (LW_OBJECT_HANDLE   *pulId, BOOL  bForce)
{
             INTREG                    iregInterLevel;
    REGISTER PLW_CLASS_PARTITION       p_part;
    REGISTER UINT16                    usIndex;
    REGISTER LW_OBJECT_HANDLE          ulId;
    
    ulId = *pulId;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
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
    
    if (!bForce) {
        if (p_part->PARTITION_ulBlockCounter != 
            p_part->PARTITION_ulFreeBlockCounter) {                     /*  是否有分段被使用            */
            LW_SPIN_UNLOCK_QUICK(&p_part->PARTITION_slLock, 
                                 iregInterLevel);                       /*  打开中断, 同时打开 spinlock */
            _ErrorHandle(ERROR_PARTITION_BLOCK_USED);
            return  (ERROR_PARTITION_BLOCK_USED);
        }
    }
    
    _ObjectCloseId(pulId);                                              /*  关闭句柄                    */
    
    p_part->PARTITION_ucType = LW_PARTITION_UNUSED;
    
    LW_SPIN_UNLOCK_QUICK(&p_part->PARTITION_slLock, iregInterLevel);    /*  打开中断, 同时打开 spinlock */
    
    __LW_OBJECT_DELETE_HOOK(ulId);
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "partition \"%s\" has been delete.\r\n", 
                 p_part->PARTITION_cPatitionName);
    
    __KERNEL_MODE_PROC(
        _Free_Partition_Object(p_part);                                 /*  交还控制块                  */
    );
    
    MONITOR_EVT_LONG1(MONITOR_EVENT_ID_PART, MONITOR_EVENT_REGION_DELETE, ulId, LW_NULL);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PartitionDelete
** 功能描述: 删除一个内存分区
** 输　入  : 
**           pulId                         PARTITION 句柄指针
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API  
ULONG  API_PartitionDelete (LW_OBJECT_HANDLE   *pulId)
{
    return  (API_PartitionDeleteEx(pulId, LW_FALSE));
}

#endif                                                                  /*  (LW_CFG_PARTITION_EN > 0)   */
                                                                        /*  (LW_CFG_MAX_PARTITIONS > 0) */
/*********************************************************************************************************
  END
*********************************************************************************************************/
