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
** 文   件   名: PartitionShow.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 04 月 01 日
**
** 描        述: 显示指定的内存分区信息.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_FIO_LIB_EN > 0
#if (LW_CFG_PARTITION_EN > 0) && (LW_CFG_MAX_PARTITIONS > 0)
/*********************************************************************************************************
** 函数名称: API_PartitionShow
** 功能描述: 显示指定的内存分区信息
** 输　入  : ulId      内存分区句柄
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API  
VOID   API_PartitionShow (LW_OBJECT_HANDLE  ulId)
{
    CHAR        cPartitionName[LW_CFG_OBJECT_NAME_SIZE];
    
    ULONG       ulBlockCounter;
    ULONG       ulFreeBlockCounter;
    size_t      stBlockByteSize;
    
    if (API_PartitionGetName(ulId, cPartitionName)) {
        return;
    }
    
    if (API_PartitionStatus(ulId,
                            &ulBlockCounter,
                            &ulFreeBlockCounter,
                            &stBlockByteSize)) {
        return;
    }
    
    printf("partition show >>\n\n");
    printf("partition name           : %s\n",    cPartitionName);
    printf("partition block number   : %11lu\n", ulBlockCounter);
    printf("partition free block     : %11lu\n", ulFreeBlockCounter);
    printf("partition per block size : %11zd\n", stBlockByteSize);
}

#endif                                                                  /*  LW_CFG_PARTITION_EN > 0     */
                                                                        /*  LW_CFG_MAX_PARTITIONS > 0   */
#endif                                                                  /*  LW_CFG_FIO_LIB_EN           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
