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
** 文   件   名: diskPartition.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 03 月 17 日
**
** 描        述: 磁盘分区表分析. (注意: 必须使能 FAT 文件系统管理).
*********************************************************************************************************/

#ifndef __DISKPARTITION_H
#define __DISKPARTITION_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_DISKPART_EN > 0)
/*********************************************************************************************************
  磁盘分区的使用示例: (以下没有判断错误状况)
  
  int                   i, partcnt;
  
  BLK_DEV               hdd;
  BLK_DEV               cache;
  BLK_DEV              *phddpart[...];
  
  DISKPART_TABLE        part;
  char                  cVolName[16];
  
  hdd = ataDiskCreate(...);                         ->创建 HDD 设备
  diskCacheCreate(hdd, ..., &cache);                ->创建设备 CACHE
  partcnt  = diskPartitionScan(cache, &part);       ->分析设备分区表
  
  for (i = 0; i < partcnt; i++) {                   ->挂载磁盘各个分区
      diskPartitionGet(&part, i, &phddpart[i]);
      sprintf(cVolName, "/hdd%d", i);
      fatFsDevCreate(cVolName, phddpart[i]);
  }
  
  ... (如果检测到需要设备移除)
  
  for (i = 0; i < partcnt; i++) {
      sprintf(cVolName, "/hdd%d", i);
      unlink(cVolName);
      diskPartitionFree(phddpart[i]);
  }
  diskCacheDelete(cache);
  ataDiskDelete(hdd);
  
  
  注意:
  diskPartitionScan 返回 -1 或者 0 , 表明设备不存在磁盘分区表,
  直接使用 fatFsDevCreate(...) 装载卷即可.
*********************************************************************************************************/
/*********************************************************************************************************
  分区类型 (格式分区)
*********************************************************************************************************/
#define LW_DISK_PART_TYPE_EMPTY                 0x00
#define LW_DISK_PART_TYPE_FAT12                 0x01
#define LW_DISK_PART_TYPE_FAT16                 0x04
#define LW_DISK_PART_TYPE_FAT16_BIG             0x06
#define LW_DISK_PART_TYPE_HPFS_NTFS             0x07
#define LW_DISK_PART_TYPE_WIN95_FAT32           0x0b
#define LW_DISK_PART_TYPE_WIN95_FAT32LBA        0x0c
#define LW_DISK_PART_TYPE_WIN95_FAT16LBA        0x0e
#define LW_DISK_PART_TYPE_NATIVE_LINUX          0x83
#define LW_DISK_PART_TYPE_QNX4_1                0x4d
#define LW_DISK_PART_TYPE_QNX4_2                0x4e
#define LW_DISK_PART_TYPE_QNX4_3                0x4f
#define LW_DISK_PART_TYPE_QNX6_1                0xb1
#define LW_DISK_PART_TYPE_QNX6_2                0xb2
#define LW_DISK_PART_TYPE_QNX6_3                0xb3
#define LW_DISK_PART_TYPE_TPS                   0x9c
#define LW_DISK_PART_TYPE_RESERVED              0xbf
/*********************************************************************************************************
  分区类型 (扩展分区)
*********************************************************************************************************/
#define LW_DISK_PART_TYPE_EXTENDED              0x05
#define LW_DISK_PART_TYPE_WIN95_EXTENDED        0x0f
#define LW_DISK_PART_TYPE_HIDDEN_EXTENDED       0x15
#define LW_DISK_PART_TYPE_HIDDEN_LBA_EXTENDED   0x1f
#define LW_DISK_PART_TYPE_LINUX_EXTENDED        0x85
/*********************************************************************************************************
  分区类型判断
*********************************************************************************************************/
#define LW_DISK_PART_IS_EXTENDED(type)          (((type) == LW_DISK_PART_TYPE_EXTENDED)            || \
                                                 ((type) == LW_DISK_PART_TYPE_WIN95_EXTENDED)      || \
                                                 ((type) == LW_DISK_PART_TYPE_HIDDEN_EXTENDED)     || \
                                                 ((type) == LW_DISK_PART_TYPE_HIDDEN_LBA_EXTENDED) || \
                                                 ((type) == LW_DISK_PART_TYPE_LINUX_EXTENDED))
/*********************************************************************************************************
  分区类型有效
*********************************************************************************************************/
#define LW_DISK_PART_IS_VALID(type)             ((!LW_DISK_PART_IS_EXTENDED(type)) && \
                                                 ((type) != LW_DISK_PART_TYPE_EMPTY))
/*********************************************************************************************************
  单个分区信息
*********************************************************************************************************/
typedef struct {
    ULONG                    DPN_ulStartSector;                         /*  分区起始扇区                */
    ULONG                    DPN_ulNSector;                             /*  分区大小                    */
    BOOL                     DPN_bIsActive;                             /*  是否为活动分区              */
    BYTE                     DPN_ucPartType;                            /*  分区类型                    */
} LW_DISKPART_NODE;
/*********************************************************************************************************
  逻辑分区操作控制块
*********************************************************************************************************/
typedef struct {
    LW_BLK_DEV               DPO_blkdLogic;                             /*  磁盘操作函数                */
    LW_DISKPART_NODE         DPO_dpnEntry;                              /*  分区区域信息                */
    PLW_BLK_DEV              DPT_pblkdDisk;                             /*  物理设备操作控制块          */
} LW_DISKPART_OPERAT;
/*********************************************************************************************************
  物理磁盘分区表
*********************************************************************************************************/
typedef struct {
    UINT                     DPT_ulNPart;                               /*  分区数量                    */
    LW_DISKPART_OPERAT       DPT_dpoLogic[LW_CFG_MAX_DISKPARTS];        /*  分区操作控制块              */
} LW_DISKPART_TABLE;
typedef LW_DISKPART_TABLE   *PLW_DISKPART_TABLE;
typedef LW_DISKPART_TABLE    DISKPART_TABLE;
/*********************************************************************************************************
  API
*********************************************************************************************************/
LW_API INT  API_DiskPartitionScan(PLW_BLK_DEV  pblkd, PLW_DISKPART_TABLE  pdptDisk);
LW_API INT  API_DiskPartitionGet(PLW_DISKPART_TABLE  pdptDisk, UINT  uiPart, PLW_BLK_DEV  *ppblkdLogic);
LW_API INT  API_DiskPartitionFree(PLW_BLK_DEV  pblkdLogic);
LW_API INT  API_DiskPartitionLinkNumGet(PLW_BLK_DEV  pblkdPhysical);

#define diskPartitionScan           API_DiskPartitionScan
#define diskPartitionGet            API_DiskPartitionGet
#define diskPartitionFree           API_DiskPartitionFree
#define diskPartitionLinkNumGet     API_DiskPartitionLinkNumGet

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_DISKPART_EN > 0)    */
#endif                                                                  /*  __DISKPARTITION_H           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
