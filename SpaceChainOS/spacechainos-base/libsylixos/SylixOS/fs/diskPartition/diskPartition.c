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
** 文   件   名: diskPartition.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 03 月 17 日
**
** 描        述: 磁盘分区表分析. (注意: 必须使能 FAT 文件系统管理).

** BUG:
2009.06.10  API_DiskPartitionScan() 在分析分区表失败时, 逻辑磁盘的 iNSector 成员需要赋值为整个物理磁盘的
            大小.
2009.06.19  优化一处代码结构.
2009.07.06  物理磁盘 Link 计数器不在分区管理进行.
2009.11.09  将分区类型宏定义放在头文件中.
2009.12.01  当无法分析分区表时, 将分区类型设置为 LW_DISK_PART_TYPE_EMPTY 标志.
2011.11.21  升级文件系统, 这里自主定义 MBR_Table.
2015.10.21  更新块设备操作接口.
2016.03.23  ioctl, FIOTRIM 与 FIOSYNCMETA 命令需要转换为物理磁盘扇区号.
2016.07.12  磁盘写操作加入一个文件系统层参数.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/include/fs_internal.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_DISKPART_EN > 0)
#include "diskPartition.h"
/*********************************************************************************************************
  一个物理磁盘带有五个逻辑分区的 BLK_DEV 文件系统示例结构:

logic HDD FAT disk volume:
    +--------+          +--------+          +--------+          +--------+          +--------+
    | /hdd0  |          | /hdd1  |          | /hdd2  |          | /hdd3  |          | /hdd4  |
    |  FAT32 |          |  FAT16 |          |  TPSFS |          |  TPSFS |          |  NTFS  |
    +--------+          +--------+          +--------+          +--------+          +--------+
        |                   |                   |                   |                   |
        |                   |                   |                   |                   |
logic disk block device:    |                   |                   |                   |
    +--------+          +--------+          +--------+          +--------+          +--------+
    | BLK_DEV|          | BLK_DEV|          | BLK_DEV|          | BLK_DEV|          | BLK_DEV|
    +--------+          +--------+          +--------+          +--------+          +--------+
        \                   \                   |                  /                   /
         \___________________\__________________|_________________/___________________/
                                                |
                                                |
                                           +----------+
disk cache block device:                   |  dcache  |
                                           |  BLK_DEV |
                                           +----------+
                                                |
                                                |
                                           +----------+
physical disk block device:                | physical |
                                           |  BLK_DEV |
                                           +----------+
                                                |
                                                |
                                           +----------+
physical HDD:                              |   HDD    |
                                           +----------+
*********************************************************************************************************/
/*********************************************************************************************************
  分区活动标志
*********************************************************************************************************/
#define __DISK_PART_ACTIVE                  0x80                        /*  活动引导分区                */
#define __DISK_PART_UNACTIVE                0x00                        /*  非活动分区                  */
/*********************************************************************************************************
  分区表中重要信息地点
*********************************************************************************************************/
#define __DISK_PART_TYPE                    0x4
#define __DISK_PART_STARTSECTOR             0x8
#define __DISK_PART_NSECTOR                 0xc
/*********************************************************************************************************
  宏操作
*********************************************************************************************************/
#define BLK_READ(pblk, buf, sec, num)   (pblk)->BLKD_pfuncBlkRd((pblk), (buf), (sec), (num))
#define BLK_RESET(pblk)                 (pblk)->BLKD_pfuncBlkReset((pblk))
#define BLK_IOCTL(pblk, cmd, arg)       (pblk)->BLKD_pfuncBlkIoctl((pblk), (cmd), (arg))
/*********************************************************************************************************
** 函数名称: __logicDiskWrt
** 功能描述: 写单分区逻辑磁盘
** 输　入  : pdpoLogic         逻辑磁盘控制块
**           pvBuffer          缓冲区
**           ulStartSector     起始扇区号
**           ulSectorCount     扇区数量
**           u64FsKey          文件系统特定数据
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __logicDiskWrt (LW_DISKPART_OPERAT    *pdpoLogic, 
                            VOID                  *pvBuffer, 
                            ULONG                  ulStartSector, 
                            ULONG                  ulSectorCount,
                            UINT64                 u64FsKey)
{
    ulStartSector += pdpoLogic->DPO_dpnEntry.DPN_ulStartSector;         /*  逻辑分区偏移量              */
    
    return  (pdpoLogic->DPT_pblkdDisk->BLKD_pfuncBlkWrt(pdpoLogic->DPT_pblkdDisk, 
                                                        pvBuffer,
                                                        ulStartSector,
                                                        ulSectorCount,
                                                        u64FsKey));
}
/*********************************************************************************************************
** 函数名称: __logicDiskRd
** 功能描述: 读单分区逻辑磁盘
** 输　入  : pdpoLogic         逻辑磁盘控制块
**           pvBuffer          缓冲区
**           ulStartSector     起始扇区号
**           ulSectorCount     扇区数量
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __logicDiskRd (LW_DISKPART_OPERAT    *pdpoLogic,
                           VOID                  *pvBuffer, 
                           ULONG                  ulStartSector, 
                           ULONG                  ulSectorCount)
{
    ulStartSector += pdpoLogic->DPO_dpnEntry.DPN_ulStartSector;         /*  逻辑分区偏移量              */
    
    return  (pdpoLogic->DPT_pblkdDisk->BLKD_pfuncBlkRd(pdpoLogic->DPT_pblkdDisk, 
                                                       pvBuffer,
                                                       ulStartSector,
                                                       ulSectorCount));
}
/*********************************************************************************************************
** 函数名称: __logicDiskIoctl
** 功能描述: 控制单分区逻辑磁盘
** 输　入  : pdpoLogic         逻辑磁盘控制块
**           iCmd              控制命令
**           lArg              控制参数
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __logicDiskIoctl (LW_DISKPART_OPERAT    *pdpoLogic, INT  iCmd, LONG  lArg)
{
    PLW_BLK_RANGE  pblkrLogic;
    LW_BLK_RANGE   blkrPhy;

    switch (iCmd) {
    
    case FIOTRIM:
    case FIOSYNCMETA:
        pblkrLogic = (PLW_BLK_RANGE)lArg;
        blkrPhy.BLKR_ulStartSector = pblkrLogic->BLKR_ulStartSector 
                                   + pdpoLogic->DPO_dpnEntry.DPN_ulStartSector;
        blkrPhy.BLKR_ulEndSector   = pblkrLogic->BLKR_ulEndSector
                                   + pdpoLogic->DPO_dpnEntry.DPN_ulStartSector;
        return  (pdpoLogic->DPT_pblkdDisk->BLKD_pfuncBlkIoctl(pdpoLogic->DPT_pblkdDisk, 
                                                              iCmd,
                                                              &blkrPhy));
                                                              
    default:
        return  (pdpoLogic->DPT_pblkdDisk->BLKD_pfuncBlkIoctl(pdpoLogic->DPT_pblkdDisk, 
                                                              iCmd,
                                                              lArg));
    }
}
/*********************************************************************************************************
** 函数名称: __logicDiskReset
** 功能描述: 复位单分区逻辑磁盘 (当前系统不会调用到这里)
** 输　入  : pdpoLogic         逻辑磁盘控制块
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __logicDiskReset (LW_DISKPART_OPERAT    *pdpoLogic)
{
    return  (pdpoLogic->DPT_pblkdDisk->BLKD_pfuncBlkReset(pdpoLogic->DPT_pblkdDisk));
}
/*********************************************************************************************************
** 函数名称: __logicDiskStatusChk
** 功能描述: 检测单分区逻辑磁盘
** 输　入  : pdpoLogic         逻辑磁盘控制块
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __logicDiskStatusChk (LW_DISKPART_OPERAT    *pdpoLogic)
{
    return  (pdpoLogic->DPT_pblkdDisk->BLKD_pfuncBlkStatusChk(pdpoLogic->DPT_pblkdDisk));
}
/*********************************************************************************************************
** 函数名称: __logicDiskInit
** 功能描述: 初始化逻辑磁盘控制块
** 输　入  : pblkdLogic        逻辑磁盘设备
**           pblkdPhysical     物理磁盘设备
**           ulNSector         磁盘扇区数量
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __logicDiskInit (PLW_BLK_DEV  pblkdLogic, PLW_BLK_DEV  pblkdPhysical, ULONG  ulNSector)
{
    pblkdLogic->BLKD_pfuncBlkRd        = __logicDiskRd;
    pblkdLogic->BLKD_pfuncBlkWrt       = __logicDiskWrt;
    pblkdLogic->BLKD_pfuncBlkIoctl     = __logicDiskIoctl;
    pblkdLogic->BLKD_pfuncBlkReset     = __logicDiskReset;
    pblkdLogic->BLKD_pfuncBlkStatusChk = __logicDiskStatusChk;
    
    pblkdLogic->BLKD_ulNSector         = ulNSector;
    pblkdLogic->BLKD_ulBytesPerSector  = pblkdPhysical->BLKD_ulBytesPerSector;
    pblkdLogic->BLKD_ulBytesPerBlock   = pblkdPhysical->BLKD_ulBytesPerBlock;
    
    pblkdLogic->BLKD_bRemovable        = pblkdPhysical->BLKD_bRemovable;
    pblkdLogic->BLKD_iRetry            = pblkdPhysical->BLKD_iRetry;
    pblkdLogic->BLKD_iFlag             = pblkdPhysical->BLKD_iFlag;
    
    pblkdLogic->BLKD_iLogic            = 1;
    pblkdLogic->BLKD_uiLinkCounter     = 0;
    
    /*
     *  链接最底层物理驱动控制块
     */
    while (pblkdPhysical->BLKD_pvLink && (pblkdPhysical != (PLW_BLK_DEV)pblkdPhysical->BLKD_pvLink)) {
        pblkdPhysical = (PLW_BLK_DEV)pblkdPhysical->BLKD_pvLink;
    }
    
    pblkdLogic->BLKD_pvLink = (PVOID)pblkdPhysical;
}
/*********************************************************************************************************
** 函数名称: __diskPartitionScan
** 功能描述: 分析单级分区表信息
** 输　入  : pblkd             块设备
**           pdpt              分区信息
**           uiCounter         分区计数器
**           ulStartSector     起始扇区
**           ulExtStartSector  扩展分区起始扇区
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __diskPartitionScan (PLW_BLK_DEV         pblkd,
                                 ULONG               ulBytesPerSector,
                                 PLW_DISKPART_TABLE  pdpt, 
                                 UINT                uiCounter,
                                 ULONG               ulStartSector,
                                 ULONG               ulExtStartSector)
{
#ifndef MBR_Table
#define MBR_Table			446	                                        /* MBR: Partition table offset  */
#endif

    INT                     i;
    INT                     iPartInfoStart;
    BYTE                    ucActiveFlag;
    BYTE                    ucPartType;
    LW_DISKPART_OPERAT     *pdoLogic;
    
    INT                     iError;
    
    BYTE                   *pucBuffer = (BYTE *)__SHEAP_ALLOC((size_t)ulBytesPerSector);
    
    if (pucBuffer == LW_NULL) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);                          /*  内存不足                    */
        return  (PX_ERROR);
    }
    lib_bzero(pucBuffer, (size_t)ulBytesPerSector);
    
    if (BLK_READ(pblkd, (PVOID)pucBuffer,
                 ulStartSector, 1) < 0) {                               /*  读取首扇区                  */
        __SHEAP_FREE(pucBuffer);
        return  (PX_ERROR);
    }
    
    if ((pucBuffer[ulBytesPerSector - 2] != 0x55) ||
        (pucBuffer[ulBytesPerSector - 1] != 0xaa)) {                    /*  扇区结束标志是否正确        */
        __SHEAP_FREE(pucBuffer);
        return  (PX_ERROR);
    }
    
    for (i = 0; i < 4; i++) {                                           /*  查看各个分区信息            */
        iPartInfoStart = MBR_Table + (i * 16);                          /*  获取分区信息起始点          */
        
        ucActiveFlag = pucBuffer[iPartInfoStart];                       /*  激活标志                    */
        ucPartType   = pucBuffer[iPartInfoStart + __DISK_PART_TYPE];    /*  分区文件系统类型            */
        
        if (LW_DISK_PART_IS_VALID(ucPartType)) {
            pdoLogic = &pdpt->DPT_dpoLogic[uiCounter];                  /*  逻辑分区信息                */
            
            pdoLogic->DPO_dpnEntry.DPN_ulStartSector = ulStartSector +
                BLK_LD_DWORD(&pucBuffer[iPartInfoStart + __DISK_PART_STARTSECTOR]);
            pdoLogic->DPO_dpnEntry.DPN_ulNSector = 
                BLK_LD_DWORD(&pucBuffer[iPartInfoStart + __DISK_PART_NSECTOR]);
            
            if (ucActiveFlag == __DISK_PART_ACTIVE) {                   /*  记录是否为活动分区          */
                pdoLogic->DPO_dpnEntry.DPN_bIsActive = LW_TRUE;
            } else {
                pdoLogic->DPO_dpnEntry.DPN_bIsActive = LW_FALSE;
            }
            pdoLogic->DPO_dpnEntry.DPN_ucPartType = ucPartType;         /*  记录分区类型                */
            
            pdoLogic->DPT_pblkdDisk = pblkd;                            /*  记录下层设备                */
            
            __logicDiskInit(&pdoLogic->DPO_blkdLogic,
                            pdoLogic->DPT_pblkdDisk,
                            pdoLogic->DPO_dpnEntry.DPN_ulNSector);      /*  初始化逻辑设备控制块        */
            
            uiCounter++;                                                /*  分区数量++                  */

        } else if (LW_DISK_PART_IS_EXTENDED(ucPartType)) {
            if (ulStartSector == 0ul) {                                 /*  是否位于主分区表            */
                ulExtStartSector = BLK_LD_DWORD(&pucBuffer[iPartInfoStart + __DISK_PART_STARTSECTOR]);
                ulStartSector    = ulExtStartSector;
            } else {                                                    /*  位于扩展分区分区表          */
                ulStartSector    = BLK_LD_DWORD(&pucBuffer[iPartInfoStart + __DISK_PART_STARTSECTOR]);
                ulStartSector   += ulExtStartSector;
            }
            
            iError = __diskPartitionScan(pblkd, ulBytesPerSector,
                                         pdpt, uiCounter, 
                                         ulStartSector, 
                                         ulExtStartSector);             /*  递归查询扩展分区            */
            if (iError < 0) {
                __SHEAP_FREE(pucBuffer);
                return  ((INT)uiCounter);
            } else {
                uiCounter = (UINT)iError;                               /*  iError 为新的 uiCounter     */
            }
        }
        
        if (uiCounter >= LW_CFG_MAX_DISKPARTS) {                        /*  不能再保存更多分区信息      */
            __SHEAP_FREE(pucBuffer);
            return  ((INT)uiCounter);
        }
    }
    
    __SHEAP_FREE(pucBuffer);
    if (uiCounter == 0) {                                               /*  没有检测到任何分区          */
        return  (PX_ERROR);
    } else {
        return  ((INT)uiCounter);                                       /*  检测到的分区数量            */
    }
}
/*********************************************************************************************************
** 函数名称: API_DiskPartitionScan
** 功能描述: 分析指定物理磁盘的分区情况, (通过分区表)
** 输　入  : pblkd             物理块设备驱动
**           pdptDisk          磁盘分区信息
** 输　出  : ERROR CODE        正确时, 返回分区数量
**                             当磁盘没有分区表或者分区信息错误时, 返回 -1.
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_DiskPartitionScan (PLW_BLK_DEV  pblkd, PLW_DISKPART_TABLE  pdptDisk)
{
    REGISTER INT    iError;
             ULONG  ulBytesPerSector;
             
    if (pblkd == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    lib_bzero(pdptDisk, sizeof(LW_DISKPART_TABLE));
    
    BLK_IOCTL(pblkd, LW_BLKD_CTRL_POWER, LW_BLKD_POWER_ON);             /*  打开电源                    */
    BLK_RESET(pblkd);                                                   /*  复位磁盘接口                */
    
    iError = BLK_IOCTL(pblkd, FIODISKINIT, 0);                          /*  初始化磁盘                  */
    if (iError < 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not initialize disk.\r\n");
        _ErrorHandle(ERROR_IO_DEVICE_ERROR);
        return  (iError);
    }
    
    if (pblkd->BLKD_ulBytesPerSector) {
        ulBytesPerSector = pblkd->BLKD_ulBytesPerSector;
    } else {
        iError = BLK_IOCTL(pblkd, LW_BLKD_GET_SECSIZE, &ulBytesPerSector);
        if (iError < 0) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "sector size invalidate.\r\n");
            _ErrorHandle(ERROR_IO_DEVICE_ERROR);
            return  (iError);
        }
    }
    
    iError = __diskPartitionScan(pblkd, ulBytesPerSector,
                                 pdptDisk, 0, 0, 0);                    /*  分析磁盘分区表              */
    if (iError >= 0) {
        pdptDisk->DPT_ulNPart = (ULONG)iError;                          /*  记录分区数量                */
    
    } else {
        LW_DISKPART_OPERAT  *pdoLogic;
        
        pdptDisk->DPT_ulNPart = 1;                                      /*  默认为单分区无分区表磁盘    */
        pdoLogic = &pdptDisk->DPT_dpoLogic[0];                          /*  逻辑分区信息                */
        
        pdoLogic->DPO_dpnEntry.DPN_ulStartSector = 0ul;
        pdoLogic->DPO_dpnEntry.DPN_ulNSector     = pblkd->BLKD_ulNSector;
        pdoLogic->DPO_dpnEntry.DPN_bIsActive     = LW_FALSE;
        pdoLogic->DPO_dpnEntry.DPN_ucPartType    = LW_DISK_PART_TYPE_EMPTY;
                                                                        /*  分区无效                    */
        pdoLogic->DPT_pblkdDisk = pblkd;                                /*  记录下层设备                */
        
        __logicDiskInit(&pdoLogic->DPO_blkdLogic,
                        pblkd,
                        pblkd->BLKD_ulNSector);                         /*  初始化逻辑设备控制块        */
        iError = 1;                                                     /*  单分区                      */
    }
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: API_DiskPartitionGet
** 功能描述: 通过先前分析的分区信息, 获得一个逻辑分区的操作 BLK_DEV
** 输　入  : ppdptDisk         物理磁盘的分区信息
**           uiPart            第几个分区
**           ppblkdLogic       逻辑分区的操作 BLK_DEV
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_DiskPartitionGet (PLW_DISKPART_TABLE  pdptDisk, UINT  uiPart, PLW_BLK_DEV  *ppblkdLogic)
{
    if (pdptDisk == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    if (ppblkdLogic == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    if (uiPart >= pdptDisk->DPT_ulNPart) {
        _ErrorHandle(ERROR_IO_VOLUME_ERROR);
        return  (PX_ERROR);
    }
    
    *ppblkdLogic = (PLW_BLK_DEV)__SHEAP_ALLOC(sizeof(LW_DISKPART_OPERAT));
    if (*ppblkdLogic == LW_NULL) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    lib_memcpy(*ppblkdLogic, 
               &pdptDisk->DPT_dpoLogic[uiPart].DPO_blkdLogic, 
               sizeof(LW_DISKPART_OPERAT));                             /*  拷贝控制块信息              */
    
    return  (ERROR_NONE);                                               /*  返回分区逻辑设备            */
}
/*********************************************************************************************************
** 函数名称: API_DiskPartitionFree
** 功能描述: 当一个分区卷从系统中被移除, 需要释放一个分区逻辑信息.
** 输　入  : pblkdLogic        分区逻辑设备控制块
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_DiskPartitionFree (PLW_BLK_DEV  pblkdLogic)
{
    if (pblkdLogic == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    if ((pblkdLogic->BLKD_iLogic == 0) ||
        (pblkdLogic->BLKD_pvLink == LW_NULL)) {
        _ErrorHandle(ERROR_IO_DISK_NOT_PRESENT);
        return  (PX_ERROR);
    }
    __SHEAP_FREE(pblkdLogic);                                           /*  释放内存                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_DiskPartitionLinkNumGet
** 功能描述: 获得一个物理设备当前挂载的逻辑磁盘数量. (用户卸载物理磁盘时判断)
** 输　入  : pblkdPhysical       物理控制块
** 输　出  : 链接数量
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_DiskPartitionLinkNumGet (PLW_BLK_DEV  pblkdPhysical)
{
    if (pblkdPhysical == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    return  ((INT)pblkdPhysical->BLKD_uiLinkCounter);
}

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_DISKPART_EN > 0)    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
