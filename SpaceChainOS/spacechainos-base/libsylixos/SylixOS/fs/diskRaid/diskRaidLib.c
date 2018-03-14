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
** 文   件   名: diskRaidLib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2014 年 06 月 08 日
**
** 描        述: 软件 RAID 磁盘阵列管理内部库.
**
** 注        意: 阵列中所有物理磁盘参数必须完全一致, 例如磁盘大小, 扇区大小等参数必须相同.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_DISKRAID_EN > 0)
/*********************************************************************************************************
** 函数名称: __diskRaidBytesPerSector
** 功能描述: 获得磁盘每扇区字节数
** 输　入  : pblkd              RAID 磁盘组
**           pulBytesPerSector  获取的磁盘每扇区字节数
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  __diskRaidBytesPerSector (PLW_BLK_DEV  pblkd, ULONG  *pulBytesPerSector)
{
    if (pblkd->BLKD_ulBytesPerSector) {
        *pulBytesPerSector = pblkd->BLKD_ulBytesPerSector;
    } else {
        if (!pblkd->BLKD_pfuncBlkIoctl) {
            return  (PX_ERROR);
        }
        if (pblkd->BLKD_pfuncBlkIoctl(pblkd, LW_BLKD_GET_SECSIZE, pulBytesPerSector) < 0) {
            return  (PX_ERROR);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __diskRaidBytesPerBlock
** 功能描述: 获得磁盘每块字节数
** 输　入  : pblkd              RAID 磁盘组
**           pulBytesPerBlock   获取的磁盘每扇区字节数
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  __diskRaidBytesPerBlock (PLW_BLK_DEV  pblkd, ULONG  *pulBytesPerBlock)
{
    if (pblkd->BLKD_ulBytesPerBlock) {
        *pulBytesPerBlock = pblkd->BLKD_ulBytesPerBlock;
    } else {
        if (!pblkd->BLKD_pfuncBlkIoctl) {
            return  (PX_ERROR);
        }
        if (pblkd->BLKD_pfuncBlkIoctl(pblkd, LW_BLKD_GET_BLKSIZE, pulBytesPerBlock) < 0) {
            return  (PX_ERROR);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __diskRaidTotalSector
** 功能描述: 获得磁盘总扇区数
** 输　入  : pblkd              RAID 磁盘组
**           pulTotalSector     获取的磁盘总扇区数
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  __diskRaidTotalSector (PLW_BLK_DEV  pblkd, ULONG  *pulTotalSector)
{
    if (pblkd->BLKD_ulNSector) {
        *pulTotalSector = pblkd->BLKD_ulNSector;
    } else {
        if (!pblkd->BLKD_pfuncBlkIoctl) {
            return  (PX_ERROR);
        }
        if (pblkd->BLKD_pfuncBlkIoctl(pblkd, LW_BLKD_GET_SECNUM, pulTotalSector) < 0) {
            return  (PX_ERROR);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __diskRaidCheck
** 功能描述: 检查一组磁盘是否满足 RAID 阵列的需求
** 输　入  : pblkd              RAID 磁盘组
**           uiNDisks           磁盘数量
**           pulBytesPerSector  获取的磁盘每扇区字节数
**           pulBytesPerBlock   获取的磁盘每块字节数
**           pulTotalSector     获取的磁盘总扇区数
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  __diskRaidCheck (PLW_BLK_DEV   pblkd[],
                      UINT          uiNDisks,
                      ULONG        *pulBytesPerSector,
                      ULONG        *pulBytesPerBlock,
                      ULONG        *pulTotalSector)
{
    INT    i;
    ULONG  ulBytes0, ulTotal0, ulBlkSize;
    ULONG  ulBytes1, ulTotal1;

    if (__diskRaidBytesPerSector(pblkd[0], &ulBytes0)) {
        return  (PX_ERROR);
    }

    if (__diskRaidTotalSector(pblkd[0], &ulTotal0)) {
        return  (PX_ERROR);
    }

    if (__diskRaidBytesPerBlock(pblkd[0], &ulBlkSize)) {
        return  (PX_ERROR);
    }

    for (i = 1; i < uiNDisks; i++) {
        if (__diskRaidBytesPerSector(pblkd[i], &ulBytes1)) {
            return  (PX_ERROR);
        }

        if (__diskRaidTotalSector(pblkd[i], &ulTotal1)) {
            return  (PX_ERROR);
        }

        if ((ulBytes0 != ulBytes1) || (ulTotal0 != ulTotal1)) {
            return  (PX_ERROR);
        }
    }

    if (pulBytesPerSector) {
        *pulBytesPerSector = ulBytes0;
    }

    if (pulBytesPerBlock) {
        *pulBytesPerBlock = ulBlkSize;
    }

    if (pulTotalSector) {
        *pulTotalSector = ulTotal0;
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_DISKRAID_EN > 0)    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
