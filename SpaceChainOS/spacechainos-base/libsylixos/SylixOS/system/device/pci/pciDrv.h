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
** 文   件   名: pciDrv.h
**
** 创   建   人: Gong.YuJian (弓羽箭)
**
** 文件创建日期: 2015 年 12 月 23 日
**
** 描        述: PCI 总线设备驱动管理.
*********************************************************************************************************/

#ifndef __PCIDRV_H
#define __PCIDRV_H

#include "pciPm.h"
#include "pciDev.h"
#include "pciError.h"

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PCI_EN > 0)

/*********************************************************************************************************
  驱动配置
*********************************************************************************************************/
#define PCI_DRV_NAME_MAX        (24 + 1)                                /*  驱动名称最大值              */
#define PCI_DRV_FLAG_ACTIVE     0x01                                    /*  是否激活                    */

/*********************************************************************************************************
  驱动支持设备列表控制块
*********************************************************************************************************/
typedef struct {
    UINT32                  PCIDEVID_uiVendor;                          /* 厂商 ID                      */
    UINT32                  PCIDEVID_uiDevice;                          /* 设备 ID                      */

    UINT32                  PCIDEVID_uiSubVendor;                       /* 子厂商 ID                    */
    UINT32                  PCIDEVID_uiSubDevice;                       /* 子设备 ID                    */

    UINT32                  PCIDEVID_uiClass;                           /* 设备类                       */
    UINT32                  PCIDEVID_uiClassMask;                       /* 设备子类                     */

    ULONG                   PCIDEVID_ulData;                            /* 设备私有数据                 */
} PCI_DEV_ID_CB;
typedef PCI_DEV_ID_CB      *PCI_DEV_ID_HANDLE;

/*********************************************************************************************************
  驱动控制块
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE            PCIDRV_lineDrvNode;                         /* 驱动节点管理                 */
    CHAR                    PCIDRV_cDrvName[PCI_DRV_NAME_MAX];          /* 驱动名称                     */
    PVOID                   PCIDRV_pvPriv;                              /* 私有数据                     */
    PCI_DEV_ID_HANDLE       PCIDRV_hDrvIdTable;                         /* 设备支持列表                 */
    UINT32                  PCIDRV_uiDrvIdTableSize;                    /* 设备支持列表大小             */

    /*
     *  驱动常用函数, PCIDRV_pfuncDevProbe 与 PCIDRV_pfuncDevRemove 不能为 LW_NULL, 其它可选
     */
    INT   (*PCIDRV_pfuncDevProbe)(PCI_DEV_HANDLE hHandle, const PCI_DEV_ID_HANDLE hIdEntry);
    VOID  (*PCIDRV_pfuncDevRemove)(PCI_DEV_HANDLE hHandle);
    INT   (*PCIDRV_pfuncDevSuspend)(PCI_DEV_HANDLE hHandle, PCI_PM_MESSAGE_HANDLE hPmMsg);
    INT   (*PCIDRV_pfuncDevSuspendLate)(PCI_DEV_HANDLE hHandle, PCI_PM_MESSAGE_HANDLE hPmMsg);
    INT   (*PCIDRV_pfuncDevResumeEarly)(PCI_DEV_HANDLE hHandle);
    INT   (*PCIDRV_pfuncDevResume)(PCI_DEV_HANDLE hHandle);
    VOID  (*PCIDRV_pfuncDevShutdown)(PCI_DEV_HANDLE hHandle);

    PCI_ERROR_HANDLE        PCIDRV_hDrvErrHandler;                      /* 错误处理句柄                 */

    INT                     PCIDRV_iDrvFlag;                            /* 驱动标志                     */
    UINT32                  PCIDRV_uiDrvDevNum;                         /* 关联设备数                   */
    LW_LIST_LINE_HEADER     PCIDRV_plineDrvDevHeader;                   /* 设备管理链表头               */
} PCI_DRV_CB;
typedef PCI_DRV_CB         *PCI_DRV_HANDLE;

/*********************************************************************************************************
  API
  API_PciConfigInit() 必须在 BSP 初始化总线系统时被调用, 而且必须保证是第一个被正确调用的 PCI 系统函数.
*********************************************************************************************************/

LW_API PCI_DRV_HANDLE       API_PciDrvHandleGet(CPCHAR pcName);
LW_API INT                  API_PciDrvRegister(PCI_DRV_HANDLE hHandle);

/*********************************************************************************************************
  API Macro
*********************************************************************************************************/

#define pciDrvHandleGet     API_PciDrvHandleGet
#define pciDrvRegister      API_PciDrvRegister

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PCI_EN > 0)         */
#endif                                                                  /*  __PCIDRV_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
