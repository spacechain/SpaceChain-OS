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
** 文   件   名: ahciPm.c
**
** 创   建   人: Gong.YuJian (弓羽箭)
**
** 文件创建日期: 2016 年 03 月 31 日
**
** 描        述: AHCI 设备电源管理.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_AHCI_EN > 0)
#include "ahci.h"
#include "ahciLib.h"
#include "ahciDrv.h"
#include "ahciDev.h"
#include "ahciPm.h"
/*********************************************************************************************************
** 函数名称: API_AhciApmDisable
** 功能描述: 禁能设备高级电源管理
** 输　入  : hCtrl      控制器句柄
**           uiDrive    驱动器号
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_AhciApmDisable (AHCI_CTRL_HANDLE  hCtrl, UINT  uiDrive)
{
    INT                 iRet;
    AHCI_DRIVE_HANDLE   hDrive;
    AHCI_PARAM_HANDLE   hParam;

    hDrive = &hCtrl->AHCICTRL_hDrive[uiDrive];
    hParam = &hDrive->AHCIDRIVE_tParam;
    if ((uiDrive >= hCtrl->AHCICTRL_uiImpPortNum     ) ||
        (hCtrl->AHCICTRL_bDrvInstalled != LW_TRUE    ) ||
        (hCtrl->AHCICTRL_bInstalled    != LW_TRUE    ) ||
        (hDrive->AHCIDRIVE_ucState     != AHCI_DEV_OK)) {
        return  (PX_ERROR);
    }

    if (hParam->AHCIPARAM_usFeaturesSupported1 & AHCI_APM_SUPPORT_APM) {
        iRet = API_AhciNoDataCommandSend(hCtrl, uiDrive,
                                         AHCI_CMD_SET_FEATURE, AHCI_SUB_DISABLE_APM, 0, 0, 0, 0, 0);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
    } else {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_AhciApmEnable
** 功能描述: 使能设备高级电源管理
** 输　入  : hCtrl      控制器句柄
**           uiDrive    驱动器号
**           iApm       电源级别
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_AhciApmEnable (AHCI_CTRL_HANDLE  hCtrl, UINT  uiDrive, INT  iApm)
{
    INT                 iRet;
    AHCI_DRIVE_HANDLE   hDrive;
    AHCI_PARAM_HANDLE   hParam;

    hDrive = &hCtrl->AHCICTRL_hDrive[uiDrive];
    hParam = &hDrive->AHCIDRIVE_tParam;
    if ((uiDrive >= hCtrl->AHCICTRL_uiImpPortNum     ) ||
        (hCtrl->AHCICTRL_bDrvInstalled != LW_TRUE    ) ||
        (hCtrl->AHCICTRL_bInstalled    != LW_TRUE    ) ||
        (hDrive->AHCIDRIVE_ucState     != AHCI_DEV_OK)) {
        return  (PX_ERROR);
    }

    if (hParam->AHCIPARAM_usFeaturesSupported1 & AHCI_APM_SUPPORT_APM) {
        iRet = API_AhciNoDataCommandSend(hCtrl, uiDrive,
                                         AHCI_CMD_SET_FEATURE, AHCI_SUB_ENABLE_APM, iApm, 0, 0, 0, 0);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
    } else {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_AhciPmActive
** 功能描述: 设备电源是否使能
** 输　入  : hCtrl      控制器句柄
**           uiDrive    驱动器号
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_AhciPmActive (AHCI_CTRL_HANDLE  hCtrl, UINT  uiDrive)
{
    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_AHCI_EN > 0)        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
