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
** 文   件   名: oemDisk.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 03 月 24 日
**
** 描        述: OEM 自动磁盘管理. 
                 由于多分区磁盘挂载, 控制, 卸载, 内存回收方面使用 API 过多, 操作繁琐, 这里将这些操作封装
                 为一个 OEM 磁盘操作库, 方便使用.
                 
** BUG:
2012.09.01 加入 OEMDISK_pdevhdr 卸载时需要验证设备是否为自己 mount 的设备.
*********************************************************************************************************/

#ifndef __OEMDISK_H
#define __OEMDISK_H

/*********************************************************************************************************
  提示: 使用 OEM 磁盘操作, 可以大大降低多分区磁盘的管理难度.
  
  例如: 一个 CF 卡检测任务:
  
  PLW_OEMDISK_CB    oemdCf;
  
  for (;;) {
      if (检测到卡插入) {
          oemdCf = oemDiskMount(...);
          ...等待卡拔出...
          oemDiskUnmountEx(oemdCf, FALSE);
      }
      
      sleep(...);
  }
  
  以上范例代码仅说明 OEM DISK 管理的流程, 当然 SylixOS 还提供了一整套热插拔管理机制, 不需要使用如上代码,
  
  热插拔系统接口使用方法详见 hotplug 模块说明.
  
  当相关的卷已经挂载完成后, 可使用 oemDiskHotplugEventMessage 将相关的热插拔消息发送给感兴趣的应用程序.
  
  注意:
  
  1: 推荐使用 oemDiskUnmountEx(), 而且不要强制卸载卷, 如果有文件打开, 强行卸载卷是非常危险的.
  
  2: 如果硬件驱动使用 DMA 则必须使用 DISK CACHE, DISK CACHE 内部缓冲区有针对 CPU CACHE 在对齐上的处理.
*********************************************************************************************************/

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if LW_CFG_OEMDISK_EN > 0
/*********************************************************************************************************
  OEM 磁盘控制块
*********************************************************************************************************/
typedef struct {
    PLW_BLK_DEV          OEMDISK_pblkdDisk;                             /*  物理磁盘驱动                */
    PLW_BLK_DEV          OEMDISK_pblkdCache;                            /*  CACHE 驱动块                */
    PLW_BLK_DEV          OEMDISK_pblkdPart[LW_CFG_MAX_DISKPARTS];       /*  各分区驱动块                */
    INT                  OEMDISK_iVolSeq[LW_CFG_MAX_DISKPARTS];         /*  对应个分区的卷序号          */
    PLW_DEV_HDR          OEMDISK_pdevhdr[LW_CFG_MAX_DISKPARTS];         /*  安装后的设备头              */
    PVOID                OEMDISK_pvCache;                               /*  自动分配内存地址            */
    UINT                 OEMDISK_uiNPart;                               /*  分区数                      */
    INT                  OEMDISK_iBlkNo;                                /*  /dev/blk/? 设备号           */
    CHAR                 OEMDISK_cVolName[1];                           /*  磁盘根挂载节点名            */
} LW_OEMDISK_CB;
typedef LW_OEMDISK_CB   *PLW_OEMDISK_CB;
/*********************************************************************************************************
  API
*********************************************************************************************************/
LW_API VOID              API_OemDiskMountInit(VOID);
LW_API VOID              API_OemDiskMountShow(VOID);

LW_API PLW_OEMDISK_CB    API_OemDiskMount(CPCHAR        pcVolName,
                                          PLW_BLK_DEV   pblkdDisk,
                                          PVOID         pvDiskCacheMem, 
                                          size_t        stMemSize, 
                                          INT           iMaxBurstSector);
LW_API PLW_OEMDISK_CB    API_OemDiskMount2(CPCHAR             pcVolName,
                                           PLW_BLK_DEV        pblkdDisk,
                                           PLW_DISKCACHE_ATTR pdcattrl);
                                          
LW_API PLW_OEMDISK_CB    API_OemDiskMountEx(CPCHAR        pcVolName,
                                            PLW_BLK_DEV   pblkdDisk,
                                            PVOID         pvDiskCacheMem, 
                                            size_t        stMemSize, 
                                            INT           iMaxBurstSector,
                                            CPCHAR        pcFsName,
                                            BOOL          bForceFsType);
LW_API PLW_OEMDISK_CB    API_OemDiskMountEx2(CPCHAR             pcVolName,
                                             PLW_BLK_DEV        pblkdDisk,
                                             PLW_DISKCACHE_ATTR pdcattrl,
                                             CPCHAR             pcFsName,
                                             BOOL               bForceFsType);
                                            
LW_API INT               API_OemDiskUnmount(PLW_OEMDISK_CB  poemd);
LW_API INT               API_OemDiskUnmountEx(PLW_OEMDISK_CB  poemd, BOOL  bForce);

LW_API INT               API_OemDiskRemount(PLW_OEMDISK_CB  poemd);
LW_API INT               API_OemDiskRemountEx(PLW_OEMDISK_CB  poemd, BOOL  bForce);

LW_API INT               API_OemDiskGetPath(PLW_OEMDISK_CB  poemd, INT  iIndex, 
                                            PCHAR  pcPath, size_t stSize);
                                            
#if LW_CFG_HOTPLUG_EN > 0
LW_API INT               API_OemDiskHotplugEventMessage(PLW_OEMDISK_CB  poemd, 
                                                        INT             iMsg, 
                                                        BOOL            bInsert,
                                                        UINT32          uiArg0,
                                                        UINT32          uiArg1,
                                                        UINT32          uiArg2,
                                                        UINT32          uiArg3);
#endif                                                                  /*  LW_CFG_HOTPLUG_EN > 0       */

#define oemDiskMountShow            API_OemDiskMountShow

#define oemDiskMount                API_OemDiskMount
#define oemDiskMount2               API_OemDiskMount2
#define oemDiskMountEx              API_OemDiskMountEx
#define oemDiskMountEx2             API_OemDiskMountEx2

#define oemDiskUnmount              API_OemDiskUnmount
#define oemDiskUnmountEx            API_OemDiskUnmountEx

#define oemDiskRemount              API_OemDiskRemount
#define oemDiskRemountEx            API_OemDiskRemountEx

#define oemDiskGetPath              API_OemDiskGetPath
#define oemDiskHotplugEventMessage  API_OemDiskHotplugEventMessage

#endif                                                                  /*  LW_CFG_OEMDISK_EN > 0       */
#endif                                                                  /*  __OEMDISK_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
