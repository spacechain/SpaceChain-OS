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
** 文   件   名: pciStorageNvme.c
**
** 创   建   人: Qin.Fei (秦飞)
**
** 文件创建日期: 2017 年 7 月 17 日
**
** 描        述: NVMe 驱动.
*********************************************************************************************************/
#define  __SYLIXOS_PCI_DRV
#define  __SYLIXOS_NVME_DRV
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "../SylixOS/config/driver/drv_cfg.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PCI_EN > 0) && (LW_CFG_NVME_EN > 0) && (LW_CFG_DRV_M2_NVME > 0)
#include "pci_ids.h"
#include "linux/compat.h"
#include "pciStorageNvme.h"
/*********************************************************************************************************
  驱动支持的设备 ID 表, 用于驱动与设备进行自动匹配.
*********************************************************************************************************/
static const PCI_DEV_ID_CB      pciStorageNvmeIdTbl[] = {
    { PCI_VDEVICE(INTEL, 0x0953), NVME_QUIRK_STRIPE_SIZE },
    { PCI_VDEVICE(INTEL, 0x5845), NVME_QUIRK_IDENTIFY_CNS },
    { PCI_DEVICE_CLASS(PCI_CLASS_STORAGE_NVM_EXPRESS, 0xffffff) },
    { PCI_DEVICE(PCI_VENDOR_ID_APPLE, 0x2001) },                        /*  iPhone6S/7 ...              */
    { 0, }
};
/*********************************************************************************************************
  NVMe 控制器计数
*********************************************************************************************************/
static UINT     pciStorageNvmeCtrlNum = 0;
/*********************************************************************************************************
** 函数名称: pciStorageNvmeHeaderQuirk
** 功能描述: 读到设备头后的特殊处理
** 输　入  : hDevHandle         PCI 设备控制块句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  pciStorageNvmeHeaderQuirk (PCI_DEV_HANDLE  hPciDevHandle)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pciNvmeDevIdTblGet
** 功能描述: 获取设备列表
** 输　入  : phPciDevId     设备 ID 列表句柄缓冲区
**           puiSzie        设备列表大小
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  pciNvmeDevIdTblGet (PCI_DEV_ID_HANDLE  *phPciDevId, UINT32  *puiSzie)
{
    if ((!phPciDevId) || (!puiSzie)) {
        return  (PX_ERROR);
    }

    *phPciDevId = (PCI_DEV_ID_HANDLE)pciStorageNvmeIdTbl;
    *puiSzie    = sizeof(pciStorageNvmeIdTbl) / sizeof(PCI_DEV_ID_CB);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pciStorageNvmeDevRemove
** 功能描述: 移除 NVMe 设备
** 输　入  : hDevHandle         PCI 设备控制块句柄
** 输　出  : 控制器句柄
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  pciStorageNvmeDevRemove (PCI_DEV_HANDLE  hHandle)
{
}
/*********************************************************************************************************
** 函数名称: pciStorageNvmeDevProbe
** 功能描述: NVMe 驱动探测到设备
** 输　入  : hDevHandle         PCI 设备控制块句柄
**           hIdEntry           匹配成功的设备 ID 条目(来自于设备 ID 表)
** 输　出  : 控制器句柄
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  pciStorageNvmeDevProbe (PCI_DEV_HANDLE  hPciDevHandle, const PCI_DEV_ID_HANDLE  hIdEntry)
{
    INT                     iRet      = PX_ERROR;
    NVME_CTRL_HANDLE        hNvmeCtrl = LW_NULL;
    UINT16                  usCap;

    if ((!hPciDevHandle) || (!hIdEntry)) {                              /*  设备参数无效                */
        _ErrorHandle(EINVAL);                                           /*  标记错误                    */
        return  (PX_ERROR);                                             /*  错误返回                    */
    }

    pciStorageNvmeHeaderQuirk(hPciDevHandle);                           /* 读到设备头后的特殊处理       */
                                                                        /* 确认设备类型                 */
    iRet = API_PciDevConfigRead(hPciDevHandle, PCI_CLASS_DEVICE, (UINT8 *)&usCap, 2);
    if ((iRet != ERROR_NONE)             ||
        ((usCap != PCI_CLASS_STORAGE_NVM) &&
         (usCap != PCI_CLASS_STORAGE_NVM_EXPRESS))) {                   /* 设备类型错误                 */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    /*
     *  更新设备参数, 设备驱动版本信息及当前类型设备的索引等
     */
    hPciDevHandle->PCIDEV_uiDevVersion = NVME_PCI_DRV_VER_NUM;          /*  当前设备驱动版本号          */
    hPciDevHandle->PCIDEV_uiUnitNumber = pciStorageNvmeCtrlNum;         /*  本类设备索引                */
    pciStorageNvmeCtrlNum++;

    hNvmeCtrl = API_NvmeCtrlCreate(NVME_PCI_DRV_NAME,
                                   hPciDevHandle->PCIDEV_uiUnitNumber,
                                   (PVOID)hPciDevHandle,
                                   hIdEntry->PCIDEVID_ulData);
    if (!hNvmeCtrl) {
        hPciDevHandle->PCIDEV_pvDevDriver = LW_NULL;
        pciStorageNvmeCtrlNum--;
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pciStorageNvmeCtrlOpt
** 功能描述: 驱动相关的选项操作
** 输　入  : hCtrl      控制器句柄
**           uiDrive    驱动器索引
**           iCmd       命令 (NVME_OPT_CMD_xxxx)
**           lArg       参数
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  pciStorageNvmeCtrlOpt (NVME_CTRL_HANDLE  hCtrl, UINT  uiDrive, INT  iCmd, LONG  lArg)
{
    if (!hCtrl) {
        return  (PX_ERROR);
    }

    switch (iCmd) {

    case NVME_OPT_CMD_DC_MSG_COUNT_GET:
        if (lArg) {
            *(ULONG *)lArg = NVME_DRIVE_DISKCACHE_MSG_COUNT;
        }
        break;

    default:
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pciStorageNvmeVendorCtrlIntEnable
** 功能描述: 使能控制器中断
** 输　入  : hCtrl         控制器句柄
**           hQueue        命令队列
**           pfuncIsr      中断服务函数
**           cpcName       中断服务名称
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  pciStorageNvmeVendorCtrlIntEnable (NVME_CTRL_HANDLE   hCtrl,
                                               NVME_QUEUE_HANDLE  hQueue,
                                               PINT_SVR_ROUTINE   pfuncIsr,
                                               CPCHAR             cpcName)
{
    UINT16              usVector   = hQueue->NVMEQUEUE_usCqVector;
    PCI_DEV_HANDLE      hPciDev    = (PCI_DEV_HANDLE)hCtrl->NVMECTRL_pvArg;
    PCI_MSI_DESC_HANDLE hMsgHandle = (PCI_MSI_DESC_HANDLE)hCtrl->NVMECTRL_pvIntHandle;

    if (hCtrl->NVMECTRL_bMsix) {
        /*
         *  MSI-X 中断信息保存在控制器结构体中
         */
        if (usVector > hCtrl->NVMECTRL_uiIntNum) {
            return  (PX_ERROR);
        }

        API_PciDevInterEnable(hPciDev,
                              hMsgHandle[usVector].PCIMSI_ulDevIrqVector,
                              (PINT_SVR_ROUTINE)pfuncIsr,
                              (PVOID)hQueue);
    } else {
        /*
         *  可能是 INTx 中断或 MSI 中断，INTx 中断只能为 0
         */
        if (!hPciDev->PCIDEV_iDevIrqMsiEn && usVector != 0) {
            return  (PX_ERROR);
        }

        API_PciDevInterEnable(hPciDev,
                              hPciDev->PCIDEV_ulDevIrqVector + usVector,
                              (PINT_SVR_ROUTINE)pfuncIsr,
                              (PVOID)hQueue);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pciStorageNvmeVendorCtrlIntConnect
** 功能描述: 链接控制器中断
** 输　入  : hCtrl      控制器句柄
**           hQueue     命令队列
**           pfuncIsr   中断服务函数
**           cpcName    中断服务名称
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  pciStorageNvmeVendorCtrlIntConnect (NVME_CTRL_HANDLE   hCtrl,
                                                NVME_QUEUE_HANDLE  hQueue,
                                                PINT_SVR_ROUTINE   pfuncIsr,
                                                CPCHAR             cpcName)
{
    UINT16              usVector   = hQueue->NVMEQUEUE_usCqVector;
    PCI_DEV_HANDLE      hPciDev    = (PCI_DEV_HANDLE)hCtrl->NVMECTRL_pvArg;
    PCI_MSI_DESC_HANDLE hMsgHandle = (PCI_MSI_DESC_HANDLE)hCtrl->NVMECTRL_pvIntHandle;

    if (hCtrl->NVMECTRL_bMsix) {
        /*
         *  MSI-X 中断信息保存在控制器结构体中
         */
        if (usVector > hCtrl->NVMECTRL_uiIntNum) {
            return  (PX_ERROR);
        }

        API_PciDevInterConnect(hPciDev,
                               hMsgHandle[usVector].PCIMSI_ulDevIrqVector,
                               (PINT_SVR_ROUTINE)pfuncIsr,
                               hQueue,
                               cpcName);
    } else {
        /*
         *  可能是 INTx 中断或 MSI 中断，INTx 中断只能为 0
         */
        if (!hPciDev->PCIDEV_iDevIrqMsiEn && usVector != 0) {
            return  (PX_ERROR);
        }

        API_PciDevInterConnect(hPciDev,
                               hPciDev->PCIDEV_ulDevIrqVector + usVector,
                               (PINT_SVR_ROUTINE)pfuncIsr,
                               hQueue,
                               cpcName);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pciStorageNvmeVendorCtrlIntDisConnect
** 功能描述: 释放控制器中断
** 输　入  : hCtrl      控制器句柄
**           hQueue     命令队列
**           pfuncIsr   中断服务函数
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
static INT  pciStorageNvmeVendorCtrlIntDisConnect (NVME_CTRL_HANDLE   hCtrl,
                                                   NVME_QUEUE_HANDLE  hQueue,
                                                   PINT_SVR_ROUTINE   pfuncIsr)
{
    UINT16              usVector   = hQueue->NVMEQUEUE_usCqVector;
    PCI_DEV_HANDLE      hPciDev    = (PCI_DEV_HANDLE)hCtrl->NVMECTRL_pvArg;
    PCI_MSI_DESC_HANDLE hMsgHandle = (PCI_MSI_DESC_HANDLE)hCtrl->NVMECTRL_pvIntHandle;

    if (hCtrl->NVMECTRL_bMsix) {
        /*
         *  MSI-X 中断信息保存在控制器结构体中
         */
        if (usVector > hCtrl->NVMECTRL_uiIntNum) {
            return  (PX_ERROR);
        }

        API_PciDevInterDisonnect(hPciDev,
                                 hMsgHandle[usVector].PCIMSI_ulDevIrqVector,
                                 (PINT_SVR_ROUTINE)pfuncIsr,
                                 hQueue);
    } else {
        /*
         *  可能是 INTx 中断或 MSI 中断，INTx 中断只能为 0
         */
        if (!hPciDev->PCIDEV_iDevIrqMsiEn && usVector != 0) {
            return  (PX_ERROR);
        }

        API_PciDevInterDisonnect(hPciDev,
                                 hPciDev->PCIDEV_ulDevIrqVector + usVector,
                                 (PINT_SVR_ROUTINE)pfuncIsr,
                                 hQueue);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pciStorageNvmeVendorCtrlReadyWork
** 功能描述: 控制器准备操作
** 输　入  : hCtrl    控制器句柄
**           uiIrqNum 中断数量
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  pciStorageNvmeVendorCtrlReadyWork (NVME_CTRL_HANDLE  hCtrl, UINT uiIrqNum)
{
    INT                     iRet;
    PCI_DEV_HANDLE          hPciDev;
    UINT16                  usPciDevId;
    ULONG                   ulBaseAddr;
    PCI_RESOURCE_HANDLE     hResource;

    hPciDev = (PCI_DEV_HANDLE)hCtrl->NVMECTRL_pvArg;                    /* 获取设备句柄                 */

    API_PciDevConfigReadWord(hPciDev, PCI_DEVICE_ID, &usPciDevId);
    NVME_LOG(NVME_LOG_PRT, "ctrl name %s index %d unit %d for pci dev %d:%d.%d dev id 0x%04x.\r\n",
             hCtrl->NVMECTRL_cCtrlName, hCtrl->NVMECTRL_uiIndex, hCtrl->NVMECTRL_uiUnitIndex,
             hPciDev->PCIDEV_iDevBus,
             hPciDev->PCIDEV_iDevDevice,
             hPciDev->PCIDEV_iDevFunction, usPciDevId);
                                                                        /* 查找对应资源信息             */
    hResource = API_PciDevStdResourceGet(hPciDev, PCI_IORESOURCE_MEM, PCI_BAR_INDEX_0);
    if (!hResource) {
        NVME_LOG(NVME_LOG_ERR, "pci BAR index %d error.\r\n", PCI_BAR_INDEX_0);
        return  (PX_ERROR);
    }

    ulBaseAddr                = (ULONG)(PCI_RESOURCE_START(hResource));
    hCtrl->NVMECTRL_pvRegAddr = (PVOID)ulBaseAddr;
    hCtrl->NVMECTRL_stRegSize = (size_t)(PCI_RESOURCE_SIZE(hResource));
    hCtrl->NVMECTRL_pvRegAddr = API_PciDevIoRemap(hCtrl->NVMECTRL_pvRegAddr,
                                                  hCtrl->NVMECTRL_stRegSize );
    if (hCtrl->NVMECTRL_pvRegAddr == LW_NULL) {
        NVME_LOG(NVME_LOG_ERR, "pci mem resource ioremap failed addr 0x%llx 0x%llx.\r\n",
                 hCtrl->NVMECTRL_pvRegAddr,  hCtrl->NVMECTRL_stRegSize);
        return  (PX_ERROR);
    }
    NVME_LOG(NVME_LOG_PRT, "nvme reg addr 0x%llx szie %llx.\r\n",
             hCtrl->NVMECTRL_pvRegAddr, hCtrl->NVMECTRL_stRegSize);
    hPciDev->PCIDEV_pvDevDriver = (PVOID)hCtrl;

    iRet = API_PciDevMasterEnable(hPciDev, LW_TRUE);                    /*  使能 Master 模式            */
    if (iRet != ERROR_NONE) {
        NVME_LOG(NVME_LOG_ERR, "%s pci master enable failed.\r\n", hPciDev->PCIDEV_cDevName);
        return  (PX_ERROR);
    }

    iRet = API_PciDevMsixEnableSet(hPciDev, LW_TRUE);                   /*  使能 MSI-X 中断             */
    if (iRet != ERROR_NONE) {
        goto    __msi_handle;
    }

    /*
     *  分配MSI-X描述符
     */
    hCtrl->NVMECTRL_pvIntHandle = __SHEAP_ZALLOC(sizeof(PCI_MSI_DESC) * uiIrqNum);
    if (!hCtrl->NVMECTRL_pvIntHandle) {                                 /* 分配描述符失败               */
        goto    __msi_handle;
    }

    iRet = API_PciDevMsixRangeEnable(hPciDev, (PCI_MSI_DESC_HANDLE)hCtrl->NVMECTRL_pvIntHandle,
                                     1, uiIrqNum);
    if (iRet != ERROR_NONE) {
        __SHEAP_FREE(hCtrl->NVMECTRL_pvIntHandle);
        goto    __msi_handle;
    }
    hCtrl->NVMECTRL_uiIntNum = hPciDev->PCIDEV_uiDevIrqMsiNum;
    hCtrl->NVMECTRL_bMsix    = LW_TRUE;

    return  (ERROR_NONE);

__msi_handle:
    iRet = API_PciDevMsixEnableSet(hPciDev, LW_FALSE);                  /*  可能会失败，不做处理        */
    iRet = API_PciDevMsiEnableSet(hPciDev, LW_TRUE);                    /*  使能 MSI 中断               */
    if (iRet != ERROR_NONE) {
        NVME_LOG(NVME_LOG_ERR, "pci msi enable failed dev %d:%d.%d.\r\n",
                 hPciDev->PCIDEV_iDevBus, hPciDev->PCIDEV_iDevDevice, hPciDev->PCIDEV_iDevFunction);
        goto    __intx_handle;
    }

    iRet = API_PciDevMsiRangeEnable(hPciDev, 1, uiIrqNum);
    if (iRet != ERROR_NONE) {
        goto    __intx_handle;
    }
    hCtrl->NVMECTRL_uiIntNum = hPciDev->PCIDEV_uiDevIrqMsiNum;
    hCtrl->NVMECTRL_bMsix    = LW_FALSE;

    return  (ERROR_NONE);

__intx_handle:
    /*
     *  若 SylixOS 不支持当前处理器架构下的 Msi 和 Msi-X 中断模式，但支持 INTx 中断，
     *  则 NVMe 设备本身必须支持 INTx 中断模式，购买设备时需确认。
     *  此时对 Msi 和 Msi-X 中断的处理会返回错误，但仍能在 INTx 中断模式下正常工作。
     */
    iRet = API_PciDevMsiEnableSet(hPciDev, LW_FALSE);                   /* 可能会失败，不做处理         */
    hCtrl->NVMECTRL_uiIntNum = 1;
    hCtrl->NVMECTRL_bMsix    = LW_FALSE;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pciStorageNvmeVendorDrvReadyWork
** 功能描述: 驱动注册前准备操作
** 输　入  : hDrv      驱动控制句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  pciStorageNvmeVendorDrvReadyWork (NVME_DRV_HANDLE  hDrv)
{
    INT                 iRet;
    PCI_DRV_CB          tPciDrv;
    PCI_DRV_HANDLE      hPciDrv = &tPciDrv;

    lib_bzero(hPciDrv, sizeof(PCI_DRV_CB));
    iRet = pciNvmeDevIdTblGet(&hPciDrv->PCIDRV_hDrvIdTable, &hPciDrv->PCIDRV_uiDrvIdTableSize);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    lib_strlcpy(& hPciDrv->PCIDRV_cDrvName[0], NVME_PCI_DRV_NAME, PCI_DRV_NAME_MAX);
    hPciDrv->PCIDRV_pvPriv         = (PVOID)hDrv;
    hPciDrv->PCIDRV_hDrvErrHandler = LW_NULL;
    hPciDrv->PCIDRV_pfuncDevProbe  = pciStorageNvmeDevProbe;
    hPciDrv->PCIDRV_pfuncDevRemove = pciStorageNvmeDevRemove;

    iRet = API_PciDrvRegister(hPciDrv);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pciNvmeInit
** 功能描述: PCI 类型控制器驱动相关初始化
** 输　入  : NONE
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  pciStorageNvmeInit (VOID)
{
    NVME_DRV_CB         tDrvReg;
    NVME_DRV_HANDLE     hDrvReg = &tDrvReg;

    API_NvmeDrvInit();

    lib_bzero(hDrvReg, sizeof(NVME_DRV_CB));
    lib_strlcpy(&hDrvReg->NVMEDRV_cDrvName[0], NVME_PCI_DRV_NAME, NVME_DRV_NAME_MAX);

    hDrvReg->NVMEDRV_uiDrvVer                      = NVME_PCI_DRV_VER_NUM;
    hDrvReg->NVMEDRV_hCtrl                         = LW_NULL;
    hDrvReg->NVMEDRV_pfuncOptCtrl                  = pciStorageNvmeCtrlOpt;
    hDrvReg->NVMEDRV_pfuncVendorCtrlIntEnable      = pciStorageNvmeVendorCtrlIntEnable;
    hDrvReg->NVMEDRV_pfuncVendorCtrlIntConnect     = pciStorageNvmeVendorCtrlIntConnect;
    hDrvReg->NVMEDRV_pfuncVendorCtrlIntDisConnect  = pciStorageNvmeVendorCtrlIntDisConnect;
    hDrvReg->NVMEDRV_pfuncVendorCtrlReadyWork      = pciStorageNvmeVendorCtrlReadyWork;
    hDrvReg->NVMEDRV_pfuncVendorDrvReadyWork       = pciStorageNvmeVendorDrvReadyWork;

    API_NvmeDrvRegister(hDrvReg);

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PCI_EN > 0) &&      */
                                                                        /*  (LW_CFG_NVME_EN > 0)        */
                                                                        /*  (LW_CFG_DRV_M2_NVME > 0)    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
