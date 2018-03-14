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
** 文   件   名: nvmeLib.c
**
** 创   建   人: Hui.Kai (惠凯)
**
** 文件创建日期: 2017 年 7 月 17 日
**
** 描        述: NVMe 驱动库.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_NVME_EN > 0)
#include "nvmeLib.h"
#include "nvmeDrv.h"
#include "nvmeDev.h"
/*********************************************************************************************************
** 函数名称: API_NvmeCtrlIntConnect
** 功能描述: 链接控制器中断
** 输　入  : hCtrl      控制器句柄
**           hQueue     命令队列
**           pfuncIsr   中断服务函数
**           cpcName    中断名称
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_NvmeCtrlIntConnect (NVME_CTRL_HANDLE   hCtrl,
                             NVME_QUEUE_HANDLE  hQueue,
                             PINT_SVR_ROUTINE   pfuncIsr,
                             CPCHAR             cpcName)
{
    INT                 iRet;
    NVME_DRV_HANDLE     hDrv;

    hDrv = hCtrl->NVMECTRL_hDrv;

    if (hDrv->NVMEDRV_pfuncVendorCtrlIntConnect) {
        iRet = hDrv->NVMEDRV_pfuncVendorCtrlIntConnect(hCtrl, hQueue, pfuncIsr, cpcName);
    } else {
        iRet = ERROR_NONE;
    }

    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    if (hDrv->NVMEDRV_pfuncVendorCtrlIntEnable) {
        hDrv->NVMEDRV_pfuncVendorCtrlIntEnable(hCtrl, hQueue, pfuncIsr, cpcName);
    }

    return  (ERROR_NONE);
}

#endif                                                                  /* (LW_CFG_DEVICE_EN > 0) &&    */
                                                                        /* (LW_CFG_NVME_EN > 0)         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
