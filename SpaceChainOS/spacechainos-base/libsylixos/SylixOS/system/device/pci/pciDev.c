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
** 文   件   名: pciDev.c
**
** 创   建   人: Gong.YuJian (弓羽箭)
**
** 文件创建日期: 2015 年 12 月 23 日
**
** 描        述: PCI 总线设备管理.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/shell/include/ttiny_shell.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PCI_EN > 0)
#include "endian.h"
#include "pciDev.h"
#include "pciDrv.h"
#include "pciLib.h"
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static UINT                     _GuiPciDevTotalNum  = 0;
static LW_OBJECT_HANDLE         _GulPciDevLock      = LW_OBJECT_HANDLE_INVALID;
static LW_LIST_LINE_HEADER      _GplinePciDevHeader = LW_NULL;
/*********************************************************************************************************
  PCI 设备锁
*********************************************************************************************************/
#define __PCI_DEV_LOCK()        API_SemaphoreMPend(_GulPciDevLock, LW_OPTION_WAIT_INFINITE)
#define __PCI_DEV_UNLOCK()      API_SemaphoreMPost(_GulPciDevLock)
/*********************************************************************************************************
** 函数名称: API_PciDevIntxEnableSet
** 功能描述: 设备 INTx 使能与禁能
** 输　入  : hHandle    设备句柄
**           iEnable    使能标志
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciDevIntxEnableSet (PCI_DEV_HANDLE  hHandle, INT  iEnable)
{
    INT     iRet = PX_ERROR;

    if (hHandle == LW_NULL) {
        return  (PX_ERROR);
    }

    iRet = API_PciIntxEnableSet(hHandle->PCIDEV_iDevBus,
                                hHandle->PCIDEV_iDevDevice,
                                hHandle->PCIDEV_iDevFunction,
                                iEnable);
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_PciDevInterDisable
** 功能描述: 禁能 PCI 设备中断
** 输　入  : hHandle    设备句柄
**           ulVector   中断向量
**           pfuncIsr   中断服务函数
**           pvArg      中断服务函数参数
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciDevInterDisable (PCI_DEV_HANDLE   hHandle,
                             ULONG            ulVector,
                             PINT_SVR_ROUTINE pfuncIsr,
                             PVOID            pvArg)
{
    if (hHandle == LW_NULL) {
        return  (PX_ERROR);
    }

    if (API_InterVectorDisable(ulVector)) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciDevInterEnable
** 功能描述: 使能 PCI 设备中断
** 输　入  : hHandle    设备句柄
**           ulVector   中断向量
**           pfuncIsr   中断服务函数
**           pvArg      中断服务函数参数
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciDevInterEnable (PCI_DEV_HANDLE   hHandle,
                            ULONG            ulVector,
                            PINT_SVR_ROUTINE pfuncIsr,
                            PVOID            pvArg)
{
    if (hHandle == LW_NULL) {
        return  (PX_ERROR);
    }

    if (API_InterVectorEnable(ulVector)) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciDevInterDisonnect
** 功能描述: 设置 PCI 设备解除中断连接
** 输　入  : hHandle    设备句柄
**           ulVector   中断向量
**           pfuncIsr   中断服务函数
**           pvArg      中断服务函数参数
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciDevInterDisonnect (PCI_DEV_HANDLE    hHandle,
                               ULONG             ulVector,
                               PINT_SVR_ROUTINE  pfuncIsr,
                               PVOID             pvArg)
{
    INT     iRet = PX_ERROR;

    if (hHandle == LW_NULL) {
        return  (PX_ERROR);
    }

    iRet = API_PciInterDisconnect(ulVector, pfuncIsr, pvArg);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    lib_bzero(hHandle->PCIDEV_cDevIrqName, PCI_DEV_IRQ_NAME_MAX);
    hHandle->PCIDEV_pfuncDevIrqHandle = LW_NULL;
    hHandle->PCIDEV_pvDevIrqArg = LW_NULL;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciDevInterConnect
** 功能描述: 设置 PCI 设备中断连接
** 输　入  : hHandle    设备句柄
**           ulVector   中断向量
**           pfuncIsr   中断服务函数
**           pvArg      中断服务函数参数
**           pcName     中断服务名称
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciDevInterConnect (PCI_DEV_HANDLE    hHandle,
                             ULONG             ulVector,
                             PINT_SVR_ROUTINE  pfuncIsr,
                             PVOID             pvArg,
                             CPCHAR            pcName)
{
    INT     iRet = PX_ERROR;

    if ((hHandle  == LW_NULL) ||
        (pfuncIsr == LW_NULL)) {
        return  (PX_ERROR);
    }

    iRet = API_PciInterConnect(ulVector, pfuncIsr, pvArg, pcName);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    lib_strlcpy(hHandle->PCIDEV_cDevIrqName, pcName, PCI_DEV_IRQ_NAME_MAX);
    hHandle->PCIDEV_pfuncDevIrqHandle = pfuncIsr;
    hHandle->PCIDEV_pvDevIrqArg       = pvArg;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciDevInterVectorGet
** 功能描述: 获取 INTx 中断向量
** 输　入  : hHandle        设备句柄
**           pulVector      中断向量
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciDevInterVectorGet (PCI_DEV_HANDLE  hHandle, ULONG *pulVector)
{
    INT     iRet     = PX_ERROR;
    INT     iLine    = 0;
    INT     iPin     = 0;
    INT     iHdrType = 0;

    if (hHandle == LW_NULL) {
        return  (PX_ERROR);
    }

    iHdrType = hHandle->PCIDEV_phDevHdr.PCIH_ucType & PCI_HEADER_TYPE_MASK;
    switch (iHdrType) {

    case PCI_HEADER_TYPE_NORMAL:
        iLine = hHandle->PCIDEV_phDevHdr.PCIH_pcidHdr.PCID_ucIntLine;
        iPin  = hHandle->PCIDEV_phDevHdr.PCIH_pcidHdr.PCID_ucIntPin;;
        break;

    case PCI_HEADER_TYPE_BRIDGE:
    case PCI_HEADER_TYPE_CARDBUS:
        return  (PX_ERROR);

    default:
        return  (PX_ERROR);
    }

    iRet = API_PciIrqGet(hHandle->PCIDEV_iDevBus,
                         hHandle->PCIDEV_iDevDevice,
                         hHandle->PCIDEV_iDevFunction,
                         hHandle->PCIDEV_iDevIrqMsiEn, iLine, iPin, pulVector);
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_PciDevInterMsiGet
** 功能描述: 获取 MSI MSI-X 中断向量
** 输　入  : hHandle        设备句柄
**           pmsidesc       MSI 中断信息描述
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciDevInterMsiGet (PCI_DEV_HANDLE  hHandle, PCI_MSI_DESC *pmsidesc)
{
    INT     iRet     = PX_ERROR;
    INT     iLine    = 0;
    INT     iPin     = 0;
    INT     iHdrType = 0;

    if (hHandle == LW_NULL) {
        return  (PX_ERROR);
    }

    iHdrType = hHandle->PCIDEV_phDevHdr.PCIH_ucType & PCI_HEADER_TYPE_MASK;
    switch (iHdrType) {

    case PCI_HEADER_TYPE_NORMAL:
        iLine = hHandle->PCIDEV_phDevHdr.PCIH_pcidHdr.PCID_ucIntLine;
        iPin  = hHandle->PCIDEV_phDevHdr.PCIH_pcidHdr.PCID_ucIntPin;;
        break;

    case PCI_HEADER_TYPE_BRIDGE:
    case PCI_HEADER_TYPE_CARDBUS:
        return  (PX_ERROR);

    default:
        return  (PX_ERROR);
    }

    iRet = API_PciIrqMsi(hHandle->PCIDEV_iDevBus,
                         hHandle->PCIDEV_iDevDevice,
                         hHandle->PCIDEV_iDevFunction,
                         hHandle->PCIDEV_iDevIrqMsiEn, iLine, iPin, pmsidesc);
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __pciDevConfigBlockOp
** 功能描述: 按块操作 PCI 设备配置空间
** 输　入  : hHandle    设备控制句柄
**           uiPos      偏移位置
**           pucBuf     数据缓冲区
**           uiLen      数据长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __pciDevConfigBlockOp (PCI_DEV_HANDLE  hHandle, INT  uiPos, UINT8 *pucBuf, INT  uiLen,
                                   INT (*pfuncOpt)(PCI_DEV_HANDLE hHandle, UINT uiPos, UINT8 *pucBuf, UINT uiLen))
{
    INT     iRet = PX_ERROR;

    if ((uiPos & 1) && uiLen >= 1) {
        iRet = pfuncOpt(hHandle, uiPos, pucBuf, 1);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }

        uiPos++; pucBuf++; uiLen--;
    }

    if ((uiPos & 3) && uiLen >= 2) {
        iRet = pfuncOpt(hHandle, uiPos, pucBuf, 2);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }

        uiPos += 2; pucBuf += 2; uiLen -= 2;
    }

    while (uiLen >= 4) {
        iRet = pfuncOpt(hHandle, uiPos, pucBuf, 4);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }

        uiPos += 4; pucBuf += 4; uiLen -= 4;
    }

    if (uiLen >= 2) {
        iRet = pfuncOpt(hHandle, uiPos, pucBuf, 2);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }

        uiPos += 2; pucBuf += 2; uiLen -= 2;
    }

    iRet = pfuncOpt(hHandle, uiPos, pucBuf, 1);
    if ((uiLen) &&
        (iRet != ERROR_NONE)) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __pciDevConfigBlockRead
** 功能描述: 按块读取 PCI 设备配置空间
** 输　入  : hHandle    设备控制句柄
**           uiPos      偏移位置
**           pucBuf     数据缓冲区
**           uiLen      数据长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __pciDevConfigBlockRead (PCI_DEV_HANDLE  hHandle, UINT  uiPos, UINT8 *pucBuf, UINT  uiLen)
{
    return  (__pciDevConfigBlockOp(hHandle, uiPos, pucBuf, uiLen, API_PciDevConfigRead));
}
/*********************************************************************************************************
** 函数名称: __pciDevConfigBlockWrite
** 功能描述: 按块写入 PCI 设备配置空间
** 输　入  : hHandle    设备控制句柄
**           uiPos     偏移位置
**           pucBuf     数据缓冲区
**           uiLen      数据长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __pciDevConfigBlockWrite (PCI_DEV_HANDLE  hHandle, UINT  uiPos, UINT8 *pucBuf, UINT  uiLen)
{
    return  (__pciDevConfigBlockOp(hHandle, uiPos, pucBuf, uiLen, API_PciDevConfigWrite));
}
/*********************************************************************************************************
** 函数名称: API_PciDevConfigRead
** 功能描述: 读取 PCI 设备配置空间
** 输　入  : hHandle    设备控制句柄
**           uiPos      偏移位置
**           pucBuf     数据缓冲区
**           uiLen      数据长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_PciDevConfigRead (PCI_DEV_HANDLE  hHandle, UINT  uiPos, UINT8 *pucBuf, UINT  uiLen)
{
    INT         iRet   = PX_ERROR;
    UINT8       ucData = (UINT8)PX_ERROR;
    UINT16      usData = (UINT16)PX_ERROR;
    UINT32      uiData = (UINT32)PX_ERROR;

    if (!((uiLen == 1) || (uiLen == 2) || (uiLen == 4))) {
        return  (__pciDevConfigBlockRead(hHandle, uiPos, pucBuf, uiLen));
    }

    if ((hHandle == LW_NULL) ||
        (pucBuf == LW_NULL) ||
        (uiPos >= PCI_CONFIG_LEN_MAX) ||
        ((uiPos + uiLen) > PCI_CONFIG_LEN_MAX)) {
        return  (PX_ERROR);
    }

    switch (uiLen) {

    case 1:
        iRet = API_PciConfigInByte(hHandle->PCIDEV_iDevBus,
                                   hHandle->PCIDEV_iDevDevice,
                                   hHandle->PCIDEV_iDevFunction,
                                   uiPos, &ucData);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
        pucBuf[0] = (UINT8)ucData;
        break;

    case 2:
        iRet = API_PciConfigInWord(hHandle->PCIDEV_iDevBus,
                                   hHandle->PCIDEV_iDevDevice,
                                   hHandle->PCIDEV_iDevFunction,
                                   uiPos, &usData);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
        ((UINT16 *)pucBuf)[0] = usData;
        break;

    case 4:
        iRet = API_PciConfigInDword(hHandle->PCIDEV_iDevBus,
                                    hHandle->PCIDEV_iDevDevice,
                                    hHandle->PCIDEV_iDevFunction,
                                    uiPos, &uiData);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
        ((UINT32 *)pucBuf)[0] = uiData;
        break;

    default:
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciDevConfigWrite
** 功能描述: 写入 PCI 设备配置空间
** 输　入  : hHandle    设备控制句柄
**           uiPos      偏移位置
**           pucBuf     数据缓冲区
**           uiLen       数据长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_PciDevConfigWrite (PCI_DEV_HANDLE  hHandle, UINT  uiPos, UINT8 *pucBuf, UINT  uiLen)
{
    INT         iRet   = PX_ERROR;
    UINT8       ucData = (UINT8)PX_ERROR;
    UINT16      usData = (UINT16)PX_ERROR;
    UINT32      uiData = (UINT32)PX_ERROR;

    if (!((uiLen == 1) || (uiLen == 2) || (uiLen == 4))) {
        return  (__pciDevConfigBlockWrite(hHandle, uiPos, pucBuf, uiLen));
    }

    if ((hHandle == LW_NULL) ||
        (pucBuf == LW_NULL) ||
        (uiPos >= PCI_CONFIG_LEN_MAX) ||
        ((uiPos + uiLen) > PCI_CONFIG_LEN_MAX)) {
        return  (PX_ERROR);
    }

    switch (uiLen) {

    case 1:
        ucData = (UINT8)pucBuf[0];
        iRet = API_PciConfigOutByte(hHandle->PCIDEV_iDevBus,
                                    hHandle->PCIDEV_iDevDevice,
                                    hHandle->PCIDEV_iDevFunction,
                                    uiPos, ucData);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
        break;

    case 2:
        usData = ((UINT16 *)pucBuf)[0];
        iRet = API_PciConfigOutWord(hHandle->PCIDEV_iDevBus,
                                    hHandle->PCIDEV_iDevDevice,
                                    hHandle->PCIDEV_iDevFunction,
                                    uiPos, usData);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
        break;

    case 4:
        uiData = ((UINT32 *)pucBuf)[0];
        iRet = API_PciConfigOutDword(hHandle->PCIDEV_iDevBus,
                                     hHandle->PCIDEV_iDevDevice,
                                     hHandle->PCIDEV_iDevFunction,
                                     uiPos, uiData);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
        break;

    default:
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciDevConfigReadByte
** 功能描述: 按字节读取 PCI 设备配置空间 (8 bit)
** 输　入  : hHandle    设备控制句柄
**           uiPos      偏移位置
**           pucValue   获取的结果
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_PciDevConfigReadByte (PCI_DEV_HANDLE  hHandle, UINT  uiPos, UINT8 *pucValue)
{
    INT     iRet;

    if ((hHandle == LW_NULL        ) ||
        (uiPos >= PCI_CONFIG_LEN_MAX)) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInByte(hHandle->PCIDEV_iDevBus,
                               hHandle->PCIDEV_iDevDevice,
                               hHandle->PCIDEV_iDevFunction,
                               uiPos, pucValue);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciDevConfigReadWord
** 功能描述: 按字读取 PCI 设备配置空间 (16 bit)
** 输　入  : hHandle    设备控制句柄
**           uiPos      偏移位置
**           pusValue   获取的结果
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciDevConfigReadWord (PCI_DEV_HANDLE  hHandle, UINT  uiPos, UINT16 *pusValue)
{
    INT     iRet;

    if ((hHandle == LW_NULL        ) ||
        (uiPos >= PCI_CONFIG_LEN_MAX)) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(hHandle->PCIDEV_iDevBus,
                               hHandle->PCIDEV_iDevDevice,
                               hHandle->PCIDEV_iDevFunction,
                               uiPos, pusValue);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciDevConfigReadDword
** 功能描述: 按双字读取 PCI 设备配置空间 (32 bit)
** 输　入  : hHandle    设备控制句柄
**           uiPos      偏移位置
**           puiValue   获取的结果
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciDevConfigReadDword (PCI_DEV_HANDLE  hHandle, UINT  uiPos, UINT32 *puiValue)
{
    INT     iRet;

    if ((hHandle == LW_NULL        ) ||
        (uiPos >= PCI_CONFIG_LEN_MAX)) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInDword(hHandle->PCIDEV_iDevBus,
                                hHandle->PCIDEV_iDevDevice,
                                hHandle->PCIDEV_iDevFunction,
                                uiPos, puiValue);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciDevConfigWriteByte
** 功能描述: 按字节写 PCI 设备配置空间 (8 bit)
** 输　入  : hHandle    设备控制句柄
**           uiPos      偏移位置
**           ucValue    设置的内容
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_PciDevConfigWriteByte (PCI_DEV_HANDLE  hHandle, UINT  uiPos, UINT8 ucValue)
{
    INT     iRet;

    if ((hHandle == LW_NULL        ) ||
        (uiPos >= PCI_CONFIG_LEN_MAX)) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigOutByte(hHandle->PCIDEV_iDevBus,
                                hHandle->PCIDEV_iDevDevice,
                                hHandle->PCIDEV_iDevFunction,
                                uiPos, ucValue);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciDevConfigWriteWord
** 功能描述: 按字写 PCI 设备配置空间 (16 bit)
** 输　入  : hHandle    设备控制句柄
**           uiPos      偏移位置
**           usValue    设置的内容
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciDevConfigWriteWord (PCI_DEV_HANDLE  hHandle, UINT  uiPos, UINT16  usValue)
{
    INT     iRet;

    if ((hHandle == LW_NULL        ) ||
        (uiPos >= PCI_CONFIG_LEN_MAX)) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigOutWord(hHandle->PCIDEV_iDevBus,
                                hHandle->PCIDEV_iDevDevice,
                                hHandle->PCIDEV_iDevFunction,
                                uiPos, usValue);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciDevConfigWriteDword
** 功能描述: 按双字写 PCI 设备配置空间 (32 bit)
** 输　入  : hHandle    设备控制句柄
**           uiPos      偏移位置
**           uiValue    设置的内容
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciDevConfigWriteDword (PCI_DEV_HANDLE  hHandle, UINT  uiPos, UINT32 uiValue)
{
    INT     iRet;

    if ((hHandle == LW_NULL        ) ||
        (uiPos >= PCI_CONFIG_LEN_MAX)) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigOutDword(hHandle->PCIDEV_iDevBus,
                                 hHandle->PCIDEV_iDevDevice,
                                 hHandle->PCIDEV_iDevFunction,
                                 uiPos, uiValue);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellPciCmdDevParent
** 功能描述: PCI 命令查找设备树父节点
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tshellPciCmdDevParent (INT  iArgC, PCHAR  ppcArgV[])
{
    INT                 iRet;
    PCI_DEV_HANDLE      hDevHandle = LW_NULL;
    INT                 iBus;
    INT                 iDevice;
    INT                 iFunction;

    if (iArgC == 2) {
        iRet = sscanf(ppcArgV[1], "%x:%x.%x", &iBus, &iDevice, &iFunction);
        if (iRet != 3) {
            fprintf(stderr, "pci device address format error.\n");
            goto  __error_handle;
        }

        hDevHandle = API_PciDevParentHandleGet(iBus, iDevice, iFunction);
        if (hDevHandle != LW_NULL) {
            printf("pci dev parent: %x:%x.%x\n",
                   hDevHandle->PCIDEV_iDevBus,
                   hDevHandle->PCIDEV_iDevDevice,
                   hDevHandle->PCIDEV_iDevFunction);

            return  (ERROR_NONE);
        }
    }

__error_handle:
    fprintf(stderr, "arguments error!\n");
    return  (-ERROR_TSHELL_EPARAM);
}
/*********************************************************************************************************
** 函数名称: __tshellPciDevAddDel
** 功能描述: 添加或删除设备
** 输　入  : iAdd           是否为添加操作
**           iAll           是否为操作全部设备
**           iBus           总线号
**           iDevice        设备号
**           iFunction      功能号
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tshellPciDevAddDel (INT  iAdd, INT  iAll, INT  iBus, INT  iDevice, INT  iFunction)
{
    PCI_DEV_HANDLE      hDevHandle = LW_NULL;
    INT                 iRet = PX_ERROR;

    if ((iAdd == LW_TRUE) && (iAll == LW_TRUE)) {
        API_PciDevListCreate();
    
    } else if ((iAdd == LW_TRUE) && (iAll == LW_FALSE)) {
        hDevHandle = API_PciDevAdd(iBus, iDevice, iFunction);
        if (hDevHandle == LW_NULL) {
            goto  __error_handle;
        }
    }

    if ((iAdd == LW_FALSE) && (iAll == LW_TRUE)) {
        API_PciDevDelete(LW_NULL);
    
    } else if ((iAdd == LW_FALSE) && (iAll == LW_FALSE)) {
        hDevHandle = API_PciDevHandleGet(iBus, iDevice, iFunction);
        if (hDevHandle == LW_NULL) {
            goto  __error_handle;
        }
        iRet = API_PciDevDelete(hDevHandle);
        if (iRet != ERROR_NONE) {
            goto  __error_handle;
        }
    }

    return  (ERROR_NONE);

__error_handle:
    fprintf(stderr, "probe pci device error.\n");
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __tshellPciCmdDevShow
** 功能描述: 打印 PCI 普通设备列表
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __tshellPciCmdDevShow (VOID)
{
    static PCHAR        pcPciDevShowHdr = \
    " INDEX    TYPE    BUS   DEV  FUNC VENDOR DEVICE SUBV(PRI) SUBD(SEC) (SUB) LINE PIN  "
    "        DRVNAME\n"
    "------- -------- ----- ----- ---- ------ ------ --------- --------- ----- ---- ---- "
    "------------------------\n";

    PLW_LIST_LINE       plineTemp  = LW_NULL;
    PCI_DEV_HANDLE      hDevHandle = LW_NULL;
    PCI_DRV_HANDLE      hDrvHandle = LW_NULL;
    REGISTER INT        i;

    printf("pci dev number total: %d\n", _GuiPciDevTotalNum);
    printf(pcPciDevShowHdr);

    __PCI_DEV_LOCK();                                                   /*  锁定 PCI 驱动               */
    i = 0;
    for (plineTemp  = _GplinePciDevHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        hDevHandle = _LIST_ENTRY(plineTemp, PCI_DEV_CB, PCIDEV_lineDevNode);
        hDrvHandle = (PCI_DRV_HANDLE)hDevHandle->PCIDEV_pvDevDriver;

        switch (hDevHandle->PCIDEV_iType) {

        case PCI_HEADER_TYPE_NORMAL:
            printf("%7d %-8s 0x%03x 0x%03x  0x%1x 0x%04x 0x%04x    0x%04x    0x%04x %-5s 0x%02x 0x%02x %-24s\n",
                   i,
                   "NORMAL",
                   hDevHandle->PCIDEV_iDevBus,
                   hDevHandle->PCIDEV_iDevDevice,
                   hDevHandle->PCIDEV_iDevFunction,
                   hDevHandle->PCIDEV_phDevHdr.PCIH_pcidHdr.PCID_usVendorId,
                   hDevHandle->PCIDEV_phDevHdr.PCIH_pcidHdr.PCID_usDeviceId,
                   hDevHandle->PCIDEV_phDevHdr.PCIH_pcidHdr.PCID_usSubVendorId,
                   hDevHandle->PCIDEV_phDevHdr.PCIH_pcidHdr.PCID_usSubSystemId,
                   "",
                   hDevHandle->PCIDEV_ucLine,
                   hDevHandle->PCIDEV_ucPin,
                   ((hDrvHandle == LW_NULL) ? "*" : hDrvHandle->PCIDRV_cDrvName));
            break;

        case PCI_HEADER_TYPE_BRIDGE:
            printf("%7d %-8s 0x%03x 0x%03x  0x%1x 0x%04x 0x%04x    0x%04x    0x%04x 0x%03x 0x%02x 0x%02x %-24s\n",
                   i,
                   "BRIDGE",
                   hDevHandle->PCIDEV_iDevBus,
                   hDevHandle->PCIDEV_iDevDevice,
                   hDevHandle->PCIDEV_iDevFunction,
                   hDevHandle->PCIDEV_phDevHdr.PCIH_pcibHdr.PCIB_usVendorId,
                   hDevHandle->PCIDEV_phDevHdr.PCIH_pcibHdr.PCIB_usDeviceId,
                   hDevHandle->PCIDEV_phDevHdr.PCIH_pcibHdr.PCIB_ucPriBus,
                   hDevHandle->PCIDEV_phDevHdr.PCIH_pcibHdr.PCIB_ucSecBus,
                   hDevHandle->PCIDEV_phDevHdr.PCIH_pcibHdr.PCIB_ucSubBus,
                   hDevHandle->PCIDEV_ucLine,
                   hDevHandle->PCIDEV_ucPin,
                   ((hDrvHandle == LW_NULL) ? "*" : hDrvHandle->PCIDRV_cDrvName));
            break;

        case PCI_HEADER_TYPE_CARDBUS:
            printf("%7d %-8s 0x%03x 0x%03x  0x%1x 0x%04x 0x%04x    0x%04x    0x%04x 0x%03x 0x%02x 0x%02x %-24s\n",
                   i,
                   "CARDBUS",
                   hDevHandle->PCIDEV_iDevBus,
                   hDevHandle->PCIDEV_iDevDevice,
                   hDevHandle->PCIDEV_iDevFunction,
                   hDevHandle->PCIDEV_phDevHdr.PCIH_pcicbHdr.PCICB_usVendorId,
                   hDevHandle->PCIDEV_phDevHdr.PCIH_pcicbHdr.PCICB_usDeviceId,
                   hDevHandle->PCIDEV_phDevHdr.PCIH_pcicbHdr.PCICB_ucPriBus,
                   hDevHandle->PCIDEV_phDevHdr.PCIH_pcicbHdr.PCICB_ucSecBus,
                   hDevHandle->PCIDEV_phDevHdr.PCIH_pcicbHdr.PCICB_ucSubBus,
                   hDevHandle->PCIDEV_ucLine,
                   hDevHandle->PCIDEV_ucPin,
                   ((hDrvHandle == LW_NULL) ? "*" : hDrvHandle->PCIDRV_cDrvName));
            break;

        default:
            break;
        }

        i += 1;
    }
    __PCI_DEV_UNLOCK();                                                 /*  解锁 PCI 驱动               */
}
/*********************************************************************************************************
** 函数名称: __tshellPciCmdDev
** 功能描述: PCI 命令 "pcidev"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tshellPciCmdDev (INT  iArgC, PCHAR  ppcArgV[])
{
    INT     iRet;
    INT     iBus;
    INT     iDevice;
    INT     iFunction;
    INT     iAddFlag = LW_FALSE;
    INT     iAllFlag = LW_FALSE;

    if (iArgC == 1) {
        __tshellPciCmdDevShow();
        return  (ERROR_NONE);
    }

    if ((lib_strcmp(ppcArgV[1], "add") == 0) && (iArgC == 3)) {
        iAddFlag = LW_TRUE;

        if (lib_strcmp(ppcArgV[2], "all") == 0) {
            iAllFlag = LW_TRUE;
        } else {
            iAllFlag = LW_FALSE;
            iRet = sscanf(ppcArgV[2], "%d:%d.%d", &iBus, &iDevice, &iFunction);
            if (iRet != 3) {
                fprintf(stderr, "pci device address format error.\n");
                goto  __error_handle;
            }
        }

        iRet = __tshellPciDevAddDel(iAddFlag, iAllFlag, iBus, iDevice, iFunction);
        if (iRet != ERROR_NONE) {
            goto  __error_handle;
        }
    } else if ((lib_strcmp(ppcArgV[1], "del") == 0) && (iArgC == 3)) {
        iAddFlag = LW_FALSE;

        if (lib_strcmp(ppcArgV[2], "all") == 0) {
            iAllFlag = LW_TRUE;
        } else {
            iAllFlag = LW_FALSE;
            iRet = sscanf(ppcArgV[2], "%d:%d.%d", &iBus, &iDevice, &iFunction);
            if (iRet != 3) {
                fprintf(stderr, "pci device address format error.\n");
                goto  __error_handle;
            }
        }

        iRet = __tshellPciDevAddDel(iAddFlag, iAllFlag, iBus, iDevice, iFunction);
        if (iRet != ERROR_NONE) {
            goto  __error_handle;
        }
    } else {
        goto  __error_handle;
    }

    return  (ERROR_NONE);

__error_handle:
    fprintf(stderr, "arguments error!\n");
    return  (-ERROR_TSHELL_EPARAM);
}
/*********************************************************************************************************
** 函数名称: API_PciDevSetupAll
** 功能描述: 配置 PCI 总线上的所有设备
** 输　入  : hDevHandle     设备句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciDevSetupAll (VOID)
{
    PLW_LIST_LINE       plineTemp  = LW_NULL;
    PCI_DEV_HANDLE      hDevHandle = LW_NULL;

    hDevHandle = LW_NULL;
    __PCI_DEV_LOCK();                                                   /*  锁定 PCI 驱动               */
    for (plineTemp  = _GplinePciDevHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        hDevHandle = _LIST_ENTRY(plineTemp, PCI_DEV_CB, PCIDEV_lineDevNode);
        __PCI_DEV_UNLOCK();                                            /*  解锁 PCI 驱动               */

        API_PciDevSetup(hDevHandle);

        __PCI_DEV_LOCK();                                               /*  锁定 PCI 驱动               */
    }
    __PCI_DEV_UNLOCK();                                                 /*  解锁 PCI 驱动               */


    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciDevParentHandleGet
** 功能描述: 获取一个设备的父节点句柄
** 输　入  : iBus           总线号
**           iDevice        设备号
**           iFunction      功能号
** 输　出  : 设备控制句柄
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
PCI_DEV_HANDLE  API_PciDevParentHandleGet (INT  iBus, INT  iDevice, INT  iFunction)
{
    PLW_LIST_LINE       plineTemp      = LW_NULL;
    PCI_DEV_HANDLE      hDevHandleTemp = LW_NULL;
    PCI_DEV_HANDLE      hDevHandle     = LW_NULL;
    UINT8               ucSecBus       = 0;
    UINT8               ucSubBus       = 0;

    hDevHandle = API_PciDevHandleGet(iBus, iDevice, iFunction);
    if (hDevHandle == LW_NULL) {
        return  (LW_NULL);
    }

    hDevHandleTemp = LW_NULL;
    __PCI_DEV_LOCK();                                                   /*  锁定 PCI 驱动               */
    for (plineTemp  = _GplinePciDevHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        hDevHandleTemp = _LIST_ENTRY(plineTemp, PCI_DEV_CB, PCIDEV_lineDevNode);
        if (hDevHandleTemp->PCIDEV_iType == PCI_HEADER_TYPE_BRIDGE) {
            ucSecBus = hDevHandleTemp->PCIDEV_phDevHdr.PCIH_pcibHdr.PCIB_ucSecBus;
            ucSubBus = hDevHandleTemp->PCIDEV_phDevHdr.PCIH_pcibHdr.PCIB_ucSubBus;

            if ((hDevHandle->PCIDEV_iType   == PCI_HEADER_TYPE_NORMAL) &&
                (hDevHandle->PCIDEV_iDevBus == ucSecBus) &&
                (hDevHandle->PCIDEV_iDevBus == ucSubBus)) {
                break;
            }

            if ((hDevHandle->PCIDEV_iType   == PCI_HEADER_TYPE_BRIDGE) &&
                (hDevHandle->PCIDEV_iDevBus >= ucSecBus) &&
                (hDevHandle->PCIDEV_iDevBus <= ucSubBus)) {
                break;
            }
        }
    }
    __PCI_DEV_UNLOCK();                                                 /*  解锁 PCI 驱动               */

    if (plineTemp) {
        return  (hDevHandleTemp);
    }

    hDevHandleTemp = LW_NULL;
    __PCI_DEV_LOCK();                                                   /*  锁定 PCI 驱动               */
    for (plineTemp  = _GplinePciDevHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        hDevHandleTemp = _LIST_ENTRY(plineTemp, PCI_DEV_CB, PCIDEV_lineDevNode);
        if (hDevHandleTemp->PCIDEV_iType == PCI_HEADER_TYPE_BRIDGE) {
            ucSecBus = hDevHandleTemp->PCIDEV_phDevHdr.PCIH_pcibHdr.PCIB_ucSecBus;
            ucSubBus = hDevHandleTemp->PCIDEV_phDevHdr.PCIH_pcibHdr.PCIB_ucSubBus;

            if ((hDevHandle->PCIDEV_iType == PCI_HEADER_TYPE_NORMAL) &&
                (hDevHandle->PCIDEV_iDevBus >= ucSecBus) &&
                (hDevHandle->PCIDEV_iDevBus <= ucSubBus)) {
                break;
            }

            if ((hDevHandle->PCIDEV_iType == PCI_HEADER_TYPE_BRIDGE) &&
                (hDevHandle->PCIDEV_iDevBus == hDevHandleTemp->PCIDEV_iDevBus)) {
                hDevHandleTemp = hDevHandle;
                break;
            }
        }
    }
    __PCI_DEV_UNLOCK();                                                 /*  解锁 PCI 驱动               */

    if (plineTemp) {
        return  (hDevHandleTemp);
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: API_PciDevHandleGet
** 功能描述: 获取一个设备的句柄
** 输　入  : iBus           总线号
**           iDevice        设备号
**           iFunction      功能号
** 输　出  : 设备控制句柄
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
PCI_DEV_HANDLE  API_PciDevHandleGet (INT  iBus, INT  iDevice, INT  iFunction)
{
    PLW_LIST_LINE       plineTemp  = LW_NULL;
    PCI_DEV_HANDLE      hDevHandle = LW_NULL;

    hDevHandle = LW_NULL;
    __PCI_DEV_LOCK();                                                   /*  锁定 PCI 驱动               */
    for (plineTemp  = _GplinePciDevHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        hDevHandle = _LIST_ENTRY(plineTemp, PCI_DEV_CB, PCIDEV_lineDevNode);
        if ((hDevHandle->PCIDEV_iDevBus      == iBus     ) &&
            (hDevHandle->PCIDEV_iDevDevice   == iDevice  ) &&
            (hDevHandle->PCIDEV_iDevFunction == iFunction)) {
            break;
        }
    }
    __PCI_DEV_UNLOCK();                                                 /*  解锁 PCI 驱动               */

    if (plineTemp) {
        return  (hDevHandle);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** 函数名称: API_PciDevAdd
** 功能描述: 增加一个设备
** 输　入  : iBus           总线号
**           iDevice        设备号
**           iFunction      功能号
** 输　出  : 设备控制句柄
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
PCI_DEV_HANDLE  API_PciDevAdd (INT  iBus, INT  iDevice, INT  iFunction)
{
    PCI_DEV_HANDLE      hDevHandle = LW_NULL;
    PCI_HDR             phPciHdr;
    INT                 iType;

    hDevHandle = API_PciDevHandleGet(iBus, iDevice, iFunction);         /* 获取设备句柄                 */
    if (hDevHandle != LW_NULL) {
        return  (hDevHandle);
    }

    lib_bzero(&phPciHdr, sizeof(PCI_HDR));
    API_PciGetHeader(iBus, iDevice, iFunction, &phPciHdr);              /* 获取设备头信息               */
    iType = phPciHdr.PCIH_ucType & PCI_HEADER_TYPE_MASK;                /* 获取设备类型                 */
    if ((iType != PCI_HEADER_TYPE_NORMAL) &&
        (iType != PCI_HEADER_TYPE_BRIDGE) &&
        (iType != PCI_HEADER_TYPE_CARDBUS)) {                           /* 设备类型错误                 */
        return  (LW_NULL);
    }

    hDevHandle = (PCI_DEV_HANDLE)__SHEAP_ZALLOC(sizeof(PCI_DEV_CB));    /* 分配设备控制块               */
    if (hDevHandle == LW_NULL) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (LW_NULL);
    }
    /*
     *  保存设备参数
     */
    hDevHandle->PCIDEV_iDevBus      = iBus;
    hDevHandle->PCIDEV_iDevDevice   = iDevice;
    hDevHandle->PCIDEV_iDevFunction = iFunction;
    lib_memcpy(&hDevHandle->PCIDEV_phDevHdr, &phPciHdr, sizeof(PCI_HDR));

    switch (iType) {

    case PCI_HEADER_TYPE_NORMAL:
        hDevHandle->PCIDEV_iType  = PCI_HEADER_TYPE_NORMAL;
        hDevHandle->PCIDEV_ucPin  = hDevHandle->PCIDEV_phDevHdr.PCIH_pcidHdr.PCID_ucIntPin;
        hDevHandle->PCIDEV_ucLine = hDevHandle->PCIDEV_phDevHdr.PCIH_pcidHdr.PCID_ucIntLine;
        break;

    case PCI_HEADER_TYPE_BRIDGE:
        hDevHandle->PCIDEV_iType  = PCI_HEADER_TYPE_BRIDGE;
        hDevHandle->PCIDEV_ucPin  = hDevHandle->PCIDEV_phDevHdr.PCIH_pcibHdr.PCIB_ucIntPin;
        hDevHandle->PCIDEV_ucLine = hDevHandle->PCIDEV_phDevHdr.PCIH_pcibHdr.PCIB_ucIntLine;
        break;

    case PCI_HEADER_TYPE_CARDBUS:
        hDevHandle->PCIDEV_iType  = PCI_HEADER_TYPE_CARDBUS;
        hDevHandle->PCIDEV_ucPin  = hDevHandle->PCIDEV_phDevHdr.PCIH_pcicbHdr.PCICB_ucIntPin;
        hDevHandle->PCIDEV_ucLine = hDevHandle->PCIDEV_phDevHdr.PCIH_pcicbHdr.PCICB_ucIntLine;
        break;

    default:
        __SHEAP_FREE(hDevHandle);
        return  (LW_NULL);
    }

    __PCI_DEV_LOCK();                                                   /*  锁定 PCI 驱动               */
    _List_Line_Add_Tail(&hDevHandle->PCIDEV_lineDevNode, &_GplinePciDevHeader);
    _GuiPciDevTotalNum += 1;
    __PCI_DEV_UNLOCK();                                                 /*  解锁 PCI 驱动               */

    return  (hDevHandle);
}
/*********************************************************************************************************
** 函数名称: API_PciDevDelete
** 功能描述: 删除一个 PCI 设备
** 输　入  : hHandle       设备控制句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_PciDevDelete (PCI_DEV_HANDLE  hHandle)
{
    PLW_LIST_LINE       plineTemp  = LW_NULL;
    PCI_DEV_HANDLE      hDevHandle = LW_NULL;

    if (hHandle == LW_NULL) {
        __PCI_DEV_LOCK();                                               /*  锁定 PCI 驱动               */
        plineTemp = _GplinePciDevHeader;
        while (plineTemp) {
            hDevHandle = _LIST_ENTRY(plineTemp, PCI_DEV_CB, PCIDEV_lineDevNode);
            plineTemp  = _list_line_get_next(plineTemp);
            
            _List_Line_Del(&hDevHandle->PCIDEV_lineDevNode, &_GplinePciDevHeader);
            _GuiPciDevTotalNum -= 1;
            __SHEAP_FREE(hDevHandle);
        }
        __PCI_DEV_UNLOCK();                                             /*  解锁 PCI 驱动               */

        return  (ERROR_NONE);
    }

    hDevHandle = API_PciDevHandleGet(hHandle->PCIDEV_iDevBus,
                                     hHandle->PCIDEV_iDevDevice,
                                     hHandle->PCIDEV_iDevFunction);
    if ((hDevHandle == LW_NULL) ||
        (hDevHandle != hHandle)) {
        return  (PX_ERROR);
    }

    __PCI_DEV_LOCK();                                                   /*  锁定 PCI 驱动               */
    _List_Line_Del(&hHandle->PCIDEV_lineDevNode, &_GplinePciDevHeader);
    _GuiPciDevTotalNum -= 1;
    __PCI_DEV_UNLOCK();                                                 /*  解锁 PCI 驱动               */

    __SHEAP_FREE(hHandle);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciDevIoRemapEx
** 功能描述: 将物理 IO 空间指定内存映射到逻辑空间
** 输　入  : pvPhysicalAddr     物理内存地址
**           stSize             需要映射的内存大小
**           ulFlags            内存属性 LW_VMM_FLAG_DMA / LW_VMM_FLAG_RDWR / LW_VMM_FLAG_READ
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
PVOID  API_PciDevIoRemapEx (PVOID  pvPhysicalAddr, size_t  stSize, ULONG  ulFlags)
{
#if LW_CFG_VMM_EN > 0
    size_t  stSizeAlign = ROUND_UP(stSize, LW_CFG_VMM_PAGE_SIZE);
    addr_t  ulBaseAlign = ROUND_DOWN(((addr_t)pvPhysicalAddr), LW_CFG_VMM_PAGE_SIZE);
    addr_t  ulOffset    = (addr_t)pvPhysicalAddr - ulBaseAlign;
    PVOID   pvRet;
    
    pvRet = API_VmmIoRemapEx((PVOID)ulBaseAlign, stSizeAlign, ulFlags);
    if (!pvRet) {
        return  (LW_NULL);
    }
    
    pvRet = (PVOID)((addr_t)pvRet + ulOffset);
    
    return  (pvRet);
#else
    return  (pvPhysicalAddr);
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** 函数名称: API_PciDevIoRemap
** 功能描述: 将物理 IO 空间指定内存映射到逻辑空间
** 输　入  : pvPhysicalAddr     物理内存地址
**           stSize             需要映射的内存大小
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
PVOID  API_PciDevIoRemap (PVOID  pvPhysicalAddr, size_t  stSize)
{
    return  (API_PciDevIoRemapEx(pvPhysicalAddr, stSize, LW_VMM_FLAG_DMA));
}
/*********************************************************************************************************
** 函数名称: API_PciDevDrvDel
** 功能描述: 删除一个 PCI 设备的驱动
** 输　入  : hDevHandle     设备控制句柄
**           hDrvHandle     驱动句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_PciDevDrvDel (PCI_DEV_HANDLE  hDevHandle, PCI_DRV_HANDLE  hDrvHandle)
{
    if ((hDevHandle == LW_NULL) ||
        (hDrvHandle == LW_NULL)) {
        return  (PX_ERROR);
    }

    if (hDevHandle->PCIDEV_pvDevDriver != hDrvHandle) {
        return  (PX_ERROR);
    }

    hDevHandle->PCIDEV_pvDevDriver = LW_NULL;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciDevDrvUpdate
** 功能描述: 更新一个 PCI 设备的驱动
** 输　入  : hDevHandle     设备控制句柄
**           hDrvHandle     驱动句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_PciDevDrvUpdate (PCI_DEV_HANDLE  hDevHandle, PCI_DRV_HANDLE  hDrvHandle)
{
    if ((hDevHandle == LW_NULL) ||
        (hDrvHandle == LW_NULL)) {
        return  (PX_ERROR);
    }

    if ((hDevHandle->PCIDEV_pvDevDriver == LW_NULL) ||
        (hDevHandle->PCIDEV_pvDevDriver != hDrvHandle)) {
        hDevHandle->PCIDEV_pvDevDriver = (PVOID)hDrvHandle;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciDrvBindEachDev
** 功能描述: 将一个PCI驱动尝试与总线上的每个设备(未加载驱动)进行绑定
** 输　入  : hDrvHandle     驱动句柄
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 由于 API_PciDrvLoad 内部会加入 DRV_LOCK 经过代码检查, 此处不会发生死锁.

                                           API 函数
*********************************************************************************************************/
LW_API
VOID  API_PciDrvBindEachDev (PCI_DRV_HANDLE hDrvHandle)
{
    PCI_DEV_HANDLE        hDevCurr;
    PLW_LIST_LINE         plineTemp;
    PCI_DEV_ID_HANDLE     hId;

    __PCI_DEV_LOCK();                                                   /*  锁定 PCI 驱动               */
    for (plineTemp  = _GplinePciDevHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {

        hDevCurr = _LIST_ENTRY(plineTemp, PCI_DEV_CB, PCIDEV_lineDevNode);

        if (hDevCurr->PCIDEV_pvDevDriver) {                             /*  已经绑定了驱动              */
            continue;
        }

        hId = API_PciDevMatchDrv(hDevCurr, hDrvHandle);
        if (!hId) {
            continue;                                                   /*  ID 不匹配                   */
        }

        API_PciDrvLoad(hDrvHandle, hDevCurr, hId);                      /*  绑定设备驱动                */
    }
    __PCI_DEV_UNLOCK();                                                 /*  解锁 PCI 驱动               */
}
/*********************************************************************************************************
** 函数名称: API_PciDevMasterEnable
** 功能描述: 使能 PCI 设备 Master 模式
** 输　入  : hDevHandle   设备控制句柄
**           bEnable      是否使能
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_PciDevMasterEnable (PCI_DEV_HANDLE  hDevHandle, BOOL bEnable)
{
    UINT16 usCmdOld;
    UINT16 usCmd;
    INT    iRet;

    iRet = API_PciDevConfigRead(hDevHandle, PCI_COMMAND, (UINT8 *)&usCmdOld, sizeof(UINT16));
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    if (bEnable) {
        usCmd = usCmdOld | PCI_COMMAND_MASTER;
    
    } else {
        usCmd = usCmdOld & ~PCI_COMMAND_MASTER;
    }

    if (usCmd != usCmdOld) {
        iRet = API_PciDevConfigWrite(hDevHandle, PCI_COMMAND, (UINT8 *)&usCmd, sizeof(UINT16));
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __pciDevListCreate
** 功能描述: 创建设备列表回调, 上层已经加锁, 不需要再进行加锁操作
** 输　入  : iBus           总线号
**           iDevice        设备号
**           iFunction      功能号
**           pvArg          参数
** 输　出  : ERROR_NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __pciDevListCreate (INT  iBus, INT  iDevice, INT  iFunction, PVOID pvArg)
{
    API_PciDevAdd(iBus, iDevice, iFunction);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciDevListCreate
** 功能描述: 创建 PCI 设备列表
** 输　入  : NONE
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciDevListCreate (VOID)
{
    API_PciTraversal(__pciDevListCreate, LW_NULL, PCI_MAX_BUS - 1);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciDevInit
** 功能描述: PCI 设备管理初始化
** 输　入  : NONE
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciDevInit (VOID)
{
    _GuiPciDevTotalNum  = 0;
    _GplinePciDevHeader = LW_NULL;
    
    _GulPciDevLock = API_SemaphoreMCreate("pci_dev_lock",
                                          LW_PRIO_DEF_CEILING,
                                          LW_OPTION_WAIT_PRIORITY |
                                          LW_OPTION_DELETE_SAFE |
                                          LW_OPTION_INHERIT_PRIORITY |
                                          LW_OPTION_OBJECT_GLOBAL,
                                          LW_NULL);
    if (_GulPciDevLock == LW_OBJECT_HANDLE_INVALID) {
        return  (PX_ERROR);
    }

    API_TShellKeywordAdd("pcidev", __tshellPciCmdDev);
    API_TShellFormatAdd("pcidev", " [add | del] [[all] | 1:0.1]");
    API_TShellHelpAdd("pcidev", "show, add, del pci device table\n"
                                "eg. pcidev\n"
                                "    pcidev add all\n"
                                "    pcidev add 1:0.1\n"
                                "    pcidev del 1:0.1\n");

    API_TShellKeywordAdd("pciparent", __tshellPciCmdDevParent);
    API_TShellFormatAdd("pciparent", " [1:0.1]");
    API_TShellHelpAdd("pciparent", "show pci device parent node\n"
                                   "eg. pciparent 1:0.1\n");

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PCI_EN > 0)         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
