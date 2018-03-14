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
** 文   件   名: sdmemory.c
**
** 创   建   人: Zeng.Bo (曾波)
**
** 文件创建日期: 2010 年 11 月 25 日
**
** 描        述: sd 记忆卡客户应用驱动源文件

** BUG:
2010.12.08  优化了 __sdmemTestBusy() 函数.
2010.12.08  SD 设备结构中加入块寻址标记, 以支持 SDHC 卡.
2011.01.12  增加对 SPI 的支持.
2011.03.25  修改 __sdMemIoctl() 函数, 增加设备状态检测.
2011.03.25  修改 API_SdMemDevCreate(), 用于底层驱动安装上层的回调.
2011.04.03  将 API_SdMemDevShowInfo() 改为 API_SdMemDevShow() 统一 SylxiOS Show 函数.
2011.04.03  更改 block io 层回调函数的参数.
2015.03.11  增加卡写保护功能.
2015.09.18  增加对保留扇区的处理.
2015.09.22  增加对 MMC/eMMC 扩展协议的支持.
2015.12.17  增加对 MMC/eMMC 兼容性处理.
2016.01.26  修正对 SPI 模式下的写块操作不符合协议的地方.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SDCARD_EN > 0)
#include "sdmemory.h"
#include "../core/sdcore.h"
#include "../core/sdcoreLib.h"
#include "../core/sdstd.h"
#include "../include/sddebug.h"
/*********************************************************************************************************
  sd 块设备内部结构
*********************************************************************************************************/
typedef struct __sd_blk_dev {
    LW_BLK_DEV            SDBLKDEV_blkDev;
    PLW_SDCORE_DEVICE     SDBLKDEV_pcoreDev;
    BOOL                  SDBLKDEV_bIsBlockAddr;                        /*  是否是块寻址                */
    BOOL                  SDBLKDEV_bNeedReSelect;                       /*  是否需要重新选择设备        */
    ULONG                 SDBLKDEV_ulSectorOff;                         /*  扇区访问偏移                */
    ULONG                 SDBLKDEV_ulDiskNSector;                       /*  挂载磁盘的实际扇区数量      */
    LW_BLK_INFO           SDBLKDEV_blkinfo;
    
	/*
     * 增加 SDM 后, 为了保持 API 不变, 增加以下成员
     */
    BOOL                  SDBLKDEV_bCoreDevSelf;                        /*  coredev 是自己创建(非SDM给) */
} __SD_BLK_DEV, *__PSD_BLK_DEV;
/*********************************************************************************************************
  内部宏
*********************************************************************************************************/
#define __SDMEM_BLKADDR(pdev)       (pdev->SDBLKDEV_bIsBlockAddr)
#define __SD_CID_PNAME(iN)          (sddevcid.DEVCID_pucProductName[iN])

#define __SD_DEV_RETRY              4
#define __SD_MILLION                1000000
/*********************************************************************************************************
  忙检测函数的等待类型
*********************************************************************************************************/
#define __SD_BUSY_TYPE_READ         0
#define __SD_BUSY_TYPE_RDYDATA      1
#define __SD_BUSY_TYPE_PROG         2
#define __SD_BUSY_TYPE_ERASE        3
#define __SD_BUSY_RETRY             0x3fffffff
#define __SD_TIMEOUT_SEC            2                                   /*  2秒超时为一个经验值         */

#define __SD_CARD_STATUS_MSK        (0x0f << 9)
#define __SD_CARD_STATUS_PRG        (0x07 << 9)                         /*  数据正在编程                */
#define __SD_CARD_STATUS_RDYDATA    (0x01 << 8)
/*********************************************************************************************************
  私有函数声明
*********************************************************************************************************/
static INT __sdMemTestBusy(PLW_SDCORE_DEVICE psdcoredevice, INT iType);
static INT __sdMemInit(PLW_SDCORE_DEVICE psdcoredevice);
static INT __sdMemWrtSingleBlk(PLW_SDCORE_DEVICE  psdcoredevice,
                               UINT8             *pucBuf,
                               UINT32             uiStartBlk);
static INT __sdMemWrtMultiBlk(PLW_SDCORE_DEVICE  psdcoredevice,
                              UINT8             *pucBuf,
                              UINT32             uiStartBlk,
                              UINT32             uiNBlks);
static INT __sdMemRdSingleBlk(PLW_SDCORE_DEVICE  psdcoredevice,
                              UINT8             *pucBuf,
                              UINT32             uiStartBlk);
static INT __sdMemRdMultiBlk(PLW_SDCORE_DEVICE  psdcoredevice,
                             UINT8             *pucBuf,
                             UINT32             uiStartBlk,
                             UINT32             uiNBlks);

static INT __sdMemBlkWrt(__PSD_BLK_DEV   psdblkdevice,
                        VOID            *pvWrtBuffer,
                        ULONG            ulStartBlk,
                        ULONG            ulBlkCount);
static INT __sdMemBlkRd(__PSD_BLK_DEV   psdblkdevice,
                        VOID           *pvRdBuffer,
                        ULONG           ulStartBlk,
                        ULONG           ulBlkCount);
static INT __sdMemIoctl(__PSD_BLK_DEV    psdblkdevice,
                        INT              iCmd,
                        LONG             lArg);

static INT __sdMemBlkInfoFmt(__PSD_BLK_DEV psdblkdevice);
static INT __sdMemStatus(__PSD_BLK_DEV     psdblkdevice);
static INT __sdMemReset(__PSD_BLK_DEV      psdblkdevice);

static INT __sdMemSdSwCapGet(PLW_SDCORE_DEVICE  psdcoredevice,
                             LW_SDDEV_SCR      *psdscr,
                             LW_SDDEV_CSD      *psdcsd,
                             LW_SDDEV_SW_CAP   *psdswcap);
static INT __sdMemSdHsSwitch(PLW_SDCORE_DEVICE  psdcoredevice, LW_SDDEV_SW_CAP  *psdswcap);

static INT __sdMemMmcFreqChange(PLW_SDCORE_DEVICE  psdcoredevice,
                                LW_SDDEV_CSD      *psdcsd,
                                INT               *piCardCap);
static INT __sdMemMmcBusWidthChange(PLW_SDCORE_DEVICE psdcoredevice, INT iCardCap);
static CPCHAR __sdMemProtVsnStr(UINT8 ucType, UINT8 ucVsn);
/*********************************************************************************************************
** 函数名称: API_SdMemDevCreate
** 功能描述: 创建一个SD记忆卡设备
** 输    入: iAdapterType     设备挂接的适配器类型 (SDADAPTER_TYPE_SPI 或 SDADAPTER_TYPE_SD)
**           pcAdapterName    挂接的适配器名称
**           pcDeviceName     设备名称
**           psdmemchan       通道
** 输    出: NONE
** 返    回: 成功,返回设备块设备指针,否则返回LW_NULL
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API PLW_BLK_DEV API_SdMemDevCreate (INT                       iAdapterType,
                                       CPCHAR                    pcAdapterName,
                                       CPCHAR                    pcDeviceName,
                                       PLW_SDMEM_CHAN            psdmemchan)
{
    PLW_SDCORE_DEVICE   psdcoredevice   = LW_NULL;
    __PSD_BLK_DEV       psdblkdevice    = LW_NULL;
    PLW_BLK_DEV         pblkdevice      = LW_NULL;
    PLW_SDCORE_CHAN     psdcorechan     = LW_NULL;
    BOOL                bCoreDevSelf    = LW_TRUE;

    LW_SDDEV_CSD        sddevcsd;
    BOOL                bBlkAddr;
    INT                 iBlkDevFlag;
    ULONG               ulSectorOff;
    ULONG               ulReSelect;
    INT                 iError;

    /*
     * 增加了 SDM 后, 约定：当适配器名称和设备名称为空时, 表示 coredev 由 SDM 创建
     * 此时, psdmemchan 指向对应的coredev
     */
    if (!pcAdapterName && !pcDeviceName) {
        psdcoredevice = (PLW_SDCORE_DEVICE)psdmemchan;
        bCoreDevSelf  = LW_FALSE;

    } else {
        psdcorechan   = (PLW_SDCORE_CHAN)psdmemchan;
        psdcoredevice = API_SdCoreDevCreate(iAdapterType,
                                            pcAdapterName,
                                            pcDeviceName,
                                            psdcorechan);
    }

    if (!psdcoredevice) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "create coredevice failed.\r\n");
        return  (LW_NULL);
    }

    /*
     * 初始化这个设备(针对记忆卡)
     */
    iError = __sdMemInit(psdcoredevice);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "do memory initialize failed.\r\n");

        if (bCoreDevSelf) {
            API_SdCoreDevDelete(psdcoredevice);
        }

        return  (LW_NULL);
    }

    psdblkdevice  = (__PSD_BLK_DEV)__SHEAP_ALLOC(sizeof(__SD_BLK_DEV));
    if (!psdblkdevice) {
        if (bCoreDevSelf) {
            API_SdCoreDevDelete(psdcoredevice);
        }

        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (LW_NULL);

    }

    iError = API_SdCoreDevCsdView(psdcoredevice, &sddevcsd);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "view csd of device failed.\r\n");
        __SHEAP_FREE(psdblkdevice);

        if (bCoreDevSelf) {
            API_SdCoreDevDelete(psdcoredevice);
        }

        return  (LW_NULL);
    }

    iError = API_SdCoreDevIsBlockAddr(psdcoredevice, &bBlkAddr);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "unkonwn address access way.\r\n");
        __SHEAP_FREE(psdblkdevice);

        if (bCoreDevSelf) {
            API_SdCoreDevDelete(psdcoredevice);
        }

        return  (LW_NULL);
    }

    if (API_SdmHostIsCardWp(psdcoredevice)) {
        iBlkDevFlag = O_RDONLY;
    } else {
        iBlkDevFlag = O_RDWR;
    }

    API_SdmHostExtOptGet(psdcoredevice,
                         SDHOST_EXTOPT_CONFIG_FLAG_GET,
                         (LONG)&ulReSelect);

    if (ulReSelect & SDHOST_EXTOPT_CONFIG_RESELECT_SDMEM) {
        psdblkdevice->SDBLKDEV_bNeedReSelect = LW_TRUE;
    } else {
        psdblkdevice->SDBLKDEV_bNeedReSelect = LW_FALSE;
        API_SdCoreDevSelect(psdcoredevice);                             /*  只在这里调用一次            */
    }

    API_SdmHostExtOptGet(psdcoredevice,
                         SDHOST_EXTOPT_RESERVE_SECTOR_GET,
                         (LONG)&ulSectorOff);

    if (sddevcsd.DEVCSD_uiCapacity <= ulSectorOff) {
        ulSectorOff = 0;
    }

    psdblkdevice->SDBLKDEV_bIsBlockAddr  = bBlkAddr;                    /*  设置寻址方式                */
    psdblkdevice->SDBLKDEV_pcoreDev      = psdcoredevice;               /*  连接核心设备                */
    psdblkdevice->SDBLKDEV_bCoreDevSelf  = bCoreDevSelf;
    psdblkdevice->SDBLKDEV_ulSectorOff   = ulSectorOff;
    psdblkdevice->SDBLKDEV_ulDiskNSector = sddevcsd.DEVCSD_uiCapacity - ulSectorOff;

    pblkdevice = &psdblkdevice->SDBLKDEV_blkDev;

    pblkdevice->BLKD_pcName            = "SD-MemCard";
    pblkdevice->BLKD_pfuncBlkRd        = __sdMemBlkRd;
    pblkdevice->BLKD_pfuncBlkWrt       = __sdMemBlkWrt;
    pblkdevice->BLKD_pfuncBlkIoctl     = __sdMemIoctl;
    pblkdevice->BLKD_pfuncBlkReset     = __sdMemReset;
    pblkdevice->BLKD_pfuncBlkStatusChk = __sdMemStatus;

    pblkdevice->BLKD_ulNSector         = psdblkdevice->SDBLKDEV_ulDiskNSector;

    pblkdevice->BLKD_ulBytesPerSector  = SD_MEM_DEFAULT_BLKSIZE;
    pblkdevice->BLKD_ulBytesPerBlock   = SD_MEM_DEFAULT_BLKSIZE;

    pblkdevice->BLKD_bRemovable        = LW_TRUE;
    pblkdevice->BLKD_bDiskChange       = LW_FALSE;                      /*  媒质没有改变                */
    pblkdevice->BLKD_iRetry            = __SD_DEV_RETRY;                /*  重试次数                    */
    pblkdevice->BLKD_iFlag             = iBlkDevFlag;
    pblkdevice->BLKD_iLogic            = 0;
    pblkdevice->BLKD_uiLinkCounter     = 0;
    pblkdevice->BLKD_pvLink            = LW_NULL;
    pblkdevice->BLKD_uiPowerCounter    = 0;
    pblkdevice->BLKD_uiInitCounter     = 0;

    __sdMemBlkInfoFmt(psdblkdevice);                                    /*  格式化块设备信息            */

    return  (pblkdevice);
}
/*********************************************************************************************************
** 函数名称: API_SdMemDevDelete
** 功能描述: 删除一个SD记忆卡设备
** 输    入: pblkdevice 块设备结构指针
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API INT  API_SdMemDevDelete (PLW_BLK_DEV pblkdevice)
{
    PLW_SDCORE_DEVICE   psdcoredevice   = LW_NULL;
    __PSD_BLK_DEV       psdblkdevice    = LW_NULL;
    INT                 iError;

    if (!pblkdevice) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    psdblkdevice  = (__PSD_BLK_DEV)pblkdevice;
    psdcoredevice = psdblkdevice->SDBLKDEV_pcoreDev;

    if (psdcoredevice && psdblkdevice->SDBLKDEV_bCoreDevSelf) {
        iError = API_SdCoreDevDelete(psdcoredevice);                    /*  先删除core设备              */
        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "delet coredevice failed.\r\n");
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
    }

    __SHEAP_FREE(psdblkdevice);                                         /*  再释放sd blk                */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_SdMemDevShow
** 功能描述: 打印SD设备信息
** 输    入: pblkdevice  块设备结构指针
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API INT  API_SdMemDevShow (PLW_BLK_DEV pblkdevice)
{
    PLW_SDCORE_DEVICE   psdcoredevice   = LW_NULL;
    __PSD_BLK_DEV       psdblkdevice    = LW_NULL;
    LW_SDDEV_CSD        sddevcsd;
    LW_SDDEV_CID        sddevcid;
    LW_SDDEV_SW_CAP     sddevswcap;

    UINT32              uiCapMod;
    UINT64              ullCap;
    UINT32              uiMaxSpeed;
    UINT8               ucType;
    CPCHAR              pcTypeStr = "unknown";
    CPCHAR              pcVsnStr  = "unknown";

    if (!pblkdevice) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (API_IoTaskStdGet(API_ThreadIdSelf(), STD_OUT) < 0) {
        return  (ERROR_NONE);
    }

    psdblkdevice   = (__PSD_BLK_DEV)pblkdevice;
    psdcoredevice = psdblkdevice->SDBLKDEV_pcoreDev;

    if (!psdcoredevice) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "no core device.\r\n");
        return  (PX_ERROR);
    }

    API_SdCoreDevCsdView(psdcoredevice, &sddevcsd);
    API_SdCoreDevCidView(psdcoredevice, &sddevcid);
    API_SdCoreDevSwCapView(psdcoredevice, &sddevswcap);
    API_SdCoreDevTypeView(psdcoredevice, &ucType);

    if (sddevswcap.DEVSWCAP_uiHsMaxDtr) {
        uiMaxSpeed = sddevswcap.DEVSWCAP_uiHsMaxDtr;
    } else {
        uiMaxSpeed = sddevcsd.DEVCSD_uiTranSpeed;
    }

    ullCap   = (UINT64)sddevcsd.DEVCSD_uiCapacity * ((UINT64)1 << sddevcsd.DEVCSD_ucReadBlkLenBits);
    uiCapMod = ullCap % LW_CFG_MB_SIZE;

    switch (ucType) {

    case SDDEV_TYPE_MMC:
        pcTypeStr = "MMC/eMMC";
        break;

    case SDDEV_TYPE_SDSC:
        pcTypeStr = "SDSC";
        break;

    case SDDEV_TYPE_SDHC:
        pcTypeStr = "SDHC";
        break;

    case SDDEV_TYPE_SDXC:
        pcTypeStr = "SDXC";
        break;

    default:
        break;
    }

    pcVsnStr = __sdMemProtVsnStr(ucType, sddevcsd.DEVCSD_ucStructure);

    printf("\nSD Memory Information >>\n");
    printf("Manufacturer : 0x%02X\n", sddevcid.DEVCID_ucMainFid);
    if (ucType == SDDEV_TYPE_MMC) {
        printf("OEM ID       : %08X\n", sddevcid.DEVCID_usOemId);
    } else {
        printf("OEM ID       : %c%c\n", sddevcid.DEVCID_usOemId >> 8,
                                        sddevcid.DEVCID_usOemId & 0xff);
    }

    printf("Media Type   : %s(%s)\n", pcTypeStr, pcVsnStr);
    printf("Product Name : %c%c%c%c%c\n",
                           __SD_CID_PNAME(0),
                           __SD_CID_PNAME(1),
                           __SD_CID_PNAME(2),
                           __SD_CID_PNAME(3),
                           __SD_CID_PNAME(4));
    printf("Product Vsn  : v%d.%d\n", sddevcid.DEVCID_ucProductVsn >> 4,
                                      sddevcid.DEVCID_ucProductVsn & 0xf);
    printf("Serial Num   : %X\n", sddevcid.DEVCID_uiSerialNum);
    printf("Date         : %d/%02d\n", sddevcid.DEVCID_uiYear, sddevcid.DEVCID_ucMonth);
    printf("Max Speed    : %dMB/s\n", uiMaxSpeed / __SD_MILLION);
    printf("Capacity     : %u.%03u MB\n", (UINT32)(ullCap / LW_CFG_MB_SIZE), uiCapMod / 1000);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdMemTestBusy
** 功能描述: 忙检测函数.在写时使用
** 输    入: psdcoredevice  核心设备
**           iType          检测类型
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdMemTestBusy (PLW_SDCORE_DEVICE psdcoredevice, INT iType)
{
    INT     iError;
    UINT    uiSta;
    INT     iRetry = 0;

    struct timespec   tvOld;
    struct timespec   tvNow;

    lib_clock_gettime(CLOCK_MONOTONIC, &tvOld);

    while (iRetry++ < __SD_BUSY_RETRY) {
        iError = API_SdCoreDevGetStatus(psdcoredevice, &uiSta);

        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "get device status failed.\r\n");
            break;
        }

        if ((uiSta & __SD_CARD_STATUS_MSK) != __SD_CARD_STATUS_PRG) {
            return   (ERROR_NONE);
        }

        lib_clock_gettime(CLOCK_MONOTONIC, &tvNow);
        if ((tvNow.tv_sec - tvOld.tv_sec) >= __SD_TIMEOUT_SEC) {        /*  超时退出                    */
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "timeout.\r\n");
            break;
        }
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __sdMemInit
** 功能描述: SD记忆卡初始化
** 输    入: psdcoredevice  核心设备
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdMemInit (PLW_SDCORE_DEVICE psdcoredevice)
{
    INT             iError;
    UINT8           ucType;
    UINT32          uiOCR;
    UINT32          uiRCA;
    LW_SDDEV_OCR    sddevocr;
    LW_SDDEV_CID    sddevcid;
    LW_SDDEV_CSD    sddevcsd;
    LW_SDDEV_SCR    sddevscr;
    LW_SDDEV_SW_CAP sddevswcap;
    INT             iCardCap = 0;
    INT             iHostCap = 0;

    lib_bzero(&sddevcid, sizeof(LW_SDDEV_CID));
    lib_bzero(&sddevcsd, sizeof(LW_SDDEV_CSD));
    lib_bzero(&sddevscr, sizeof(LW_SDDEV_SCR));
    lib_bzero(&sddevswcap, sizeof(LW_SDDEV_SW_CAP));

    switch (psdcoredevice->COREDEV_iAdapterType) {

    case SDADAPTER_TYPE_SD:
        API_SdCoreDevCtl(psdcoredevice, SDBUS_CTRL_POWEROFF, 0);
        bspDelayUs(10);
        API_SdCoreDevCtl(psdcoredevice, SDBUS_CTRL_POWERON, 0);

        iError = API_SdCoreDevCtl(psdcoredevice,
                                  SDBUS_CTRL_SETCLK,
                                  SDARG_SETCLK_LOW);                    /*  初始化时 低速时钟           */
        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "set clock to low failed.\r\n");
            return  (PX_ERROR);
        }

        SD_DELAYMS(10);
        API_SdCoreDevCtl(psdcoredevice, SDBUS_CTRL_SETBUSWIDTH, SDARG_SETBUSWIDTH_1);

        iError = API_SdCoreDevReset(psdcoredevice);                     /*  cmd0 go idle                */
        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "device reset failed.\r\n");
            return  (PX_ERROR);
        }

        iError = API_SdCoreDevCtl(psdcoredevice,
                                  SDBUS_CTRL_GETOCR,
                                  (LONG)&uiOCR);

        iError = API_SdCoreDevSendIfCond(psdcoredevice);                /*  cmd8 (v2.0以上的卡必须这个) */
                                                                        /*  v2.0以下的卡无应答,忽略错误 */
        if (iError == ERROR_NONE) {                                     /*  SDHC卡初始化支持            */
            uiOCR |= SD_OCR_HCS;
        }

        iError = API_SdCoreDevSendAppOpCond(psdcoredevice,
                                            uiOCR,
                                            &sddevocr,
                                            &ucType);                   /*  acmd41                      */
        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "device don't support the ocr.\r\n");
            return  (PX_ERROR);
        }

        /*
         * 这里保存设备 OCR 信息, 随后会根据此信息判断设备是否真的是
         * 字节寻址还是块寻址
         */
        if (sddevocr & SD_OCR_HCS) {
            psdcoredevice->COREDEV_iDevSta |= COREDEV_STA_HIGHCAP_OCR;
        }

        API_SdCoreDevTypeSet(psdcoredevice, ucType);                    /*  设置type域                  */

        iError = API_SdCoreDevSendAllCID(psdcoredevice, &sddevcid);     /*  cmd2                        */
        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "get device cid failed.\r\n");
            return  (PX_ERROR);
        }
        if (ucType == SDDEV_TYPE_MMC) {
            uiRCA = 0x01;
            iError = API_SdCoreDevMmcSetRelativeAddr(psdcoredevice, uiRCA);
        } else {
            iError = API_SdCoreDevSendRelativeAddr(psdcoredevice, &uiRCA);
                                                                        /*  cmd3                        */
        } if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "get device rca failed.\r\n");
            return  (PX_ERROR);
        }

        API_SdCoreDevRcaSet(psdcoredevice, uiRCA);                      /*  设置RCA域                   */

        iError = API_SdCoreDevSendAllCSD(psdcoredevice, &sddevcsd);
        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "get device csd failed.\r\n");
            return  (PX_ERROR);
        }

        if (ucType == SDDEV_TYPE_MMC) {
            LW_SDDEV_EXT_CSD  sddevextcsd;
            lib_bzero(&sddevextcsd, sizeof(sddevextcsd));
            API_SdCoreDevSelect(psdcoredevice);
            API_SdCoreDecodeExtCSD(psdcoredevice, &sddevcsd, &sddevextcsd);
            API_SdCoreDevDeSelect(psdcoredevice);
        }

        sddevcsd.DEVCSD_uiCapacity       <<= sddevcsd.DEVCSD_ucReadBlkLenBits - \
                                             SD_MEM_DEFAULT_BLKSIZE_NBITS;
        sddevcsd.DEVCSD_ucReadBlkLenBits   = SD_MEM_DEFAULT_BLKSIZE_NBITS;
        sddevcsd.DEVCSD_ucWriteBlkLenBits  = SD_MEM_DEFAULT_BLKSIZE_NBITS;

        API_SdCoreDevCsdSet(psdcoredevice, &sddevcsd);                  /*  设置CSD域                   */
        API_SdCoreDevCidSet(psdcoredevice, &sddevcid);                  /*  设置CID域                   */

        if (ucType != SDDEV_TYPE_MMC) {
            iError = API_SdCoreDevCtl(psdcoredevice,
                                      SDBUS_CTRL_SETCLK,
                                      SDARG_SETCLK_NORMAL);
            if (iError != ERROR_NONE) {
                SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "set clock to normal failed.\r\n");
                return  (PX_ERROR);
            }

            API_SdmHostCapGet(psdcoredevice, &iHostCap);

            API_SdCoreDevSelect(psdcoredevice);
            iError = API_SdCoreDevSendAllSCR(psdcoredevice, &sddevscr);
            if ((iError == ERROR_NONE) && (iHostCap & SDHOST_CAP_HIGHSPEED)) {
                API_SdCoreDevScrSet(psdcoredevice, &sddevscr);          /*  设置SCR域                   */
                iError = __sdMemSdSwCapGet(psdcoredevice,
                                           &sddevscr,
                                           &sddevcsd,
                                           &sddevswcap);
                if (iError == ERROR_NONE) {
                    API_SdCoreDevSwCapSet(psdcoredevice, &sddevswcap);  /*  设置SWCAP域                 */
                    iError = __sdMemSdHsSwitch(psdcoredevice, &sddevswcap);
                    if (iError == ERROR_NONE) {
                        API_SdCoreDevCtl(psdcoredevice,
                                         SDBUS_CTRL_SETCLK,
                                         SDARG_SETCLK_MAX);
                    }
                }
            }
            API_SdCoreDevDeSelect(psdcoredevice);

            iError = API_SdCoreDevSetBlkLen(psdcoredevice, SD_MEM_DEFAULT_BLKSIZE);
            if (iError != ERROR_NONE) {
                SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "set blklen failed.\r\n");
                return  (PX_ERROR);
            }

            iError = API_SdCoreDevSetBusWidth(psdcoredevice, SDARG_SETBUSWIDTH_4);
                                                                        /*  acmd6 set bus width         */
            if (iError != ERROR_NONE) {
                SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "set bus width error.\r\n");
                return  (PX_ERROR);
            }
            API_SdCoreDevCtl(psdcoredevice, SDBUS_CTRL_SETBUSWIDTH, SDARG_SETBUSWIDTH_4);

        } else {                                                        /*  mmc 总线特殊设置            */
            iError = API_SdCoreDevSetBlkLen(psdcoredevice, SD_MEM_DEFAULT_BLKSIZE);
            if (iError != ERROR_NONE) {
                SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "set blklen failed.\r\n");
                return  (PX_ERROR);
            }

            API_SdCoreDevSelect(psdcoredevice);
            iError = __sdMemMmcFreqChange(psdcoredevice, &sddevcsd, &iCardCap);
            if (iError != ERROR_NONE) {
                SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "mmc change frequency error.\r\n");
                return  (PX_ERROR);
            }

            iError = __sdMemMmcBusWidthChange(psdcoredevice, iCardCap);
            if (iError != ERROR_NONE) {
                SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "mmc change bus width error.\r\n");
                return  (PX_ERROR);
            }
            API_SdCoreDevDeSelect(psdcoredevice);
        }
        return  (ERROR_NONE);

    case SDADAPTER_TYPE_SPI:
        API_SdCoreDevCtl(psdcoredevice, SDBUS_CTRL_POWERON, 0);
        SD_DELAYMS(3);

        iError = API_SdCoreDevCtl(psdcoredevice,
                                  SDBUS_CTRL_SETCLK,
                                  SDARG_SETCLK_LOW);                    /*  初始化时 低速时钟           */
        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "set clock to low failed.\r\n");
            return  (PX_ERROR);
        }

        API_SdCoreDevSpiClkDely(psdcoredevice, 100);                    /*  延时大于74个时钟            */

        iError = API_SdCoreDevReset(psdcoredevice);                     /*  cmd0 go idle                */
        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "device reset failed.\r\n");
            return  (PX_ERROR);
        }

        iError = API_SdCoreDevCtl(psdcoredevice,
                                  SDBUS_CTRL_GETOCR,
                                  (LONG)&uiOCR);

        iError = API_SdCoreSpiSendIfCond(psdcoredevice);                /*  cmd8 (v2.0以上的卡必须这个) */
                                                                        /*  v2.0以下的卡无应答,忽略错误 */

        uiOCR |= SD_OCR_HCS;                                            /*  SDHC卡初始化支持            */

        iError = API_SdCoreDevSendAppOpCond(psdcoredevice,
                                            uiOCR,
                                            &sddevocr,
                                            &ucType);                   /*  acmd41   +  cmd58(spi)      */
        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "device don't support the ocr.\r\n");
            return  (PX_ERROR);
        }

        /*
         * 这里保存设备 OCR 信息, 随后会根据此信息判断设备是否真的是
         * 字节寻址还是块寻址
         */
        if (sddevocr & SD_OCR_HCS) {
            psdcoredevice->COREDEV_iDevSta |= COREDEV_STA_HIGHCAP_OCR;
        }

        API_SdCoreDevTypeSet(psdcoredevice, ucType);                    /*  设置type域                  */

#if LW_CFG_SDCARD_CRC_EN > 0
        API_SdCoreDevSpiCrcEn(psdcoredevice, LW_TRUE);                  /*  使能crc                     */
#endif

        iError = API_SdCoreDevSetBlkLen(psdcoredevice, SD_MEM_DEFAULT_BLKSIZE);
        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "set blklen failed.\r\n");
            return  (PX_ERROR);
        }

        iError = API_SdCoreDevSendAllCID(psdcoredevice, &sddevcid);     /*  cmd2                        */
        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "get device cid failed.\r\n");
            return  (PX_ERROR);
        }

        iError = API_SdCoreDevSendAllCSD(psdcoredevice, &sddevcsd);
        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "get device csd failed.\r\n");
            return  (PX_ERROR);
        }

        if (ucType == SDDEV_TYPE_MMC) {
            LW_SDDEV_EXT_CSD  sddevextcsd;
            lib_bzero(&sddevextcsd, sizeof(sddevextcsd));
            API_SdCoreDecodeExtCSD(psdcoredevice, &sddevcsd, &sddevextcsd);
        }

        sddevcsd.DEVCSD_uiCapacity       <<= sddevcsd.DEVCSD_ucReadBlkLenBits - \
                                             SD_MEM_DEFAULT_BLKSIZE_NBITS;
        sddevcsd.DEVCSD_ucReadBlkLenBits   = SD_MEM_DEFAULT_BLKSIZE_NBITS;
        sddevcsd.DEVCSD_ucWriteBlkLenBits  = SD_MEM_DEFAULT_BLKSIZE_NBITS;

        API_SdCoreDevCsdSet(psdcoredevice, &sddevcsd);                  /*  设置CSD域                   */
        API_SdCoreDevCidSet(psdcoredevice, &sddevcid);                  /*  设置CID域                   */

        iError = API_SdCoreDevCtl(psdcoredevice,
                                  SDBUS_CTRL_SETCLK,
                                  SDARG_SETCLK_NORMAL);                 /*  设置为全速时钟              */
        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "set to normal clock mode failed.\r\n");
        }
        return  (ERROR_NONE);

    default:
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "unknown adapter type.\r\n");
        break;
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __sdMemWrtSingleBlk
** 功能描述: SD记忆卡设备写单块
** 输    入: psdcoredevice 核心设备结构
**           pucBuf        写缓冲
**           uiStartBlk    起始地址
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdMemWrtSingleBlk (PLW_SDCORE_DEVICE  psdcoredevice,
                                UINT8             *pucBuf,
                                UINT32             uiStartBlk)
{
    LW_SD_MESSAGE   sdmsg;
    LW_SD_COMMAND   sdcmd;
    LW_SD_DATA      sddat;
    INT             iError;
    INT             iDevSta;

    iDevSta = API_SdCoreDevStaView(psdcoredevice);
    if (iDevSta != SD_DEVSTA_EXIST) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "device is not exist.\r\n");
        return  (PX_ERROR);
    }

    lib_bzero(&sdcmd, sizeof(sdcmd));
    lib_bzero(&sddat, sizeof(sddat));

    sdcmd.SDCMD_uiOpcode   = SD_WRITE_BLOCK;
    sdcmd.SDCMD_uiArg      = uiStartBlk;
    sdcmd.SDCMD_uiFlag     = SD_RSP_SPI_R1 | SD_RSP_R1 | SD_CMD_ADTC;   /*  命令                        */

    sddat.SDDAT_uiBlkNum   = 1;
    sddat.SDDAT_uiBlkSize  = SD_MEM_DEFAULT_BLKSIZE;
    sddat.SDDAT_uiFlags    = SD_DAT_WRITE;                              /*  数据                        */

    sdmsg.SDMSG_pucWrtBuffer = pucBuf;
    sdmsg.SDMSG_psddata      = &sddat;
    sdmsg.SDMSG_psdcmdCmd    = &sdcmd;
    sdmsg.SDMSG_psdcmdStop   = LW_NULL;
    sdmsg.SDMSG_pucRdBuffer  = LW_NULL;                                 /*  读缓冲为空                  */

    iError = API_SdCoreDevTransfer(psdcoredevice, &sdmsg, 1);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "request error.\r\n");
        return  (PX_ERROR);
    }

    if (COREDEV_IS_SD(psdcoredevice)) {
        iError = __sdMemTestBusy(psdcoredevice, __SD_BUSY_TYPE_PROG);
        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "check busy error.\r\n");
            return  (PX_ERROR);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdMemWrtMultiBlk
** 功能描述: SD记忆卡设备写多块
** 输    入: psdcoredevice 核心设备结构
**           pucBuf      写缓冲
**           uiStartBlk  起始地址
**           uiNBlks     块数量
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdMemWrtMultiBlk (PLW_SDCORE_DEVICE  psdcoredevice,
                               UINT8             *pucBuf,
                               UINT32             uiStartBlk,
                               UINT32             uiNBlks)
{
    LW_SD_MESSAGE   sdmsg;
    LW_SD_COMMAND   sdcmd;
    LW_SD_DATA      sddat;
    LW_SD_COMMAND   sdcmdStop;
    INT             iError;
    INT             iDevSta;

    iDevSta = API_SdCoreDevStaView(psdcoredevice);
    if (iDevSta != SD_DEVSTA_EXIST) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "device is not exist.\r\n");
        return  (PX_ERROR);
    }

    lib_bzero(&sdcmd, sizeof(sdcmd));
    lib_bzero(&sdcmdStop, sizeof(sdcmdStop));
    lib_bzero(&sddat, sizeof(sddat));

    sdcmd.SDCMD_uiOpcode  = SD_WRITE_MULTIPLE_BLOCK;
    sdcmd.SDCMD_uiArg     = uiStartBlk;
    sdcmd.SDCMD_uiFlag    = SD_RSP_SPI_R1 | SD_RSP_R1 | SD_CMD_ADTC;    /*  命令                        */

    sddat.SDDAT_uiBlkNum  = uiNBlks;
    sddat.SDDAT_uiBlkSize = SD_MEM_DEFAULT_BLKSIZE;
    sddat.SDDAT_uiFlags   = SD_DAT_WRITE;                               /*  数据                        */

    /*
     * 在多块写传输中,要发送停止命令.该命令在SD和SPI模式下不同
     */
    if (COREDEV_IS_SD(psdcoredevice)) {
        sdcmdStop.SDCMD_uiOpcode = SD_STOP_TRANSMISSION;
        sdcmdStop.SDCMD_uiFlag   = SD_RSP_SPI_R1B | SD_RSP_R1B | SD_CMD_AC;
        sdmsg.SDMSG_psdcmdStop   = &sdcmdStop;                          /*  停止命令                    */
    } else {
        sdmsg.SDMSG_psdcmdStop   = LW_NULL;
    }

    sdmsg.SDMSG_pucWrtBuffer = pucBuf;
    sdmsg.SDMSG_psddata      = &sddat;
    sdmsg.SDMSG_psdcmdCmd    = &sdcmd;
    sdmsg.SDMSG_pucRdBuffer  = LW_NULL;                                 /*  读缓冲为空                  */

    iError = API_SdCoreDevTransfer(psdcoredevice, &sdmsg, 1);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "request error.\r\n");
        return  (PX_ERROR);
    }

    /*
     * SPI多块写,最后发送停止数据令牌
     */
    if (COREDEV_IS_SPI(psdcoredevice)) {
        API_SdCoreSpiMulWrtStop(psdcoredevice);
    } else {
        iError = __sdMemTestBusy(psdcoredevice, __SD_BUSY_TYPE_PROG);
        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "check busy error.\r\n");
            return  (PX_ERROR);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdMemRdSingleBlk
** 功能描述: SD记忆卡设备读单块
** 输    入: psdcoredevice 核心设备结构
**           pucBuf      读缓冲
**           uiStartBlk  起始地址
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdMemRdSingleBlk (PLW_SDCORE_DEVICE  psdcoredevice,
                               UINT8             *pucBuf,
                               UINT32             uiStartBlk)
{
    LW_SD_MESSAGE   sdmsg;
    LW_SD_COMMAND   sdcmd;
    LW_SD_DATA      sddat;
    INT             iError;
    INT             iDevSta;

    iDevSta = API_SdCoreDevStaView(psdcoredevice);
    if (iDevSta != SD_DEVSTA_EXIST) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "device is not exist.\r\n");
        return  (PX_ERROR);
    }

    lib_bzero(&sdcmd, sizeof(sdcmd));
    lib_bzero(&sddat, sizeof(sddat));

    sdcmd.SDCMD_uiOpcode   = SD_READ_SINGLE_BLOCK;
    sdcmd.SDCMD_uiArg      = uiStartBlk;
    sdcmd.SDCMD_uiFlag     = SD_RSP_SPI_R1 | SD_RSP_R1 | SD_CMD_ADTC;   /*  命令                        */

    sddat.SDDAT_uiBlkNum   = 1;
    sddat.SDDAT_uiBlkSize  = SD_MEM_DEFAULT_BLKSIZE;
    sddat.SDDAT_uiFlags    = SD_DAT_READ;                               /*  数据                        */

    sdmsg.SDMSG_pucRdBuffer  = pucBuf;
    sdmsg.SDMSG_psddata      = &sddat;
    sdmsg.SDMSG_psdcmdCmd    = &sdcmd;
    sdmsg.SDMSG_psdcmdStop   = LW_NULL;
    sdmsg.SDMSG_pucWrtBuffer = LW_NULL;                                 /*  写缓冲为空                  */

    iError = API_SdCoreDevTransfer(psdcoredevice, &sdmsg, 1);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "request error.\r\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdMemRdMultiBlk
** 功能描述: SD记忆卡设备读多块
** 输    入: psdcoredevice 核心设备结构
**           pucBuf      读缓冲
**           uiStartBlk  起始地址
**           uiNBlks     块数量
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdMemRdMultiBlk (PLW_SDCORE_DEVICE   psdcoredevice,
                              UINT8              *pucBuf,
                              UINT32              uiStartBlk,
                              UINT32              uiNBlks)
{
    LW_SD_MESSAGE   sdmsg;
    LW_SD_COMMAND   sdcmd;
    LW_SD_DATA      sddat;
    LW_SD_COMMAND   sdcmdStop;
    INT             iError;
    INT             iDevSta;

    iDevSta = API_SdCoreDevStaView(psdcoredevice);
    if (iDevSta != SD_DEVSTA_EXIST) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "device is not exist.\r\n");
        return  (PX_ERROR);
    }

    lib_bzero(&sdcmd, sizeof(sdcmd));
    lib_bzero(&sdcmdStop, sizeof(sdcmdStop));
    lib_bzero(&sddat, sizeof(sddat));

    sdcmd.SDCMD_uiOpcode  = SD_READ_MULTIPLE_BLOCK;
    sdcmd.SDCMD_uiArg     = uiStartBlk;
    sdcmd.SDCMD_uiFlag    = SD_RSP_SPI_R1 | SD_RSP_R1 | SD_CMD_ADTC;    /*  命令                        */

    sddat.SDDAT_uiBlkNum  = uiNBlks;
    sddat.SDDAT_uiBlkSize = SD_MEM_DEFAULT_BLKSIZE;
    sddat.SDDAT_uiFlags   = SD_DAT_READ;                                /*  数据                        */

    /*
     * 在多块读操作中, SPI和SD模式的停止命令是相同的
     */
    sdcmdStop.SDCMD_uiOpcode = SD_STOP_TRANSMISSION;
    sdcmdStop.SDCMD_uiFlag   = SD_RSP_SPI_R1B | SD_RSP_R1B | SD_CMD_AC; /*  停止命令                    */

    sdmsg.SDMSG_pucRdBuffer  = pucBuf;
    sdmsg.SDMSG_psddata      = &sddat;
    sdmsg.SDMSG_psdcmdCmd    = &sdcmd;
    sdmsg.SDMSG_psdcmdStop   = &sdcmdStop;
    sdmsg.SDMSG_pucWrtBuffer = LW_NULL;                                 /*  写缓冲为空                  */

    iError = API_SdCoreDevTransfer(psdcoredevice, &sdmsg, 1);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "request error.\r\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdMemBlkWrt
** 功能描述: SD记忆卡块设备写
** 输    入: psdblkdevice     块设备结构
**           pvWrtBuffer      写缓冲
**           ulStartBlk       起始地址
**           ulBlkCount       块数量
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdMemBlkWrt(__PSD_BLK_DEV   psdblkdevice,
                         VOID           *pvWrtBuffer,
                         ULONG           ulStartBlk,
                         ULONG           ulBlkCount)
{
    INT                iError;
    INT                iDevSta;
    PLW_SDCORE_DEVICE  psdcoredevice;
    LW_SDDEV_CSD       sddevcsd;

    if (!psdblkdevice || !pvWrtBuffer) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    psdcoredevice = ((__PSD_BLK_DEV)psdblkdevice)->SDBLKDEV_pcoreDev;
    if (!psdcoredevice) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "no core device member.\r\n");
        return  (PX_ERROR);
    }

    iDevSta = API_SdCoreDevStaView(psdcoredevice);
    if (iDevSta != SD_DEVSTA_EXIST) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "device is not exist.\r\n");
        return  (PX_ERROR);
    }

    if (COREDEV_IS_SD(psdcoredevice) && psdblkdevice->SDBLKDEV_bNeedReSelect) {
        iError = API_SdCoreDevSelect(psdcoredevice);                        /*  选择设备                */
        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "select device failed.\r\n");
            return  (PX_ERROR);
        }
    }

    API_SdCoreDevCsdView(psdcoredevice, &sddevcsd);

    ulStartBlk += psdblkdevice->SDBLKDEV_ulSectorOff;
    if ((ulStartBlk + ulBlkCount) > psdblkdevice->SDBLKDEV_ulDiskNSector) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "block number out of range.\r\n");
        iError = PX_ERROR;
        goto    __error_handle;
    }

    /*
     * 块地址转换
     */
    if (!__SDMEM_BLKADDR(psdblkdevice)) {
        ulStartBlk = ulStartBlk << sddevcsd.DEVCSD_ucWriteBlkLenBits;
    }

    if (ulBlkCount <= 1) {
        iError = __sdMemWrtSingleBlk(psdcoredevice, (UINT8 *)pvWrtBuffer, (UINT32)ulStartBlk);
    } else {
        iError = __sdMemWrtMultiBlk(psdcoredevice, (UINT8 *)pvWrtBuffer,
                                    (UINT32)ulStartBlk, (UINT32)ulBlkCount);
    }

__error_handle:
    if (COREDEV_IS_SD(psdcoredevice) && psdblkdevice->SDBLKDEV_bNeedReSelect) {
        API_SdCoreDevDeSelect(psdcoredevice);                           /*  取消设备                    */
    }

    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __sdMemBlkWrt
** 功能描述: SD记忆卡块设备读
** 输    入: psdblkdevice   块设备结构
**           pvRdBuffer     读缓冲
**           ulStartBlk     起始地址
**           ulBlkCount     块数量
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdMemBlkRd (__PSD_BLK_DEV   psdblkdevice,
                         VOID           *pvRdBuffer,
                         ULONG           ulStartBlk,
                         ULONG           ulBlkCount)
{
    INT                iError;
    INT                iDevSta;
    PLW_SDCORE_DEVICE  psdcoredevice;
    LW_SDDEV_CSD       sddevcsd;

    if (!psdblkdevice || !pvRdBuffer) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    psdcoredevice = ((__PSD_BLK_DEV)psdblkdevice)->SDBLKDEV_pcoreDev;
    if (!psdcoredevice) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "no core device.\r\n");
        return  (PX_ERROR);
    }

    iDevSta = API_SdCoreDevStaView(psdcoredevice);
    if (iDevSta != SD_DEVSTA_EXIST) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "device is not exist.\r\n");
        return  (PX_ERROR);
    }

    if (COREDEV_IS_SD(psdcoredevice) && psdblkdevice->SDBLKDEV_bNeedReSelect) {
        iError = API_SdCoreDevSelect(psdcoredevice);                    /*  选择设备                    */
        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "select device failed.\r\n");
            return  (PX_ERROR);
        }
    }

    API_SdCoreDevCsdView(psdcoredevice, &sddevcsd);

    ulStartBlk += psdblkdevice->SDBLKDEV_ulSectorOff;
    if ((ulStartBlk + ulBlkCount) > psdblkdevice->SDBLKDEV_ulDiskNSector) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "block number is out of range.\r\n");
        iError = PX_ERROR;
        goto    __error_handle;
    }

    /*
     * 块地址转换
     */
    if (!__SDMEM_BLKADDR(psdblkdevice)) {
        ulStartBlk = ulStartBlk << sddevcsd.DEVCSD_ucReadBlkLenBits;
    }

    if (ulBlkCount <= 1) {
        iError = __sdMemRdSingleBlk(psdcoredevice, (UINT8 *)pvRdBuffer, (UINT32)ulStartBlk);
    } else {
        iError = __sdMemRdMultiBlk(psdcoredevice, (UINT8 *)pvRdBuffer,
                                   (UINT32)ulStartBlk, (UINT32)ulBlkCount);
    }

__error_handle:
    if (COREDEV_IS_SD(psdcoredevice) && psdblkdevice->SDBLKDEV_bNeedReSelect) {
        API_SdCoreDevDeSelect(psdcoredevice);                           /*  取消设备                    */
    }

    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __sdMemIoctl
** 功能描述: SD记忆卡块设备IO控制
** 输    入: psdblkdevice   块设备结构
**           iCmd           控制命令
**           lArg           参数
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdMemIoctl (__PSD_BLK_DEV    psdblkdevice,
                         INT              iCmd,
                         LONG             lArg)
{
    INT           iDevSta;
    LW_SDDEV_CSD  sdcsd;
    LW_BLK_INFO  *pblkinfo;

    if (!psdblkdevice) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    iDevSta = API_SdCoreDevStaView(psdblkdevice->SDBLKDEV_pcoreDev);
    if (iDevSta != SD_DEVSTA_EXIST) {
        return  (PX_ERROR);
    }

    switch (iCmd) {

    case FIOSYNC:
    case FIODATASYNC:
    case FIOFLUSH:                                                      /*  将缓存回写到磁盘            */
    case FIOSYNCMETA:
    case FIOUNMOUNT:                                                    /*  卸载卷                      */
    case FIODISKINIT:                                                   /*  初始化设备                  */
    case FIODISKCHANGE:                                                 /*  磁盘媒质发生变化            */
        break;

    case LW_BLKD_GET_SECSIZE:
    case LW_BLKD_GET_BLKSIZE:
        *((ULONG *)lArg) = SD_MEM_DEFAULT_BLKSIZE;
        break;

    case LW_BLKD_GET_SECNUM:
        API_SdCoreDevCsdView(psdblkdevice->SDBLKDEV_pcoreDev, &sdcsd);
        *((UINT32 *)lArg) = sdcsd.DEVCSD_uiCapacity;
        break;

    case LW_BLKD_CTRL_INFO:
        pblkinfo = (LW_BLK_INFO *)lArg;
        lib_memcpy(pblkinfo, &psdblkdevice->SDBLKDEV_blkinfo, sizeof(LW_BLK_INFO));
        break;

    case FIOWTIMEOUT:
    case FIORTIMEOUT:
        break;

    default:
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdMemBlkInfoFmt
** 功能描述: 格式化块设备信息
** 输    入: psdblkdevice  块设备结构
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdMemBlkInfoFmt (__PSD_BLK_DEV psdblkdevice)
{
    PLW_SDCORE_DEVICE  psdcoredevice;
    LW_SDDEV_CSD       sddevcsd;
    LW_SDDEV_CID       sddevcid;
    LW_BLK_INFO       *pblkinfo;

    UINT8              ucType;
    CPCHAR             pcTypeStr = "unknown";
    CPCHAR             pcVsnStr  = "unknown";

    psdcoredevice = psdblkdevice->SDBLKDEV_pcoreDev;
    pblkinfo      = &psdblkdevice->SDBLKDEV_blkinfo;
    
    pblkinfo->BLKI_uiType = LW_BLKD_CTRL_INFO_TYPE_SDMMC;

    API_SdCoreDevCsdView(psdcoredevice, &sddevcsd);
    API_SdCoreDevCidView(psdcoredevice, &sddevcid);
    API_SdCoreDevTypeView(psdcoredevice, &ucType);

    switch (ucType) {

    case SDDEV_TYPE_MMC:
        pcTypeStr = "MMC/eMMC";
        break;

    case SDDEV_TYPE_SDSC:
        pcTypeStr = "SDSC";
        break;

    case SDDEV_TYPE_SDHC:
        pcTypeStr = "SDHC";
        break;

    case SDDEV_TYPE_SDXC:
        pcTypeStr = "SDXC";
        break;

    default:
        break;
    }

    pcVsnStr = __sdMemProtVsnStr(ucType, sddevcsd.DEVCSD_ucStructure);

    snprintf(pblkinfo->BLKI_cSerial, 32,
             "%08X",
             sddevcid.DEVCID_uiSerialNum);

    snprintf(pblkinfo->BLKI_cFirmware, 32,
             "%d.%02d, v%d.%d",
             sddevcid.DEVCID_uiYear,
             sddevcid.DEVCID_ucMonth,
             sddevcid.DEVCID_ucProductVsn >> 4,
             sddevcid.DEVCID_ucProductVsn & 0xf);

    snprintf(pblkinfo->BLKI_cProduct, 32,
             "%c%c%c%c%c %s memory card",
             __SD_CID_PNAME(0),
             __SD_CID_PNAME(1),
             __SD_CID_PNAME(2),
             __SD_CID_PNAME(3),
             __SD_CID_PNAME(4),
             pcTypeStr);

    snprintf(pblkinfo->BLKI_cMedia, 32,
             "%s(%s)",
             pcTypeStr,
             pcVsnStr);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdMemStatus
** 功能描述: SD记忆卡块设备状态函数
** 输    入: psdblkdevice  块设备结构
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdMemStatus (__PSD_BLK_DEV   psdblkdevice)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdMemStatus
** 功能描述: SD记忆卡块设备复位
** 输    入: psdblkdevice  块设备结构
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdMemReset (__PSD_BLK_DEV    psdblkdevice)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdMemProtVsnStr
** 功能描述: 获取卡的协议版本字符串
** 输    入: ucType   卡类型(MMC 或 SD)
**           ucVsn    CSD里面的结构标识
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static CPCHAR __sdMemProtVsnStr (UINT8 ucType, UINT8 ucVsn)
{
    CPCHAR pcVsnStr = "v1.0";

    if (ucType != SDDEV_TYPE_MMC) {
        switch (ucVsn) {

        case CSD_STRUCT_VER_1_0:
            pcVsnStr = "v1.0";
            break;

        case CSD_STRUCT_VER_2_0:
            pcVsnStr = "v2.0";
            break;

        default:
            pcVsnStr = "v2.0";
            break;
        }

    } else {
        switch (ucVsn) {

        case MMC_VERSION_1_2:
            pcVsnStr = "v1.2";
            break;

        case MMC_VERSION_1_4:
            pcVsnStr = "v1.4";
            break;

        case MMC_VERSION_2_2:
            pcVsnStr = "v2.2";
            break;

        case MMC_VERSION_3:
            pcVsnStr = "v3.0";
            break;

        case MMC_VERSION_4:
            pcVsnStr = "v4.0";
            break;

        default:
            pcVsnStr = "v1.2";
            break;
        }
    }

    return  (pcVsnStr);
}
/*********************************************************************************************************
** 函数名称: __sdMemSdSwCapGet
** 功能描述: 获得SD卡的SWITCH功能信息
** 输    入: psdcoredevice   核心设备对象
**           psdscr          SCR 信息
**           psdcsd          CSD 信息
**           psdswcap        保存获得的SWITCH功能信息
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdMemSdSwCapGet (PLW_SDCORE_DEVICE  psdcoredevice,
                              LW_SDDEV_SCR      *psdscr,
                              LW_SDDEV_CSD      *psdcsd,
                              LW_SDDEV_SW_CAP   *psdswcap)
{
    UINT8   ucSwCap[64];
    INT     iRet;

    lib_bzero(psdswcap, sizeof(LW_SDDEV_SW_CAP));

    if (psdscr->DEVSCR_ucSdaVsn < SD_SCR_SPEC_VER_1) {
        return  (PX_ERROR);
    }

    if (!(psdcsd->DEVCSD_usCmdclass & CCC_SWITCH)) {
        return  (PX_ERROR);
    }

    iRet = API_SdCoreDevSwitchEx(psdcoredevice, 0, 0, 0, ucSwCap);
    if (iRet != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "get sd switch information error.\r\n");
        return  (PX_ERROR);
    }

    if (ucSwCap[13] & SD_SW_MODE_HIGH_SPEED) {
        psdswcap->DEVSWCAP_uiHsMaxDtr = 50000000;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdMemSdHsSwitch
** 功能描述: SD 卡高速模式切换
** 输    入: psdcoredevice   核心设备对象
**           psdswcap        SWITCH功能信息
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdMemSdHsSwitch (PLW_SDCORE_DEVICE  psdcoredevice, LW_SDDEV_SW_CAP   *psdswcap)
{
    INT     iRet;
    UINT8   ucStatus[64];

    if (!psdswcap->DEVSWCAP_uiHsMaxDtr) {
        return  (PX_ERROR);
    }

    iRet = API_SdCoreDevSwitchEx(psdcoredevice, 1, 0, 1, ucStatus);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    if ((ucStatus[16] & 0xF) != 1) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdMemMmcFreqChange
** 功能描述: 设置 MMC 卡的总线频率
** 输    入: psdcoredevice   核心设备对象
**           psdcsd          已经解析后的 CSD
**           piCardCap       保存CARD的功能(速度和支持的位宽模式)
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdMemMmcFreqChange (PLW_SDCORE_DEVICE  psdcoredevice,
                                 LW_SDDEV_CSD      *psdcsd,
                                 INT               *piCardCap)
{
    UINT8   ucExtCSD[512];
    CHAR    cCardType;
    INT     iError;
    INT     iCapab  = 0;

    if (psdcsd->DEVCSD_ucStructure < MMC_VERSION_4) {
        return	(ERROR_NONE);
    }

    iError = API_SdCoreDevSendExtCSD(psdcoredevice, ucExtCSD);
    if (iError) {
        return  (iError);
    }

    if (ucExtCSD[212] || ucExtCSD[213] || ucExtCSD[214] || ucExtCSD[215]) {
        COREDEV_HIGHSPEED_SET(psdcoredevice);
    }

    cCardType = ucExtCSD[196] & 0xf;

    /*
     * 这里的功能位标使用 sddrvm.h里面的定义,
     * 这样在后面可以直接使用位与的方式得到实际需要使用的功能
     */
    if (cCardType & MMC_HS_52MHZ_1_8V_3V_IO) {
        iCapab |= MMC_MODE_HS_52MHz_DDR_18_3V
               |  MMC_MODE_HS_52MHz
               |  MMC_MODE_HS
               |  SDHOST_CAP_DATA_8BIT_DDR
               |  SDHOST_CAP_DATA_4BIT_DDR
               |  SDHOST_CAP_DATA_4BIT
               |  SDHOST_CAP_DATA_8BIT;

    } else if (cCardType & MMC_HS_52MHZ) {
        iCapab |= MMC_MODE_HS_52MHz
               |  MMC_MODE_HS
               |  SDHOST_CAP_DATA_4BIT
               |  SDHOST_CAP_DATA_8BIT;
    }

    *piCardCap = iCapab;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdMemMmcBusWidthChange
** 功能描述: 设置 MMC 卡的总线位宽
** 输    入: psdcoredevice   核心设备对象
**           iCardCap        MMC卡支持的位宽功能
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdMemMmcBusWidthChange (PLW_SDCORE_DEVICE psdcoredevice, INT iCardCap)
{
    INT iHostCap = 0;
    INT iError   = ERROR_NONE;

    iError = API_SdmHostCapGet(psdcoredevice, &iHostCap);
    if (iError != ERROR_NONE) {
        return  (PX_ERROR);
    }

    if (iHostCap & SDHOST_CAP_MMC_FORCE_1BIT) {
        return  (ERROR_NONE);
    }

    iCardCap = iCardCap & iHostCap;

    if (iCardCap & MMC_MODE_HS) {
        iError = API_SdCoreDevSwitch(psdcoredevice,
                                     EXT_CSD_CMD_SET_NORMAL,
                                     EXT_CSD_HS_TIMING,
                                     1);

    } else {
        iError = API_SdCoreDevSwitch(psdcoredevice,
                                     EXT_CSD_CMD_SET_NORMAL,
                                     EXT_CSD_HS_TIMING,
                                     0);
    }

    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSGX(__ERRORMESSAGE_LEVEL, "switch to %s mode failed.\r\n",
                          iCardCap & MMC_MODE_HS ? "high speed" : "full speed");
        return  (iError);
    }

    if (iCardCap & SDHOST_CAP_DATA_8BIT_DDR) {
        API_SdCoreDevCtl(psdcoredevice, SDBUS_CTRL_SETBUSWIDTH, SDARG_SETBUSWIDTH_8_DDR);
        iError = API_SdCoreDevSwitch(psdcoredevice,
                                     EXT_CSD_CMD_SET_NORMAL,
                                     EXT_CSD_BUS_WIDTH,
                                     EXT_CSD_BUS_WIDTH_8_DDR);

        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "switch to 8Bit-DDR mode failed.\r\n");
            return  (iError);
        }

    } else if (iCardCap & SDHOST_CAP_DATA_4BIT_DDR) {
        API_SdCoreDevCtl(psdcoredevice, SDBUS_CTRL_SETBUSWIDTH, SDARG_SETBUSWIDTH_4_DDR);
        iError = API_SdCoreDevSwitch(psdcoredevice,
                                     EXT_CSD_CMD_SET_NORMAL,
                                     EXT_CSD_BUS_WIDTH,
                                     EXT_CSD_BUS_WIDTH_4_DDR);

        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "switch to 4Bit-DDR mode failed.\r\n");
            return  (iError);
        }

    } else if (iCardCap & SDHOST_CAP_DATA_8BIT) {
        API_SdCoreDevCtl(psdcoredevice, SDBUS_CTRL_SETBUSWIDTH, SDARG_SETBUSWIDTH_8);
        iError = API_SdCoreDevSwitch(psdcoredevice,
                                     EXT_CSD_CMD_SET_NORMAL,
                                     EXT_CSD_BUS_WIDTH,
                                     EXT_CSD_BUS_WIDTH_8);

        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "switch to 8Bit mode failed.\r\n");
            return  (iError);
        }

    } else if (iCardCap & SDHOST_CAP_DATA_4BIT) {
        API_SdCoreDevCtl(psdcoredevice, SDBUS_CTRL_SETBUSWIDTH, SDARG_SETBUSWIDTH_4);
        iError = API_SdCoreDevSwitch(psdcoredevice,
                                     EXT_CSD_CMD_SET_NORMAL,
                                     EXT_CSD_BUS_WIDTH,
                                     EXT_CSD_BUS_WIDTH_4);

        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "switch to 4Bit mode failed.\r\n");
            return  (iError);
        }
    }

    if (iCardCap & MMC_MODE_HS) {
        iError = API_SdCoreDevCtl(psdcoredevice,
                                  SDBUS_CTRL_SETCLK,
                                  SDARG_SETCLK_MAX);
    }

    return  (iError);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_SDCARD_EN > 0)      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
