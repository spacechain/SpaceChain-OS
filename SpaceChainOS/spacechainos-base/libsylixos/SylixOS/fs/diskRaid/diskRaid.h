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
** 文   件   名: diskRaid.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2014 年 06 月 06 日
**
** 描        述: 软件 RAID 磁盘阵列管理.
*********************************************************************************************************/

#ifndef __DISKRAID_H
#define __DISKRAID_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_DISKRAID_EN > 0)

LW_API ULONG    API_DiskRiad0Create(PLW_BLK_DEV  pblkd[],
                                    UINT         uiNDisks,
                                    size_t       stStripe,
                                    PLW_BLK_DEV *ppblkDiskRaid);

LW_API INT      API_DiskRiad0Delete(PLW_BLK_DEV  pblkDiskRaid);

LW_API ULONG    API_DiskRiad1Create(PLW_BLK_DEV  pblkd[],
                                    UINT         uiNDisks,
                                    PLW_BLK_DEV *ppblkDiskRaid);

LW_API INT      API_DiskRiad1Delete(PLW_BLK_DEV  pblkDiskRaid);

LW_API ULONG    API_DiskRiad1Ghost(PLW_BLK_DEV  pblkDest,
                                   PLW_BLK_DEV  pblkSrc,
                                   ULONG        ulStartSector,
                                   ULONG        ulSectorNum);

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_DISKRAID_EN > 0)    */
#endif                                                                  /*  __DISKRAID_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
