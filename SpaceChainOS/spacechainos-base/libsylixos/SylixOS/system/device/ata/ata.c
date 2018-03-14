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
** 文   件   名: ata.c
**
** 创   建   人: Hou.XiaoLong (侯小龙)
**
** 文件创建日期: 2010 年 02 月 23 日
**
** 描        述: ATA 设备驱动

** BUG:
2010.05.10  修改__ataRW()函数,在函数的开始先判断卡是否存在,并在对__ataWait()的返回值进行判断.
            并且在接收同步信号前先等待卡设备不忙
2011.04.06  为兼容64位CPU，修改__ataBlkRW()，__ataBlkRd()，__ataBlkWrt()的参数类型.
2014.03.03  优化代码.
2017.09.12  支持获取磁盘信息. Gong.YuJian (弓羽箭)
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "ataLib.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_ATA_EN > 0)
/*********************************************************************************************************
  内部全局变量
*********************************************************************************************************/
static __PATA_CTRL  _G_patactrlTable[LW_CFG_ATA_MAX_CTLS];
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
INT                __ataCmd(__PATA_CTRL pAtaCtrl,
                            INT         iDrive,
                            INT         iCmd,
                            INT         iArg0,
                            INT         iArg1);
INT                __ataWait(__PATA_CTRL patactrler, INT iRequest);
INT                __ataCtrlReset(__PATA_CTRL pAtaCtrl);
INT                __ataDriveInit(__PATA_CTRL patactrler, INT iDrive);
PCHAR              __ataDriveSerialInfoGet(__PATA_DRIVE hDrive, PCHAR  pcBuf, size_t stLen);
PCHAR              __ataDriveFwRevInfoGet(__PATA_DRIVE hDrive, PCHAR  pcBuf, size_t stLen);
PCHAR              __ataDriveModelInfoGet(__PATA_DRIVE hDrive, PCHAR  pcBuf, size_t stLen);
static __PATA_CTRL __ataSearchNode(INT iCtrl);
static INT         __ataAddNode(__PATA_CTRL patactrl);
static INT         __ataWrite(__PATA_CTRL patactrl);
static INT         __ataRW(__PATA_CTRL patactrler,
                           INT         iDrive,
                           INT         iCylinder,
                           INT         iHead,
                           INT         iSector,
                           PVOID       pBuffer,
                           INT         iNSecs,
                           INT         iDirection,
                           INT         iRetry);
static INT         __ataBlkRW(__ATA_DEV  *patadev,
                              VOID       *pvBuffer,
                              ULONG       ulStartBlk,
                              ULONG       ulNBlks,
                              INT         iDirection);
static INT         __ataBlkRd(__PATA_DEV  patadev,
                              VOID       *pvBuffer,
                              ULONG       ulStartBlk,
                              ULONG       ulNBlks);
static INT         __ataBlkWrt(__PATA_DEV  patadev,
                               VOID       *pvBuffer,
                               ULONG       ulStartBlk,
                               ULONG       ulNBlks);
static INT         __ataIoctl(__PATA_DEV patadev,
                              INT        iCmd,
                              LONG       lArg);
static INT         __ataStatus(__PATA_DEV  patadev);
static INT         __ataReset(__PATA_DEV  patadev);
/*********************************************************************************************************
  IO 命令
*********************************************************************************************************/
#define __ATA_CMD_USERBASE          0x2000                              /*  用户命令起始值              */
#define __ATA_CMD_ENB_POWERMANAGE   (0x01 + __ATA_CMD_USERBASE)         /*  使能高级电源管理            */
#define __ATA_CMD_DIS_POWERMANAGE   (0x02 + __ATA_CMD_USERBASE)         /*  禁止高级电源管理            */
#define __ATA_CMD_IDLEIMMEDIATE     (0x03 + __ATA_CMD_USERBASE)         /*  设备立即空闲                */
#define __ATA_CMD_STANDBYIMMEDIATE  (0x04 + __ATA_CMD_USERBASE)         /*  设备立即准备好              */
#define __ATA_CMD_HARDRESET         (0x05 + __ATA_CMD_USERBASE)         /*  设备硬件复位                */
/*********************************************************************************************************
  ATACTRL LOCK
*********************************************************************************************************/
#define ATACTRL_LOCK(atactrl)       \
        API_SemaphoreMPend(patactrl->ATACTRL_ulMuteSem, LW_OPTION_WAIT_INFINITE)
#define ATACTRL_UNLOCK(atactrl)     \
        API_SemaphoreMPost(patactrl->ATACTRL_ulMuteSem)
/*********************************************************************************************************
** 函数名称: __ataSearchNode
** 功能描述: 根据控制器号,查找ATA设备节点
** 输　入  : iCtrl       控制器号
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static __PATA_CTRL __ataSearchNode (INT iCtrl)
{
    if (iCtrl < LW_CFG_ATA_MAX_CTLS) {
        return  (_G_patactrlTable[iCtrl]);

    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** 函数名称: __ataAddNode
** 功能描述: 添加ATA节点
** 输　入  : patactrl       ATA节点结构指针
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __ataAddNode (__PATA_CTRL patactrl)
{
    _G_patactrlTable[patactrl->ATACTRL_iCtrl] = patactrl;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ataWrite
** 功能描述: ata设备写操作回调
** 输　入  : patactrl   ATA控制器节点结构指针
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __ataWrite (__PATA_CTRL patactrl)
{
    INTREG        iregInterLevel;

    if (!patactrl) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    LW_SPIN_LOCK_QUICK(&patactrl->ATACTRL_slLock, &iregInterLevel);
    patactrl->ATACTRL_iIntStatus =  __ATA_CTRL_INBYTE(patactrl, __ATA_STATUS(patactrl));
    LW_SPIN_UNLOCK_QUICK(&patactrl->ATACTRL_slLock, iregInterLevel);

    API_SemaphoreBPost(patactrl->ATACTRL_ulSyncSem);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ataDevChk
** 功能描述: ata设备检测函数
** 输　入  : patactrl    ATA控制器结构指针
**           bDevIsExist 设备是否存在标志
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __ataDevChk (__PATA_CTRL patactrl, BOOL bDevIsExist)
{
    INTREG        iregInterLevel;

    if (!patactrl) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    LW_SPIN_LOCK_QUICK(&patactrl->ATACTRL_slLock, &iregInterLevel);
    patactrl->ATACTRL_bIsExist = bDevIsExist;
    LW_SPIN_UNLOCK_QUICK(&patactrl->ATACTRL_slLock, iregInterLevel);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ataRW
** 功能描述: 读写当前磁道一定数量的扇区数
** 输　入  : iCtrl       控制器号
**           iDrive      设备驱动号
**           iCylinder   柱面
**           iHead       磁头
**           iSector     扇区
**           pusBuf      缓冲区
**           iNSecs      扇区数
**           iDirection  操作方向
**           iRetry      重试次数
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __ataRW (__PATA_CTRL patactrler,
                    INT         iDrive,
                    INT         iCylinder,
                    INT         iHead,
                    INT         iSector,
                    PVOID       pvBuffer,
                    INT         iNSecs,
                    INT         iDirection,
                    INT         iRetry)
{
    __PATA_CTRL  patactrl  = LW_NULL;
    __PATA_DRIVE patadrive = LW_NULL;
    __PATA_TYPE  patatype  = LW_NULL;

    INT     iRetryCount    = 0;
    INT     iBlock         = 1;
    INT     iNSectors      = iNSecs;
    INT     iNWords        = 0;
    INT     iStatus        = 0;
    ULONG   ulSemStatus    = ERROR_NONE;
    volatile INT i         = 0;

    INT16 *psBuf           = LW_NULL;

    if ((!patactrler) || (!pvBuffer)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "buffer address error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    patactrl  = patactrler;
    patadrive = &patactrl->ATACTRL_ataDrive[iDrive];
    patatype  = patadrive->ATADRIVE_patatypeDriverInfo;

    ATA_DEBUG_MSG(("__ataRW(): ctrl=%d drive=%d c=%d h=%d s=%d n=%d dir=%d\n",
                  patactrl->ATACTRL_iCtrl, iDrive, iCylinder, iHead, iSector, iNSecs, iDirection));

    if (patactrl->ATACTRL_bIsExist != LW_TRUE) {                        /*  ATA设备不存在               */
        return  (PX_ERROR);
    }

__retry_rw:
    iStatus = __ataWait(patactrl, __ATA_STAT_IDLE);                     /*  等待BSY跟DRQ为0             */
    if (iStatus != ERROR_NONE) {
        ATA_DEBUG_MSG(("__ataRW() %d/%d: status = 0x%x read timed out\n",
                       patactrl->ATACTRL_iCtrl, iDrive, 
                       __ATA_CTRL_INBYTE(patactrl, __ATA_ASTATUS(patactrl))));
        return  (PX_ERROR);
    }

    psBuf     = (INT16 *)pvBuffer;
    iNSectors = iNSecs;

    __ATA_CTRL_OUTBYTE(patactrl, __ATA_SECTOR(patactrl), (UINT8)(iSector >> 8));
    __ATA_CTRL_OUTBYTE(patactrl, __ATA_SECTOR(patactrl), (UINT8)iSector);
    __ATA_CTRL_OUTBYTE(patactrl, __ATA_SECCNT(patactrl), (UINT8)iNSecs);
    __ATA_CTRL_OUTBYTE(patactrl, __ATA_CYLLO(patactrl), (UINT8)iCylinder);
    __ATA_CTRL_OUTBYTE(patactrl, __ATA_CYLHI(patactrl), (UINT8)(iCylinder >> 8));

    iStatus = __ataWait(patactrl, __ATA_STAT_IDLE);
    if (iStatus != ERROR_NONE) {
        ATA_DEBUG_MSG(("__ataRW() %d/%d: status = 0x%x read timed out\n",
                       patactrl->ATACTRL_iCtrl, iDrive, 
                       __ATA_CTRL_INBYTE(patactrl, __ATA_ASTATUS(patactrl))));
        return  (PX_ERROR);
    }

    if (patadrive->ATADRIVE_sOkLba) {
        __ATA_CTRL_OUTBYTE(patactrl, __ATA_SDH(patactrl),
                           (UINT8)(__ATA_SDH_LBA        |         \
                           (iDrive << __ATA_DRIVE_BIT)  |         \
                           (iHead & 0xf)));

    } else {
        __ATA_CTRL_OUTBYTE(patactrl, __ATA_SDH(patactrl),
                           (UINT8)(__ATA_SDH_CHS        |         \
                           (iDrive << __ATA_DRIVE_BIT)  |         \
                           (iHead & 0xf)));
    }

    if (patadrive->ATADRIVE_sRwPio == ATA_PIO_MULTI) {
        iBlock = patadrive->ATADRIVE_sMultiSecs;
    }

    iNWords = (patatype->ATATYPE_iBytes * iBlock) >> 1;                 /*  所需读写的数据的字数量      */

    iStatus = __ataWait(patactrl, __ATA_STAT_IDLE);
    if (iStatus != ERROR_NONE) {
        ATA_DEBUG_MSG(("__ataRW() %d/%d: status = 0x%x read timed out\n",
                       patactrl->ATACTRL_iCtrl, iDrive, 
                       __ATA_CTRL_INBYTE(patactrl, __ATA_ASTATUS(patactrl))));
        return  (PX_ERROR);
    }

    if (iDirection == O_WRONLY) {                                       /*  写操作                      */
        if (patadrive->ATADRIVE_sRwPio == ATA_PIO_MULTI) {
            __ATA_CTRL_OUTBYTE(patactrl, __ATA_COMMAND(patactrl), __ATA_CMD_WRITE_MULTI);
                                                                        /*  发送多重写操作命令          */
        } else {
            __ATA_CTRL_OUTBYTE(patactrl, __ATA_COMMAND(patactrl), __ATA_CMD_WRITE);
                                                                        /*  发送单扇区写操作命令        */
        }

        while (iNSectors > 0) {
            if ((patadrive->ATADRIVE_sRwPio == ATA_PIO_MULTI) &&
                (iNSectors < iBlock)) {                                 /*  所需操作的扇区数少于默认设置
                                                                            的多重读写扇区数            */
                iBlock  = iNSectors;
                iNWords = (patatype->ATATYPE_iBytes * iBlock) >> 1;     /*  一次读写操作的字数          */
            }

            iStatus = __ataWait(patactrl, __ATA_STAT_DRQ);
            if (iStatus != ERROR_NONE) {
                ATA_DEBUG_MSG(("__ataRW() %d/%d: status = 0x%x read timed out\n",
                               patactrl->ATACTRL_iCtrl, iDrive, 
                               __ATA_CTRL_INBYTE(patactrl, __ATA_ASTATUS(patactrl))));
                return  (PX_ERROR);
            }

            if (patadrive->ATADRIVE_sRwBits == ATA_BITS_8) {            /*  8位操作                     */
                INT8   *pcBuff      = (INT8 *)psBuf;
                INT     iNByteWords =  (1 << iNWords);

                for (i = 0; i < iNByteWords; i++) {
                    __ATA_CTRL_OUTBYTE(patactrl, __ATA_DATA(patactrl), *pcBuff);
                    pcBuff++;
                }

            } else if (patadrive->ATADRIVE_sRwBits == ATA_BITS_16) {    /*  16位操作                    */
                __ATA_CTRL_OUTSTRING(patactrl, __ATA_DATA(patactrl), psBuf, iNWords);
            
            } else {                                                    /*  32位操作                    */
               /*
                *   TODO: 32位操作
                */
                ATA_DEBUG_MSG(("__ataRW() error: 32 bits write operation\n"));
                return  (PX_ERROR);
            }

            iStatus = __ataWait(patactrl, __ATA_STAT_BUSY);
            if (iStatus != ERROR_NONE) {
                ATA_DEBUG_MSG(("__ataRW() %d/%d: status = 0x%x read timed out\n",
                               patactrl->ATACTRL_iCtrl, iDrive, 
                               __ATA_CTRL_INBYTE(patactrl, __ATA_ASTATUS(patactrl))));
                return  (PX_ERROR);
            }

            if (patactrl->ATACTRL_bIntDisable == LW_FALSE) {            /*  假如使能了中断              */
                ulSemStatus = API_SemaphoreBPend(patactrl->ATACTRL_ulSyncSem,   \
                                    patactrl->ATACTRL_ulSyncSemTimeout);/*  等待同步信号                */
            }

            if ((patactrl->ATACTRL_iIntStatus & __ATA_STAT_ERR) || (ulSemStatus != ERROR_NONE)) {
                goto __error_handle;
            }

            psBuf     += iNWords;                                       /*  调整缓冲区的指针            */
            iNSectors -= iBlock;                                        /*  计算还需操作的扇区数        */
        }

    } else {                                                            /*  读操作                      */
        if (patadrive->ATADRIVE_sRwPio == ATA_PIO_MULTI) {
            __ATA_CTRL_OUTBYTE(patactrl, __ATA_COMMAND(patactrl), __ATA_CMD_READ_MULTI);
                                                                        /*  发送多重读操作命令          */
        } else {
            __ATA_CTRL_OUTBYTE(patactrl, __ATA_COMMAND(patactrl), __ATA_CMD_READ);
                                                                        /*  发送单扇区读操作命令        */
        }

        while (iNSectors > 0) {
            if ((patadrive->ATADRIVE_sRwPio == ATA_PIO_MULTI) &&
                (iNSectors < iBlock)) {                                 /*  所需操作的扇区数少于默认设置
                                                                            的多重读写扇区数            */
                iBlock  = iNSectors;
                iNWords = (patatype->ATATYPE_iBytes * iBlock) >> 1;
            }

            iStatus = __ataWait(patactrl, __ATA_STAT_BUSY);             /*  等待慢速磁盘设备            */
            if (iStatus != ERROR_NONE) {
                ATA_DEBUG_MSG(("__ataRW() %d/%d: status = 0x%x read timed out\n",
                               patactrl->ATACTRL_iCtrl, iDrive, 
                               __ATA_CTRL_INBYTE(patactrl, __ATA_ASTATUS(patactrl))));
                return  (PX_ERROR);
            }

            iStatus = __ataWait(patactrl, __ATA_STAT_DRQ);
            if (iStatus != ERROR_NONE) {
                ATA_DEBUG_MSG(("__ataRW() %d/%d: status = 0x%x read timed out\n",
                               patactrl->ATACTRL_iCtrl, iDrive, 
                               __ATA_CTRL_INBYTE(patactrl, __ATA_ASTATUS(patactrl))));
                return  (PX_ERROR);
            }

            if (patactrl->ATACTRL_bIntDisable == LW_FALSE) {
                ulSemStatus = API_SemaphoreBPend(patactrl->ATACTRL_ulSyncSem,   \
                                    patactrl->ATACTRL_ulSyncSemTimeout);/*  等待同步信号                */
            }

            if ((patactrl->ATACTRL_iIntStatus & __ATA_STAT_ERR) || (ulSemStatus != ERROR_NONE)) {
                goto __error_handle;
            }

            if (patadrive->ATADRIVE_sRwBits == ATA_BITS_8) {            /*  8位操作                     */
                INT8   *pcBuff      = (INT8 *)psBuf;
                INT     iNByteWords =  (1 << iNWords);

                for (i = 0; i < iNByteWords; i++) {
                    __ATA_CTRL_OUTBYTE(patactrl, __ATA_DATA(patactrl), *pcBuff);
                    pcBuff++;
                }

            } else if (patadrive->ATADRIVE_sRwBits == ATA_BITS_16) {    /*  16位操作                    */
                __ATA_CTRL_INSTRING(patactrl, __ATA_DATA(patactrl), psBuf, iNWords);
            
            } else {                                                    /*  32位操作                    */
                /*
                 *   TODO: 32位操作
                 */
                ATA_DEBUG_MSG(("__ataRW() error: 32 bits read operation\n"));
                return  (PX_ERROR);
            }

            psBuf     += iNWords;                                       /*  调整缓冲区的指针            */
            iNSectors -= iBlock;                                        /*  计算还需操作的扇区数        */
        }
    }

    ATA_DEBUG_MSG(("__ataRW(): end\n"));
    return  (ERROR_NONE);

__error_handle:
    ATA_DEBUG_MSG(("__ataRW err: stat=0x%x 0x%x  error=0x%x\n",
                   patactrl->ATACTRL_iIntStatus,
                   __ATA_CTRL_INBYTE(patactrl, __ATA_ASTATUS(patactrl)),
                   __ATA_CTRL_INBYTE(patactrl, __ATA_ERROR(patactrl))));
    (VOID)__ataCmd(patactrl, iDrive, __ATA_CMD_RECALIB, 0, 0);

    if (++iRetryCount < iRetry) {                                       /*  重试                        */
        goto    __retry_rw;
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __ataBlkRW
** 功能描述: ATA设备读写操作
** 输　入  : patadev     块设备数据结构指针
**           pvBuffer    缓冲区
**           ulStartBlk  读操作的起始块
**           ulNBlks     读的块数
**           iDirection  操作方向
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __ataBlkRW (__ATA_DEV  *patadev,
                       VOID       *pvBuffer,
                       ULONG       ulStartBlk,
                       ULONG       ulNBlks,
                       INT         iDirection)
{
    __PATA_CTRL   patactrl    = LW_NULL;
    __PATA_DRIVE  patadrive   = LW_NULL;
    LW_BLK_DEV   *pblkdev     = LW_NULL;
    __PATA_TYPE   patatype    = LW_NULL;

    ULONG         ulNSecs     = 0;
    INT           iRetryNum0  = 0;
    INT           iRretrySeek = 0;
    INT           iCylinder   = 0;
    INT           iHead       = 0;
    INT           iSector     = 0;
    INT           iReturn     = PX_ERROR;

    PCHAR         pcBuf       = LW_NULL;
    volatile INT  i;

    if ((!patadev) || (!pvBuffer)) {                                    /*  参数错误                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    patactrl = __ataSearchNode(patadev->ATAD_iCtrl);
    if (!patactrl) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (patactrl->ATACTRL_bIsExist != LW_TRUE) {                        /*  ATA设备不存在               */
        return  (PX_ERROR);
    }

    patadrive = &patactrl->ATACTRL_ataDrive[patadev->ATAD_iDrive];
    pblkdev   = &patadev->ATAD_blkdBlkDev;
    patatype  = patadrive->ATADRIVE_patatypeDriverInfo;

    ulNSecs   = pblkdev->BLKD_ulNSector;                                /*  ATA设备总扇区               */

    if ((ulStartBlk + ulNBlks) > ulNSecs) {                             /*  读写扇区大于设备的总扇区    */
        ATA_DEBUG_MSG(("Error, startBlk=%d nBlks=%d: 0 - %d\n",
                       ulStartBlk, ulNBlks, ulNSecs));
        return  (PX_ERROR);
    }

    ulStartBlk += patadev->ATAD_iBlkOffset;
    pcBuf       = (PCHAR)pvBuffer;

    ATACTRL_LOCK(patactrl);                                             /*  等待信号量                  */
    
    for (i = 0; i < ulNBlks; i += ulNSecs) {
        if (patadrive->ATADRIVE_sOkLba) {                               /*  使用逻辑块寻址方式          */
            iHead     = (ulStartBlk & __ATA_LBA_HEAD_MASK) >> 24;
            iCylinder = (ulStartBlk & __ATA_LBA_CYL_MASK) >> 8;
            iSector   = (ulStartBlk & __ATA_LBA_SECTOR_MASK);
        
        } else {                                                        /*  使用柱面/磁头/扇区方式      */
            iCylinder = (INT)(ulStartBlk / (patatype->ATATYPE_iSectors * patatype->ATATYPE_iHeads));
            iSector   = (INT)(ulStartBlk % (patatype->ATATYPE_iSectors * patatype->ATATYPE_iHeads));
            iHead     = iSector / patatype->ATATYPE_iSectors;
            iSector   = iSector % patatype->ATATYPE_iSectors + 1;
        }

        ulNSecs = ((ulNBlks - i) < __ATA_MAX_RW_SECTORS) ? (ulNBlks - i) : __ATA_MAX_RW_SECTORS;

        while (__ataRW(patactrl, patadev->ATAD_iDrive, iCylinder,
               iHead, iSector, pcBuf, ulNSecs, iDirection, pblkdev->BLKD_iRetry)) {
            (VOID)__ataCmd(patactrl, patadev->ATAD_iDrive, __ATA_CMD_RECALIB, 0, 0);
                                                                        /*  校准                        */
            if (++iRetryNum0 > pblkdev->BLKD_iRetry) {
                iReturn = PX_ERROR;
                goto __error_handle;
            }

            iRretrySeek = 0;

            while (__ataCmd(patactrl, patadev->ATAD_iDrive, __ATA_CMD_SEEK,
                   iCylinder, iHead)) {                                 /*  查询定位                    */
                if (++iRretrySeek > pblkdev->BLKD_iRetry) {
                    iReturn = PX_ERROR;
                    goto __error_handle;
                }
            }
        }

        ulStartBlk += ulNSecs;                                          /*  调整所需读取扇区的起始位置  */
        pcBuf      += pblkdev->BLKD_ulBytesPerSector * ulNSecs;         /*  调整缓冲的位置              */
    }

    iReturn = ERROR_NONE;

__error_handle:
    ATACTRL_UNLOCK(patactrl);                                           /*  发送信号量                  */

    return  (iReturn);
}
/*********************************************************************************************************
** 函数名称: __ataBlkRd
** 功能描述: ATA读操作
** 输　入  : patadev     块设备数据结构指针
**           ulStartBlk  写操作的起始块
**           ulNBlks     写的块数
**           pvBuffer    缓冲区
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __ataBlkRd (__PATA_DEV  patadev,
                       VOID       *pvBuffer,
                       ULONG       ulStartBlk,
                       ULONG       ulNBlks)
{
    return  (__ataBlkRW(patadev, pvBuffer, ulStartBlk, ulNBlks, O_RDONLY));
}
/*********************************************************************************************************
** 函数名称: __ataBlkWrt
** 功能描述: ATA写操作
** 输　入  : patadev     块设备数据结构指针
**           ulStartBlk  写操作的起始块
**           ulNBlks     写的块数
**           pvBuffer    缓冲区
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __ataBlkWrt (__PATA_DEV  patadev,
                        VOID       *pvBuffer,
                        ULONG       ulStartBlk,
                        ULONG       ulNBlks)
{
    return  (__ataBlkRW(patadev, pvBuffer, ulStartBlk, ulNBlks, O_WRONLY));
}
/*********************************************************************************************************
** 函数名称: __ataIoctl
** 功能描述: 块设备控制函数
** 输　入  : patadev     块设备数据结构指针
**           iCmd        命令
**           lArg        其它参数
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __ataIoctl (__PATA_DEV patadev,
                       INT        iCmd,
                       LONG       lArg)
{
    __PATA_CTRL     patactrl     = LW_NULL;
    ATA_DRV_FUNCS  *patadevfuscs = LW_NULL;
    struct timeval *timevalTemp  = LW_NULL;
    __PATA_DRIVE    hDrive;                                             /* 驱动器句柄                   */
    PLW_BLK_INFO    hBlkInfo;                                           /* 设备信息                     */

    INT             iReturn     = ERROR_NONE;

    if (!patadev) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    patactrl = __ataSearchNode(patadev->ATAD_iCtrl);
    if (!patactrl) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (patactrl->ATACTRL_bIsExist != LW_TRUE){                         /*  ATA设备不存在               */
        return  (PX_ERROR);
    }

    patadevfuscs = patactrl->ATACTRL_pataChan->pDrvFuncs;

    ATACTRL_LOCK(patactrl);                                             /*  等待信号量                  */
    
    switch (iCmd) {

    case FIOSYNC:
    case FIODATASYNC:
    case FIOSYNCMETA:
    case FIOFLUSH:                                                      /*  将缓存回写到磁盘            */
    case FIOUNMOUNT:                                                    /*  卸载卷                      */
    case FIODISKINIT:                                                   /*  初始化ATA设备               */
        break;

    case FIODISKCHANGE:                                                 /*  磁盘媒质发生变化            */
        patadev->ATAD_blkdBlkDev.BLKD_bDiskChange = LW_TRUE;
        break;

    case LW_BLKD_GET_SECSIZE:
        *(ULONG *)lArg = patadev->ATAD_blkdBlkDev.BLKD_ulBytesPerSector;
        break;

    case LW_BLKD_GET_BLKSIZE:
        *(ULONG *)lArg = patadev->ATAD_blkdBlkDev.BLKD_ulBytesPerBlock;
        break;

    case LW_BLKD_CTRL_INFO:
        hDrive = &patactrl->ATACTRL_ataDrive[patadev->ATAD_iDrive];
        hBlkInfo = (PLW_BLK_INFO)lArg;
        if (!hBlkInfo) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
        lib_bzero(hBlkInfo, sizeof(LW_BLK_INFO));
        hBlkInfo->BLKI_uiType = LW_BLKD_CTRL_INFO_TYPE_ATA;
        __ataDriveSerialInfoGet(hDrive, hBlkInfo->BLKI_cSerial,   LW_BLKD_CTRL_INFO_STR_SZ);
        __ataDriveFwRevInfoGet(hDrive,  hBlkInfo->BLKI_cFirmware, LW_BLKD_CTRL_INFO_STR_SZ);
        __ataDriveModelInfoGet(hDrive,  hBlkInfo->BLKI_cProduct,  LW_BLKD_CTRL_INFO_STR_SZ);
        break;

    case FIOWTIMEOUT:
        if (lArg) {
            timevalTemp = (struct timeval *)lArg;
            patactrl->ATACTRL_ulSyncSemTimeout = __timevalToTick(timevalTemp);
                                                                        /*  转换为系统时钟              */
        } else {
            patactrl->ATACTRL_ulSyncSemTimeout = LW_OPTION_WAIT_INFINITE;
        }
        break;

    case FIORTIMEOUT:
        if (lArg) {
            timevalTemp = (struct timeval *)lArg;
            patactrl->ATACTRL_ulSyncSemTimeout = __timevalToTick(timevalTemp);
                                                                        /*  转换为系统时钟              */
        } else {
            patactrl->ATACTRL_ulSyncSemTimeout = LW_OPTION_WAIT_INFINITE;
        }
        break;

    case __ATA_CMD_ENB_POWERMANAGE:
        iReturn = __ataCmd(patactrl,
                           patadev->ATAD_iDrive,
                           __ATA_CMD_SET_FEATURE,
                           __ATA_SUB_ENABLE_APM,
                           (INT)lArg);
        break;

    case __ATA_CMD_DIS_POWERMANAGE:
        iReturn = __ataCmd(patactrl,
                           patadev->ATAD_iDrive,
                           __ATA_CMD_SET_FEATURE,
                           __ATA_SUB_DISABLE_APM,
                           (INT)lArg);
        break;

    case __ATA_CMD_IDLEIMMEDIATE:                                       /*  使指定设备立即空闲          */
        iReturn = __ataCmd(patactrl,
                           patadev->ATAD_iDrive,
                           __ATA_CMD_IDLE_IMMEDIATE,
                           0, 0);
        break;

    case __ATA_CMD_STANDBYIMMEDIATE:                                    /*  使指定设备立即待机          */
        iReturn = __ataCmd(patactrl,
                           patadev->ATAD_iDrive,
                           __ATA_CMD_STANDBY_IMMEDIATE,
                           0, 0);
        break;

    case __ATA_CMD_HARDRESET:
        if (patadevfuscs->sysReset) {
            patadevfuscs->sysReset(patactrl->ATACTRL_pataChan, (INT)lArg);
        }
        break;

    default:
        iReturn = patadevfuscs->ioctl(patactrl->ATACTRL_pataChan, iCmd, (PVOID)lArg);
        if (iReturn != ERROR_NONE) {                                    /*  错误                        */
            _ErrorHandle(ERROR_IO_UNKNOWN_REQUEST);
        }
        break;
    }

    ATACTRL_UNLOCK(patactrl);                                           /*  发送信号量                  */

    return  (iReturn);
}
/*********************************************************************************************************
** 函数名称: ataStatus
** 功能描述: 检查ATA设备状态
** 输　入  : patadev       块设备数据结构指针
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __ataStatus (__PATA_DEV  patadev)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ataReset
** 功能描述: 复位ATA设备
** 输　入  : patadev       块设备数据结构指针
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __ataReset (__PATA_DEV  patadev)
{
    __PATA_CTRL  patactrl  = LW_NULL;
    __PATA_DRIVE patadrive = LW_NULL;

    if (!patadev) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    patactrl  = __ataSearchNode(patadev->ATAD_iCtrl);
    patadrive = &patactrl->ATACTRL_ataDrive[patactrl->ATACTRL_iDrives];

    if (!patactrl) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (patactrl->ATACTRL_bIsExist != LW_TRUE) {                        /*  ATA设备不存在               */
        return  (PX_ERROR);
    }

    if (patadrive->ATADRIVE_ucState != __ATA_DEV_MED_CH) {
        ATACTRL_LOCK(patactrl);

        __ATA_CTRL_OUTBYTE(patactrl, __ATA_DCONTROL(patactrl), (__ATA_CTL_RST | __ATA_CTL_IDS));

        __ATA_DELAYMS(100);

        __ATA_CTRL_OUTBYTE(patactrl,                                        \
                           __ATA_DCONTROL(patactrl),                        \
                          (UINT8)(patactrl->ATACTRL_bIntDisable << 1)); /*  清除复位,并设置中断位       */
        __ATA_DELAYMS(100);

        if (__ataDriveInit(patactrl, patadev->ATAD_iDrive) != ERROR_NONE) {
            ATACTRL_UNLOCK(patactrl);
            return  (PX_ERROR);
        }

        ATACTRL_UNLOCK(patactrl);
    }

    patadrive->ATADRIVE_ucState = __ATA_DEV_OK;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_AtaDrv
** 功能描述: 安装 ATA 设备驱动程序 (每次插卡都需要调用, 然后才能创建设备)
** 输　入  : patachan ATA设备通道结构指针
**           patacp   ATA设备通道参数
** 输　出  : ERROR
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT API_AtaDrv (ATA_CHAN  *patachan, ATA_CHAN_PARAM *patacp)
{
    __PATA_CTRL      patactrl   = LW_NULL;
    __PATA_DRIVE     patadrive  = LW_NULL;
    __PATA_TYPE      patatype   = LW_NULL;

    INT              iDrive;
    INT              iError     = 0;

    if ((!patachan) || (!patacp)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if ((!patachan->pDrvFuncs->ioctl)           ||
        (!patachan->pDrvFuncs->ioOutByte)       ||
        (!patachan->pDrvFuncs->ioInByte)        ||
        (!patachan->pDrvFuncs->ioOutWordString) ||
        (!patachan->pDrvFuncs->ioInWordString)  ||
        (!patachan->pDrvFuncs->callbackInstall)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (patacp->ATACP_iCtrlNum >= LW_CFG_ATA_MAX_CTLS) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "IDE controller number error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if ((patacp->ATACP_iDrives >= __ATA_MAX_DRIVES) ||
        (patacp->ATACP_iDrives <= 0)) {
        patacp->ATACP_iDrives = 1;
    }

    patactrl = __ataSearchNode(patacp->ATACP_iCtrlNum);
    if (patactrl != LW_NULL) {                                          /*  不为空,则已安装过驱动       */
        goto    __drive_init;
    }

    patactrl = (__PATA_CTRL)__SHEAP_ALLOC(sizeof(__ATA_CTRL));          /*  为该控制器分配内存          */
    if (!patactrl) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_bzero(patactrl, sizeof(__ATA_CTRL));

    /*
     *  保存关键参数
     */
    patactrl->ATACTRL_iCtrl           = patacp->ATACP_iCtrlNum;         /*  保存控制器号                */
    patactrl->ATACTRL_iDrives         = patacp->ATACP_iDrives;          /*  保存设备个数                */
    patactrl->ATACTRL_iBytesPerSector = patacp->ATACP_iBytesPerSector;
                                                                        /*  每扇区的大小                */
    patactrl->ATACTRL_iConfigType     = patacp->ATACP_iConfigType;
    patactrl->ATACTRL_ataReg          = patacp->ATACP_atareg;

    patactrl->ATACTRL_pataChan = patachan;

    patactrl->ATACTRL_ulMuteSem = API_SemaphoreMCreate("ata_mutesem",
                                                       LW_PRIO_DEF_CEILING,
                                                       LW_OPTION_WAIT_PRIORITY |
                                                       LW_OPTION_INHERIT_PRIORITY | 
                                                       LW_OPTION_DELETE_SAFE |
                                                       LW_OPTION_OBJECT_GLOBAL,
                                                       LW_NULL);        /*  建立互斥信号量              */
    if (!(patactrl->ATACTRL_ulMuteSem)) {
        __SHEAP_FREE(patactrl);                                         /*  释放内存                    */
        return  (PX_ERROR);
    }

    patactrl->ATACTRL_ulSyncSem = API_SemaphoreBCreate("ata_syncsem",
                                                       LW_TRUE,
                                                       LW_OPTION_OBJECT_GLOBAL,
                                                       LW_NULL);        /*  建立同步信号量              */
    if (!(patactrl->ATACTRL_ulSyncSem)) {
        API_SemaphoreMDelete(&patactrl->ATACTRL_ulMuteSem);
        __SHEAP_FREE(patactrl);                                         /*  释放内存                    */
        return  (PX_ERROR);
    }

    patactrl->ATACTRL_ulSyncSemTimeout = patacp->ATACP_ulSyncSemTimeout;/*  保存同步信号量超时时间      */

    LW_SPIN_INIT(&patactrl->ATACTRL_slLock);                            /*  初始化自旋锁                */

    __ATA_CTRL_CBINSTALL(patactrl, ATA_CALLBACK_CHECK_DEV,
                         (ATA_CALLBACK)__ataDevChk, (PVOID)patactrl);   /*  安装检测函数回调            */

    patactrl->ATACTRL_bPreadBeSwap = patacp->ATACP_bPreadBeSwap;
    
    if (patacp->ATACP_bIntEnable == LW_TRUE) {
        patactrl->ATACTRL_bIntDisable = LW_FALSE;                       /*  允许中断                    */
        __ATA_CTRL_CBINSTALL(patactrl, ATA_CALLBACK_WRITE_DATA,    \
                            (ATA_CALLBACK)__ataWrite, (PVOID)patactrl); /*  安装写操作函数回调          */

    } else {
        patactrl->ATACTRL_bIntDisable = LW_TRUE;                        /*  禁止中断                    */
    }

    /*
     *  为该控制器分配内存
     */
    patatype = (__PATA_TYPE)__SHEAP_ALLOC(sizeof(__ATA_TYPE) * (size_t)patactrl->ATACTRL_iDrives);
                                                                        /*  为该控制器分配内存          */
    if (!patatype) {
        API_SemaphoreMDelete(&patactrl->ATACTRL_ulMuteSem);
        API_SemaphoreBDelete(&patactrl->ATACTRL_ulSyncSem);
        __SHEAP_FREE(patactrl);                                         /*  释放内存                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "invalid controller number or number of drives.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_bzero(patatype, (sizeof(__ATA_TYPE) * patactrl->ATACTRL_iDrives));

    for (iDrive = 0; iDrive < patactrl->ATACTRL_iDrives; iDrive++) {
        patadrive = &patactrl->ATACTRL_ataDrive[iDrive];
        patadrive->ATADRIVE_ucState = __ATA_DEV_INIT;
        patadrive->ATADRIVE_ucType  = __ATA_TYPE_INIT;

        patadrive->ATADRIVE_patatypeDriverInfo = patatype;              /*  保存设备信息指针            */
        patatype++;
    }

    /*
     *  把设备加入链表
     */
    __ataAddNode(patactrl);

__drive_init:
    if (__ataCtrlReset(patactrl) != ERROR_NONE) {                       /*  复位控制器                  */
        ATA_DEBUG_MSG(("API_AtaDrv(): Controller %d reset failed\r\n", patactrl->ATACTRL_iCtrl));
        return  (PX_ERROR);
    }

    for (iDrive = 0; iDrive < patactrl->ATACTRL_iDrives; iDrive++) {
        if ((__ataDriveInit(patactrl, iDrive)) != ERROR_NONE) {
            iError++;
            ATA_DEBUG_MSG(("API_AtaDrv(): ataDriveInit failed on Channel %d Device %d\n",
                           patactrl->ATACTRL_iCtrl, iDrive));
            if (iError == patactrl->ATACTRL_iDrives) {                  /*  控制器两个设备初始化都失败  */
                return  (PX_ERROR);
            }
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_AtaDevCreate
** 功能描述: 创建ATA块设备
** 输　入  : iCtrl       控制器号,0为第一个控制器
**           iDrive      驱动号,0为主设备
**           iNBlocks    设备的块数量,0为使用整个磁盘
**           iBlkOffset  设备的偏移量
** 输　出  : ERROR
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
PLW_BLK_DEV API_AtaDevCreate (INT iCtrl,
                              INT iDrive,
                              INT iNBlocks,
                              INT iBlkOffset)
{
    __PATA_CTRL  patactrl   = LW_NULL;
    __PATA_DRIVE patadrive  = LW_NULL;
    __PATA_TYPE  patatype   = LW_NULL;
    __PATA_DEV   patadev    = LW_NULL;
    PLW_BLK_DEV  pblkdev    = LW_NULL;

    UINT         uiMaxBlks;

    if (iDrive >= __ATA_MAX_DRIVES) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
    }

    patactrl = __ataSearchNode(iCtrl);
    if (!patactrl) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    patadrive = &patactrl->ATACTRL_ataDrive[iDrive];
    patatype  = patadrive->ATADRIVE_patatypeDriverInfo;

    patadev = (__PATA_DEV)__SHEAP_ALLOC(sizeof(__ATA_DEV));
    if (!patadev) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (LW_NULL);
    }
    lib_bzero(patadev, sizeof(__ATA_DEV));                              /*  清空内存                    */

    pblkdev = &patadev->ATAD_blkdBlkDev;

    if ((patadrive->ATADRIVE_ucState == __ATA_DEV_OK) ||
        (patadrive->ATADRIVE_ucState == __ATA_DEV_MED_CH)) {
        if (patadrive->ATADRIVE_ucType == __ATA_TYPE_ATA) {
            if ((patadrive->ATADRIVE_sOkLba == 1)      &&
                (patadrive->ATADRIVE_uiCapacity != 0)  &&
                (patadrive->ATADRIVE_uiCapacity >
                (patatype->ATATYPE_iCylinders *
                 patatype->ATATYPE_iHeads     *
                 patatype->ATATYPE_iSectors))) {                        /*  LBA模式                     */
                uiMaxBlks = (UINT)(patadrive->ATADRIVE_uiCapacity - iBlkOffset);
            
            } else {                                                    /*  使用CHS模式                 */
                uiMaxBlks = ((patatype->ATATYPE_iCylinders *
                              patatype->ATATYPE_iHeads     *
                              patatype->ATATYPE_iSectors)  - iBlkOffset);
            }

            if (iNBlocks == 0) {
                iNBlocks = uiMaxBlks;
            }

            if (iNBlocks > uiMaxBlks) {
                iNBlocks = uiMaxBlks;
            }

            pblkdev->BLKD_pcName            = "ATA-Hard Disk";
            pblkdev->BLKD_pfuncBlkRd        = __ataBlkRd;               /*  读操作函数                  */
            pblkdev->BLKD_pfuncBlkWrt       = __ataBlkWrt;              /*  写操作函数                  */
            pblkdev->BLKD_pfuncBlkIoctl     = __ataIoctl;               /*  IOCTRL函数                  */
            pblkdev->BLKD_pfuncBlkReset     = __ataReset;               /*  复位函数                    */
            pblkdev->BLKD_pfuncBlkStatusChk = __ataStatus;              /*  状态检查                    */
            pblkdev->BLKD_ulNSector         = iNBlocks;                 /*  总扇区数                    */
            pblkdev->BLKD_ulBytesPerSector  = patactrl->ATACTRL_iBytesPerSector;
                                                                        /*  每扇区字节数                */
            pblkdev->BLKD_ulBytesPerBlock   = 4096;
                                                                        /*  每块字节数                  */
            pblkdev->BLKD_bRemovable        = LW_TRUE;                  /*  可移动                      */
            pblkdev->BLKD_bDiskChange       = LW_FALSE;                 /*  媒质没有改变                */
            pblkdev->BLKD_iRetry            = __ATA_RETRY_TIMES;        /*  重试次数                    */
            pblkdev->BLKD_iFlag             = O_RDWR;                   /*  读写属性                    */
            pblkdev->BLKD_iLogic            = 0;
            pblkdev->BLKD_uiLinkCounter     = 0;
            pblkdev->BLKD_pvLink            = LW_NULL;
            pblkdev->BLKD_uiPowerCounter    = 0;
            pblkdev->BLKD_uiInitCounter     = 0;

            patadev->ATAD_iCtrl             = iCtrl;                    /*  控制器号                    */
            patadev->ATAD_iDrive            = iDrive;                   /*  设备号                      */
            patadev->ATAD_iBlkOffset        = iBlkOffset;               /*  偏移量                      */
            patadev->ATAD_iNBlocks          = iNBlocks;                 /*  总块数                      */

            return  (pblkdev);                                          /*  返回块设备                  */

        } else if (patadrive->ATADRIVE_ucType == __ATA_TYPE_ATAPI) {
            /*
             * TODO:  ATAPI类型设备的支持
             */
        }
    }
    __SHEAP_FREE(patadev);                                              /*  释放内存                    */

    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: API_AtaDevDelete
** 功能描述: 删除ATA块设备
** 输　入  : pblkdev       块设备
** 输　出  : ERROR
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT API_AtaDevDelete (PLW_BLK_DEV pblkdev)
{
    __PATA_CTRL patactrl = LW_NULL;
    __PATA_DEV  patadev  = LW_NULL;

    if (!pblkdev) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    patadev  = (__PATA_DEV)pblkdev;
    patactrl = __ataSearchNode(patadev->ATAD_iCtrl);
    if (!patactrl) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    ATACTRL_LOCK(patactrl);
    __SHEAP_FREE(pblkdev);                                              /*  释放内存                    */
    ATACTRL_UNLOCK(patactrl);                                           /*  发送信号量                  */

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_ATA_EN > 0)         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
