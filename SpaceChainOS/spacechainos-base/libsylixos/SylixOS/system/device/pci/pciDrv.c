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
** 文   件   名: pciDrv.c
**
** 创   建   人: Gong.YuJian (弓羽箭)
**
** 文件创建日期: 2015 年 12 月 31 日
**
** 描        述: PCI 总线设备驱动管理.

** BUG:
2016.11.02  当系统中没有 PCI 控制器时, 调用 API_PciDrvRegister() 会产生信号量句柄无效.
            如 USB 驱动库在 ZYNQ7000 (无 PCI 控制器)平台注册驱动时, 会产生信号量句柄无效
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/shell/include/ttiny_shell.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PCI_EN > 0)
#include "pciDev.h"
#include "pciDrv.h"
#include "pciLib.h"
/*********************************************************************************************************
  PCI 主控器
*********************************************************************************************************/
extern  PCI_CTRL_HANDLE      _G_hPciCtrlHandle;
#define PCI_CTRL             _G_hPciCtrlHandle
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static UINT                     _GuiPciDrvTotalNum  = 0;
static UINT                     _GuiPciDrvActiveNum = 0;
static LW_LIST_LINE_HEADER      _GplinePciDrvHeader = LW_NULL;
static LW_OBJECT_HANDLE         _GulPciDrvLock      = LW_OBJECT_HANDLE_INVALID;
/*********************************************************************************************************
  PCI 驱动锁
*********************************************************************************************************/
#define __PCI_DRV_LOCK()        API_SemaphoreMPend(_GulPciDrvLock, LW_OPTION_WAIT_INFINITE)
#define __PCI_DRV_UNLOCK()      API_SemaphoreMPost(_GulPciDrvLock)
/*********************************************************************************************************
** 函数名称: __tshellPciDrvDevDel
** 功能描述: 删除驱动关联的设备
** 输　入  : cpcName        驱动名称
**           iAll           是否为操作全部设备
**           iBus           总线号
**           iDevice        设备号
**           iFunction      功能号
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tshellPciDrvDevDel (CPCHAR  cpcName, INT iAll, INT  iBus, INT  iDevice, INT  iFunction)
{
    INT                     iRet = PX_ERROR;
    PCI_DRV_HANDLE          hDrvHandle;
    PCI_DEV_HANDLE          hDevHandle;
    PCI_DRV_DEV_HANDLE      hDrvDevHandle;

    hDrvHandle = API_PciDrvHandleGet(cpcName);
    if (hDrvHandle == LW_NULL) {
        goto  __error_handle;
    }

    if (iAll == LW_FALSE) {
        hDevHandle = API_PciDevHandleGet(iBus, iDevice, iFunction);
        if (hDevHandle == LW_NULL) {
            goto  __error_handle;
        }
        hDrvDevHandle = API_PciDrvDevFind(hDrvHandle, hDevHandle);
        if (hDrvDevHandle == LW_NULL) {
            goto  __error_handle;
        }
    } else {
        hDevHandle = LW_NULL;
    }

    iRet = API_PciDrvDevDel(hDrvHandle, hDevHandle);
    if (iRet != ERROR_NONE) {
        goto  __error_handle;
    }

    return  (ERROR_NONE);

__error_handle:
    fprintf(stderr, "delete pci device error.\n");
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __tshellPciDrvCmdShow
** 功能描述: 打印 PCI 设备驱动列表
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __tshellPciDrvCmdShow (VOID)
{
    static PCHAR        pcPciDrvShowHdr = \
    " INDEX          DRVNAME          INDEX  BUS   DEV  FUNC VENDOR DEVICE  SUBV   SUBD   TOTAL\n"
    "------- ------------------------ ----- ----- ----- ---- ------ ------ ------ ------ -------\n";

    PLW_LIST_LINE       plineDrvTemp  = LW_NULL;
    PLW_LIST_LINE       plineDevTemp  = LW_NULL;
    PCI_DEV_HANDLE      hDevHandle    = LW_NULL;
    PCI_DRV_HANDLE      hDrvHandle    = LW_NULL;
    PCI_DRV_DEV_HANDLE  hDrvDevHandle = LW_NULL;
    REGISTER INT        i;
    REGISTER INT        j;

    printf("pci device driver total: %d active: %d.\n", _GuiPciDrvTotalNum, _GuiPciDrvActiveNum);

    printf(pcPciDrvShowHdr);

    __PCI_DRV_LOCK();                                                   /*  锁定 PCI 驱动               */
    i = 0;
    for (plineDrvTemp  = _GplinePciDrvHeader;
         plineDrvTemp != LW_NULL;
         plineDrvTemp  = _list_line_get_next(plineDrvTemp)) {
        hDrvHandle = _LIST_ENTRY(plineDrvTemp, PCI_DRV_CB, PCIDRV_lineDrvNode);
        printf("%7d %-24s %-5s %-5s %-5s %-4s %-6s %-6s %-6s %-6s %7d\n",
               i,
               hDrvHandle->PCIDRV_cDrvName,
               "", "", "", "", "", "", "", "",
               hDrvHandle->PCIDRV_uiDrvDevNum);

        j = 0;
        for (plineDevTemp  = hDrvHandle->PCIDRV_plineDrvDevHeader;
             plineDevTemp != LW_NULL;
             plineDevTemp  = _list_line_get_next(plineDevTemp)) {
            hDrvDevHandle = _LIST_ENTRY(plineDevTemp, PCI_DRV_DEV_CB, PDDCB_lineDrvDevNode);
            hDevHandle = hDrvDevHandle->PDDCB_hDrvDevHandle;
            printf("%-7s %-24s %5d 0x%03x 0x%03x  0x%1x 0x%04x 0x%04x 0x%04x 0x%04x\n",
                   "", "",
                   j,
                   hDevHandle->PCIDEV_iDevBus,
                   hDevHandle->PCIDEV_iDevDevice,
                   hDevHandle->PCIDEV_iDevFunction,
                   hDevHandle->PCIDEV_phDevHdr.PCIH_pcidHdr.PCID_usVendorId,
                   hDevHandle->PCIDEV_phDevHdr.PCIH_pcidHdr.PCID_usDeviceId,
                   hDevHandle->PCIDEV_phDevHdr.PCIH_pcidHdr.PCID_usSubVendorId,
                   hDevHandle->PCIDEV_phDevHdr.PCIH_pcidHdr.PCID_usSubSystemId);
            j += 1;
        }

        i += 1;
    }
    __PCI_DRV_UNLOCK();                                                 /*  解锁 PCI 驱动               */
}
/*********************************************************************************************************
** 函数名称: __tshellPciDrvCmd
** 功能描述: PCI 命令 "pcidrv"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tshellPciDrvCmd (INT  iArgC, PCHAR  ppcArgV[])
{
    INT     iRet;
    INT     iBus;
    INT     iDevice;
    INT     iFunction;
    INT     iAllFlag = LW_FALSE;

    if (iArgC == 1) {
        __tshellPciDrvCmdShow();

        return  (ERROR_NONE);
    }

    if ((lib_strcmp(ppcArgV[1], "add") == 0) && (iArgC == 4)) {
        /*
         *  TODO:
         */
        goto  __error_handle;
    } else if ((lib_strcmp(ppcArgV[1], "del") == 0) && (iArgC == 4)) {
        if (lib_strcmp(ppcArgV[3], "all") == 0) {
            iAllFlag = LW_TRUE;
        } else {
            iAllFlag = LW_FALSE;
            iRet = sscanf(ppcArgV[3], "%d:%d.%d", &iBus, &iDevice, &iFunction);
            if (iRet != 3) {
                fprintf(stderr, "pci device address format error.\n");
                goto  __error_handle;
            }
        }

        iRet = __tshellPciDrvDevDel(ppcArgV[2], iAllFlag, iBus, iDevice, iFunction);
        if (iRet != ERROR_NONE) {
            goto  __error_handle;
        }
    } else {
        goto  __error_handle;
    }

    return  (ERROR_NONE);

__error_handle:
    fprintf(stderr, "arguments error!\n");
    return  (-ERROR_TSHELL_EPARAM);
}
/*********************************************************************************************************
** 函数名称: API_PciDrvLoad
** 功能描述: 加载 PCI 驱动
** 输　入  : hDrvHandle     驱动句柄
**           hDevHandle     设备句柄
**           hIdEntry       当前匹配成功的一个 PCI ID 条目
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciDrvLoad (PCI_DRV_HANDLE       hDrvHandle,
                     PCI_DEV_HANDLE       hDevHandle,
                     PCI_DEV_ID_HANDLE    hIdEntry)
{
    INT     iRet;

    if ((hDrvHandle == LW_NULL) ||
        (hDevHandle == LW_NULL)) {
        return  (PX_ERROR);
    }

    if (!hIdEntry) {
        hIdEntry = hDrvHandle->PCIDRV_hDrvIdTable;
    }

    iRet = hDrvHandle->PCIDRV_pfuncDevProbe(hDevHandle, hIdEntry);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    iRet = API_PciDevDrvUpdate(hDevHandle, hDrvHandle);
    if (iRet != ERROR_NONE) {
        hDrvHandle->PCIDRV_pfuncDevRemove(hDevHandle);
        return  (PX_ERROR);
    }

    iRet = API_PciDrvDevAdd(hDrvHandle, hDevHandle);
    if (iRet != ERROR_NONE) {
        API_PciDevDrvDel(hDevHandle, hDrvHandle);
        hDrvHandle->PCIDRV_pfuncDevRemove(hDevHandle);
        return  (PX_ERROR);
    }

    if (!(hDrvHandle->PCIDRV_iDrvFlag & PCI_DRV_FLAG_ACTIVE)) {
        hDrvHandle->PCIDRV_iDrvFlag |= PCI_DRV_FLAG_ACTIVE;
        __PCI_DRV_LOCK();                                               /*  锁定 PCI 驱动               */
        _GuiPciDrvActiveNum += 1;
        __PCI_DRV_UNLOCK();                                             /*  解锁 PCI 驱动               */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciDrvDevFind
** 功能描述: 查询一个驱动上的指定设备
** 输　入  : hDrvHandle     驱动句柄
**           hDevHandle     设备句柄
** 输　出  : 驱动设备控制句柄
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
PCI_DRV_DEV_HANDLE  API_PciDrvDevFind (PCI_DRV_HANDLE  hDrvHandle, PCI_DEV_HANDLE  hDevHandle)
{
    PLW_LIST_LINE           plineTemp     = LW_NULL;
    PCI_DRV_DEV_HANDLE      hDrvDevHandle = LW_NULL;

    hDrvDevHandle = LW_NULL;
    __PCI_DRV_LOCK();                                                   /*  锁定 PCI 驱动               */
    for (plineTemp  = hDrvHandle->PCIDRV_plineDrvDevHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        hDrvDevHandle = _LIST_ENTRY(plineTemp, PCI_DRV_DEV_CB, PDDCB_lineDrvDevNode);
        if (hDrvDevHandle->PDDCB_hDrvDevHandle == hDevHandle) {
            break;
        }
    }
    __PCI_DRV_UNLOCK();                                                 /*  解锁 PCI 驱动               */

    if (plineTemp) {
        return  (hDrvDevHandle);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** 函数名称: API_PciDrvDevDel
** 功能描述: 指定 PCI 驱动删除一个设备
** 输　入  : hDrvHandle     驱动控制块句柄
**           hDevHandle     设备控制块句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciDrvDevDel (PCI_DRV_HANDLE  hDrvHandle, PCI_DEV_HANDLE  hDevHandle)
{
    PLW_LIST_LINE           plineTemp = LW_NULL;
    PCI_DRV_DEV_HANDLE      hDrvDevHandle = LW_NULL;

    if (hDrvHandle == LW_NULL) {
        return  (PX_ERROR);
    }

    if (hDevHandle == LW_NULL) {
        __PCI_DRV_UNLOCK();
        plineTemp = hDrvHandle->PCIDRV_plineDrvDevHeader;
        while (plineTemp) {
            hDrvDevHandle = _LIST_ENTRY(plineTemp, PCI_DRV_DEV_CB, PDDCB_lineDrvDevNode);
            plineTemp     = _list_line_get_next(plineTemp);
            
            _List_Line_Del(&hDrvDevHandle->PDDCB_lineDrvDevNode, &hDrvHandle->PCIDRV_plineDrvDevHeader);
            hDrvHandle->PCIDRV_uiDrvDevNum -= 1;
            if (hDrvHandle->PCIDRV_uiDrvDevNum < 1) {
                hDrvHandle->PCIDRV_iDrvFlag &= ~(PCI_DRV_FLAG_ACTIVE);
                hDrvHandle->PCIDRV_plineDrvDevHeader = LW_NULL;
            }
            
            API_PciDevDrvDel(hDrvDevHandle->PDDCB_hDrvDevHandle, hDrvHandle);
            hDrvHandle->PCIDRV_pfuncDevRemove(hDrvDevHandle->PDDCB_hDrvDevHandle);
            hDrvDevHandle->PDDCB_hDrvDevHandle = LW_NULL;
            __SHEAP_FREE(hDrvDevHandle);
        }
        __PCI_DRV_UNLOCK();

        API_PciDrvDelete(hDrvHandle);

        return  (ERROR_NONE);
    }

    hDrvDevHandle = API_PciDrvDevFind(hDrvHandle, hDevHandle);
    if (hDrvDevHandle == LW_NULL) {
        return  (PX_ERROR);
    }

    __PCI_DRV_LOCK();                                                   /*  锁定 PCI 驱动               */
    _List_Line_Del(&hDrvDevHandle->PDDCB_lineDrvDevNode, &hDrvHandle->PCIDRV_plineDrvDevHeader);
    hDrvHandle->PCIDRV_uiDrvDevNum -= 1;
    if (hDrvHandle->PCIDRV_uiDrvDevNum < 1) {
        hDrvHandle->PCIDRV_iDrvFlag &= ~(PCI_DRV_FLAG_ACTIVE);
        hDrvHandle->PCIDRV_plineDrvDevHeader = LW_NULL;
    }
    
    API_PciDevDrvDel(hDrvDevHandle->PDDCB_hDrvDevHandle, hDrvHandle);
    hDrvHandle->PCIDRV_pfuncDevRemove(hDrvDevHandle->PDDCB_hDrvDevHandle);
    hDrvDevHandle->PDDCB_hDrvDevHandle = LW_NULL;
    __PCI_DRV_UNLOCK();                                                 /*  解锁 PCI 驱动               */

    __SHEAP_FREE(hDrvDevHandle);

    API_PciDrvDelete(hDrvHandle);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciDrvDevAdd
** 功能描述: 指定 PCI 驱动增加一个设备
** 输　入  : hDrvHandle     驱动控制块句柄
**           hDevHandle     设备控制块句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciDrvDevAdd (PCI_DRV_HANDLE  hDrvHandle, PCI_DEV_HANDLE  hDevHandle)
{
    PCI_DRV_DEV_HANDLE      hDrvDevHandle = LW_NULL;

    if ((hDrvHandle == LW_NULL) ||
        (hDevHandle == LW_NULL)) {
        return  (PX_ERROR);
    }

    hDrvDevHandle = API_PciDrvDevFind(hDrvHandle, hDevHandle);
    if (hDrvDevHandle != LW_NULL) {
        return  (ERROR_NONE);
    }

    hDrvDevHandle = (PCI_DRV_DEV_HANDLE)__SHEAP_ZALLOC(sizeof(PCI_DRV_DEV_CB));
    if (hDrvDevHandle == LW_NULL) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }

    __PCI_DRV_LOCK();                                                   /*  锁定 PCI 驱动               */
    hDrvDevHandle->PDDCB_hDrvDevHandle = hDevHandle;
    _List_Line_Add_Ahead(&hDrvDevHandle->PDDCB_lineDrvDevNode, &hDrvHandle->PCIDRV_plineDrvDevHeader);
    hDrvHandle->PCIDRV_uiDrvDevNum += 1;
    __PCI_DRV_UNLOCK();                                                 /*  解锁 PCI 驱动               */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciDrvHandleGet
** 功能描述: 获取一个驱动的句柄
** 输　入  : cpcName       驱动名称
** 输　出  : 设备控制句柄
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
PCI_DRV_HANDLE  API_PciDrvHandleGet (CPCHAR  cpcName)
{
    PLW_LIST_LINE       plineTemp  = LW_NULL;
    PCI_DRV_HANDLE      hDrvHandle = LW_NULL;

    hDrvHandle = LW_NULL;
    __PCI_DRV_LOCK();                                                   /*  锁定 PCI 驱动               */
    for (plineTemp  = _GplinePciDrvHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        hDrvHandle = _LIST_ENTRY(plineTemp, PCI_DRV_CB, PCIDRV_lineDrvNode);
        if (lib_strncmp(&hDrvHandle->PCIDRV_cDrvName[0], cpcName, PCI_DRV_NAME_MAX) == 0) {
            break;
        }
    }
    __PCI_DRV_UNLOCK();                                                 /*  解锁 PCI 驱动               */

    if (plineTemp) {
        return  (hDrvHandle);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** 函数名称: API_PciDrvDelete
** 功能描述: 删除一个 PCI 驱动
** 输　入  : hDrvHandle     驱动控制块句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_PciDrvDelete (PCI_DRV_HANDLE  hDrvHandle)
{
    if (hDrvHandle == LW_NULL) {
        return  (PX_ERROR);
    }

    if ((hDrvHandle->PCIDRV_iDrvFlag & PCI_DRV_FLAG_ACTIVE) ||
        (hDrvHandle->PCIDRV_uiDrvDevNum > 0)) {
        return  (PX_ERROR);
    }

    __PCI_DRV_LOCK();                                                   /*  锁定 PCI 驱动               */
    if (!(hDrvHandle->PCIDRV_iDrvFlag & PCI_DRV_FLAG_ACTIVE)) {
        _GuiPciDrvActiveNum -= 1;
    }
    _List_Line_Del(&hDrvHandle->PCIDRV_lineDrvNode, &_GplinePciDrvHeader);
    _GuiPciDrvTotalNum -= 1;
    __PCI_DRV_UNLOCK();                                                 /*  解锁 PCI 驱动               */

    __SHEAP_FREE(hDrvHandle);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciDrvRegister
** 功能描述: 注册 PCI 驱动
** 输　入  : hHandle       驱动注册控制块句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciDrvRegister (PCI_DRV_HANDLE  hHandle)
{
    PCI_DRV_HANDLE      hDrvHandle = LW_NULL;

    if (PCI_CTRL == LW_NULL) {
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }

    if (!hHandle) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if ((hHandle->PCIDRV_cDrvName[0]      == PX_EOS ) ||
        (hHandle->PCIDRV_uiDrvIdTableSize <  1      ) ||
        (hHandle->PCIDRV_hDrvIdTable      == LW_NULL) ||
        (hHandle->PCIDRV_pfuncDevProbe    == LW_NULL) ||
        (hHandle->PCIDRV_pfuncDevRemove   == LW_NULL)) {
        return  (PX_ERROR);
    }

    hDrvHandle = API_PciDrvHandleGet(&hHandle->PCIDRV_cDrvName[0]);
    if (hDrvHandle != LW_NULL) {
        return  (PX_ERROR);
    }

    hDrvHandle = (PCI_DRV_HANDLE)__SHEAP_ZALLOC(sizeof(PCI_DRV_CB));
    if (hDrvHandle == LW_NULL) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }

    lib_strlcpy(&hDrvHandle->PCIDRV_cDrvName[0], &hHandle->PCIDRV_cDrvName[0], PCI_DRV_NAME_MAX);
    hDrvHandle->PCIDRV_hDrvIdTable      = hHandle->PCIDRV_hDrvIdTable;
    hDrvHandle->PCIDRV_uiDrvIdTableSize = hHandle->PCIDRV_uiDrvIdTableSize;

    hDrvHandle->PCIDRV_pfuncDevProbe       = hHandle->PCIDRV_pfuncDevProbe;
    hDrvHandle->PCIDRV_pfuncDevRemove      = hHandle->PCIDRV_pfuncDevRemove;
    hDrvHandle->PCIDRV_pfuncDevSuspend     = hHandle->PCIDRV_pfuncDevSuspend;
    hDrvHandle->PCIDRV_pfuncDevSuspendLate = hHandle->PCIDRV_pfuncDevSuspendLate;
    hDrvHandle->PCIDRV_pfuncDevResumeEarly = hHandle->PCIDRV_pfuncDevResumeEarly;
    hDrvHandle->PCIDRV_pfuncDevResume      = hHandle->PCIDRV_pfuncDevResume;
    hDrvHandle->PCIDRV_pfuncDevShutdown    = hHandle->PCIDRV_pfuncDevShutdown;
    hDrvHandle->PCIDRV_hDrvErrHandler      = hHandle->PCIDRV_hDrvErrHandler;

    hDrvHandle->PCIDRV_iDrvFlag         &= ~(PCI_DRV_FLAG_ACTIVE);
    hDrvHandle->PCIDRV_uiDrvDevNum       = 0;
    hDrvHandle->PCIDRV_plineDrvDevHeader = LW_NULL;

    __PCI_DRV_LOCK();                                                   /*  锁定 PCI 驱动               */
    _List_Line_Add_Ahead(&hDrvHandle->PCIDRV_lineDrvNode, &_GplinePciDrvHeader);
    _GuiPciDrvTotalNum += 1;
    if (hDrvHandle->PCIDRV_iDrvFlag & PCI_DRV_FLAG_ACTIVE) {
        _GuiPciDrvActiveNum += 1;
    }
    __PCI_DRV_UNLOCK();                                                 /*  解锁 PCI 驱动               */

    API_PciDrvBindEachDev(hDrvHandle);                                  /*  尝试绑定设备                */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciDevBindEachDrv
** 功能描述: 对一个新的PCI设备, 尝试与系统中的驱动进行绑定(适用于热插拔支持)
** 输　入  : hDevHandle     设备句柄
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 代码内部只会调用 DRV_LOCK 所以不会发生死锁.

                                           API 函数
*********************************************************************************************************/
LW_API
VOID  API_PciDevBindEachDrv (PCI_DEV_HANDLE hDevHandle)
{
    PCI_DRV_HANDLE        hDrvCurr;
    PLW_LIST_LINE         plineTemp;
    PCI_DEV_ID_HANDLE     hId;
    INT                   iRet;

    if (hDevHandle->PCIDEV_pvDevDriver) {
        return;
    }

    __PCI_DRV_LOCK();                                                   /*  锁定 PCI 驱动               */
    for (plineTemp  = _GplinePciDrvHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {

        hDrvCurr = _LIST_ENTRY(plineTemp, PCI_DRV_CB, PCIDRV_lineDrvNode);

        hId = API_PciDevMatchDrv(hDevHandle, hDrvCurr);
        if (!hId) {
            continue;                                                   /*  ID 不匹配                   */
        }

        /*
         * 如果探测失败, 不直接返回. 因为系统中可能还存在合适的驱动
         * 这完全由 PCI 设备驱动编写者决定
         */
        iRet = API_PciDrvLoad(hDrvCurr, hDevHandle, hId);
        if (iRet == ERROR_NONE) {
            break;
        }
    }
    __PCI_DRV_UNLOCK();                                                 /*  解锁 PCI 驱动               */
}
/*********************************************************************************************************
** 函数名称: API_PciDrvInit
** 功能描述: PCI 驱动注册初始化, 使用注册接口前应先初始化
** 输　入  : NONE
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciDrvInit (VOID)
{
    _GuiPciDrvTotalNum  = 0;
    _GuiPciDrvActiveNum = 0;
    _GplinePciDrvHeader = LW_NULL;
    _GulPciDrvLock      = LW_OBJECT_HANDLE_INVALID;

    _GulPciDrvLock = API_SemaphoreMCreate("pci_drv_lock",
                                          LW_PRIO_DEF_CEILING,
                                          LW_OPTION_WAIT_PRIORITY |
                                          LW_OPTION_DELETE_SAFE |
                                          LW_OPTION_INHERIT_PRIORITY |
                                          LW_OPTION_OBJECT_GLOBAL,
                                          LW_NULL);
    if (_GulPciDrvLock == LW_OBJECT_HANDLE_INVALID) {
        return  (PX_ERROR);
    }

    API_TShellKeywordAdd("pcidrv", __tshellPciDrvCmd);
    API_TShellFormatAdd("pcidrv", " [add | del] [driver] [[all] | name]");
    API_TShellHelpAdd("pcidrv", "show, add, del pci driver table\n"
                                "eg. pcidrv\n"
                                "    pcidrv add ahci all (Invalid)\n"
                                "    pcidrv add ahci 1:0.1 (Invalid)\n"
                                "    pcidrv del ahci 1:0.1\n");

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PCI_EN > 0)         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
