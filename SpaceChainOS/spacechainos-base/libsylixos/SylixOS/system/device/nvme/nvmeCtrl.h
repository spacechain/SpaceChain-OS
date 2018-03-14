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
** 文   件   名: nvmeCtrl.h
**
** 创   建   人: Qin.Fei (秦飞)
**
** 文件创建日期: 2017 年 7 月 17 日
**
** 描        述: NVMe 控制器管理.
*********************************************************************************************************/

#ifndef __NVME_CTRL_H
#define __NVME_CTRL_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_NVME_EN > 0)

LW_API INT                  API_NvmeCtrlDelete(NVME_CTRL_HANDLE  hCtrl);
LW_API INT                  API_NvmeCtrlAdd(NVME_CTRL_HANDLE  hCtrl);
LW_API NVME_CTRL_HANDLE     API_NvmeCtrlHandleGetFromPciArg(PVOID  pvCtrlPciArg);
LW_API NVME_CTRL_HANDLE     API_NvmeCtrlHandleGetFromName(CPCHAR  cpcName, UINT  uiUnit);
LW_API NVME_CTRL_HANDLE     API_NvmeCtrlHandleGetFromIndex(UINT  uiIndex);
LW_API INT                  API_NvmeCtrlIndexGet(VOID);
LW_API UINT32               API_NvmeCtrlCountGet(VOID);
LW_API INT                  API_NvmeCtrlInit(VOID);

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_NVME_EN > 0)        */
#endif                                                                  /*  __NVME_CTRL_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
