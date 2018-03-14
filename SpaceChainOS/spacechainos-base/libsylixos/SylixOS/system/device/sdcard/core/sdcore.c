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
** 文   件   名: sdcore.c
**
** 创   建   人: Zeng.Bo (曾波)
**
** 文件创建日期: 2010 年 11 月 23 日
**
** 描        述: sd 卡内核协议层接口源文件

** BUG:
2010.11.25  增加对 SPI 设备的封装.增加几个设备信息查看 API.
2010.12.08  增加对 SDHC 的支持.
2011.01.10  增加对 SPI 的支持.
2011.02.21  增加 API_SdCoreSpiSendIfCond 函数.该函数只能用于 SPI 模式下.
2011.02.21  增 SPI 下设备寄存器的读取函数: API_SdCoreSpiRegisterRead().
2011.02.22  增 SPI 下 CRC 校验.
2011.03.25  修改 API_SdCoreDevCreate(), 用于底层驱动安装上层的回调.
2011.04.02  全面修改 SPI 模式下的相关函数,使用新的 SPI 模型.
2014.11.05  设备的状态增加自旋锁保护.
2016.01.21  修正 SPI 模式下的总线控制操作以及协议相关的错误.
2017.01.06  修正 SPI 模式下发送数据相关代码.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SDCARD_EN > 0)
#include "sdstd.h"
#include "sdcrc.h"
#include "sdcore.h"
#include "../include/sddebug.h"
/*********************************************************************************************************
  SPI通信配置
*********************************************************************************************************/
#define __SD_SPI_BITS_PEROP   8
#define __SD_SPI_CLKMOD_FLG   LW_SPI_M_CPOL_0  | LW_SPI_M_CPHA_0  |  \
                              LW_SPI_M_CPOL_EN | LW_SPI_M_CPHA_EN       /*  SPI通信使用的时钟模式       */

#define __SD_SPI_TMOD_RD      __SD_SPI_CLKMOD_FLG | LW_SPI_M_WRBUF_FIX  /*  读操作控制标记              */
#define __SD_SPI_TMOD_WR      __SD_SPI_CLKMOD_FLG | LW_SPI_M_RDBUF_FIX  /*  写操作控制标记              */

#define __SD_SPI_RSP_TIMEOUT  0x3fffff                                  /*  SPI应答超时最大计数         */
#define __SD_SPI_RSP_WAITSEC  4                                         /*  SPI应答超时最大物理时间     */
#define __SD_SPI_DAT_WAITSEC  4                                         /*  SPI数据编程超时最大物理时间 */

#define __SD_SPI_CLK_LOW      400000                                    /*  一般时钟, 400k              */
#define __SD_SPI_CLK_MAX      25000000                                  /*  高速时钟, 25m               */
/*********************************************************************************************************
  SPI 总线请求\释放
*********************************************************************************************************/
#define __SD_SPI_CSEN(pcsdev)           (pcsdev)->CSPIDEV_cbCsEn((pcsdev)->CSPIDEV_psdcorecha)
#define __SD_SPI_CSDIS(pcsdev)          (pcsdev)->CSPIDEV_cbCsDis((pcsdev)->CSPIDEV_psdcorecha)
/*********************************************************************************************************
  SD 核心层设备操作宏
*********************************************************************************************************/
#define __CORE_DEV_TXF(cd, pmsg, num)   cd->COREDEV_pfuncCoreDevXfer(cd->COREDEV_pvDevHandle,\
                                                                     pmsg, num)
#define __CORE_DEV_CTL(cd, icmd, larg)  cd->COREDEV_pfuncCoreDevCtl(cd->COREDEV_pvDevHandle, \
                                                                    icmd, larg)
#define __CORE_DEV_DEL(cd)              cd->COREDEV_pfuncCoreDevDelet(cd->COREDEV_pvDevHandle)
/*********************************************************************************************************
   核心层封装的SPI设备
*********************************************************************************************************/
typedef struct __core_spi_dev {
    PLW_SPI_DEVICE                CSPIDEV_pspiDev;
    UINT8                         CSPIDEV_ucType;
    UINT32                        CSPIDEV_uiState;                      /*  设备状态位标                */

    LW_SDDEV_CID                  CSPIDEV_cid;
    LW_SDDEV_CSD                  CSPIDEV_csd;
    LW_SDDEV_SCR                  CSPIDEV_scr;
    LW_SDDEV_SW_CAP               CSPIDEV_swcap;

    PLW_SDCORE_CHAN               CSPIDEV_psdcorecha;
    SDCORE_CALLBACK_SPICS_ENABLE  CSPIDEV_cbCsEn;
    SDCORE_CALLBACK_SPICS_DISABLE CSPIDEV_cbCsDis;
} __CORE_SPI_DEV, *__PCORE_SPI_DEV;
/*********************************************************************************************************
  私有接口声明
*********************************************************************************************************/
static INT  __sdCoreSpiDevTransfer(PVOID pvDevHandle, PLW_SD_MESSAGE psdmsg, INT iNum);
static INT  __sdCoreSdDevTransfer(PVOID pvDevHandle, PLW_SD_MESSAGE psdmsg, INT iNum);

static INT  __sdCoreSpiDevCtl(PVOID  pvDevHandle, INT  iCmd, LONG lArg);
static INT  __sdCoreSdDevCtl(PVOID  pvDevHandle, INT  iCmd, LONG lArg);

static INT  __sdCoreSpiDevDelet(PVOID pvDevHandle);
static INT  __sdCoreSdDevDelet(PVOID pvDevHandle);

static INT  __sdCoreSpiCallbackCheckDev(PVOID pvDevHandle, INT iDevSta);
static INT  __sdCoreSdCallbackCheckDev(PVOID pvDevHandle, INT iDevSta);

static INT  __sdCoreCallbackCheckDev(PLW_SDCORE_DEVICE psdcoredevice, INT iDevSta);

static INT  __sdCoreDevSwitchToApp(PLW_SDCORE_DEVICE psdcoredevice, BOOL bIsBc);
/*********************************************************************************************************
  以下私有接口为sd对spi传输控制的封装
*********************************************************************************************************/
static INT  __sdCoreSpiCmd(__PCORE_SPI_DEV pcspidevice, LW_SD_COMMAND *psdcmd);
static INT  __sdCoreSpiDataRd(__PCORE_SPI_DEV pcspidevice,
                              UINT32          uiBlkNum,
                              UINT32          uiBlkLen,
                              UINT8          *pucRdBuff);
static INT  __sdCoreSpiDataWrt(__PCORE_SPI_DEV pcspidevice,
                               UINT32          uiBlkNum,
                               UINT32          uiBlkLen,
                               UINT8          *pucWrtBuff);

static INT  __sdCoreSpiBlkRd(__PCORE_SPI_DEV pcspidevice,
                             UINT32          uiBlkLen,
                             UINT8          *pucRdBuff);
static INT  __sdCoreSpiBlkWrt(__PCORE_SPI_DEV pcspidevice,
                              UINT32          uiBlkLen,
                              UINT8          *pucWrtBuff,
                              BOOL            bIsMul);

static INT  __sdCoreSpiByteRd(__PCORE_SPI_DEV pcspidevice,
                              UINT32          uiLen,
                              UINT8          *pucRdBuff);
static INT  __sdCoreSpiByteWrt(__PCORE_SPI_DEV pcspidevice,
                               UINT32          uiLen,
                               UINT8          *pucWrtBuff);
static INT  __sdCoreSpiWaitBusy(__PCORE_SPI_DEV pcspidevice);
static VOID __sdCoreSpiMulWrtStop(__PCORE_SPI_DEV pcspidevice);
/*********************************************************************************************************
  SPI 工具函数
*********************************************************************************************************/
static VOID  __sdCoreSpiParamConvert(UINT8 *pucParam, UINT32 uiParam);
static VOID  __sdCoreSpiRespConvert(UINT32 *puiResp, const UINT8 *pucResp, INT iRespLen);
static INT   __sdCoreSpiRespLen(UINT32   uiCmdFlg);
/*********************************************************************************************************
** 函数名称: API_SdCoreDevCreate
** 功能描述: 创建一个核心SD设备
** 输    入: iAdapterType     设备挂接的适配器类型 (SDADAPTER_TYPE_SPI 或 SDADAPTER_TYPE_SD)
**           pcAdapterName    挂接的适配器名称
**           pcDeviceName     设备名称
**           psdcorechan      通道
** 输    出: NONE
** 返    回: 成功,返回设备设备指针,否则返回LW_NULL
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API PLW_SDCORE_DEVICE  API_SdCoreDevCreate (INT                       iAdapterType,
                                               CPCHAR                    pcAdapterName,
                                               CPCHAR                    pcDeviceName,
                                               PLW_SDCORE_CHAN           psdcorechan)
{
    PLW_SDCORE_DEVICE   psdcoredevice   = LW_NULL;
    PLW_SD_DEVICE       psddevice       = LW_NULL;
    PLW_SPI_DEVICE      pspidevice      = LW_NULL;
    __PCORE_SPI_DEV     pcspidevice     = LW_NULL;

    if (!pcAdapterName || !pcDeviceName || !psdcorechan) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    switch (iAdapterType) {

    case SDADAPTER_TYPE_SD:
        psddevice = API_SdDeviceCreate(pcAdapterName, pcDeviceName);
        if (!psddevice) {
            goto    __createdev_failed;
        }
        psddevice->SDDEV_uiState |= SD_STATE_EXIST;                     /*  该设备已经存在(此处很关键)  */

        psdcoredevice = (PLW_SDCORE_DEVICE)__SHEAP_ALLOC(sizeof(LW_SDCORE_DEVICE));
        if (!psdcoredevice) {
            API_SdDeviceDelete(psddevice);
            goto    __allocdev_failed;
        }
        lib_bzero(psdcoredevice, sizeof(LW_SDCORE_DEVICE));
        
        psdcoredevice->COREDEV_pvDevHandle        = (PVOID)psddevice;
        psdcoredevice->COREDEV_pfuncCoreDevXfer   = __sdCoreSdDevTransfer;
        psdcoredevice->COREDEV_pfuncCoreDevCtl    = __sdCoreSdDevCtl;
        psdcoredevice->COREDEV_pfuncCoreDevDelet  = __sdCoreSdDevDelet;
        psdcoredevice->COREDEV_iAdapterType       = SDADAPTER_TYPE_SD;

        LW_SPIN_INIT(&psdcoredevice->COREDEV_slLock);

        if (SDCORE_CHAN_INSTALL(psdcorechan)) {                         /*  安装回调                    */
            SDCORE_CHAN_CBINSTALL(psdcorechan,
                                  SD_CALLBACK_CHECK_DEV,
                                  __sdCoreCallbackCheckDev,
                                  psdcoredevice);
        } else {
            __SHEAP_FREE(psdcoredevice);
            API_SdDeviceDelete(psddevice);
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "need callback of install.\r\n");
            return  (LW_NULL);
        }
        return  (psdcoredevice);

    case SDADAPTER_TYPE_SPI:
        if (!SDCORE_CHAN_SPICSEN(psdcorechan) ||
            !SDCORE_CHAN_SPICSDIS(psdcorechan)) {                       /*  spi下必须有片选回调         */
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "need callback of spi card select in spi mode.\r\n");
            _ErrorHandle(EINVAL);
            return  (LW_NULL);
        }

        pspidevice = API_SpiDeviceCreate(pcAdapterName, pcDeviceName);  /*  创建spi设备                 */
        if (!pspidevice) {
            goto    __createdev_failed;
        }
        pcspidevice = (__PCORE_SPI_DEV)__SHEAP_ALLOC(sizeof(__CORE_SPI_DEV));
        if (!pcspidevice) {                                             /*  创建core spi 设备           */
            API_SpiDeviceDelete(pspidevice);
            goto    __allocdev_failed;
        }
        lib_bzero(pcspidevice, sizeof(__CORE_SPI_DEV));
        
        pcspidevice->CSPIDEV_uiState = SD_DEVSTA_EXIST;                 /*  该设备已经存在(此处很关键)  */

        psdcoredevice = (PLW_SDCORE_DEVICE)__SHEAP_ALLOC(sizeof(LW_SDCORE_DEVICE));
        if (!psdcoredevice) {                                           /*  创建核心设备                */
            API_SpiDeviceDelete(pspidevice);
            __SHEAP_FREE(pcspidevice);
            goto    __allocdev_failed;
        }

        pcspidevice->CSPIDEV_pspiDev    = pspidevice;                   /*  绑定                        */
        pcspidevice->CSPIDEV_psdcorecha = psdcorechan;                  /*  保存通道                    */
        pcspidevice->CSPIDEV_cbCsEn     = SDCORE_CHAN_SPICSEN(psdcorechan);
        pcspidevice->CSPIDEV_cbCsDis    = SDCORE_CHAN_SPICSDIS(psdcorechan);
                                                                        /*  保存片选回调                */
        lib_bzero(psdcoredevice, sizeof(LW_SDCORE_DEVICE));
        
        psdcoredevice->COREDEV_pvDevHandle        = (PVOID)pcspidevice;
        psdcoredevice->COREDEV_pfuncCoreDevXfer   = __sdCoreSpiDevTransfer;
        psdcoredevice->COREDEV_pfuncCoreDevCtl    = __sdCoreSpiDevCtl;
        psdcoredevice->COREDEV_pfuncCoreDevDelet  = __sdCoreSpiDevDelet;
        psdcoredevice->COREDEV_iAdapterType       = SDADAPTER_TYPE_SPI;

        LW_SPIN_INIT(&psdcoredevice->COREDEV_slLock);

        if (SDCORE_CHAN_INSTALL(psdcorechan)) {                         /*  安装回调函数                */
            SDCORE_CHAN_CBINSTALL(psdcorechan,
                                  SD_CALLBACK_CHECK_DEV,
                                  __sdCoreCallbackCheckDev,
                                  psdcoredevice);
        } else {
            __SHEAP_FREE(psdcoredevice);
            __SHEAP_FREE(pcspidevice);
            API_SpiDeviceDelete(pspidevice);
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "callback install error.\r\n");
            return  (LW_NULL);
        }
        return  (psdcoredevice);

    default:
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "adapter type error.\r\n");
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

__createdev_failed:
    SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "create device(sd/spi) failed.\r\n");
    return  (LW_NULL);

__allocdev_failed:
    SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
    _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);

    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: API_SdCoreDevDelete
** 功能描述: 删除一个核心SD设备
** 输    入: psdcoredevice 设备结构指针
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API INT  API_SdCoreDevDelete (PLW_SDCORE_DEVICE    psdcoredevice)
{
    INT     iError;

    if (!psdcoredevice) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    /*
     * 首先删除对应的SPI或SD设备
     */
    iError = __CORE_DEV_DEL(psdcoredevice);

    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "delete device(sd/spi) failed.\r\n");
        return  (PX_ERROR);
    }

    /*
     * 再释放core设备
     */
    __SHEAP_FREE(psdcoredevice);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_SdCoreDevCtl
** 功能描述: 核心设备控制
** 输    入: psdcoredevice 设备结构指针
**           iCmd          控制命令
**           lArg          命令参数
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API INT  API_SdCoreDevCtl (PLW_SDCORE_DEVICE    psdcoredevice,
                              INT                  iCmd,
                              LONG                 lArg)
{
    INT     iError;

    if (!psdcoredevice) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    iError = API_SdCoreDevStaView(psdcoredevice);
    if (iError == SD_DEVSTA_UNEXIST) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "dev is not exist.\r\n");
        return  (PX_ERROR);
    }

    iError = __CORE_DEV_CTL(psdcoredevice, iCmd, lArg);

    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "ctrl of device(sd/spi) failed.\r\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_SdCoreDevTransfer
** 功能描述: 核心设备请求
** 输    入: psdcoredevice 设备结构指针
**           psdmsg        控制消息
**           iNum          消息个数
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API INT  API_SdCoreDevTransfer (PLW_SDCORE_DEVICE  psdcoredevice,
                                   PLW_SD_MESSAGE     psdmsg,
                                   INT                iNum)
{
    INT     iError;

    if (!psdcoredevice || !psdmsg) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    iError = API_SdCoreDevStaView(psdcoredevice);
    if (iError == SD_DEVSTA_UNEXIST) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "dev is not exist.\r\n");
        return  (PX_ERROR);
    }

    iError = __CORE_DEV_TXF(psdcoredevice, psdmsg, iNum);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "request of device(sd/spi) failed.\r\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_SdCoreDevCmd
** 功能描述: 核心设备发送命令
** 输    入: psdcoredevice 设备结构指针
**           psdcmd        命令
**           uiRetry       重试计数
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API INT  API_SdCoreDevCmd (PLW_SDCORE_DEVICE psdcoredevice,
                              PLW_SD_COMMAND    psdcmd,
                              UINT32            uiRetry)
{
    LW_SD_MESSAGE   sdmsg;
    INT             iError;

    if (!psdcoredevice || !psdcmd) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    lib_bzero(&sdmsg, sizeof(sdmsg));
    lib_bzero(psdcmd->SDCMD_uiResp, sizeof(psdcmd->SDCMD_uiResp));
    
    psdcmd->SDCMD_uiRetry = uiRetry;
    sdmsg.SDMSG_psdcmdCmd = psdcmd;

    iError = API_SdCoreDevTransfer(psdcoredevice, &sdmsg, 1);

    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSGX(__ERRORMESSAGE_LEVEL, "send cmd%d error.\r\n", psdcmd->SDCMD_uiOpcode);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_SdCoreDevAppSwitch
** 功能描述: 切换到APP命令
** 输    入: psdcoredevice  核心结构指针
**           bIsBc          是否针对广播命令
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API INT  API_SdCoreDevAppSwitch (PLW_SDCORE_DEVICE psdcoredevice, BOOL bIsBc)
{
    INT iRet;

    iRet = __sdCoreDevSwitchToApp(psdcoredevice, bIsBc);

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_SdCoreDevAppCmd
** 功能描述: 核心设备发送APP命令
** 输    入: psdcoredevice  设备结构指针
**           psdcmdAppCmd   应用命令
**           bIsBc          是否是广播命令
**           uiRetry        重试计数
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API INT   API_SdCoreDevAppCmd (PLW_SDCORE_DEVICE psdcoredevice,
                                  PLW_SD_COMMAND    psdcmdAppCmd,
                                  BOOL              bIsBc,
                                  UINT32            uiRetry)
{
    INT     iError = PX_ERROR;
    INT     i;

    /*
     * 对于应用命令,每次发送前都要使用CMD55切换到应用命令状态.
     * 切换失败原因有很多,因此可以多试几次.
     */
    for (i = 0; i <= uiRetry; i++) {
       iError = __sdCoreDevSwitchToApp(psdcoredevice, bIsBc);
       if (iError != ERROR_NONE) {
           continue;
       }

       iError = API_SdCoreDevCmd(psdcoredevice, psdcmdAppCmd, 0);
       break;
    }

    if (i > uiRetry) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "timeout.\r\n");
        iError = PX_ERROR;
    } else if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "send app cmd error.\r\n");
    }

    return  (iError);
}
/*********************************************************************************************************
** 函数名称: API_SdCoreDevAdapterName
** 功能描述: 返回对应的总线适配器名称
** 输    入: psdcoredevice  设备结构指针
** 输    出:
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API CPCHAR  API_SdCoreDevAdapterName (PLW_SDCORE_DEVICE psdcoredevice)
{
    PLW_SD_DEVICE    psddevice;
    __PCORE_SPI_DEV  pcspidevice;
    LW_BUS_ADAPTER  *pBusAdapter;

    if (!psdcoredevice) {
        return  (LW_NULL);
    }

    switch (psdcoredevice->COREDEV_iAdapterType) {

    case SDADAPTER_TYPE_SD:
        psddevice   = (PLW_SD_DEVICE)psdcoredevice->COREDEV_pvDevHandle;
        pBusAdapter = &psddevice->SDDEV_psdAdapter->SDADAPTER_busadapter;
        break;

    case SDADAPTER_TYPE_SPI:
        pcspidevice = (__PCORE_SPI_DEV)psdcoredevice->COREDEV_pvDevHandle;
        pBusAdapter = &pcspidevice->CSPIDEV_pspiDev->SPIDEV_pspiadapter->SPIADAPTER_pbusadapter;
        break;

    default:
        return  (LW_NULL);
    }

    return  ((CPCHAR)pBusAdapter->BUSADAPTER_cName);
}
/*********************************************************************************************************
** 函数名称: API_SdCoreDevCsdView
** 功能描述: 查看设备的CSD
** 输    入: psdcoredevice 设备结构指针
** 输    出: psdcsd        设备CSD
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API INT   API_SdCoreDevCsdView (PLW_SDCORE_DEVICE psdcoredevice,  PLW_SDDEV_CSD psdcsd)
{
    PLW_SD_DEVICE    psddevice;
    __PCORE_SPI_DEV  pcspidevice;

    switch (psdcoredevice->COREDEV_iAdapterType) {

    case SDADAPTER_TYPE_SD:
        psddevice = (PLW_SD_DEVICE)psdcoredevice->COREDEV_pvDevHandle;
        lib_memcpy(psdcsd, &psddevice->SDDEV_csd, sizeof(LW_SDDEV_CSD));
        return  (ERROR_NONE);

    case SDADAPTER_TYPE_SPI:
        pcspidevice = (__PCORE_SPI_DEV)psdcoredevice->COREDEV_pvDevHandle;
        lib_memcpy(psdcsd, &pcspidevice->CSPIDEV_csd, sizeof(LW_SDDEV_CSD));
        return  (ERROR_NONE);

    default:
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_SdCoreDevCidView
** 功能描述: 查看设备的CID
** 输    入: psdcoredevice 设备结构指针
** 输    出: psdcid        设备CID
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API INT  API_SdCoreDevCidView (PLW_SDCORE_DEVICE psdcoredevice,  PLW_SDDEV_CID psdcid)
{
    PLW_SD_DEVICE    psddevice;
    __PCORE_SPI_DEV  pcspidevice;

    if (!psdcoredevice || !psdcid) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    switch (psdcoredevice->COREDEV_iAdapterType) {

    case SDADAPTER_TYPE_SD:
        psddevice = (PLW_SD_DEVICE)psdcoredevice->COREDEV_pvDevHandle;
        lib_memcpy(psdcid, &psddevice->SDDEV_cid, sizeof(LW_SDDEV_CID));
        return  (ERROR_NONE);

    case SDADAPTER_TYPE_SPI:
        pcspidevice = (__PCORE_SPI_DEV)psdcoredevice->COREDEV_pvDevHandle;
        lib_memcpy(psdcid, &pcspidevice->CSPIDEV_cid, sizeof(LW_SDDEV_CID));
        return  (ERROR_NONE);

    default:
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_SdCoreDevScrView
** 功能描述: 查看设备的SCR
** 输    入: psdcoredevice 设备结构指针
** 输    出: psdscr        设备SCR
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API INT  API_SdCoreDevScrView (PLW_SDCORE_DEVICE psdcoredevice,  PLW_SDDEV_SCR psdscr)
{
    PLW_SD_DEVICE    psddevice;
    __PCORE_SPI_DEV  pcspidevice;

    if (!psdcoredevice || !psdscr) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    switch (psdcoredevice->COREDEV_iAdapterType) {

    case SDADAPTER_TYPE_SD:
        psddevice = (PLW_SD_DEVICE)psdcoredevice->COREDEV_pvDevHandle;
        lib_memcpy(psdscr, &psddevice->SDDEV_scr, sizeof(LW_SDDEV_SCR));
        return  (ERROR_NONE);

    case SDADAPTER_TYPE_SPI:
        pcspidevice = (__PCORE_SPI_DEV)psdcoredevice->COREDEV_pvDevHandle;
        lib_memcpy(psdscr, &pcspidevice->CSPIDEV_scr, sizeof(LW_SDDEV_SCR));
        return  (ERROR_NONE);

    default:
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_SdCoreDevSwCapView
** 功能描述: 查看设备的 SWITCH 功能
** 输    入: psdcoredevice 设备结构指针
** 输    出: psdswcap      SWITCH 功能
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API INT  API_SdCoreDevSwCapView (PLW_SDCORE_DEVICE psdcoredevice,  PLW_SDDEV_SW_CAP psdswcap)
{
    PLW_SD_DEVICE    psddevice;
    __PCORE_SPI_DEV  pcspidevice;

    if (!psdcoredevice || !psdswcap) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    switch (psdcoredevice->COREDEV_iAdapterType) {

    case SDADAPTER_TYPE_SD:
        psddevice = (PLW_SD_DEVICE)psdcoredevice->COREDEV_pvDevHandle;
        lib_memcpy(psdswcap, &psddevice->SDDEV_swcap, sizeof(LW_SDDEV_SW_CAP));
        return  (ERROR_NONE);

    case SDADAPTER_TYPE_SPI:
        pcspidevice = (__PCORE_SPI_DEV)psdcoredevice->COREDEV_pvDevHandle;
        lib_memcpy(psdswcap, &pcspidevice->CSPIDEV_swcap, sizeof(LW_SDDEV_SW_CAP));
        return  (ERROR_NONE);

    default:
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_SdCoreDevRcaView
** 功能描述: 查看设备的本地地址(只能用于SD总线上的设备)
** 输    入: psdcoredevice 设备结构指针
** 输    出: puiRCA       设备RCA
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API INT  API_SdCoreDevRcaView (PLW_SDCORE_DEVICE psdcoredevice,  UINT32   *puiRCA)
{
    PLW_SD_DEVICE    psddevice;

    switch (psdcoredevice->COREDEV_iAdapterType) {

    case SDADAPTER_TYPE_SD:
        psddevice = (PLW_SD_DEVICE)psdcoredevice->COREDEV_pvDevHandle;
        *puiRCA = psddevice->SDDEV_uiRCA;
        return  (ERROR_NONE);

    case SDADAPTER_TYPE_SPI:
        *puiRCA = 0;                                                    /*  这里与rca set 函数不冲突    */
        return  (ERROR_NONE);

    default:
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_SdCoreDevTypeView
** 功能描述: 查看设备的类型(MMC\SDSC\SDXC\SDHC\SDIO\COMM)
** 输    入: psdcoredevice 设备结构指针
** 输    出: pucType     设备类型
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API INT   API_SdCoreDevTypeView (PLW_SDCORE_DEVICE psdcoredevice,  UINT8  *pucType)
{
    PLW_SD_DEVICE    psddevice;
    __PCORE_SPI_DEV  pcspidevice;

    if (!psdcoredevice || !pucType) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    switch (psdcoredevice->COREDEV_iAdapterType) {

    case SDADAPTER_TYPE_SD:
         psddevice  = (PLW_SD_DEVICE)psdcoredevice->COREDEV_pvDevHandle;
        *pucType    = psddevice->SDDEV_ucType;
        return  (ERROR_NONE);

    case SDADAPTER_TYPE_SPI:
         pcspidevice = (__PCORE_SPI_DEV)psdcoredevice->COREDEV_pvDevHandle;
        *pucType     = pcspidevice->CSPIDEV_ucType;
        return  (ERROR_NONE);

    default:
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_SdCoreDevCsdSet
** 功能描述: 设置设备的CSD
** 输    入: psdcoredevice 设备结构指针
**           psdcsd        设备CSD
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API INT  API_SdCoreDevCsdSet (PLW_SDCORE_DEVICE psdcoredevice,  PLW_SDDEV_CSD psdcsd)
{
    PLW_SD_DEVICE    psddevice;
    __PCORE_SPI_DEV  pcspidevice;

    if (!psdcoredevice || !psdcsd) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    switch (psdcoredevice->COREDEV_iAdapterType) {

    case SDADAPTER_TYPE_SD:
        psddevice = (PLW_SD_DEVICE)psdcoredevice->COREDEV_pvDevHandle;
        lib_memcpy(&psddevice->SDDEV_csd, psdcsd, sizeof(LW_SDDEV_CSD));
        return  (ERROR_NONE);

    case SDADAPTER_TYPE_SPI:
        pcspidevice = (__PCORE_SPI_DEV)psdcoredevice->COREDEV_pvDevHandle;
        lib_memcpy(&pcspidevice->CSPIDEV_csd, psdcsd, sizeof(LW_SDDEV_CSD));
        return  (ERROR_NONE);

    default:
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_SdCoreDevCidSet
** 功能描述: 设置设备的CID
** 输    入: psdcoredevice 设备结构指针
**           psdcid        设备CID
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API INT  API_SdCoreDevCidSet (PLW_SDCORE_DEVICE psdcoredevice,  PLW_SDDEV_CID psdcid)
{
    PLW_SD_DEVICE    psddevice;
    __PCORE_SPI_DEV  pcspidevice;

    if (!psdcoredevice || !psdcid) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    switch (psdcoredevice->COREDEV_iAdapterType) {

    case SDADAPTER_TYPE_SD:
        psddevice = (PLW_SD_DEVICE)psdcoredevice->COREDEV_pvDevHandle;
        lib_memcpy(&psddevice->SDDEV_cid, psdcid,  sizeof(LW_SDDEV_CID));
        return  (ERROR_NONE);

    case SDADAPTER_TYPE_SPI:
        pcspidevice = (__PCORE_SPI_DEV)psdcoredevice->COREDEV_pvDevHandle;
        lib_memcpy( &pcspidevice->CSPIDEV_cid, psdcid, sizeof(LW_SDDEV_CID));
        return  (ERROR_NONE);

    default:
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_SdCoreDevScrSet
** 功能描述: 设置设备的SCR
** 输    入: psdcoredevice 设备结构指针
**           psdscr        设备SCR
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API INT  API_SdCoreDevScrSet (PLW_SDCORE_DEVICE psdcoredevice,  PLW_SDDEV_SCR psdscr)
{
    PLW_SD_DEVICE    psddevice;
    __PCORE_SPI_DEV  pcspidevice;

    if (!psdcoredevice || !psdscr) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    switch (psdcoredevice->COREDEV_iAdapterType) {

    case SDADAPTER_TYPE_SD:
        psddevice = (PLW_SD_DEVICE)psdcoredevice->COREDEV_pvDevHandle;
        lib_memcpy(&psddevice->SDDEV_scr, psdscr,  sizeof(LW_SDDEV_SCR));
        return  (ERROR_NONE);

    case SDADAPTER_TYPE_SPI:
        pcspidevice = (__PCORE_SPI_DEV)psdcoredevice->COREDEV_pvDevHandle;
        lib_memcpy( &pcspidevice->CSPIDEV_scr, psdscr, sizeof(LW_SDDEV_SCR));
        return  (ERROR_NONE);

    default:
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_SdCoreDevSwCapSet
** 功能描述: 设置设备的 SWITCH 功能
** 输    入: psdcoredevice 设备结构指针
**           psdswcap      SWITCH 功能
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API INT  API_SdCoreDevSwCapSet (PLW_SDCORE_DEVICE psdcoredevice,  PLW_SDDEV_SW_CAP psdswcap)
{
    PLW_SD_DEVICE    psddevice;
    __PCORE_SPI_DEV  pcspidevice;

    if (!psdcoredevice || !psdswcap) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    switch (psdcoredevice->COREDEV_iAdapterType) {

    case SDADAPTER_TYPE_SD:
        psddevice = (PLW_SD_DEVICE)psdcoredevice->COREDEV_pvDevHandle;
        lib_memcpy(&psddevice->SDDEV_swcap, psdswcap,  sizeof(LW_SDDEV_SW_CAP));
        return  (ERROR_NONE);

    case SDADAPTER_TYPE_SPI:
        pcspidevice = (__PCORE_SPI_DEV)psdcoredevice->COREDEV_pvDevHandle;
        lib_memcpy(&pcspidevice->CSPIDEV_swcap, psdswcap, sizeof(LW_SDDEV_SW_CAP));
        return  (ERROR_NONE);

    default:
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_SdCoreDevRcaSet
** 功能描述: 设置设备的RCA(只能用于SD总线的设备)
** 输    入: psdcoredevice 设备结构指针
**           uiRCA       设备RCA
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API INT  API_SdCoreDevRcaSet (PLW_SDCORE_DEVICE psdcoredevice,  UINT32  uiRCA)
{
    PLW_SD_DEVICE    psddevice;

    if (!psdcoredevice) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    switch (psdcoredevice->COREDEV_iAdapterType) {

    case SDADAPTER_TYPE_SD:
        psddevice = (PLW_SD_DEVICE)psdcoredevice->COREDEV_pvDevHandle;
        psddevice->SDDEV_uiRCA = uiRCA;
        return  (ERROR_NONE);

    case SDADAPTER_TYPE_SPI:
    default:
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "in spi mode, device has no rca .\r\n");
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_SdCoreDevTypeSet
** 功能描述: 设置设备的类型(MMC\SDSC\SDXC\SDHC\SDIO\COMM)
** 输    入: psdcoredevice 设备结构指针
**           iType         设备类型
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API INT   API_SdCoreDevTypeSet (PLW_SDCORE_DEVICE psdcoredevice,  UINT8  ucType)
{
    PLW_SD_DEVICE    psddevice;
    __PCORE_SPI_DEV  pcspidevice;

    if (!psdcoredevice || (ucType > SDDEV_TYPE_MAXVAL)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    switch (psdcoredevice->COREDEV_iAdapterType) {

    case SDADAPTER_TYPE_SD:
        psddevice = (PLW_SD_DEVICE)psdcoredevice->COREDEV_pvDevHandle;
        psddevice->SDDEV_ucType = ucType;
        return  (ERROR_NONE);

    case SDADAPTER_TYPE_SPI:
        pcspidevice = (__PCORE_SPI_DEV)psdcoredevice->COREDEV_pvDevHandle;
        pcspidevice->CSPIDEV_ucType = ucType;
        return  (ERROR_NONE);

    default:
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_SdCoreDevStaView
** 功能描述: 查看设备的状态
** 输    入: psdcoredevice 设备结构指针
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API INT   API_SdCoreDevStaView (PLW_SDCORE_DEVICE  psdcoredevice)
{
    PLW_SD_DEVICE    psddevice;
    __PCORE_SPI_DEV  pcspidevice;
    INT              iSta;
    INTREG           iregInterLevel;

    if (!psdcoredevice) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    LW_SPIN_LOCK_QUICK(&psdcoredevice->COREDEV_slLock, &iregInterLevel);
    if (COREDEV_IS_SD(psdcoredevice)) {
        psddevice = (PLW_SD_DEVICE)psdcoredevice->COREDEV_pvDevHandle;
        if (psddevice->SDDEV_uiState & SD_STATE_EXIST) {
            iSta = SD_DEVSTA_EXIST;
        } else {
            iSta = SD_DEVSTA_UNEXIST;
        }

    } else {
        pcspidevice = (__PCORE_SPI_DEV)psdcoredevice->COREDEV_pvDevHandle;
        iSta = pcspidevice->CSPIDEV_uiState;
    }
    LW_SPIN_UNLOCK_QUICK(&psdcoredevice->COREDEV_slLock, iregInterLevel);

    return  (iSta);
}
/*********************************************************************************************************
** 函数名称: API_SdCoreSpiCxdFormat
** 功能描述: SPI 下的CID或CSD数据格式化工具(16字节转 4*4 word)
** 输    入: puiCxdOut   输出
**           pucRawCxd   原始字节
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API VOID API_SdCoreSpiCxdFormat (UINT32 *puiCxdOut, UINT8 *pucRawCxd)
{
    *puiCxdOut   = *pucRawCxd++;
    *puiCxdOut   = (*puiCxdOut << 8) | *pucRawCxd++;
    *puiCxdOut   = (*puiCxdOut << 8) | *pucRawCxd++;
    *puiCxdOut   = (*puiCxdOut << 8) | *pucRawCxd++;                    /*  0                           */

     puiCxdOut++;
    *puiCxdOut   = *pucRawCxd++;
    *puiCxdOut   = (*puiCxdOut << 8) | *pucRawCxd++;
    *puiCxdOut   = (*puiCxdOut << 8) | *pucRawCxd++;
    *puiCxdOut   = (*puiCxdOut << 8) | *pucRawCxd++;                    /*  1                           */

     puiCxdOut++;
    *puiCxdOut   = *pucRawCxd++;
    *puiCxdOut   = (*puiCxdOut << 8) | *pucRawCxd++;
    *puiCxdOut   = (*puiCxdOut << 8) | *pucRawCxd++;
    *puiCxdOut   = (*puiCxdOut << 8) | *pucRawCxd++;                    /*  2                           */

     puiCxdOut++;
    *puiCxdOut   = *pucRawCxd++;
    *puiCxdOut   = (*puiCxdOut << 8) | *pucRawCxd++;
    *puiCxdOut   = (*puiCxdOut << 8) | *pucRawCxd++;
    *puiCxdOut   = (*puiCxdOut << 8) | *pucRawCxd++;                    /*  3                           */
}
/*********************************************************************************************************
** 函数名称: API_SdCoreSpiMulWrtStop
** 功能描述: SPI 下的多块写操作停止令牌
** 输    入: psdcoredevice sd内核设备指针
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API VOID  API_SdCoreSpiMulWrtStop (PLW_SDCORE_DEVICE psdcoredevice)
{
    UINT8           pucWrtTmp[3] = {0xff, SD_SPITOKEN_STOP_MULBLK, 0xff};
    INT             iError;
    __PCORE_SPI_DEV pcspidevice;

    if (!psdcoredevice) {
        _ErrorHandle(EINVAL);
        return;
    }

    pcspidevice = (__PCORE_SPI_DEV)psdcoredevice->COREDEV_pvDevHandle;
    if (!pcspidevice) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "no core spi device.\r\n");
        return;
    }

    iError = API_SpiDeviceBusRequest(pcspidevice->CSPIDEV_pspiDev); 
    if (iError == PX_ERROR) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "bus request error.\r\n");
        return;
    }
    __SD_SPI_CSEN(pcspidevice);                                    

    __sdCoreSpiByteWrt(pcspidevice, 3, pucWrtTmp);

    iError = __sdCoreSpiWaitBusy(pcspidevice);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "wait data program timeout.\r\n");
    }

    __SD_SPI_CSDIS(pcspidevice);
    API_SpiDeviceBusRelease(pcspidevice->CSPIDEV_pspiDev);
}
/*********************************************************************************************************
** 函数名称: API_SdCoreSpiSendIfCond
** 功能描述: SPI 下cmd8命令.因为该命令需要crc校验,并且应答也不同,因此不同于sd模式下的cmd8
** 输    入: psdcoredevice sd内核设备指针
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API INT  API_SdCoreSpiSendIfCond (PLW_SDCORE_DEVICE psdcoredevice)
{
    UINT8           ucBuf[6] = {
                         8 + 0x40,                                      /*  命令                        */
                         0x00, 0x00, 0x01, 0xaa,                        /*  参数                        */
                         0x87,                                          /*  crc                         */
                    };

    UINT8           ucTmp;
    INT             iError;
    INT             iRetry;

    LW_SPI_MESSAGE  spimsg;
    __PCORE_SPI_DEV pcspidev;

    struct timespec   tvOld;
    struct timespec   tvNow;

    if (!psdcoredevice) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    spimsg.SPIMSG_pfuncComplete = LW_NULL;

    pcspidev = (__PCORE_SPI_DEV)psdcoredevice->COREDEV_pvDevHandle;
    if (!pcspidev) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "no core spi device.\r\n");
        return  (PX_ERROR);
    }

    iError = API_SpiDeviceBusRequest(pcspidev->CSPIDEV_pspiDev);        /*  请求spi总线                 */
    if (iError == PX_ERROR) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "bus request error.\r\n");
        return  (PX_ERROR);
    }
    __SD_SPI_CSEN(pcspidev);                                            /*  使能片选                    */

    /*
     * 写入命令参数和crc
     */
    spimsg.SPIMSG_usBitsPerOp = __SD_SPI_BITS_PEROP;
    spimsg.SPIMSG_pucWrBuffer = ucBuf;
    spimsg.SPIMSG_pucRdBuffer = &ucTmp;
    spimsg.SPIMSG_uiLen       = 6;
    spimsg.SPIMSG_usFlag      = __SD_SPI_TMOD_WR;
    iError = API_SpiDeviceTransfer(pcspidev->CSPIDEV_pspiDev,
                                   &spimsg,
                                   1);
    if (iError == PX_ERROR) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "send cmd error.\r\n");
        goto    __spibus_release;
    }

    /*
     * 等待响应
     */
    iRetry                    = 0;
    ucTmp                     = 0xff;
    spimsg.SPIMSG_usBitsPerOp = __SD_SPI_BITS_PEROP;
    spimsg.SPIMSG_pucWrBuffer = &ucTmp;
    spimsg.SPIMSG_pucRdBuffer = ucBuf;
    spimsg.SPIMSG_uiLen       = 1;
    spimsg.SPIMSG_usFlag      = __SD_SPI_TMOD_RD;

    lib_clock_gettime(CLOCK_MONOTONIC, &tvOld);
    
    while (iRetry++ < __SD_SPI_RSP_TIMEOUT) {
        iError = API_SpiDeviceTransfer(pcspidev->CSPIDEV_pspiDev,
                                       &spimsg,
                                       1);
        if (iError == PX_ERROR) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "get response error.\r\n");
            goto    __spibus_release;
        }
        if (!(ucBuf[0] & 0x80)) {
            goto    __resp_accept;
        }
        
        lib_clock_gettime(CLOCK_MONOTONIC, &tvNow);
        if ((tvNow.tv_sec - tvOld.tv_sec) >= __SD_SPI_RSP_WAITSEC) {    /*  超时退出                    */
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "get response timeout.\r\n");
            goto    __spibus_release;
        }
    }

__resp_accept:

    ucTmp                     = 0xff;
    spimsg.SPIMSG_usBitsPerOp = __SD_SPI_BITS_PEROP;
    spimsg.SPIMSG_pucWrBuffer = &ucTmp;
    spimsg.SPIMSG_pucRdBuffer = ucBuf;
    spimsg.SPIMSG_uiLen       = 4;
    spimsg.SPIMSG_usFlag      = __SD_SPI_TMOD_RD;
    iError = API_SpiDeviceTransfer(pcspidev->CSPIDEV_pspiDev,
                                   &spimsg,
                                   1);
    if (iError == PX_ERROR) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "get response error.\r\n");
        goto    __spibus_release;
    }

    if (ucBuf[3] != 0xaa) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "check pattern error.\r\n");
        goto    __spibus_release;
    }

    __SD_SPI_CSDIS(pcspidev);                                           /*  禁能片选                    */
    API_SpiDeviceBusRelease(pcspidev->CSPIDEV_pspiDev);                 /*  释放spi总线                 */
    return  (ERROR_NONE);                                               /*  成功返回                    */

__spibus_release:
    __SD_SPI_CSDIS(pcspidev);                                           /*  禁能片选                    */
    API_SpiDeviceBusRelease(pcspidev->CSPIDEV_pspiDev);                 /*  释放spi总线                 */
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: API_SdCoreSpiRegisterRead
** 功能描述: SPI 下读取寄存器的值.与读取数据不同的是,有的sd卡设备不一定会先发送一个数据起始令牌(0xfe),而
**           是直接发送其寄存器的值,但有的设备却会发送起始令牌.但是数据的读取,设备一定会首先发送数据起始
**           令牌.
** 输    入: psdcoredevice sd内核设备指针
**           pucReg        读取结果缓冲
**           uiLen         寄存器长度
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API INT  API_SdCoreSpiRegisterRead (PLW_SDCORE_DEVICE  psdcoredevice,
                                       UINT8             *pucReg,
                                       UINT               uiLen)
{
    __PCORE_SPI_DEV   pcspidevice;
    UINT8             ucRdToken;
    INT               iRetry   = 0;
    UINT8             ucWrtClk = 0xff;
    INT               iError;
    UINT8             pucCrc16[2];

    struct timespec   tvOld;
    struct timespec   tvNow;

    if (!psdcoredevice || !pucReg) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pcspidevice = (__PCORE_SPI_DEV)psdcoredevice->COREDEV_pvDevHandle;
    if (!pcspidevice) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "no spi device error.\r\n");
        return  (PX_ERROR);

    }

    iError = API_SpiDeviceBusRequest(pcspidevice->CSPIDEV_pspiDev);     /*  请求spi总线                 */
    if (iError == PX_ERROR) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "bus request error.\r\n");
        return  (PX_ERROR);
    }
    __SD_SPI_CSEN(pcspidevice);                                         /*  使能片选                    */

    __sdCoreSpiByteWrt(pcspidevice, 1, &ucWrtClk);                      /*  sync clock                  */

    /*
     * 等待数据起始令牌.
     */
    lib_clock_gettime(CLOCK_MONOTONIC, &tvOld);
    
    while (iRetry++ < __SD_SPI_RSP_TIMEOUT) {
        __sdCoreSpiByteRd(pcspidevice, 1, &ucRdToken);
        if (ucRdToken != 0xff) {
            goto    __resp_accept;
        }
        
        lib_clock_gettime(CLOCK_MONOTONIC, &tvNow);
        if ((tvNow.tv_sec - tvOld.tv_sec) >= __SD_SPI_RSP_WAITSEC) {    /*  超时退出                    */
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "wait read start token timeout.\r\n");
            goto    __spibus_release;
        }
    }

__resp_accept:
    /*
     * 不是起始令牌, 说明该字节就是要读的寄存器的值.
     * 并且还剩下iLen - 1个字节要读取.
     */
    if (ucRdToken != SD_SPITOKEN_START_SIGBLK) {
        uiLen--;
        *pucReg++ = ucRdToken;
    }

    /*
     * 接受数据块.
     */
    iError = __sdCoreSpiByteRd(pcspidevice, uiLen, pucReg);

    if (iError == PX_ERROR) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "get data error.\r\n");
        goto    __spibus_release;
    }

    /*
     * 读取16位 CRC
     */
    iError = __sdCoreSpiByteRd(pcspidevice, 2, pucCrc16);
    if (iError == PX_ERROR) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "get data error.\r\n");
        goto    __spibus_release;
    }

#if LW_CFG_SDCARD_CRC_EN > 0
    if (ucRdToken != SD_SPITOKEN_START_SIGBLK) {
        uiLen++;
        pucReg--;                                                       /*  恢复其值以计算crc           */
    }

    if (((pucCrc16[0] << 8) | pucCrc16[1]) != __sdCrc16(pucReg, (UINT16)uiLen)) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "crc error.\r\n");
        goto    __spibus_release;
    }
#endif

    __sdCoreSpiByteWrt(pcspidevice, 1, &ucWrtClk);
    __SD_SPI_CSDIS(pcspidevice);                                        /*  禁能片选                    */
    API_SpiDeviceBusRelease(pcspidevice->CSPIDEV_pspiDev);              /*  释放spi总线                 */
    return  (ERROR_NONE);

__spibus_release:
    __sdCoreSpiByteWrt(pcspidevice, 1, &ucWrtClk);
    __SD_SPI_CSDIS(pcspidevice);                                        /*  禁能片选                    */
    API_SpiDeviceBusRelease(pcspidevice->CSPIDEV_pspiDev);              /*  释放spi总线                 */

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __sdCoreSdDevTransfer
** 功能描述: 向sD设备发送请求
** 输    入: pvDevHandle 设备句柄
**           psdmsg      请求消息组
**           iNum        消息个数
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdCoreSdDevTransfer (PVOID          pvDevHandle,
                                  PLW_SD_MESSAGE psdmsg,
                                  INT            iNum)
{
    PLW_SD_DEVICE   psddevice = (PLW_SD_DEVICE)pvDevHandle;
    INT             iError;

    iError = API_SdDeviceTransfer(psddevice, psdmsg, iNum);

    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "request failed.\r\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdCoreSdDevCtl
** 功能描述: 向sd设备发出控制
** 输    入: pvDevHandle 设备句柄
**           iCmd        控制命令
**           lArg        命令参数
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdCoreSdDevCtl (PVOID      pvDevHandle,
                             INT        iCmd,
                             LONG       lArg)
{
    PLW_SD_DEVICE   psddevice = (PLW_SD_DEVICE)pvDevHandle;
    INT             iError;

    if (!pvDevHandle) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    iError = API_SdDeviceCtl(psddevice, iCmd, lArg);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "ctrl failed.\r\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdCoreSdDevDelet
** 功能描述: 删除一个SD核心设备
** 输    入: pvDevHandle  设备句柄
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdCoreSdDevDelet (PVOID  pvDevHandle)
{
    PLW_SD_DEVICE   psddevice;
    INT             iError;

    if (!pvDevHandle) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    psddevice = (PLW_SD_DEVICE)pvDevHandle;
    iError = API_SdDeviceDelete(psddevice);

    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "delet sd device failed.\r\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdCoreSdCallbackCheckDev
** 功能描述: sd设备状态检测(旨在更新内部状态标识)
** 输    入: pvDevHandle  设备句柄
**           iDevSta      设备状态
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdCoreSdCallbackCheckDev (PVOID  pvDevHandle, INT iDevSta)
{
    PLW_SD_DEVICE   psddevice;

    if (!pvDevHandle) {
        return  (PX_ERROR);
    }

    psddevice = (PLW_SD_DEVICE)pvDevHandle;
    if (iDevSta == SD_DEVSTA_UNEXIST) {
        psddevice->SDDEV_uiState &= ~SD_STATE_EXIST;
    } else {
        psddevice->SDDEV_uiState |= SD_STATE_EXIST;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdCoreSdCallbackCheckDev
** 功能描述: sd设备状态检测(旨在更新内部状态标识)
** 输    入: pvDevHandle  设备句柄
**           iDevSta      设备状态
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdCoreCallbackCheckDev (PLW_SDCORE_DEVICE psdcoredevice, INT iDevSta)
{
    INTREG  iregInterLevel;

    if (!psdcoredevice) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    switch (psdcoredevice->COREDEV_iAdapterType) {

    case SDADAPTER_TYPE_SD:
        LW_SPIN_LOCK_QUICK(&psdcoredevice->COREDEV_slLock, &iregInterLevel);
        __sdCoreSdCallbackCheckDev(psdcoredevice->COREDEV_pvDevHandle, iDevSta);
        LW_SPIN_UNLOCK_QUICK(&psdcoredevice->COREDEV_slLock, iregInterLevel);
        break;

    case SDADAPTER_TYPE_SPI:
        LW_SPIN_LOCK_QUICK(&psdcoredevice->COREDEV_slLock, &iregInterLevel);
        __sdCoreSpiCallbackCheckDev(psdcoredevice->COREDEV_pvDevHandle, iDevSta);
        LW_SPIN_UNLOCK_QUICK(&psdcoredevice->COREDEV_slLock, iregInterLevel);
        break;

    default:
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdCoreSpiDevTransfer
** 功能描述: 向spi设备发送请求
** 输    入: pvDevHandle  设备句柄
**           psdmsg       请求消息
**           iNum         消息个数
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdCoreSpiDevTransfer (PVOID  pvDevHandle, PLW_SD_MESSAGE psdmsg, INT iNum)
{
    __PCORE_SPI_DEV  pcspidevice = (__PCORE_SPI_DEV)pvDevHandle;
    PLW_SD_COMMAND   psdcmd;
    PLW_SD_DATA      psddat;
    INT              iError;
    INT              i = 0;

    if (!pvDevHandle) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    while ((i < iNum) && (psdmsg != LW_NULL)) {
        iError = API_SpiDeviceBusRequest(pcspidevice->CSPIDEV_pspiDev); /*  请求spi总线                 */
        if (iError == PX_ERROR) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "bus request error.\r\n");
            return  (PX_ERROR);
        }
        __SD_SPI_CSEN(pcspidevice);                                     /*  使能片选                    */

        psdcmd = psdmsg->SDMSG_psdcmdCmd;
        psddat = psdmsg->SDMSG_psddata;

        /*
         * 首先发送请求命令
         */
        iError = __sdCoreSpiCmd(pcspidevice, psdcmd);
        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "send cmd error.\r\n");
            goto    __spibus_release;
        }

        /*
         * 数据处理
         */
        if (psddat != LW_NULL) {
            if (SD_DAT_IS_STREAM(psddat)) {
                SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "don't support stream operation.\r\n");
                goto    __spibus_release;
            }

            if (SD_DAT_IS_BOTHRW(psddat)) {
                SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "don't support both read and write data.\r\n");
                goto    __spibus_release;
            }

            if (SD_DAT_IS_WRITE(psddat)) {
                iError = __sdCoreSpiDataWrt(pcspidevice,
                                            psddat->SDDAT_uiBlkNum,
                                            psddat->SDDAT_uiBlkSize,
                                            psdmsg->SDMSG_pucWrtBuffer);
                if (iError != ERROR_NONE) {
                    SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "write block error.\r\n");
                    goto    __spibus_release;
                }

            } else if (SD_DAT_IS_READ(psddat)) {
                iError = __sdCoreSpiDataRd(pcspidevice,
                                           psddat->SDDAT_uiBlkNum,
                                           psddat->SDDAT_uiBlkSize,
                                           psdmsg->SDMSG_pucRdBuffer);
                if (iError != ERROR_NONE) {
                    SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "read block error.\r\n");
                    goto    __spibus_release;
                }

            } else {
                SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "unknown data flag.\r\n");
                goto    __spibus_release;
            }
        }

        /*
         * 如果有停止命令, 发送到设备
         */
        psdcmd = psdmsg->SDMSG_psdcmdStop;
        if (psdcmd) {
            iError = __sdCoreSpiCmd(pcspidevice, psdcmd);
            if (iError != ERROR_NONE) {
                SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "send stop cmd error.\r\n");
                goto    __spibus_release;
            }
        }

        i++;
        psdmsg++;
        __SD_SPI_CSDIS(pcspidevice);                                    /*  禁能片选                    */
        API_SpiDeviceBusRelease(pcspidevice->CSPIDEV_pspiDev);          /*  每一次消息结束都释放spi总线 */
    }

    return  (ERROR_NONE);

__spibus_release:
    __SD_SPI_CSDIS(pcspidevice);                                        /*  禁能片选                    */
    API_SpiDeviceBusRelease(pcspidevice->CSPIDEV_pspiDev);              /*  释放spi总线                 */
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __sdCoreSpiDevCtl
** 功能描述: 向spi设备发出控制
** 输    入: pvDevHandle  设备句柄
**           iCmd         控制命令
**           lArg         命令参数
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdCoreSpiDevCtl (PVOID  pvDevHandle, INT  iCmd, LONG lArg)
{
    __PCORE_SPI_DEV  pcspidevice = (__PCORE_SPI_DEV)pvDevHandle;
    UINT8            ucWrtBuf;
    INT              iError = ERROR_NONE;

    if (!pvDevHandle) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    switch (iCmd) {

    case SDBUS_CTRL_POWEROFF:                                           /*  关闭电源                    */
        break;

    case SDBUS_CTRL_POWERUP:
    case SDBUS_CTRL_POWERON:                                            /*  上电\开电源                 */
        break;

    case SDBUS_CTRL_SETBUSWIDTH:                                        /*  总线位宽IO设置              */
        break;

    case SDBUS_CTRL_SETCLK:                                             /*  时钟频率设置                */
        iError = API_SpiDeviceBusRequest(pcspidevice->CSPIDEV_pspiDev); 
        if (iError == PX_ERROR) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "bus request error.\r\n");
            break;
        }
        __SD_SPI_CSEN(pcspidevice);                                    

        if (lArg >= __SD_SPI_CLK_MAX) {
            lArg = __SD_SPI_CLK_MAX;
        }

        iError = API_SpiDeviceCtl(pcspidevice->CSPIDEV_pspiDev,
                                  LW_SPI_CTL_BAUDRATE,
                                  lArg);


        __SD_SPI_CSDIS(pcspidevice);                                    
        API_SpiDeviceBusRelease(pcspidevice->CSPIDEV_pspiDev);         
        break;

    case SDBUS_CTRL_DELAYCLK:                                           /*  总线时钟延时设置            */
        iError = API_SpiDeviceBusRequest(pcspidevice->CSPIDEV_pspiDev); 
        if (iError == PX_ERROR) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "bus request error.\r\n");
            break;
        }
        __SD_SPI_CSEN(pcspidevice);                                   

        while (lArg--) {
            ucWrtBuf = 0xff;
            iError = __sdCoreSpiByteWrt(pcspidevice, 1, &ucWrtBuf);     /*  延时n*8个时钟               */
            if (iError != ERROR_NONE) {
                SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "delay clock error.\r\n");
                break;
            }
        }

        __SD_SPI_CSDIS(pcspidevice);                                    
        API_SpiDeviceBusRelease(pcspidevice->CSPIDEV_pspiDev);        
        break;


    case SDBUS_CTRL_GETOCR:                                             /*  获取主控OCR                 */
        *(UINT32 *)lArg = SD_VDD_33_34;
        iError = ERROR_NONE;
        break;

    default:
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "invalidate command.\r\n");
        iError = PX_ERROR;
        break;
    }

    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __sdCoreSpiDevDelet
** 功能描述: 删除一个SPI设备
** 输    入: pvDevHandle  设备句柄
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdCoreSpiDevDelet (PVOID pvDevHandle)
{
    __PCORE_SPI_DEV  pcspidevice;
    INT              iError;

    if (!pvDevHandle) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pcspidevice = (__PCORE_SPI_DEV)pvDevHandle;

    /*
     * 首先删除SPI设备
     */
    iError = API_SpiDeviceDelete(pcspidevice->CSPIDEV_pspiDev);

    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "delet spi device failed.\r\n");
        return  (PX_ERROR);
    }

    /*
     * 再删除封装的核心SPI设备
     */
    __SHEAP_FREE(pcspidevice);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdCoreSpiCallbackCheckDev
** 功能描述: spi设备状态检测(旨在更新内部状态标识)
** 输    入: pvDevHandle  设备句柄
**           iDevSta      设备状态
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdCoreSpiCallbackCheckDev (PVOID  pvDevHandle, INT iDevSta)
{
    __PCORE_SPI_DEV  pcspidevice;

    if (!pvDevHandle) {
        return  (PX_ERROR);
    }

    pcspidevice = (__PCORE_SPI_DEV)pvDevHandle;
    pcspidevice->CSPIDEV_uiState = iDevSta;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdCoreDevSwitchToApp
** 功能描述: 切换到APP命令
** 输    入: psdcoredevice  核心结构指针
**           bIsBc        是否针对广播命令
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __sdCoreDevSwitchToApp (PLW_SDCORE_DEVICE psdcoredevice, BOOL bIsBc)
{
    LW_SD_COMMAND  sdcmd;
    INT            iError;
    UINT32         uiRca;

    if (!bIsBc) {
        iError = API_SdCoreDevRcaView(psdcoredevice, &uiRca);
        if (iError != ERROR_NONE) {
            return  (PX_ERROR);
        }
        uiRca = uiRca << 16;
    } else {
        uiRca = 0;
    }

    sdcmd.SDCMD_uiArg    = uiRca;
    sdcmd.SDCMD_uiOpcode = SD_APP_CMD;
    sdcmd.SDCMD_uiFlag   = bIsBc ?
                           (SD_RSP_SPI_R1 | SD_RSP_R1 | SD_CMD_BCR) :
                           (SD_RSP_SPI_R1 | SD_RSP_R1 | SD_CMD_AC);

    iError = API_SdCoreDevCmd(psdcoredevice, &sdcmd, 0);
    if (iError != ERROR_NONE) {
    }

    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __sdCoreSpiCmd
** 功能描述: spi设备发送命令
** 输    入: pcspidevice  spi核心设备结构指针
**           psdcmd       要发送的命令指针
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdCoreSpiCmd (__PCORE_SPI_DEV pcspidevice, LW_SD_COMMAND *psdcmd)
{
    LW_SPI_MESSAGE    spimsg;
    UINT8             ucWrtBuf[4];                                      /*  写缓冲.用于临时存放命令     */
    UINT8             ucRdBuf[6];                                       /*  读缓冲.用于临时存放应答     */
    INT               iError;
    INT               iRespLen;
    INT               iRetry = 0;

    struct timespec   tvOld;
    struct timespec   tvNow;

#if LW_CFG_SDCARD_CRC_EN > 0
    UINT8             ucCmdBackUp;                                      /*  CRC计算时,需要用命令作参数  */
#endif

    spimsg.SPIMSG_pfuncComplete = LW_NULL;

    ucWrtBuf[0] = 0xff;
    __sdCoreSpiByteWrt(pcspidevice, 1, ucWrtBuf);                       /*  8个同步时钟确保卡ready      */

    /*
     * 首先,发送命令字.
     * 把SD命令结构转换为SPI传输控制消息结构.
     */
    ucWrtBuf[0] = (UINT8)((SD_CMD_OPC(psdcmd) & 0x3f) | 0x40);

#if LW_CFG_SDCARD_CRC_EN > 0
    ucCmdBackUp = ucWrtBuf[0];
#endif

    spimsg.SPIMSG_usBitsPerOp = __SD_SPI_BITS_PEROP;
    spimsg.SPIMSG_pucWrBuffer = ucWrtBuf;
    spimsg.SPIMSG_pucRdBuffer = ucRdBuf;
    spimsg.SPIMSG_uiLen       = 1;
    spimsg.SPIMSG_usFlag      = __SD_SPI_TMOD_WR;
    iError = API_SpiDeviceTransfer(pcspidevice->CSPIDEV_pspiDev,
                                   &spimsg,
                                   1);

    if (iError == PX_ERROR) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "send cmd error.\r\n");
        goto    __error_out;
    }

    /*
     * 其次,发送参数
     */
    __sdCoreSpiParamConvert(ucWrtBuf, psdcmd->SDCMD_uiArg);

    spimsg.SPIMSG_usBitsPerOp = __SD_SPI_BITS_PEROP;
    spimsg.SPIMSG_pucWrBuffer = ucWrtBuf;
    spimsg.SPIMSG_pucRdBuffer = ucRdBuf;
    spimsg.SPIMSG_uiLen       = 4;
    spimsg.SPIMSG_usFlag      = __SD_SPI_TMOD_WR;
    iError = API_SpiDeviceTransfer(pcspidevice->CSPIDEV_pspiDev,
                                   &spimsg,
                                   1);

    if (iError == PX_ERROR) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "send argument error.\r\n");
        goto    __error_out;
    }

    /*
     * 发送最后一个字节crc
     */
#if LW_CFG_SDCARD_CRC_EN > 0
    ucWrtBuf[0] = __sdCrcCmdCrc7(ucCmdBackUp, ucWrtBuf);
#else
    ucWrtBuf[0] = 0x95;
#endif

    spimsg.SPIMSG_usBitsPerOp = __SD_SPI_BITS_PEROP;
    spimsg.SPIMSG_pucWrBuffer = ucWrtBuf;
    spimsg.SPIMSG_pucRdBuffer = ucRdBuf;
    spimsg.SPIMSG_uiLen       = 1;
    spimsg.SPIMSG_usFlag      = __SD_SPI_TMOD_WR;
    iError = API_SpiDeviceTransfer(pcspidevice->CSPIDEV_pspiDev,
                                   &spimsg,
                                   1);

    if (iError == PX_ERROR) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "send last byte error.\r\n");
        goto    __error_out;
    }

    /*
     * 等待应答.spi模式下总有应答.
     */
    iRetry                    = 0;
    ucWrtBuf[0]               = 0xff;
    spimsg.SPIMSG_usBitsPerOp = __SD_SPI_BITS_PEROP;
    spimsg.SPIMSG_pucWrBuffer = ucWrtBuf;
    spimsg.SPIMSG_pucRdBuffer = ucRdBuf;
    spimsg.SPIMSG_uiLen       = 1;
    spimsg.SPIMSG_usFlag      = __SD_SPI_TMOD_RD;
    lib_clock_gettime(CLOCK_MONOTONIC, &tvOld);
    
    while (iRetry++ < __SD_SPI_RSP_TIMEOUT) {
        iError = API_SpiDeviceTransfer(pcspidevice->CSPIDEV_pspiDev,
                                       &spimsg,
                                       1);
        if (iError == PX_ERROR) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "get response error.\r\n");
            goto    __error_out;
        }

        if (!(ucRdBuf[0] & 0x80)) {                                     /*  应答起始位以0开始           */
            goto    __resp_accept;
        }

        lib_clock_gettime(CLOCK_MONOTONIC, &tvNow);
        if ((tvNow.tv_sec - tvOld.tv_sec) >= __SD_SPI_RSP_WAITSEC) {    /*  超时退出                    */
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "response time out.\r\n");
            goto    __error_out;
        }
    }

    /*
     * 接收并处理应答
     */
__resp_accept:
    iRespLen = __sdCoreSpiRespLen(psdcmd->SDCMD_uiFlag);
    if (iRespLen <= 0) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "response length error.\r\n");
        goto    __error_out;
    }
    
    if (iRespLen > 1) {
        ucWrtBuf[0]               = 0xff;
        spimsg.SPIMSG_usBitsPerOp = __SD_SPI_BITS_PEROP;
        spimsg.SPIMSG_pucWrBuffer = ucWrtBuf;
        spimsg.SPIMSG_pucRdBuffer = ucRdBuf + 1;                        /*  之前已经接收了一个字节应答  */
        spimsg.SPIMSG_uiLen       = iRespLen - 1;
        spimsg.SPIMSG_usFlag      = __SD_SPI_TMOD_RD;
        iError = API_SpiDeviceTransfer(pcspidevice->CSPIDEV_pspiDev,
                                       &spimsg,
                                       1);
        if (iError == PX_ERROR) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "get response error.\r\n");
            goto    __error_out;
        }
    }

    __sdCoreSpiRespConvert(psdcmd->SDCMD_uiResp, ucRdBuf, iRespLen);    /*  转换为SD消息结构的应答      */

    /*
     * 对于R1B类型的应答,表示应答后设备会处于忙状态,
     * 此时设备会把数据线拉低,检测并等待.
     */
    if ((SD_RSP_SPI_MASK & psdcmd->SDCMD_uiFlag) == SD_RSP_SPI_R1B) {
        iRetry                    = 0;
        ucWrtBuf[0]               = 0xff;
        spimsg.SPIMSG_usBitsPerOp = __SD_SPI_BITS_PEROP;
        spimsg.SPIMSG_pucWrBuffer = ucWrtBuf;
        spimsg.SPIMSG_pucRdBuffer = ucRdBuf;
        spimsg.SPIMSG_uiLen       = 1;
        spimsg.SPIMSG_usFlag      = __SD_SPI_TMOD_RD;
        lib_clock_gettime(CLOCK_MONOTONIC, &tvOld);
        
        while (iRetry++ < __SD_SPI_RSP_TIMEOUT) {
            iError = API_SpiDeviceTransfer(pcspidevice->CSPIDEV_pspiDev,
                                           &spimsg,
                                           1);
            if (iError == PX_ERROR) {
                goto    __error_out;
            }

            if (ucRdBuf[0] != 0) {                                      /*  忙信号为0                   */
                break;
            }

            lib_clock_gettime(CLOCK_MONOTONIC, &tvNow);
            if ((tvNow.tv_sec - tvOld.tv_sec) >= __SD_SPI_RSP_WAITSEC) {
                goto    __error_out;
            }
        }
    }

    __SD_SPI_CSDIS(pcspidevice);
    ucWrtBuf[0] = 0xff;
    __sdCoreSpiByteWrt(pcspidevice, 1, ucWrtBuf);
    __SD_SPI_CSEN(pcspidevice);

    return  (ERROR_NONE);

__error_out:
    __SD_SPI_CSDIS(pcspidevice);
    ucWrtBuf[0] = 0xff;
    __sdCoreSpiByteWrt(pcspidevice, 1, ucWrtBuf);
    __SD_SPI_CSEN(pcspidevice);

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __sdCoreSpiDataRd
** 功能描述: spi设备读数据块
** 输    入: pcspidevice  spi核心设备结构指针
**           uiBlkNum     块数量
**           uiBlkLen     块长度
**           pucRdBuff    读缓冲
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdCoreSpiDataRd (__PCORE_SPI_DEV pcspidevice,
                              UINT32          uiBlkNum,
                              UINT32          uiBlkLen,
                              UINT8          *pucRdBuff)
{
    INT iError = ERROR_NONE;
    INT i      = 0;

    while (i++ < uiBlkNum) {
        iError = __sdCoreSpiBlkRd(pcspidevice, uiBlkLen, pucRdBuff);
        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "read a block error.\r\n");
            return  (PX_ERROR);
        }
        pucRdBuff += uiBlkLen;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdCoreSpiDataWrt
** 功能描述: spi设备写块
** 输    入: pcspidevice  spi核心设备结构指针
**           uiBlkNum     块数量
**           uiBlkLen     块长度
**           pucWrtBuff    写缓冲
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdCoreSpiDataWrt (__PCORE_SPI_DEV pcspidevice,
                               UINT32          uiBlkNum,
                               UINT32          uiBlkLen,
                               UINT8          *pucWrtBuff)
{
    INT     iError = ERROR_NONE;
    INT     i      = 0;
    BOOL    bIsMul = uiBlkNum > 1 ? LW_TRUE : LW_FALSE;

    while (i++ < uiBlkNum) {
        iError = __sdCoreSpiBlkWrt(pcspidevice, uiBlkLen, pucWrtBuff, bIsMul);
        if (iError != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "write a block error.\r\n");

            /*
             * 如果是多块,则终止SPI多块传输状态
             */
            if (bIsMul) {
                __sdCoreSpiMulWrtStop(pcspidevice);
            }

            return  (PX_ERROR);
        }
        pucWrtBuff += uiBlkLen;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdCoreSpiBlkRd
** 功能描述: 从spi块设备读取一个块数据
** 输    入: pcspidevice  spi核心设备结构指针
**           uiBlkLen     块长度
**           pucRdBuff    读缓冲
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdCoreSpiBlkRd (__PCORE_SPI_DEV pcspidevice,
                             UINT32          uiBlkLen,
                             UINT8          *pucRdBuff)
{
    UINT8             ucRdToken;
    struct timespec   tvOld;
    struct timespec   tvNow;
    INT               iRetry = 0;
    INT               iError;
    UINT8             pucCrc16[2];

    /*
     * 等待数据起始令牌.
     */
    lib_clock_gettime(CLOCK_MONOTONIC, &tvOld);
    
    while (iRetry++ < __SD_SPI_RSP_TIMEOUT) {
        __sdCoreSpiByteRd(pcspidevice, 1, &ucRdToken);
        if (ucRdToken == SD_SPITOKEN_START_SIGBLK) {
            goto    __resp_accept;
        }

        lib_clock_gettime(CLOCK_MONOTONIC, &tvNow);
        if ((tvNow.tv_sec - tvOld.tv_sec) >= __SD_SPI_RSP_WAITSEC) {    /*  超时退出                    */
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "wait read token timeout.\r\n");
            goto    __error_out;
        }
    }

__resp_accept:

    /*
     * 接受数据块.
     */
    iError = __sdCoreSpiByteRd(pcspidevice, uiBlkLen, pucRdBuff);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "get data error.\r\n");
        goto    __error_out;
    }

    /*
     * 读取16位 CRC
     */
    iError = __sdCoreSpiByteRd(pcspidevice, 2, pucCrc16);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "get crc16 error.\r\n");
        goto    __error_out;
    }

#if LW_CFG_SDCARD_CRC_EN > 0
    if (((pucCrc16[0] << 8) | pucCrc16[1]) != __sdCrc16(pucRdBuff, (UINT16)uiBlkLen)) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "crc error.\r\n");
        goto    __error_out;
    }

#endif

    __SD_SPI_CSDIS(pcspidevice);
    pucCrc16[0] = 0xff;
    __sdCoreSpiByteWrt(pcspidevice, 1, pucCrc16);
    __SD_SPI_CSEN(pcspidevice);
    return  (ERROR_NONE);

__error_out:
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __sdCoreSpiBlkWrt
** 功能描述: 往spi块设备写入一个块数据
** 输    入: pcspidevice  spi核心设备结构指针
**           uiBlkLen     块长度
**           pucRdBuff    写入缓冲
**           bIsMul       是否是多块写(单块写与多块写的操作不同)
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdCoreSpiBlkWrt (__PCORE_SPI_DEV pcspidevice,
                              UINT32          uiBlkLen,
                              UINT8          *pucWrtBuff,
                              BOOL            bIsMul)
{
    UINT8             ucWrtToken;
    UINT8             ucWrtClk = 0xff;
    INT               iError;
    INT               iRetry;

    struct timespec   tvOld;
    struct timespec   tvNow;

#if LW_CFG_SDCARD_CRC_EN > 0
    UINT16            usCrc16;
    UINT8             ucCrc16[2];
#endif

    /*
     * 开始发送之前发送同步时钟
     */
    __sdCoreSpiByteWrt(pcspidevice, 1, &ucWrtClk);

    if (bIsMul) {
        ucWrtToken = SD_SPITOKEN_START_MULBLK;
    } else {
        ucWrtToken = SD_SPITOKEN_START_SIGBLK;
    }

    iError = __sdCoreSpiByteWrt(pcspidevice, 1, &ucWrtToken);           /*  发送写块开始令牌            */
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "send write token error.\r\n");
        return  (PX_ERROR);
    }

    iError = __sdCoreSpiByteWrt(pcspidevice, uiBlkLen, pucWrtBuff);     /*  发送数据                    */
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "write data error.\r\n");
        return  (PX_ERROR);
    }

#if LW_CFG_SDCARD_CRC_EN > 0
    usCrc16 = __sdCrc16(pucWrtBuff, (UINT16)uiBlkLen);
    ucCrc16[0] = (usCrc16 >> 8) & 0xff;
    ucCrc16[1] =  usCrc16 & 0xff;
    __sdCoreSpiByteWrt(pcspidevice, 2, ucCrc16);                        /*  发送crc16                   */
#else
    __sdCoreSpiByteWrt(pcspidevice, 1, &ucWrtToken);
    __sdCoreSpiByteWrt(pcspidevice, 1, &ucWrtToken);                    /*  发送任意的16位数据          */
#endif

    /*
     * SPI模式下,设备对每一个请求写入的块都会回送一个响应令牌.
     * 查看设备对写入数据的响应情况.
     */
    lib_clock_gettime(CLOCK_MONOTONIC, &tvOld);
    
    iRetry = 0;
    while (iRetry++ < __SD_SPI_RSP_TIMEOUT) {
        __sdCoreSpiByteRd(pcspidevice, 1, &ucWrtToken);
        if ((!(ucWrtToken & 0x10)) && ((ucWrtToken & 0x01))) {          /*  应答令牌                    */

            if ((ucWrtToken & SD_SPITOKEN_DATRSP_MASK) != SD_SPITOKEN_DATRSP_DATACCEPT) {
                return  (PX_ERROR);

            } else {
                break;
            }
        }

        lib_clock_gettime(CLOCK_MONOTONIC, &tvNow);
        if ((tvNow.tv_sec - tvOld.tv_sec) >= __SD_SPI_RSP_WAITSEC) {    /*  超时退出                    */
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "wait write token timeout.\r\n");
            return  (PX_ERROR);
        }
    }

    if (iRetry >= __SD_SPI_RSP_TIMEOUT) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "wait write token timeout(retry timeout).\r\n");
        return  (PX_ERROR);
    }


    /*
     * 等待数据编程结束
     */
    iError = __sdCoreSpiWaitBusy(pcspidevice);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "wait data program timeout.\r\n");
    }

    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __sdCoreSpiByteRd
** 功能描述: spi设备读字节
** 输    入: pcspidevice  spi核心设备结构指针
**           uiLen        读字节长度
**           pucRdBuff     读缓冲
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdCoreSpiByteRd (__PCORE_SPI_DEV pcspidevice, UINT32 uiLen, UINT8 *pucRdBuff)
{
    INT             iError;
    LW_SPI_MESSAGE  spimsg;
    UINT8           ucWrtTmp  = 0xff;

    spimsg.SPIMSG_pfuncComplete = LW_NULL;
    spimsg.SPIMSG_usBitsPerOp   = __SD_SPI_BITS_PEROP;
    spimsg.SPIMSG_pucWrBuffer   = &ucWrtTmp;
    spimsg.SPIMSG_pucRdBuffer   = pucRdBuff;
    spimsg.SPIMSG_uiLen         = uiLen;
    spimsg.SPIMSG_usFlag        = __SD_SPI_TMOD_RD;
    iError = API_SpiDeviceTransfer(pcspidevice->CSPIDEV_pspiDev,
                                   &spimsg,
                                   1);
    if (iError == PX_ERROR) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "spi transfer error.\r\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdCoreSpiByteWrt
** 功能描述: spi设备写字节
** 输    入: pcspidevice  spi核心设备结构指针
**           uiLen        写字节长度
**           pucWrtBuff    写缓冲
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdCoreSpiByteWrt (__PCORE_SPI_DEV pcspidevice, UINT32 uiLen, UINT8 *pucWrtBuff)
{
    INT             iError;
    LW_SPI_MESSAGE  spimsg;
    UINT8           ucRdTmp;

    spimsg.SPIMSG_pfuncComplete = LW_NULL;
    spimsg.SPIMSG_usBitsPerOp   = __SD_SPI_BITS_PEROP;
    spimsg.SPIMSG_pucWrBuffer   = pucWrtBuff;
    spimsg.SPIMSG_pucRdBuffer   = &ucRdTmp;
    spimsg.SPIMSG_uiLen         = uiLen;
    spimsg.SPIMSG_usFlag        = __SD_SPI_TMOD_WR;
    iError = API_SpiDeviceTransfer(pcspidevice->CSPIDEV_pspiDev,
                                   &spimsg,
                                   1);
    if (iError == PX_ERROR) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "spi transfer error.\r\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdCoreSpiWaitBusy
** 功能描述: spi忙等待
** 输    入: pcspidevice  spi核心设备结构指针
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdCoreSpiWaitBusy (__PCORE_SPI_DEV pcspidevice)
{
    struct timespec   tvOld;
    struct timespec   tvNow;

    UINT8             ucDatBsy;
    INT               iError;
    INT               iRetry = 0;

    lib_clock_gettime(CLOCK_MONOTONIC, &tvOld);
    
    while (iRetry++ < __SD_SPI_RSP_TIMEOUT) {
        iError = __sdCoreSpiByteRd(pcspidevice, 1, &ucDatBsy);
        if (iError != ERROR_NONE) {
            return  (PX_ERROR);
        }

        if (ucDatBsy == 0xff) {
            return  (ERROR_NONE);                                       /*  数据编程结束                */
        }

        lib_clock_gettime(CLOCK_MONOTONIC, &tvNow);
        if ((tvNow.tv_sec - tvOld.tv_sec) >= __SD_SPI_RSP_WAITSEC) {    /*  超时退出                    */
            break;
        }
    }

    /*
     * TIME OUT
     */
    SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "programing data timeout.\r\n");
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __sdCoreSpiMulWrtStop
** 功能描述: spi多块写停止
** 输    入: pcspidevice  spi核心设备结构指针
** 输    出: NONE
** 返    回: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID __sdCoreSpiMulWrtStop (__PCORE_SPI_DEV pcspidevice)
{
    UINT8   pucWrtTmp[3] = {0xff, SD_SPITOKEN_STOP_MULBLK, 0xff};
    INT     iError;

    __sdCoreSpiByteWrt(pcspidevice, 3, pucWrtTmp);
    iError = __sdCoreSpiWaitBusy(pcspidevice);
    if (iError != ERROR_NONE) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "stop to wait data program timeout.\r\n");
    }
}
/*********************************************************************************************************
** 函数名称: __sdCoreSpiParamConvert
** 功能描述: SPI 参数转换
** 输    入: pucParam     保存转换后的参数
**           uiParam      上层传入的原始参数
** 输    出: NONE
** 返    回: 成功,返回设备设备指针,否则返回LW_NULL
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __sdCoreSpiParamConvert (UINT8 *pucParam, UINT32 uiParam)
{
    pucParam[0] = (UINT8)(uiParam >> 24);
    pucParam[1] = (UINT8)(uiParam >> 16);
    pucParam[2] = (UINT8)(uiParam >> 8);
    pucParam[3] = (UINT8)(uiParam);
}
/*********************************************************************************************************
** 函数名称: __sdCoreSpiRespConvert
** 功能描述: SPI 应答转换
** 输    入: puiResp     保存转换后的应答
**           pucResp     原始应答数据缓冲
**           iRespLen    应答的长度
** 输    出: NONE
** 返    回: 成功,返回设备设备指针,否则返回LW_NULL
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __sdCoreSpiRespConvert (UINT32 *puiResp, const UINT8 *pucResp, INT iRespLen)
{
    switch (iRespLen) {

    case 1:
        *puiResp = *pucResp;
        break;

    case 2:
        *puiResp = *pucResp++;
        *puiResp = (*puiResp << 8) | *pucResp;
        break;

    case 5:
        *puiResp++ = *pucResp++;                                        /*  resp[0]                     */
        *puiResp   = *pucResp++;                                        /*  resp[1]                     */
        *puiResp   = (*puiResp << 8) | *pucResp++;
        *puiResp   = (*puiResp << 8) | *pucResp++;
        *puiResp   = (*puiResp << 8) | *pucResp;
        break;

    default:
        return;
    }
}
/*********************************************************************************************************
** 函数名称: __sdCoreSpiRespLen
** 功能描述: 根据命令标记判断 SPI 应答的长度
** 输    入: uiCmdFlg     命令标志
** 输    出: NONE
** 返    回: 成功,返回设备设备指针,否则返回LW_NULL
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __sdCoreSpiRespLen (UINT32   uiCmdFlg)
{
    INT    iRspLen = 0;

    switch (SD_RSP_SPI_MASK & uiCmdFlg) {

    case SD_RSP_SPI_R1:
    case SD_RSP_SPI_R1B:
        iRspLen = 1;
        break;

    case SD_RSP_SPI_R2:                                                 /*  same as SD_RSP_SPI_R5:      */
        iRspLen = 2;
        break;

    case SD_RSP_SPI_R3:                                                 /*  same as SD_RSP_SPI_R4:      */
                                                                        /*  same as SD_RSP_SPI_R7:      */
        iRspLen = 5;
        break;

    default:
        iRspLen = -1;
        break;
    }

    return  (iRspLen);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_SDCARD_EN > 0)      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
