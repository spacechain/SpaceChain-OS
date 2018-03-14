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
** 文   件   名: blockRaw.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2015 年 08 月 24 日
**
** 描        述: 系统块设备 RAW IO 接口.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_BLKRAW_EN > 0) && (LW_CFG_MAX_VOLUMES > 0)
/*********************************************************************************************************
  宏定义
*********************************************************************************************************/
#define __BLKRAW_SERIAL             "BLKRAW"
#define __BLKRAW_FWREV              "VER "
#define __BLKRAW_MODLE              "BLKRAW"
/*********************************************************************************************************
  默认参数
*********************************************************************************************************/
#define LW_BLKRAW_DEF_SEC_SIZE      512
/*********************************************************************************************************
** 函数名称: __blkRawReset
** 功能描述: 复位 blk raw 块设备.
** 输　入  : pblkraw       blk raw 块设备
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __blkRawReset (PLW_BLK_RAW  pblkraw)
{
    if (!S_ISBLK(pblkraw->BLKRAW_mode)) {
        return  (ERROR_NONE);
    }
    
    return  (ioctl(pblkraw->BLKRAW_iFd, LW_BLKD_CTRL_RESET));
}
/*********************************************************************************************************
** 函数名称: __blkRawStatus
** 功能描述: 检测 blk raw 块设备.
** 输　入  : pblkraw       blk raw 块设备
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __blkRawStatus (PLW_BLK_RAW  pblkraw)
{
    if (!S_ISBLK(pblkraw->BLKRAW_mode)) {
        return  (ERROR_NONE);
    }
    
    return  (ioctl(pblkraw->BLKRAW_iFd, LW_BLKD_CTRL_STATUS));
}
/*********************************************************************************************************
** 函数名称: __blkRawIoctl
** 功能描述: 控制 blk raw 块设备.
** 输　入  : pblkraw       blk raw 块设备
**           iCmd          控制命令
**           lArg          控制参数
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __blkRawIoctl (PLW_BLK_RAW  pblkraw, INT  iCmd, LONG  lArg)
{
    if (!S_ISBLK(pblkraw->BLKRAW_mode)) {
        PLW_BLK_INFO    hBlkInfo;                                       /* 设备信息                     */
        
        switch (iCmd) {
        
        case FIOSYNC:
        case FIOFLUSH:
        case FIODATASYNC:
        case FIOSYNCMETA:
            return  (ioctl(pblkraw->BLKRAW_iFd, iCmd, lArg));
            
        case FIOUNMOUNT:
        case FIODISKINIT:
        case FIOTRIM:
        case FIOCANCEL:
        case FIODISKCHANGE:
            return  (ERROR_NONE);
            
        case LW_BLKD_CTRL_INFO:
            hBlkInfo = (PLW_BLK_INFO)lArg;
            if (!hBlkInfo) {
                _ErrorHandle(EINVAL);
                return  (PX_ERROR);
            }
            lib_bzero(hBlkInfo, sizeof(LW_BLK_INFO));
            hBlkInfo->BLKI_uiType = LW_BLKD_CTRL_INFO_TYPE_BLKRAW;
            snprintf(hBlkInfo->BLKI_cSerial, LW_BLKD_CTRL_INFO_STR_SZ, "%s %08lX", 
                     __BLKRAW_SERIAL, (addr_t)pblkraw);
            snprintf(hBlkInfo->BLKI_cFirmware, LW_BLKD_CTRL_INFO_STR_SZ, "%s%s", 
                     __BLKRAW_FWREV, __SYLIXOS_VERSTR);
            snprintf(hBlkInfo->BLKI_cProduct, LW_BLKD_CTRL_INFO_STR_SZ, "%s", 
                     __BLKRAW_MODLE);
            return  (ERROR_NONE);
        }
    }
    
    return  (ioctl(pblkraw->BLKRAW_iFd, iCmd, lArg));
}
/*********************************************************************************************************
** 函数名称: __blkRawDevWrite
** 功能描述: 写 blk raw 块设备.
** 输　入  : pblkraw           blk raw 块设备
**           pvBuffer          缓冲区
**           ulStartSector     起始扇区号
**           ulSectorCount     扇区数量
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __blkRawWrite (PLW_BLK_RAW  pblkraw, 
                           const VOID *pvBuffer, 
                           ULONG       ulStartSector, 
                           ULONG       ulSectorCount)
{
    size_t  stBytes  = (size_t)ulSectorCount * pblkraw->BLKRAW_blkd.BLKD_ulBytesPerSector;
    off_t   oftStart = (off_t)ulStartSector  * pblkraw->BLKRAW_blkd.BLKD_ulBytesPerSector;
    
    if (pwrite(pblkraw->BLKRAW_iFd, pvBuffer, stBytes, oftStart) == stBytes) {
        return  (ERROR_NONE);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __blkRawDevRead
** 功能描述: 读 blk raw 块设备.
** 输　入  : pblkraw           blk raw 块设备
**           pvBuffer          缓冲区
**           ulStartSector     起始扇区号
**           ulSectorCount     扇区数量
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __blkRawRead (PLW_BLK_RAW  pblkraw,
                          VOID       *pvBuffer, 
                          ULONG       ulStartSector, 
                          ULONG       ulSectorCount)
{
    size_t  stBytes  = (size_t)ulSectorCount * pblkraw->BLKRAW_blkd.BLKD_ulBytesPerSector;
    off_t   oftStart = (off_t)ulStartSector  * pblkraw->BLKRAW_blkd.BLKD_ulBytesPerSector;

    if (pread(pblkraw->BLKRAW_iFd, pvBuffer, stBytes, oftStart) == stBytes) {
        return  (ERROR_NONE);
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __blkRawCreate
** 功能描述: 通过 /dev/blk/xxx 块设备生成一个 BLOCK 控制块.
** 输　入  : pblkraw       BLOCK RAW 设备
**           bRdOnly       只读
**           bLogic        是否为逻辑分区
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __blkRawCreate (PLW_BLK_RAW  pblkraw, INT  iFlag, BOOL  bLogic)
{
    struct stat       statBuf;
    PLW_BLK_DEV       pblkd = &pblkraw->BLKRAW_blkd;

    pblkraw->BLKRAW_iFd = open(pblkd->BLKD_pcName, iFlag);
    if (pblkraw->BLKRAW_iFd < 0) {
        iFlag = O_RDONLY;
        pblkraw->BLKRAW_iFd = open(pblkd->BLKD_pcName, iFlag);
        if (pblkraw->BLKRAW_iFd < 0) {
            return  (PX_ERROR);
        }
    }
    
    if (fstat(pblkraw->BLKRAW_iFd, &statBuf) < 0) {
        close(pblkraw->BLKRAW_iFd);
        return  (PX_ERROR);
    }
    
    pblkd->BLKD_pfuncBlkRd        = __blkRawRead;
    pblkd->BLKD_pfuncBlkWrt       = __blkRawWrite;
    pblkd->BLKD_pfuncBlkIoctl     = __blkRawIoctl;
    pblkd->BLKD_pfuncBlkReset     = __blkRawReset;
    pblkd->BLKD_pfuncBlkStatusChk = __blkRawStatus;
    
    if (S_ISBLK(statBuf.st_mode)) {
        ioctl(pblkraw->BLKRAW_iFd, LW_BLKD_GET_SECNUM,  &pblkd->BLKD_ulNSector);
        ioctl(pblkraw->BLKRAW_iFd, LW_BLKD_GET_SECSIZE, &pblkd->BLKD_ulBytesPerSector);
        ioctl(pblkraw->BLKRAW_iFd, LW_BLKD_GET_BLKSIZE, &pblkd->BLKD_ulBytesPerBlock);
    
    } else {
        pblkd->BLKD_ulNSector        = (ULONG)(statBuf.st_size / LW_BLKRAW_DEF_SEC_SIZE);
        pblkd->BLKD_ulBytesPerSector = LW_BLKRAW_DEF_SEC_SIZE;
        pblkd->BLKD_ulBytesPerBlock  = LW_BLKRAW_DEF_SEC_SIZE;
    }
    
    if (!pblkd->BLKD_ulNSector        ||
        !pblkd->BLKD_ulBytesPerSector ||
        !pblkd->BLKD_ulBytesPerBlock) {
        _ErrorHandle(ENOTSUP);
        close(pblkraw->BLKRAW_iFd);
        return  (PX_ERROR);
    }
    
    pblkd->BLKD_bRemovable  = LW_TRUE;
    pblkd->BLKD_bDiskChange = LW_FALSE;
    pblkd->BLKD_iRetry      = 2;
    pblkd->BLKD_iFlag       = iFlag;
    
    if (bLogic) {
        pblkd->BLKD_iLogic = 1;
        pblkd->BLKD_pvLink = (PLW_BLK_DEV)pblkd;
    }
    
    pblkraw->BLKRAW_mode = statBuf.st_mode;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __blkRawDelete
** 功能描述: 删除通过 /dev/blk/xxx 块设备生成一个 BLOCK 控制块.
** 输　入  : pblkraw       BLOCK RAW 设备
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __blkRawDelete (PLW_BLK_RAW  pblkraw)
{
    fsync(pblkraw->BLKRAW_iFd);
    close(pblkraw->BLKRAW_iFd);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_BlkRawCreate
** 功能描述: 通过 /dev/blk/xxx 块设备生成一个 BLOCK 控制块 (只能内核程序调用).
** 输　入  : pcBlkName     块设备名称
**           bRdOnly       只读
**           bLogic        是否为逻辑分区
**           pblkraw       创建的 blk raw 控制块
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API 
INT  API_BlkRawCreate (CPCHAR  pcBlkName, BOOL  bRdOnly, BOOL  bLogic, PLW_BLK_RAW  pblkraw)
{
    INT         iFlag;
    INT         iRet;
    
    if (!pcBlkName || !pblkraw) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    iFlag = (bRdOnly) ? O_RDONLY : O_RDWR;
    
    lib_bzero(pblkraw, sizeof(LW_BLK_RAW));
    
    pblkraw->BLKRAW_blkd.BLKD_pcName = lib_strdup(pcBlkName);
    if (pblkraw == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }
    
    __KERNEL_SPACE_ENTER();
    iRet = __blkRawCreate(pblkraw, iFlag, bLogic);
    __KERNEL_SPACE_EXIT();
    
    if (iRet < ERROR_NONE) {
        __SHEAP_FREE(pblkraw);
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_BlkRawDelete
** 功能描述: 删除通过 /dev/blk/xxx 块设备生成一个 BLOCK 控制块.
** 输　入  : pblkraw       BLOCK RAW 设备
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API 
INT  API_BlkRawDelete (PLW_BLK_RAW  pblkraw)
{
    INT   iRet;

    if (!pblkraw) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    __KERNEL_SPACE_ENTER();
    iRet = __blkRawDelete(pblkraw);
    __KERNEL_SPACE_EXIT();
    
    lib_free(pblkraw->BLKRAW_blkd.BLKD_pcName);
    
    return  (iRet);
}

#endif                                                                  /*  LW_CFG_BLKRAW_EN > 0        */
                                                                        /*  LW_CFG_MAX_VOLUMES > 0      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
