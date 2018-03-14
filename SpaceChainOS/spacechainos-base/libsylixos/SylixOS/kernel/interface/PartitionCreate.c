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
** 文   件   名: PartitionCreate.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 01 月 11 日
**
** 描        述: 建立一个内存分区

** BUG
2007.08.29  加入对非对齐地址的检查功能。
2008.01.13  加入 _DebugHandle() 功能.
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock();
2008.05.31  使用 __KERNEL_MODE_...().
2009.04.08  加入对 SMP 多核的支持.
2009.07.28  自旋锁的初始化放在初始化所有的控制块中, 这里去除相关操作.
2011.07.29  加入对象创建/销毁回调.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_PartitionCreate
** 功能描述: 建立一个内存分区
** 输　入  : 
**           pcName                        名字
**           pvLowAddr                     内存块起始地址
**           ulBlockCounter                内存块个数 
**           stBlockByteSize               内存块大小
**           ulOption                      选项
**           pulId                         Id 号
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
#if (LW_CFG_PARTITION_EN > 0) && (LW_CFG_MAX_PARTITIONS > 0)

LW_API  
LW_OBJECT_HANDLE  API_PartitionCreate (CPCHAR             pcName,
                                       PVOID              pvLowAddr,
                                       ULONG              ulBlockCounter,
                                       size_t             stBlockByteSize,
                                       ULONG              ulOption,
                                       LW_OBJECT_ID      *pulId)
{
    REGISTER PLW_CLASS_PARTITION   p_part;
    REGISTER ULONG                 ulIdTemp;
    REGISTER ULONG                 ulI;

    REGISTER PLW_LIST_MONO         pmonoTemp1;
    REGISTER PLW_LIST_MONO         pmonoTemp2;
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!pvLowAddr) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pvLowAddr invalidate.\r\n");
        _ErrorHandle(ERROR_PARTITION_NULL);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    if (!_Addresses_Is_Aligned(pvLowAddr)) {                            /*  检查地址是否对齐            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pvLowAddr is not aligned.\r\n");
        _ErrorHandle(ERROR_KERNEL_MEMORY);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    if (ulBlockCounter == 0ul) {                                        /*  至少有两个分块              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "ulBlockCounter invalidate.\r\n");
        _ErrorHandle(ERROR_PARTITION_BLOCK_COUNTER);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    if (stBlockByteSize < sizeof(LW_LIST_MONO)) {                       /*  分段太小                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "ulBockByteSize is too low.\r\n");
        _ErrorHandle(ERROR_PARTITION_BLOCK_SIZE);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
#endif
    
    if (_Object_Name_Invalid(pcName)) {                                 /*  检查名字有效性              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name too long.\r\n");
        _ErrorHandle(ERROR_KERNEL_PNAME_TOO_LONG);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    __KERNEL_MODE_PROC(
        p_part = _Allocate_Partition_Object();                          /*  获得一个分区控制块          */
    );
    
    if (!p_part) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "there is no ID to buide a partition.\r\n");
        _ErrorHandle(ERROR_PARTITION_FULL);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    if (pcName) {                                                       /*  拷贝名字                    */
        lib_strcpy(p_part->PARTITION_cPatitionName, pcName);
    } else {
        p_part->PARTITION_cPatitionName[0] = PX_EOS;                    /*  清空名字                    */
    }
    
    p_part->PARTITION_ucType              = LW_PARTITION_USED;          /*  类型标志                    */
    p_part->PARTITION_pmonoFreeBlockList  = (PLW_LIST_MONO)pvLowAddr;   /*  空闲内存块表头              */
    p_part->PARTITION_stBlockByteSize     = stBlockByteSize;            /*  每一块的大小                */
    p_part->PARTITION_ulBlockCounter      = ulBlockCounter;             /*  块数量                      */
    p_part->PARTITION_ulFreeBlockCounter  = ulBlockCounter;             /*  空闲块数量                  */
    
    pmonoTemp1 = (PLW_LIST_MONO)pvLowAddr;
    pmonoTemp2 = (PLW_LIST_MONO)((UINT8 *)pvLowAddr + stBlockByteSize);
    
    for (ulI = 0; ulI < (ulBlockCounter - 1); ulI++) {                  /*  块连接                      */
        _LIST_MONO_LINK(pmonoTemp1, pmonoTemp2);
        
        pmonoTemp1 = (PLW_LIST_MONO)((UINT8 *)pmonoTemp1 + stBlockByteSize);
        pmonoTemp2 = (PLW_LIST_MONO)((UINT8 *)pmonoTemp2 + stBlockByteSize);
    }
    
    _INIT_LIST_MONO_HEAD(pmonoTemp1);                                   /*  最后一个块                  */
    
    ulIdTemp = _MakeObjectId(_OBJECT_PARTITION, 
                             LW_CFG_PROCESSOR_NUMBER, 
                             p_part->PARTITION_usIndex);                /*  构建对象 id                 */
    
    if (pulId) {
        *pulId = ulIdTemp;
    }
    
    __LW_OBJECT_CREATE_HOOK(ulIdTemp, ulOption);
    
    MONITOR_EVT_LONG4(MONITOR_EVENT_ID_PART, MONITOR_EVENT_PART_CREATE,
                      ulIdTemp, pvLowAddr, ulBlockCounter, stBlockByteSize, LW_NULL);
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "partition \"%s\" has been create.\r\n", (pcName ? pcName : ""));
    
    return  (ulIdTemp);
}

#endif                                                                  /*  (LW_CFG_PARTITION_EN > 0)   */
                                                                        /*  (LW_CFG_MAX_PARTITIONS > 0) */
/*********************************************************************************************************
  END
*********************************************************************************************************/
