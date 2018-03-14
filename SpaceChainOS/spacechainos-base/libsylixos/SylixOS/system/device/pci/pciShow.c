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
** 文   件   名: pciShow.c
**
** 创   建   人: Gong.YuJian (弓羽箭)
**
** 文件创建日期: 2015 年 09 月 17 日
**
** 描        述: PCI 总线信息打印.
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
#include "pciDev.h"
#include "pciLib.h"
#include "pciShow.h"
/*********************************************************************************************************
  PCI 主控器
*********************************************************************************************************/
extern  PCI_CTRL_HANDLE     _G_hPciCtrlHandle;
#define PCI_CTRL            _G_hPciCtrlHandle
INT                         _G_iPciVerbose = 0;
/*********************************************************************************************************
** 函数名称: API_PciBusDeviceShow
** 功能描述: 打印某个总线上所有设备的信息.
** 输　入  : iBus      总线号
** 输　出  : NONE
** 全局变量: ERROR or OK
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciBusDeviceShow (INT iBus)
{
    INT         iSlot;
    UINT16      usVendorId;
    UINT16      usDeviceId;
    UINT32      uiClassCode;
    UINT8       ucHeaderType;
    INT         iFunc;

    if ((PCI_CTRL->PCI_ucMechanism == PCI_MECHANISM_2) &&
        (PCI_MAX_SLOTS                     >  16             )) {
        printf ("Invalid configuration. PCI_MAX_SLOTS > 16, PCI mechanism #2\n");
        return  (PX_ERROR);
    }

    printf("Scanning functions of each PCI device on bus %d\n", iBus);
    printf("Using configuration mechanism %d\n", PCI_CTRL->PCI_ucMechanism);
    printf("bus       device    function  vendorID  deviceID  class/rev\n");

    for (iSlot = 0; iSlot < PCI_MAX_SLOTS; iSlot++) {
        for (iFunc = 0; iFunc < PCI_MAX_FUNCTIONS; iFunc++) {
            if ((iSlot == (PCI_MAX_SLOTS     - 1)) &&
                (iFunc == (PCI_MAX_FUNCTIONS - 1))) {
                continue;
            }
            
            API_PciConfigInWord(iBus, iSlot, iFunc, PCI_VENDOR_ID, &usVendorId);
            if (usVendorId == 0xFFFF) {
                if (iFunc == 0) {
                    break;
                }
                continue;
            }

            API_PciConfigInWord( iBus, iSlot, iFunc, PCI_DEVICE_ID,      &usDeviceId);
            API_PciConfigInDword(iBus, iSlot, iFunc, PCI_CLASS_REVISION, &uiClassCode);
            API_PciConfigInByte( iBus, iSlot, iFunc, PCI_HEADER_TYPE,    &ucHeaderType);

            printf("%7d   %6d    %8d   0x%04x    0x%04x  0x%08x\n",
                   iBus, iSlot, iFunc, usVendorId, usDeviceId, uiClassCode);

            if ((iFunc == 0                                 ) &&
                ((ucHeaderType & PCI_HEADER_MULTI_FUNC) == 0)) {
                break;
            }
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciFindDevShow
** 功能描述: 打印查找到的设备信息.
** 输　入  : usVendorId    供应商号
**           usDeviceId    设备 ID
**           iInstance     第几个设备实例
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciFindDevShow (UINT16 usVendorId, UINT16 usDeviceId, INT iInstance)
{
    INT     iRet  = PX_ERROR;
    INT     iBus  = 0;
    INT     iSlot = 0;
    INT     iFunc = 0;

    iRet = API_PciFindDev(usVendorId, usDeviceId, iInstance, &iBus, &iSlot, &iFunc);
    if (iRet == ERROR_NONE) {
        printf("deviceId = 0x%.8x\n", usDeviceId);
        printf("vendorId = 0x%.8x\n", usVendorId);
        printf("index    = 0x%.8x\n", iInstance);
        printf("busNo  = 0x%.8x\n", iBus);
        printf("slotNo = 0x%.8x\n", iSlot);
        printf("funcNo = 0x%.8x\n", iFunc);
    }

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_PciFindClassShow
** 功能描述: 打印指定设备类型的 pci 设备
** 输　入  : usClassCode   类型编码 (PCI_CLASS_STORAGE_SCSI, PCI_CLASS_DISPLAY_XGA ...)
**           iInstance     第几个设备实例
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciFindClassShow (UINT16 usClassCode, INT iInstance)
{
    INT     iRet  = PX_ERROR;
    INT     iBus  = 0;
    INT     iSlot = 0;
    INT     iFunc = 0;

    iRet = API_PciFindClass(usClassCode, iInstance, &iBus, &iSlot, &iFunc);
    if (iRet == ERROR_NONE) {
        printf("class code = 0x%.8x\n", usClassCode);
        printf("index      = 0x%.8x\n", iInstance);
        printf("busNo  = 0x%.8x\n", iBus);
        printf("slotNo = 0x%.8x\n", iSlot);
        printf("funcNo = 0x%.8x\n", iFunc);
    }

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __pciDeviceHeaderShow
** 功能描述: 打印 PCI 设备头
** 输　入  : phdrDevice  设备头控制句柄
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciDeviceHeaderShow (PCI_DEV_HDR *phdrDevice)
{
    printf("vendor ID =                   0x%.4x\n", (UINT16)phdrDevice->PCID_usVendorId);
    printf("device ID =                   0x%.4x\n", (UINT16)phdrDevice->PCID_usDeviceId);
    printf("command register =            0x%.4x\n", (UINT16)phdrDevice->PCID_usCommand);
    printf("status register =             0x%.4x\n", (UINT16)phdrDevice->PCID_usStatus);
    printf("revision ID =                 0x%.2x\n", (UINT8)phdrDevice->PCID_ucRevisionId);
    printf("class code =                  0x%.2x\n", (UINT8)phdrDevice->PCID_ucClassCode);
    printf("sub class code =              0x%.2x\n", (UINT8)phdrDevice->PCID_ucSubClass);
    printf("programming interface =       0x%.2x\n", (UINT8)phdrDevice->PCID_ucProgIf);
    printf("cache line =                  0x%.2x\n", (UINT8)phdrDevice->PCID_ucCacheLine);
    printf("latency time =                0x%.2x\n", (UINT8)phdrDevice->PCID_ucLatency);
    printf("header type =                 0x%.2x\n", (UINT8)phdrDevice->PCID_ucHeaderType);
    printf("BIST =                        0x%.2x\n", (UINT8)phdrDevice->PCID_ucBist);
    printf("base address 0 =              0x%.8x\n", phdrDevice->PCID_uiBase[PCI_BAR_INDEX_0]);
    printf("base address 1 =              0x%.8x\n", phdrDevice->PCID_uiBase[PCI_BAR_INDEX_1]);
    printf("base address 2 =              0x%.8x\n", phdrDevice->PCID_uiBase[PCI_BAR_INDEX_2]);
    printf("base address 3 =              0x%.8x\n", phdrDevice->PCID_uiBase[PCI_BAR_INDEX_3]);
    printf("base address 4 =              0x%.8x\n", phdrDevice->PCID_uiBase[PCI_BAR_INDEX_4]);
    printf("base address 5 =              0x%.8x\n", phdrDevice->PCID_uiBase[PCI_BAR_INDEX_5]);
    printf("cardBus CIS pointer =         0x%.8x\n", phdrDevice->PCID_uiCis);
    printf("sub system vendor ID =        0x%.4x\n", (UINT16)phdrDevice->PCID_usSubVendorId);
    printf("sub system ID =               0x%.4x\n", (UINT16)phdrDevice->PCID_usSubSystemId);
    printf("expansion ROM base address =  0x%.8x\n", phdrDevice->PCID_uiRomBase);
    printf("interrupt line =              0x%.2x\n", (UINT8)phdrDevice->PCID_ucIntLine);
    printf("interrupt pin =               0x%.2x\n", (UINT8)phdrDevice->PCID_ucIntPin);
    printf("min Grant =                   0x%.2x\n", (UINT8)phdrDevice->PCID_ucMinGrant);
    printf("max Latency =                 0x%.2x\n", (UINT8)phdrDevice->PCID_ucMaxLatency);
}
/*********************************************************************************************************
** 函数名称: __pciBridgeHeaderShow
** 功能描述: 打印 PCI 桥设备头
** 输　入  : phdrBridge  桥设备头控制句柄
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciBridgeHeaderShow (PCI_BRG_HDR *phdrBridge)
{
    printf("vendor ID =                   0x%.4x\n", (UINT16)phdrBridge->PCIB_usVendorId);
    printf("device ID =                   0x%.4x\n", (UINT16)phdrBridge->PCIB_usDeviceId);
    printf("command register =            0x%.4x\n", (UINT16)phdrBridge->PCIB_usCommand);
    printf("status register =             0x%.4x\n", (UINT16)phdrBridge->PCIB_usStatus);
    printf("revision ID =                 0x%.2x\n", (UINT8)phdrBridge->PCIB_ucRevisionId);
    printf("class code =                  0x%.2x\n", (UINT8)phdrBridge->PCIB_ucClassCode);
    printf("sub class code =              0x%.2x\n", (UINT8)phdrBridge->PCIB_ucSubClass);
    printf("programming interface =       0x%.2x\n", (UINT8)phdrBridge->PCIB_ucProgIf);
    printf("cache line =                  0x%.2x\n", (UINT8)phdrBridge->PCIB_ucCacheLine);
    printf("latency time =                0x%.2x\n", (UINT8)phdrBridge->PCIB_ucLatency);
    printf("header type =                 0x%.2x\n", (UINT8)phdrBridge->PCIB_ucHeaderType);
    printf("BIST =                        0x%.2x\n", (UINT8)phdrBridge->PCIB_ucBist);
    printf("base address 0 =              0x%.8x\n", phdrBridge->PCIB_uiBase[PCI_BAR_INDEX_0]);
    printf("base address 1 =              0x%.8x\n", phdrBridge->PCIB_uiBase[PCI_BAR_INDEX_1]);
    printf("primary bus number =          0x%.2x\n", (UINT8)phdrBridge->PCIB_ucPriBus);
    printf("secondary bus number =        0x%.2x\n", (UINT8)phdrBridge->PCIB_ucSecBus);
    printf("subordinate bus number =      0x%.2x\n", (UINT8)phdrBridge->PCIB_ucSubBus);
    printf("secondary latency timer =     0x%.2x\n", (UINT8)phdrBridge->PCIB_ucSecLatency);
    printf("IO base =                     0x%.2x\n", (UINT8)phdrBridge->PCIB_ucIoBase);
    printf("IO limit =                    0x%.2x\n", (UINT8)phdrBridge->PCIB_ucIoLimit);
    printf("secondary status =            0x%.4x\n", (UINT16)phdrBridge->PCIB_usSecStatus);
    printf("memory base =                 0x%.4x\n", (UINT16)phdrBridge->PCIB_usMemBase);
    printf("memory limit =                0x%.4x\n", (UINT16)phdrBridge->PCIB_usMemLimit);
    printf("prefetch memory base =        0x%.4x\n", (UINT16)phdrBridge->PCIB_usPreBase);
    printf("prefetch memory limit =       0x%.4x\n", (UINT16)phdrBridge->PCIB_usPreLimit);
    printf("prefetch memory base upper =  0x%.8x\n", phdrBridge->PCIB_uiPreBaseUpper);
    printf("prefetch memory limit upper = 0x%.8x\n", phdrBridge->PCIB_uiPreLimitUpper);
    printf("IO base upper 16 bits =       0x%.4x\n", (UINT16)phdrBridge->PCIB_usIoBaseUpper);
    printf("IO limit upper 16 bits =      0x%.4x\n", (UINT16)phdrBridge->PCIB_usIoLimitUpper);
    printf("expansion ROM base address =  0x%.8x\n", phdrBridge->PCIB_uiRomBase);
    printf("interrupt line =              0x%.2x\n", (UINT8)phdrBridge->PCIB_ucIntLine);
    printf("interrupt pin =               0x%.2x\n", (UINT8)phdrBridge->PCIB_ucIntPin);
    printf("bridge control =              0x%.4x\n", (UINT16)phdrBridge->PCIB_usControl);
}
/*********************************************************************************************************
** 函数名称: __pciCardBridgeHeaderShow
** 功能描述: 打印 PCI 卡桥设备头
** 输　入  : phdrBridge  桥设备头控制句柄
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciCardBridgeHeaderShow (PCI_CBUS_HDR *phdrCardBridge)
{
    printf("vendor ID =                   0x%.4x\n", (UINT16)phdrCardBridge->PCICB_usVendorId);
    printf("device ID =                   0x%.4x\n", (UINT16)phdrCardBridge->PCICB_usDeviceId);
    printf("command register =            0x%.4x\n", (UINT16)phdrCardBridge->PCICB_usCommand);
    printf("status register =             0x%.4x\n", (UINT16)phdrCardBridge->PCICB_usStatus);
    printf("revision ID =                 0x%.2x\n", (UINT8)phdrCardBridge->PCICB_ucRevisionId);
    printf("class code =                  0x%.2x\n", (UINT8)phdrCardBridge->PCICB_ucClassCode);
    printf("sub class code =              0x%.2x\n", (UINT8)phdrCardBridge->PCICB_ucSubClass);
    printf("programming interface =       0x%.2x\n", (UINT8)phdrCardBridge->PCICB_ucProgIf);
    printf("cache line =                  0x%.2x\n", (UINT8)phdrCardBridge->PCICB_ucCacheLine);
    printf("latency time =                0x%.2x\n", (UINT8)phdrCardBridge->PCICB_ucLatency);
    printf("header type =                 0x%.2x\n", (UINT8)phdrCardBridge->PCICB_ucHeaderType);
    printf("BIST =                        0x%.2x\n", (UINT8)phdrCardBridge->PCICB_ucBist);
    printf("base address 0 =              0x%.8x\n", phdrCardBridge->PCICB_uiBase[PCI_BAR_INDEX_0]);
    printf("capabilities pointer =        0x%.2x\n", (UINT8)phdrCardBridge->PCICB_ucCapPtr);
    printf("secondary status =            0x%.4x\n", (UINT16)phdrCardBridge->PCICB_usSecStatus);
    printf("primary bus number =          0x%.2x\n", (UINT8)phdrCardBridge->PCICB_ucPriBus);
    printf("secondary bus number =        0x%.2x\n", (UINT8)phdrCardBridge->PCICB_ucSecBus);
    printf("subordinate bus number =      0x%.2x\n", (UINT8)phdrCardBridge->PCICB_ucSubBus);
    printf("secondary latency timer =     0x%.2x\n", (UINT8)phdrCardBridge->PCICB_ucSecLatency);
    printf("memory base 0 =               0x%.8x\n", phdrCardBridge->PCICB_uiMemBase0);
    printf("memory limit 0 =              0x%.8x\n", phdrCardBridge->PCICB_uiMemLimit0);
    printf("memory base 1 =               0x%.8x\n", phdrCardBridge->PCICB_uiMemBase1);
    printf("memory limit 1 =              0x%.8x\n", phdrCardBridge->PCICB_uiMemLimit1);
    printf("IO base 0 =                   0x%.8x\n", phdrCardBridge->PCICB_uiIoBase0);
    printf("IO limit 0 =                  0x%.8x\n", phdrCardBridge->PCICB_uiIoLimit0);
    printf("IO base 1 =                   0x%.8x\n", phdrCardBridge->PCICB_uiIoBase1);
    printf("IO limit 1 =                  0x%.8x\n", phdrCardBridge->PCICB_uiIoLimit1);
    printf("interrupt line =              0x%.2x\n", (UINT8)phdrCardBridge->PCICB_ucIntLine);
    printf("interrupt pin =               0x%.2x\n", (UINT8)phdrCardBridge->PCICB_ucIntPin);
    printf("bridge control =              0x%.4x\n", (UINT16)phdrCardBridge->PCICB_usControl);
    printf("sub system vendor ID =        0x%.4x\n", (UINT16)phdrCardBridge->PCICB_usSubVendorId);
    printf("sub system ID =               0x%.4x\n", (UINT16)phdrCardBridge->PCICB_usSubSystemId);
    printf("16 bit legacy mode base =     0x%.8x\n", phdrCardBridge->PCICB_uiLegacyBase);
}
/*********************************************************************************************************
** 函数名称: API_PciHeaderShow
** 功能描述: 打印设备头信息.
** 输　入  : iBus        总线号
**           iSlot       插槽
**           iFunc       功能
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciHeaderShow (INT iBus, INT iSlot, INT iFunc)
{
    PCI_DEV_HDR         hdrDevice;
    PCI_BRG_HDR         hdrBridge;
    PCI_CBUS_HDR        hdrCardBridge;
    PCI_DEV_HDR        *phdrDevice     = &hdrDevice;
    PCI_BRG_HDR        *phdrBridge     = &hdrBridge;
    PCI_CBUS_HDR       *phdrCardBridge = &hdrCardBridge;

    API_PciConfigInByte(iBus, iSlot, iFunc, PCI_HEADER_TYPE, (UINT8 *)&phdrDevice->PCID_ucHeaderType);
    
    if ((phdrDevice->PCID_ucHeaderType & PCI_HEADER_TYPE_MASK) == PCI_HEADER_TYPE_BRIDGE) {
        API_PciConfigInWord(iBus, iSlot, iFunc, PCI_VENDOR_ID,
                            (UINT16 *)&phdrBridge->PCIB_usVendorId);
        API_PciConfigInWord(iBus, iSlot, iFunc, PCI_DEVICE_ID,
                            (UINT16 *)&phdrBridge->PCIB_usDeviceId);
        API_PciConfigInWord(iBus, iSlot, iFunc, PCI_COMMAND,
                            (UINT16 *)&phdrBridge->PCIB_usCommand);
        API_PciConfigInWord(iBus, iSlot, iFunc, PCI_STATUS,
                            (UINT16 *)&phdrBridge->PCIB_usStatus);
        API_PciConfigInByte(iBus, iSlot, iFunc, PCI_REVISION_ID,
                            (UINT8 *)&phdrBridge->PCIB_ucRevisionId);
        API_PciConfigInByte(iBus, iSlot, iFunc, PCI_CLASS_PROG,
                            (UINT8 *)&phdrBridge->PCIB_ucProgIf);
        API_PciConfigInByte(iBus, iSlot, iFunc, PCI_CLASS_DEVICE,
                            (UINT8 *)&phdrBridge->PCIB_ucSubClass);
        API_PciConfigInByte(iBus, iSlot, iFunc, PCI_CLASS,
                            (UINT8 *)&phdrBridge->PCIB_ucClassCode);
        API_PciConfigInByte(iBus, iSlot, iFunc, PCI_CACHE_LINE_SIZE,
                            (UINT8 *)&phdrBridge->PCIB_ucCacheLine);
        API_PciConfigInByte(iBus, iSlot, iFunc, PCI_LATENCY_TIMER,
                            (UINT8 *)&phdrBridge->PCIB_ucLatency);
        API_PciConfigInByte(iBus, iSlot, iFunc, PCI_HEADER_TYPE,
                            (UINT8 *)&phdrBridge->PCIB_ucHeaderType);
        API_PciConfigInByte(iBus, iSlot, iFunc, PCI_BIST,
                            (UINT8 *)&phdrBridge->PCIB_ucBist);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_BASE_ADDRESS_0,
                             (UINT32 *)&phdrBridge->PCIB_uiBase[PCI_BAR_INDEX_0]);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_BASE_ADDRESS_1,
                             (UINT32 *)&phdrBridge->PCIB_uiBase[PCI_BAR_INDEX_1]);
        API_PciConfigInByte(iBus, iSlot, iFunc, PCI_PRIMARY_BUS,
                            (UINT8 *)&phdrBridge->PCIB_ucPriBus);
        API_PciConfigInByte(iBus, iSlot, iFunc, PCI_SECONDARY_BUS,
                            (UINT8 *)&phdrBridge->PCIB_ucSecBus);
        API_PciConfigInByte(iBus, iSlot, iFunc, PCI_SUBORDINATE_BUS,
                            (UINT8 *)&phdrBridge->PCIB_ucSubBus);
        API_PciConfigInByte(iBus, iSlot, iFunc, PCI_SEC_LATENCY_TIMER,
                            (UINT8 *)&phdrBridge->PCIB_ucSecLatency);
        API_PciConfigInByte(iBus, iSlot, iFunc, PCI_IO_BASE,
                            (UINT8 *)&phdrBridge->PCIB_ucIoBase);
        API_PciConfigInByte(iBus, iSlot, iFunc, PCI_IO_LIMIT,
                            (UINT8 *)&phdrBridge->PCIB_ucIoLimit);
        API_PciConfigInWord(iBus, iSlot, iFunc, PCI_SEC_STATUS,
                            (UINT16 *)&phdrBridge->PCIB_usSecStatus);
        API_PciConfigInWord(iBus, iSlot, iFunc, PCI_MEMORY_BASE,
                            (UINT16 *)&phdrBridge->PCIB_usMemBase);
        API_PciConfigInWord(iBus, iSlot, iFunc, PCI_MEMORY_LIMIT,
                            (UINT16 *)&phdrBridge->PCIB_usMemLimit);
        API_PciConfigInWord(iBus, iSlot, iFunc, PCI_PREF_MEMORY_BASE,
                            (UINT16 *)&phdrBridge->PCIB_usPreBase);
        API_PciConfigInWord(iBus, iSlot, iFunc, PCI_PREF_MEMORY_LIMIT,
                            (UINT16 *)&phdrBridge->PCIB_usPreLimit);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_PREF_BASE_UPPER32,
                             (UINT32 *)&phdrBridge->PCIB_uiPreBaseUpper);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_PREF_LIMIT_UPPER32,
                             (UINT32 *)&phdrBridge->PCIB_uiPreLimitUpper);
        API_PciConfigInWord(iBus, iSlot, iFunc, PCI_IO_BASE_UPPER16,
                            (UINT16 *)&phdrBridge->PCIB_usIoBaseUpper);
        API_PciConfigInWord(iBus, iSlot, iFunc, PCI_IO_LIMIT_UPPER16,
                            (UINT16 *)&phdrBridge->PCIB_usIoLimitUpper);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_ROM_ADDRESS1,
                             (UINT32 *)&phdrBridge->PCIB_uiRomBase);
        API_PciConfigInByte(iBus, iSlot, iFunc, PCI_INTERRUPT_LINE,
                            (UINT8 *)&phdrBridge->PCIB_ucIntLine);
        API_PciConfigInByte(iBus, iSlot, iFunc, PCI_INTERRUPT_PIN,
                            (UINT8 *)&phdrBridge->PCIB_ucIntPin);
        API_PciConfigInWord(iBus, iSlot, iFunc, PCI_BRIDGE_CONTROL,
                             (UINT16 *)&phdrBridge->PCIB_usControl);
        __pciBridgeHeaderShow(phdrBridge);

        if (phdrBridge->PCIB_usStatus & PCI_STATUS_CAP_LIST) {
            API_PciCapShow(iBus, iSlot, iFunc);
        }
    
    } else if ((phdrDevice->PCID_ucHeaderType & PCI_HEADER_TYPE_MASK) == PCI_HEADER_TYPE_CARDBUS) {
        API_PciConfigInWord(iBus, iSlot, iFunc, PCI_VENDOR_ID,
                            (UINT16 *)&phdrCardBridge->PCICB_usVendorId);
        API_PciConfigInWord(iBus, iSlot, iFunc, PCI_DEVICE_ID,
                            (UINT16 *)&phdrCardBridge->PCICB_usDeviceId);
        API_PciConfigInWord(iBus, iSlot, iFunc, PCI_COMMAND,
                            (UINT16 *)&phdrCardBridge->PCICB_usCommand);
        API_PciConfigInWord(iBus, iSlot, iFunc, PCI_STATUS,
                            (UINT16 *)&phdrCardBridge->PCICB_usStatus);
        API_PciConfigInByte(iBus, iSlot, iFunc, PCI_REVISION_ID,
                            (UINT8 *)&phdrCardBridge->PCICB_ucRevisionId);
        API_PciConfigInByte(iBus, iSlot, iFunc, PCI_CLASS_PROG,
                            (UINT8 *)&phdrCardBridge->PCICB_ucProgIf);
        API_PciConfigInByte(iBus, iSlot, iFunc, PCI_CLASS_DEVICE,
                            (UINT8 *)&phdrCardBridge->PCICB_ucSubClass);
        API_PciConfigInByte(iBus, iSlot, iFunc, PCI_CLASS,
                            (UINT8 *)&phdrCardBridge->PCICB_ucClassCode);
        API_PciConfigInByte(iBus, iSlot, iFunc, PCI_CACHE_LINE_SIZE,
                            (UINT8 *)&phdrCardBridge->PCICB_ucCacheLine);
        API_PciConfigInByte(iBus, iSlot, iFunc, PCI_LATENCY_TIMER,
                            (UINT8 *)&phdrCardBridge->PCICB_ucLatency);
        API_PciConfigInByte(iBus, iSlot, iFunc, PCI_HEADER_TYPE,
                            (UINT8 *)&phdrCardBridge->PCICB_ucHeaderType);
        API_PciConfigInByte(iBus, iSlot, iFunc, PCI_BIST,
                            (UINT8 *)&phdrCardBridge->PCICB_ucBist);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_BASE_ADDRESS_0,
                             (UINT32 *)&phdrCardBridge->PCICB_uiBase[PCI_BAR_INDEX_0]);
        API_PciConfigInByte(iBus, iSlot, iFunc, PCI_CB_CAPABILITY_LIST,
                            (UINT8 *)&phdrCardBridge->PCICB_ucCapPtr);
        API_PciConfigInWord(iBus, iSlot, iFunc, PCI_CB_SEC_STATUS,
                            (UINT16 *)&phdrCardBridge->PCICB_usSecStatus);
        API_PciConfigInByte(iBus, iSlot, iFunc, PCI_CB_PRIMARY_BUS,
                            (UINT8 *)&phdrCardBridge->PCICB_ucPriBus);
        API_PciConfigInByte(iBus, iSlot, iFunc, PCI_CB_CARD_BUS,
                            (UINT8 *)&phdrCardBridge->PCICB_ucSecBus);
        API_PciConfigInByte(iBus, iSlot, iFunc, PCI_CB_SUBORDINATE_BUS,
                            (UINT8 *)&phdrCardBridge->PCICB_ucSubBus);
        API_PciConfigInByte(iBus, iSlot, iFunc, PCI_CB_LATENCY_TIMER,
                            (UINT8 *)&phdrCardBridge->PCICB_ucSecLatency);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_CB_MEMORY_BASE_0,
                             (UINT32 *)&phdrCardBridge->PCICB_uiMemBase0);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_CB_MEMORY_LIMIT_0,
                             (UINT32 *)&phdrCardBridge->PCICB_uiMemLimit0);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_CB_MEMORY_BASE_1,
                             (UINT32 *)&phdrCardBridge->PCICB_uiMemBase1);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_CB_MEMORY_LIMIT_1,
                             (UINT32 *)&phdrCardBridge->PCICB_uiMemLimit1);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_CB_IO_BASE_0,
                             (UINT32 *)&phdrCardBridge->PCICB_uiIoBase0);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_CB_IO_LIMIT_0,
                             (UINT32 *)&phdrCardBridge->PCICB_uiIoLimit0);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_CB_IO_BASE_1,
                             (UINT32 *)&phdrCardBridge->PCICB_uiIoBase1);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_CB_IO_LIMIT_1,
                             (UINT32 *)&phdrCardBridge->PCICB_uiIoLimit1);
        API_PciConfigInByte(iBus, iSlot, iFunc, PCI_INTERRUPT_LINE,
                            (UINT8 *)&phdrCardBridge->PCICB_ucIntLine);
        API_PciConfigInByte(iBus, iSlot, iFunc, PCI_INTERRUPT_PIN,
                            (UINT8 *)&phdrCardBridge->PCICB_ucIntPin);
        API_PciConfigInWord(iBus, iSlot, iFunc, PCI_CB_BRIDGE_CONTROL,
                            (UINT16 *)&phdrCardBridge->PCICB_usControl);
        API_PciConfigInWord(iBus, iSlot, iFunc, PCI_CB_SUBSYSTEM_VENDOR_ID,
                            (UINT16 *)&phdrCardBridge->PCICB_usSubVendorId);
        API_PciConfigInWord(iBus, iSlot, iFunc, PCI_CB_SUBSYSTEM_ID,
                            (UINT16 *)&phdrCardBridge->PCICB_usSubSystemId);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_CB_LEGACY_MODE_BASE,
                             (UINT32 *)&phdrCardBridge->PCICB_uiLegacyBase);
        __pciCardBridgeHeaderShow(phdrCardBridge);

        if (phdrCardBridge->PCICB_usStatus & PCI_STATUS_CAP_LIST) {
            API_PciCapShow(iBus, iSlot, iFunc);
        }
    
    } else {
        API_PciConfigInWord(iBus, iSlot, iFunc, PCI_VENDOR_ID,
                            (UINT16 *)&phdrDevice->PCID_usVendorId);
        API_PciConfigInWord(iBus, iSlot, iFunc, PCI_DEVICE_ID,
                            (UINT16 *)&phdrDevice->PCID_usDeviceId);
        API_PciConfigInWord(iBus, iSlot, iFunc, PCI_COMMAND,
                            (UINT16 *)&phdrDevice->PCID_usCommand);
        API_PciConfigInWord(iBus, iSlot, iFunc, PCI_STATUS,
                            (UINT16 *)&phdrDevice->PCID_usStatus);
        API_PciConfigInByte(iBus, iSlot, iFunc, PCI_REVISION_ID,
                            (UINT8 *)&phdrDevice->PCID_ucRevisionId);
        API_PciConfigInByte(iBus, iSlot, iFunc, PCI_CLASS_PROG,
                            (UINT8 *)&phdrDevice->PCID_ucProgIf);
        API_PciConfigInByte(iBus, iSlot, iFunc, PCI_CLASS_DEVICE,
                            (UINT8 *)&phdrDevice->PCID_ucSubClass);
        API_PciConfigInByte(iBus, iSlot, iFunc, PCI_CLASS,
                            (UINT8 *)&phdrDevice->PCID_ucClassCode);
        API_PciConfigInByte(iBus, iSlot, iFunc, PCI_CACHE_LINE_SIZE,
                            (UINT8 *)&phdrDevice->PCID_ucCacheLine);
        API_PciConfigInByte(iBus, iSlot, iFunc, PCI_LATENCY_TIMER,
                            (UINT8 *)&phdrDevice->PCID_ucLatency);
        API_PciConfigInByte(iBus, iSlot, iFunc, PCI_HEADER_TYPE,
                            (UINT8 *)&phdrDevice->PCID_ucHeaderType);
        API_PciConfigInByte(iBus, iSlot, iFunc, PCI_BIST,
                            (UINT8 *)&phdrDevice->PCID_ucBist);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_BASE_ADDRESS_0,
                             (UINT32 *)&phdrDevice->PCID_uiBase[PCI_BAR_INDEX_0]);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_BASE_ADDRESS_1,
                             (UINT32 *)&phdrDevice->PCID_uiBase[PCI_BAR_INDEX_1]);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_BASE_ADDRESS_2,
                             (UINT32 *)&phdrDevice->PCID_uiBase[PCI_BAR_INDEX_2]);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_BASE_ADDRESS_3,
                             (UINT32 *)&phdrDevice->PCID_uiBase[PCI_BAR_INDEX_3]);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_BASE_ADDRESS_4,
                             (UINT32 *)&phdrDevice->PCID_uiBase[PCI_BAR_INDEX_4]);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_BASE_ADDRESS_5,
                             (UINT32 *)&phdrDevice->PCID_uiBase[PCI_BAR_INDEX_5]);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_CARDBUS_CIS,
                             (UINT32 *)&phdrDevice->PCID_uiCis);
        API_PciConfigInWord(iBus, iSlot, iFunc, PCI_SUBSYSTEM_VENDOR_ID,
                            (UINT16 *)&phdrDevice->PCID_usSubVendorId);
        API_PciConfigInWord(iBus, iSlot, iFunc, PCI_SUBSYSTEM_ID,
                            (UINT16 *)&phdrDevice->PCID_usSubSystemId);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_ROM_ADDRESS,
                             (UINT32 *)&phdrDevice->PCID_uiRomBase);
        API_PciConfigInByte(iBus, iSlot, iFunc, PCI_INTERRUPT_LINE,
                            (UINT8 *)&phdrDevice->PCID_ucIntLine);
        API_PciConfigInByte(iBus, iSlot, iFunc, PCI_INTERRUPT_PIN,
                            (UINT8 *)&phdrDevice->PCID_ucIntPin);
        API_PciConfigInByte(iBus, iSlot, iFunc, PCI_MIN_GNT,
                            (UINT8 *)&phdrDevice->PCID_ucMinGrant);
        API_PciConfigInByte(iBus, iSlot, iFunc, PCI_MAX_LAT,
                            (UINT8 *)&phdrDevice->PCID_ucMaxLatency);
        __pciDeviceHeaderShow(phdrDevice);

        if (phdrDevice->PCID_usStatus & PCI_STATUS_CAP_LIST) {
            API_PciCapShow(iBus, iSlot, iFunc);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciInterruptShow
** 功能描述: 打印设备的 Interrupt 信息.
** 输　入  : iBus   总线号
**           iSlot  插槽
**           iFunc  功能
** 输　出  : NONE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
VOID  API_PciInterruptShow (INT iBus, INT iSlot, INT iFunc)
{
    UINT8       ucIrq = 0;
    UINT8       ucPin = 0;

    API_PciConfigInByte(iBus, iSlot, iFunc, PCI_INTERRUPT_LINE, &ucIrq);
    API_PciConfigInByte(iBus, iSlot, iFunc, PCI_INTERRUPT_PIN,  &ucPin);

    printf("\tInterrupt Line =0x%02x, Pin = 0x%02x\n", ucIrq, ucPin);

    if (ucPin || ucIrq) {
        printf("\tInterrupt: pin %c routed to IRQ %d \n", (ucPin ? 'A' + ucPin - 1 : '?'), ucIrq);
    }
}
/*********************************************************************************************************
** 函数名称: API_PciBarShow
** 功能描述: 打印设备的 BAR 信息.
** 输　入  : iBus   总线号
**           iSlot  插槽
**           iFunc  功能
**           iBar   BAR 索引
** 输　出  : NONE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
VOID  API_PciBarShow (INT iBus, INT iSlot, INT iFunc, INT iBar)
{
    UINT32      uiBarVal;
    INT         iSpace;
    INT         iPrefetch;
    INT         iOffset;

    iOffset = PCI_BASE_ADDRESS_0 + (iBar * sizeof(UINT32));
    API_PciConfigInDword(iBus, iSlot, iFunc, iOffset, (UINT32 *)&uiBarVal);
    if ((uiBarVal == 0) ||
        (uiBarVal == 0xffffffff)) {
        return;
    }
    
    if ((uiBarVal & PCI_BASE_ADDRESS_SPACE ) == PCI_BASE_ADDRESS_SPACE_IO) {
        printf("\tbar%d in I/O space at 0x%08x\n", iBar, (uiBarVal & (~0x00000001)));
    
    } else {
        iPrefetch = (uiBarVal & PCI_BASE_ADDRESS_MEM_PREFETCH);
        iSpace    = (uiBarVal & PCI_BASE_ADDRESS_MEM_TYPE_MASK);
        uiBarVal  = (uiBarVal & PCI_BASE_ADDRESS_MEM_MASK);
        printf("\tbar%d in %s%s mem space at 0x%08x\n", iBar,
               iPrefetch ? "prefetchable " : "",
               (iSpace == PCI_BASE_ADDRESS_MEM_TYPE_32 ) ? "32-bit" :
               (iSpace == PCI_BASE_ADDRESS_MEM_TYPE_64) ? "64-bit" :
               "reserved",
               uiBarVal);
    }
}
/*********************************************************************************************************
** 函数名称: API_PciBistShow
** 功能描述: 打印设备的内建测试信息.
** 输　入  : iBus   总线号
**           iSlot  插槽
**           iFunc  功能
** 输　出  : NONE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
VOID  API_PciBistShow (INT iBus, INT iSlot, INT iFunc)
{
    UINT8       ucBist;

    API_PciConfigInByte(iBus, iSlot, iFunc, PCI_BIST, &ucBist);

    printf("\tBist =0x%02x\n", ucBist);

    if (ucBist & PCI_BIST_CAPABLE) {
        if (ucBist & PCI_BIST_START) {
            printf("\tBIST is running\n");
        
        } else {
            printf("\tBIST result: %02x\n", ucBist & PCI_BIST_CODE_MASK);
        }
    }
}
/*********************************************************************************************************
** 函数名称: API_PciStatusShow
** 功能描述: 打印设备的 Status 信息.
** 输　入  : iBus   总线号
**           iSlot  插槽
**           iFunc  功能
** 输　出  : NONE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
VOID  API_PciStatusShow (INT iBus, INT iSlot, INT iFunc)
{
    UINT16      usStatus;

    API_PciConfigInWord(iBus, iSlot, iFunc, PCI_STATUS, &usStatus);

    printf("\tStatus=0x%04x\n", usStatus);

    printf("\tStatus: Cap%c 66MHz%c UDF%c FastB2B%c ParErr%c DEVSEL=%s >TAbort%c <TAbort%c <MAbort%c"
           ">SERR%c <PERR%c INTx%c\n",
           PCI_FLAG(usStatus, PCI_STATUS_CAP_LIST),
           PCI_FLAG(usStatus, PCI_STATUS_66MHZ),
           PCI_FLAG(usStatus, PCI_STATUS_UDF),
           PCI_FLAG(usStatus, PCI_STATUS_FAST_BACK),
           PCI_FLAG(usStatus, PCI_STATUS_PARITY),
           ((usStatus & PCI_STATUS_DEVSEL_MASK) == PCI_STATUS_DEVSEL_SLOW) ? "slow" :
           ((usStatus & PCI_STATUS_DEVSEL_MASK) == PCI_STATUS_DEVSEL_MEDIUM) ? "medium" :
           ((usStatus & PCI_STATUS_DEVSEL_MASK) == PCI_STATUS_DEVSEL_FAST) ? "fast" : "??",
           PCI_FLAG(usStatus, PCI_STATUS_SIG_TARGET_ABORT),
           PCI_FLAG(usStatus, PCI_STATUS_REC_TARGET_ABORT),
           PCI_FLAG(usStatus, PCI_STATUS_REC_MASTER_ABORT),
           PCI_FLAG(usStatus, PCI_STATUS_SIG_SYSTEM_ERROR),
           PCI_FLAG(usStatus, PCI_STATUS_DETECTED_PARITY),
           PCI_FLAG(usStatus, PCI_COMMAND_INTX_DISABLE));
}
/*********************************************************************************************************
** 函数名称: API_PciCommandShow
** 功能描述: 打印设备的 Command 信息.
** 输　入  : iBus   总线号
**           iSlot  插槽
**           iFunc  功能
** 输　出  : NONE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
VOID  API_PciCommandShow (INT iBus, INT iSlot, INT iFunc)
{
    UINT16      usCommand;
    UINT16      usClass;
    UINT8       ucLatency;
    UINT8       ucCacheLine;
    UINT8       ucHeaderType;
    UINT8       ucMaxLat = 0;
    UINT8       ucMinGnt = 0;

    API_PciConfigInWord(iBus, iSlot, iFunc, PCI_COMMAND,         &usCommand);

    printf("\tCommand=0x%04x\n", usCommand);

    API_PciConfigInWord(iBus, iSlot, iFunc, PCI_CLASS_DEVICE,    &usClass);
    API_PciConfigInByte(iBus, iSlot, iFunc, PCI_LATENCY_TIMER,   &ucLatency);
    API_PciConfigInByte(iBus, iSlot, iFunc, PCI_LATENCY_TIMER,   &ucLatency);
    API_PciConfigInByte(iBus, iSlot, iFunc, PCI_CACHE_LINE_SIZE, &ucCacheLine);

    API_PciHeaderTypeGet(iBus, iSlot, iFunc, &ucHeaderType);

    switch (ucHeaderType) {

    case PCI_HEADER_TYPE_NORMAL:
        if (usClass == PCI_CLASS_BRIDGE_PCI) {
            printf("\t!!! Invalid class %04x for header type %02x\n", usClass, ucHeaderType);
        }
        API_PciConfigInByte(iBus, iSlot, iFunc, PCI_MAX_LAT, &ucMaxLat);
        API_PciConfigInByte(iBus, iSlot, iFunc, PCI_MIN_GNT, &ucMinGnt);
        break;

    case PCI_HEADER_TYPE_BRIDGE:
        if ((usClass >> 8) != PCI_BASE_CLASS_BRIDGE) {
            printf("\t!!! Invalid class %04x for header type %02x\n", usClass, ucHeaderType);
        }
        ucMinGnt = 0;
        ucMaxLat = 0;
        break;

    case PCI_HEADER_TYPE_CARDBUS:
        if ((usClass >> 8) != PCI_BASE_CLASS_BRIDGE) {
            printf("\t!!! Invalid class %04x for header type %02x\n", usClass, ucHeaderType);
        }
        ucMinGnt = 0;
        ucMaxLat = 0;
        break;

    default:
        printf("\t!!! Unknown header type %02x\n", ucHeaderType);
        return;
    }

    printf("\tcommand=0x%04x (", usCommand);

    printf("\tControl: I/O%c Mem%c BusMaster%c SpecCycle%c MemWINV%c VGASnoop%c ParErr%c Stepping%c"
           "SERR%c FastB2B%c DisINTx%c\n",
           PCI_FLAG(usCommand, PCI_COMMAND_IO),
           PCI_FLAG(usCommand, PCI_COMMAND_MEMORY),
           PCI_FLAG(usCommand, PCI_COMMAND_MASTER),
           PCI_FLAG(usCommand, PCI_COMMAND_SPECIAL),
           PCI_FLAG(usCommand, PCI_COMMAND_INVALIDATE),
           PCI_FLAG(usCommand, PCI_COMMAND_VGA_PALETTE),
           PCI_FLAG(usCommand, PCI_COMMAND_PARITY),
           PCI_FLAG(usCommand, PCI_COMMAND_WAIT),
           PCI_FLAG(usCommand, PCI_COMMAND_SERR),
           PCI_FLAG(usCommand, PCI_COMMAND_FAST_BACK),
           PCI_FLAG(usCommand, PCI_COMMAND_INTX_DISABLE));

    if (usCommand & PCI_COMMAND_MASTER) {
        printf("\tLatency: %d", ucLatency);
        if (ucMinGnt || ucMaxLat) {
            printf(" (");
            if (ucMinGnt) {
                printf("%dns min", (ucMinGnt * 250));
            }
            if (ucMinGnt && ucMaxLat) {
                printf(", ");
            }
            if (ucMaxLat) {
                printf("%dns max", (ucMaxLat * 250));
            }
            putchar(')');
        }
        if (ucCacheLine) {
          printf(", Cache Line Size: %d bytes", (ucCacheLine * 4));
        }
        putchar('\n');
    }
}
/*********************************************************************************************************
** 函数名称: API_PciReversionShow
** 功能描述: 打印设备版本信息.
** 输　入  : iBus      总线号
**           iSlot     插槽
**           iFunc     功能
** 输　出  : NONE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
VOID  API_PciReversionShow (INT iBus, INT iSlot, INT iFunc)
{
    UINT8       ucRev = 0;

    API_PciConfigInByte(iBus, iSlot, iFunc, PCI_REVISION_ID,   &ucRev);

    printf("\tReversion %02x\n", ucRev);
}
/*********************************************************************************************************
** 函数名称: API_PciIdsShow
** 功能描述: 打印设备ID信息 (Vendor ID, Device ID).
** 输　入  : iBus      总线号
**           iSlot     插槽
**           iFunc     功能
** 输　出  : NONE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
VOID  API_PciIdsShow (INT iBus, INT iSlot, INT iFunc)
{
    UINT16      usVendor = 0;
    UINT16      usDevice = 0;

    API_PciConfigInWord(iBus, iSlot, iFunc, PCI_VENDOR_ID, &usVendor);
    API_PciConfigInWord(iBus, iSlot, iFunc, PCI_DEVICE_ID, &usDevice);

    printf("\tVendor ID 0x%04x, Device ID 0x%04x\n", usVendor, usDevice);
}
/*********************************************************************************************************
** 函数名称: API_PciConfigShow
** 功能描述: 打印设备配置信息.
** 输　入  : iBus      总线号
**           iSlot     插槽
**           iFunc     功能
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciConfigShow (INT iBus, INT iSlot, INT iFunc)
{
    REGISTER INT        i;
    UINT8               ucClassCode;
    UINT16              usSubClass;
    UINT8               ucSecBus = 0;
    INT                 iNumBars = ((PCI_BASE_ADDRESS_5 - PCI_BASE_ADDRESS_0) / sizeof(UINT32)) + 1;
    UINT16              usMemBase;
    UINT16              usMemLimit;
    UINT32              uiMemBaseU;
    UINT32              uiMemLimitU;
    UINT8               ucIoBase;
    UINT8               ucIoLimit;
    UINT16              usIoBaseU;
    UINT16              usIoLimitU;
    UINT8               ucHeaderType;
    UINT16              usCmdReg;
    UINT32              uiCardBusMemBase;
    UINT32              uiCardBusMemLimit;
    UINT32              uiCardBusIoBase;
    UINT32              uiCardBusIoLimit;

    printf("%02x:%02x.%d] type=", iBus, iSlot, iFunc);

    API_PciConfigInByte(iBus, iSlot, iFunc, PCI_CLASS, &ucClassCode);
    API_PciConfigInByte(iBus, iSlot, iFunc, PCI_HEADER_TYPE, &ucHeaderType);

    if ((ucHeaderType & PCI_HEADER_TYPE_MASK ) == PCI_HEADER_TYPE_BRIDGE) {
        iNumBars = 2;
    }

    if ((ucHeaderType & PCI_HEADER_TYPE_MASK) == PCI_HEADER_TYPE_CARDBUS) {
        iNumBars = 1;
    }

    switch (ucClassCode) {

    case PCI_BASE_CLASS_NOT_DEFINED:       printf("NOT_DEFINED\n"); break;
    case PCI_BASE_CLASS_STORAGE:           printf("MASS STORAGE\n"); break;
    case PCI_BASE_CLASS_NETWORK:           printf("NET_CNTLR\n"); break;
    case PCI_BASE_CLASS_DISPLAY:           printf("DISP_CNTLR\n"); break;
    case PCI_BASE_CLASS_MULTIMEDIA:        printf("MULTI_MEDIA\n"); break;
    case PCI_BASE_CLASS_MEMORY:            printf("MEM_CNTLR\n"); break;
    case PCI_BASE_CLASS_COMMUNICATION:     printf("COMMUNICATION\n"); break;
    case PCI_BASE_CLASS_SYSTEM:            printf("SYSTEM\n"); break;
    case PCI_BASE_CLASS_INPUT:             printf("INPUT\n"); break;
    case PCI_BASE_CLASS_DOCKING:           printf("DOCKING STATION\n"); break;
    case PCI_BASE_CLASS_SERIAL:            printf("SERIAL BUS\n"); break;
    case PCI_BASE_CLASS_WIRELESS:          printf("WIRELESS\n"); break;
    case PCI_BASE_CLASS_INTELLIGENT:       printf("INTELLIGENT_IO\n"); break;
    case PCI_BASE_CLASS_SATELLITE:         printf("SATELLITE\n"); break;
    case PCI_BASE_CLASS_CRYPT:             printf("ENCRYPTION DEV\n"); break;
    case PCI_BASE_CLASS_SIGNAL_PROCESSING: printf("DATA ACQUISITION DEV\n"); break;
    case PCI_CLASS_OTHERS:                 printf("OTHER DEVICE\n"); break;

    case PCI_BASE_CLASS_PROCESSOR:
        if ((ucHeaderType & PCI_HEADER_TYPE_MASK) != PCI_HEADER_TYPE_BRIDGE) {
            printf("PROCESSOR\n");
            break;
        
        } else {
            ucSecBus = 0;
            API_PciConfigInWord(iBus, iSlot, iFunc, PCI_CLASS_DEVICE, &usSubClass);

            switch (usSubClass) {

            case PCI_CLASS_PROCESSOR_POWERPC:
                printf("PowerPC");
                API_PciConfigInByte(iBus, iSlot, iFunc, PCI_SECONDARY_BUS, &ucSecBus);
                break;

            default:
                printf("UNKNOWN (0x%04x)", usSubClass);
                break;
            }

            printf(" PROCESSOR");

            if (ucSecBus != 0 ) {
                printf(" to [%d,0,0]", ucSecBus);
                printf("\n");

                API_PciConfigInWord(iBus, iSlot, iFunc, PCI_COMMAND, &usCmdReg);
                if (usCmdReg & PCI_COMMAND_MEMORY) {
                    API_PciConfigInWord(iBus, iSlot, iFunc, PCI_MEMORY_BASE,  &usMemBase );
                    API_PciConfigInWord(iBus, iSlot, iFunc, PCI_MEMORY_LIMIT, &usMemLimit);
                    printf("\tbase/limit:\n");
                    printf("\t  mem=   0x%04x0000/0x%04xffff\n",
                           (usMemBase & 0xFFF0), (usMemLimit | 0x000F));

                    API_PciConfigInWord(iBus, iSlot, iFunc, PCI_PREF_MEMORY_BASE,  &usMemBase );
                    API_PciConfigInWord(iBus, iSlot, iFunc, PCI_PREF_MEMORY_LIMIT, &usMemLimit);
                    if ((usMemBase & PCI_MEMORY_RANGE_TYPE_MASK) == PCI_PREF_RANGE_TYPE_64) {
                        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_PREF_BASE_UPPER32,  &uiMemBaseU );
                        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_PREF_LIMIT_UPPER32, &uiMemLimitU);
                        printf("\t  preMem=0x%08x%04x0000/" "0x%08x%04xffff\n",
                               uiMemBaseU, (usMemBase & 0xFFF0), uiMemLimitU, (usMemLimit | 0x000F));
                    } else {
                        printf("\t  preMem=0x%04x0000/0x%04xffff\n",
                               (usMemBase & 0xFFF0), (usMemLimit | 0x000F));
                    }
                }

                if (usCmdReg & PCI_COMMAND_IO) {
                    API_PciConfigInByte(iBus, iSlot, iFunc, PCI_IO_BASE,  &ucIoBase );
                    API_PciConfigInByte(iBus, iSlot, iFunc, PCI_IO_LIMIT, &ucIoLimit);
                    if ((ucIoBase & PCI_IO_RANGE_TYPE_MASK) == PCI_IO_RANGE_TYPE_32) {
                        API_PciConfigInWord(iBus, iSlot, iFunc, PCI_IO_BASE_UPPER16,  &usIoBaseU );
                        API_PciConfigInWord(iBus, iSlot, iFunc, PCI_IO_LIMIT_UPPER16, &usIoLimitU);
                        printf("\t  I/O=   0x%04x%02x00/0x%04x%02xff\n",
                               usIoBaseU, (ucIoBase & 0xF0), usIoLimitU, (ucIoLimit | 0x0F));
                    } else {
                        printf("\t  I/O=   0x%02x00/0x%02xff\n", (ucIoBase & 0xF0), (ucIoLimit | 0x0F));
                    }
                }
            } else {
                printf("\n");
            }
        }
        break;

    case PCI_BASE_CLASS_BRIDGE:
        ucSecBus = 0;
        API_PciConfigInWord(iBus, iSlot, iFunc, PCI_CLASS_DEVICE, &usSubClass);

        switch (usSubClass) {

        case PCI_CLASS_BRIDGE_HOST:
            printf("HOST");
            break;

        case PCI_CLASS_BRIDGE_ISA:
            printf("ISA");
            break;

        case PCI_CLASS_BRIDGE_EISA:
            printf("EISA");
            break;

        case PCI_CLASS_BRIDGE_MC:
            printf("MC");
            break;

        case PCI_CLASS_BRIDGE_PCI:
            printf("P2P");
            API_PciConfigInByte(iBus, iSlot, iFunc, PCI_SECONDARY_BUS, &ucSecBus);
            break;

        case PCI_CLASS_BRIDGE_PCMCIA:
            printf("PCMCIA");
            break;

        case PCI_CLASS_BRIDGE_CARDBUS:
            printf("CARDBUS");
            API_PciConfigInByte(iBus, iSlot, iFunc, PCI_SECONDARY_BUS, &ucSecBus);
            break;

        case PCI_CLASS_BRIDGE_RACEWAY:
            printf("RACEWAY");
            break;

        default:
            printf("UNKNOWN (0x%04x)", usSubClass);
            break;
        }

        printf(" BRIDGE");
        if (ucSecBus != 0 ) {
            printf(" to [%d,0,0]", ucSecBus);
            printf("\n");

            API_PciConfigInWord(iBus, iSlot, iFunc, PCI_COMMAND, &usCmdReg);
            if (usSubClass == PCI_CLASS_BRIDGE_CARDBUS) {
                printf ("\tbase/limit:\n");

                if (usCmdReg & PCI_COMMAND_MEMORY) {
                    for (i = 0; i < 2; i++) {
                        API_PciConfigInDword(iBus, iSlot, iFunc,
                                             (PCI_CB_MEMORY_BASE_0 + (i * 8)), &uiCardBusMemBase);
                        API_PciConfigInDword(iBus, iSlot, iFunc,
                                             (PCI_CB_MEMORY_LIMIT_0 + (i * 8)), &uiCardBusMemLimit);
                        printf ("\t  mem%d=0x%08x/0x%08x\n", i,
                                uiCardBusMemBase, (uiCardBusMemLimit | 0x0FFF));
                    }
                }

                if (usCmdReg & PCI_COMMAND_IO) {
                    for (i = 0; i < 2; i++) {
                        API_PciConfigInDword(iBus, iSlot, iFunc,
                                             (PCI_CB_IO_BASE_0 + (i * 8)), &uiCardBusIoBase);
                        API_PciConfigInDword(iBus, iSlot, iFunc,
                                             (PCI_CB_IO_LIMIT_0 + (i * 8)), &uiCardBusIoLimit);
                        printf ("\t  I/O%d=0x%08x/0x%08x\n", i, uiCardBusIoBase, uiCardBusIoLimit);
                    }
                }

                break;
            }

            if (usCmdReg & PCI_COMMAND_MEMORY) {
                API_PciConfigInWord(iBus, iSlot, iFunc, PCI_MEMORY_BASE,  &usMemBase );
                API_PciConfigInWord(iBus, iSlot, iFunc, PCI_MEMORY_LIMIT, &usMemLimit);
                printf("\tbase/limit:\n");
                printf("\t  mem=   0x%04x0000/0x%04xffff\n",
                       (usMemBase & 0xFFF0), (usMemLimit | 0x000F));

                API_PciConfigInWord(iBus, iSlot, iFunc, PCI_PREF_MEMORY_BASE,  &usMemBase );
                API_PciConfigInWord(iBus, iSlot, iFunc, PCI_PREF_MEMORY_LIMIT, &usMemLimit);
                if ((usMemBase & PCI_MEMORY_RANGE_TYPE_MASK) == PCI_PREF_RANGE_TYPE_64) {
                    API_PciConfigInDword(iBus, iSlot, iFunc, PCI_PREF_BASE_UPPER32,  &uiMemBaseU );
                    API_PciConfigInDword(iBus, iSlot, iFunc, PCI_PREF_LIMIT_UPPER32, &uiMemLimitU);
                    printf("\t  preMem=0x%08x%04x0000/"
                           "0x%08x%04xffff\n",
                           uiMemBaseU, (usMemBase & 0xFFF0), uiMemLimitU, (usMemLimit | 0x000F));
                    }
                else
                    printf("\t  preMem=0x%04x0000/0x%04xffff\n",
                           (usMemBase & 0xFFF0), (usMemLimit | 0x000F));
                }

            if (usCmdReg & PCI_COMMAND_IO) {
                API_PciConfigInByte(iBus, iSlot, iFunc, PCI_IO_BASE,  &ucIoBase );
                API_PciConfigInByte(iBus, iSlot, iFunc, PCI_IO_LIMIT, &ucIoLimit);
                if ((ucIoBase & PCI_IO_RANGE_TYPE_MASK) == PCI_IO_RANGE_TYPE_32) {
                    API_PciConfigInWord(iBus, iSlot, iFunc, PCI_IO_BASE_UPPER16,  &usIoBaseU );
                    API_PciConfigInWord(iBus, iSlot, iFunc, PCI_IO_LIMIT_UPPER16, &usIoLimitU);
                    printf("\t  I/O=   0x%04x%02x00/0x%04x%02xff\n",
                           usIoBaseU, (ucIoBase & 0xF0), usIoLimitU, (ucIoLimit | 0x0F));
                } else {
                    printf("\t  I/O=   0x%02x00/0x%02xff\n", (ucIoBase & 0xF0), (ucIoLimit | 0x0F));
                }
            }
        } else {
            printf("\n");
        }

        break;

    default:
        printf("Unknown!\n");
        break;
    }

    API_PciIdsShow(iBus, iSlot, iFunc);
    API_PciReversionShow(iBus, iSlot, iFunc);
    API_PciStatusShow(iBus, iSlot, iFunc);
    API_PciCommandShow(iBus, iSlot, iFunc);
    API_PciBistShow(iBus, iSlot, iFunc);

    for (i = 0; i < iNumBars; i++) {
        API_PciBarShow(iBus, iSlot, iFunc, i);
    }

    API_PciInterruptShow(iBus, iSlot, iFunc);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciCapPcixShow
** 功能描述: 打印设备 PCI-X 信息.
** 输　入  : iBus      总线号
**           iSlot     插槽
**           iFunc     功能
**           uiOffset  扩展功能保存的地址偏移
** 输　出  : NONE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
VOID  API_PciCapPcixShow (INT iBus, INT iSlot, INT iFunc, UINT uiOffset)
{
    UINT8       ucHeaderType;
    UINT16      usCmd;
    UINT16      usStatus;
    UINT16      usSecStat;
    UINT32      uiStat;
    UINT32      uiUpstr;
    UINT32      uiDwnstr;

    API_PciConfigInByte(iBus, iSlot, iFunc, PCI_HEADER_TYPE, &ucHeaderType);
    switch (ucHeaderType) {

    case PCI_HEADER_TYPE_NORMAL:
        printf("PCI-X \n");
        API_PciConfigInWord (iBus, iSlot, iFunc, (uiOffset + PCI_PCIX_COMMAND   ), &usCmd );
        API_PciConfigInDword(iBus, iSlot, iFunc, (uiOffset + PCI_PCIX_STATUS), &uiStat);
        printf("\tCommand: 0x%04x\n", usCmd);
        printf("\tStatus:  0x%08x\n", uiStat);
        break;

    case PCI_HEADER_TYPE_BRIDGE:
        printf("PCI-X bridge \n");
        API_PciConfigInWord (iBus, iSlot, iFunc, (uiOffset + PCI_PCIX_BRIDGE_SEC_STATUS), &usSecStat);
        API_PciConfigInWord (iBus, iSlot, iFunc, (uiOffset + PCI_PCIX_BRIDGE_STATUS    ), &usStatus );
        API_PciConfigInDword(iBus, iSlot, iFunc,
                             (uiOffset + PCI_PCIX_BRIDGE_UPSTREAM_SPLIT_TRANS_CTRL), &uiUpstr);
        API_PciConfigInDword(iBus, iSlot, iFunc,
                             (uiOffset + PCI_PCIX_BRIDGE_DOWNSTREAM_SPLIT_TRANS_CTRL), &uiDwnstr);

        printf("\tSecondary Status:   0x%04x\n", usSecStat);
        printf("\tPrimary Status:     0x%08x\n", usStatus);
        printf("\tUpstream Control:   0x%08x\n", uiUpstr);
        printf("\tDownstream Control: 0x%08x\n", uiDwnstr);
        break;
    }
}

/*********************************************************************************************************
** 函数名称: API_PciCapMsiShow
** 功能描述: 打印设备 MSI 信息.
** 输　入  : iBus      总线号
**           iSlot     插槽
**           iFunc     功能
**           uiOffset  扩展功能保存的地址偏移
** 输　出  : NONE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
VOID  API_PciCapMsiShow (INT iBus, INT iSlot, INT iFunc, UINT uiOffset)
{
    UINT8       ucCtlReg = 0;
    UINT16      usData   = 0;
    UINT32      uiAddr   = 0;

    API_PciConfigInByte(iBus, iSlot, iFunc, (uiOffset + PCI_MSI_FLAGS), &ucCtlReg);
    printf("Message Signaled Interrupts: %s, %s, MME: %d MMC: %d\n",
           (ucCtlReg & PCI_MSI_FLAGS_ENABLE) == 0x00  ? "Disabled" : "Enabled",
           (ucCtlReg & PCI_MSI_FLAGS_64BIT) == 0x00  ? "32-bit" : "64-bit",
           (ucCtlReg & PCI_MSI_FLAGS_QSIZE) >> 4,
           (ucCtlReg & PCI_MSI_FLAGS_QMASK) >> 1);

    printf("\tAddress: ");
    if (ucCtlReg & PCI_MSI_FLAGS_64BIT) {
        API_PciConfigInDword(iBus, iSlot, iFunc, (uiOffset + PCI_MSI_ADDRESS_HI), &uiAddr);
        API_PciConfigInWord (iBus, iSlot, iFunc, (uiOffset + PCI_MSI_DATA_64), &usData);
        printf("%08x", uiAddr);
    
    } else {
        API_PciConfigInWord(iBus, iSlot, iFunc, (uiOffset + PCI_MSI_DATA_32), &usData);
    }

    API_PciConfigInDword(iBus, iSlot, iFunc, (uiOffset + PCI_MSI_ADDRESS_LO), &uiAddr);
    printf("%08x  Data: 0x%04x\n", uiAddr, usData);
}
/*********************************************************************************************************
** 函数名称: __pciCapPcieLatencyL0s
** 功能描述: 打印 PCIe L0s 等待时间.
** 输　入  : iValue  参数值
** 输　出  : 等待时间信息
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static const PCHAR __pciCapPcieLatencyL0s (INT iValue)
{
    static const PCHAR cpcPcieLatencyL0s[] = {"< 64ns", "< 128ns", "< 256ns", "< 512ns", "< 1us", "< 2us",
                                              "< 4us", "> 4us"};

    return  (cpcPcieLatencyL0s[iValue]);
}
/*********************************************************************************************************
** 函数名称: __pciCapPcieLatencyL1
** 功能描述: 打印 PCIe L1 等待时间.
** 输　入  : iValue  参数值
** 输　出  : 等待时间信息
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static const PCHAR __pciCapPcieLatencyL1 (INT iValue)
{
    static const PCHAR cpcPcieLatencyL1[] = {"< 1us", "< 2us", "< 4us","< 8us", "< 16us", "< 32us",
                                             "< 64us", "> 64us"};

    return  (cpcPcieLatencyL1[iValue]);
}
/*********************************************************************************************************
** 函数名称: __pciCapPcieLinkSpeed
** 功能描述: 打印 PCIe 槽电源限制信息.
** 输　入  : iValue  限定值
** 输　出  : 限定信息
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT __pciCapPcieSlotPowerLimit (INT iValue)
{
    static const UINT cfPcieLimit[] = {1000, 100, 10, 1};

    return  (cfPcieLimit[iValue]);
}
/*********************************************************************************************************
** 函数名称: __pciCapPcieLinkSpeed
** 功能描述: 打印 PCIe 链接速度信息.
** 输　入  : iSpeed  链接速度值
** 输　出  : 速度信息
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static const PCHAR __pciCapPcieLinkSpeed (INT iSpeed)
{
    switch (iSpeed) {

    case 1:
        return  ("2.5Gb/s");

    default:
        break;
    }

    return  ("unknown");
}
/*********************************************************************************************************
** 函数名称: __pciCapPcieASPM
** 功能描述: 打印 PCIe 激活状态管理信息.
** 输　入  : iValue  状态值
** 输　出  : 状态信息
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static const PCHAR __pciCapPcieASPM (INT iValue)
{
    switch (iValue) {

    case 1:
        return  ("L0s");

    case 3:
        return  ("L0s & L1");

    default:
        break;
    }

    return  ("unknown");
}
/*********************************************************************************************************
** 函数名称: __pciCapPcieASPMEnable
** 功能描述: 打印 PCIe 激活状态管理使能信息.
** 输　入  : iValue  状态值
** 输　出  : 状态信息
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static const PCHAR __pciCapPcieASPMEnable (INT iValue)
{
    static const PCHAR cpcPcieASPM[] = {"Disabled", "L0s", "L1","L0s & L1"};

    return  (cpcPcieASPM[iValue]);
}
/*********************************************************************************************************
** 函数名称: __pciCapPcieIndicator
** 功能描述: 打印 PCIe 指示灯信息.
** 输　入  : iValue  状态值
** 输　出  : 状态信息
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static const PCHAR __pciCapPcieIndicator (INT iValue)
{
    static const PCHAR cpcPcieIndicatorState[] = {"Unknown", "On", "Blink", "Off" };

    return  (cpcPcieIndicatorState[iValue]);
}
/*********************************************************************************************************
** 函数名称: __pciCapPcieSlot
** 功能描述: 打印 PCIe 槽信息.
** 输　入  : iBus      总线号
**           iSlot     插槽
**           iFunc     功能
**           uiOffset  扩展功能保存的地址偏移
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciCapPcieSlot (INT iBus, INT iSlot, INT iFunc, UINT  uiOffset)
{
    UINT32      uiSlotCap;
    UINT16      usSlotCtl;

    API_PciConfigInDword(iBus, iSlot, iFunc, (uiOffset + PCI_EXP_SLTCAP), &uiSlotCap);
    printf("\tSlot:");
    if (uiSlotCap & PCI_EXP_SLTCAP_ATNB) {
        printf(" Attn Button");
    }
    if (uiSlotCap & PCI_EXP_SLTCAP_PWRC) {
        printf(" Power Controller");
    }
    if (uiSlotCap & PCI_EXP_SLTCAP_MRL) {
        printf(" MRL Sensor");
    }
    if (uiSlotCap & PCI_EXP_SLTCAP_PWRI) {
        printf(" Pwr Indicator");
    }
    if (uiSlotCap & PCI_EXP_SLTCAP_HPC) {
        printf(" Hot-Plug");
    }
    if (uiSlotCap & PCI_EXP_SLTCAP_HPS) {
        printf(" Hot-Plug Surprise");
    }
    printf("\t\tSlot #%d, MAX Slot Power Limit (Milliwatts) %u\n",
           uiSlotCap >> 19,
           ((uiSlotCap & PCI_EXP_SLTCAP_PWR_VAL) >> 7) *
           __pciCapPcieSlotPowerLimit((uiSlotCap & PCI_EXP_SLTCAP_PWR_SCL) >> 15));

    API_PciConfigInWord(iBus, iSlot, iFunc, (uiOffset + PCI_EXP_SLTCTL), &usSlotCtl);
    printf("\tEnabled:");
    if (usSlotCtl & PCI_EXP_SLTCTL_ATNB) {
        printf(" Attn Button Pressed");
    }
    if (usSlotCtl & PCI_EXP_SLTCTL_PWRF) {
        printf(" Power Fault Detected");
    }
    if (usSlotCtl & PCI_EXP_SLTCTL_MRLS) {
        printf(" MRL Sensor Changed");
    }
    if (usSlotCtl & PCI_EXP_SLTCTL_PRSD) {
        printf(" Presence Detect");
    }
    if (usSlotCtl & PCI_EXP_SLTCTL_CMDC) {
        printf(" Cmd Complete Int");
    }
    if (usSlotCtl & PCI_EXP_SLTCTL_HPIE) {
        printf(" Hot-Plug Int");
    }
     printf("\n");

    if (usSlotCtl & PCI_EXP_SLTCTL_ATNI) {
        printf("\t\tAttn Indicator %s",
               __pciCapPcieIndicator((usSlotCtl & PCI_EXP_SLTCTL_ATNI) >> 6));
    }
    if (usSlotCtl & PCI_EXP_SLTCTL_PWRI) {
        printf(" Power Indicator %s",
                __pciCapPcieIndicator((usSlotCtl & PCI_EXP_SLTCTL_PWRI) >> 8));
    }
    if (usSlotCtl & PCI_EXP_SLTCTL_PWRC) {
        printf(" Power Controller %s",
               (usSlotCtl & PCI_EXP_SLTCTL_PWRI) >> 10 == 0 ? "on" : "off");
    }
}
/*********************************************************************************************************
** 函数名称: __pciCapPcieLink
** 功能描述: 打印 PCIe 链接信息.
** 输　入  : iBus      总线号
**           iSlot     插槽
**           iFunc     功能
**           uiOffset  扩展功能保存的地址偏移
**           uiType    类型信息
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciCapPcieLink (INT iBus, INT iSlot, INT iFunc, UINT  uiOffset, UINT  uiType)
{
    UINT32      uiLinkCap;
    UINT16      usLinkVal;

    API_PciConfigInDword(iBus, iSlot, iFunc, (uiOffset + PCI_EXP_LNKCAP), &uiLinkCap);

    printf("\tLink: MAX Speed - %s, MAX Width - by %d Port - %d ASPM - %s\n",
           __pciCapPcieLinkSpeed(uiLinkCap & PCI_EXP_LNKCAP_SPEED),
           (uiLinkCap & PCI_EXP_LNKCAP_WIDTH) >> 4,
           uiLinkCap >> 24,
           __pciCapPcieASPM((uiLinkCap & PCI_EXP_LNKCAP_ASPM)>>10) );

    printf("\t\tLatency: L0s - %s, L1 - %s\n",
           __pciCapPcieLatencyL0s((uiLinkCap & PCI_EXP_LNKCAP_L0S) >> 12),
           __pciCapPcieLatencyL1((uiLinkCap & PCI_EXP_LNKCAP_L1) >> 15));

    API_PciConfigInWord(iBus, iSlot, iFunc, (uiOffset + PCI_EXP_LNKCTL), &usLinkVal);

    printf("\t\tASPM - %s,", __pciCapPcieASPMEnable(usLinkVal & PCI_EXP_LNKCTL_ASPM));

    if ((uiType == PCI_EXP_TYPE_ROOT_PORT) ||
        (uiType == PCI_EXP_TYPE_ENDPOINT )  ||
        (uiType == PCI_EXP_TYPE_LEG_END  )) {
        printf(" RCB - %dbytes", usLinkVal & PCI_EXP_LNKCTL_RCB ? 128 : 64);
    }
    if (usLinkVal & PCI_EXP_LNKCTL_DISABLE) {
        printf(" Link Disabled");
    }
    if (usLinkVal & PCI_EXP_LNKCTL_CLOCK) {
        printf(" Common Clock");
    }
    if (usLinkVal & PCI_EXP_LNKCTL_XSYNCH) {
        printf(" Extended Sync");
    }
    printf("\n");

    API_PciConfigInWord(iBus, iSlot, iFunc, (uiOffset + PCI_EXP_LNKSTA), &usLinkVal);
    printf("\t\tSpeed - %s, Width - by %d\n",
           __pciCapPcieLinkSpeed(usLinkVal & PCI_EXP_LNKSTA_SPEED),
           (usLinkVal & PCI_EXP_LNKSTA_WIDTH) >> 4);
}
/*********************************************************************************************************
** 函数名称: __pciCapPcieDev
** 功能描述: 打印 PCIe 设备信息.
** 输　入  : iBus      总线号
**           iSlot     插槽
**           iFunc     功能
**           uiOffset  扩展功能保存的地址偏移
**           uiType    类型信息
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciCapPcieDev (INT iBus, INT iSlot, INT iFunc, UINT  uiOffset, UINT  uiType)
{
    UINT32      uiDevCap;
    UINT16      usDevCtl;

    API_PciConfigInDword(iBus, iSlot, iFunc, (uiOffset + PCI_EXP_DEVCAP), &uiDevCap);
    printf("\tDevice: Max Payload: %d bytes", 128 << (uiDevCap & PCI_EXP_DEVCAP_PAYLOAD));
    if (uiDevCap & PCI_EXP_DEVCAP_PHANTOM) {
        printf(", Phantom Funcs %d msb", (uiDevCap & PCI_EXP_DEVCAP_PHANTOM) >> 3);
    }
    printf(", Extended Tag: %d-bit\n", ((uiDevCap & PCI_EXP_DEVCAP_EXT_TAG) >> 5) == 0 ? 5 : 8);

    printf("\t\tAcceptable Latency: L0 - %s, L1 - %s\n",
           __pciCapPcieLatencyL0s((uiDevCap & PCI_EXP_DEVCAP_L0S) >> 6),
           __pciCapPcieLatencyL1((uiDevCap & PCI_EXP_DEVCAP_L1) >> 9));

    if ((uiType == PCI_EXP_TYPE_ENDPOINT) ||
        (uiType == PCI_EXP_TYPE_LEG_END)  ||
        (uiType == PCI_EXP_TYPE_UPSTREAM) ||
        (uiType == PCI_EXP_TYPE_PCIE_BRIDGE)) {
        if (uiDevCap & PCI_EXP_DEVCAP_ATN_BUT) {
            printf("\tAttn Button");
        }
        if (uiDevCap & PCI_EXP_DEVCAP_ATN_IND) {
            printf(", Attn Indicator");
        }
        if (uiDevCap & PCI_EXP_DEVCAP_PWR_IND) {
            printf(", Pwr Indicator\n");
        }
    }

    if (uiType == PCI_EXP_TYPE_UPSTREAM) {
        printf("\tSlot Power Limit (Milliwatts) %u\n",
               ((uiDevCap & PCI_EXP_DEVCAP_PWR_VAL) >> 18) *
                __pciCapPcieSlotPowerLimit((uiDevCap & PCI_EXP_DEVCAP_PWR_SCL) >> 26));
    }

    API_PciConfigInWord(iBus, iSlot, iFunc, (uiOffset + PCI_EXP_DEVCTL), &usDevCtl);
    printf("\t\tErrors Enabled: ");
    if (usDevCtl & PCI_EXP_DEVCTL_CERE) {
        printf(" Correctable");
    }
    if (usDevCtl & PCI_EXP_DEVCTL_NFERE) {
        printf(" Non-Fatal");
    }
    if (usDevCtl & PCI_EXP_DEVCTL_FERE) {
        printf(" Fatal");
    }
    if (usDevCtl & PCI_EXP_DEVCTL_URRE) {
        printf(" Unsupported Request");
    }
    if (usDevCtl & PCI_EXP_DEVCTL_RELAXED) {
        printf("Relaxed Ordering");
    }
    if (usDevCtl & PCI_EXP_DEVCTL_EXT_TAG) {
        printf(" Extended Tag");
    }
    if (usDevCtl & PCI_EXP_DEVCTL_PHANTOM) {
        printf(" Phantom Funcs");
    }
    if (usDevCtl & PCI_EXP_DEVCTL_AUX_PME) {
        printf(" AUX Pwr PM");
    }
    if (usDevCtl & PCI_EXP_DEVCTL_NOSNOOP) {
        printf(" No Snoop");
    }
    printf("\n\t\t");

    if (usDevCtl & PCI_EXP_DEVCTL_PAYLOAD) {
        printf("Max Payload %d bytes ", 128 << ((usDevCtl & PCI_EXP_DEVCTL_PAYLOAD) >> 5));
    }
    if (usDevCtl & PCI_EXP_DEVCTL_READRQ) {
        printf("Max Read Request %d bytes\n", 128 << ((usDevCtl & PCI_EXP_DEVCTL_READRQ) >> 12));
    }
}
/*********************************************************************************************************
** 函数名称: API_PciCapPcieShow
** 功能描述: 打印设备 PCIe 信息.
** 输　入  : iBus      总线号
**           iSlot     插槽
**           iFunc     功能
**           uiOffset  扩展功能保存的地址偏移
** 输　出  : NONE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
VOID  API_PciCapPcieShow (INT iBus, INT iSlot, INT iFunc, UINT uiOffset)
{
    UINT16      usCapReg = 0;
    UINT16      usType   = 0;
    UINT16      usSlot   = 0;

    printf("PCIe: ");
    API_PciConfigInWord(iBus, iSlot, iFunc, (uiOffset + PCI_EXP_FLAGS), &usCapReg);
    usType = (usCapReg & PCI_EXP_FLAGS_TYPE) >> 4;

    switch (usType) {
    
    case PCI_EXP_TYPE_ENDPOINT:
        printf("Endpoint");
        break;

    case PCI_EXP_TYPE_LEG_END:
        printf("Legacy Endpoint");
        break;

    case PCI_EXP_TYPE_ROOT_PORT:
        usSlot = usCapReg & PCI_EXP_FLAGS_SLOT;
        printf("Root Port");
        if (usSlot) {
           printf("- Slot");
        }
        break;

    case PCI_EXP_TYPE_UPSTREAM:
        printf("Upstream Port");
        break;

    case PCI_EXP_TYPE_DOWNSTREAM:
        usSlot = usCapReg & PCI_EXP_FLAGS_SLOT;
        printf("Downstream Port");
        if (usSlot) {
           printf("- Slot");
        }
        break;

    case PCI_EXP_TYPE_PCIE_BRIDGE:
        printf("PCI/PCI-X to Express Bridge");
        break;

    default:
        printf("Unknown");
        break;
    }

    printf(", IRQ %d\n", (usCapReg & PCI_EXP_FLAGS_IRQ) >> 9);

    __pciCapPcieDev(iBus, iSlot, iFunc, uiOffset, usType);

    __pciCapPcieLink(iBus, iSlot, iFunc, uiOffset, usType);

    if (usSlot) {
        __pciCapPcieSlot(iBus, iSlot, iFunc, uiOffset);
    }

    if (usType == PCI_EXP_TYPE_ROOT_PORT) {
        API_PciConfigInWord(iBus, iSlot, iFunc, (uiOffset + PCI_EXP_RTCTL), &usCapReg);
        printf("\tRoot Control Enabled: ");
        if (usCapReg & PCI_EXP_RTCTL_SECEE) {
            printf("Correctable ");
        }
        if (usCapReg & PCI_EXP_RTCTL_SENFEE) {
            printf("System Error (NF) ");
        }
        if (usCapReg & PCI_EXP_RTCTL_SEFEE) {
            printf("System Error (F) ");
        }
        if (usCapReg & PCI_EXP_RTCTL_PMEIE) {
            printf("PME Interrupt ");
        }
        printf("\n");
    }
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PCI_EN > 0)         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
