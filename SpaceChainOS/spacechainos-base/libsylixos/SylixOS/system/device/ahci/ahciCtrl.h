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
** 文   件   名: ahciDev.h
**
** 创   建   人: Gong.YuJian (弓羽箭)
**
** 文件创建日期: 2016 年 03 月 29 日
**
** 描        述: AHCI 设备管理.
*********************************************************************************************************/

#ifndef __AHCI_CTRL_H
#define __AHCI_CTRL_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_AHCI_EN > 0)

LW_API INT                  API_AhciCtrlDrvDel(AHCI_CTRL_HANDLE hCtrlHandle,
                                               AHCI_DRV_HANDLE  hDrvHandle);
LW_API INT                  API_AhciCtrlDrvDel(AHCI_CTRL_HANDLE hCtrlHandle,
                                               AHCI_DRV_HANDLE  hDrvHandle);
LW_API INT                  API_AhciCtrlDelete(AHCI_CTRL_HANDLE hCtrl);
LW_API INT                  API_AhciCtrlAdd(AHCI_CTRL_HANDLE hCtrl);
LW_API AHCI_CTRL_HANDLE     API_AhciCtrlHandleGetFromPciArg(PVOID pvCtrlPciArg);
LW_API AHCI_CTRL_HANDLE     API_AhciCtrlHandleGetFromName(CPCHAR cpcName, UINT uiUnit);
LW_API AHCI_CTRL_HANDLE     API_AhciCtrlHandleGetFromIndex(UINT uiIndex);
LW_API INT                  API_AhciCtrlIndexGet(VOID);
LW_API UINT32               API_AhciCtrlCountGet(VOID);
LW_API INT                  API_AhciCtrlInit(VOID);

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_AHCI_EN > 0)        */
#endif                                                                  /*  __AHCI_CTRL_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
