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
** 文   件   名: diskRaidLib.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2014 年 06 月 08 日
**
** 描        述: 软件 RAID 磁盘阵列管理内部库.
**
** 注        意: 阵列中所有物理磁盘参数必须完全一致, 例如磁盘大小, 扇区大小等参数必须相同.
*********************************************************************************************************/

#ifndef __DISKRAIDLIB_H
#define __DISKRAIDLIB_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_DISKRAID_EN > 0)

INT  __diskRaidBytesPerSector(PLW_BLK_DEV  pblkd, ULONG  *pulBytesPerSector);
INT  __diskRaidBytesPerBlock(PLW_BLK_DEV  pblkd, ULONG  *pulBytesPerBlock);
INT  __diskRaidTotalSector(PLW_BLK_DEV  pblkd, ULONG  *pulTotalSector);
INT  __diskRaidCheck(PLW_BLK_DEV   pblkd[],
                     UINT          uiNDisks,
                     ULONG        *pulBytesPerSector,
                     ULONG        *pulBytesPerBlock,
                     ULONG        *pulTotalSector);

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_DISKRAID_EN > 0)    */
#endif                                                                  /*  __DISKRAIDLIB_H             */
/*********************************************************************************************************
  END
*********************************************************************************************************/
