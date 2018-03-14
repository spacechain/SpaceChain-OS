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
** 文   件   名: pciMsi.h
**
** 创   建   人: Gong.YuJian (弓羽箭)
**
** 文件创建日期: 2015 年 09 月 10 日
**
** 描        述: PCI 总线 MSI(Message Signaled Interrupts) 管理.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PCI_EN > 0)
#include "linux/bitops.h"
#include "linux/log2.h"
#include "pciLib.h"
#include "pciMsi.h"
/*********************************************************************************************************
** 函数名称: API_PciMsixClearSet
** 功能描述: MSIx 控制域清除与设置
** 输　入  : iBus               总线号
**           iSlot              插槽
**           iFunc              功能
**           uiMsixCapOft       偏移地址
**           usClear            清除标志
**           usSet              设置标志
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciMsixClearSet (INT     iBus,
                          INT     iSlot,
                          INT     iFunc,
                          UINT32  uiMsixCapOft,
                          UINT16  usClear,
                          UINT16  usSet)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;
    UINT16      usNew     = 0;

    if (uiMsixCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, uiMsixCapOft + PCI_MSIX_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    usNew  = usControl & (~usClear);
    usNew |= usSet;
    if (usNew != usControl) {
        iRet = API_PciConfigOutWord(iBus, iSlot, iFunc, uiMsixCapOft + PCI_MSIX_FLAGS, usNew);
    } else {
        iRet = ERROR_NONE;
    }

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_PciMsiMsgWrite
** 功能描述: 写 MSI 消息
** 输　入  : iBus           总线号
**           iSlot          插槽
**           iFunc          功能
**           uiMsixCapOft   偏移地址
**           ucMultiple     倍数
**           ppmmMsg        消息内容
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciMsiMsgWrite (INT          iBus,
                         INT          iSlot,
                         INT          iFunc,
                         UINT32       uiMsixCapOft,
                         UINT8        ucMultiple,
                         PCI_MSI_MSG *ppmmMsg)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;
    INT         iEnable   = 0;
    INT         iIs64     = 0;

    if (uiMsixCapOft <= 0) {
        return  (PX_ERROR);
    }

    if (!ppmmMsg) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, uiMsixCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    iEnable = !!(usControl & PCI_MSI_FLAGS_ENABLE);
    if (!iEnable) {
        return  (PX_ERROR);
    }
    usControl &= ~PCI_MSI_FLAGS_QSIZE;
    usControl |= ucMultiple << 4;
    iRet = API_PciConfigOutWord(iBus, iSlot, iFunc, uiMsixCapOft + PCI_MSI_FLAGS, usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigOutDword(iBus, iSlot, iFunc,
                                 uiMsixCapOft + PCI_MSI_ADDRESS_LO, ppmmMsg->uiAddressLo);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    iIs64 = !!(usControl & PCI_MSI_FLAGS_64BIT);
    if (iIs64) {
        iRet = API_PciConfigOutDword(iBus, iSlot, iFunc,
                                     uiMsixCapOft + PCI_MSI_ADDRESS_HI, ppmmMsg->uiAddressHi);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
        iRet = API_PciConfigOutWord(iBus, iSlot, iFunc,
                                    uiMsixCapOft + PCI_MSI_DATA_64, (UINT16)ppmmMsg->uiData);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
    } else {
        iRet = API_PciConfigOutWord(iBus, iSlot, iFunc,
                                    uiMsixCapOft + PCI_MSI_DATA_32, (UINT16)ppmmMsg->uiData);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciMsiMsgRead
** 功能描述: 读 MSI 消息
** 输　入  : iBus           总线号
**           iSlot          插槽
**           iFunc          功能
**           uiMsixCapOft   偏移地址
**           ucMultiple     倍数
**           ppmmMsg        消息内容
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciMsiMsgRead (INT          iBus,
                        INT          iSlot,
                        INT          iFunc,
                        UINT32       uiMsixCapOft,
                        UINT8        ucMultiple,
                        PCI_MSI_MSG *ppmmMsg)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;
    INT         iEnable   = 0;
    INT         iIs64     = 0;

    if (uiMsixCapOft <= 0) {
        return  (PX_ERROR);
    }

    if (!ppmmMsg) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, uiMsixCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    iEnable = !!(usControl & PCI_MSI_FLAGS_ENABLE);
    if (!iEnable) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInDword(iBus, iSlot, iFunc,
                                uiMsixCapOft + PCI_MSI_ADDRESS_LO, &ppmmMsg->uiAddressLo);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    iIs64 = !!(usControl & PCI_MSI_FLAGS_64BIT);
    if (iIs64) {
        iRet = API_PciConfigInDword(iBus, iSlot, iFunc,
                                    uiMsixCapOft + PCI_MSI_ADDRESS_HI, &ppmmMsg->uiAddressHi);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
        iRet = API_PciConfigInWord(iBus, iSlot, iFunc,
                                   uiMsixCapOft + PCI_MSI_DATA_64, (UINT16 *)&ppmmMsg->uiData);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
    } else {
        ppmmMsg->uiAddressHi = 0;
        iRet = API_PciConfigInWord(iBus, iSlot, iFunc,
                                   uiMsixCapOft + PCI_MSI_DATA_32, (UINT16 *)&ppmmMsg->uiData);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciMsiPendingSet
** 功能描述: MSI Pending 设置
** 输　入  : iBus           总线号
**           iSlot          插槽
**           iFunc          功能
**           uiMsixCapOft   偏移地址
**           uiPending      指定位
**           uiFlag         新标志
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciMsiPendingSet (INT     iBus,
                           INT     iSlot,
                           INT     iFunc,
                           UINT32  uiMsixCapOft,
                           UINT32  uiPending,
                           UINT32  uiFlag)
{
    INT         iRet        = PX_ERROR;
    UINT16      usControl   = 0;
    INT         iIs64       = 0;
    INT         iIsMask     = 0;
    INT         iPendingPos = 0;
    UINT32      uiPendinged = 0;

    if (uiMsixCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, uiMsixCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    iIsMask = !!(usControl & PCI_MSI_FLAGS_MASK_BIT);
    if (!iIsMask) {
        return  (PX_ERROR);
    }
    iIs64 = !!(usControl & PCI_MSI_FLAGS_64BIT);
    if (iIs64) {
        iPendingPos = uiMsixCapOft + PCI_MSI_PENDING_64;
    } else {
        iPendingPos = uiMsixCapOft + PCI_MSI_PENDING_32;
    }
    uiPendinged &= ~uiPending;
    uiPendinged |=  uiFlag;
    iRet = API_PciConfigOutDword(iBus, iSlot, iFunc, iPendingPos, uiPendinged);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciMsiPendingGet
** 功能描述: 获取 MSI Pending 获取
** 输　入  : iBus           总线号
**           iSlot          插槽
**           iFunc          功能
**           uiMsixCapOft   偏移地址
**           puiPending     获取到的状态
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciMsiPendingGet (INT  iBus, INT  iSlot, INT  iFunc, UINT32  uiMsixCapOft, UINT32 *puiPending)
{
    INT         iRet        = PX_ERROR;
    UINT16      usControl   = 0;
    INT         iEnable     = 0;
    INT         iIs64       = 0;
    INT         iIsMask     = 0;
    INT         iPendingPos = 0;
    UINT32      uiPendinged = 0;

    if (uiMsixCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, uiMsixCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    iEnable = !!(usControl & PCI_MSI_FLAGS_ENABLE);
    if (!iEnable) {
        return  (PX_ERROR);
    }
    iIsMask = !!(usControl & PCI_MSI_FLAGS_MASK_BIT);
    if (!iIsMask) {
        return  (PX_ERROR);
    }
    iIs64 = !!(usControl & PCI_MSI_FLAGS_64BIT);
    if (iIs64) {
        iPendingPos = uiMsixCapOft + PCI_MSI_PENDING_64;
    } else {
        iPendingPos = uiMsixCapOft + PCI_MSI_PENDING_32;
    }
    iRet = API_PciConfigInDword(iBus, iSlot, iFunc, iPendingPos, &uiPendinged);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    if (puiPending) {
        *puiPending = uiPendinged;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciMsiPendingPosGet
** 功能描述: 获取 MSI Pending 位置
** 输　入  : iBus           总线号
**           iSlot          插槽
**           iFunc          功能
**           uiMsixCapOft   偏移地址
**           piPendingPos   标记位置
**                          0  掩码标记无效
**                          1  掩码标记有效
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciMsiPendingPosGet (INT  iBus, INT  iSlot, INT  iFunc, UINT32  uiMsixCapOft, INT *piPendingPos)
{
    INT         iRet        = PX_ERROR;
    UINT16      usControl   = 0;
    INT         iEnable     = 0;
    INT         iIs64       = 0;
    INT         iIsMask     = 0;

    if (uiMsixCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, uiMsixCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    iEnable = !!(usControl & PCI_MSI_FLAGS_ENABLE);
    if (!iEnable) {
        return  (PX_ERROR);
    }
    iIsMask = !!(usControl & PCI_MSI_FLAGS_MASK_BIT);
    if (!iIsMask) {
        return  (PX_ERROR);
    }
    if (piPendingPos) {
        iIs64 = !!(usControl & PCI_MSI_FLAGS_64BIT);
        if (iIs64) {
            *piPendingPos = uiMsixCapOft + PCI_MSI_PENDING_64;
        } else {
            *piPendingPos = uiMsixCapOft + PCI_MSI_PENDING_32;
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciMsiMaskSet
** 功能描述: MSI 掩码设置
** 输　入  : iBus           总线号
**           iSlot          插槽
**           iFunc          功能
**           uiMsixCapOft   偏移地址
**           uiMask         掩码
**           uiFlag         新标志
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciMsiMaskSet (INT     iBus,
                        INT     iSlot,
                        INT     iFunc,
                        UINT32  uiMsixCapOft,
                        UINT32  uiMask,
                        UINT32  uiFlag)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;
    INT         iEnable   = 0;
    INT         iIs64     = 0;
    INT         iIsMask   = 0;
    INT         iMaskPos  = 0;
    UINT32      uiMasked  = 0;

    if (uiMsixCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, uiMsixCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    iEnable = !!(usControl & PCI_MSI_FLAGS_ENABLE);
    if (!iEnable) {
        return  (PX_ERROR);
    }
    iIsMask = !!(usControl & PCI_MSI_FLAGS_MASK_BIT);
    if (!iIsMask) {
        return  (PX_ERROR);
    }
    iIs64 = !!(usControl & PCI_MSI_FLAGS_64BIT);
    if (iIs64) {
        iMaskPos = uiMsixCapOft + PCI_MSI_MASK_BIT_64;
    } else {
        iMaskPos = uiMsixCapOft + PCI_MSI_MASK_BIT_32;
    }
    uiMasked &= ~uiMask;
    uiMasked |= uiFlag;
    iRet = API_PciConfigOutDword(iBus, iSlot, iFunc, iMaskPos, uiMasked);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciMsiMaskGet
** 功能描述: 获取 MSI 掩码
** 输　入  : iBus           总线号
**           iSlot          插槽
**           iFunc          功能
**           uiMsixCapOft   偏移地址
**           puiMask        掩码
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciMsiMaskGet (INT  iBus, INT  iSlot, INT  iFunc, UINT32  uiMsixCapOft, UINT32 *puiMask)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;
    INT         iEnable   = 0;
    INT         iIs64     = 0;
    INT         iIsMask   = 0;
    INT         iMaskPos  = 0;
    UINT32      uiMasked  = 0;

    if (uiMsixCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, uiMsixCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    iEnable = !!(usControl & PCI_MSI_FLAGS_ENABLE);
    if (!iEnable) {
        return  (PX_ERROR);
    }
    iIsMask = !!(usControl & PCI_MSI_FLAGS_MASK_BIT);
    if (!iIsMask) {
        return  (PX_ERROR);
    }
    iIs64 = !!(usControl & PCI_MSI_FLAGS_64BIT);
    if (iIs64) {
        iMaskPos = uiMsixCapOft + PCI_MSI_MASK_BIT_64;
    } else {
        iMaskPos = uiMsixCapOft + PCI_MSI_MASK_BIT_32;
    }
    iRet = API_PciConfigInDword(iBus, iSlot, iFunc, iMaskPos, &uiMasked);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    if (puiMask) {
        *puiMask = uiMasked;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciMsiMaskPosGet
** 功能描述: 获取 MSI 掩码位置
** 输　入  : iBus           总线号
**           iSlot          插槽
**           iFunc          功能
**           uiMsixCapOft   偏移地址
**           piMaskPos      标记位置
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciMsiMaskPosGet (INT  iBus, INT  iSlot, INT  iFunc, UINT32  uiMsixCapOft, INT *piMaskPos)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;
    INT         iEnable   = 0;
    INT         iIs64     = 0;
    INT         iIsMask   = 0;

    if (uiMsixCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, uiMsixCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    iEnable = !!(usControl & PCI_MSI_FLAGS_ENABLE);
    if (!iEnable) {
        return  (PX_ERROR);
    }
    iIsMask = !!(usControl & PCI_MSI_FLAGS_MASK_BIT);
    if (!iIsMask) {
        return  (PX_ERROR);
    }
    if (piMaskPos) {
        iIs64 = !!(usControl & PCI_MSI_FLAGS_64BIT);
        if (iIs64) {
            *piMaskPos = uiMsixCapOft + PCI_MSI_MASK_BIT_64;
        } else {
            *piMaskPos = uiMsixCapOft + PCI_MSI_MASK_BIT_32;
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciMsiMaskConvert
** 功能描述: MSI 掩码转换
** 输　入  : uiMask     掩码
** 输　出  : 转换后的掩码
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
UINT32  API_PciMsiMaskConvert (UINT32  uiMask)
{
    if (uiMask >= 5) {
        return  (0xffffffff);
    }

    return  ((1 << (1 << uiMask)) - 1);
}
/*********************************************************************************************************
** 函数名称: API_PciMsiMultipleGet
** 功能描述: 获取 MSI 消息数量
** 输　入  : iBus           总线号
**           iSlot          插槽
**           iFunc          功能
**           uiMsixCapOft   偏移地址
**           iNvec          中断数量
**           piMultiple     消息消息数量
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciMsiMultipleGet (INT     iBus,
                            INT     iSlot,
                            INT     iFunc,
                            UINT32  uiMsiCapOft,
                            INT     iNvec,
                            INT    *piMultiple)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;

    if (uiMsiCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, uiMsiCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    if (piMultiple) {
        *piMultiple = ilog2(__roundup_pow_of_two(iNvec));
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciMsiVecCountGet
** 功能描述: 获取 MSI 向量数
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
INT  API_PciMsiVecCountGet (INT  iBus, INT  iSlot, INT  iFunc, UINT32  uiMsiCapOft, UINT32 *puiVecCount)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;

    if (uiMsiCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, uiMsiCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    if (puiVecCount) {
        *puiVecCount = 1 << ((usControl & PCI_MSI_FLAGS_QMASK) >> 1);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciMsiMultiCapGet
** 功能描述: 获取 MSI 消息数量
** 输　入  : iBus           总线号
**           iSlot          插槽
**           iFunc          功能
**           uiMsiCapOft    偏移地址
**           piMultiCap     消息消息数量
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciMsiMultiCapGet (INT  iBus, INT  iSlot, INT  iFunc, UINT32  uiMsiCapOft, INT *piMultiCap)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;

    if (uiMsiCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, uiMsiCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    if (piMultiCap) {
        *piMultiCap = (usControl & PCI_MSI_FLAGS_QMASK) >> 1;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciMsi64BitGet
** 功能描述: 获取 MSI 64 位地址
** 输　入  : iBus           总线号
**           iSlot          插槽
**           iFunc          功能
**           uiMsiCapOft    偏移地址
**           pi64Bit        64 位地址标志
**                          0  64 位地址标记无效
**                          1  64 位地址标记有效
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciMsi64BitGet (INT  iBus, INT  iSlot, INT  iFunc, UINT32  uiMsiCapOft, INT *pi64Bit)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;

    if (uiMsiCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, uiMsiCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    if (pi64Bit) {
        *pi64Bit = !!(usControl & PCI_MSI_FLAGS_64BIT);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciMsiMaskBitGet
** 功能描述: 获取 MSI 掩码位
** 输　入  : iBus           总线号
**           iSlot          插槽
**           iFunc          功能
**           uiMsiCapOft    偏移地址
**           piMaskBit      掩码标志
**                          0  掩码标记无效
**                          1  掩码标记有效
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciMsiMaskBitGet (INT  iBus, INT  iSlot, INT  iFunc, UINT32  uiMsiCapOft, INT *piMaskBit)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;

    if (uiMsiCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, uiMsiCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    if (piMaskBit) {
        *piMaskBit = !!(usControl & PCI_MSI_FLAGS_MASK_BIT);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciMsiEnableSet
** 功能描述: MSI 使能控制
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
INT  API_PciMsiEnableSet (INT  iBus, INT  iSlot, INT  iFunc, UINT32  uiMsiCapOft, INT  iEnable)
{
    INT         iRet         = PX_ERROR;
    UINT16      usControl    = 0;
    UINT16      usControlNew = 0;

    if (uiMsiCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, uiMsiCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    usControlNew = usControl & (~(PCI_MSI_FLAGS_ENABLE));
    if (iEnable) {
        usControlNew |= PCI_MSI_FLAGS_ENABLE;
    }
    iRet = API_PciConfigOutWord(iBus, iSlot, iFunc, uiMsiCapOft + PCI_MSI_FLAGS, usControlNew);

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_PciMsiEnableGet
** 功能描述: 获取 MSI 使能控制状态
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
INT  API_PciMsiEnableGet (INT  iBus, INT  iSlot, INT  iFunc, UINT32  uiMsiCapOft, INT *piEnable)
{
    INT         iRet         = PX_ERROR;
    UINT16      usControl    = 0;

    if (uiMsiCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, uiMsiCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    if (piEnable) {
        *piEnable = !!(usControl & PCI_MSI_FLAGS_ENABLE);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciDevMsiRangeEnable
** 功能描述: 设置 MSI 区域使能
** 输　入  : hHandle        设备控制句柄
**           uiVecMin       使能区域中断向量最小值 (程序内部会自动修正为 1, 2, 4, 8, 16, 32)
**           uiVecMax       使能区域中断向量最大值 (程序内部会自动修正为 1, 2, 4, 8, 16, 32)
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
** 注  意  : 每个 PCI 设备驱动程序只需要调用一次
**
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciDevMsiRangeEnable (PCI_DEV_HANDLE  hHandle, UINT  uiVecMin, UINT  uiVecMax)
{
    INT                     i, j, iRet  = PX_ERROR;
    UINT8                   ucMsiEn     = 0;
    UINT32                  uiMsiCapOft = 0;
    UINT32                  uiVecNum    = 0;
    PCI_MSI_DESC_HANDLE     hMsgHandle  = LW_NULL;

    if (hHandle == LW_NULL) {
        return  (PX_ERROR);
    }
    
    if ((uiVecMin > 32) || (uiVecMax > 32) ||
        (uiVecMax < uiVecMin) || (uiVecMin < 1)) {
        return  (PX_ERROR);
    }

    iRet = API_PciCapFind(hHandle->PCIDEV_iDevBus,
                          hHandle->PCIDEV_iDevDevice,
                          hHandle->PCIDEV_iDevFunction,
                          PCI_CAP_ID_MSI,
                          &uiMsiCapOft);
    if ((iRet != ERROR_NONE) ||
        (!PCI_DEV_MSI_IS_EN(hHandle))) {
        return  (PX_ERROR);
    }
    
    for (i = 0; i < 6; i++) {
        j = 1 << i;
        if (j >= uiVecMin) {
            break;
        }
    }
    uiVecMin = j;
    
    for (i = 0; i < 6; i++) {
        j = 1 << i;
        if (j >= uiVecMax) {
            break;
        }
    }
    uiVecMax = j;

    iRet = API_PciMsiVecCountGet(hHandle->PCIDEV_iDevBus,
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

    hMsgHandle = &hHandle->PCIDEV_pmdDevIrqMsiDesc;
    hMsgHandle->PCIMSI_uiNum = uiVecNum;

__reget:
    iRet = API_PciDevInterMsiGet(hHandle, hMsgHandle);
    if (iRet != ERROR_NONE) {
        hMsgHandle->PCIMSI_uiNum >>= 1;
        if (hMsgHandle->PCIMSI_uiNum < uiVecMin) {
            return  (PX_ERROR);
        }
        goto    __reget;
    }

    hHandle->PCIDEV_uiDevIrqMsiNum = hMsgHandle->PCIMSI_uiNum;
    hHandle->PCIDEV_ulDevIrqVector = hMsgHandle->PCIMSI_ulDevIrqVector;

    /*
     *  MSI can support only 1, 2, 4, 8, 16, 32 number of vectors
     */
    switch (hHandle->PCIDEV_uiDevIrqMsiNum) {
    
    case 1:
        ucMsiEn = 0;
        break;

    case 2:
        ucMsiEn = 1;
        break;

    case 4:
        ucMsiEn = 2;
        break;

    case 8:
        ucMsiEn = 3;
        break;

    case 16:
        ucMsiEn = 4;
        break;

    case 32:
        ucMsiEn = 5;
        break;

    default:
        return  (PX_ERROR);
    }

    iRet = API_PciMsiMsgWrite(hHandle->PCIDEV_iDevBus,
                              hHandle->PCIDEV_iDevDevice,
                              hHandle->PCIDEV_iDevFunction,
                              uiMsiCapOft,
                              ucMsiEn,
                              &hHandle->PCIDEV_pmdDevIrqMsiDesc.PCIMSI_pmmMsg);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciDevMsiVecCountGet
** 功能描述: 获取 MSI 向量数
** 输　入  : hHandle        设备控制句柄
**           puiVecCount    向量数
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciDevMsiVecCountGet (PCI_DEV_HANDLE  hHandle, UINT32 *puiVecCount)
{
    INT         iRet        = PX_ERROR;
    UINT32      uiMsiCapOft = 0;

    if (hHandle == LW_NULL) {
        return  (PX_ERROR);
    }

    iRet = API_PciCapFind(hHandle->PCIDEV_iDevBus,
                          hHandle->PCIDEV_iDevDevice,
                          hHandle->PCIDEV_iDevFunction,
                          PCI_CAP_ID_MSI,
                          &uiMsiCapOft);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    iRet = API_PciMsiVecCountGet(hHandle->PCIDEV_iDevBus,
                                 hHandle->PCIDEV_iDevDevice,
                                 hHandle->PCIDEV_iDevFunction,
                                 uiMsiCapOft, puiVecCount);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciDevMsiEnableGet
** 功能描述: 获取 MSI 使能控制状态
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
INT  API_PciDevMsiEnableGet (PCI_DEV_HANDLE  hHandle, INT *piEnable)
{
    INT         iRet        = PX_ERROR;
    UINT32      uiMsiCapOft = 0;

    if (hHandle == LW_NULL) {
        return  (PX_ERROR);
    }

    iRet = API_PciCapFind(hHandle->PCIDEV_iDevBus,
                          hHandle->PCIDEV_iDevDevice,
                          hHandle->PCIDEV_iDevFunction,
                          PCI_CAP_ID_MSI,
                          &uiMsiCapOft);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    iRet = API_PciMsiEnableGet(hHandle->PCIDEV_iDevBus,
                               hHandle->PCIDEV_iDevDevice,
                               hHandle->PCIDEV_iDevFunction,
                               uiMsiCapOft,
                               piEnable);
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_PciDevMsiEnableSet
** 功能描述: 设置 MSI 使能控制状态
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
INT  API_PciDevMsiEnableSet (PCI_DEV_HANDLE  hHandle, INT  iEnable)
{
    INT         iRet        = PX_ERROR;
    UINT32      uiMsiCapOft = 0;

    if (hHandle == LW_NULL) {
        return  (PX_ERROR);
    }

    if (!iEnable) {
        iRet = API_PciIntxEnableSet(hHandle->PCIDEV_iDevBus,
                                    hHandle->PCIDEV_iDevDevice,
                                    hHandle->PCIDEV_iDevFunction, 1);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
    } else {
        iRet = API_PciIntxEnableSet(hHandle->PCIDEV_iDevBus,
                                    hHandle->PCIDEV_iDevDevice,
                                    hHandle->PCIDEV_iDevFunction, 0);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
    }

    iRet = API_PciCapFind(hHandle->PCIDEV_iDevBus,
                          hHandle->PCIDEV_iDevDevice,
                          hHandle->PCIDEV_iDevFunction,
                          PCI_CAP_ID_MSI,
                          &uiMsiCapOft);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    iRet = API_PciMsiEnableSet(hHandle->PCIDEV_iDevBus,
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
