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
** 文   件   名: sdiobaseDrv.c
**
** 创   建   人: Zeng.Bo (曾波)
**
** 文件创建日期: 2014 年 10 月 26 日
**
** 描        述: sdio基础驱动源文件

** BUG:
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SDCARD_EN > 0) && (LW_CFG_SDCARD_SDIO_EN > 0)
#include "../core/sddrvm.h"
#include "../core/sdiodrvm.h"
#include "../core/sdcoreLib.h"
#include "../core/sdiocoreLib.h"
#include "../include/sddebug.h"
#include "sdiobaseDrv.h"
/*********************************************************************************************************
  sdiobase 私有数据
*********************************************************************************************************/
struct __sdm_sdio_base {
    SDIO_INIT_DATA  SDMIOBASE_initdata;
    SDIO_DRV       *SDMIOBASE_psdiodrv;
    VOID           *SDMIOBASE_pvDevPriv;
};
typedef struct __sdm_sdio_base  __SDM_SDIO_BASE;
/*********************************************************************************************************
  前置声明
*********************************************************************************************************/
static INT    __sdiobaseDevCreate(SD_DRV *psddrv, PLW_SDCORE_DEVICE psdcoredev, VOID **ppvDevPriv);
static INT    __sdiobaseDevDelete(SD_DRV *psddrv, VOID *pvDevPriv);

PLW_LIST_LINE __sdmSdioDrvHeader(VOID);
VOID          __sdmSdioDrvAccessRequest(VOID);
VOID          __sdmSdioDrvAccessRelease(VOID);

static INT    __sdiobaseDrvMatch(SDIO_FUNC *psdiofuncTbl, INT iFuncCnt, SDIO_DRV *psdiodrv);
static VOID   __sdiobaseMatchFuncIdSet(SDIO_INIT_DATA *psdioinitdata,
                                       SDIO_FUNC      *psdiofuncTbl,
                                       INT             iFuncCnt,
                                       SDIO_DRV       *psdiodrv);
static INT    __sdiobaseDrvMatchOne(SDIO_FUNC *psdiofunc, SDIO_DEV_ID *psdiodevid);

static INT    __sdiobasePreInit(SDIO_INIT_DATA *pinitdata, PLW_SDCORE_DEVICE psdcoredev);
static INT    __sdiobaseCommonInit(SDIO_INIT_DATA *pinitdata, UINT32 uiRealOcr);
static INT    __sdiobaseFuncInit(PLW_SDCORE_DEVICE psdcoredev, SDIO_FUNC *psdiofunc);
/*********************************************************************************************************
  sdio base 驱动对象
*********************************************************************************************************/
static SD_DRV _G_sddrvSdioBase;
/*********************************************************************************************************
** 函数名称: API_SdioBaseDrvInstall
** 功能描述: 安装SDIO Base驱动
** 输    入: NONE
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API INT API_SdioBaseDrvInstall (VOID)
{
    _G_sddrvSdioBase.SDDRV_cpcName        = SDDRV_SDIOB_NAME;
    _G_sddrvSdioBase.SDDRV_pfuncDevCreate = __sdiobaseDevCreate;
    _G_sddrvSdioBase.SDDRV_pfuncDevDelete = __sdiobaseDevDelete;

    API_SdmSdDrvRegister(&_G_sddrvSdioBase);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdiobaseDevCreate
** 功能描述: SDIO Base设备创建 方法
** 输    入: psddrv       sd 驱动
**           psdcoredev   sd 核心传输对象
**           ppvDevPriv   用于保存设备创建成功后的设备私有数据
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT   __sdiobaseDevCreate (SD_DRV *psddrv, PLW_SDCORE_DEVICE psdcoredev, VOID **ppvDevPriv)
{
    __SDM_SDIO_BASE   *psdiobase;
    SDIO_INIT_DATA    *psdioinitdata;
    SDIO_DRV          *psdiodrv;
    PLW_LIST_LINE      plineTmp;
    INT                iRet;

    psdiobase= (__SDM_SDIO_BASE *)__SHEAP_ALLOC(sizeof(__SDM_SDIO_BASE));
    if (!psdiobase) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        return  (PX_ERROR);
    }

    psdioinitdata = &psdiobase->SDMIOBASE_initdata;
    *ppvDevPriv   = (VOID *)psdiobase;

    iRet = __sdiobasePreInit(psdioinitdata, psdcoredev);
    if (iRet != ERROR_NONE) {
        /*
         * 如果预初始化失败, 说明不是一个 SDIO 设备, 无需进行
         * 下面的 SDIO 设备驱动匹配工作
         */
        goto    __err;
    }

    __sdmSdioDrvAccessRequest();
    for (plineTmp  = __sdmSdioDrvHeader();
         plineTmp != LW_NULL;
         plineTmp  = _list_line_get_next(plineTmp)) {

        psdiodrv = _LIST_ENTRY(plineTmp, SDIO_DRV, SDIODRV_lineManage);
        iRet = __sdiobaseDrvMatch(&psdioinitdata->INIT_psdiofuncTbl[0],
                                  psdioinitdata->INIT_iFuncCnt + 1,
                                  psdiodrv);
        if (iRet != ERROR_NONE) {
            /*
             * 当前驱动匹配失败,匹配下一个
             */
            continue;
        }

        __sdiobaseMatchFuncIdSet(psdioinitdata,
                                 &psdioinitdata->INIT_psdiofuncTbl[0],
                                 psdioinitdata->INIT_iFuncCnt + 1,
                                 psdiodrv);

        /*
         * 因为在接下来的具体驱动设备创建过程中可能会产生中断事件
         * 这里提前完成处理中断时需要的数据
         */
        psdiobase->SDMIOBASE_psdiodrv = psdiodrv;

        iRet = psdiodrv->SDIODRV_pfuncDevCreate(psdiodrv,
                                                psdioinitdata,
                                                &psdiobase->SDMIOBASE_pvDevPriv);
        if (iRet == ERROR_NONE) {
            API_AtomicInc(&psdiodrv->SDIODRV_atomicDevCnt);
            __sdmSdioDrvAccessRelease();
            return  (ERROR_NONE);

        } else {
            /*
             * 这里不返回错误,因为可能存在另一个 ID 匹配的驱动
             * 这样完全取决于应用驱动的编写者
             */
        }
    }
    __sdmSdioDrvAccessRelease();

__err:
    __SHEAP_FREE(psdiobase);

    *ppvDevPriv = LW_NULL;

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __sdiobaseDevDelete
** 功能描述: SDIO Base设备删除 方法
** 输    入: psddrv       sd 驱动
**           pvDevPriv    设备私有数据
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __sdiobaseDevDelete (SD_DRV *psddrv,  VOID *pvDevPriv)
{
    __SDM_SDIO_BASE   *psdiobase = (__SDM_SDIO_BASE *)pvDevPriv;
    SDIO_DRV          *psdioDrv;
    SDIO_FUNC         *psdiofunc;
    INT                i;

    if (!psdiobase) {
        return  (PX_ERROR);
    }

    psdioDrv = psdiobase->SDMIOBASE_psdiodrv;
    psdioDrv->SDIODRV_pfuncDevDelete(psdioDrv, psdiobase->SDMIOBASE_pvDevPriv);

    psdiofunc = &psdiobase->SDMIOBASE_initdata.INIT_psdiofuncTbl[0];
    for (i = 0; i < (psdiobase->SDMIOBASE_initdata.INIT_iFuncCnt + 1); i++) {
        API_SdioCoreDevFuncClean(psdiofunc);
        psdiofunc++;
    }

    __SHEAP_FREE(psdiobase);

    API_AtomicDec(&psdioDrv->SDIODRV_atomicDevCnt);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdiobaseDevIrqHandle
** 功能描述: SDIO Base中断处理(该函数仅仅SDM模块可见)
** 输    入: psddrv       sd 驱动
**           pvDevPriv    设备私有数据
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  __sdiobaseDevIrqHandle (SD_DRV *psddrv,  VOID *pvDevPriv)
{
    __SDM_SDIO_BASE   *psdiobase = (__SDM_SDIO_BASE *)pvDevPriv;
    SDIO_DRV          *psdiodrv;

    if (!psdiobase) {
        return  (PX_ERROR);
    }

    psdiodrv = psdiobase->SDMIOBASE_psdiodrv;
    psdiodrv->SDIODRV_pfuncIrqHandle(psdiodrv, psdiobase->SDMIOBASE_pvDevPriv);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdiobasePreInit
** 功能描述: SDIO 设备预初始化(获得SDIO设备共同的信息)
** 输    入: pinitdata        保存初始化的信息
**           psdcoredev       核心设备传输对象
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __sdiobasePreInit (SDIO_INIT_DATA *pinitdata, PLW_SDCORE_DEVICE psdcoredev)
{
    UINT32      uiOcr     = 0;
    UINT32      uiHostOcr = 0;
    SDIO_FUNC  *psdiofunc;
    INT         iRet;
    INT         i;

    if (!pinitdata || !psdcoredev) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        return  (PX_ERROR);
    }

    /*
     * first init the data
     */
    lib_bzero(pinitdata, sizeof(SDIO_INIT_DATA));

    psdiofunc = &pinitdata->INIT_psdiofuncTbl[0];
    for (i = 0; i < 8; i++) {
        psdiofunc->FUNC_uiNum            = i;
        psdiofunc->FUNC_cpsdiocccr       = &pinitdata->INIT_sdiocccr;
        psdiofunc->FUNC_ptupleListHeader = LW_NULL;

        psdiofunc++;
    }

    pinitdata->INIT_psdcoredev = psdcoredev;

    /*
     * do common init
     */
    API_SdCoreDevCtl(psdcoredev, SDBUS_CTRL_POWEROFF, 0);
    bspDelayUs(100000);
    API_SdCoreDevCtl(psdcoredev, SDBUS_CTRL_POWERON, 0);
    API_SdCoreDevCtl(psdcoredev, SDBUS_CTRL_SETCLK, SDARG_SETCLK_LOW);
    API_SdCoreDevCtl(psdcoredev, SDBUS_CTRL_SETBUSWIDTH, SDARG_SETBUSWIDTH_1);

    API_SdioCoreDevReset(psdcoredev);
    API_SdCoreDevReset(psdcoredev);

    iRet = API_SdioCoreDevSendIoOpCond(psdcoredev, 0, &uiOcr);      /*  获取设备支持的OCR               */
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    uiOcr &= ~0x7f;                                                 /*  这里忽略协议里保留的电压范围    */

    pinitdata->INIT_iFuncCnt = (uiOcr & 0x70000000) >> 28;

    API_SdCoreDevCtl(psdcoredev,
                     SDBUS_CTRL_GETOCR,
                     (LONG)&uiHostOcr);

    uiOcr = uiOcr & uiHostOcr;
    if (uiOcr == 0) {
        SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "dev ocr not match with host ocr.\r\n");
        return  (PX_ERROR);
    }

    iRet = __sdiobaseCommonInit(pinitdata, uiOcr);
    if (iRet != ERROR_NONE) {
        goto    __clean;
    }

    /*
     * TODO: disable card cd
     */

    /*
     * then init func 1~7
     */
    psdiofunc = &pinitdata->INIT_psdiofuncTbl[1];
    for (i = 0; i < pinitdata->INIT_iFuncCnt; i++) {
        iRet = __sdiobaseFuncInit(psdcoredev, psdiofunc);
        if (iRet != ERROR_NONE) {
            goto    __clean;
        }
        psdiofunc++;
    }

    return  (ERROR_NONE);

__clean:
    psdiofunc = &pinitdata->INIT_psdiofuncTbl[0];
    for (i = 0; i < (pinitdata->INIT_iFuncCnt + 1); i++) {
        API_SdioCoreDevFuncClean(psdiofunc);
        psdiofunc++;
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __sdiobaseDoInit
** 功能描述: sdio 一般初始化
** 输    入: pinitdata        保存初始化数据
**           uiRealOcr        实际使用的电压支持信息
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT   __sdiobaseCommonInit (SDIO_INIT_DATA *pinitdata, UINT32 uiRealOcr)
{
    PLW_SDCORE_DEVICE psdcoredev = pinitdata->INIT_psdcoredev;
    INT               iRet;
    UINT32            uiRca;

    iRet = API_SdioCoreDevSendIoOpCond(psdcoredev, uiRealOcr, LW_NULL);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

#if LW_CFG_SDCARD_CRC_EN > 0
    if (COREDEV_IS_SPI(psdcoredev)) {
        API_SdCoreDevSpiCrcEn(psdcoredev, LW_TRUE);
    }
#endif

    API_SdCoreDevTypeSet(psdcoredev, SDDEV_TYPE_SDIO);

    if (COREDEV_IS_SD(psdcoredev)) {
        iRet = API_SdCoreDevSendRelativeAddr(psdcoredev, &uiRca);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }

        API_SdCoreDevRcaSet(psdcoredev, uiRca);

        iRet = API_SdCoreDevSelect(psdcoredev);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
    }

    iRet = API_SdioCoreDevReadCCCR(psdcoredev, &pinitdata->INIT_sdiocccr);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    /*
     * func0 : the common CIS
     */
    iRet = API_SdioCoreDevReadCis(psdcoredev, &pinitdata->INIT_psdiofuncTbl[0]);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    /*
     * try to switch to high speed
     */
    API_SdioCoreDevHighSpeedEn(psdcoredev, &pinitdata->INIT_sdiocccr);

    if (COREDEV_IS_HIGHSPEED(psdcoredev)) {
        iRet = API_SdCoreDevCtl(psdcoredev,
                                SDBUS_CTRL_SETCLK,
                                SDARG_SETCLK_MAX);
        if (iRet != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "host clock set to max failed.\r\n");
        }
    } else {
        iRet = API_SdCoreDevCtl(psdcoredev,
                                SDBUS_CTRL_SETCLK,
                                pinitdata->INIT_psdiofuncTbl[0].FUNC_uiMaxDtr);
        if (iRet != ERROR_NONE) {
            SDCARD_DEBUG_MSG(__ERRORMESSAGE_LEVEL, "host clock set to normal failed.\r\n");
        }
    }

    iRet = API_SdioCoreDevWideBusEn(psdcoredev, &pinitdata->INIT_sdiocccr);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdiobaseFuncInit
** 功能描述: 初始化一个 SDIO 设备的指定功能
** 输    入: psdcoredev       核心设备传输对象
**           psdiofunc        需要初始化的功能
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __sdiobaseFuncInit (PLW_SDCORE_DEVICE psdcoredev, SDIO_FUNC *psdiofunc)
{
    INT iRet;

    iRet = API_SdioCoreDevReadFbr(psdcoredev, psdiofunc);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    iRet = API_SdioCoreDevReadCis(psdcoredev, psdiofunc);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdiobaseDrvMatchDev
** 功能描述: sdio 设备驱动匹配.(只要某一个功能与某一个 ID 匹配上就成功)
** 输    入: psdiofuncTbl     设备的功能描述表
**           iFuncCnt         设备的功能个数
**           psdiodrv         设备对应的驱动
** 输    出: ERROR CODE(匹配成功返回 ERROR_NONE, 否则 PX_ERROR)
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __sdiobaseDrvMatch (SDIO_FUNC *psdiofuncTbl, INT iFuncCnt, SDIO_DRV *psdiodrv)
{
    SDIO_FUNC    *psdiofunc;
    SDIO_DEV_ID  *psdiodevid;
    INT           iIdCnt;
    INT           iRet;

    INT           iFunc;
    INT           iId;

    psdiofunc = psdiofuncTbl;
    iIdCnt    = psdiodrv->SDIODRV_iDevidCnt;
    for (iFunc = 0; iFunc < iFuncCnt; iFunc++) {

        psdiodevid = psdiodrv->SDIODRV_pdevidTbl;
        for (iId = 0; iId < iIdCnt; iId++) {
            iRet = __sdiobaseDrvMatchOne(psdiofunc, psdiodevid);
            if (iRet == ERROR_NONE) {
                return  (ERROR_NONE);
            }
            psdiodevid++;
        }

        psdiofunc++;
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __sdiobaseMatchFuncIdSet
** 功能描述: 设置匹配的功能 ID
** 输    入: psdioinitdata    SDIO 初始化数据
**           psdiofuncTbl     设备的功能描述表
**           iFuncCnt         设备的功能个数
**           psdiodrv         设备对应的驱动
** 输    出: NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID __sdiobaseMatchFuncIdSet (SDIO_INIT_DATA *psdioinitdata,
                                      SDIO_FUNC      *psdiofuncTbl,
                                      INT             iFuncCnt,
                                      SDIO_DRV       *psdiodrv)
{
    SDIO_FUNC    *psdiofunc;
    SDIO_DEV_ID  *psdiodevid;
    INT           iIdCnt;
    INT           iRet;

    INT           iFunc;
    INT           iId;

    psdiofunc = psdiofuncTbl;
    iIdCnt    = psdiodrv->SDIODRV_iDevidCnt;
    for (iFunc = 0; iFunc < iFuncCnt; iFunc++) {

        psdiodevid = psdiodrv->SDIODRV_pdevidTbl;
        for (iId = 0; iId < iIdCnt; iId++) {
            iRet = __sdiobaseDrvMatchOne(psdiofunc, psdiodevid);
            if (iRet == ERROR_NONE) {
                psdioinitdata->INIT_pdevidCurr[iFunc] = psdiodevid;
                break;
            }
            psdiodevid++;
        }

        psdiofunc++;
    }
}
/*********************************************************************************************************
** 函数名称: __sdiobaseDrvMatchOne
** 功能描述: 一个功能与一个ID的匹配
** 输    入: psdiofunc        sdio 功能描述对象
**           psdiodevid       设备 ID
** 输    出: ERROR CODE(匹配成功返回 ERROR_NONE, 否则 PX_ERROR)
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdiobaseDrvMatchOne (SDIO_FUNC *psdiofunc, SDIO_DEV_ID *psdiodevid)
{
    if ((psdiodevid->DEVID_ucClass != (UINT8)SDIO_DEV_ID_ANY) &&
        (psdiodevid->DEVID_ucClass != psdiofunc->FUNC_ucClass)) {
        return  (PX_ERROR);
    }

    if ((psdiodevid->DEVID_usVendor != (UINT8)SDIO_DEV_ID_ANY) &&
        (psdiodevid->DEVID_usVendor != psdiofunc->FUNC_usVendor)) {
        return  (PX_ERROR);
    }
    if ((psdiodevid->DEVID_usDevice != (UINT8)SDIO_DEV_ID_ANY) &&
        (psdiodevid->DEVID_usDevice != psdiofunc->FUNC_usDevice)) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_SDCARD_EN > 0)      */
                                                                        /*  (LW_CFG_SDCARD_SDIO_EN > 0) */
/*********************************************************************************************************
  END
*********************************************************************************************************/
