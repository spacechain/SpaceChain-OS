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
** 文   件   名: pciNullDev.c
**
** 创   建   人: Gong.YuJian (弓羽箭)
**
** 文件创建日期: 2016 年 06 月 11 日
**
** 描        述: PCI NULL (示例) 设备驱动.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_PCI_DRV
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PCI_EN > 0)
#include "pciNullDev.h"
#include "pci_ids.h"
/*********************************************************************************************************
  读取设备寄存器信息 read32((addr_t)((ULONG)(ctrl)->addr + reg)).
*********************************************************************************************************/
#define PCI_NONE_READ(ctrl, reg)    0
/*********************************************************************************************************
  板载类型, 设备驱动扩展数据, 与 Linux 扩展数据保持一致, 此处只列举出部分内容.
*********************************************************************************************************/
enum {
    board_none,
    board_none_yes_fbs
};
/*********************************************************************************************************
  驱动支持的设备 ID 表, 用于驱动与设备进行自动匹配, 与 Linux 参数保持一致, 此处只列举出部分内容.
*********************************************************************************************************/
static const PCI_DEV_ID_CB  pciNullDrvIdTbl[] = {
    {
        PCI_VDEVICE(INTEL, 0x2652), 
        board_none  
    },
    {
        PCI_VDEVICE(INTEL, 0x2653), 
        board_none  
    },
    {   
    }                                                                   /* terminate list               */
};
/*********************************************************************************************************
** 函数名称: pciNullDevIsr
** 功能描述: PCI 设备中断服务
** 输　入  : pvArg      中断参数
**           ulVector   中断向量
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static irqreturn_t  pciNullDevIsr (PVOID  pvArg, ULONG  ulVector)
{
    UINT32  uiReg;                                                      /*  寄存器信息                  */

    uiReg = PCI_NONE_READ(pvArg, 0);                                    /*  获取中状态                  */
    if (!uiReg) {                                                       /*  不是本设备的中断            */
        return  (LW_IRQ_NONE);                                          /*  标识不是本设备的中断        */
    }

    /*
     *  TODO 中断处理
     */
    return  (LW_IRQ_HANDLED);                                            /*  标识是本设备的中断         */
}
/*********************************************************************************************************
** 函数名称: pciNullDevIdTblGet
** 功能描述: 获取设备 ID 表的表头与表的大小
** 输　入  : hPciDevId      设备 ID 列表句柄缓冲区
**           puiSzie        设备 ID 列表大小缓冲区
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  pciNullDevIdTblGet (PCI_DEV_ID_HANDLE *hPciDevId, UINT32 *puiSzie)
{
    if ((!hPciDevId) || (!puiSzie)) {                                   /*  参数无效                    */
        return  (PX_ERROR);                                             /*  错误返回                    */
    }

    *hPciDevId = (PCI_DEV_ID_HANDLE)pciNullDrvIdTbl;                    /*  获取表头                    */
    *puiSzie   = sizeof(pciNullDrvIdTbl) / sizeof(PCI_DEV_ID_CB);       /*  获取表的大小                */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pciNullDevRemove
** 功能描述: 总线上移除 PCI 设备时的处理
** 输　入  : hPciDevHandle     PCI 设备句柄
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  pciNullDevRemove (PCI_DEV_HANDLE hPciDevHandle)
{
    /*
     *  TODO 回收驱动创建的资源 (内存, 信号量, 线程等), 同时可用于处理设备的电源管理等.
     */
}
/*********************************************************************************************************
** 函数名称: pciNullDevProbe
** 功能描述: 总线探测到 ID 列表里设备时的处理
** 输　入  : hPciDevHandle      PCI 设备句柄
**           hIdEntry           匹配成功的设备 ID 条目(来自于设备 ID 表)
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  pciNullDevProbe (PCI_DEV_HANDLE hPciDevHandle, const PCI_DEV_ID_HANDLE hIdEntry)
{
    PCI_RESOURCE_HANDLE     hResource;                                  /*  PCI 设备资源信息            */
    ULONG                   ulBaseAddr;                                 /*  起始地址                    */
    PVOID                   pvBaseAddr;                                 /*  起始地址                    */
    size_t                  stBaseSize;                                 /*  资源大小                    */
    ULONG                   ulIrqVector;                                /*  中断向量                    */

    if ((!hPciDevHandle) || (!hIdEntry)) {                              /*  设备参数无效                */
        _ErrorHandle(EINVAL);                                           /*  标记错误                    */
        return  (PX_ERROR);                                             /*  错误返回                    */
    }

    /*
     *  更新设备参数, 设备驱动版本信息及当前类型设备的索引等
     */
    hPciDevHandle->PCIDEV_uiDevVersion = PCI_NULL_DRV_VER_NUM;          /*  当前设备驱动版本号          */
    hPciDevHandle->PCIDEV_uiUnitNumber = 0;                             /*  本类设备索引                */

    /*
     *  获取设备资源信息 (MEM IO IRQ 等)
     */
    hResource = API_PciDevResourceGet(hPciDevHandle, PCI_IORESOURCE_MEM, 0);
    if (!hResource) {                                                   /*  获取 MEM 资源信息           */
        return  (PX_ERROR);
    }
    
    ulBaseAddr = (ULONG)(PCI_RESOURCE_START(hResource));                /*  获取 MEM 的起始地址         */
    stBaseSize = (size_t)(PCI_RESOURCE_SIZE(hResource));                /*  获取 MEM 的大小             */
    pvBaseAddr = API_PciDevIoRemap((PVOID)ulBaseAddr, stBaseSize);      /*  进行重映射后方可使用        */
    if (!pvBaseAddr) {
        return  (PX_ERROR);
    }
                                                                        /*  获取 IRQ 资源               */
    hResource = API_PciDevResourceGet(hPciDevHandle, PCI_IORESOURCE_IRQ, 0);
    if (!hResource) {
        return  (PX_ERROR);
    }
    
    ulIrqVector = (ULONG)(PCI_RESOURCE_START(hResource));               /*  获取中断向量                */

    /*
     *  TODO 其它处理
     */

    /*
     *  连接中断并使能中断
     */
    API_PciDevInterConnect(hPciDevHandle, ulIrqVector, pciNullDevIsr, LW_NULL, "pci_nulldev");
    API_PciDevInterEnable(hPciDevHandle, ulIrqVector, pciNullDevIsr, LW_NULL);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pciNullDevInit
** 功能描述: 设备驱动初始化
** 输　入  : NONE
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  pciNullDevInit (VOID)
{
    INT                 iRet;                                           /*  操作结果                    */
    PCI_DRV_CB          tPciDrv;                                        /*  驱动控制器用于注册驱动      */
    PCI_DRV_HANDLE      hPciDrv = &tPciDrv;                             /*  驱动控制块句柄              */

    lib_bzero(hPciDrv, sizeof(PCI_DRV_CB));                             /*  复位驱动控制块参数          */
    iRet = pciNullDevIdTblGet(&hPciDrv->PCIDRV_hDrvIdTable, &hPciDrv->PCIDRV_uiDrvIdTableSize);
    if (iRet != ERROR_NONE) {                                           /*  获取设备 ID 表失败          */
        return  (PX_ERROR);
    }
                                                                        /*  设置驱动名称                */
    lib_strlcpy(& hPciDrv->PCIDRV_cDrvName[0], PCI_NULL_DRV_NAME, PCI_DRV_NAME_MAX);
    hPciDrv->PCIDRV_pvPriv         = LW_NULL;                           /*  设备驱动的私有数据          */
    hPciDrv->PCIDRV_hDrvErrHandler = LW_NULL;                           /*  驱动错误处理                */
    hPciDrv->PCIDRV_pfuncDevProbe  = pciNullDevProbe;                   /*  设备探测函数, 不能为空      */
    hPciDrv->PCIDRV_pfuncDevRemove = pciNullDevRemove;                  /*  驱动移除函数, 不能为空      */

    iRet = API_PciDrvRegister(hPciDrv);                                 /*  注册 PCI 设备驱动           */
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PCI_EN > 0) &&      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
