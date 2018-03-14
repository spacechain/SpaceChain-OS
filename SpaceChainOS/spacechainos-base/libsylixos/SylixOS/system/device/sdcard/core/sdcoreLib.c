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
** 文   件   名: sdcoreLib.c
**
** 创   建   人: Zeng.Bo (曾波)
**
** 文件创建日期: 2010 年 11 月 23 日
**
** 描        述: sd 卡特殊操作接口源文件

** BUG:
2010.11.23  修改 __sdCoreDevSendIfCond(), 在 SPI 和 SD 模式下,检验码在应答中的位置不同.
2010.11.27  增加了几个 API.
2010.12.02  由于最初测试不周密, 发现 __getBits() 函数有错误,改正之.
2011.02.12  更改 SD 卡复位函数 __sdCoreDevReset() .对其增加 SPI 模式下的复位操作.
2011.02.21  更改 __sdCoreDevSendAllCSD() 和 __sdCoreDevSendAllCID()以适应spi模式.
2011.02.21  更改 __sdCoreDevGetStatus()函数.今天才发现 cmd13 是一个 app 命令,对一些卡来说,cmd13 可以当作
            一般命令发送没有问题,但有的卡就不行了.
2011.03.30  增加 __sdCoreDevMmcSetRelativeAddr () 以支持 MMC.
2011.03.30  修改 __sdCoreDecodeCID(), 加入对 MMC 卡 CID 的解析
2011.04.12  修改 __sdCoreDevSendAppOpCond(). 其传入的参数 uiOCR 为 主控支持的电压,但是对于 memory 卡,其卡
            电压是有一定范围要求的.所以在发送命令时,将 uiOCR 进行处理后再作为参数发送.
2015.09.15  更改 MMC 卡是否是块寻址的判断条件.
2015.09.22  增加 MMC/eMMC 扩展协议的支持.
2016.07.15  修正 MMC/eMMC 在使用 EXT-CSD 结构信息获取容量时可能产生错误的结果.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "endian.h"
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SDCARD_EN > 0)
#include "sdcore.h"
#include "sdstd.h"
#include "../include/sddebug.h"
/*********************************************************************************************************
 CSD 中TACC域中的指数值(以纳秒为单位)和系数(为了避免使用浮点,已经乘以了10)查找表
*********************************************************************************************************/
static const UINT32    _G_puiCsdTaccExp[] = {
        1, 10, 100, 1000, 10000, 100000, 1000000, 10000000
};
static const UINT32    _G_puiCsdTaccMnt[] = {
        0,  10, 12, 13, 15, 20, 25, 30,
        35, 40, 45, 50, 55, 60, 70, 80
};
/*********************************************************************************************************
 CSD 中TRAN_SPEED域中的指数(以bit/s为单位)值和系数(为了避免使用浮点,已经乘以了10)查找表
*********************************************************************************************************/
static const UINT32    _G_CsdTrspExp[] = {
        10000, 100000, 1000000, 10000000,
            0,      0,       0,        0
};
static const UINT32    _G_CsdTrspMnt[] = {
        0,  10, 12, 13, 15, 20, 25, 30,
        35, 40, 45, 50, 55, 60, 70, 80
};
/*********************************************************************************************************
** 函数名称: __getBits
** 功能描述: 在应答内容中,获得指定位的数据.
**           原始的应答数据是 UINT32类型,大小固定为4个元素(128位).
**           !!!注意：函数未作参数检查,调用时应该注意.
** 输    入: puiResp       应答的原始数据
**           uiStart       数据起始位置(位)
**                         !!!注意: uiStart为0 对应原始数据最后一个元素的最低位.
**                         如RSP[4] = {0,0,0,0X0000003F}  则 __getBits(RSP, 0, 8) 的值为0x3F.
**           uiSize        数据宽度(位)
**           pcDeviceName  设备名称
** 输    出: NONE
** 返    回: 最终提取的数据(最大为32位).
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT32 __getBits (UINT32  *puiResp, UINT32 uiStart, UINT32 uiSize)
{
    UINT32  uiRes;
    UINT32  uiMask  = (uiSize < 32 ? 1 << uiSize : 0) - 1;
    INT     iOff    = 3 - (uiStart / 32);
    INT     iSft    = uiStart & 31;

    uiRes = puiResp[iOff] >> iSft;
    if (uiSize + iSft > 32) {
        uiRes |= puiResp[iOff - 1] << ((32 - iSft) % 32);
    }
    uiRes &= uiMask;

    return  (uiRes);
}
/*********************************************************************************************************
** 函数名称: __mmcCsdDecode
** 功能描述: 解析 MMC CSD
** 输    入: psdcsdDec
**           pRawCSD
** 输    出: ERROR CODE
** 返    回: NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __mmcCsdDecode (LW_SDDEV_CSD  *psdcsdDec, UINT32 *pRawCSD)
{
    UINT32  uiExp;
    UINT32  uiMnt;
    UINT8   ucMmcVsn = MMC_VERSION_1_2;

    ucMmcVsn = __getBits(pRawCSD, 122, 4);
    switch (ucMmcVsn) {
    case 0:
        ucMmcVsn = MMC_VERSION_1_2;
        break;
    case 1:
        ucMmcVsn = MMC_VERSION_1_4;
        break;
    case 2:
        ucMmcVsn = MMC_VERSION_2_2;
        break;
    case 3:
        ucMmcVsn = MMC_VERSION_3;
        break;
    case 4:
        ucMmcVsn = MMC_VERSION_4;
        break;
    }

    psdcsdDec->DEVCSD_ucStructure = ucMmcVsn;

    uiMnt = __getBits(pRawCSD, 115, 4);
    uiExp = __getBits(pRawCSD, 112, 3);
    psdcsdDec->DEVCSD_uiTaccNs     = (_G_puiCsdTaccExp[uiExp] * _G_puiCsdTaccMnt[uiMnt] + 9) / 10;
    psdcsdDec->DEVCSD_usTaccClks   = __getBits(pRawCSD, 104, 8) * 100;

    uiMnt = __getBits(pRawCSD, 99, 4);
    uiExp = __getBits(pRawCSD, 96, 3);
    psdcsdDec->DEVCSD_uiTranSpeed = _G_CsdTrspExp[uiExp] * _G_CsdTrspMnt[uiMnt];
    psdcsdDec->DEVCSD_usCmdclass  = __getBits(pRawCSD, 84, 12);

    uiExp = __getBits(pRawCSD, 47, 3);
    uiMnt = __getBits(pRawCSD, 62, 12);
    psdcsdDec->DEVCSD_uiCapacity = (1 + uiMnt) << (uiExp + 2);

    psdcsdDec->DEVCSD_ucReadBlkLenBits  = __getBits(pRawCSD, 80, 4);
    psdcsdDec->DEVCSD_bReadBlkPartial   = __getBits(pRawCSD, 79, 1);
    psdcsdDec->DEVCSD_bWriteMissAlign   = __getBits(pRawCSD, 78, 1);
    psdcsdDec->DEVCSD_bReadMissAlign    = __getBits(pRawCSD, 77, 1);
    psdcsdDec->DEVCSD_ucR2W_Factor      = __getBits(pRawCSD, 26, 3);
    psdcsdDec->DEVCSD_ucWriteBlkLenBits = __getBits(pRawCSD, 22, 4);
    psdcsdDec->DEVCSD_bWriteBlkPartial  = __getBits(pRawCSD, 21, 1);
    psdcsdDec->DEVCSD_ucEraseBlkLen     = 1;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_SdCoreDevSendExtCSD
** 功能描述: 获得扩展CSD数据
** 输    入: psdcoredevice    SD核心传输对象
**           pucExtCsd        保存获取的数据缓冲区(不小于512字节)
** 输    出: ERROR CODE
** 返    回: NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  API_SdCoreDevSendExtCSD (PLW_SDCORE_DEVICE psdcoredevice, UINT8 *pucExtCsd)
{

    LW_SD_MESSAGE   sdmsg;
    LW_SD_COMMAND   sdcmd;
    LW_SD_DATA      sddat;
    INT             iError;

    sdcmd.SDCMD_uiOpcode = MMC_CMD_SEND_EXT_CSD;
    sdcmd.SDCMD_uiFlag   = SD_RSP_R1 | SD_CMD_ADTC;
    sdcmd.SDCMD_uiArg    = 0;
    sdcmd.SDCMD_uiRetry  = 0;

    sddat.SDDAT_uiBlkNum  = 1;
    sddat.SDDAT_uiBlkSize = 512;
    sddat.SDDAT_uiFlags   = SD_DAT_READ;

    sdmsg.SDMSG_psdcmdCmd    = &sdcmd;
    sdmsg.SDMSG_psddata      = &sddat;
    sdmsg.SDMSG_psdcmdStop   = LW_NULL;
    sdmsg.SDMSG_pucRdBuffer  = pucExtCsd;
    sdmsg.SDMSG_pucWrtBuffer = LW_NULL;

    iError = API_SdCoreDevTransfer(psdcoredevice, &sdmsg, 1);

    return  (iError);
}
/*********************************************************************************************************
** 函数名称: API_SdCoreDevSwitch
** 功能描述: SD 设备功能/模式切换
** 输    入: psdcoredevice    SD核心传输对象
**           ucCmdSet         命令集代码(通常为0)
**           ucIndex          命令索引
**           ucValue          命令值
** 输    出: ERROR CODE
** 返    回: NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  API_SdCoreDevSwitch (PLW_SDCORE_DEVICE     psdcoredevice,
                          UINT8                 ucCmdSet,
                          UINT8                 ucIndex,
                          UINT8                 ucValue)
{
    LW_SD_COMMAND   sdcmd;
    INT             iRet;

    sdcmd.SDCMD_uiOpcode = SD_SWITCH_FUNC;
    sdcmd.SDCMD_uiFlag   = SD_RSP_R1B;
    sdcmd.SDCMD_uiArg    = (SD_SWITCH_WRITE_BYTE << 24)
                         | (ucIndex << 16)
                         | (ucValue << 8)
                         | ucCmdSet;

    sdcmd.SDCMD_uiRetry  = 0;
    iRet = API_SdCoreDevCmd(psdcoredevice, &sdcmd, 0);

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_SdCoreDevSwitchEx
** 功能描述: SD 设备功能/模式切换扩展接口
** 输    入: psdcoredevice    SD核心传输对象
**           iMode            切换的模式
**           iGroup           切换所在组
**           ucValue          切换值
**           pucResp          保存应答数据
** 输    出: ERROR CODE
** 返    回: NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT API_SdCoreDevSwitchEx (PLW_SDCORE_DEVICE psdcoredevice,
                           INT               iMode,
                           INT               iGroup,
                           INT               ucValue,
                           UINT8            *pucResp)
{
    LW_SD_COMMAND   sdcmd;
    LW_SD_DATA      sddat;
    LW_SD_MESSAGE   sdmsg;
    INT             iRet;

    lib_bzero(&sdmsg, sizeof(sdmsg));
    lib_bzero(&sdcmd, sizeof(sdcmd));

    iMode   = !!iMode;
    ucValue &= 0xF;

    sdcmd.SDCMD_uiOpcode  = SD_SWITCH_FUNC;
    sdcmd.SDCMD_uiArg     = iMode << 31 | 0x00FFFFFF;
    sdcmd.SDCMD_uiArg    &= ~(0xF << (iGroup << 2));
    sdcmd.SDCMD_uiArg    |= ucValue << (iGroup << 2);
    sdcmd.SDCMD_uiFlag    = SD_RSP_SPI_R1 | SD_RSP_R1 | SD_CMD_ADTC;

    sddat.SDDAT_uiBlkNum  = 1;
    sddat.SDDAT_uiBlkSize = 64;
    sddat.SDDAT_uiFlags   = SD_DAT_READ;

    sdmsg.SDMSG_psdcmdCmd   = &sdcmd;
    sdmsg.SDMSG_psddata     = &sddat;
    sdmsg.SDMSG_pucRdBuffer = pucResp;
    iRet = API_SdCoreDevTransfer(psdcoredevice, &sdmsg, 1);

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_SdCoreDecodeExtCSD
** 功能描述: 获取并解析扩展CSD信息(同时更新标准CSD信息结构)
** 输    入: psdcoredevice    SD核心传输对象
**           psdcsd           标准CSD信息
**           psdextcsd        扩展CSD信息
** 输    出: ERROR CODE
** 返    回: NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT API_SdCoreDecodeExtCSD (PLW_SDCORE_DEVICE  psdcoredevice,
                            LW_SDDEV_CSD      *psdcsd,
                            LW_SDDEV_EXT_CSD  *psdextcsd)
{
    UINT   uiExtCsdRev;
    UINT8  pucExtCsd[512];
    INT    iError;


    if (!psdcoredevice || !psdcsd || !psdextcsd) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (psdcsd->DEVCSD_ucStructure < (MMC_VERSION_4 | MMC_VERSION_MMC)) {
        return  (ERROR_NONE);
    }

    iError = API_SdCoreDevSendExtCSD(psdcoredevice, pucExtCsd);
    if (iError) {
        /*
         * High capacity cards should have this "magic" size
         * stored in their CSD. But here we ignore it
         */
        return  (ERROR_NONE);
    }

    uiExtCsdRev = pucExtCsd[EXT_CSD_REV];
    psdextcsd->DEVEXTCSD_uiBootSizeMulti = pucExtCsd[BOOT_SIZE_MULTI];

    if (uiExtCsdRev >= 2) {
        psdextcsd->DEVEXTCSD_uiSectorCnt = (pucExtCsd[EXT_CSD_SEC_CNT + 0] << 0)
                                         | (pucExtCsd[EXT_CSD_SEC_CNT + 1] << 8)
                                         | (pucExtCsd[EXT_CSD_SEC_CNT + 2] << 16)
                                         | (pucExtCsd[EXT_CSD_SEC_CNT + 3] << 24);

        if (psdextcsd->DEVEXTCSD_uiSectorCnt) {
            /*
             * 当使用扩展CSD容量时, 扇区大小固定为512
             */
            psdcsd->DEVCSD_ucReadBlkLenBits = SD_MEM_DEFAULT_BLKSIZE_NBITS;
            psdcsd->DEVCSD_uiCapacity       = psdextcsd->DEVEXTCSD_uiSectorCnt;
        }

        if (psdextcsd->DEVEXTCSD_uiSectorCnt > (2u * LW_CFG_GB_SIZE / 512)) {
            psdcoredevice->COREDEV_iDevSta |= COREDEV_STA_HIGHCAP_OCR;
        }
    }

    if ((pucExtCsd[EXT_CSD_CARD_TYPE] & 0xf) &
        (EXT_CSD_CARD_TYPE_52 | EXT_CSD_CARD_TYPE_52_DDR_18_30 | EXT_CSD_CARD_TYPE_52_DDR_12)) {
        psdcsd->DEVCSD_uiTranSpeed = 52000000;
    
    } else if (pucExtCsd[EXT_CSD_CARD_TYPE] & EXT_CSD_CARD_TYPE_26) {
        psdcsd->DEVCSD_uiTranSpeed = 26000000;

    } else {
        /*
         * will never happen
         */
    }

    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __printCID
** 功能描述: 打印CID
** 输    入: psdcid
**           ucType
** 输    出: NONE
** 返    回: NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#ifdef  __SYLIXOS_DEBUG

static void  __printCID (LW_SDDEV_CID *psdcid, UINT8 ucType)
{
#ifndef printk
#define printk
#endif                                                                  /*  printk                      */

#define __CID_PNAME(iN)   (psdcid->DEVCID_pucProductName[iN])

    printk("\nCID INFO >>\n");
    printk("Manufacturer :  %08x\n", psdcid->DEVCID_ucMainFid);
    if (ucType == SDDEV_TYPE_MMC) {
        printk("OEM ID       :  %08x\n", psdcid->DEVCID_usOemId);
    } else {
        printk("OEM ID       :  %c%c\n", psdcid->DEVCID_usOemId >> 8,
                                     psdcid->DEVCID_usOemId & 0xff);
    }
    printk("Product Name :  %c%c%c%c%c\n",
                                        __CID_PNAME(0),
                                        __CID_PNAME(1),
                                        __CID_PNAME(2),
                                        __CID_PNAME(3),
                                        __CID_PNAME(4));
    printk("Product Vsn  :  %02d.%02d\n", psdcid->DEVCID_ucProductVsn >> 4,
                                          psdcid->DEVCID_ucProductVsn & 0xf);
    printk("Serial Num   :  %08x\n", psdcid->DEVCID_uiSerialNum);
    printk("Year         :  %04d\n", psdcid->DEVCID_uiYear);
    printk("Month        :  %02d\n", psdcid->DEVCID_ucMonth);
}

#endif                                                                  /*  __SYLIXOS_DEBUG             */
/*********************************************************************************************************
** 函数名称: API_SdCoreDecodeCID
** 功能描述: 解码CID
** 输    入: pRawCID         原始CID数据
**           ucType          卡的类型
** 输    出: psdcidDec       解码后的CID
** 返    回: ERROR    CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT API_SdCoreDecodeCID (LW_SDDEV_CID  *psdcidDec, UINT32 *pRawCID, UINT8 ucType)
{
    if (!psdcidDec || !pRawCID) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    lib_bzero(psdcidDec, sizeof(LW_SDDEV_CID));

    switch (ucType) {
    
    case SDDEV_TYPE_MMC:
        psdcidDec->DEVCID_ucMainFid          =  __getBits(pRawCID, 120, 8);
        psdcidDec->DEVCID_usOemId            =  __getBits(pRawCID, 104, 16);
        psdcidDec->DEVCID_pucProductName[0]  =  __getBits(pRawCID, 88,  8);
        psdcidDec->DEVCID_pucProductName[1]  =  __getBits(pRawCID, 80,  8);
        psdcidDec->DEVCID_pucProductName[2]  =  __getBits(pRawCID, 72,  8);
        psdcidDec->DEVCID_pucProductName[3]  =  __getBits(pRawCID, 64,  8);
        psdcidDec->DEVCID_pucProductName[4]  =  __getBits(pRawCID, 56,  8);
        psdcidDec->DEVCID_ucProductVsn       =  __getBits(pRawCID, 48,  8);
        psdcidDec->DEVCID_uiSerialNum        =  __getBits(pRawCID, 16,  32);
        psdcidDec->DEVCID_ucMonth            =  __getBits(pRawCID, 12,  4);
        psdcidDec->DEVCID_uiYear             =  __getBits(pRawCID, 8,   4);
        psdcidDec->DEVCID_uiYear            +=  1997;
        break;

    default:
        psdcidDec->DEVCID_ucMainFid          =  __getBits(pRawCID, 120, 8);
        psdcidDec->DEVCID_usOemId            =  __getBits(pRawCID, 104, 16);
        psdcidDec->DEVCID_pucProductName[0]  =  __getBits(pRawCID, 96,  8);
        psdcidDec->DEVCID_pucProductName[1]  =  __getBits(pRawCID, 88,  8);
        psdcidDec->DEVCID_pucProductName[2]  =  __getBits(pRawCID, 80,  8);
        psdcidDec->DEVCID_pucProductName[3]  =  __getBits(pRawCID, 72,  8);
        psdcidDec->DEVCID_pucProductName[4]  =  __getBits(pRawCID, 64,  8);
        psdcidDec->DEVCID_ucProductVsn       =  __getBits(pRawCID, 56,  8);
        psdcidDec->DEVCID_uiSerialNum        =  __getBits(pRawCID, 24,  32);
        psdcidDec->DEVCID_uiYear             =  __getBits(pRawCID, 12,  8);
        psdcidDec->DEVCID_ucMonth            =  __getBits(pRawCID, 8,   4);
        psdcidDec->DEVCID_uiYear            +=  2000;
        break;
    }

#ifdef  __SYLIXOS_DEBUG
    __printCID(psdcidDec, ucType);
#endif                                                                  /*  __SYLIXOS_DEBUG             */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __printCSD
** 功能描述: 打印CSD
** 输    入: psdcsd
** 输    出: NONE
** 返    回: NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#ifdef  __SYLIXOS_DEBUG

static void  __printCSD (LW_SDDEV_CSD *psdcsd)
{
#ifndef printk
#define printk
#endif                                                                  /*  printk                      */

    printk("\nCSD INFO >>\n");
    printk("CSD structure:  %08d\n", psdcsd->DEVCSD_ucStructure);
    printk("Tacc(Ns)     :  %08d\n", psdcsd->DEVCSD_uiTaccNs);
    printk("TACC(CLK)    :  %08d\n", psdcsd->DEVCSD_usTaccClks);
    printk("Tran Speed   :  %08d\n", psdcsd->DEVCSD_uiTranSpeed);
    printk("R2W Factor   :  %08d\n", psdcsd->DEVCSD_ucR2W_Factor);
    printk("Read Blk Len :  %08d\n", 1 << psdcsd->DEVCSD_ucReadBlkLenBits);
    printk("Write Blk Len:  %08d\n", 1 << psdcsd->DEVCSD_ucWriteBlkLenBits);

    printk("Erase Enable :  %08d\n", psdcsd->DEVCSD_bEraseEnable);
    printk("Erase Blk Len:  %08d\n", psdcsd->DEVCSD_ucEraseBlkLen);
    printk("Sector Size  :  %08d\n", psdcsd->DEVCSD_ucSectorSize);

    printk("Read MisAlign:  %08d\n", psdcsd->DEVCSD_bReadMissAlign);
    printk("Writ MisAlign:  %08d\n", psdcsd->DEVCSD_bWriteMissAlign);
    printk("Read Partial :  %08d\n", psdcsd->DEVCSD_bReadBlkPartial);
    printk("Write Partial:  %08d\n", psdcsd->DEVCSD_bWriteBlkPartial);

    printk("Support Cmd  :  %08x\n", psdcsd->DEVCSD_usCmdclass);
    printk("Block Number :  %08d\n", psdcsd->DEVCSD_uiCapacity);
}

#endif                                                                  /*  __SYLIXOS_DEBUG             */
/*********************************************************************************************************
** 函数名称: API_SdCoreDecodeCSD
** 功能描述: 解码CSD
** 输    入: pRawCSD         原始CSD数据
**           ucType          卡的类型
** 输    出: psdcsdDec       解码后的CSD
** 返    回: ERROR    CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT API_SdCoreDecodeCSD (LW_SDDEV_CSD  *psdcsdDec, UINT32 *pRawCSD, UINT8 ucType)
{
    UINT8   ucStruct;
    UINT32  uiExp;
    UINT32  uiMnt;

    if (!psdcsdDec || !pRawCSD) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    lib_bzero(psdcsdDec, sizeof(LW_SDDEV_CSD));

    if (ucType == SDDEV_TYPE_MMC) {
        __mmcCsdDecode(psdcsdDec, pRawCSD);
        goto __end;
    }

    /*
     * SD规范中，CSD 有两个版本. v1.0用于针对一般SD卡.之后出现了v2.0,是为了支持 SDHC 和 SDXC 卡.
     * 在 SDHC 和 SDXC 中,很多域都是固定的.
     */
    ucStruct = __getBits(pRawCSD, 126, 2);
    psdcsdDec->DEVCSD_ucStructure = ucStruct;

    switch (ucStruct) {
    
    case CSD_STRUCT_VER_1_0:
        uiMnt = __getBits(pRawCSD, 115, 4);
        uiExp = __getBits(pRawCSD, 112, 3);
        /*
         * 这里除以10,是因为在查找表里面乘了10
         */
        psdcsdDec->DEVCSD_uiTaccNs   = (_G_puiCsdTaccExp[uiExp] * _G_puiCsdTaccMnt[uiMnt] + 9) / 10;
        psdcsdDec->DEVCSD_usTaccClks = __getBits(pRawCSD, 104, 8) * 100;

        uiMnt = __getBits(pRawCSD, 99, 4);
        uiExp = __getBits(pRawCSD, 96, 3);
        psdcsdDec->DEVCSD_uiTranSpeed = _G_CsdTrspExp[uiExp] * _G_CsdTrspMnt[uiMnt];
        psdcsdDec->DEVCSD_usCmdclass  = __getBits(pRawCSD, 84, 12);

        uiExp = __getBits(pRawCSD, 47, 3);
        uiMnt = __getBits(pRawCSD, 62, 12);
        psdcsdDec->DEVCSD_uiCapacity = (1 + uiMnt) << (uiExp + 2);

        psdcsdDec->DEVCSD_ucR2W_Factor      = __getBits(pRawCSD, 36, 3);

        psdcsdDec->DEVCSD_ucReadBlkLenBits  = __getBits(pRawCSD, 80, 4);
        psdcsdDec->DEVCSD_ucWriteBlkLenBits = __getBits(pRawCSD, 22, 4);

        psdcsdDec->DEVCSD_bReadMissAlign    = __getBits(pRawCSD, 77, 1);
        psdcsdDec->DEVCSD_bWriteMissAlign   = __getBits(pRawCSD, 78, 1);
        psdcsdDec->DEVCSD_bReadBlkPartial   = __getBits(pRawCSD, 79, 1);
        psdcsdDec->DEVCSD_bWriteBlkPartial  = __getBits(pRawCSD, 21, 1);

        /*
         * 擦块的大小在规范中定义：
         *   如果CSD中指明了  使能擦除(ERASE_EN = 1)那么,擦块的大小为1.在这种情况下,
         * 擦除的就仅仅是从擦除起始地址到结束地址的数据...
         *   如果CSD中指明了  禁止擦除(ERASE_EN = 0)那么 ,擦除块的大小由CSD中的SIZE_SECTOR
         * 域来指明.而且还依赖于WRT_BLK_LEN.这种情况下,指定擦除范围所在的块将全部被擦除
         */
        uiExp = __getBits(pRawCSD, 46, 1);
        if (uiExp) {
            psdcsdDec->DEVCSD_ucEraseBlkLen = 1;
        } else if (psdcsdDec->DEVCSD_ucWriteBlkLenBits >= 9) {
            psdcsdDec->DEVCSD_ucEraseBlkLen =  (__getBits(pRawCSD, 47, 3) + 1) <<
                                               (psdcsdDec->DEVCSD_ucWriteBlkLenBits - 9);
        }
        break;

    case CSD_STRUCT_VER_2_0:
        uiMnt = __getBits(pRawCSD, 99, 4);
        uiExp = __getBits(pRawCSD, 96, 3);
        psdcsdDec->DEVCSD_uiTranSpeed = _G_CsdTrspExp[uiExp] *  _G_CsdTrspMnt[uiMnt];
        psdcsdDec->DEVCSD_usCmdclass  = __getBits(pRawCSD, 84, 12);

        uiMnt = __getBits(pRawCSD, 48, 22);
        psdcsdDec->DEVCSD_uiCapacity = (1 + uiMnt) << 10;

        psdcsdDec->DEVCSD_ucReadBlkLenBits  = 9;
        psdcsdDec->DEVCSD_ucWriteBlkLenBits = 9;
        psdcsdDec->DEVCSD_ucEraseBlkLen     = 1;

        psdcsdDec->DEVCSD_ucR2W_Factor      = 4;
        psdcsdDec->DEVCSD_uiTaccNs          = 1000000;                  /*  固定为1ms                   */
        psdcsdDec->DEVCSD_usTaccClks        = 0;                        /*  固定为0                     */
        break;

    default:
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "unknown CSD structure.\r\n");
        return  (PX_ERROR);
    }

__end:

#ifdef  __SYLIXOS_DEBUG
    __printCSD(psdcsdDec);
#endif                                                                  /*  __SYLIXOS_DEBUG             */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_SdCoreDevReset
** 功能描述: 复位设备
** 输    入: psdcoredevice 设备结构指针
** 输    出: NONE
** 返    回: ERROR    CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT API_SdCoreDevReset (PLW_SDCORE_DEVICE psdcoredevice)
{
    LW_SD_COMMAND  sdcmd;
    INT            iError;
    INT            iRetry = 50;                                        /*  重试 50 次复位命令           */

    lib_bzero(&sdcmd, sizeof(LW_SD_COMMAND));
    
    sdcmd.SDCMD_uiOpcode = SD_GO_IDLE_STATE;
    sdcmd.SDCMD_uiArg    = 0;
    sdcmd.SDCMD_uiFlag   = SD_RSP_SPI_R1 | SD_RSP_NONE | SD_CMD_BC;

    do {
        SD_DELAYMS(1);
        iError = API_SdCoreDevCmd(psdcoredevice, &sdcmd, 0);
        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "send reset cmd error.\r\n");
            return  (PX_ERROR);
        }

        if (COREDEV_IS_SD(psdcoredevice)) {                             /*  SD模式下不检查响应          */
            return  (ERROR_NONE);
        }

        if ((sdcmd.SDCMD_uiResp[0] == 0x01)) {
            return  (ERROR_NONE);
        }
    } while (iRetry--);

    if (iRetry <= 0) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "reset timeout.\r\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);                                               /*  PREVENT WARNING             */
}
/*********************************************************************************************************
** 函数名称: API_SdCoreDevSendIfCond
** 功能描述: 检查卡操作接口条件(CMD8).在复位之后,主机并不知道卡能支持的电压是多少,或在哪个范围.
**           于是CMD8 (SEND_IF_COND) 就用来做这个事.
** 输    入: psdcoredevice 设备结构指针
** 输    出: NONE
** 返    回: ERROR    CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT API_SdCoreDevSendIfCond (PLW_SDCORE_DEVICE psdcoredevice)
{
    LW_SD_COMMAND   sdcmd;
    INT             iError;
    UINT8           ucChkPattern = 0xaa;                                /* 标准建议在Cmd8中使用0xaa校验 */
    UINT32          uiOCR;

    /*
     * 获得适配器的电源支持情况
     */
    iError = API_SdCoreDevCtl(psdcoredevice,
                              SDBUS_CTRL_GETOCR,
                              (LONG)&uiOCR);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "get adapter ocr failed.\r\n");
        return  (PX_ERROR);
    }

    lib_bzero(&sdcmd, sizeof(LW_SD_COMMAND));
    
    sdcmd.SDCMD_uiOpcode = SD_SEND_IF_COND;
    sdcmd.SDCMD_uiFlag   = SD_RSP_SPI_R7 | SD_RSP_R7 | SD_CMD_BCR;

    /*
     * 在SD_SEND_IF_COND(Cmd8)里面的有效参数格式:
     * ----------------------------------------------------------
     *  bits: |            4          |       8          |
     *  ......| HVS(host vol support) |  check pattern   |......
     * ----------------------------------------------------------
     * 目前版本里面的HVS只定义了一个值1表示适配器支持2.7~3.6V的电压.
     */
    sdcmd.SDCMD_uiArg    = (((uiOCR & SD_OCR_MEM_VDD_MSK) != 0) << 8) | ucChkPattern;

    iError = API_SdCoreDevCmd(psdcoredevice, &sdcmd, 0);

    /*
     * 对于Cmd8,如果卡支持参数中的uiOCR信息,那么会回送这些信息,
     * 否则,不会应答,并且卡自身进入空闲模式.
     * TODO: 告诉上层具体失败的原因.
     */
    if (iError == ERROR_NONE) {

        if (COREDEV_IS_SD(psdcoredevice)) {
            ucChkPattern = sdcmd.SDCMD_uiResp[0] & 0xff;
        } else if (COREDEV_IS_SPI(psdcoredevice)) {
            ucChkPattern = sdcmd.SDCMD_uiResp[1] & 0xff;
        } else {
            return  (PX_ERROR);
        }

        if (ucChkPattern != 0xaa) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL,
                             "the check pattern is not correct, it maybe I/0 error.\r\n");
            return  (PX_ERROR);
        } else {
            return  (ERROR_NONE);
        }
    } else {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "the device can't supply the voltage.\r\n");
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_SdCoreDevSendAppOpCond
** 功能描述: 发送ACMD41.
**           当该命令的参数OCR为0,称作"查询ACMD41",效果同CMD8.
**           当该命令的参数OCR不为0,称作"第一个ACMD41",用来启动卡初始化的同时,决定最终的操作电压.
**           该函数为了一次性测出卡的类型(MMC),所以增加了MMC的识别.
** 输    入: psdcoredevice   设备结构指针
**           uiOCR           代表适配器的OCR
** 输    出: psddevocrOut    设备应答的OCR信息
**           pucType         设备的类型
** 返    回: ERROR    CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT API_SdCoreDevSendAppOpCond (PLW_SDCORE_DEVICE  psdcoredevice,
                                UINT32             uiOCR,
                                LW_SDDEV_OCR      *psddevocrOut,
                                UINT8             *pucType)
{
    LW_SD_COMMAND   sdcmd;
    INT             iError;
    INT             i;
    BOOL            bMmc = LW_FALSE;

    if (!psddevocrOut) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    /*
     * 首先针对SD卡作初始化
     */
    lib_bzero(&sdcmd, sizeof(LW_SD_COMMAND));
    
    sdcmd.SDCMD_uiOpcode = APP_SD_SEND_OP_COND;

    /*
     * 存储卡设备的电压范围为2.7 ~ 3.6 v,如果主控提供的电压有不在这个范围内的,则该函数会失败.
     */
    sdcmd.SDCMD_uiArg  = (uiOCR & SD_OCR_HCS) 
                       ? (uiOCR & SD_OCR_MEM_VDD_MSK) | SD_OCR_HCS 
                       : (uiOCR & SD_OCR_MEM_VDD_MSK);
    sdcmd.SDCMD_uiFlag = SD_RSP_SPI_R1| SD_RSP_R3 | SD_CMD_BCR;

    for (i = 0; i < (SD_OPCOND_DELAY_CONTS); i++) {
        iError = API_SdCoreDevAppCmd(psdcoredevice,
                                     &sdcmd,
                                     LW_FALSE,
                                     SD_CMD_GEN_RETRY);
        if (iError != ERROR_NONE) {
            goto    __mmc_ident;
        }

        /*
         * 对于查询ACMD41,只需要一次
         */
        if (uiOCR == 0) {
            break;
        }

        if (COREDEV_IS_SPI(psdcoredevice)) {
            if (!(sdcmd.SDCMD_uiResp[0] & R1_SPI_IDLE)) {               /*  spi 模式下,r1 的最低位清零  */
                break;
            }
        } else {
            if (sdcmd.SDCMD_uiResp[0] & SD_OCR_BUSY) {                  /*  busy置位,表示完成           */
                                                                        /*  进入ready 状态              */
                break;
            }
        }

        SD_DELAYMS(2);
    }

    if (i >= SD_OPCOND_DELAY_CONTS) {                                   /*  sd卡识别失败                */
        goto    __mmc_ident;
    } else {
        goto    __ident_done;
    }

__mmc_ident:                                                            /*  mmc 识别                    */
    /*
     * 加入MMC的识别
     */
    API_SdCoreDevReset(psdcoredevice);

    sdcmd.SDCMD_uiOpcode = SD_SEND_OP_COND;
    sdcmd.SDCMD_uiArg    = COREDEV_IS_SPI(psdcoredevice) ? 0 : (uiOCR | SD_OCR_HCS);
    sdcmd.SDCMD_uiFlag   = SD_RSP_SPI_R1 | SD_RSP_R3 | SD_CMD_BCR;
    sdcmd.SDCMD_uiRetry  = 3;
    for (i = 0; i < SD_OPCOND_DELAY_CONTS; i++) {
        iError = API_SdCoreDevCmd(psdcoredevice, &sdcmd, 0);
        if (iError != ERROR_NONE) {                                     /*  错误退出                    */
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "can't send cmd1.\r\n");
            return  (iError);
        }

        if (uiOCR == 0) {
            break;
        }

        if (COREDEV_IS_SPI(psdcoredevice)) {
            if (!(sdcmd.SDCMD_uiResp[0] & R1_SPI_IDLE)) {               /*  spi 模式下,r1 的最低位清零  */
                break;
            }
        } else {
            if (sdcmd.SDCMD_uiResp[0] & SD_OCR_BUSY) {                  /*  busy置位,表示完成           */
                                                                        /*  进入ready 状态              */
                break;
            }
        }
    }

    if (i >= SD_OPCOND_DELAY_CONTS) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "timeout(may device is not sd or mmc card).\r\n");
        return  (PX_ERROR);
    } else {
        bMmc = LW_TRUE;
    }

__ident_done:                                                           /*  识别完成                    */
    if (COREDEV_IS_SD(psdcoredevice)) {
        *psddevocrOut = sdcmd.SDCMD_uiResp[0];
    } else {
        /*
         * SPI 模式下使用专有命令读取设备OCR寄存器
         */
        lib_bzero(&sdcmd, sizeof(LW_SD_COMMAND));
        
        sdcmd.SDCMD_uiOpcode = SD_SPI_READ_OCR;
        sdcmd.SDCMD_uiArg    = 0;                                       /*  TODO: 默认支持HIGH CAP      */
        sdcmd.SDCMD_uiFlag   = SD_RSP_SPI_R3;

        iError = API_SdCoreDevCmd(psdcoredevice, &sdcmd, 1);
        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "spi read ocr error.\r\n");
            return  (iError);
        }

        *psddevocrOut = sdcmd.SDCMD_uiResp[1];
    }

    /*
     * 判断设备类型
     * TODO:SDXC ?
     */
    if (bMmc) {
        *pucType = SDDEV_TYPE_MMC;
    } else if (*psddevocrOut & SD_OCR_HCS) {
        *pucType = SDDEV_TYPE_SDHC;
    } else {
        *pucType = SDDEV_TYPE_SDSC;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_SdCoreDevSendRelativeAddr
** 功能描述: 让卡发送自己的本地地址.
**           !!注意,该函数只能用于SD总线上的设备.
** 输    入: psdcoredevice   设备结构指针
** 输    出: puiRCA          卡应答的RCA地址
** 返    回: ERROR    CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT API_SdCoreDevSendRelativeAddr (PLW_SDCORE_DEVICE psdcoredevice, UINT32 *puiRCA)
{
    INT              iError;
    LW_SD_COMMAND    sdcmd;

    if (!puiRCA) {
        return  (PX_ERROR);
    }

    if (!COREDEV_IS_SD(psdcoredevice)) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "function just for sd bus.\r\n");
        return  (PX_ERROR);
    }

    lib_bzero(&sdcmd, sizeof(LW_SD_COMMAND));
    
    sdcmd.SDCMD_uiOpcode = SD_SEND_RELATIVE_ADDR;
    sdcmd.SDCMD_uiArg    = 0;
    sdcmd.SDCMD_uiFlag   = SD_RSP_R6 | SD_CMD_BCR;

    iError = API_SdCoreDevCmd(psdcoredevice,
                              &sdcmd,
                              SD_CMD_GEN_RETRY);

    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "send cmd error.\r\n");
        return  (PX_ERROR);
    }

    *puiRCA = sdcmd.SDCMD_uiResp[0] >> 16;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_SdCoreDevMmcSetRelativeAddr
** 功能描述: 设置MMC卡的RCA.
**           MMC的本地地址是由用户设置的(SD是由卡获得的).
** 输    入: psdcoredevice   设备结构指针
**           uiRCA           MMC的RCA
** 输    出: NONE
** 返    回: ERROR    CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT API_SdCoreDevMmcSetRelativeAddr (PLW_SDCORE_DEVICE psdcoredevice, UINT32 uiRCA)
{
    INT              iError;
    LW_SD_COMMAND    sdcmd;

    if (!COREDEV_IS_SD(psdcoredevice)) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "function just for sd bus.\r\n");
        return  (PX_ERROR);
    }

    lib_bzero(&sdcmd, sizeof(LW_SD_COMMAND));
    
    sdcmd.SDCMD_uiOpcode = SD_SEND_RELATIVE_ADDR;
    sdcmd.SDCMD_uiArg    = uiRCA << 16;
    sdcmd.SDCMD_uiFlag   = SD_RSP_R1 | SD_CMD_AC;

    iError = API_SdCoreDevCmd(psdcoredevice,
                              &sdcmd,
                              SD_CMD_GEN_RETRY);

    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "send cmd error.\r\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_SdCoreDevSendAllCID
** 功能描述: 获得卡的CID
** 输    入: psdcoredevice   设备结构指针
** 输    出: psdcid          卡应答的CID(已经解码)
** 返    回: ERROR    CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT API_SdCoreDevSendAllCID (PLW_SDCORE_DEVICE psdcoredevice, LW_SDDEV_CID *psdcid)
{
    INT              iError;
    LW_SD_COMMAND    sdcmd;
    UINT8            pucCidBuf[16];
    UINT8            ucType;

    if (!psdcid) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        return  (PX_ERROR);
    }

    lib_bzero(&sdcmd, sizeof(LW_SD_COMMAND));
    
    sdcmd.SDCMD_uiOpcode = COREDEV_IS_SD(psdcoredevice) ? SD_ALL_SEND_CID : SD_SEND_CID;
    sdcmd.SDCMD_uiArg    = 0;
    sdcmd.SDCMD_uiFlag   = SD_RSP_SPI_R1 | SD_RSP_R2 | SD_CMD_BCR;
    iError = API_SdCoreDevCmd(psdcoredevice,
                              &sdcmd,
                              SD_CMD_GEN_RETRY);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "send cmd error.\r\n");
        return  (PX_ERROR);
    }

    /*
     * SD模式下直接用命令获取CID
     */
    API_SdCoreDevTypeView(psdcoredevice, &ucType);
    if (COREDEV_IS_SD(psdcoredevice)) {
        API_SdCoreDecodeCID(psdcid, sdcmd.SDCMD_uiResp, ucType);
        return  (ERROR_NONE);
    }

    /*
     * SPI模式使用特殊的读CID寄存器方式
     */
    API_SdCoreSpiRegisterRead(psdcoredevice, pucCidBuf, 16);
    API_SdCoreSpiCxdFormat(sdcmd.SDCMD_uiResp, pucCidBuf);
    API_SdCoreDecodeCID(psdcid, sdcmd.SDCMD_uiResp, ucType);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_SdCoreDevSendAllCSD
** 功能描述: 获得卡的CSD
** 输    入: psdcoredevice   设备结构指针
** 输    出: psdcsd          卡应答的CSD(已经解码)
** 返    回: ERROR    CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT API_SdCoreDevSendAllCSD (PLW_SDCORE_DEVICE psdcoredevice, LW_SDDEV_CSD *psdcsd)
{
    INT              iError;
    LW_SD_COMMAND    sdcmd;
    UINT32           uiRca;
    UINT8            pucCsdBuf[16];
    UINT8            ucType;

    if (!psdcsd) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        return  (PX_ERROR);
    }

    iError = API_SdCoreDevRcaView(psdcoredevice, &uiRca);

    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "device without rca.\r\n");
        return  (PX_ERROR);
    }

    lib_bzero(&sdcmd, sizeof(LW_SD_COMMAND));
    
    sdcmd.SDCMD_uiOpcode = SD_SEND_CSD;
    sdcmd.SDCMD_uiArg    = COREDEV_IS_SPI(psdcoredevice) ? 0 : uiRca << 16;
    sdcmd.SDCMD_uiFlag   = SD_RSP_SPI_R1 | SD_RSP_R2 | SD_CMD_BCR;
    iError = API_SdCoreDevCmd(psdcoredevice,
                              &sdcmd,
                              SD_CMD_GEN_RETRY);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "send cmd error.\r\n");
        return  (PX_ERROR);
    }

    /*
     * SD模式直接从命令的应答中获取
     */
    API_SdCoreDevTypeView(psdcoredevice, &ucType);
    if (COREDEV_IS_SD(psdcoredevice)) {
        API_SdCoreDecodeCSD(psdcsd, sdcmd.SDCMD_uiResp, ucType);
        return  (ERROR_NONE);
    }

    /*
     * SPI 模式
     */
    API_SdCoreSpiRegisterRead(psdcoredevice, pucCsdBuf, 16);
    API_SdCoreSpiCxdFormat(sdcmd.SDCMD_uiResp, pucCsdBuf);
    API_SdCoreDecodeCSD(psdcsd, sdcmd.SDCMD_uiResp, ucType);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdCoreSendScr
** 功能描述: 发送命令获得卡的CSR
** 输    入: psdcoredevice   设备结构指针
** 输    出: puiScr          SCR 原始数据
** 返    回: ERROR    CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdCoreSendScr (PLW_SDCORE_DEVICE psdcoredevice, UINT32 *puiScr)
{
    LW_SD_COMMAND   sdcmd;
    LW_SD_DATA      sddat;
    LW_SD_MESSAGE   sdmsg;
    INT             iRet;

    iRet = API_SdCoreDevAppSwitch(psdcoredevice, LW_FALSE);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    lib_bzero(&sdmsg, sizeof(sdmsg));
    lib_bzero(&sdcmd, sizeof(sdcmd));

    sdcmd.SDCMD_uiOpcode  = APP_SEND_SCR;
    sdcmd.SDCMD_uiArg     = 0;
    sdcmd.SDCMD_uiFlag    = SD_RSP_SPI_R1 | SD_RSP_R1 | SD_CMD_ADTC;

    sddat.SDDAT_uiBlkNum  = 1;
    sddat.SDDAT_uiBlkSize = 8;
    sddat.SDDAT_uiFlags   = SD_DAT_READ;

    sdmsg.SDMSG_psdcmdCmd   = &sdcmd;
    sdmsg.SDMSG_psddata     = &sddat;
    sdmsg.SDMSG_pucRdBuffer = (UINT8 *)puiScr;
    iRet = API_SdCoreDevTransfer(psdcoredevice, &sdmsg, 1);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    puiScr[0] = be32toh(puiScr[0]);
    puiScr[1] = be32toh(puiScr[1]);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_SdCoreDevSendAllSCR
** 功能描述: 获得并解析卡的SCR
** 输    入: psdcoredevice   设备结构指针
** 输    出: psdscr          卡应答的SCR(已经解码)
** 返    回: ERROR    CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT API_SdCoreDevSendAllSCR (PLW_SDCORE_DEVICE psdcoredevice, LW_SDDEV_SCR *psdscr)
{
    UINT32   uiScr[4];
    INT      iRet;

    uiScr[0] = 0;
    uiScr[1] = 0;
    iRet = __sdCoreSendScr(psdcoredevice, uiScr + 2);
    if (iRet != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "send scr error.\r\n");
        return  (PX_ERROR);
    }

    psdscr->DEVSCR_ucSdaVsn   = __getBits(uiScr, 56, 4);
    psdscr->DEVSCR_ucBusWidth = __getBits(uiScr, 48, 4);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdCoreSelectDev
** 功能描述: 设备选择
** 输    入: psdcoredevice   设备结构指针
**           bSel            选择\取消
** 输    出: NONE
** 返    回: ERROR    CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdCoreSelectDev (PLW_SDCORE_DEVICE psdcoredevice, BOOL bSel)
{
    INT             iError;
    LW_SD_COMMAND   sdcmd;
    UINT32          uiRca;

    if (COREDEV_IS_SPI(psdcoredevice)) {                                /*  SPI设备使用物理片选         */
        return  (ERROR_NONE);
    }

    iError = API_SdCoreDevRcaView(psdcoredevice, &uiRca);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "device without rca.\r\n");
        return  (PX_ERROR);
    }

    lib_bzero(&sdcmd, sizeof(LW_SD_COMMAND));
    
    sdcmd.SDCMD_uiOpcode   = SD_SELECT_CARD;

    if (bSel) {
        sdcmd.SDCMD_uiArg  = uiRca << 16;
        sdcmd.SDCMD_uiFlag = SD_RSP_R1B | SD_CMD_AC;
    } else {
        sdcmd.SDCMD_uiArg  = 0;
        sdcmd.SDCMD_uiFlag = SD_RSP_NONE | SD_CMD_BC;
    }

    iError = API_SdCoreDevCmd(psdcoredevice,
                              &sdcmd,
                              SD_CMD_GEN_RETRY);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "send cmd error.\r\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
 ** 函数名称: API_SdCoreDevSelect
 ** 功能描述: 设备选择
 ** 输    入: psdcoredevice     设备结构指针
 ** 输    出: NONE
 ** 返    回: ERROR    CODE
 ** 全局变量:
 ** 调用模块:
 ********************************************************************************************************/
INT API_SdCoreDevSelect (PLW_SDCORE_DEVICE psdcoredevice)
{
    INT iError;

    iError = __sdCoreSelectDev(psdcoredevice, LW_TRUE);

    return  (iError);
}
/*********************************************************************************************************
 ** 函数名称: API_SdCoreDevDeSelect
 ** 功能描述: 设备取消
 ** 输    入: psdcoredevice     设备结构指针
 ** 输    出: NONE
 ** 返    回: ERROR    CODE
 ** 全局变量:
 ** 调用模块:
 ********************************************************************************************************/
INT API_SdCoreDevDeSelect (PLW_SDCORE_DEVICE psdcoredevice)
{
    INT iError;

    iError = __sdCoreSelectDev(psdcoredevice, LW_FALSE);

    return  (iError);
}
/*********************************************************************************************************
 ** 函数名称: API_SdCoreDevSetBusWidth
 ** 功能描述: 设置总线通信位宽
 ** 输    入: psdcoredevice     设备结构指针
 **           iBusWidth         总线宽度
 ** 输    出: NONE
 ** 返    回: ERROR    CODE
 ** 全局变量:
 ** 调用模块:
 ********************************************************************************************************/
INT API_SdCoreDevSetBusWidth (PLW_SDCORE_DEVICE psdcoredevice, INT iBusWidth)
{
    INT             iError = ERROR_NONE;
    LW_SD_COMMAND   sdcmd;

    iError = API_SdCoreDevSelect(psdcoredevice);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "select device failed.\r\n");
        return  (PX_ERROR);
    }

    if (!psdcoredevice) {
        API_SdCoreDevDeSelect(psdcoredevice);
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if ((iBusWidth != SDBUS_WIDTH_1) && (iBusWidth != SDBUS_WIDTH_4)) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "invalid bus width in current sd version.\r\n");
        API_SdCoreDevDeSelect(psdcoredevice);
        return  (PX_ERROR);
    }

    /*
     * 只在SD模式下才设置, SPI模式下忽略
     */
    if (COREDEV_IS_SD(psdcoredevice)) {
        lib_bzero(&sdcmd, sizeof(sdcmd));
        
        sdcmd.SDCMD_uiOpcode = APP_SET_BUS_WIDTH;
        sdcmd.SDCMD_uiFlag   = SD_RSP_R1 | SD_CMD_AC;
        sdcmd.SDCMD_uiArg    = iBusWidth;

        iError = API_SdCoreDevAppCmd(psdcoredevice,
                                     &sdcmd,
                                     LW_FALSE,
                                     0);
    }
    API_SdCoreDevDeSelect(psdcoredevice);

    return  (iError);
}
/*********************************************************************************************************
 ** 函数名称: API_SdCoreDevSetBlkLen
 ** 功能描述: 设置记忆卡的块长度.该函数影响之后的数据读、写、锁定中的块大小参数.
 **           但是在SDHC和SDXC中,只影响锁定命令中的参数(因为读写固定为512byte块大小).
 ** 输    入: psdcoredevice   设备结构指针
 **           iBlkLen         块大小
 ** 输    出: NONE
 ** 返    回: ERROR    CODE
 ** 全局变量:
 ** 调用模块:
 ********************************************************************************************************/
INT API_SdCoreDevSetBlkLen (PLW_SDCORE_DEVICE psdcoredevice, INT iBlkLen)
{
    INT             iError;
    LW_SD_COMMAND   sdcmd;

    sdcmd.SDCMD_uiOpcode = SD_SET_BLOCKLEN;
    sdcmd.SDCMD_uiArg    = iBlkLen;
    sdcmd.SDCMD_uiFlag   = SD_RSP_SPI_R1 | SD_RSP_R1 | SD_CMD_AC;

    iError = API_SdCoreDevSelect(psdcoredevice);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "select device failed.\r\n");
        return  (PX_ERROR);
    }

    iError = API_SdCoreDevCmd(psdcoredevice, &sdcmd, 0);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "send cmd16 failed.\r\n");
        API_SdCoreDevDeSelect(psdcoredevice);
        return  (PX_ERROR);
    }

    if (COREDEV_IS_SPI(psdcoredevice)) {
        if ((sdcmd.SDCMD_uiResp[0] & 0xff) != 0) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "spi response error.\r\n");
            return  (PX_ERROR);
        }
    }

    return  (API_SdCoreDevDeSelect(psdcoredevice));
}
/*********************************************************************************************************
 ** 函数名称: API_SdCoreDevGetStatus
 ** 功能描述: 得到卡的状态字
 ** 输    入: psdcoredevice   设备结构指针
 ** 输    出: puiStatus       状态
 ** 返    回: ERROR    CODE
 ** 全局变量:
 ** 调用模块:
 ********************************************************************************************************/
INT API_SdCoreDevGetStatus (PLW_SDCORE_DEVICE psdcoredevice, UINT32 *puiStatus)
{
    LW_SD_COMMAND  sdcmd;
    UINT32         uiRca;
    INT            iError;

    API_SdCoreDevRcaView(psdcoredevice, &uiRca);

    lib_bzero(&sdcmd, sizeof(sdcmd));
    
    sdcmd.SDCMD_uiOpcode = SD_SEND_STATUS;
    sdcmd.SDCMD_uiFlag   = SD_RSP_SPI_R2 | SD_RSP_R1 | SD_CMD_AC;
    sdcmd.SDCMD_uiArg    = uiRca << 16;
    sdcmd.SDCMD_uiRetry  = 0;

    iError = API_SdCoreDevCmd(psdcoredevice, &sdcmd, 1);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "send cmd13 failed.\r\n");
        return  (PX_ERROR);
    }

    *puiStatus = sdcmd.SDCMD_uiResp[0];

    return  (ERROR_NONE);
}
/*********************************************************************************************************
 ** 函数名称: API_SdCoreDevSetPreBlkLen
 ** 功能描述: 设置预先擦除块计数.在 "<多块写>" 操作中,该功能用于让卡预先擦除指定的块大小,这样可以提高速度.
 ** 输    入: psdcoredevice   设备结构指针
 **           iPreBlkLen   块大小
 ** 输    出: NONE
 ** 返    回: ERROR    CODE
 ** 全局变量:
 ** 调用模块:
 ********************************************************************************************************/
INT API_SdCoreDevSetPreBlkLen (PLW_SDCORE_DEVICE psdcoredevice, INT iPreBlkLen)
{
    LW_SD_COMMAND  sdcmd;
    INT            iError;

    lib_bzero(&sdcmd, sizeof(sdcmd));
    
    sdcmd.SDCMD_uiOpcode = SD_SET_BLOCK_COUNT;
    sdcmd.SDCMD_uiFlag   = SD_RSP_SPI_R1 | SD_RSP_R1 | SD_CMD_AC;
    sdcmd.SDCMD_uiArg    = iPreBlkLen;
    sdcmd.SDCMD_uiRetry  = 0;

    iError = API_SdCoreDevAppCmd(psdcoredevice,
                                 &sdcmd,
                                 LW_FALSE,
                                 1);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "send acmd23 failed.\r\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
 ** 函数名称: API_SdCoreDevIsBlockAddr
 ** 功能描述: 判断设备是否是块寻址
 ** 输    入: psdcoredevice     设备结构指针
 ** 输    出: pbResult          结果
 ** 返    回: ERROR    CODE
 ** 全局变量:
 ** 调用模块:
 ********************************************************************************************************/
INT API_SdCoreDevIsBlockAddr (PLW_SDCORE_DEVICE psdcoredevice, BOOL *pbResult)
{
    UINT8   ucType;

    API_SdCoreDevTypeView(psdcoredevice, &ucType);

    switch (ucType) {
    
    case SDDEV_TYPE_MMC  :
        if (psdcoredevice->COREDEV_iDevSta & COREDEV_STA_HIGHCAP_OCR) {
            *pbResult = LW_TRUE;
        } else {
            *pbResult = LW_FALSE;
        }
        break;

    case SDDEV_TYPE_SDSC :
        *pbResult = LW_FALSE;
        break;

    case SDDEV_TYPE_SDHC :
    case SDDEV_TYPE_SDXC :
        *pbResult = LW_TRUE;
        break;

    case SDDEV_TYPE_SDIO :
    case SDDEV_TYPE_COMM :
    default:
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "don't support.\r\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
 ** 函数名称: API_SdCoreDevSpiClkDely
 ** 功能描述: 在SPI模式下的时钟延时
 ** 输    入: psdcoredevice     设备结构指针
 **           iClkConts         延时clock个数
 ** 输    出: NONE
 ** 返    回: ERROR    CODE
 ** 全局变量:
 ** 调用模块:
 ********************************************************************************************************/
INT API_SdCoreDevSpiClkDely (PLW_SDCORE_DEVICE psdcoredevice, INT iClkConts)
{
    INT iError;

    iClkConts = (iClkConts + 7) / 8;

    iError = API_SdCoreDevCtl(psdcoredevice, SDBUS_CTRL_DELAYCLK, iClkConts);
    return  (iError);
}
/*********************************************************************************************************
 ** 函数名称: API_SdCoreDevSpiCrcEn
 ** 功能描述: 在SPI模式下使能设备的CRC校验
 ** 输    入: psdcoredevice     设备结构指针
 **           bEnable           是否使能.(0:禁止crc  1:使能crc)
 ** 输    出: NONE
 ** 返    回: ERROR    CODE
 ** 全局变量:
 ** 调用模块:
 ********************************************************************************************************/
INT API_SdCoreDevSpiCrcEn (PLW_SDCORE_DEVICE psdcoredevice, BOOL bEnable)
{
    LW_SD_COMMAND  sdcmd;
    INT            iError;

    lib_bzero(&sdcmd, sizeof(sdcmd));
    
    sdcmd.SDCMD_uiOpcode = SD_SPI_CRC_ON_OFF;
    sdcmd.SDCMD_uiFlag   = SD_RSP_SPI_R1;
    sdcmd.SDCMD_uiArg    = bEnable ? 1 : 0;
    sdcmd.SDCMD_uiRetry  = 0;

    iError = API_SdCoreDevCmd(psdcoredevice, &sdcmd, 0);
    if (iError == ERROR_NONE) {
        if ((sdcmd.SDCMD_uiResp[0] & 0xff) != 0x00) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "set crc failed.\r\n");
            return  (PX_ERROR);
        } else {
            return  (ERROR_NONE);
        }
    }

    SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "send cmd error.\r\n");

    return  (PX_ERROR);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_SDCARD_EN > 0)      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
