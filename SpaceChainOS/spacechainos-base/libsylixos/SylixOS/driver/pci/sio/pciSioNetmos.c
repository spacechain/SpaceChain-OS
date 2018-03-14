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
** 文   件   名: pciSioNetmos.c
**
** 创   建   人: Lu.Zhenping (卢振平)
**
** 文件创建日期: 2016 年 06 月 12 日
**
** 描        述: PCI NETMOS 16c550 设备驱动.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_PCI_DRV
#include "SylixOS.h"
#include "../SylixOS/config/driver/drv_cfg.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PCI_EN > 0) && (LW_CFG_DRV_SIO_16C550 > 0)
#include "pci_ids.h"
#include "linux/compat.h"
#include "driver/sio/16c550.h"
#include "pciSioNetmos.h"
/*********************************************************************************************************
  端口地址宏定义
*********************************************************************************************************/
#define FL_BASE_MASK        0x0007
#define FL_BASE0            0x0000
#define FL_BASE1            0x0001
#define FL_BASE2            0x0002
#define FL_BASE3            0x0003
#define FL_BASE4            0x0004
#define FL_GET_BASE(x)      (x & FL_BASE_MASK)
/*********************************************************************************************************
  Use successive BARs (PCI base address registers), else use offset into some specified BAR
*********************************************************************************************************/
#define FL_BASE_BARS        0x0008
/*********************************************************************************************************
  Do not assign an irq
*********************************************************************************************************/
#define FL_NOIRQ            0x0080
/*********************************************************************************************************
  Use the Base address register size to cap number of ports
*********************************************************************************************************/
#define FL_REGION_SZ_CAP    0x0100
/*********************************************************************************************************
  板载类型.
*********************************************************************************************************/
enum {
    netmos_9912 = 0
};
/*********************************************************************************************************
  驱动支持的设备 ID 表, 用于驱动与设备进行自动匹配, 与 Linux 参数保持一致.
*********************************************************************************************************/
static const PCI_DEV_ID_CB  pciSioNetmosIdTbl[] = {
    {
        PCI_VENDOR_ID_NETMOS, PCI_DEVICE_ID_NETMOS_9901,
        0xa000, 0x1000, 0, 0,
        netmos_9912
    },
    {
        PCI_VENDOR_ID_NETMOS, PCI_DEVICE_ID_NETMOS_9912,
        0xa000, 0x1000, 0, 0,
        netmos_9912
    },
    {
        PCI_VENDOR_ID_NETMOS, PCI_DEVICE_ID_NETMOS_9922,
        0xa000, 0x1000, 0, 0,
        netmos_9912
    },
    {
        PCI_VENDOR_ID_NETMOS, PCI_DEVICE_ID_NETMOS_9904,
        0xa000, 0x1000, 0, 0,
        netmos_9912
    },
    {
        PCI_VENDOR_ID_NETMOS, PCI_DEVICE_ID_NETMOS_9900,
        0xa000, 0x1000, 0, 0,
        netmos_9912
    },
    {
    }                                                                   /* terminate list               */
};
/*********************************************************************************************************
  设备支持情况
*********************************************************************************************************/
typedef struct {
    UINT32      NETMOS_uiFlags;
    UINT32      NETMOS_uiPorts;
    UINT32      NETMOS_uiBaud;
    UINT32      NETMOS_uiUartOff;
    UINT32      NETMOS_uiRegShift;
    UINT32      NETMOS_uiFirstOff;
} PCI_SIO_NETMOS;
/*********************************************************************************************************
  NETMOS 16C550 SIO 配置定义
*********************************************************************************************************/
typedef struct {
    ULONG            CFG_ulBase;
    ULONG            CFG_ulXtal;
    ULONG            CFG_ulBaud;
    ULONG            CFG_ulVector;
    INT              CFG_idx;                                           /* 通道索引号                   */
    PCI_DEV_HANDLE   CFG_pciHandle;
} PCI_SIO_NETMOS_CFG;
/*********************************************************************************************************
  设备板卡信息
*********************************************************************************************************/
static PCI_SIO_NETMOS   pciSioNetmosCard[] = {
    {
        FL_BASE1, 1, 115200, 0, 0, 0
    },
};
/*********************************************************************************************************
** 函数名称: pciSioNetmosSetReg
** 功能描述: 设置 NETMOS 16C550 寄存器
** 输　入  : psiochan        16C550 SIO 通道
**           iReg            寄存器
**           ucValue         值
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  pciSioNetmosSetReg (SIO16C550_CHAN  *psiochan, INT  iReg, UINT8  ucValue)
{
    REGISTER PCI_SIO_NETMOS_CFG  *pcisiocfg = (PCI_SIO_NETMOS_CFG *)psiochan->priv;

    write8(ucValue, pcisiocfg->CFG_ulBase + 0x280 + ((addr_t)iReg * 4));
}
/*********************************************************************************************************
** 函数名称: pciSioNetmosGetReg
** 功能描述: 获得 NETMOS 16C550 寄存器的值
** 输　入  : psiochan        16C550 SIO 通道
**           iReg            寄存器
** 输　出  : 寄存器的值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT8  pciSioNetmosGetReg (SIO16C550_CHAN  *psiochan, INT  iReg)
{
    REGISTER PCI_SIO_NETMOS_CFG  *pcisiocfg = (PCI_SIO_NETMOS_CFG *)psiochan->priv;
    
    return  (read8(pcisiocfg->CFG_ulBase + 0x280 + ((addr_t)iReg * 4)));
}
/*********************************************************************************************************
** 函数名称: pciSioNetmosIsr
** 功能描述: PCI 设备中断服务
** 输　入  : pvArg      中断参数
**           ulVector   中断向量
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static irqreturn_t  pciSioNetmosIsr (PVOID  pvArg, ULONG  ulVector)
{
    REGISTER SIO16C550_CHAN  *psiochan = (SIO16C550_CHAN *)pvArg;
             UINT8            ucIIR;
    
    ucIIR = pciSioNetmosGetReg(psiochan, IIR);
    if (ucIIR & 0x01) {
        return  (LW_IRQ_NONE);
    }

    sio16c550Isr(psiochan);
    
    return  (LW_IRQ_HANDLED);
}
/*********************************************************************************************************
** 函数名称: pciSioNetmosChan
** 功能描述: 创建一个 SIO 通道
** 输　入  : uiChannel     硬件通道号
**           psiochan      16c550 chan
**           pcisiocfg     netmos cfg
** 输　出  : SIO 通道
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static SIO_CHAN  *pciSioNetmosChan (UINT                uiChannel, 
                                    SIO16C550_CHAN     *psiochan, 
                                    PCI_SIO_NETMOS_CFG *pcisiocfg)
{
    CHAR    cIrqName[64];

    psiochan->pdeferq = API_InterDeferGet(0);

    /*
     *  Receiver FIFO Trigger Level and Tirgger bytes table
     *  level  16 Bytes FIFO Trigger   32 Bytes FIFO Trigger  64 Bytes FIFO Trigger
     *    0              1                       8                    1
     *    1              4                      16                   16
     *    2              8                      24                   32
     *    3             14                      28                   56
     */
    psiochan->fifo_len         = 8;
    psiochan->rx_trigger_level = 1;
    psiochan->iobase           = 0;                                     /*  NO IO Base                  */

    psiochan->baud   = pcisiocfg->CFG_ulBaud;
    psiochan->xtal   = pcisiocfg->CFG_ulXtal;
    psiochan->setreg = pciSioNetmosSetReg;
    psiochan->getreg = pciSioNetmosGetReg;

    psiochan->priv = pcisiocfg;

    API_PciDevInterDisable(pcisiocfg->CFG_pciHandle, pcisiocfg->CFG_ulVector,
                           (PINT_SVR_ROUTINE)pciSioNetmosIsr,
                           (PVOID)psiochan);

    sio16c550Init(psiochan);

    snprintf(cIrqName, sizeof(cIrqName), "pci_netmos_%d", uiChannel);
    
    API_PciDevInterConnect(pcisiocfg->CFG_pciHandle, pcisiocfg->CFG_ulVector,
                           (PINT_SVR_ROUTINE)pciSioNetmosIsr, 
                           (PVOID)psiochan, cIrqName);

    API_PciDevInterEnable(pcisiocfg->CFG_pciHandle, pcisiocfg->CFG_ulVector,
                          (PINT_SVR_ROUTINE)pciSioNetmosIsr,
                          (PVOID)psiochan);

    return  ((SIO_CHAN *)psiochan);
}
/*********************************************************************************************************
** 函数名称: pciSioNetmosIdTblGet
** 功能描述: 获取设备 ID 表的表头与表的大小
** 输　入  : hPciDevId      设备 ID 列表句柄缓冲区
**           puiSzie        设备 ID 列表大小缓冲区
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  pciSioNetmosIdTblGet (PCI_DEV_ID_HANDLE *hPciDevId, UINT32 *puiSzie)
{
    if ((!hPciDevId) || (!puiSzie)) {                                   /*  参数无效                    */
        return  (PX_ERROR);                                             /*  错误返回                    */
    }

    *hPciDevId = (PCI_DEV_ID_HANDLE)pciSioNetmosIdTbl;                  /*  获取表头                    */
    *puiSzie   = sizeof(pciSioNetmosIdTbl) / sizeof(PCI_DEV_ID_CB);     /*  获取表的大小                */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pciSioNetmosRemove
** 功能描述: 总线上移除 PCI 设备时的处理
** 输　入  : hPciDevHandle     PCI 设备句柄
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  pciSioNetmosRemove (PCI_DEV_HANDLE hPciDevHandle)
{
}
/*********************************************************************************************************
** 函数名称: pciSioNetmosProbe
** 功能描述: NETMOS 16c550 板卡驱动探测设备
** 输　入  : hDevHandle         PCI 设备控制块句柄
**           hIdEntry           匹配成功的设备 ID 条目(来自于设备 ID 表)
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
static INT  pciSioNetmosProbe (PCI_DEV_HANDLE hPciDevHandle, const PCI_DEV_ID_HANDLE hIdEntry)
{
    INT                     i, iChanNum, iTtyNum;
    PCI_SIO_NETMOS         *pcisio;
    PCI_SIO_NETMOS_CFG     *pcisiocfg;
    SIO16C550_CHAN         *psiochan;
    SIO_CHAN               *psio;
    
    ULONG                   ulVector;
    CHAR                    cDevName[64];
    PCI_RESOURCE_HANDLE     hResource;
    addr_t                  ulBaseAddr;                                 /*  起始地址                    */
    PVOID                   pvBaseAddr;                                 /*  起始地址                    */
    size_t                  stBaseSize;                                 /*  资源大小                    */
    
    if ((!hPciDevHandle) || (!hIdEntry)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (hIdEntry->PCIDEVID_ulData > ARRAY_SIZE(pciSioNetmosCard)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    hResource  = API_PciDevResourceGet(hPciDevHandle, PCI_IORESOURCE_MEM, 0);
    ulBaseAddr = (ULONG)(PCI_RESOURCE_START(hResource));                /*  获取 MEM 的起始地址         */
    stBaseSize = (size_t)(PCI_RESOURCE_SIZE(hResource));                /*  获取 MEM 的大小             */
    pvBaseAddr = API_PciDevIoRemap((PVOID)ulBaseAddr, stBaseSize);
    if (!pvBaseAddr) {
        return  (PX_ERROR);
    }

    pcisio    = &pciSioNetmosCard[hIdEntry->PCIDEVID_ulData];
    iChanNum  = pcisio->NETMOS_uiPorts;                                 /*  获得设备通道数              */

    hResource = API_PciDevResourceGet(hPciDevHandle, PCI_IORESOURCE_IRQ, 0);
    ulVector  = (ULONG)PCI_RESOURCE_START(hResource);

    API_PciDevMasterEnable(hPciDevHandle, LW_TRUE);

    write32(0, (addr_t)pvBaseAddr + 0x3fc);

    /*
     *  创建串口通道
     */
    for (i = 0; i < iChanNum; ++i) {
        psiochan = (SIO16C550_CHAN *)__SHEAP_ZALLOC(sizeof(SIO16C550_CHAN));
        if (!psiochan) {
            _ErrorHandle(ENOMEM);
            return  (PX_ERROR);
        }

        pcisiocfg = (PCI_SIO_NETMOS_CFG *)__SHEAP_ZALLOC(sizeof(PCI_SIO_NETMOS_CFG));
        if (!pcisiocfg) {
            __SHEAP_FREE(psiochan);
            _ErrorHandle(ENOMEM);
            return  (PX_ERROR);
        }

        pcisiocfg->CFG_idx       = hPciDevHandle->PCIDEV_iDevFunction;
        pcisiocfg->CFG_ulVector  = ulVector;
        pcisiocfg->CFG_ulBase    = (addr_t)pvBaseAddr;
        pcisiocfg->CFG_ulBaud    = pcisio->NETMOS_uiBaud;
        pcisiocfg->CFG_ulXtal    = pcisio->NETMOS_uiBaud * 16;
        pcisiocfg->CFG_pciHandle = hPciDevHandle;

        psio = pciSioNetmosChan(hPciDevHandle->PCIDEV_iDevFunction, psiochan, pcisiocfg);

        for (iTtyNum = 0; iTtyNum < 512; iTtyNum++) {
            snprintf(cDevName, sizeof(cDevName), 
                     PCI_SIO_NETMOS_TTY_PERFIX "%d", iTtyNum);
            if (!API_IosDevMatchFull(cDevName)) {
                break;
            }
        }
        
        ttyDevCreate(cDevName, psio, 
                     PCI_SIO_NETMOS_TTY_RBUF_SZ, 
                     PCI_SIO_NETMOS_TTY_SBUF_SZ);                       /*  add tty device              */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pciSioNetmosInit
** 功能描述: PCI NETMOS 16c550 控制器驱动相关初始化
** 输　入  : NONE
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  pciSioNetmosInit (VOID)
{
    INT               iRet;
    PCI_DRV_CB        tPciDrv;
    PCI_DRV_HANDLE    hPciDrv = &tPciDrv;

    lib_bzero(hPciDrv, sizeof(PCI_DRV_CB));
    iRet = pciSioNetmosIdTblGet(&hPciDrv->PCIDRV_hDrvIdTable, &hPciDrv->PCIDRV_uiDrvIdTableSize);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    lib_strlcpy(&hPciDrv->PCIDRV_cDrvName[0], "pci_netmos", PCI_DRV_NAME_MAX);
    hPciDrv->PCIDRV_pvPriv         = LW_NULL;                           /*  设备驱动的私有数据          */
    hPciDrv->PCIDRV_hDrvErrHandler = LW_NULL;                           /*  驱动错误处理                */
    hPciDrv->PCIDRV_pfuncDevProbe  = pciSioNetmosProbe;
    hPciDrv->PCIDRV_pfuncDevRemove = pciSioNetmosRemove;

    iRet = API_PciDrvRegister(hPciDrv);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PCI_EN > 0) &&      */
                                                                        /*  (LW_CFG_DRV_SIO_16C550 > 0) */
/*********************************************************************************************************
  END
*********************************************************************************************************/
