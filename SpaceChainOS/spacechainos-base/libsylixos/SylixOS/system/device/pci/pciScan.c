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
** 文   件   名: pciScan.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 10 月 01 日
**
** 描        述: PCI 总线自动扫描匹配的设备驱动程序.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PCI_EN > 0)
#include "pciDev.h"
#include "pciLib.h"
#include "pciScan.h"
/*********************************************************************************************************
  PCI 主控器
*********************************************************************************************************/
extern  PCI_CTRL_HANDLE      _G_hPciCtrlHandle;
#define PCI_CTRL             _G_hPciCtrlHandle
/*********************************************************************************************************
  内部回调参数
*********************************************************************************************************/
typedef struct {
    PCI_DEV_DRV_DESC  *PSA_p_pdddTable;
    UINT               PSA_uiNum;
} PCI_SCAN_ARG;
/*********************************************************************************************************
** 函数名称: __pciScanCb
** 功能描述: PCI 扫描回调函数.
** 输　入  : iBus      总线号
**           iSlot     插槽
**           iFunc     功能
**           p_psa     扫描参数
** 输　出  : ERROR_NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __pciScanCb (INT iBus, INT iSlot, INT iFunc, PCI_SCAN_ARG *p_psa)
{
    INT     i;
    UINT32  usVenDevId;
    UINT16  usVendorId;
    UINT16  usDeviceId;
    
    API_PciConfigInDword(iBus, iSlot, iFunc, PCI_VENDOR_ID, &usVenDevId);
    
    usVendorId = (UINT16)(usVenDevId & 0xffff);
    usDeviceId = (UINT16)(usVenDevId >> 16);
    
    for (i = 0; i < p_psa->PSA_uiNum; i++) {
        if ((p_psa->PSA_p_pdddTable[i].PDDD_usVendorId == usVendorId) ||
            (p_psa->PSA_p_pdddTable[i].PDDD_usVendorId == 0xffff)) {
            if ((p_psa->PSA_p_pdddTable[i].PDDD_usDeviceId == usDeviceId) ||
                (p_psa->PSA_p_pdddTable[i].PDDD_usDeviceId == 0xffff)) {
                if (p_psa->PSA_p_pdddTable[i].PDDD_pfuncDrv) {
                    p_psa->PSA_p_pdddTable[i].PDDD_pfuncDrv(iBus, iSlot, iFunc,
                                                            p_psa->PSA_p_pdddTable[i].PDDD_pvArg);
                }
            }
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciScan
** 功能描述: 扫描所有 PCI 设备并且与给定的驱动程序表进行匹配, 如果允许, 则自动安装驱动程序.
** 输　入  : p_pdddTable   驱动程序表
**           uiNum         驱动程序表大小
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API 
INT  API_PciScan (PCI_DEV_DRV_DESC  *p_pdddTable, UINT  uiNum)
{
    PCI_SCAN_ARG    psa;
    
    if (PCI_CTRL == LW_NULL) {
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
    
    if (!p_pdddTable || !uiNum) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    psa.PSA_p_pdddTable = p_pdddTable;
    psa.PSA_uiNum       = uiNum;
    
    return  (API_PciTraversalDev(0, LW_TRUE, __pciScanCb, &psa));
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PCI_EN > 0)         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
