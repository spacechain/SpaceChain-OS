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
** 文   件   名: ataLib.c
**
** 创   建   人: Hou.XiaoLong (侯小龙)
**
** 文件创建日期: 2010 年 02 月 01 日
**
** 描        述: ATA设备库

** BUG:
2010.03.29  __ataWait()函数使用新的延时等待机制.
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
static INT  _G_iAtaRetry    = __ATA_RETRY_TIMES;
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
INT        __ataCmd(__PATA_CTRL patactrler,
                   INT         iDrive,
                   INT         iCmd,
                   INT         iArg0,
                   INT         iArg1);
INT        __ataDevIdentify(__PATA_CTRL patactrler, INT iDrive);
INT        __ataWait(__PATA_CTRL patactrler, INT iRequest);
static INT __ataPread(__PATA_CTRL patactrler,
                      INT         iDrive,
                      PVOID       pvBuffer);
/*********************************************************************************************************
** 函数名称: __ataIdString
** 功能描述: 将 ID 值转换为字符串信息
** 输　入  : pusId      ID 参数
**           pcBuff     字符串缓存
**           uiLen      缓存大小
** 输　出  : 字符串信息
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PCHAR  __ataIdString (const UINT16 *pusId, PCHAR  pcBuff, UINT32  uiLen)
{
    REGISTER INT    i;
    UINT8           ucChar   = 0;                                       /* 字节数据                     */
    INT             iOffset  = 0;                                       /* 字偏移                       */
    PCHAR           pcString = pcBuff;                                  /* 字符串缓冲区                 */

    /*
     *  将字数据转换为字符串数据
     */
    iOffset   = 0;
    pcString = pcBuff;
    for (i = 0; i < (uiLen - 1); ) {
        ucChar = (UINT8)(pusId[iOffset] >> 8);
        *pcString = ucChar;
        pcString += 1;

        ucChar = (UINT8)(pusId[iOffset] & 0xff);
        *pcString = ucChar;
        pcString += 1;

        iOffset += 1;
        i       += 2;
    }

    /*
     *  删除无效空格信息
     */
    pcString = pcBuff + lib_strnlen(pcBuff, uiLen - 1);
    while ((pcString > pcBuff) && (pcString[-1] == ' ')) {
        pcString--;
    }
    *pcString = '\0';

    iOffset   = 0;
    pcString = pcBuff;
    for (i = 0; i < uiLen; i++) {
        if (pcString[iOffset] == ' ') {
            pcString += 1;
        } else {
            break;
        }
    }

    return  (pcString);
}
/*********************************************************************************************************
** 函数名称: __ataDriveSerialInfoGet
** 功能描述: 序列号信息
** 输　入  : hDrive     驱动器句柄
**           pcBuf      缓冲区
**           stLen      缓冲区大小
** 输　出  : 字符串信息
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PCHAR  __ataDriveSerialInfoGet (__PATA_DRIVE  hDrive, PCHAR  pcBuf, size_t  stLen)
{
    __ATA_PARAM    *hParam;                                             /* 参数句柄                     */
    PCHAR           pcSerial;                                           /* 设备串号                     */
    CHAR            cSerial[21] = { 0 };                                /* 序列号缓冲区                 */

    if ((!hDrive) || (!pcBuf)) {
        return  (LW_NULL);
    }

    hParam = &hDrive->ATADRIVE_ataParam;
    lib_bzero(&cSerial[0], 21);
    pcSerial = __ataIdString((const UINT16 *)&hParam->ATAPAR_cSerial[0], &cSerial[0], 21);
    lib_strncpy(pcBuf, pcSerial, __MIN(stLen, lib_strlen(pcSerial) + 1));

    return  (pcSerial);
}
/*********************************************************************************************************
** 函数名称: __ataDriveFwRevInfoGet
** 功能描述: 固件版本号信息
** 输　入  : hDrive     驱动器句柄
**           pcBuf      缓冲区
**           stLen      缓冲区大小
** 输　出  : 字符串信息
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PCHAR  __ataDriveFwRevInfoGet (__PATA_DRIVE  hDrive, PCHAR  pcBuf, size_t  stLen)
{
    __ATA_PARAM    *hParam;                                             /* 参数句柄                     */
    PCHAR           pcFirmware;                                         /* 固件版本                     */
    CHAR            cFirmware[9] = { 0 };                               /* 固件版本缓冲区               */

    if ((!hDrive) || (!pcBuf)) {
        return  (LW_NULL);
    }

    hParam = &hDrive->ATADRIVE_ataParam;
    lib_bzero(&cFirmware[0], 9);
    pcFirmware =__ataIdString((const UINT16 *)&hParam->ATAPAR_cRev[0], &cFirmware[0], 9);
    lib_strncpy(pcBuf, pcFirmware, __MIN(stLen, lib_strlen(pcFirmware) + 1));

    return  (pcFirmware);
}
/*********************************************************************************************************
** 函数名称: __ataDriveModelInfoGet
** 功能描述: 设备详细信息
** 输　入  : hDrive     驱动器句柄
**           pcBuf      缓冲区
**           stLen      缓冲区大小
** 输　出  : 字符串信息
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PCHAR  __ataDriveModelInfoGet (__PATA_DRIVE  hDrive, PCHAR  pcBuf, size_t  stLen)
{
    __ATA_PARAM    *hParam;                                             /* 参数句柄                     */
    PCHAR           pcProduct;                                          /* 产品信息                     */
    CHAR            cProduct[41] = { 0 };                               /* 产品信息缓冲区               */

    if ((!hDrive) || (!pcBuf)) {
        return  (LW_NULL);
    }

    hParam = &hDrive->ATADRIVE_ataParam;
    lib_bzero(&cProduct[0], 41);
    pcProduct = __ataIdString((const UINT16 *)&hParam->ATAPAR_cModel[0], &cProduct[0], 41);
    lib_strncpy(pcBuf, pcProduct, __MIN(stLen, lib_strlen(pcProduct) + 1));

    return  (pcProduct);
}
/*********************************************************************************************************
** 函数名称: __ataCmd
** 功能描述: ATA命令
** 输　入  : patactrler  ATA控制器结构指针
**           iDrive      驱动号
**           iCmd        ATA命令
**           iArg0       参数0
**           iArg1       参数1
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT __ataCmd (__PATA_CTRL patactrler,
              INT         iDrive,
              INT         iCmd,
              INT         iArg0,
              INT         iArg1)
{
    __PATA_CTRL   patactrl    = LW_NULL;
    __PATA_DRIVE  patadrive   = LW_NULL;
    __PATA_TYPE   patatype    = LW_NULL;

    INT           iRetry      = 1;
    INT           iRetryCount = 0;
    ULONG         ulSemStatus = 0;

    if (!patactrler) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    patactrl    = patactrler;
    patadrive   = &patactrl->ATACTRL_ataDrive[iDrive];
    patatype    = patadrive->ATADRIVE_patatypeDriverInfo;

    while (iRetry) {
        if (__ataWait(patactrler, __ATA_STAT_READY)) {                  /*  超时,错误返回               */
            ATA_DEBUG_MSG(("__ataCmd() error : time out"));
            return  (PX_ERROR);
        }

        switch (iCmd) {

        case __ATA_CMD_DIAGNOSE:                                        /*  运行驱动器诊断              */
        case __ATA_CMD_RECALIB:                                         /*  校准                        */
            __ATA_CTRL_OUTBYTE(patactrl, __ATA_SDH(patactrl),
                               (UINT8)(__ATA_SDH_LBA | (iDrive << __ATA_DRIVE_BIT)));
            break;

        case __ATA_CMD_INITP:                                           /*  驱动器参数初始化            */
            __ATA_CTRL_OUTBYTE(patactrl, __ATA_CYLLO(patactrl), (UINT8)patatype->ATATYPE_iCylinders);
            __ATA_CTRL_OUTBYTE(patactrl, __ATA_CYLHI(patactrl),  \
                               (UINT8)(patatype->ATATYPE_iCylinders >> 8));
            __ATA_CTRL_OUTBYTE(patactrl, __ATA_SECCNT(patactrl), (UINT8)patatype->ATATYPE_iSectors);
            __ATA_CTRL_OUTBYTE(patactrl, __ATA_SDH(patactrl),    \
                               (UINT8)(__ATA_SDH_LBA         |   \
                               (iDrive << __ATA_DRIVE_BIT)   |   \
                               ((patatype->ATATYPE_iHeads - 1) & 0x0f)));
            break;

        case __ATA_CMD_SEEK:                                            /*  查询定位                    */
            __ATA_CTRL_OUTBYTE(patactrl, __ATA_CYLLO(patactrl), (UINT8)iArg0);
            __ATA_CTRL_OUTBYTE(patactrl, __ATA_CYLHI(patactrl), (UINT8)(iArg0 >> 8));
            __ATA_CTRL_OUTBYTE(patactrl, __ATA_SDH(patactrl),   \
                               (UINT8)(__ATA_SDH_LBA         |  \
                               (iDrive << __ATA_DRIVE_BIT)   |  \
                               (iArg1 & 0xf)));
            break;

        case __ATA_CMD_SET_FEATURE:                                     /*  设置特征                    */
            __ATA_CTRL_OUTBYTE(patactrl, __ATA_SECCNT(patactrl), (UINT8)iArg1);
            __ATA_CTRL_OUTBYTE(patactrl, __ATA_FEATURE(patactrl), (UINT8)iArg0);
            __ATA_CTRL_OUTBYTE(patactrl, __ATA_SDH(patactrl),   \
                               (UINT8)(__ATA_SDH_LBA | (iDrive << __ATA_DRIVE_BIT)));
            break;

        case __ATA_CMD_SET_MULTI:                                       /*  多重模式设定                */
            __ATA_CTRL_OUTBYTE(patactrl, __ATA_SECCNT(patactrl), (UINT8)iArg0);
            __ATA_CTRL_OUTBYTE(patactrl, __ATA_SDH(patactrl),   \
                               (UINT8)(__ATA_SDH_LBA | (iDrive << __ATA_DRIVE_BIT)));
            break;

        default:
            __ATA_CTRL_OUTBYTE(patactrl, __ATA_SDH(patactrl),   \
                               (UINT8)(__ATA_SDH_LBA | (iDrive << __ATA_DRIVE_BIT)));
            break;
        }

        __ATA_CTRL_OUTBYTE(patactrl, __ATA_COMMAND(patactrl), (UINT8)iCmd);
                                                                        /*  写命令寄存器                */
        if (patactrl->ATACTRL_bIntDisable == LW_FALSE) {
            ulSemStatus = API_SemaphoreBPend(patactrl->ATACTRL_ulSyncSem,  \
                                patactrl->ATACTRL_ulSyncSemTimeout);    /*  等待同步信号                */
        }

        if ((patactrl->ATACTRL_iIntStatus & __ATA_STAT_ERR) || (ulSemStatus != ERROR_NONE)) {
            ATA_DEBUG_MSG(("__ataCmd() error : status=0x%x ulSemStatus=%d err=0x%x\n",
                           patactrl->ATACTRL_iIntStatus, ulSemStatus,
                           __ATA_CTRL_INBYTE(patactrl, __ATA_ERROR(patactrl))));
            if (++iRetryCount > _G_iAtaRetry) {                         /*  重试大于默认次数,错误返回   */
                return  (PX_ERROR);
            }
        } else {
            iRetry = 0;
        }
    }

    if (iCmd == __ATA_CMD_SEEK) {
        if (__ataWait(patactrler, __ATA_STAT_SEEKCMPLT) != ERROR_NONE) {
            return  (PX_ERROR);
        }
    }

    ATA_DEBUG_MSG(("__ataCmd() end : - iCtrl %d, iDrive %d: Ok\n", patactrl->ATACTRL_iCtrl, iDrive));

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ataWait
** 功能描述: 等待ATA设备准备好
** 输　入  : patactrler  ATA控制器结构指针
**           iRequest    等待状态
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT __ataWait (__PATA_CTRL patactrler, INT iRequest)
{
    __PATA_CTRL       patactrl = LW_NULL;
    struct timespec   tvOld;
    struct timespec   tvNow;

    volatile INT  i;

    if (!patactrler) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    patactrl = patactrler;
    lib_clock_gettime(CLOCK_MONOTONIC, &tvOld);

    switch (iRequest) {

    case __ATA_STAT_READY:
        for (i = 0; i < __ATA_TIMEOUT_LOOP; i++) {
            if ((__ATA_CTRL_INBYTE(patactrl, __ATA_ASTATUS(patactrl)) & __ATA_STAT_BUSY) == 0) {
                                                                        /*  等待设备不忙                */
                break;
            }

            lib_clock_gettime(CLOCK_MONOTONIC, &tvNow);
            if ((tvNow.tv_sec - tvOld.tv_sec) >= __ATA_TIMEOUT_SEC) {   /*  超时退出                    */
                break;
            }
        }

        for (i = 0; i < __ATA_TIMEOUT_LOOP; i++) {
            if (__ATA_CTRL_INBYTE(patactrl, __ATA_ASTATUS(patactrl)) & __ATA_STAT_READY) {
                                                                        /*  设备准备好                  */
                return  (0);
            }

            lib_clock_gettime(CLOCK_MONOTONIC, &tvNow);
            if ((tvNow.tv_sec - tvOld.tv_sec) >= __ATA_TIMEOUT_SEC) {   /*  超时退出                    */
                break;
            }
        }
        break;

    case __ATA_STAT_BUSY:
        for (i = 0; i < __ATA_TIMEOUT_LOOP; i++) {
            if ((__ATA_CTRL_INBYTE(patactrl, __ATA_ASTATUS(patactrl)) & __ATA_STAT_BUSY) == 0) {
                                                                        /*  等待设备不忙                */
                return  (ERROR_NONE);
            }

            lib_clock_gettime(CLOCK_MONOTONIC, &tvNow);
            if ((tvNow.tv_sec - tvOld.tv_sec) >= __ATA_TIMEOUT_SEC) {   /*  超时退出                    */
                break;
            }
        }
        break;

    case __ATA_STAT_DRQ:
        for (i = 0; i < __ATA_TIMEOUT_LOOP; i++) {
            if (__ATA_CTRL_INBYTE(patactrl, __ATA_ASTATUS(patactrl)) & __ATA_STAT_DRQ) {
                                                                        /*  设备准备好传输数据          */
                return  (ERROR_NONE);
            }

            lib_clock_gettime(CLOCK_MONOTONIC, &tvNow);
            if ((tvNow.tv_sec - tvOld.tv_sec) >= __ATA_TIMEOUT_SEC) {   /*  超时退出                    */
                break;
            }
        }
        break;

    case __ATA_STAT_SEEKCMPLT:
        for (i = 0; i < __ATA_TIMEOUT_LOOP; i++) {
            if (__ATA_CTRL_INBYTE(patactrl, __ATA_ASTATUS(patactrl)) & __ATA_STAT_SEEKCMPLT) {
                                                                        /*  设备就绪                    */
                return  (ERROR_NONE);
            }

            lib_clock_gettime(CLOCK_MONOTONIC, &tvNow);
            if ((tvNow.tv_sec - tvOld.tv_sec) >= __ATA_TIMEOUT_SEC) {   /*  超时退出                    */
                break;
            }
        }
        break;

    case __ATA_STAT_IDLE:
        for (i = 0; i < __ATA_TIMEOUT_LOOP; i++) {
            if ((__ATA_CTRL_INBYTE(patactrl, __ATA_ASTATUS(patactrl)) &  \
                (__ATA_STAT_BUSY | __ATA_STAT_DRQ)) == 0) {             /*  设备空闲                    */
                return  (ERROR_NONE);
            }

            lib_clock_gettime(CLOCK_MONOTONIC, &tvNow);
            if ((tvNow.tv_sec - tvOld.tv_sec) >= __ATA_TIMEOUT_SEC) {   /*  超时退出                    */
                break;
            }
        }
        break;

    default:
        break;
    }

    return  (PX_ERROR);                                                 /*  错误返回                    */
}
/*********************************************************************************************************
** 函数名称: __ataPread
** 功能描述: 读取设备的参数
** 输　入  : patactrler  ATA控制器结构指针
**                       iDrive    驱动号
**                       pvBuffer  缓冲
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __ataPread (__PATA_CTRL patactrler,
                       INT         iDrive,
                       PVOID       pvBuffer)
{
    __PATA_CTRL patactrl = LW_NULL;

    INT         iRetry      = 1;
    INT         iRetryCount = 0;
    ULONG       ulSemStatus = 0;

#if LW_CFG_CPU_ENDIAN == 1
    INT         i;
#endif

    INT16      *psBuf = LW_NULL;

    if ((!patactrler) || (!pvBuffer)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    patactrl = patactrler;

    while (iRetry) {
        if (__ataWait(patactrl, __ATA_STAT_READY) != ERROR_NONE) {
            return  (PX_ERROR);
        }

        __ATA_CTRL_OUTBYTE(patactrl, __ATA_SDH(patactrl),   \
                           (UINT8)(__ATA_SDH_LBA | (iDrive << __ATA_DRIVE_BIT)));
        __ATA_CTRL_OUTBYTE(patactrl, __ATA_COMMAND(patactrl), __ATA_CMD_READP);
                                                                        /*  写入确认命令                */
        if (patactrl->ATACTRL_bIntDisable == LW_FALSE) {
            ulSemStatus = API_SemaphoreBPend(patactrl->ATACTRL_ulSyncSem,   \
                                patactrl->ATACTRL_ulSyncSemTimeout);    /*  等待同步信号                */
        }

        if ((patactrl->ATACTRL_iIntStatus & __ATA_STAT_ERR) || (ulSemStatus != ERROR_NONE)) {
            ATA_DEBUG_MSG(("__ataPread() error : status=0x%x intStatus=0x%x "     \
                           "error=0x%x ulSemStatus=%d\n",                         \
                           __ATA_CTRL_INBYTE(patactrl, __ATA_ASTATUS(patactrl)),  \
                           patactrl->ATACTRL_iIntStatus,                          \
                           __ATA_CTRL_INBYTE(patactrl, __ATA_ERROR(patactrl)), ulSemStatus));
            if (++iRetryCount > _G_iAtaRetry) {
                return  (PX_ERROR);
            }
        
        } else {
            iRetry = 0;
        }
    }

    if (__ataWait(patactrl, __ATA_STAT_DRQ) != ERROR_NONE) {            /*  等待设备准备好传输数据      */
        return  (PX_ERROR);
    }

    psBuf = (INT16 *)pvBuffer;
    __ATA_CTRL_INSTRING(patactrl, __ATA_DATA(patactrl), psBuf, 256);

#if LW_CFG_CPU_ENDIAN == 1                                              /*  big-endian                  */
#define __ATA_DATA_MSB(x)   (((x) >> 8) & 0xff)
#define __ATA_DATA_LSB(x)   ((x) & 0xff)
#define __ATA_DATA_SWAP(x)  ((__ATA_DATA_LSB(x) << 8) | __ATA_DATA_MSB(x))
    
    if (patactrl->ATACTRL_bPreadBeSwap) {
        for (i = 0; i < 256; i++) {
            psBuf[i] = (INT16)__ATA_DATA_SWAP(psBuf[i]);
        }
    }
#endif

    ATA_DEBUG_MSG(("__ataPread() end\n"));

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ataDriveInit
** 功能描述: 初始化ATA设备
** 输　入  : patactrler  ATA控制器结构指针
**           iDrive      驱动器号
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT __ataDriveInit (__PATA_CTRL patactrler, INT iDrive)
{
    __ATA_CTRL   *patactrl       = LW_NULL;
    __ATA_DRIVE  *patadrive      = LW_NULL;
    __ATA_PARAM  *pataparam      = LW_NULL;
    __ATA_TYPE   *patatype       = LW_NULL;

    INT           iConfigType    = 0;

    if (!patactrler) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    ATA_DEBUG_MSG(("Enter into __ataiDriveInit()\n"));

    patactrl    = patactrler;
    patadrive   = &patactrl->ATACTRL_ataDrive[iDrive];
    pataparam   = &patadrive->ATADRIVE_ataParam;
    patatype    = patadrive->ATADRIVE_patatypeDriverInfo;
    iConfigType = patactrl->ATACTRL_iConfigType;

    if (__ataDevIdentify(patactrler, iDrive) != ERROR_NONE) {           /*  确认设备是否存在以及设备类型*/
        patadrive->ATADRIVE_ucState = __ATA_DEV_NONE;
        goto    __error_handle;
    }

    if (patadrive->ATADRIVE_ucType == __ATA_TYPE_ATA) {                 /*  设备为ATA类型设备           */
        /*
         *  得出设备存储的几何结构
         */
        if ((iConfigType & __ATA_GEO_MASK) == ATA_GEO_FORCE) {
            __ataCmd(patactrl, iDrive, __ATA_CMD_INITP, 0, 0);
            __ataPread(patactrl, iDrive, (INT8 *)pataparam);
        
        } else if ((iConfigType & __ATA_GEO_MASK) == ATA_GEO_PHYSICAL) {
            patatype->ATATYPE_iCylinders = pataparam->ATAPAR_sCylinders;
            patatype->ATATYPE_iHeads     = pataparam->ATAPAR_sHeads;
            patatype->ATATYPE_iSectors   = pataparam->ATAPAR_sSectors;
        
        } else if ((iConfigType & __ATA_GEO_MASK) == ATA_GEO_CURRENT) {
            if ((pataparam->ATAPAR_sCurrentCylinders != 0) &&
                (pataparam->ATAPAR_sCurrentHeads     != 0) &&
                (pataparam->ATAPAR_sCurrentSectors   != 0)) {
                patatype->ATATYPE_iCylinders = pataparam->ATAPAR_sCurrentCylinders;
                patatype->ATATYPE_iHeads     = pataparam->ATAPAR_sCurrentHeads;
                patatype->ATATYPE_iSectors   = pataparam->ATAPAR_sCurrentSectors;
            
            } else {
                patatype->ATATYPE_iCylinders = pataparam->ATAPAR_sCylinders;
                patatype->ATATYPE_iHeads     = pataparam->ATAPAR_sHeads;
                patatype->ATATYPE_iSectors   = pataparam->ATAPAR_sSectors;
            }
        }

        patatype->ATATYPE_iBytes = patactrl->ATACTRL_iBytesPerSector;   /*  每扇区的大小                */

        if (pataparam->ATAPAR_sCapabilities & __ATA_IOLBA_MASK) {       /*  设备支持LBA模式,保存总扇区数*/
            patadrive->ATADRIVE_uiCapacity =
            (UINT)((((UINT)((pataparam->ATAPAR_usSectors0) & 0x0000ffff)) << 0) |
            (((UINT)((pataparam->ATAPAR_usSectors1) & 0x0000ffff)) << 16));

            if (patadrive->ATADRIVE_uiCapacity == __ATA_MAX_28LBA) {
                patadrive->ATADRIVE_uiCapacity = ((INT64)pataparam->ATAPAR_sMaxLBA[0]) |
                                                 ((INT64)pataparam->ATAPAR_sMaxLBA[1] << 16) |
                                                 ((INT64)pataparam->ATAPAR_sMaxLBA[2] << 32) |
                                                 ((INT64)pataparam->ATAPAR_sMaxLBA[3] << 48);

            }

            if (patadrive->ATADRIVE_uiCapacity > __ATA_MAX_48LBA) {
                return  (PX_ERROR);
            }

            ATA_DEBUG_MSG(("ID_iDrive reports LBA (60-61) as 0x%x\n",
                           patadrive->ATADRIVE_uiCapacity));
        }
    } else if (patadrive->ATADRIVE_ucType == __ATA_TYPE_ATAPI) {
        /*
         *  TODO: 对ATAPI类型设备的支持
         */
        return  (PX_ERROR);
    
    } else {
        ATA_DEBUG_MSG(("__ataDriveInit() error: Unknow ata drive type!\r\n"));
        return  (PX_ERROR);
    }
    
    /*
     *  得到设备所支持的特性
     */
    patadrive->ATADRIVE_sMultiSecs = (INT16)(pataparam->ATAPAR_sMultiSecs & __ATA_MULTISEC_MASK);
    patadrive->ATADRIVE_sOkMulti   = (INT16)((patadrive->ATADRIVE_sMultiSecs != 0) ? 1 : 0);
    patadrive->ATADRIVE_sOkIordy   = (INT16)((pataparam->ATAPAR_sCapabilities &   \
                                     __ATA_IORDY_MASK) ? 1 : 0);
    patadrive->ATADRIVE_sOkLba     = (INT16)((pataparam->ATAPAR_sCapabilities &   \
                                     __ATA_IOLBA_MASK) ? 1 : 0);
    patadrive->ATADRIVE_sOkDma     = (INT16)((pataparam->ATAPAR_sCapabilities &   \
                                     __ATA_DMA_CAP_MASK) ? 1 : 0);
    /*
     *  得到设备所支持的最大PIO模式
     */
    patadrive->ATADRIVE_sPioMode = (INT16)((pataparam->ATAPAR_sPioMode >> 8) & __ATA_PIO_MASK_012);
                                                                        /*  PIO 0,1,2                   */
    if (patadrive->ATADRIVE_sPioMode > __ATA_SET_PIO_MODE_2) {
        patadrive->ATADRIVE_sPioMode = __ATA_SET_PIO_MODE_0;
    }

    if ((patadrive->ATADRIVE_sOkIordy) && (pataparam->ATAPAR_sValid & __ATA_PIO_MASK_34)) {
                                                                        /*  PIO 3,4                     */
        if (pataparam->ATAPAR_sAdvancedPio & __ATA_BIT_MASK0) {
            patadrive->ATADRIVE_sPioMode = __ATA_SET_PIO_MODE_3;
        }

        if (pataparam->ATAPAR_sAdvancedPio & __ATA_BIT_MASK1) {
            patadrive->ATADRIVE_sPioMode = __ATA_SET_PIO_MODE_4;
        }
    }
    /*
     *  得到设备所支持的DMA模式
     */
    if ((patadrive->ATADRIVE_sRwDma) && (pataparam->ATAPAR_sValid & __ATA_WORD64_70_VALID)) {
        /*
         *  TODO: 对ATA设备DMA模式的支持, 目前为空操作
         */
    }
    /*
     *  得到所使用的传输模式
     */
    patadrive->ATADRIVE_sRwBits = (INT16)(iConfigType & __ATA_BITS_MASK);
    patadrive->ATADRIVE_sRwPio  = (INT16)(iConfigType & __ATA_PIO_MASK);
    patadrive->ATADRIVE_sRwDma  = (INT16)(iConfigType & __ATA_DMA_MASK);
    patadrive->ATADRIVE_sRwMode = ATA_PIO_DEF_0;                      /*  默认的PIO模式               */
    /*
     *  设备多重扇区读写模式
     */
    if ((patadrive->ATADRIVE_sRwPio == ATA_PIO_MULTI) &&
        (patadrive->ATADRIVE_ucType == __ATA_TYPE_ATA)) {
        if (patadrive->ATADRIVE_sOkMulti) {
            (VOID)__ataCmd(patactrler, iDrive, __ATA_CMD_SET_MULTI,
                           patadrive->ATADRIVE_sMultiSecs, 0);
        
        } else {
            patadrive->ATADRIVE_sRwPio = ATA_PIO_SINGLE;
        }
    }

    switch (iConfigType & __ATA_MODE_MASK) {

    case ATA_PIO_DEF_0:
    case ATA_PIO_DEF_1:
    case ATA_PIO_0:
    case ATA_PIO_1:
    case ATA_PIO_2:
    case ATA_PIO_3:
    case ATA_PIO_4:
        patadrive->ATADRIVE_sRwMode = (INT16)(iConfigType & __ATA_MODE_MASK);
        break;

    case ATA_PIO_AUTO:
        patadrive->ATADRIVE_sRwMode = (INT16)(ATA_PIO_DEF_0 + patadrive->ATADRIVE_sPioMode);
        break;

    case ATA_DMA_0:
    case ATA_DMA_1:
    case ATA_DMA_2:
    case ATA_DMA_AUTO:
        /*
         *  TODO:对ATA设备DMA传输方式的支持
         */
        break;

    default:
        break;
    }
    
    /*
     *  设置传输模式
     */
    (VOID)__ataCmd(patactrler, iDrive, __ATA_CMD_SET_FEATURE, __ATA_SUB_SET_RWMODE,
                   patadrive->ATADRIVE_sRwMode);

    patadrive->ATADRIVE_ucState = __ATA_DEV_OK;

__error_handle:
    if (patadrive->ATADRIVE_ucState != __ATA_DEV_OK) {
        ATA_DEBUG_MSG(("__ataiDriveInit() %d/%d: ERROR: state=%d iDev=0x%x "
                       "status=0x%x error=0x%x\n", patactrl->ATACTRL_iCtrl, iDrive,
                       patadrive->ATADRIVE_ucState,
                       __ATA_CTRL_INBYTE(patactrl, __ATA_SDH(patactrl)),
                       __ATA_CTRL_INBYTE(patactrl, __ATA_STATUS(patactrl)),
                       __ATA_CTRL_INBYTE(patactrl, __ATA_ERROR(patactrl))));
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ataDevIdentify
** 功能描述: 确认ATA设备
** 输　入  : patactrler  ATA控制器结构指针
**           iDrive      驱动号
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT __ataDevIdentify (__PATA_CTRL patactrler, INT iDrive)
{
    __ATA_CTRL    *patactrl  = LW_NULL;
    __ATA_DRIVE   *patadrive = LW_NULL;
    __ATA_PARAM   *pataparam = LW_NULL;

    INT            istatus   = 0;

    if (!patactrler) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    ATA_DEBUG_MSG(("Enter into __ataDevIdentify()\n"));

    patactrl  = patactrler;
    patadrive = &patactrl->ATACTRL_ataDrive[iDrive];
    pataparam = &patadrive->ATADRIVE_ataParam;

    patadrive->ATADRIVE_ucType = __ATA_TYPE_NONE;

    istatus = __ataWait(patactrl, __ATA_STAT_IDLE);
    if (istatus != ERROR_NONE) {
        ATA_DEBUG_MSG(("__ataDevIdentify() %d/%d: status = 0x%x read timed out\n",
                       patactrl->ATACTRL_iCtrl, iDrive, 
                       __ATA_CTRL_INBYTE(patactrl, __ATA_ASTATUS(patactrl))));
        return  (PX_ERROR);
    }

    __ATA_CTRL_OUTBYTE(patactrl, __ATA_SDH(patactrl),   \
                       (UINT8)(__ATA_SDH_LBA |          \
                       (iDrive << __ATA_DRIVE_BIT)));                   /*  选择设备                    */

    istatus = __ataWait(patactrl, __ATA_STAT_BUSY);
    if (istatus != ERROR_NONE) {                                        /*  等待设备选择完成            */
        ATA_DEBUG_MSG(("__ataDevIdentify() %d/%d: status = 0x%x read timed out\n",
                      patactrl->ATACTRL_iCtrl, iDrive, 
                      __ATA_CTRL_INBYTE(patactrl, __ATA_ASTATUS(patactrl))));
        return  (PX_ERROR);
    }

    __ATA_CTRL_OUTBYTE(patactrl, __ATA_DCONTROL(patactrl), (UINT8)(patactrl->ATACTRL_bIntDisable << 1));
                                                                        /*  是否使能中断                */
    istatus = __ataWait(patactrl, __ATA_STAT_IDLE);
    if (istatus != ERROR_NONE) {
        ATA_DEBUG_MSG(("__ataDevIdentify() %d/%d: status = 0x%x read timed out\n",
                       patactrl->ATACTRL_iCtrl, iDrive, 
                       __ATA_CTRL_INBYTE(patactrl, __ATA_ASTATUS(patactrl))));
        return  (PX_ERROR);
    }

    __ATA_CTRL_OUTBYTE(patactrl, __ATA_SECCNT(patactrl), 0x23);
    __ATA_CTRL_OUTBYTE(patactrl, __ATA_SECTOR(patactrl), 0x55);

    if (__ATA_CTRL_INBYTE(patactrl, __ATA_SECCNT(patactrl)) == 0x23) {
        /*
         *  已确认设备存在, 然后找出设备类型, uiSignature 由 __ataCtrlReset() 函数确定
         */
        patactrl->ATACTRL_bIsExist = LW_TRUE;

        if (patadrive->ATADRIVE_uiSignature == __ATAPI_SIGNATURE) {
            patadrive->ATADRIVE_ucType = __ATA_TYPE_ATAPI;

        } else if (patadrive->ATADRIVE_uiSignature == __ATA_SIGNATURE) {
            patadrive->ATADRIVE_ucType = __ATA_TYPE_ATA;

        } else {
            patadrive->ATADRIVE_ucType      = __ATA_TYPE_NONE;
            patadrive->ATADRIVE_uiSignature = 0;
            ATA_DEBUG_MSG(("__ataDeviceIdentify(): Unknown device found on %d/%d\n",
                           patactrl->ATACTRL_iCtrl, iDrive));
        }

        if (patadrive->ATADRIVE_ucType != __ATA_TYPE_NONE) {
            istatus = __ataPread(patactrler, iDrive, pataparam);
            if (istatus != ERROR_NONE) {
                return  (PX_ERROR);
            }
        }

    } else {                                                            /*  设备不存在                  */
        patadrive->ATADRIVE_ucType      = __ATA_TYPE_NONE;
        patadrive->ATADRIVE_uiSignature = 0xffffffff;

        ATA_DEBUG_MSG(("__ataDeviceIdentify() error: Unknown device found on %d/%d\n",
                       patactrl->ATACTRL_iCtrl, iDrive));

        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ataCtrlReset
** 功能描述: 复位ATA控制器
** 输　入  : patactrler    控制器结构指针
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT __ataCtrlReset (__PATA_CTRL patactrler)
{
    __PATA_CTRL    patactrl  = LW_NULL;
    __PATA_DRIVE   patadrive = LW_NULL;

    INT            iDrive;
    volatile INT   i;

    if (!patactrler) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    patactrl = patactrler;

    __ATA_CTRL_OUTBYTE(patactrl, __ATA_DCONTROL(patactrl), __ATA_CTL_RST | __ATA_CTL_IDS);
    __ATA_DELAYMS(1);

    __ATA_CTRL_OUTBYTE(patactrl, __ATA_DCONTROL(patactrl),  \
                       (UINT8)(patactrl->ATACTRL_bIntDisable << 1));    /*  清除复位,并设置中断位       */
    __ATA_DELAYMS(1);

    i = 0;
    while (i < 3) {
        i++;
        if (__ataWait(patactrl, __ATA_STAT_BUSY) == ERROR_NONE) {
            break;
        }
    }

    for (iDrive = 0; iDrive < patactrl->ATACTRL_iDrives; iDrive++) {
        patadrive = &patactrl->ATACTRL_ataDrive[iDrive];
        __ATA_CTRL_OUTBYTE(patactrl, __ATA_SDH(patactrl), (iDrive << __ATA_DRIVE_BIT));

        patadrive->ATADRIVE_uiSignature =                                         \
                    (__ATA_CTRL_INBYTE(patactrl, __ATA_SECCNT(patactrl)) << 24) | \
                    (__ATA_CTRL_INBYTE(patactrl, __ATA_SECTOR(patactrl)) << 16) | \
                    (__ATA_CTRL_INBYTE(patactrl, __ATA_CYLLO(patactrl))  << 8)  | \
                    (__ATA_CTRL_INBYTE(patactrl, __ATA_CYLHI(patactrl)));
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_ATA_EN > 0)         */
/*********************************************************************************************************
    END FILE
*********************************************************************************************************/
