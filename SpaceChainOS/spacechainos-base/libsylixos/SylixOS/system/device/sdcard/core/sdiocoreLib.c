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
** 文   件   名: sdiocoreLib.c
**
** 创   建   人: Zeng.Bo (曾波)
**
** 文件创建日期: 2014 年 10 月 28 日
**
** 描        述: sdio 卡特殊操作接口源文件

** BUG:
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SDCARD_EN > 0) && (LW_CFG_SDCARD_SDIO_EN > 0)
#include "sdstd.h"
#include "sdiostd.h"
#include "sddrvm.h"
#include "sdiodrvm.h"
#include "sdcore.h"
#include "sdiocoreLib.h"
#include "../include/sddebug.h"
/*********************************************************************************************************
 SDIO CIS 相关数据结构
*********************************************************************************************************/
typedef  INT (*CIS_TPL_PARSE_FUNC)(SDIO_FUNC *psdiofunc, const UINT8 *cpucData, UINT32 uiSize);

typedef struct __cis_tpl_parser {
    UINT8               PARSER_ucCode;
    UINT8               PARSER_ucMinSz;
    CIS_TPL_PARSE_FUNC  PARSER_pfuncParse;
    CPCHAR              PARSER_cpcDesc;
} __CIS_TPL_PARSER;
/*********************************************************************************************************
  内部函数声明
*********************************************************************************************************/
static INT __cistplParseFunceCommon(SDIO_FUNC *psdiofunc, const UINT8 *cpucData, UINT32 uiSize);
static INT __cistplParseFunceFuncN(SDIO_FUNC *psdiofunc, const UINT8 *cpucData, UINT32 uiSize);
static INT __cistplParseFunce(SDIO_FUNC *psdiofunc, const UINT8 *cpucData, UINT32 uiSize);
static INT __cistplParseManfid(SDIO_FUNC *psdiofunc, const UINT8 *cpucData, UINT32 uiSize);
static INT __cistplParseVers_1(SDIO_FUNC *psdiofunc, const UINT8 *cpucData, UINT32 uiSize);

static INT __cistplParse(SDIO_FUNC                 *psdiofunc,
                         const __CIS_TPL_PARSER    *cpParser,
                         INT                        iParserCnt,
                         UINT8                      ucCode,
                         const UINT8               *cpucData,
                         UINT32                     uiSize);
/*********************************************************************************************************
  Funce Comme 解析查找表
*********************************************************************************************************/
static const UINT8 _G_pucSpeedVal[16]  = {
        0, 10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80
};
static const UINT32 _G_puiSpeedUnit[8] = {
        10000, 100000, 1000000, 10000000, 0, 0, 0, 0
};
/*********************************************************************************************************
  解析器表
*********************************************************************************************************/
static const __CIS_TPL_PARSER _G_cistplparserFunceTbl[] = {
    {0x00,   4,          __cistplParseFunceCommon, "funce comme"   },
    {0x01,   0,          __cistplParseFunceFuncN,  "funce func1_7" },
    {0x04,   1 + 1 + 6,  LW_NULL,                  "unknow"},
};
static const __CIS_TPL_PARSER _G_cistplparserTbl[] = {
    {0x15,   3,  __cistplParseVers_1, "version_1" },
    {0x20,   4,  __cistplParseManfid, "manfid"    },
    {0x21,   2,  LW_NULL            , "null"      },
    {0x22,   0,  __cistplParseFunce , "funce"     },
};
#define __NELE(array)       (sizeof(array) / sizeof(array[0]))
/*********************************************************************************************************
** 函数名称: API_SdioCoreDevReset
** 功能描述: SDIO 设备复位
** 输    入: psdcoredev       核心设备传输对象
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT API_SdioCoreDevReset (PLW_SDCORE_DEVICE   psdcoredev)
{
    INT   iRet;
    UINT8 ucAbort;

    /*
     * SDIO Simplified Specification V2.0, 4.4 Reset for SDIO
     */
    iRet = API_SdioCoreDevRwDirect(psdcoredev, 0, 0, SDIO_CCCR_ABORT, 0, &ucAbort);
    if (iRet) {
        ucAbort = 0x08;
    } else {
        ucAbort |= 0x08;
    }
    iRet = API_SdioCoreDevRwDirect(psdcoredev, 1, 0, SDIO_CCCR_ABORT, ucAbort, LW_NULL);

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_SdioCoreDevSendIoOpCond
** 功能描述: SDIO 设备 操作条件 设置/查看 命令
** 输    入: psdcoredev       核心设备传输对象
**           uiOcr            设置的OCR
**           puiOcrOut        设备返回的OCR
** 输    出:
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT API_SdioCoreDevSendIoOpCond (PLW_SDCORE_DEVICE   psdcoredev, UINT32 uiOcr, UINT32 *puiOcrOut)
{
    LW_SD_COMMAND  sdcmd;
    INT            iError = ERROR_NONE;
    INT            i;

    lib_bzero(&sdcmd, sizeof(LW_SD_COMMAND));

    sdcmd.SDCMD_uiOpcode = SDIO_SEND_OP_COND;
    sdcmd.SDCMD_uiArg    = uiOcr;
    sdcmd.SDCMD_uiFlag   = SD_RSP_SPI_R4 | SD_RSP_R4 | SD_CMD_BCR;

    for (i = 100; i; i--) {
        iError = API_SdCoreDevCmd(psdcoredev, &sdcmd, SD_CMD_GEN_RETRY);
        if (iError != ERROR_NONE) {
            continue;
        }

        /*
         * if just probing, do a single pass
         */
        if (uiOcr == 0) {
            break;
        }

        /*
         * otherwise wait until reset completes
         */
        if (COREDEV_IS_SPI(psdcoredev)) {
            /*
             * Both R1_SPI_IDLE and MMC_CARD_BUSY indicate
             * an initialized card under SPI, but some cards
             * (Marvell's) only behave when looking at this
             * one.
             */
            if (sdcmd.SDCMD_uiResp[1] & SD_OCR_BUSY) {
                break;
            }
        } else {
            if (sdcmd.SDCMD_uiResp[0] & SD_OCR_BUSY) {
                break;
            }
        }

        iError = PX_ERROR;

        SD_DELAYMS(10);
    }

    if (puiOcrOut) {
        *puiOcrOut = sdcmd.SDCMD_uiResp[COREDEV_IS_SPI(psdcoredev) ? 1 : 0];
    }

    return  (iError);
}
/*********************************************************************************************************
** 函数名称: API_SdioCoreDevReadByte
** 功能描述: 读取指定 IO 功能上指定地址的一字节数据
** 输    入: psdcoredev       核心设备传输对象
**           uiFn             功能号
**           uiAddr           地址
**           pucByte          返回读取的字节
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT API_SdioCoreDevReadByte (PLW_SDCORE_DEVICE   psdcoredev,
                             UINT32              uiFn,
                             UINT32              uiAddr,
                             UINT8              *pucByte)
{
    INT iRet;

    iRet = API_SdioCoreDevRwDirect(psdcoredev, 0, uiFn, uiAddr, 0, pucByte);

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_SdioCoreDevWriteByte
** 功能描述: 向指定 IO 功能上指定地址写一个字节的数据
** 输    入: psdcoredev       核心设备传输对象
**           uiFn             功能号
**           uiAddr           地址
**           ucByte           要写入的字节
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT API_SdioCoreDevWriteByte (PLW_SDCORE_DEVICE   psdcoredev,
                              UINT32              uiFn,
                              UINT32              uiAddr,
                              UINT8               ucByte)
{
    INT iRet;

    iRet = API_SdioCoreDevRwDirect(psdcoredev, 1, uiFn, uiAddr, ucByte, LW_NULL);

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_SdioCoreDevWriteThenReadByte
** 功能描述: 写入一个字节, 随后读回一个字节
** 输    入: psdcoredev       核心设备传输对象
**           uiFn             功能号
**           uiAddr           地址
**           ucWrByte         要写入的字节
**           pucRdByte        保存读回的字节
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT API_SdioCoreDevWriteThenReadByte (PLW_SDCORE_DEVICE   psdcoredev,
                                      UINT32              uiFn,
                                      UINT32              uiAddr,
                                      UINT8               ucWrByte,
                                      UINT8              *pucRdByte)
{
    INT iRet;

    iRet = API_SdioCoreDevRwDirect(psdcoredev, 1, uiFn, uiAddr, ucWrByte, pucRdByte);

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_SdioCoreDevFuncEn
** 功能描述: 使能一个功能
** 输    入: psdcoredev       核心设备传输对象
**           psdiofunc        功能描述对象
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT API_SdioCoreDevFuncEn (PLW_SDCORE_DEVICE   psdcoredev,
                           SDIO_FUNC          *psdiofunc)
{
    INT               iRet;
    UINT8             ucReg;
    
    struct timespec   tvDead;
    struct timespec   tvTimeout;
    struct timespec   tvNow;

    iRet = API_SdioCoreDevRwDirect(psdcoredev, 0, 0, SDIO_CCCR_IOEX, 0, &ucReg);
    if (iRet) {
        goto    __err;
    }

    ucReg |= 1 << psdiofunc->FUNC_uiNum;
    iRet = API_SdioCoreDevRwDirect(psdcoredev, 1, 0, SDIO_CCCR_IOEX, ucReg, LW_NULL);
    if (iRet) {
        goto    __err;
    }

    tvTimeout.tv_sec  = (time_t)(psdiofunc->FUNC_uiEnableTimeout / 1000);
    tvTimeout.tv_nsec = (LONG)((psdiofunc->FUNC_uiEnableTimeout % 1000) * 1000 * 1000);
    
    lib_clock_gettime(CLOCK_MONOTONIC, &tvDead);
    __timespecAdd(&tvDead, &tvTimeout);

    while (1) {
        iRet = API_SdioCoreDevRwDirect(psdcoredev, 0, 0, SDIO_CCCR_IORX, 0, &ucReg);
        if (iRet) {
            goto    __err;
        }

        if (ucReg & (1 << psdiofunc->FUNC_uiNum)) {
            break;
        }

        iRet = PX_ERROR;
        lib_clock_gettime(CLOCK_MONOTONIC, &tvNow);
        if (__timespecLeftTime(&tvDead, &tvNow)) {
            goto    __err;
        }
    }

    return  (ERROR_NONE);

__err:
    SDCARD_DEBUG_MSGX(__ERRORMESSAGE_LEVEL, "enable func(%d) failed.\r\n", psdiofunc->FUNC_uiNum);

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_SdioCoreDevFuncDis
** 功能描述: 禁止一个功能
** 输    入: psdcoredev       核心设备传输对象
**           psdiofunc        功能描述对象
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT API_SdioCoreDevFuncDis (PLW_SDCORE_DEVICE   psdcoredev,
                            SDIO_FUNC          *psdiofunc)
{
    INT     iRet;
    UINT8   ucReg;

    iRet = API_SdioCoreDevRwDirect(psdcoredev, 0, 0, SDIO_CCCR_IOEX, 0, &ucReg);
    if (iRet) {
        goto    __err;
    }

    ucReg &= ~(1 << psdiofunc->FUNC_uiNum);
    iRet = API_SdioCoreDevRwDirect(psdcoredev, 1, 0, SDIO_CCCR_IOEX, ucReg, LW_NULL);
    if (iRet) {
        goto    __err;
    }

    return  (ERROR_NONE);

__err:
    SDCARD_DEBUG_MSGX(__ERRORMESSAGE_LEVEL, "enable func(%d) failed.\r\n", psdiofunc->FUNC_uiNum);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: API_SdioCoreDevFuncIntEn
** 功能描述: 使能一个功能的中断
** 输    入: psdcoredev       核心设备传输对象
**           psdiofunc        功能描述对象
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT API_SdioCoreDevFuncIntEn (PLW_SDCORE_DEVICE   psdcoredev,
                              SDIO_FUNC          *psdiofunc)
{
    INT     iRet;
    UINT8   ucReg;

    iRet = API_SdioCoreDevRwDirect(psdcoredev, 0, 0, SDIO_CCCR_IENX, 0, &ucReg);
    if (iRet) {
        goto    __err;
    }

    ucReg |= 1 << psdiofunc->FUNC_uiNum;
    ucReg |= 1;
    iRet = API_SdioCoreDevRwDirect(psdcoredev, 1, 0, SDIO_CCCR_IENX, ucReg, LW_NULL);
    if (iRet) {
        goto    __err;
    }

    return  (ERROR_NONE);

__err:
    SDCARD_DEBUG_MSGX(__ERRORMESSAGE_LEVEL, "enable func(%d) interrupt failed.\r\n",
                      psdiofunc->FUNC_uiNum);

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_SdioCoreDevFuncIntDis
** 功能描述: 禁止一个功能的中断
** 输    入: psdcoredev       核心设备传输对象
**           psdiofunc        功能描述对象
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT API_SdioCoreDevFuncIntDis (PLW_SDCORE_DEVICE   psdcoredev,
                               SDIO_FUNC          *psdiofunc)
{
    INT     iRet;
    UINT8   ucReg;

    iRet = API_SdioCoreDevRwDirect(psdcoredev, 0, 0, SDIO_CCCR_IENX, 0, &ucReg);
    if (iRet) {
        goto    __err;
    }

    ucReg &= ~(1 << psdiofunc->FUNC_uiNum);
    if (!(ucReg & 0xfe)) {
        ucReg = 0;
    }
    iRet = API_SdioCoreDevRwDirect(psdcoredev, 1, 0, SDIO_CCCR_IENX, ucReg, LW_NULL);
    if (iRet) {
        goto    __err;
    }

    return  (ERROR_NONE);

__err:
    SDCARD_DEBUG_MSGX(__ERRORMESSAGE_LEVEL, "disable func(%d) interrupt failed.\r\n",
                      psdiofunc->FUNC_uiNum);

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_SdioCoreDevFuncBlkSzSet
** 功能描述: 设置一个功能的传输块大小
** 输    入: psdcoredev       核心设备传输对象
**           psdiofunc        功能描述对象
**           uiBlkSz          块大小
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT API_SdioCoreDevFuncBlkSzSet (PLW_SDCORE_DEVICE   psdcoredev,
                                 SDIO_FUNC          *psdiofunc,
                                 UINT32              uiBlkSz)
{
    INT     iRet;
    UINT32  uiMaxBlkSz;

    if (!psdcoredev || !psdiofunc) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        return  (PX_ERROR);
    }

    /*
     * 这里没有判断 uiBlkSz 是否大于 uiMaxBlkSz
     * 因为实际情况中, 有的设备允许传输块大小大于从 CIS 读取的最大块大小
     */
    uiMaxBlkSz = psdiofunc->FUNC_ulMaxBlkSize;
    if (uiBlkSz == 0) {
        uiBlkSz = min(uiMaxBlkSz, 512u);
    }

    iRet = API_SdioCoreDevRwDirect(psdcoredev, 1, 0,
                                   SDIO_FBR_BASE(psdiofunc->FUNC_uiNum) + SDIO_FBR_BLKSIZE,
                                   uiBlkSz & 0xff, LW_NULL);
    if (iRet != ERROR_NONE) {
        return  (iRet);
    }

    iRet = API_SdioCoreDevRwDirect(psdcoredev, 1, 0,
                                   SDIO_FBR_BASE(psdiofunc->FUNC_uiNum) + SDIO_FBR_BLKSIZE + 1,
                                   (uiBlkSz >> 8) & 0xff, LW_NULL);
    if (iRet != ERROR_NONE) {
        return  (iRet);
    }

    psdiofunc->FUNC_ulCurBlkSize = uiBlkSz;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_SdioCoreDevRwDirect
** 功能描述: SDIO CMD52 直接读写命令函数
** 输    入: psdcoredev       核心设备传输对象
**           bWrite           是否是写操作
**           uiFn             功能号
**           uiAddr           地址
**           ucWrData         要写入的字节
**           pucRdBack        要读回的字节
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT API_SdioCoreDevRwDirect (PLW_SDCORE_DEVICE   psdcoredev,
                             BOOL                bWrite,
                             UINT32              uiFn,
                             UINT32              uiAddr,
                             UINT8               ucWrData,
                             UINT8              *pucRdBack)
{
    LW_SD_COMMAND  sdcmd;
    INT            iError;

    if (!psdcoredev || (uiFn > 7)) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        return  (PX_ERROR);
    }

    if (uiAddr & (~0x1ffff)) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "addr not available.\r\n");
        return  (PX_ERROR);
    }

    lib_bzero(&sdcmd, sizeof(LW_SD_COMMAND));

    sdcmd.SDCMD_uiOpcode = SDIO_RW_DIRECT;
    sdcmd.SDCMD_uiArg    = bWrite ? (1 << 31) : 0;
    sdcmd.SDCMD_uiArg   |= uiFn << 28;
    sdcmd.SDCMD_uiArg   |= (bWrite && pucRdBack) ? (1 << 27) : 0;
    sdcmd.SDCMD_uiArg   |= uiAddr << 9;
    sdcmd.SDCMD_uiArg   |= ucWrData;
    sdcmd.SDCMD_uiFlag   = SD_RSP_SPI_R5 | SD_RSP_R5 | SD_CMD_AC;

    iError = API_SdCoreDevCmd(psdcoredev, &sdcmd, 0);
    if (iError != ERROR_NONE) {
        return  (PX_ERROR);
    }

    if (COREDEV_IS_SPI(psdcoredev)) {

    } else {
        if (sdcmd.SDCMD_uiResp[0] & R5_ERROR) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "unknow error.\r\n");
            return  (PX_ERROR);
        }
        if (sdcmd.SDCMD_uiResp[0] & R5_FUNCTION_NUM) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "function num inval.\r\n");
            return  (PX_ERROR);
        }
        if (sdcmd.SDCMD_uiResp[0] & R5_OUT_OF_RANGE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "arg out of range.\r\n");
            return  (PX_ERROR);
        }
    }

    if (pucRdBack) {
        if (COREDEV_IS_SPI(psdcoredev)) {
            *pucRdBack = (sdcmd.SDCMD_uiResp[0] >> 8) & 0xff;
        } else {
            *pucRdBack = (sdcmd.SDCMD_uiResp[0] >> 0) & 0xff;
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_SdioCoreDevRwExtend
** 功能描述: SDIO CMD53 扩展的数据读写命令(该函数根据块大小和长度自动设置传输模式)
** 输    入: psdcoredev       核心设备传输对象
**           bWrite           是否是写操作
**           uiFn             功能号
**           uiAddr           地址
**           bAddrInc         地址是否自动增长
**           pucBuf           读/写缓冲去
**           uiBlkCnt         块数量
**           uiBlkSz          块大小
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT API_SdioCoreDevRwExtend (PLW_SDCORE_DEVICE   psdcoredev,
                             BOOL                bWrite,
                             UINT32              uiFn,
                             UINT32              uiAddr,
                             BOOL                bAddrInc,
                             UINT8              *pucBuf,
                             UINT32              uiBlkCnt,
                             UINT32              uiBlkSz)
{
    LW_SD_MESSAGE   sdmsg;
    LW_SD_COMMAND   sdcmd;
    LW_SD_DATA      sddat;
    INT             iError;
    INT             iDevSta;

    if (!psdcoredev                           ||
        (uiFn      >  7)                      ||
        ((uiBlkCnt == 1) && (uiBlkSz > 512))  ||
        (uiBlkCnt  == 0)                      ||
        (uiBlkSz   == 0)) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        return  (PX_ERROR);
    }

    if (uiAddr & (~0x1ffff)) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "addr not available.\r\n");
        return  (PX_ERROR);
    }

    iDevSta = API_SdCoreDevStaView(psdcoredev);
    if (iDevSta != SD_DEVSTA_EXIST) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "device is not exist.\r\n");
        return  (PX_ERROR);
    }

    lib_bzero(&sdcmd, sizeof(sdcmd));
    lib_bzero(&sddat, sizeof(sddat));

    sdcmd.SDCMD_uiOpcode = SDIO_RW_EXTENDED;
    sdcmd.SDCMD_uiArg    = bWrite ? (1 << 31) : 0;
    sdcmd.SDCMD_uiArg   |= uiFn << 28;
    sdcmd.SDCMD_uiArg   |= bAddrInc ? (1 << 26) : 0;
    sdcmd.SDCMD_uiArg   |= uiAddr << 9;
    if (uiBlkCnt == 1 && uiBlkSz <= 512) {
        sdcmd.SDCMD_uiArg  |= (uiBlkSz == 512) ? 0 : uiBlkSz;
        sddat.SDDAT_uiFlags = SD_DAT_STREAM;
    } else {
        sdcmd.SDCMD_uiArg |= (1 << 27) | uiBlkCnt;
    }

    sdcmd.SDCMD_uiFlag   = SD_RSP_SPI_R5 | SD_RSP_R5 | SD_CMD_ADTC;

    sddat.SDDAT_uiBlkNum   = uiBlkCnt;
    sddat.SDDAT_uiBlkSize  = uiBlkSz;
    sddat.SDDAT_uiFlags   |= bWrite ? SD_DAT_WRITE : SD_DAT_READ;

    sdmsg.SDMSG_pucWrtBuffer = pucBuf;
    sdmsg.SDMSG_pucRdBuffer  = pucBuf;
    sdmsg.SDMSG_psddata      = &sddat;
    sdmsg.SDMSG_psdcmdCmd    = &sdcmd;
    sdmsg.SDMSG_psdcmdStop   = LW_NULL;

    iError = API_SdCoreDevTransfer(psdcoredev, &sdmsg, 1);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "request error.\r\n");
        return  (PX_ERROR);
    }

    if (COREDEV_IS_SPI(psdcoredev)) {

    } else {
        if (sdcmd.SDCMD_uiResp[0] & R5_ERROR) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "unknow error.\r\n");
            return  (PX_ERROR);
        }
        if (sdcmd.SDCMD_uiResp[0] & R5_FUNCTION_NUM) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "function num inval.\r\n");
            return  (PX_ERROR);
        }
        if (sdcmd.SDCMD_uiResp[0] & R5_OUT_OF_RANGE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "arg out of range.\r\n");
            return  (PX_ERROR);
        }
    }

    return  (ERROR_NONE);
}

/*********************************************************************************************************
** 函数名称: API_SdioCoreDevRwExtendX
** 功能描述: SDIO CMD53 扩展的数据读写命令(由用户指定传输模式)
** 输    入: psdcoredev       核心设备传输对象
**           bWrite           是否是写操作
**           uiFn             功能号
**           bIsBlkMode       是否是块传输模式(区别于字节传输模式)
**           uiAddr           地址
**           bAddrInc         地址是否自动增长
**           pucBuf           读/写缓冲去
**           uiBlkCnt         块数量
**           uiBlkSz          块大小
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT API_SdioCoreDevRwExtendX (PLW_SDCORE_DEVICE   psdcoredev,
                              BOOL                bWrite,
                              UINT32              uiFn,
                              BOOL                bIsBlkMode,
                              UINT32              uiAddr,
                              BOOL                bAddrInc,
                              UINT8              *pucBuf,
                              UINT32              uiBlkCnt,
                              UINT32              uiBlkSz)
{
    LW_SD_MESSAGE   sdmsg;
    LW_SD_COMMAND   sdcmd;
    LW_SD_DATA      sddat;
    INT             iError;
    INT             iDevSta;

    if (!psdcoredev                          ||
        (uiFn      >  7)                     ||
        ((uiBlkCnt == 1) && (uiBlkSz > 512)) ||
        (uiBlkCnt  == 0)                     ||
        (uiBlkSz   == 0)) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        return  (PX_ERROR);
    }

    if (uiAddr & (~0x1ffff)) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "addr not available.\r\n");
        return  (PX_ERROR);
    }

    iDevSta = API_SdCoreDevStaView(psdcoredev);
    if (iDevSta != SD_DEVSTA_EXIST) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "device is not exist.\r\n");
        return  (PX_ERROR);
    }

    lib_bzero(&sdcmd, sizeof(sdcmd));
    lib_bzero(&sddat, sizeof(sddat));

    sdcmd.SDCMD_uiOpcode = SDIO_RW_EXTENDED;
    sdcmd.SDCMD_uiArg    = bWrite ? (1 << 31) : 0;
    sdcmd.SDCMD_uiArg   |= uiFn << 28;
    sdcmd.SDCMD_uiArg   |= bAddrInc ? (1 << 26) : 0;
    sdcmd.SDCMD_uiArg   |= uiAddr << 9;
    if (!bIsBlkMode) {
        sdcmd.SDCMD_uiArg |= (uiBlkSz == 512) ? 0 : uiBlkSz;
        sddat.SDDAT_uiFlags = SD_DAT_STREAM;
    } else {
        sdcmd.SDCMD_uiArg |= (1 << 27) | uiBlkCnt;
    }

    sdcmd.SDCMD_uiFlag   = SD_RSP_SPI_R5 | SD_RSP_R5 | SD_CMD_ADTC;

    sddat.SDDAT_uiBlkNum   = uiBlkCnt;
    sddat.SDDAT_uiBlkSize  = uiBlkSz;
    sddat.SDDAT_uiFlags   |= bWrite ? SD_DAT_WRITE : SD_DAT_READ;

    sdmsg.SDMSG_pucWrtBuffer = pucBuf;
    sdmsg.SDMSG_pucRdBuffer  = pucBuf;
    sdmsg.SDMSG_psddata      = &sddat;
    sdmsg.SDMSG_psdcmdCmd    = &sdcmd;
    sdmsg.SDMSG_psdcmdStop   = LW_NULL;

    iError = API_SdCoreDevTransfer(psdcoredev, &sdmsg, 1);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "request error.\r\n");
        return  (PX_ERROR);
    }

    if (COREDEV_IS_SPI(psdcoredev)) {

    } else {
        if (sdcmd.SDCMD_uiResp[0] & R5_ERROR) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "unknow error.\r\n");
            return  (PX_ERROR);
        }
        if (sdcmd.SDCMD_uiResp[0] & R5_FUNCTION_NUM) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "function num inval.\r\n");
            return  (PX_ERROR);
        }
        if (sdcmd.SDCMD_uiResp[0] & R5_OUT_OF_RANGE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "arg out of range.\r\n");
            return  (PX_ERROR);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_SdioCoreDevReadCis
** 功能描述: 读取并尝试解析功能的 CIS 信息
** 输    入: psdcoredev       核心设备传输对象
**           psdiofunc        SDIO 功能描述对象
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT API_SdioCoreDevReadCis (PLW_SDCORE_DEVICE   psdcoredev, SDIO_FUNC *psdiofunc)
{
    INT                iRet;
    SDIO_FUNC_TUPLE   *psdiofunctplThis;
    SDIO_FUNC_TUPLE  **ppsdiofunctplPrev;
    UINT32             uiCisPtr = 0;
    UINT32             i;

    /*
     * Note that this works for the common CIS (function number 0) as
     * well as a function's CIS * since SDIO_CCCR_CIS and SDIO_FBR_CIS
     * have the same offset.
     */
    if (!psdcoredev || !psdiofunc) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        return  (PX_ERROR);
    }

    for (i = 0; i < 3; i++) {
        UINT8 ucByte;

        iRet = API_SdioCoreDevRwDirect(psdcoredev, 0, 0,
                                       SDIO_FBR_BASE(psdiofunc->FUNC_uiNum) + SDIO_FBR_CIS + i,
                                       0, &ucByte);
        if (iRet) {
            return  (iRet);
        }

        uiCisPtr |= ucByte << (i * 8);
    }

    ppsdiofunctplPrev = &psdiofunc->FUNC_ptupleListHeader;
    if (*ppsdiofunctplPrev) {
        SDCARD_DEBUG_MSG(__LOGMESSAGE_LEVEL, " warning: tuple header init not-null.\r\n");
    }

    do {
        UINT8  ucTplCode;
        UINT8  ucTplLink;

        iRet = API_SdioCoreDevRwDirect(psdcoredev, 0, 0, uiCisPtr++, 0, &ucTplCode);
        if (iRet) {
            break;
        }

        if (ucTplCode == SDIO_CIS_TPL_END) {
            break;
        }

        if (ucTplCode == SDIO_CIS_TPL_NULL) {
            continue;
        }

        iRet = API_SdioCoreDevRwDirect(psdcoredev, 0, 0, uiCisPtr++, 0, &ucTplLink);
        if (iRet) {
            break;
        }

        if (ucTplLink == SDIO_CIS_TPL_END) {
            break;
        }

        psdiofunctplThis = (SDIO_FUNC_TUPLE *)__SHEAP_ALLOC(sizeof(*psdiofunctplThis) + ucTplLink - 1);
        if (!psdiofunctplThis) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            return  (-ENOMEM);
        }

        for (i = 0; i < ucTplLink; i++) {
            iRet = API_SdioCoreDevRwDirect(psdcoredev, 0, 0,
                                           uiCisPtr + i, 0,
                                           &psdiofunctplThis->TUPLE_pucData[i]);
            if (iRet) {
                break;
            }
        }

        if (iRet) {
            __SHEAP_FREE(psdiofunctplThis);
            break;
        }

        /*
         * Try to parse the CIS tuple
         */
        iRet = __cistplParse(psdiofunc,
                             _G_cistplparserTbl,
                             __NELE(_G_cistplparserTbl),
                             ucTplCode,
                             psdiofunctplThis->TUPLE_pucData,
                             ucTplLink);

        if (iRet == -EILSEQ || iRet == -ENOENT) {
            /*
             * The tuple is unknown or known but not parsed.
             * Queue the tuple for the function driver.
             */
            psdiofunctplThis->TUPLE_ptupleNext  = LW_NULL;
            psdiofunctplThis->TUPLE_ucCode      = ucTplCode;
            psdiofunctplThis->TUPLE_ucSize      = ucTplLink;

            *ppsdiofunctplPrev = psdiofunctplThis;
            ppsdiofunctplPrev  = &psdiofunctplThis->TUPLE_ptupleNext;

            if (iRet == -ENOENT) {
                /*
                 * always warning...
                 */
            }

            /*
             * keep on analyzing tuples
             */
            iRet = 0;

        } else {
            /*
             * We don't need the tuple anymore if it was
             * successfully parsed by the SDIO core or if it is
             * not going to be queued for a driver.
             */
            __SHEAP_FREE(psdiofunctplThis);
        }

        uiCisPtr += ucTplLink;

    } while (!iRet);

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_SdioCoreDevReadFbr
** 功能描述: 读取并解析 FBR
** 输    入: psdcoredev       核心设备传输对象
**           psdiofunc        SDIO 功能描述对象
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT API_SdioCoreDevReadFbr (PLW_SDCORE_DEVICE  psdcoredev, SDIO_FUNC *psdiofunc)
{
    INT    iRet;
    UINT8  ucData;

    iRet = API_SdioCoreDevRwDirect(psdcoredev, 0, 0,
                                   SDIO_FBR_BASE(psdiofunc->FUNC_uiNum) + SDIO_FBR_STD_IF,
                                   0, &ucData);
    if (iRet != ERROR_NONE) {
        goto    __err;
    }

    ucData &= 0x0f;
    if (ucData == 0x0f) {
        iRet = API_SdioCoreDevRwDirect(psdcoredev, 0, 0,
                                       SDIO_FBR_BASE(psdiofunc->FUNC_uiNum) + SDIO_FBR_STD_IF_EXT,
                                       0, &ucData);
        if (iRet != ERROR_NONE) {
            goto    __err;
        }
    }

    psdiofunc->FUNC_ucClass = ucData;

__err:
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_SdioCoreDevReadCCCR
** 功能描述: 读取并解析 CCCR
** 输    入: psdcoredev       核心设备传输对象
**           psdiocccr        保存解析的 cccr 信息
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT API_SdioCoreDevReadCCCR (PLW_SDCORE_DEVICE   psdcoredev, SDIO_CCCR *psdiocccr)
{
    INT     iRet;
    INT     iCccrVsn;
    UINT8   ucData;

    lib_bzero(psdiocccr, sizeof(SDIO_CCCR));

    iRet = API_SdioCoreDevRwDirect(psdcoredev, 0, 0, SDIO_CCCR_CCCR, 0, &ucData);
    if (iRet != ERROR_NONE) {
        goto    __err;
    }

    iCccrVsn = ucData & 0x0f;

    if (iCccrVsn > SDIO_CCCR_REV_1_20) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "unrecognised CCCR vsn.\r\n");
        return  (PX_ERROR);
    }
    psdiocccr->CCCR_uiSdioVsn = (ucData & 0xf0) >> 4;

    iRet = API_SdioCoreDevRwDirect(psdcoredev, 0, 0, SDIO_CCCR_SD, 0, &ucData);
    if (iRet != ERROR_NONE) {
        goto    __err;
    }
    psdiocccr->CCCR_uiSdVsn = ucData & 0x0f;

    iRet = API_SdioCoreDevRwDirect(psdcoredev, 0, 0, SDIO_CCCR_CAPS, 0, &ucData);
    if (iRet != ERROR_NONE) {
        goto    __err;
    }

    if (ucData & SDIO_CCCR_CAP_SMB) {
        psdiocccr->CCCR_bMulBlk = LW_TRUE;
    }
    if (ucData & SDIO_CCCR_CAP_LSC) {
        psdiocccr->CCCR_bLowSpeed = LW_TRUE;
    }
    if (ucData & SDIO_CCCR_CAP_4BLS) {
        psdiocccr->CCCR_bWideBus = LW_TRUE;
    }

    if (iCccrVsn >= SDIO_CCCR_REV_1_10) {
        iRet = API_SdioCoreDevRwDirect(psdcoredev, 0, 0, SDIO_CCCR_POWER, 0, &ucData);
        if (iRet) {
            goto    __err;
        }

        if (ucData & SDIO_POWER_SMPC) {
            psdiocccr->CCCR_bHighPwr = LW_TRUE;
        }
    }

    if (iCccrVsn >= SDIO_CCCR_REV_1_20) {
        iRet = API_SdioCoreDevRwDirect(psdcoredev, 0, 0, SDIO_CCCR_SPEED, 0, &ucData);
        if (iRet) {
            goto    __err;
        }
        if (ucData & SDIO_SPEED_SHS) {
            psdiocccr->CCCR_bHighSpeed = LW_TRUE;
        }
    }

__err:
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_SdioCoreDevFuncClean
** 功能描述: 在 __sdioCoreDevReadCis() 过程中会有内存分配. 使用该函数进行内存清理
** 输    入: psdiofunc        功能描述对象
** 输    出: NONE
** 全局变量: ERROR CODE
** 调用模块:
*********************************************************************************************************/
INT  API_SdioCoreDevFuncClean (SDIO_FUNC *psdiofunc)
{
    SDIO_FUNC_TUPLE *psdiofunctpl;
    SDIO_FUNC_TUPLE *psdiofunctplNext;

    if (!psdiofunc) {
        return  (PX_ERROR);
    }

    psdiofunctpl = psdiofunc->FUNC_ptupleListHeader;
    while (psdiofunctpl) {
        psdiofunctplNext = psdiofunctpl->TUPLE_ptupleNext;
        __SHEAP_FREE(psdiofunctpl);

        psdiofunctpl = psdiofunctplNext;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_SdioCoreDevHighSpeedEn
** 功能描述: 使能 SDIO 高速模式
** 输    入: psdcoredev       核心设备传输对象
**           psdiocccr        已经解析的 CCCR 信息.(函数会参考该信息进行实际的高速使能工作)
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT API_SdioCoreDevHighSpeedEn (PLW_SDCORE_DEVICE   psdcoredev, SDIO_CCCR *psdiocccr)
{
    INT     iHostCap;
    INT     iRet;
    UINT8   ucSpeed;

    iRet = API_SdmHostCapGet(psdcoredev, &iHostCap);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    if (!(iHostCap & SDHOST_CAP_HIGHSPEED)) {
        return  (ERROR_NONE);
    }

    if (!psdiocccr->CCCR_bHighSpeed) {
        return  (ERROR_NONE);
    }

    iRet = API_SdioCoreDevRwDirect(psdcoredev, 0, 0, SDIO_CCCR_SPEED, 0, &ucSpeed);
    if (iRet != ERROR_NONE) {
        return  (iRet);
    }

    ucSpeed |= SDIO_SPEED_EHS;
    iRet = API_SdioCoreDevRwDirect(psdcoredev, 1, 0, SDIO_CCCR_SPEED, ucSpeed, LW_NULL);
    if (iRet) {
        return  (iRet);
    }

    COREDEV_HIGHSPEED_SET(psdcoredev);

    /*
     * TODO: host set timeout value
     */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_SdioCoreDevWideBusEn
** 功能描述: 使能 SDIO 宽总线模式
** 输    入: psdcoredev       核心设备传输对象
**           psdiocccr        已经解析的 CCCR 信息.(函数会参考该信息进行实际的宽总线设置工作)
** 输    出:
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT API_SdioCoreDevWideBusEn (PLW_SDCORE_DEVICE   psdcoredev, SDIO_CCCR *psdiocccr)
{
    INT     iHostCap;
    INT     iRet;
    UINT8   ucWidth;

    iRet = API_SdmHostCapGet(psdcoredev, &iHostCap);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    if (!(iHostCap & SDHOST_CAP_DATA_4BIT)) {
        return  (ERROR_NONE);
    }

    if (iHostCap & SDHOST_CAP_SDIO_FORCE_1BIT) {
        return  (ERROR_NONE);
    }

    if (psdiocccr->CCCR_bLowSpeed && !psdiocccr->CCCR_bWideBus) {
        return  (ERROR_NONE);
    }

    iRet = API_SdioCoreDevRwDirect(psdcoredev, 0, 0, SDIO_CCCR_IF, 0, &ucWidth);
    if (iRet != ERROR_NONE) {
        return  (iRet);
    }

    ucWidth &= ~SDIO_BUS_WIDTH_MASK;
    ucWidth |= SDIO_BUS_WIDTH_4BIT;
    iRet = API_SdioCoreDevRwDirect(psdcoredev, 1, 0, SDIO_CCCR_IF, ucWidth, LW_NULL);
    if (iRet != ERROR_NONE) {
        return  (iRet);
    }

    iRet = API_SdCoreDevCtl(psdcoredev,
                            SDBUS_CTRL_SETBUSWIDTH,
                            SDARG_SETBUSWIDTH_4);
    if (iRet != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__LOGMESSAGE_LEVEL, " warning: dev widebus en,but host set not succ.\r\n");
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __cistplParse
** 功能描述: 解析一个功能的 CIS Tuple
** 输    入: psdiofunc        SDIO 功能描述对象
**           cpParser         具体的解析器描述对象表
**           iParserCnt       解析器的个数
**           uiCode           当前需要解析的 Tuple 的代码
**           cpucData         当前需要解析的 Tuple 的数据
**           uiSize           当前需要解析的 Tuple 的数据长度
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __cistplParse (SDIO_FUNC                 *psdiofunc,
                          const __CIS_TPL_PARSER    *cpParser,
                          INT                        iParserCnt,
                          UINT8                      ucCode,
                          const UINT8               *cpucData,
                          UINT32                     uiSize)
{
    INT  iRet;
    INT  i;

    for (i = 0; i < iParserCnt; i++, cpParser++) {
        if (cpParser->PARSER_ucCode == ucCode) {
            break;
        }
    }

    if (i < iParserCnt) {
        if (uiSize >= cpParser->PARSER_ucMinSz) {
            if (cpParser->PARSER_pfuncParse) {
                iRet = cpParser->PARSER_pfuncParse(psdiofunc, cpucData, uiSize);
            } else {
                iRet = -EILSEQ;                                     /*  known tuple, not parsed         */
            }
        } else {
            iRet = -EINVAL;                                         /*  invalid tuple                   */
        }

        if (iRet && (iRet != -EILSEQ) && (iRet != -ENOENT)) {
            SDCARD_DEBUG_MSGX(__LOGMESSAGE_LEVEL,
                              "warning: parser[%s] "
                              "with bad tuple code[0x%02x] size[%u].\r\n",
                              cpParser->PARSER_cpcDesc, ucCode, uiSize);
        }

    } else {
        iRet = -ENOENT;                                             /*  unknown tuple                   */
    }

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __cistplParseFunceCommon
** 功能描述: 解析功能0 的CIS
** 输    入: psdiofunc        功能0描述对象
**           cpucData         对应 Tuple 数据
**           uiSize           对应 Tuple 数据长度
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __cistplParseFunceCommon (SDIO_FUNC *psdiofunc, const UINT8 *cpucData, UINT32 uiSize)
{
    if (!psdiofunc || psdiofunc->FUNC_uiNum != 0) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "just func0 available.\r\n");
        return  (-EINVAL);
    }

    psdiofunc->FUNC_ulMaxBlkSize = cpucData[1] | (cpucData[2] << 8);
    psdiofunc->FUNC_uiMaxDtr     = _G_pucSpeedVal[(cpucData[3] >> 3) & 15]
                                 * _G_puiSpeedUnit[cpucData[3] & 7];

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __cistplParseFunceFuncN
** 功能描述: 解析功1~7 的CIS
** 输    入: psdiofunc        功能描述对象
**           cpucData         对应 Tuple 数据
**           uiSize           对应 Tuple 数据长度
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __cistplParseFunceFuncN (SDIO_FUNC *psdiofunc, const UINT8 *cpucData, UINT32 uiSize)
{
    UINT32  uiVsn;
    UINT32  uiMinSz;

    if (!psdiofunc || psdiofunc->FUNC_uiNum == 0) {
        return  (-EINVAL);
    }

    /*
     * This tuple has a different length depending on
     * the SDIO spec version.
     */
    uiVsn   = psdiofunc->FUNC_cpsdiocccr->CCCR_uiSdioVsn;
    uiMinSz = (uiVsn == SDIO_SDIO_REV_1_00) ? 28 : 42;

    if (uiSize < uiMinSz) {
        return  (-EINVAL);
    }

    psdiofunc->FUNC_ulMaxBlkSize = cpucData[12] | (cpucData[13] << 8);

    if (uiVsn > SDIO_SDIO_REV_1_00) {
        psdiofunc->FUNC_uiEnableTimeout = (cpucData[28] | (cpucData[29] << 8)) * 10;
    } else {
        psdiofunc->FUNC_uiEnableTimeout = 1000;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __cistplParseFunce
** 功能描述: 解析功0~7 的CIS总函数
** 输    入: psdiofunc        功能描述对象
**           cpucData         对应 Tuple 数据
**           uiSize           对应 Tuple 数据长度
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __cistplParseFunce (SDIO_FUNC *psdiofunc, const UINT8 *cpucData, UINT32 uiSize)
{
    INT   iRet;

    if (uiSize < 1) {
        SDCARD_DEBUG_MSGX(__ERRORMESSAGE_LEVEL, "data size(%u) not available.\r\n", uiSize);
        return  (-EINVAL);
    }

    iRet = __cistplParse(psdiofunc,
                         _G_cistplparserFunceTbl,
                         __NELE(_G_cistplparserFunceTbl),
                         cpucData[0],
                         cpucData,
                         uiSize);

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __cistplParseManfid
** 功能描述: 解析 厂商 ID 的方法
** 输    入: psdiofunc        功能描述对象
**           cpucData         对应 Tuple 数据
**           uiSize           对应 Tuple 数据长度
** 输    出:
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __cistplParseManfid (SDIO_FUNC *psdiofunc, const UINT8 *cpucData, UINT32 uiSize)
{
    UINT16  usVendor;
    UINT16  usDevice;

    usVendor = cpucData[0] | (cpucData[1] << 8);
    usDevice = cpucData[2] | (cpucData[3] << 8);

    if (psdiofunc) {
        psdiofunc->FUNC_usVendor = usVendor;
        psdiofunc->FUNC_usDevice = usDevice;
    } else {
        return  (-EINVAL);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __cistplParseVers_1
** 功能描述: 解析 SDIO 版本
** 输    入: psdiofunc        功能描述对象
**           cpucData         对应 Tuple 数据
**           uiSize           对应 Tuple 数据长度
** 输    出:
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __cistplParseVers_1 (SDIO_FUNC *psdiofunc, const UINT8 *cpucData, UINT32 uiSize)
{
    return  (-ENOENT);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_SDCARD_EN > 0)      */
                                                                        /*  (LW_CFG_SDCARD_SDIO_EN > 0) */
/*********************************************************************************************************
  END
*********************************************************************************************************/
