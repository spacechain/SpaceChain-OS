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
** 文   件   名: ahciDrv.h
**
** 创   建   人: Gong.YuJian (弓羽箭)
**
** 文件创建日期: 2016 年 03 月 29 日
**
** 描        述: AHCI 驱动管理.
*********************************************************************************************************/

#ifndef __AHCI_DRV_H
#define __AHCI_DRV_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_AHCI_EN > 0)

LW_API AHCI_DRV_CTRL_HANDLE API_AhciDrvCtrlFind(AHCI_DRV_HANDLE  hDrvHandle,
                                                AHCI_CTRL_HANDLE hCtrlHandle);
LW_API INT                  API_AhciDrvCtrlDelete(AHCI_DRV_HANDLE  hDrvHandle,
                                                  AHCI_CTRL_HANDLE hCtrlHandle);
LW_API INT                  API_AhciDrvCtrlAdd(AHCI_DRV_HANDLE  hDrvHandle,
                                               AHCI_CTRL_HANDLE hCtrlHandle);
LW_API AHCI_DRV_HANDLE      API_AhciDrvHandleGet(CPCHAR  cpcName);
LW_API INT                  API_AhciDrvDelete(AHCI_DRV_HANDLE  hDrvHandle);
LW_API INT                  API_AhciDrvRegister(AHCI_DRV_HANDLE  hDrv);
LW_API INT                  API_AhciDrvInit(VOID);

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_AHCI_EN > 0)        */
#endif                                                                  /*  __AHCI_DRV_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
