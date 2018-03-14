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
** 文   件   名: diskRaid1.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2014 年 06 月 06 日
**
** 描        述: 软件 RAID 磁盘阵列管理. (RAID-1 多磁盘扩展容量, 完全备份)
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
#include "diskRaidLib.h"
/*********************************************************************************************************
  2 块磁盘 RAID-1 阵列的 BLK_DEV 文件系统示例结构:

                                               ...
                                                |
                                                |
                                           +----------+
disk cache block device:                   |  dcache  |
                                           |  BLK_DEV |
                                           +----------+
                                                |
                                                |
                                           +----------+
raid disk block device:                    |  raid-1  |  (size is phy #0)
                                           |  BLK_DEV |
                                           +----------+
                                                |
                                                |
                                     ------------------------
                                    /                        \
                              +----------+              +----------+
physical disk block device:   |  phy #0  |              |  phy #1  |
                              |  BLK_DEV |              |  BLK_DEV |
                              +----------+              +----------+
                                    |                         |
                                    |                         |
                              +----------+              +----------+
physical HDD:                 |   HDD0   |              |   HDD1   |
                              +----------+              +----------+
*********************************************************************************************************/
/*********************************************************************************************************
  RAID-1 型磁盘阵列虚拟设备类型
*********************************************************************************************************/
typedef struct {
    LW_BLK_DEV              DISKR_blkdRaid;                             /*  DISK CACHE 的 BLK IO 控制块 */
    PLW_BLK_DEV            *DISKR_ppblkdDisk;                           /*  阵列物理磁盘列表            */
    UINT                    DISKR_uiNDisks;                             /*  阵列磁盘数量                */
} LW_DISKRAID1_CB;
typedef LW_DISKRAID1_CB    *PLW_DISKRAID1_CB;
/*********************************************************************************************************
  操作宏
*********************************************************************************************************/
#define RAID_BLK_RESET(pdiskr, disk)    \
        pdiskr->DISKR_ppblkdDisk[disk]->BLKD_pfuncBlkReset(pdiskr->DISKR_ppblkdDisk[disk])
#define RAID_BLK_STATUS(pdiskr, disk)   \
        pdiskr->DISKR_ppblkdDisk[disk]->BLKD_pfuncBlkStatusChk(pdiskr->DISKR_ppblkdDisk[disk])
#define RAID_BLK_IOCTL(pdiskr, disk, cmd, arg)  \
        pdiskr->DISKR_ppblkdDisk[disk]->BLKD_pfuncBlkIoctl(pdiskr->DISKR_ppblkdDisk[disk], cmd, arg)
#define RAID_BLK_READ(pdiskr, disk, buf, sect, num) \
        pdiskr->DISKR_ppblkdDisk[disk]->BLKD_pfuncBlkRd(pdiskr->DISKR_ppblkdDisk[disk], buf, sect, num)
#define RAID_BLK_WRITE(pdiskr, disk, buf, sect, num)    \
        pdiskr->DISKR_ppblkdDisk[disk]->BLKD_pfuncBlkWrt(pdiskr->DISKR_ppblkdDisk[disk], buf, sect, num)
/*********************************************************************************************************
** 函数名称: __raid1DevReset
** 功能描述: 复位块设备.
** 输　入  : pdiskr             RAID-1 节点
** 输　出  : 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __raid1DevReset (PLW_DISKRAID1_CB  pdiskr)
{
    INT  i;

    for (i = 0; i < pdiskr->DISKR_uiNDisks; i++) {
        RAID_BLK_RESET(pdiskr, i);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __raid1DevStatusChk
** 功能描述: 检测块设备.
** 输　入  : pdiskr             RAID-1 节点
** 输　出  : 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __raid1DevStatusChk (PLW_DISKRAID1_CB  pdiskr)
{
    INT  iStatus = ERROR_NONE;
    INT  i;

    for (i = 0; i < pdiskr->DISKR_uiNDisks; i++) {
        iStatus = RAID_BLK_STATUS(pdiskr, i);
        if (iStatus) {
            return  (iStatus);
        }
    }

    return  (iStatus);
}
/*********************************************************************************************************
** 函数名称: __raid1DevIoctl
** 功能描述: 控制块设备.
** 输　入  : pdiskr            RAID-1 节点
**           iCmd              控制命令
**           lArg              控制参数
** 输　出  : 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __raid1DevIoctl (PLW_DISKRAID1_CB  pdiskr, INT  iCmd, LONG  lArg)
{
    INT  i;
    INT  iRet = ERROR_NONE;

    switch (iCmd) {

    case FIODISKINIT:
    case FIOUNMOUNT:
    case FIOFLUSH:
    case FIOSYNC:
    case FIODATASYNC:
    case LW_BLKD_CTRL_POWER:
    case LW_BLKD_CTRL_LOCK:
        for (i = 0; i < pdiskr->DISKR_uiNDisks; i++) {
            if (RAID_BLK_IOCTL(pdiskr, i, iCmd, lArg)) {
                iRet = PX_ERROR;
            }
        }
        return  (iRet);

    default:
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: __raid1DevWrt
** 功能描述: 写块设备.
** 输　入  : pdiskr            RAID-1 节点
**           pvBuffer          缓冲区
**           ulStartSector     起始扇区号
**           ulSectorCount     扇区数量
** 输　出  : 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __raid1DevWrt (PLW_DISKRAID1_CB  pdiskr,
                           VOID             *pvBuffer,
                           ULONG             ulStartSector,
                           ULONG             ulSectorCount)
{
    INT  i;
    INT  iRet = ERROR_NONE;

    for (i = 0; i < pdiskr->DISKR_uiNDisks; i++) {
        if (RAID_BLK_WRITE(pdiskr, i, pvBuffer, ulStartSector, ulSectorCount)) {
            iRet = PX_ERROR;
        }
    }

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __raid1DevRd
** 功能描述: 读块设备.
** 输　入  : pdiskr            RAID-1 节点
**           pvBuffer          缓冲区
**           ulStartSector     起始扇区号
**           ulSectorCount     扇区数量
** 输　出  : 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __raid1DevRd (PLW_DISKRAID1_CB   pdiskr,
                          VOID              *pvBuffer,
                          ULONG              ulStartSector,
                          ULONG              ulSectorCount)
{
    INT  i;
    INT  iRet = ERROR_NONE;

    for (i = 0; i < pdiskr->DISKR_uiNDisks; i++) {
        if (RAID_BLK_READ(pdiskr, i, pvBuffer, ulStartSector, ulSectorCount)) {
            _DebugFormat(__ERRORMESSAGE_LEVEL, "RAID-1 system block disk %u error.\r\n", i);
            iRet = PX_ERROR;
        } else {
            break;
        }
    }

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_DiskRiad1Create
** 功能描述: 创建一个 RAID-1 类型磁盘阵列块设备
** 输　入  : pblkd[]           物理块设备列表
**           uiNDisks          物理磁盘数量 必须为 2, 4 块这两种选择其中之一
**           ppblkDiskRaid     返回的 RAID 虚拟磁盘
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
ULONG  API_DiskRiad1Create (PLW_BLK_DEV  pblkd[],
                            UINT         uiNDisks,
                            PLW_BLK_DEV *ppblkDiskRaid)
{
    INT                 i;
    PLW_DISKRAID1_CB    pdiskr = LW_NULL;
    ULONG               ulBytesPerSector;
    ULONG               ulBytesPerBlock;
    ULONG               ulTotalSector;

    if (!pblkd || !ppblkDiskRaid ||
        ((uiNDisks != 2) && (uiNDisks != 4))) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }

    pdiskr = (PLW_DISKRAID1_CB)__SHEAP_ALLOC(sizeof(LW_DISKRAID1_CB));
    if (pdiskr == LW_NULL) {
        goto    __handle_errror;
    }

    pdiskr->DISKR_ppblkdDisk = (PLW_BLK_DEV *)__SHEAP_ALLOC(sizeof(PLW_BLK_DEV) * uiNDisks);
    if (pdiskr->DISKR_ppblkdDisk == LW_NULL) {
        goto    __handle_errror;
    }

    for (i = 0; i < uiNDisks; i++) {
        pdiskr->DISKR_ppblkdDisk[i] = pblkd[i];
        if (pblkd[i] == LW_NULL) {
            goto    __handle_errror;
        }
    }

    pdiskr->DISKR_uiNDisks = uiNDisks;

    if (__diskRaidCheck(pblkd, uiNDisks,
                        &ulBytesPerSector,
                        &ulBytesPerBlock,
                        &ulTotalSector)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "RAID-1 system block disk not EQU.\r\n");
        _ErrorHandle(ENOTSUP);
        return  (ENOTSUP);
    }

    pdiskr->DISKR_blkdRaid.BLKD_pcName            = "RAID-1";
    pdiskr->DISKR_blkdRaid.BLKD_pfuncBlkRd        = __raid1DevRd;
    pdiskr->DISKR_blkdRaid.BLKD_pfuncBlkWrt       = __raid1DevWrt;
    pdiskr->DISKR_blkdRaid.BLKD_pfuncBlkIoctl     = __raid1DevIoctl;
    pdiskr->DISKR_blkdRaid.BLKD_pfuncBlkReset     = __raid1DevReset;
    pdiskr->DISKR_blkdRaid.BLKD_pfuncBlkStatusChk = __raid1DevStatusChk;

    pdiskr->DISKR_blkdRaid.BLKD_ulNSector        = ulTotalSector;
    pdiskr->DISKR_blkdRaid.BLKD_ulBytesPerSector = ulBytesPerSector;
    pdiskr->DISKR_blkdRaid.BLKD_ulBytesPerBlock  = ulBytesPerBlock;

    pdiskr->DISKR_blkdRaid.BLKD_bRemovable  = pblkd[0]->BLKD_bRemovable;
    pdiskr->DISKR_blkdRaid.BLKD_bDiskChange = LW_FALSE;
    pdiskr->DISKR_blkdRaid.BLKD_iRetry      = pblkd[0]->BLKD_iRetry;
    pdiskr->DISKR_blkdRaid.BLKD_iFlag       = pblkd[0]->BLKD_iFlag;

    pdiskr->DISKR_blkdRaid.BLKD_iLogic        = LW_FALSE;
    pdiskr->DISKR_blkdRaid.BLKD_uiLinkCounter = 0;
    pdiskr->DISKR_blkdRaid.BLKD_pvLink        = LW_NULL;

    pdiskr->DISKR_blkdRaid.BLKD_uiPowerCounter = 0;
    pdiskr->DISKR_blkdRaid.BLKD_uiInitCounter  = 0;

    *ppblkDiskRaid = (PLW_BLK_DEV)pdiskr;

    _DebugHandle(__LOGMESSAGE_LEVEL, "RAID-1 system has been create.\r\n");
    return  (ERROR_NONE);

__handle_errror:
    if (pdiskr) {
        __SHEAP_FREE(pdiskr);
    }
    _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
    _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
    return  (ERROR_SYSTEM_LOW_MEMORY);
}
/*********************************************************************************************************
** 函数名称: API_DiskRiad1Delete
** 功能描述: 删除一个 RAID-1 类型磁盘阵列块设备
** 输　入  : pblkDiskRaid      之前创建的 RAID-1 虚拟磁盘
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_DiskRiad1Delete (PLW_BLK_DEV  pblkDiskRaid)
{
    if (pblkDiskRaid == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    _DebugHandle(__LOGMESSAGE_LEVEL, "RAID-1 system has been delete.\r\n");
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_DiskRiad1Ghost
** 功能描述: 磁盘拷贝
** 输　入  : pblkDest          目的磁盘
**           pblkSrc           源磁盘
**           ulStartSector     起始扇区
**           ulSectorNum       扇区数量
** 输　出  : 拷贝的扇区数量
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
ULONG  API_DiskRiad1Ghost (PLW_BLK_DEV  pblkDest,
                           PLW_BLK_DEV  pblkSrc,
                           ULONG        ulStartSector,
                           ULONG        ulSectorNum)
{
#define SECTORS_PER_TIME    16

    ULONG   ulCopyCount = 0ul;
    ULONG   ulTemp;
    ULONG   ulSectorSize;
    ULONG   ulTotalSectors;
    PVOID   pvBuffer;

    if (!pblkDest || !pblkSrc || !ulSectorNum) {
        _ErrorHandle(EINVAL);
        return  (ulCopyCount);
    }

    if (__diskRaidTotalSector(pblkSrc, &ulTotalSectors)) {
        _DebugHandle(__LOGMESSAGE_LEVEL, "RAID-1 system can not get total sector.\r\n");
        _ErrorHandle(EIO);
        return  (ulCopyCount);
    }

    if (ulStartSector >= ulTotalSectors) {
        _ErrorHandle(EINVAL);
        return  (ulCopyCount);
    }

    if ((ulTotalSectors - ulStartSector) > ulSectorNum) {
        ulSectorNum = ulTotalSectors - ulStartSector;
    }

    if (__diskRaidBytesPerSector(pblkSrc, &ulSectorSize)) {
        _DebugHandle(__LOGMESSAGE_LEVEL, "RAID-1 system can not get sector size.\r\n");
        _ErrorHandle(EIO);
        return  (ulCopyCount);
    }

    if (__diskRaidBytesPerSector(pblkDest, &ulTemp)) {
        _DebugHandle(__LOGMESSAGE_LEVEL, "RAID-1 system can not get sector size.\r\n");
        _ErrorHandle(EIO);
        return  (ulCopyCount);
    }

    if (ulSectorSize != ulTemp) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "RAID-1 system sector size not EQU.\r\n");
        _ErrorHandle(ENOTSUP);
        return  (ulCopyCount);
    }

    pvBuffer = __SHEAP_ALLOC(ulSectorSize * SECTORS_PER_TIME);
    if (pvBuffer == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (ulCopyCount);
    }

    do {
        if (ulSectorNum >= SECTORS_PER_TIME) {
            ulTemp = SECTORS_PER_TIME;
        } else {
            ulTemp = ulSectorNum;
        }
        if (pblkSrc->BLKD_pfuncBlkRd(pblkSrc, pvBuffer, ulStartSector, ulTemp)) {
            break;
        }
        if (pblkDest->BLKD_pfuncBlkWrt(pblkDest, pvBuffer, ulStartSector, ulTemp)) {
            break;
        }
        ulCopyCount   += ulTemp;
        ulStartSector += ulTemp;
        ulSectorNum   -= ulTemp;
    } while (ulSectorNum);

    __SHEAP_FREE(pvBuffer);
    return  (ulCopyCount);
}

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_DISKRAID_EN > 0)    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
