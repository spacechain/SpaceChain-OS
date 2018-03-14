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
** 文   件   名: pciAuto.c
**
** 创   建   人: Gong.YuJian (弓羽箭)
**
** 文件创建日期: 2016 年 10 月 21 日
**
** 描        述: PCI 总线自动配置.
**
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PCI_EN > 0)
#include "pciIds.h"
#include "pciLib.h"
#include "pciAuto.h"
/*********************************************************************************************************
** 函数名称: __pciAutoDevSkip
** 功能描述: 忽略起始桥设备
** 输　入  : hCtrl      控制器句柄
**           hAutoDev   设备句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __pciAutoDevSkip (PCI_CTRL_HANDLE  hCtrl, PCI_AUTO_DEV_HANDLE  hAutoDev)
{
    PCI_AUTO_HANDLE     hPciAuto;                                       /* 自动配置句柄                 */

    hPciAuto = &hCtrl->PCI_tAutoConfig;                                 /* 获取自动配置句柄             */
                                                                        /* 是否为起始设备               */
    if (hAutoDev == PCI_AUTO_BDF(hPciAuto->PCIAUTO_uiFirstBusNo, 0, 0)) {
        if (hPciAuto->PCIAUTO_iHostBridegCfgEn) {                       /* 配置主桥使能                 */
            return  (PX_ERROR);
        
        } else {                                                        /* 不配置主桥                   */
            PCI_AUTO_LOG(PCI_AUTO_LOG_PRT,
                         "%02x:%02x.%01x dev skip.\n",
                         PCI_AUTO_BUS(hAutoDev), PCI_AUTO_DEV(hAutoDev), PCI_AUTO_FUNC(hAutoDev));
            return  (ERROR_NONE);                                       /* 忽略设备                     */
        }
    }

    return  (PX_ERROR);                                                 /* 不忽略设备                   */
}
/*********************************************************************************************************
** 函数名称: API_PciAutoBusReset
** 功能描述: 复位总线上所有设备
** 输　入  : hCtrl      控制器句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciAutoBusReset (PCI_CTRL_HANDLE  hCtrl)
{
    PCI_AUTO_DEV_HANDLE     hAutoDev;                                   /* 自动配置设备句柄             */
    INT                     iBus = 0;                                   /* 总线号                       */
    INT                     iDev = 0;                                   /* 设备号                       */
    INT                     iFunc = 0;                                  /* 功能号                       */
    UINT32                  uiMulti;                                    /* 多功能标识                   */
    UINT16                  usVendor;                                   /* 厂商信息                     */
    UINT16                  usClass;                                    /* 类信息                       */
    UINT8                   ucHeaderType;                               /* 设备头类型                   */

    uiMulti = 0;

    for (iBus = 0; iBus < PCI_MAX_BUS; iBus++) {                        /* 遍历所有总线                 */
        for (hAutoDev  = PCI_AUTO_BDF(iBus, 0, 0);
             hAutoDev <= PCI_AUTO_BDF(iBus, PCI_MAX_SLOTS - 1, PCI_MAX_FUNCTIONS - 1);
             hAutoDev += PCI_AUTO_BDF(0, 0, 1)) {                       /* 遍历设备与功能               */
            iDev  = PCI_AUTO_DEV(hAutoDev);
            iFunc = PCI_AUTO_FUNC(hAutoDev);

            PCI_AUTO_LOG(PCI_AUTO_LOG_PRT, "dev reset %02x:%02x.%01x.\n", iBus, iDev, iFunc);

            if ((PCI_AUTO_FUNC(hAutoDev)) && (!uiMulti)) {              /* 单功能设备                   */
                continue;
            }

            API_PciConfigInByte(iBus, iDev, iFunc, PCI_HEADER_TYPE, &ucHeaderType);
            API_PciConfigInWord(iBus, iDev, iFunc, PCI_VENDOR_ID, &usVendor);
            if ((usVendor == 0xffff) ||
                (usVendor == 0x0000)) {                                 /* 设备无效                     */
                continue;
            }

            if (!PCI_AUTO_FUNC(hAutoDev)) {
                uiMulti = ucHeaderType & PCI_HEADER_MULTI_FUNC;
            }

            /*
             *  复位所有设备
             */
            API_PciConfigInWord(iBus, iDev, iFunc, PCI_CLASS_DEVICE, &usClass);
            
            switch (usClass) {

            case PCI_CLASS_BRIDGE_PCI:
                API_PciConfigOutWord(iBus, iDev, iFunc, PCI_COMMAND, 0x0000);

                API_PciConfigOutByte(iBus, iDev, iFunc, PCI_PRIMARY_BUS, 0x00);
                API_PciConfigOutByte(iBus, iDev, iFunc, PCI_SECONDARY_BUS, 0x00);
                API_PciConfigOutByte(iBus, iDev, iFunc, PCI_SUBORDINATE_BUS, 0x00);

                API_PciConfigOutDword(iBus, iDev, iFunc, PCI_BASE_ADDRESS_0, 0x00000000);
                API_PciConfigOutDword(iBus, iDev, iFunc, PCI_BASE_ADDRESS_1, 0x00000000);

                API_PciConfigOutByte(iBus, iDev, iFunc, PCI_IO_BASE, 0x00);
                API_PciConfigOutByte(iBus, iDev, iFunc, PCI_IO_LIMIT, 0x00);

                API_PciConfigOutWord(iBus, iDev, iFunc, PCI_MEMORY_BASE, 0x0000);
                API_PciConfigOutWord(iBus, iDev, iFunc, PCI_MEMORY_LIMIT, 0x0000);

                API_PciConfigOutWord(iBus, iDev, iFunc, PCI_PREF_MEMORY_BASE, 0x0000);
                API_PciConfigOutWord(iBus, iDev, iFunc, PCI_PREF_MEMORY_LIMIT, 0x0000);
                break;

            case PCI_CLASS_BRIDGE_CARDBUS:
                API_PciConfigOutWord(iBus, iDev, iFunc, PCI_COMMAND, 0x0000);

                API_PciConfigOutByte(iBus, iDev, iFunc, PCI_PRIMARY_BUS, 0x00);
                API_PciConfigOutByte(iBus, iDev, iFunc, PCI_SECONDARY_BUS, 0x00);
                API_PciConfigOutByte(iBus, iDev, iFunc, PCI_SUBORDINATE_BUS, 0x00);

                API_PciConfigOutDword(iBus, iDev, iFunc, PCI_BASE_ADDRESS_0, 0x00000000);
                break;

            default:
                API_PciConfigOutWord(iBus, iDev, iFunc, PCI_COMMAND, 0x0000);

                API_PciConfigOutDword(iBus, iDev, iFunc, PCI_BASE_ADDRESS_0, 0x00000000);
                API_PciConfigOutDword(iBus, iDev, iFunc, PCI_BASE_ADDRESS_1, 0x00000000);
                API_PciConfigOutDword(iBus, iDev, iFunc, PCI_BASE_ADDRESS_2, 0x00000000);
                API_PciConfigOutDword(iBus, iDev, iFunc, PCI_BASE_ADDRESS_3, 0x00000000);
                API_PciConfigOutDword(iBus, iDev, iFunc, PCI_BASE_ADDRESS_4, 0x00000000);
                API_PciConfigOutDword(iBus, iDev, iFunc, PCI_BASE_ADDRESS_5, 0x00000000);

                API_PciConfigOutDword(iBus, iDev, iFunc, PCI_ROM_ADDRESS, 0x00000000);
                break;
            }
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciAutoScan
** 功能描述: 扫描总线
** 输　入  : hCtrl      控制器句柄
**           puiSubBus  总线数目
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciAutoScan (PCI_CTRL_HANDLE  hCtrl, UINT32 *puiSubBus)
{
    INT                 iRet;                                           /* 操作结果                     */
    PCI_AUTO_HANDLE     hPciAuto;                                       /* 自动配置控制句柄             */
    UINT32              uiSubBus = 0;                                   /* 子总线                       */

    if (!hCtrl) {
        return  (PX_ERROR);
    }

    hPciAuto = &hCtrl->PCI_tAutoConfig;
    if (!hPciAuto->PCIAUTO_iConfigEn) {                                 /* 自动配置禁能                 */
        return  (ERROR_NONE);
    }

    if (hPciAuto->PCIAUTO_uiFirstBusNo > hPciAuto->PCIAUTO_uiCurrentBusNo) {
        hPciAuto->PCIAUTO_uiCurrentBusNo = hPciAuto->PCIAUTO_uiFirstBusNo;
    }

    iRet = API_PciAutoCtrlInit(hCtrl);                                  /* 初始化控制器自动配置参数     */
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
                                                                        /* 扫描总线上设备               */
    iRet = API_PciAutoBusScan(hCtrl, hPciAuto->PCIAUTO_uiCurrentBusNo, &uiSubBus);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    if (puiSubBus) {
        *puiSubBus = uiSubBus;                                          /* 保存总线数                   */
    }

    hPciAuto->PCIAUTO_uiLastBusNo = uiSubBus;                           /* 更新结束总线号               */

    PCI_AUTO_LOG(PCI_AUTO_LOG_PRT, "scan bus no %02x.\n", uiSubBus);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciAutoBusScan
** 功能描述: 扫描总线
** 输　入  : hCtrl      控制器句柄
**           iBus       总线号
**           puiSubBus  总线数目
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciAutoBusScan (PCI_CTRL_HANDLE  hCtrl, INT  iBus, UINT32 *puiSubBus)
{
    INT                     iRet;                                       /* 操作结果                     */
    PCI_AUTO_HANDLE         hPciAuto = &hCtrl->PCI_tAutoConfig;         /* 自动配置控制句柄             */
    PCI_AUTO_DEV_HANDLE     hAutoDev;                                   /* 自动配置设备句柄             */
    INT                     iDev;                                       /* 设备号                       */
    INT                     iFunc;                                      /* 功能号                       */
    UINT32                  uiSubBus;                                   /* 子总线号                     */
    UINT32                  uiMulti;                                    /* 多功能标识                   */
    UINT16                  usVendor;                                   /* 厂商信息                     */
    UINT16                  usDevice;                                   /* 设备信息                     */
    UINT16                  usClass;                                    /* 类信息                       */
    UINT8                   ucHeaderType;                               /* 设备头类型                   */
    UINT32                  uiBusNum = 0;                               /* 总线数量                     */

    uiSubBus = iBus;
    uiMulti = 0;

    for (hAutoDev  = PCI_AUTO_BDF(iBus, 0, 0);
         hAutoDev <= PCI_AUTO_BDF(iBus, PCI_MAX_SLOTS - 1, PCI_MAX_FUNCTIONS - 1);
         hAutoDev += PCI_AUTO_BDF(0, 0, 1)) {                           /* 遍历当前总线上的设备         */
        iDev  = PCI_AUTO_DEV(hAutoDev);
        iFunc = PCI_AUTO_FUNC(hAutoDev);

        PCI_AUTO_LOG(PCI_AUTO_LOG_PRT, "dev scan %02x:%02x.%01x.\n", iBus, iDev, iFunc);

        iRet = __pciAutoDevSkip(hCtrl, hAutoDev);                       /* 是否忽略设备                 */
        if (iRet == ERROR_NONE) {
            PCI_AUTO_LOG(PCI_AUTO_LOG_PRT, "dev skip %02x:%02x.%01x.\n", iBus, iDev, iFunc);
            continue;
        }

        if ((PCI_AUTO_FUNC(hAutoDev)) && (!uiMulti)) {                  /* 单功能设备                   */
            continue;
        }

        API_PciConfigInByte(iBus, iDev, iFunc, PCI_HEADER_TYPE, &ucHeaderType);
        API_PciConfigInWord(iBus, iDev, iFunc, PCI_VENDOR_ID, &usVendor);
        if ((usVendor == 0xffff) ||
            (usVendor == 0x0000)) {                                     /* 设备无效                     */
            continue;
        }

        if (!PCI_AUTO_FUNC(hAutoDev)) {
            uiMulti = ucHeaderType & PCI_HEADER_MULTI_FUNC;
        }
        API_PciConfigInWord(iBus, iDev, iFunc, PCI_DEVICE_ID, &usDevice);
        API_PciConfigInWord(iBus, iDev, iFunc, PCI_CLASS_DEVICE, &usClass);

        PCI_AUTO_LOG(PCI_AUTO_LOG_PRT,
                     "dev fix up %02x:%02x.%01x device 0x%04x class 0x%04x.\n",
                     iBus, iDev, iFunc, usDevice, usClass);

        if (hPciAuto->PCIAUTO_pfuncDevFixup) {                          /* 配置指定设备                 */
            hPciAuto->PCIAUTO_pfuncDevFixup(hCtrl, hAutoDev, usVendor, usDevice, usClass);
        }

        API_PciAutoDeviceConfig(hCtrl, hAutoDev, &uiBusNum);            /* 遍历其它设备                 */
        uiSubBus = __MAX(uiBusNum, uiSubBus);

        if (hPciAuto->PCIAUTO_pfuncDevIrqFixup) {                       /* 配置指定设备中断             */
            hPciAuto->PCIAUTO_pfuncDevIrqFixup(hCtrl, hAutoDev);
        }
    }

    if (puiSubBus) {
        *puiSubBus = uiSubBus;                                          /* 保存总线号                   */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciAutoPostScanBridgeSetup
** 功能描述: 自动配置指定控制器上的桥设备
** 输　入  : hCtrl      控制器句柄
**           hAutoDev   设备句柄
**           iSubBus    子总线号
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciAutoPostScanBridgeSetup (PCI_CTRL_HANDLE  hCtrl, PCI_AUTO_DEV_HANDLE  hAutoDev, INT  iSubBus)
{
    PCI_AUTO_HANDLE         hPciAuto;                                   /* 自动配置控制句柄             */
    INT                     iBus  = PCI_AUTO_BUS(hAutoDev);             /* 总线号                       */
    INT                     iDev  = PCI_AUTO_DEV(hAutoDev);             /* 设备号                       */
    INT                     iFunc = PCI_AUTO_FUNC(hAutoDev);            /* 功能号                       */
    PCI_AUTO_REGION_HANDLE  hRegionIo;                                  /* I/O 资源                     */
    PCI_AUTO_REGION_HANDLE  hRegionMem;                                 /* MEM 资源                     */
    PCI_AUTO_REGION_HANDLE  hRegionPre;                                 /* PRE 资源                     */
    UINT8                   ucAddr;                                     /* 8 位地址                     */
    UINT16                  usAddr;                                     /* 16 位地址                    */
    UINT32                  uiAddr;                                     /* 32 位地址                    */
    UINT16                  usPrefechable64;                            /* 64 位预取标识                */
    UINT8                   ucPri;                                      /* PRI 参数                     */
    UINT8                   ucSec;                                      /* SEC 参数                     */
    UINT8                   ucSub;                                      /* SUB 参数                     */


    if ((!hCtrl) || (iSubBus < 0)) {                                    /* 参数无效                     */
        return  (PX_ERROR);
    }

    /*
     *  获取自动配置参数
     */
    hPciAuto   = &hCtrl->PCI_tAutoConfig;
    hRegionIo  = hPciAuto->PCIAUTO_hRegionIo;
    hRegionMem = hPciAuto->PCIAUTO_hRegionMem;
    hRegionPre = hPciAuto->PCIAUTO_hRegionPre;

    /*
     *  配置路由参数
     */
    API_PciConfigOutByte(iBus, iDev, iFunc,
                         PCI_SUBORDINATE_BUS, iSubBus - hPciAuto->PCIAUTO_uiFirstBusNo);

    PCI_AUTO_LOG(PCI_AUTO_LOG_PRT,
                 "post scan bridge %02x:%02x.%01x first bus 0x%02x sub bus 0x%02x.\n",
                 iBus, iDev, iFunc, hPciAuto->PCIAUTO_uiFirstBusNo, iSubBus);

    API_PciConfigInByte(iBus, iDev, iFunc, PCI_PRIMARY_BUS, &ucPri);
    API_PciConfigInByte(iBus, iDev, iFunc, PCI_SECONDARY_BUS, &ucSec);
    API_PciConfigInByte(iBus, iDev, iFunc, PCI_SUBORDINATE_BUS, &ucSub);
    PCI_AUTO_LOG(PCI_AUTO_LOG_PRT,
                 "post scan bridge %02x:%02x.%01x pri 0x%02x sec 0x%02x sub 0x%02x.\n",
                 iBus, iDev, iFunc, ucPri, ucSec, ucSub);

    /*
     *  配置资源参数
     */
    if (hRegionMem) {
        API_PciAutoRegionAlign(hRegionMem, 0x100000);
        usAddr = (hRegionMem->PCIAUTOREG_addrBusLower - 1) >> 16;
        API_PciConfigOutWord(iBus, iDev, iFunc, PCI_MEMORY_LIMIT, usAddr);

        PCI_AUTO_LOG(PCI_AUTO_LOG_PRT,
                     "%02x:%02x.%01x memory limit 0x%qx.\n",
                     iBus, iDev, iFunc, hRegionMem->PCIAUTOREG_addrBusLower - 1);
    }

    if (hRegionPre) {
        API_PciConfigInWord(iBus, iDev, iFunc, PCI_PREF_MEMORY_LIMIT, &usPrefechable64);
        usPrefechable64 &= PCI_PREF_RANGE_TYPE_MASK;

        API_PciAutoRegionAlign(hRegionPre, 0x100000);
        usAddr = (hRegionPre->PCIAUTOREG_addrBusLower - 1) >> 16;
        API_PciConfigOutWord(iBus, iDev, iFunc, PCI_PREF_MEMORY_LIMIT, usAddr);

        PCI_AUTO_LOG(PCI_AUTO_LOG_PRT,
                     "%02x:%02x.%01x pref memory limit 0x%qx.\n",
                     iBus, iDev, iFunc, hRegionPre->PCIAUTOREG_addrBusLower - 1);

        if (usPrefechable64 == PCI_PREF_RANGE_TYPE_64) {
            uiAddr = (hRegionPre->PCIAUTOREG_addrBusLower - 1) >> 32;
            API_PciConfigOutDword(iBus, iDev, iFunc, PCI_PREF_LIMIT_UPPER32, uiAddr);

            PCI_AUTO_LOG(PCI_AUTO_LOG_PRT,
                         "%02x:%02x.%01x pref memory limit upper 0x%qx.\n",
                         iBus, iDev, iFunc, (hRegionPre->PCIAUTOREG_addrBusLower - 1) >> 32);
        }
    }

    if (hRegionIo) {
        API_PciAutoRegionAlign(hRegionIo, 0x1000);
        ucAddr = ((hRegionIo->PCIAUTOREG_addrBusLower - 1) & 0x0000f000) >> 8;
        API_PciConfigOutByte(iBus, iDev, iFunc, PCI_IO_LIMIT, ucAddr);
        usAddr = ((hRegionIo->PCIAUTOREG_addrBusLower - 1) & 0xffff0000) >> 16;
        API_PciConfigOutWord(iBus, iDev, iFunc, PCI_IO_LIMIT_UPPER16, usAddr);

        PCI_AUTO_LOG(PCI_AUTO_LOG_PRT,
                     "%02x:%02x.%01x io limit 0x%qx.\n",
                     iBus, iDev, iFunc, hRegionIo->PCIAUTOREG_addrBusLower - 1);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciAutoPreScanBridgeSetup
** 功能描述: 自动配置指定控制器上的桥设备
** 输　入  : hCtrl      控制器句柄
**           hAutoDev   设备句柄
**           iSubBus    子总线号
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciAutoPreScanBridgeSetup (PCI_CTRL_HANDLE      hCtrl,
                                    PCI_AUTO_DEV_HANDLE  hAutoDev,
                                    INT                  iSubBus)
{
    PCI_AUTO_HANDLE         hPciAuto;                                   /* 自动配置控制句柄             */
    INT                     iBus  = PCI_AUTO_BUS(hAutoDev);             /* 总线号                       */
    INT                     iDev  = PCI_AUTO_DEV(hAutoDev);             /* 设备号                       */
    INT                     iFunc = PCI_AUTO_FUNC(hAutoDev);            /* 功能号                       */
    PCI_AUTO_REGION_HANDLE  hRegionIo;                                  /* I/O 资源                     */
    PCI_AUTO_REGION_HANDLE  hRegionMem;                                 /* MEM 资源                     */
    PCI_AUTO_REGION_HANDLE  hRegionPre;                                 /* PRE 资源                     */
    UINT16                  usCmdStatus;                                /* 控制及状态信息               */
    UINT16                  usPrefechable;                              /* 预取标识                     */
    UINT8                   ucAddr;                                     /* 8 位地址                     */
    UINT16                  usAddr;                                     /* 16 位地址                    */
    UINT32                  uiAddr;                                     /* 32 位地址                    */

    if ((!hCtrl) || (iSubBus < 0)) {
        return  (PX_ERROR);
    }

    /*
     *  获取自动配置参数
     */
    hPciAuto   = &hCtrl->PCI_tAutoConfig;
    hRegionIo  = hPciAuto->PCIAUTO_hRegionIo;
    hRegionMem = hPciAuto->PCIAUTO_hRegionMem;
    hRegionPre = hPciAuto->PCIAUTO_hRegionPre;

    /*
     *  配置控制参数及预取基地址
     */
    API_PciConfigInWord(iBus, iDev, iFunc, PCI_COMMAND, &usCmdStatus);
    API_PciConfigInWord(iBus, iDev, iFunc, PCI_PREF_MEMORY_BASE, &usPrefechable);
    usPrefechable &= PCI_PREF_RANGE_TYPE_MASK;

    /*
     *  配置路由参数
     */
    API_PciConfigOutByte(iBus, iDev, iFunc, PCI_PRIMARY_BUS, iBus - hPciAuto->PCIAUTO_uiFirstBusNo);
    API_PciConfigOutByte(iBus, iDev, iFunc, PCI_SECONDARY_BUS, iSubBus - hPciAuto->PCIAUTO_uiFirstBusNo);
    API_PciConfigOutByte(iBus, iDev, iFunc, PCI_SUBORDINATE_BUS, 0xff);

    PCI_AUTO_LOG(PCI_AUTO_LOG_PRT,
                 "%02x:%02x.%01x pre scan bridge first bus 0x%02x sub bus 0x%02x prefechable 0x%02x.\n",
                 iBus, iDev, iFunc, hPciAuto->PCIAUTO_uiFirstBusNo, iSubBus, usPrefechable);

    /*
     *  配置资源参数
     */
    if (hRegionMem) {
        API_PciAutoRegionAlign(hRegionMem, 0x100000);
        usAddr = (hRegionMem->PCIAUTOREG_addrBusLower & 0xfff00000) >> 16;
        API_PciConfigOutWord(iBus, iDev, iFunc, PCI_MEMORY_BASE, usAddr);

        PCI_AUTO_LOG(PCI_AUTO_LOG_PRT,
                     "%02x:%02x.%01x memory base 0x%qx.\n",
                     iBus, iDev, iFunc, (UINT64)hRegionMem->PCIAUTOREG_addrBusLower);

        usCmdStatus |= PCI_COMMAND_MEMORY;
    }

    if (hRegionPre) {
        API_PciAutoRegionAlign(hRegionPre, 0x100000);
        usAddr = (hRegionPre->PCIAUTOREG_addrBusLower & 0xfff00000) >> 16;
        API_PciConfigOutWord(iBus, iDev, iFunc, PCI_PREF_MEMORY_BASE, usAddr);
        if (usPrefechable == PCI_PREF_RANGE_TYPE_64) {
            uiAddr = hRegionPre->PCIAUTOREG_addrBusLower >> 32;
            API_PciConfigOutDword(iBus, iDev, iFunc, PCI_PREF_BASE_UPPER32, uiAddr);
        }

        PCI_AUTO_LOG(PCI_AUTO_LOG_PRT,
                     "%02x:%02x.%01x %s pref memory base 0x%qx.\n",
                     iBus, iDev, iFunc,
                     (usPrefechable == PCI_PREF_RANGE_TYPE_64) ? "64bit" : "32bit",
                     (UINT64)hRegionPre->PCIAUTOREG_addrBusLower);

        usCmdStatus |= PCI_COMMAND_MEMORY;
    
    } else {
        API_PciConfigOutWord(iBus, iDev, iFunc, PCI_PREF_MEMORY_BASE, 0x1000);
        API_PciConfigOutWord(iBus, iDev, iFunc, PCI_PREF_MEMORY_LIMIT, 0x0);
        if (usPrefechable == PCI_PREF_RANGE_TYPE_64) {
            API_PciConfigOutWord(iBus, iDev, iFunc, PCI_PREF_BASE_UPPER32, 0x0);
            API_PciConfigOutWord(iBus, iDev, iFunc, PCI_PREF_LIMIT_UPPER32, 0x0);
        }
    }

    if (hRegionIo) {
        API_PciAutoRegionAlign(hRegionIo, 0x1000);
        ucAddr = (hRegionIo->PCIAUTOREG_addrBusLower & 0x0000f000) >> 8;
        API_PciConfigOutByte(iBus, iDev, iFunc, PCI_IO_BASE, ucAddr);
        usAddr = (hRegionIo->PCIAUTOREG_addrBusLower & 0xffff0000) >> 16;
        API_PciConfigOutWord(iBus, iDev, iFunc, PCI_IO_BASE_UPPER16, usAddr);

        PCI_AUTO_LOG(PCI_AUTO_LOG_PRT,
                     "%02x:%02x.%01x io base 0x%qx.\n",
                     iBus, iDev, iFunc, (UINT64)hRegionIo->PCIAUTOREG_addrBusLower);

        usCmdStatus |= PCI_COMMAND_IO;
    }

    API_PciConfigOutWord(iBus, iDev, iFunc, PCI_COMMAND, usCmdStatus | PCI_COMMAND_MASTER);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciAutoDeviceSetup
** 功能描述: 自动配置指定控制器上的设备
** 输　入  : hCtrl              控制器句柄
**           hAutoDev           设备句柄
**           uiBarNum           资源数目
**           hRegionIo          IO 资源
**           hRegionMem         MMEM 资源
**           hRegionPrefetch    可预取资源
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciAutoDeviceSetup (PCI_CTRL_HANDLE         hCtrl,
                             PCI_AUTO_DEV_HANDLE     hAutoDev,
                             UINT32                  uiBarNum,
                             PCI_AUTO_REGION_HANDLE  hRegionIo,
                             PCI_AUTO_REGION_HANDLE  hRegionMem,
                             PCI_AUTO_REGION_HANDLE  hRegionPrefetch)
{
    INT                     iRet;                                       /* 操作结果                     */
    REGISTER INT            uiBar;                                      /* BAR 索引                     */
    PCI_AUTO_HANDLE         hPciAuto;                                   /* 自动配置句柄                 */
    INT                     iBus  = PCI_AUTO_BUS(hAutoDev);             /* 总线号                       */
    INT                     iDev  = PCI_AUTO_DEV(hAutoDev);             /* 设备号                       */
    INT                     iFunc = PCI_AUTO_FUNC(hAutoDev);            /* 功能号                       */
    UINT32                  uiBarResponse;                              /* 资源信息                     */
    UINT32                  uiBarResponseUpper;                         /* 资源信息高地址信息           */
    pci_addr_t              addrBarAddr;                                /* BAR 地址                     */
    pci_size_t              stBarSize;                                  /* BAR 大小                     */
    UINT16                  usClass;                                    /* 类信息                       */
    UINT16                  usCmdStatus;                                /* 命令与状态                   */
    UINT8                   ucHeaderType;                               /* 设备头类型                   */
    INT                     iBarIndex = 0;                              /* BAR 索引                     */
    INT                     iIndex = 0;                                 /* 遍历索引                     */
    UINT                    uiRomAddrIndex;                             /* ROM 地址索引                 */
    pci_addr_t              addrBarValue;                               /* BAR 值                       */
    PCI_AUTO_REGION_HANDLE  hBarRegion;                                 /* 资源句柄                     */
    INT                     iMem64En;                                   /* 64 位地址使能标识            */
    UINT8                   ucCacheLineSize;                            /* 高速缓冲行大小               */
    UINT8                   ucLatencyTimer;                             /* 时间参数                     */

    if (!hCtrl) {
        return  (PX_ERROR);
    }

    hPciAuto = &hCtrl->PCI_tAutoConfig;

    API_PciConfigInWord(iBus, iDev, iFunc, PCI_COMMAND, &usCmdStatus);  /* 获取初始控制信息             */
    usCmdStatus = (usCmdStatus & ~(PCI_COMMAND_IO | PCI_COMMAND_MEMORY)) | PCI_COMMAND_MASTER;

    /*
     *  遍历所有资源并进行配置
     */
    iIndex = 0;
    iBarIndex = 0;
    for (uiBar = PCI_BASE_ADDRESS_0; uiBar < (PCI_BASE_ADDRESS_0 + (uiBarNum * 4)); uiBar += 4) {
        API_PciConfigOutDword(iBus, iDev, iFunc, uiBar, 0xffffffff);
        API_PciConfigInDword(iBus, iDev, iFunc, uiBar, &uiBarResponse);
        PCI_AUTO_LOG(PCI_AUTO_LOG_PRT,
                     "%02x:%02x.%01x BAR %d response=0x%x.\n", iBus, iDev, iFunc, iIndex, uiBarResponse);
        iIndex++;
        if (!uiBarResponse) {
            continue;
        }

        iMem64En = 0;

        if (uiBarResponse & PCI_BASE_ADDRESS_SPACE) {
            stBarSize  = (pci_size_t)(((~(uiBarResponse & PCI_BASE_ADDRESS_IO_MASK)) & 0xffff) + 1);
            hBarRegion = hRegionIo;

            PCI_AUTO_LOG(PCI_AUTO_LOG_PRT,
                         "%02x:%02x.%01x BAR %d, I/O, size=0x%qx.\n",
                         iBus, iDev, iFunc, iBarIndex, (UINT64)stBarSize);
        
        } else {
            if ((uiBarResponse & PCI_BASE_ADDRESS_MEM_TYPE_MASK) == PCI_BASE_ADDRESS_MEM_TYPE_64) {
                API_PciConfigOutDword(iBus, iDev, iFunc, uiBar + 4, 0xffffffff);
                API_PciConfigInDword(iBus, iDev, iFunc, uiBar + 4, &uiBarResponseUpper);
                addrBarAddr = ((pci_addr_t)uiBarResponseUpper << 32) | uiBarResponse;
                stBarSize = ~(addrBarAddr & PCI_BASE_ADDRESS_MEM_MASK) + 1;
                iMem64En = 1;
            
            } else {
                stBarSize = (UINT32)(~(uiBarResponse & PCI_BASE_ADDRESS_MEM_MASK) + 1);
            }

            if ((hRegionPrefetch) &&
                (uiBarResponse & PCI_BASE_ADDRESS_MEM_PREFETCH)) {
                hBarRegion = hRegionPrefetch;
            
            } else {
                hBarRegion = hRegionMem;
            }

            PCI_AUTO_LOG(PCI_AUTO_LOG_PRT,
                         "%02x:%02x.%01x BAR %d, type %s %s, size=0x%qx.\n",
                         iBus, iDev, iFunc, iBarIndex,
                         iMem64En ? "64bit" : "32bit",
                         (hBarRegion == hRegionPrefetch) ? "Prf" : "Mem", (UINT64)stBarSize);
        }

        iRet = API_PciAutoRegionAllocate(hBarRegion, stBarSize, &addrBarValue);
        if (iRet != PX_ERROR) {
            API_PciConfigOutDword(iBus, iDev, iFunc, uiBar, (UINT32)addrBarValue);
            if (iMem64En) {
                uiBar += 4;
                API_PciConfigOutDword(iBus, iDev, iFunc, uiBar, (UINT32)(addrBarValue >> 32));
            }
        }

        usCmdStatus |= (uiBarResponse & PCI_BASE_ADDRESS_SPACE) ? PCI_COMMAND_IO : PCI_COMMAND_MEMORY;

        iBarIndex++;
    }

    /*
     *  配置 ROM 参数
     */
    API_PciConfigInByte(iBus, iDev, iFunc, PCI_HEADER_TYPE, &ucHeaderType);
    ucHeaderType &= 0x7f;
    if (ucHeaderType != PCI_HEADER_TYPE_CARDBUS) {
        uiRomAddrIndex = (ucHeaderType == PCI_HEADER_TYPE_NORMAL) ? PCI_ROM_ADDRESS : PCI_ROM_ADDRESS1;
        API_PciConfigOutDword(iBus, iDev, iFunc, uiRomAddrIndex, 0xfffffffe);
        API_PciConfigInDword(iBus, iDev, iFunc, uiRomAddrIndex, &uiBarResponse);
        if (uiBarResponse) {
            stBarSize = -(uiBarResponse & ~1);

            PCI_AUTO_LOG(PCI_AUTO_LOG_PRT, "ROM, size = 0x%qx.\n", (UINT64)stBarSize);

            iRet = API_PciAutoRegionAllocate(hRegionMem, stBarSize, &addrBarValue);
            if (iRet != PX_ERROR) {
                API_PciConfigOutDword(iBus, iDev, iFunc, uiRomAddrIndex, addrBarValue);
            }
            usCmdStatus |= PCI_COMMAND_MEMORY;
        }
    }

    API_PciConfigInWord(iBus, iDev, iFunc, PCI_CLASS_DEVICE, &usClass);
    if (usClass == PCI_CLASS_DISPLAY_VGA) {
        usCmdStatus |= PCI_COMMAND_IO;
    }

    API_PciConfigOutWord(iBus, iDev, iFunc, PCI_COMMAND, usCmdStatus);
    if (!hPciAuto->PCIAUTO_ucCacheLineSize) {
        ucCacheLineSize = PCI_AUTO_CACHE_LINE_SIZE;
    
    } else {
        ucCacheLineSize = hPciAuto->PCIAUTO_ucCacheLineSize;
    }
    if (!hPciAuto->PCIAUTO_ucLatencyTimer) {
        ucLatencyTimer = PCI_AUTO_LATENCY_TIMER;
    
    } else {
        ucLatencyTimer = hPciAuto->PCIAUTO_ucLatencyTimer;
    }
    API_PciConfigOutByte(iBus, iDev, iFunc, PCI_CACHE_LINE_SIZE, ucCacheLineSize);
    API_PciConfigOutByte(iBus, iDev, iFunc, PCI_LATENCY_TIMER, ucLatencyTimer);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciAutoRegionAllocate
** 功能描述: 调整资源分区
** 输　入  : hRegion    资源分区句柄
**           stSize     资源大小
**           addrBar    资源缓冲
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciAutoRegionAllocate (PCI_AUTO_REGION_HANDLE  hRegion, pci_size_t stSize, pci_addr_t *paddrBar)
{
    pci_addr_t      addrAddr;

    if (!hRegion) {
        if (paddrBar) {
            *paddrBar = (pci_addr_t)-1;
        }
        return  (PX_ERROR);
    }

    /*
     *  进行对齐处理
     */
    addrAddr = ((hRegion->PCIAUTOREG_addrBusLower - 1) | (stSize - 1)) + 1;
    if ((addrAddr - hRegion->PCIAUTOREG_addrBusStart + stSize) > hRegion->PCIAUTOREG_stSize) {
        if (paddrBar) {
            *paddrBar = (pci_addr_t)-1;
        }
        return  (PX_ERROR);
    }

    hRegion->PCIAUTOREG_addrBusLower = addrAddr + stSize;               /* 更新当前地址信息             */
    if (paddrBar) {
        *paddrBar = addrAddr;
    }

    PCI_AUTO_LOG(PCI_AUTO_LOG_PRT,
                 "region allocate address = 0x%qx bus_lower = 0x%qx.\n",
                 (UINT64)addrAddr, (UINT64)hRegion->PCIAUTOREG_addrBusLower);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciAutoRegionAlign
** 功能描述: 自动配置的资源分区对齐
** 输　入  : hRegion    资源分区句柄
**           stSize     对齐大小
** 输　出  : NONE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
VOID  API_PciAutoRegionAlign (PCI_AUTO_REGION_HANDLE  hRegion, pci_size_t stSize)
{
    hRegion->PCIAUTOREG_addrBusLower = ((hRegion->PCIAUTOREG_addrBusLower - 1) | (stSize - 1)) + 1;
}
/*********************************************************************************************************
** 函数名称: API_PciAutoRegionInit
** 功能描述: 自动配置的资源分区初始化
** 输　入  : hRegion    资源分区句柄
** 输　出  : NONE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
VOID  API_PciAutoRegionInit (PCI_AUTO_REGION_HANDLE  hRegion)
{
    PCHAR       pcRegionName = LW_NULL;

    if (!hRegion) {
        return;
    }

    /*
     *  为了避免 0 地址的存在, 使用 0x1000 代替.
     */
    if (hRegion->PCIAUTOREG_addrBusStart) {
        hRegion->PCIAUTOREG_addrBusLower = hRegion->PCIAUTOREG_addrBusStart;
    
    } else {
        hRegion->PCIAUTOREG_addrBusLower = 0x1000;
    }

    /*
     *  获取资源类型
     */
    switch (hRegion->PCIAUTOREG_ulFlags) {

    case PCI_AUTO_REGION_IO:
        pcRegionName = "I/O             ";
        break;

    case PCI_AUTO_REGION_MEM:
        pcRegionName = "Memory          ";
        break;

    case PCI_AUTO_REGION_MEM | PCI_AUTO_REGION_PREFETCH:
        pcRegionName = "Prefetchable Mem";
        break;

    default:
        pcRegionName = "xxxx";
        break;
    }

    PCI_AUTO_LOG(PCI_AUTO_LOG_PRT,
                 "Bus %s region [0x%qx-0x%qx] Physical Memory [0x%qx-0x%qx].\n",
                 pcRegionName,
                 (UINT64)hRegion->PCIAUTOREG_addrBusStart,
                 (UINT64)(hRegion->PCIAUTOREG_addrBusStart + hRegion->PCIAUTOREG_stSize - 1),
                 (UINT64)hRegion->PCIAUTOREG_addrPhyStart,
                 (UINT64)(hRegion->PCIAUTOREG_addrPhyStart + hRegion->PCIAUTOREG_stSize - 1));
}
/*********************************************************************************************************
** 函数名称: API_PciAutoDeviceConfig
** 功能描述: 自动配置指定控制器上的设备
** 输　入  : hCtrl      控制器句柄
**           hAutoDev   设备句柄
**           puiSubBus  总线数量
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciAutoDeviceConfig (PCI_CTRL_HANDLE  hCtrl, PCI_AUTO_DEV_HANDLE  hAutoDev, UINT32 *puiSubBus)
{
    INT                     iRet;                                       /* 操作结果                     */
    PCI_AUTO_HANDLE         hPciAuto;                                   /* 自动配置控制句柄             */
    PCI_AUTO_REGION_HANDLE  hRegionIo;                                  /* I/O 资源                     */
    PCI_AUTO_REGION_HANDLE  hRegionMem;                                 /* MEM 资源                     */
    PCI_AUTO_REGION_HANDLE  hRegionPre;                                 /* PRE 资源                     */
    UINT32                  uiSubBus = PCI_AUTO_BUS(hAutoDev);          /* 子总线号                     */
    UINT32                  uiBusNum;                                   /* 总线数目                     */
    UINT16                  usVendor;                                   /* 厂商信息                     */
    UINT16                  usClass;                                    /* 类信息                       */
    INT                     iBus  = PCI_AUTO_BUS(hAutoDev);             /* 总线号                       */
    INT                     iDev  = PCI_AUTO_DEV(hAutoDev);             /* 设备号                       */
    INT                     iFunc = PCI_AUTO_FUNC(hAutoDev);            /* 功能号                       */

    if (!hCtrl) {
        return  (PX_ERROR);
    }

    /*
     *  获取自动配置参数
     */
    hPciAuto   = &hCtrl->PCI_tAutoConfig;
    hRegionIo  = hPciAuto->PCIAUTO_hRegionIo;
    hRegionMem = hPciAuto->PCIAUTO_hRegionMem;
    hRegionPre = hPciAuto->PCIAUTO_hRegionPre;

    iRet = API_PciConfigInWord(iBus, iDev, iFunc, PCI_VENDOR_ID, &usVendor);
    if ((iRet != ERROR_NONE) ||
        (usVendor == 0xffff) ||
        (usVendor == 0x0000)) {                                         /* 设备无效                     */
        return  (PX_ERROR);
    }

    /*
     *  通过设备类型进行设备配置
     */
    API_PciConfigInWord(iBus, iDev, iFunc, PCI_CLASS_DEVICE, &usClass);

    switch (usClass) {

    case PCI_CLASS_BRIDGE_PCI:
        PCI_AUTO_LOG(PCI_AUTO_LOG_PRT, "Found P2P bridge, %02x:%02x.%01x.\n", iBus, iDev, iFunc);

        API_PciAutoDeviceSetup(hCtrl, hAutoDev, 2, hRegionIo, hRegionMem, hRegionPre);

        hPciAuto->PCIAUTO_uiCurrentBusNo++;
        API_PciAutoPreScanBridgeSetup(hCtrl, hAutoDev, hPciAuto->PCIAUTO_uiCurrentBusNo);
        API_PciAutoBusScan(hCtrl, hPciAuto->PCIAUTO_uiCurrentBusNo, &uiBusNum);
        uiSubBus = __MAX(uiBusNum, uiSubBus);
        API_PciAutoPostScanBridgeSetup(hCtrl, hAutoDev, uiSubBus);
        uiSubBus = hPciAuto->PCIAUTO_uiCurrentBusNo;
        break;

    case PCI_CLASS_BRIDGE_CARDBUS:
        PCI_AUTO_LOG(PCI_AUTO_LOG_PRT, "Found P2CardBus bridge, %02x:%02x.%01x.\n", iBus, iDev, iFunc);

        API_PciAutoDeviceSetup(hCtrl, hAutoDev, 0, hRegionIo, hRegionMem, hRegionPre);
        hPciAuto->PCIAUTO_uiCurrentBusNo++;
        break;

    default:
        PCI_AUTO_LOG(PCI_AUTO_LOG_PRT, "Found Normal devices, %02x:%02x.%01x.\n", iBus, iDev, iFunc);

        API_PciAutoDeviceSetup(hCtrl, hAutoDev, 6, hRegionIo, hRegionMem, hRegionPre);
        break;
    }

    PCI_AUTO_LOG(PCI_AUTO_LOG_PRT, "auto config sub bus 0x%02x.\n", uiSubBus);

    if (puiSubBus) {
        *puiSubBus = uiSubBus;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciAutoCtrlInit
** 功能描述: 控制器自动配置的初始化
** 输　入  : hCtrl      控制器句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciAutoCtrlInit (PCI_CTRL_HANDLE  hCtrl)
{
    REGISTER INT            i;                                          /* 循环因子                     */
    PCI_AUTO_HANDLE         hPciAuto;                                   /* 自动配置控制句柄             */
    PCI_AUTO_REGION_HANDLE  hRegion = LW_NULL;                          /* 资源控制句柄                 */

    if (!hCtrl) {
        return  (PX_ERROR);
    }

    /*
     *  获取自动配置参数
     */
    hPciAuto = &hCtrl->PCI_tAutoConfig;
    hPciAuto->PCIAUTO_hRegionIo  = LW_NULL;
    hPciAuto->PCIAUTO_hRegionMem = LW_NULL;
    hPciAuto->PCIAUTO_hRegionPre = LW_NULL;
    hPciAuto->PCIAUTO_pvPriv     = (PVOID)hCtrl;

    if (!hPciAuto->PCIAUTO_iConfigEn) {
        return  (ERROR_NONE);
    }

    /*
     *  获取资源信息
     */
    for (i = 0; i < hPciAuto->PCIAUTO_uiRegionCount; i++) {
        hRegion = &hPciAuto->PCIAUTO_tRegion[i];

        switch (hRegion->PCIAUTOREG_ulFlags) {

        case PCI_AUTO_REGION_IO:
            if ((!hPciAuto->PCIAUTO_hRegionIo) ||
                (hPciAuto->PCIAUTO_hRegionIo->PCIAUTOREG_stSize < hRegion->PCIAUTOREG_stSize)) {
                hPciAuto->PCIAUTO_hRegionIo = hRegion;
            }
            break;

        case PCI_AUTO_REGION_MEM:
            if ((!hPciAuto->PCIAUTO_hRegionMem) ||
                (hPciAuto->PCIAUTO_hRegionMem->PCIAUTOREG_stSize < hRegion->PCIAUTOREG_stSize)) {
                hPciAuto->PCIAUTO_hRegionMem = hRegion;
            }
            break;

        case (PCI_AUTO_REGION_MEM | PCI_AUTO_REGION_PREFETCH):
            if ((!hPciAuto->PCIAUTO_hRegionPre) ||
                (hPciAuto->PCIAUTO_hRegionPre->PCIAUTOREG_stSize < hRegion->PCIAUTOREG_stSize)) {
                hPciAuto->PCIAUTO_hRegionPre = hRegion;
            }
            break;

        default:
            break;
        }
    }

    /*
     *  配置资源信息
     */
    if (hPciAuto->PCIAUTO_hRegionIo) {
        API_PciAutoRegionInit(hPciAuto->PCIAUTO_hRegionIo);
    }
    if (hPciAuto->PCIAUTO_hRegionMem) {
        API_PciAutoRegionInit(hPciAuto->PCIAUTO_hRegionMem);
    }
    if (hPciAuto->PCIAUTO_hRegionPre) {
        API_PciAutoRegionInit(hPciAuto->PCIAUTO_hRegionPre);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciAutoCtrlRegionSet
** 功能描述: 设置控制器自动配置的资源
** 输　入  : hCtrl          控制器句柄
**           uiIndex        资源分区索引
**           addrBusStart   PCI 总线域起始地址
**           addrPhyStart   起始物理地址
**           stSize         空间大小
**           ulFlags        资源类型 PCI_AUTO_REGION_MEM  PCI_AUTO_REGION_IO  PCI_AUTO_REGION_PREFETCH
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciAutoCtrlRegionSet (PCI_CTRL_HANDLE  hCtrl,
                               UINT             uiIndex,
                               pci_bus_addr_t   addrBusStart,
                               pci_addr_t       addrPhyStart,
                               pci_size_t       stSize,
                               ULONG            ulFlags)
{
    PCI_AUTO_HANDLE         hPciAuto;                                   /* 自动配置控制句柄             */
    PCI_AUTO_REGION_HANDLE  hRegion = LW_NULL;                          /* 资源句柄                     */

    if ((!hCtrl) ||
        (uiIndex >= PCI_AUTO_REGION_MAX)) {                             /* 控制器句柄或资源索引无效     */
        PCI_AUTO_LOG(PCI_AUTO_LOG_ERR,
                     "set region failed ctrl handle %px index %d [%d-%d].\n",
                     hCtrl, uiIndex, PCI_AUTO_REGION_INDEX_0, (PCI_AUTO_REGION_MAX - 1));

        return  (PX_ERROR);
    }

    /*
     *  配置指定资源
     */
    hPciAuto = &hCtrl->PCI_tAutoConfig;
    hRegion  = &hPciAuto->PCIAUTO_tRegion[uiIndex];
    hRegion->PCIAUTOREG_addrBusStart = addrBusStart;
    hRegion->PCIAUTOREG_ulFlags      = ulFlags;
    hRegion->PCIAUTOREG_stSize       = stSize;
    hRegion->PCIAUTOREG_addrPhyStart = addrPhyStart;

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PCI_EN > 0)         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
