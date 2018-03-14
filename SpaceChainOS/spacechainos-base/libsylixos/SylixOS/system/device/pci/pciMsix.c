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
** 文   件   名: pciMsix.c
**
** 创   建   人: Hui.Kai (惠凯)
**
** 文件创建日期: 2017 年 07 月 26 日
**
** 描        述: PCI 总线 MSI-X 管理.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PCI_EN > 0)
#include "linux/compat.h"
#include "pciLib.h"
/*********************************************************************************************************
** 函数名称: API_PciMsixMsgWrite
** 功能描述: 写 MSI-X 消息
** 输　入  : pvAddr         中断条目首地址
**           iVector        中断号
**           ppmmMsg        消息内容
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciMsixMsgWrite (PVOID  pvAddr, UINT  iVector, PCI_MSI_MSG  *ppmmMsg)
{
    if (!pvAddr) {
        return  (PX_ERROR);
    }

    if (!ppmmMsg) {
        return  (PX_ERROR);
    }

    writel(ppmmMsg->uiAddressLo, (addr_t)pvAddr + iVector * PCI_MSIX_ENTRY_SIZE + PCI_MSIX_ENTRY_LOWER_ADDR);
    writel(ppmmMsg->uiAddressHi, (addr_t)pvAddr + iVector * PCI_MSIX_ENTRY_SIZE + PCI_MSIX_ENTRY_UPPER_ADDR);
    writel(ppmmMsg->uiData,      (addr_t)pvAddr + iVector * PCI_MSIX_ENTRY_SIZE + PCI_MSIX_ENTRY_DATA);
    writel(0,                    (addr_t)pvAddr + iVector * PCI_MSIX_ENTRY_SIZE + PCI_MSIX_ENTRY_VECTOR_CTRL);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciMsixMsgRead
** 功能描述: 读 MSI-X 消息
** 输　入  : pvAddr         中断条目首地址
**           iVector        中断号
**           ppmmMsg        消息内容
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciMsixMsgRead (PVOID  pvAddr, UINT  iVector, PCI_MSI_MSG  *ppmmMsg)
{
    if (!pvAddr) {
        return  (PX_ERROR);
    }

    if (!ppmmMsg) {
        return  (PX_ERROR);
    }

    ppmmMsg->uiAddressLo = readl((addr_t)pvAddr + iVector * PCI_MSIX_ENTRY_SIZE + PCI_MSIX_ENTRY_LOWER_ADDR);
    ppmmMsg->uiAddressHi = readl((addr_t)pvAddr + iVector * PCI_MSIX_ENTRY_SIZE + PCI_MSIX_ENTRY_UPPER_ADDR);
    ppmmMsg->uiData      = readl((addr_t)pvAddr + iVector * PCI_MSIX_ENTRY_SIZE + PCI_MSIX_ENTRY_DATA);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciMsixPendingPosGet
** 功能描述: 获取 MSI-X Pending 位置
** 输　入  : iBus           总线号
**           iSlot          插槽
**           iFunc          功能
**           uiMsixCapOft   偏移地址
**           pulPos         标记位置
**                          0  掩码标记无效
**                          1  掩码标记有效
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciMsixPendingPosGet (INT  iBus, INT  iSlot, INT  iFunc, UINT32  uiMsixCapOft, ULONG  *pulPos)
{
    INT                     iRet       = PX_ERROR;
    UINT32                  uiMsiTab   = 0;
    UINT32                  iBarIndex  = 0;
    UINT32                  iOffset    = 0;
    addr_t                  ulBaseAddr = 0;
    PCI_RESOURCE_HANDLE     hResource  = LW_NULL;
                                                                        /*  读取 MSI-X PBA 字段         */
    iRet = API_PciConfigInDword(iBus, iSlot, iFunc, uiMsixCapOft + PCI_MSIX_PBA, &uiMsiTab);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    iBarIndex  = uiMsiTab & PCI_MSIX_PBA_BIR;                           /* MSI-X PBA所在BAR空间索引     */
    iOffset    = uiMsiTab & PCI_MSIX_PBA_OFFSET;                        /* MSI-X PBA所在BAR空间中偏移   */

    hResource  = API_PciResourceGet(iBus, iSlot, iFunc, PCI_IORESOURCE_MEM, iBarIndex);
    ulBaseAddr = (ULONG)(PCI_RESOURCE_START(hResource));                /* MSI-X PBA所在BAR空间基址     */

    if (pulPos) {
        *pulPos = ulBaseAddr + iOffset;                                 /* MSI-X PBA 的PCI总线域地址    */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciMsixPendingGet
** 功能描述: 获取 MSI-X Pending 状态
** 输　入  : iBus           总线号
**           iSlot          插槽
**           iFunc          功能
**           uiMsixCapOft   偏移地址
**           iVector        中断号
**           piPending      获取到的状态
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciMsixPendingGet (INT      iBus,
                            INT      iSlot,
                            INT      iFunc,
                            UINT32   uiMsixCapOft,
                            INT      iVector,
                            INT     *piPending)
{
    INT          iRet       = PX_ERROR;
    addr_t       ulBaseAddr = 0;
    INT          iEntry     = (iVector + 64) >> 6;                      /* 每个条目包含 64 个 pending 位*/
    UINT64       ullPended  = 0;
    UINT64      *pullAddr   = LW_NULL;

                                                                        /*  获取 Pending Table 位置     */
    iRet = API_PciMsixPendingPosGet(iBus, iSlot, iFunc, uiMsixCapOft, &ulBaseAddr);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
                                                                        /*  映射Pending Table到逻辑空间 */
    pullAddr = (UINT64 *)API_PciDevIoRemap((PVOID)ulBaseAddr, iEntry *  sizeof(UINT64));
    if (pullAddr == LW_NULL) {
        return  (PX_ERROR);
    }

    ullPended = readq(pullAddr + (iVector >> 6));                       /*  读取中断所在条目            */
    if (piPending) {
        *piPending = !!(ullPended & (1 << (iVector & 0x3f)));           /*  获取中断对应Pending位       */
    }

    API_VmmIoUnmap((PVOID)pullAddr);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciMsixTablePosGet
** 功能描述: 获取 MSI-X Table位置
** 输　入  : iBus           总线号
**           iSlot          插槽
**           iFunc          功能
**           uiMsixCapOft   偏移地址
**           pulMaskPos     MSI-X Table位置
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciMsixTablePosGet (INT  iBus, INT  iSlot, INT  iFunc, UINT32  uiMsixCapOft, ULONG  *pulTablePos)
{
    INT                     iRet       = PX_ERROR;
    UINT32                  uiMsiTab   = 0;
    UINT32                  iBarIndex  = 0;
    UINT32                  iOffset    = 0;
    addr_t                  ulBaseAddr = 0;
    PCI_RESOURCE_HANDLE     hResource  = LW_NULL;
                                                                        /*  读取 MSI-X Table 字段       */
    iRet = API_PciConfigInDword(iBus, iSlot, iFunc, uiMsixCapOft + PCI_MSIX_TABLE, &uiMsiTab);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    iBarIndex  = uiMsiTab & PCI_MSIX_TABLE_BIR;                         /* MSI-X Table所在BAR空间索引   */
    iOffset    = uiMsiTab & PCI_MSIX_TABLE_OFFSET;                      /* MSI-X Table所在BAR空间中偏移 */

    hResource  = API_PciResourceGet(iBus, iSlot, iFunc, PCI_IORESOURCE_MEM, iBarIndex);
    ulBaseAddr = (ULONG)(PCI_RESOURCE_START(hResource));                /* MSI-X Table所在BAR空间基址   */

    if (pulTablePos) {
        *pulTablePos = ulBaseAddr + iOffset;                            /* MSI-X Table 的PCI总线域地址  */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciMsixMaskSet
** 功能描述: 设置 MSI-X 掩码状态
** 输　入  : iBus           总线号
**           iSlot          插槽
**           iFunc          功能
**           uiMsixCapOft   偏移地址
**           iVector        中断号
**           piPending      设置的状态
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciMsixMaskSet (INT     iBus,
                         INT     iSlot,
                         INT     iFunc,
                         UINT32  uiMsixCapOft,
                         INT     iVector,
                         INT     iMask)
{
    INT         iRet       = PX_ERROR;
    PVOID       pvAddr     = LW_NULL;
    addr_t      ulBaseAddr = 0;
    UINT32      uiControl  = 0;
                                                                        /*  获取 MSI-X Table 位置       */
    iRet = API_PciMsixTablePosGet(iBus, iSlot, iFunc, uiMsixCapOft, &ulBaseAddr);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
                                                                        /*  映射 MSI-X Tabel到虚拟地址  */
    pvAddr = API_PciDevIoRemap((PVOID)ulBaseAddr, (iVector + 1) * 16);
    if (pvAddr == LW_NULL) {
        return  (PX_ERROR);
    }
                                                                        /*  读取MSI-X Table中断控制字段 */
    uiControl  = readl((addr_t)pvAddr + (iVector * PCI_MSIX_ENTRY_SIZE) + PCI_MSIX_ENTRY_VECTOR_CTRL);
    uiControl &= ~PCI_MSIX_ENTRY_CTRL_MASKBIT;                          /*  重新设置控制字段            */
    if (iMask) {
        uiControl |= PCI_MSIX_ENTRY_CTRL_MASKBIT;
    }
                                                                        /*  回写MSI-X Table中断控制字段 */
    writel(uiControl, (addr_t)pvAddr + (iVector * PCI_MSIX_ENTRY_SIZE) + PCI_MSIX_ENTRY_VECTOR_CTRL);

    API_VmmIoUnmap(pvAddr);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciMsixMaskGet
** 功能描述: 获取 MSI-X 掩码状态
** 输　入  : iBus           总线号
**           iSlot          插槽
**           iFunc          功能
**           uiMsixCapOft   偏移地址
**           iVector        中断号
**           piMask         获取到的状态
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciMsixMaskGet (INT      iBus,
                         INT      iSlot,
                         INT      iFunc,
                         UINT32   uiMsixCapOft,
                         INT      iVector,
                         INT     *piMask)
{
    INT         iRet       = PX_ERROR;
    PVOID       pvAddr     = LW_NULL;
    addr_t      ulBaseAddr = 0;
    UINT32      uiControl  = 0;

                                                                        /*  获取 MSI-X Table 位置       */
    iRet = API_PciMsixTablePosGet(iBus, iSlot, iFunc, uiMsixCapOft, &ulBaseAddr);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
                                                                        /*  获取 MSI-X Table 位置       */
    pvAddr = API_PciDevIoRemap((PVOID)ulBaseAddr, (iVector + 1) * PCI_MSIX_ENTRY_SIZE);
    if (pvAddr == LW_NULL) {
        return  (PX_ERROR);
    }
                                                                        /*  读取MSI-X Table中断控制字段 */
    uiControl = readl((addr_t)pvAddr + (iVector * PCI_MSIX_ENTRY_SIZE) + PCI_MSIX_ENTRY_VECTOR_CTRL);
    if (piMask) {
        *piMask = uiControl & PCI_MSIX_ENTRY_CTRL_MASKBIT;
    }

    API_VmmIoUnmap(pvAddr);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciMsixVecCountGet
** 功能描述: 获取 MSI-X 向量数
** 输　入  : iBus           总线号
**           iSlot          插槽
**           iFunc          功能
**           uiMsiCapOft    偏移地址
**           puiVecCount    向量数
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciMsixVecCountGet (INT  iBus, INT  iSlot, INT  iFunc, UINT32  uiMsiCapOft, UINT32  *puiVecCount)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;

    if (uiMsiCapOft <= 0) {
        return  (PX_ERROR);
    }
                                                                        /*  读取 MSI-X 控制状态字段     */
    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, uiMsiCapOft + PCI_MSIX_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    if (puiVecCount) {
        *puiVecCount = (usControl & PCI_MSIX_FLAGS_QSIZE) + 1;          /*  获取 MSI-X 向量数量         */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciMsixFunctionMaskSet
** 功能描述: 设置 MSI-X 全局掩码功能
** 输　入  : iBus           总线号
**           iSlot          插槽
**           iFunc          功能
**           uiMsiCapOft    偏移地址
**           iEnable        使能与禁能标志
**                          0  禁能
**                          1  使能
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciMsixFunctionMaskSet (INT  iBus, INT  iSlot, INT  iFunc, UINT32  uiMsiCapOft, INT  iEnable)
{
    INT         iRet         = PX_ERROR;
    UINT16      usControl    = 0;
    UINT16      usControlNew = 0;

    if (uiMsiCapOft <= 0) {
        return  (PX_ERROR);
    }
                                                                        /*  读取 MSI-X 控制状态字段     */
    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, uiMsiCapOft + PCI_MSIX_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    usControlNew = usControl & (~(PCI_MSIX_FLAGS_MASKALL));             /*  设置 MSI-X 全局掩码位       */
    if (iEnable) {
        usControlNew |= PCI_MSIX_FLAGS_MASKALL;
    }                                                                   /*  重新回写 MSI-X 控制状态字段 */
    iRet = API_PciConfigOutWord(iBus, iSlot, iFunc, uiMsiCapOft + PCI_MSIX_FLAGS, usControlNew);

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_PciMsixFunctionMaskGet
** 功能描述: 获取 MSI-X 全局掩码功能
** 输　入  : iBus           总线号
**           iSlot          插槽
**           iFunc          功能
**           uiMsiCapOft    偏移地址
**           iEnable        使能与禁能标志
**                          0  禁能
**                          1  使能
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciMsixFunctionMaskGet (INT  iBus, INT  iSlot, INT  iFunc, UINT32  uiMsiCapOft, INT *piEnable)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;

    if (uiMsiCapOft <= 0) {
        return  (PX_ERROR);
    }
                                                                        /*  读取 MSI-X 控制状态字段     */
    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, uiMsiCapOft + PCI_MSIX_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    if (piEnable) {
        *piEnable = !!(usControl & PCI_MSIX_FLAGS_MASKALL);             /*  获取全局掩码位              */
    }

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_PciMsiEnableSet
** 功能描述: MSI-X 使能控制
** 输　入  : iBus           总线号
**           iSlot          插槽
**           iFunc          功能
**           uiMsiCapOft    偏移地址
**           iEnable        使能与禁能标志
**                          0  禁能
**                          1  使能
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciMsixEnableSet (INT  iBus, INT  iSlot, INT  iFunc, UINT32  uiMsiCapOft, INT  iEnable)
{
    INT         iRet         = PX_ERROR;
    UINT16      usControl    = 0;
    UINT16      usControlNew = 0;

    if (uiMsiCapOft <= 0) {
        return  (PX_ERROR);
    }
                                                                        /*  读取 MSI-X 控制状态字段     */
    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, uiMsiCapOft + PCI_MSIX_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    usControlNew = usControl & (~(PCI_MSIX_FLAGS_ENABLE));              /*  设置 MSI-X 使能状态位       */
    if (iEnable) {
        usControlNew |= PCI_MSIX_FLAGS_ENABLE;
    }                                                                   /*  重新设置 MSI-X 控制状态字段 */
    iRet = API_PciConfigOutWord(iBus, iSlot, iFunc, uiMsiCapOft + PCI_MSIX_FLAGS, usControlNew);

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_PciMsiEnableGet
** 功能描述: 获取 MSI-X 使能控制状态
** 输　入  : iBus           总线号
**           iSlot          插槽
**           iFunc          功能
**           uiMsiCapOft    偏移地址
**           piEnable       使能与禁能标志
**                          0  禁能
**                          1  使能
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciMsixEnableGet (INT  iBus, INT  iSlot, INT  iFunc, UINT32  uiMsiCapOft, INT *piEnable)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;

    if (uiMsiCapOft <= 0) {
        return  (PX_ERROR);
    }
                                                                        /*  读取 MSI-X 控制状态字段     */
    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, uiMsiCapOft + PCI_MSIX_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    if (piEnable) {
        *piEnable = !!(usControl & PCI_MSIX_FLAGS_ENABLE);              /*  获取使能控制状态位          */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciDevMsiRangeEnable
** 功能描述: 设置 MSI-X 区域使能
** 输　入  : hHandle        设备控制句柄
**           uiVecMin       使能区域中断向量最小值
**           uiVecMax       使能区域中断向量最大值
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
** 注  意  : 每个 PCI 设备驱动程序只需要调用一次
**
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciDevMsixRangeEnable (PCI_DEV_HANDLE       hHandle,
                                PCI_MSI_DESC_HANDLE  hMsgHandle,
                                UINT                 uiVecMin,
                                UINT                 uiVecMax)
{
    INT         i           = 0;
    INT         iRet        = PX_ERROR;
    INT         iEnable     = 0;
    UINT32      uiMsiCapOft = 0;
    UINT32      uiVecNum    = 0;
    addr_t      ulBaseAddr;
    PVOID       pvAddr      = 0;

    if (hHandle == LW_NULL) {
        return  (PX_ERROR);
    }
    
    if ((uiVecMax < uiVecMin) || (uiVecMin < 1)) {
        return  (PX_ERROR);
    }

    iRet = API_PciCapFind(hHandle->PCIDEV_iDevBus,                      /*  获取 MSI-X 结构体偏移       */
                          hHandle->PCIDEV_iDevDevice,
                          hHandle->PCIDEV_iDevFunction,
                          PCI_CAP_ID_MSIX,
                          &uiMsiCapOft);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    iRet = API_PciMsixEnableGet(hHandle->PCIDEV_iDevBus,                /* 获取 MSI-X 使能控制状态      */
                                hHandle->PCIDEV_iDevDevice,
                                hHandle->PCIDEV_iDevFunction,
                                uiMsiCapOft,
                                &iEnable);
    if (!iEnable) {
        return  (PX_ERROR);
    }

    iRet = API_PciMsixVecCountGet(hHandle->PCIDEV_iDevBus,              /*  获取 MSI-X 向量数           */
                                  hHandle->PCIDEV_iDevDevice,
                                  hHandle->PCIDEV_iDevFunction,
                                  uiMsiCapOft, &uiVecNum);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    if (uiVecNum < uiVecMin) {
        return  (PX_ERROR);
    
    } else if (uiVecNum > uiVecMax) {
        uiVecNum = uiVecMax;
    }

    iRet = API_PciMsixTablePosGet(hHandle->PCIDEV_iDevBus,              /*  获取 MSI-X Table 位置       */
                                  hHandle->PCIDEV_iDevDevice,
                                  hHandle->PCIDEV_iDevFunction,
                                  uiMsiCapOft,
                                  &ulBaseAddr);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
                                                                        /*  映射 MSI-X Tabel到虚拟地址  */
    pvAddr = API_PciDevIoRemap((PVOID)ulBaseAddr, uiVecNum * PCI_MSIX_ENTRY_SIZE);
    if (pvAddr == LW_NULL) {
        return  (PX_ERROR);
    }

    for (i = 0; i < uiVecNum; i++) {                                    /*  循环申请每个 MSI-X 中断     */
        hMsgHandle[i].PCIMSI_uiNum = 1;                                 /*  向系统申请一个 MSI 中断     */
        
        iRet = API_PciDevInterMsiGet(hHandle, &hMsgHandle[i]);
        if (iRet != ERROR_NONE) {
            break;
        }
                                                                        /*  写 MSI-X 消息               */
        iRet = API_PciMsixMsgWrite(pvAddr, i, &hMsgHandle[i].PCIMSI_pmmMsg);
        if (iRet != ERROR_NONE) {
            break;
        }

        hHandle->PCIDEV_uiDevIrqMsiNum++;                               /*  更新 MSI-X 数量             */
    }

    API_VmmIoUnmap(pvAddr);                                             /*  释放占用的逻辑空间          */

    if (i >= uiVecMin) {
        return  (ERROR_NONE);

    } else {
        return  (iRet);                                                 /*  TODO: 出错需要回收中断向量  */
    }
}
/*********************************************************************************************************
** 函数名称: API_PciDevMsixVecCountGet
** 功能描述: 获取 MSI 向量数
** 输　入  : hHandle        设备控制句柄
**           puiVecCount    向量数
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciDevMsixVecCountGet (PCI_DEV_HANDLE  hHandle, UINT32  *puiVecCount)
{
    INT         iRet        = PX_ERROR;
    UINT32      uiMsiCapOft = 0;

    if (hHandle == LW_NULL) {
        return  (PX_ERROR);
    }

    iRet = API_PciCapFind(hHandle->PCIDEV_iDevBus,                      /*  获取 MSI-X 结构体偏移       */
                          hHandle->PCIDEV_iDevDevice,
                          hHandle->PCIDEV_iDevFunction,
                          PCI_CAP_ID_MSI,
                          &uiMsiCapOft);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    iRet = API_PciMsixVecCountGet(hHandle->PCIDEV_iDevBus,              /*  获取 MSI-X 向量数           */
                                  hHandle->PCIDEV_iDevDevice,
                                  hHandle->PCIDEV_iDevFunction,
                                  uiMsiCapOft, puiVecCount);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciDevMsixEnableGet
** 功能描述: 获取 MSI-X 使能控制状态
** 输　入  : hHandle        设备控制句柄
**           piEnable       使能与禁能标志
**                          0  禁能
**                          1  使能
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciDevMsixEnableGet (PCI_DEV_HANDLE  hHandle, INT  *piEnable)
{
    INT         iRet        = PX_ERROR;
    UINT32      uiMsiCapOft = 0;

    if (hHandle == LW_NULL) {
        return  (PX_ERROR);
    }

    iRet = API_PciCapFind(hHandle->PCIDEV_iDevBus,                      /*  获取 MSI-X 结构体偏移       */
                          hHandle->PCIDEV_iDevDevice,
                          hHandle->PCIDEV_iDevFunction,
                          PCI_CAP_ID_MSIX,
                          &uiMsiCapOft);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    iRet = API_PciMsixEnableGet(hHandle->PCIDEV_iDevBus,                /*  获取 MSI-X 使能控制状态     */
                                hHandle->PCIDEV_iDevDevice,
                                hHandle->PCIDEV_iDevFunction,
                                uiMsiCapOft,
                                piEnable);
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_PciDevMsiEnableSet
** 功能描述: 设置 MSI-X 使能控制状态
**           注意：设置使能时，默认设置禁能MSI，INTx状态无意义；设置失能时，对MSI和INTx不做操作。
** 输　入  : hHandle        设备控制句柄
**           iEnable        使能与禁能标志
**                          0  禁能
**                          1  使能
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciDevMsixEnableSet (PCI_DEV_HANDLE  hHandle, INT  iEnable)
{
    INT         iRet        = PX_ERROR;
    UINT32      uiMsiCapOft = 0;

    if (hHandle == LW_NULL) {
        return  (PX_ERROR);
    }

    if (iEnable) {
        API_PciDevMsiEnableSet(hHandle, 0);                             /*  禁能 MSI                    */
    }

    iRet = API_PciCapFind(hHandle->PCIDEV_iDevBus,                      /*  获取 MSI-X 结构体偏移       */
                          hHandle->PCIDEV_iDevDevice,
                          hHandle->PCIDEV_iDevFunction,
                          PCI_CAP_ID_MSIX,
                          &uiMsiCapOft);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    iRet = API_PciMsixEnableSet(hHandle->PCIDEV_iDevBus,                /*  设置 MSI-X 使能控制         */
                                hHandle->PCIDEV_iDevDevice,
                                hHandle->PCIDEV_iDevFunction,
                                uiMsiCapOft,
                                iEnable);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    hHandle->PCIDEV_iDevIrqMsiEn = iEnable;

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PCI_EN > 0)         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
