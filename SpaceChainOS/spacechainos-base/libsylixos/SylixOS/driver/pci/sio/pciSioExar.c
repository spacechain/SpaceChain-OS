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
** 文   件   名: pciSioExar.c
**
** 创   建   人: Gong.YuJian (弓羽箭)
**
** 文件创建日期: 2016 年 06 月 12 日
**
** 描        述: PCI EXAR 16c550 设备驱动.
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
#include "pciSioExar.h"
/*********************************************************************************************************
  端口地址宏定义
*********************************************************************************************************/
#define FL_BASE_MASK                0x0007
#define FL_BASE0                    0x0000
#define FL_GET_BASE(x)              (x & FL_BASE_MASK)
/*********************************************************************************************************
  17v35x 增强寄存器
*********************************************************************************************************/
#define XR_17V35X_UART_DLD          2
#define XR_17V35X_UART_MSR          6
#define XR_17V35X_EXTENDED_FCTR     8
#define XR_17V35X_EXTENDED_EFR      9
#define XR_17V35X_TXFIFO_CNT        10
#define XR_17V35X_EXTENDED_TXTRG    10
#define XR_17V35X_RXFIFO_CNT        11
#define XR_17V35X_EXTENDED_RXTRG    11
#define XR_17V35X_UART_XOFF2        13
/*********************************************************************************************************
  17v35x FCTR
*********************************************************************************************************/
#define XR_17V35X_FCTR_RTS_8DELAY   0x03
#define XR_17V35X_FCTR_TRGD         192
#define XR_17V35x_FCTR_RS485        0x20
/*********************************************************************************************************
  17v35x Rx Tx offset
*********************************************************************************************************/
#define XR_17V35X_RX_OFFSET         0x100
#define XR_17V35X_TX_OFFSET         0x100
/*********************************************************************************************************
  17v35x IER modem
*********************************************************************************************************/
#define XR_17V35X_IER_RTSDTR        0x40
#define XR_17V35X_IER_CTSDSR        0x80
/*********************************************************************************************************
  17v35x mode
*********************************************************************************************************/
#define XR_17V35X_8XMODE            0x88
#define XR_17V35X_4XMODE            0x89
/*********************************************************************************************************
  端口描述
*********************************************************************************************************/
enum exax_port_id {
    xr_8port = 0,
    xr_4port,
    xr_2port,
    xr_4354port,
    xr_8354port,
    xr_4358port,
    xr_8358port,
    xr_258port,
    xr_254port,
    xr_252port
};
/*********************************************************************************************************
  EXAR PCI 设备ID支持信息表
*********************************************************************************************************/
static const PCI_DEV_ID_CB  pciSioExarIdTbl[] = {
    {
        PCI_VENDOR_ID_EXAR, PCI_DEVICE_ID_EXAR_XR17V358,
        PCI_ANY_ID, PCI_ANY_ID, 0, 0, 
        xr_8port
    }, 
    {
        PCI_VENDOR_ID_EXAR, PCI_DEVICE_ID_EXAR_XR17V354,
        PCI_ANY_ID, PCI_ANY_ID, 0, 0, 
        xr_4port
    }, 
    {
        PCI_VENDOR_ID_EXAR, PCI_DEVICE_ID_EXAR_XR17V352,
        PCI_ANY_ID, PCI_ANY_ID, 0, 0, 
        xr_2port
    }, 
    {
        PCI_VENDOR_ID_EXAR, PCI_DEVICE_ID_EXAR_XR17V4354,
        PCI_ANY_ID, PCI_ANY_ID, 0, 0, 
        xr_4354port
    }, 
    {
        PCI_VENDOR_ID_EXAR, PCI_DEVICE_ID_EXAR_XR17V8354,
        PCI_ANY_ID, PCI_ANY_ID, 0, 0, 
        xr_8354port
    }, 
    {
        PCI_VENDOR_ID_EXAR, PCI_DEVICE_ID_EXAR_XR17V4358,
        PCI_ANY_ID, PCI_ANY_ID, 0, 0, 
        xr_4358port
    }, 
    {
        PCI_VENDOR_ID_EXAR, PCI_DEVICE_ID_EXAR_XR17V8358,
        PCI_ANY_ID, PCI_ANY_ID, 0, 0, 
        xr_8358port
    }, 
    {
        PCI_VENDOR_ID_EXAR, PCI_DEVICE_ID_EXAR_XR17C258,
        PCI_ANY_ID, PCI_ANY_ID, 0, 0, 
        xr_258port
    }, 
    {
        PCI_VENDOR_ID_EXAR, PCI_DEVICE_ID_EXAR_XR17C254,
        PCI_ANY_ID, PCI_ANY_ID, 0, 0, 
        xr_254port
    }, 
    {
        PCI_VENDOR_ID_EXAR, PCI_DEVICE_ID_EXAR_XR17C252,
        PCI_ANY_ID, PCI_ANY_ID, 0, 0, 
        xr_252port
    }, 
    {
    }                                                                   /* terminate list               */
};
/*********************************************************************************************************
  EXAR 芯片相关信息
*********************************************************************************************************/
typedef struct {
    UINT    EXAR_uiFlags;
    UINT    EXAR_uiPorts;
    UINT    EXAR_uiBaud;
    UINT    EXAR_uiUartOff;
    UINT    EXAR_uiRegShift;
    UINT    EXAR_uiFirstOff;
} PCI_SIO_EXAR;
/*********************************************************************************************************
  EXAR 16C550 SIO 配置定义
*********************************************************************************************************/
typedef struct {
       UINT             CFG_uiType;
#define EXAR_TYPE_DFT   0
#define EXAR_TYPE_35X   1

       ULONG            CFG_ulBase;
       ULONG            CFG_ulXtal;
       ULONG            CFG_ulBaud;
       ULONG            CFG_ulVector;
       INT              CFG_idx;                                          /*  通道索引号                */
       PCI_DEV_HANDLE   CFG_pciHandle;
} PCI_SIO_EXAR_CFG;
/*********************************************************************************************************
  设备信息
*********************************************************************************************************/
static PCI_SIO_EXAR  pciSioExarCard[] = {
    {
        FL_BASE0,  8, 7812500 * 4, 0x400, 0, 0
    },
    {
        FL_BASE0,  4, 7812500 * 4, 0x400, 0, 0
    },
    {
        FL_BASE0,  2, 7812500 * 4, 0x400, 0, 0
    },
    {
        FL_BASE0,  8, 7812500 * 4, 0x400, 0, 0
    },
    {
        FL_BASE0, 12, 7812500 * 4, 0x400, 0, 0
    },
    {
        FL_BASE0, 12, 7812500 * 4, 0x400, 0, 0
    },
    {
        FL_BASE0, 16, 7812500 * 4, 0x400, 0, 0
    },
    {
        FL_BASE0,  8, 1500000,     0x200, 0, 0
    },
    {
        FL_BASE0,  4, 1500000,     0x200, 0, 0
    },
    {
        FL_BASE0,  2, 1500000,     0x200, 0, 0
    }
};
/*********************************************************************************************************
** 函数名称: pcisioExarSetReg
** 功能描述: 设置 EXAR 16C550 寄存器
** 输　入  : psiochan              16C550 SIO 通道
**           iReg                  寄存器
**           ucValue               值
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  pciSioExarSetReg (SIO16C550_CHAN  *psiochan, INT  iReg, UINT8  ucValue)
{
    REGISTER PCI_SIO_EXAR_CFG  *psioexarcfg = (PCI_SIO_EXAR_CFG *)psiochan->priv;

    write8(ucValue, psioexarcfg->CFG_ulBase + iReg);
}
/*********************************************************************************************************
** 函数名称: pcisioExarGetReg
** 功能描述: 获得 EXAR 16C550 寄存器的值
** 输　入  : psiochan              16C550 SIO 通道
**           iReg                  寄存器
** 输　出  : 寄存器的值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT8  pciSioExarGetReg (SIO16C550_CHAN  *psiochan, INT  iReg)
{
    REGISTER PCI_SIO_EXAR_CFG  *psioexarcfg = (PCI_SIO_EXAR_CFG *)psiochan->priv;
    
    return  (read8(psioexarcfg->CFG_ulBase + iReg));
}
/*********************************************************************************************************
** 函数名称: pciSioExarIsr
** 功能描述: EXAR 中断服务
** 输　入  : psiochan        16C550 SIO 通道
**           ulVector        中断向量号
** 输　出  : 中断返回值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static irqreturn_t  pciSioExarIsr (SIO16C550_CHAN  *psiochan, ULONG  ulVector)
{
    PCI_SIO_EXAR_CFG    *psioexarcfg;
    UINT8                ucReg;

    ucReg = pciSioExarGetReg(psiochan, IIR);
    if (ucReg & 0x01) {
        return  (LW_IRQ_NONE);
    }

    psioexarcfg = (PCI_SIO_EXAR_CFG *)psiochan->priv;

    /*
     * 该类型芯片需要读取相关寄存器以清除中断
     */
    if (psioexarcfg->CFG_uiType == EXAR_TYPE_35X) {
        ucReg = pciSioExarGetReg(psiochan, 0x80);
        ucReg = pciSioExarGetReg(psiochan, 0x81);
        ucReg = pciSioExarGetReg(psiochan, 0x82);
        ucReg = pciSioExarGetReg(psiochan, 0x83);
    }

    sio16c550Isr(psiochan);

    return  (LW_IRQ_HANDLED);
}
/*********************************************************************************************************
** 函数名称: pciSioExarChan
** 功能描述: 创建一个 SIO 通道
** 输　入  : uiChannel                 硬件通道号
**           psiochan                  16C550 SIO 通道
**           psioexarcfg               EXAR 通道配置
** 输　出  : SIO 通道
** 全局变量:
** 调用模块:
*********************************************************************************************************/
SIO_CHAN  *pciSioExarChan (UINT               uiChannel,
                           SIO16C550_CHAN    *psiochan,
                           PCI_SIO_EXAR_CFG  *psioexarcfg)
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

    psiochan->baud   = psioexarcfg->CFG_ulBaud;
    psiochan->xtal   = psioexarcfg->CFG_ulXtal;
    psiochan->setreg = pciSioExarSetReg;
    psiochan->getreg = pciSioExarGetReg;

    psiochan->priv = psioexarcfg;

    API_PciDevInterDisable(psioexarcfg->CFG_pciHandle, psioexarcfg->CFG_ulVector,
                              (PINT_SVR_ROUTINE)pciSioExarIsr,
                              (PVOID)psiochan);

    sio16c550Init(psiochan);

    snprintf(cIrqName, 64, "pci_exar_%d", uiChannel);
    
    API_PciDevInterConnect(psioexarcfg->CFG_pciHandle, psioexarcfg->CFG_ulVector,
                           (PINT_SVR_ROUTINE)pciSioExarIsr, 
                           (PVOID)psiochan, cIrqName);

    API_PciDevInterEnable(psioexarcfg->CFG_pciHandle, psioexarcfg->CFG_ulVector,
                          (PINT_SVR_ROUTINE)pciSioExarIsr,
                          (PVOID)psiochan);

    return  ((SIO_CHAN *)psiochan);
}
/*********************************************************************************************************
** 函数名称: pciSioExarRemove
** 功能描述: 总线上移除 PCI 设备时的处理
** 输　入  : hPciDevHandle     PCI 设备句柄
** 输　出  : NONE
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
static VOID  pciSioExarRemove (PCI_DEV_HANDLE hHandle)
{
}
/*********************************************************************************************************
** 函数名称: pciSioExarPortCfg
** 功能描述: 配置 EXAR 串口通道
** 输　入  : iDev         PCI 设备ID
**           psioexar     EXAR 描述
**           psioexarcfg  EXAR 通道配置
** 输　出  : 控制器句柄
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
static VOID  pciSioExarPortCfg (INT                 iDevId,
                                PCI_SIO_EXAR       *psioexar,
                                PCI_SIO_EXAR_CFG   *psioexarcfg)
{
    UINT    uiBar;
    UINT    uiOff = psioexar->EXAR_uiFirstOff;

    uiBar = FL_GET_BASE(psioexar->EXAR_uiFlags);
    if (uiBar > 6) {
        return;
    }

    uiOff += psioexarcfg->CFG_idx * psioexar->EXAR_uiUartOff;
    if (((iDevId == 0x4354) || (iDevId == 0x8354)) && (psioexarcfg->CFG_idx >= 4)) {
        uiOff += 0x1000;
    }

    psioexarcfg->CFG_ulBase = psioexarcfg->CFG_ulBase + uiOff;
}
/*********************************************************************************************************
** 函数名称: pciSioExarProbe
** 功能描述: PCI 串口驱动探测方法
** 输　入  : hDevHandle         PCI 设备控制块句柄
**           hIdEntry           匹配成功的设备 ID 条目(来自于设备 ID 表)
** 输　出  : 控制器句柄
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
static INT  pciSioExarProbe (PCI_DEV_HANDLE hPciDevHandle, const PCI_DEV_ID_HANDLE hIdEntry)
{
    INT                     i, iTtyNum;
    INT                     iPortCnt;
    
    PCI_SIO_EXAR           *pciexar;
    PCI_SIO_EXAR_CFG       *psiocfg;
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

    if (hIdEntry->PCIDEVID_ulData > ARRAY_SIZE(pciSioExarCard)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    hResource  = API_PciDevResourceGet(hPciDevHandle, PCI_IORESOURCE_MEM, 0);
    ulBaseAddr = (ULONG)(PCI_RESOURCE_START(hResource));
    stBaseSize = (size_t)(PCI_RESOURCE_SIZE(hResource));
    pvBaseAddr = API_PciDevIoRemap((PVOID)ulBaseAddr, stBaseSize);
    if (!pvBaseAddr) {
        return  (PX_ERROR);
    }

    pciexar  = &pciSioExarCard[hIdEntry->PCIDEVID_ulData];
    iPortCnt = pciexar->EXAR_uiPorts;

    hResource = API_PciDevResourceGet(hPciDevHandle, PCI_IORESOURCE_IRQ, 0);
    ulVector  = (ULONG)PCI_RESOURCE_START(hResource);

    /*
     *  创建串口通道
     */
    for (i = 0; i < iPortCnt; ++i) {
        psiochan = (SIO16C550_CHAN *)__SHEAP_ZALLOC(sizeof(SIO16C550_CHAN));
        if (!psiochan) {
            _ErrorHandle(ENOMEM);
            return  (PX_ERROR);
        }

        psiocfg = (PCI_SIO_EXAR_CFG *)__SHEAP_ZALLOC(sizeof(PCI_SIO_EXAR_CFG));
        if (!psiocfg) {
            __SHEAP_FREE(psiocfg);
            _ErrorHandle(ENOMEM);
            return  (PX_ERROR);
        }

        psiocfg->CFG_idx       = i;
        psiocfg->CFG_ulVector  = ulVector;
        psiocfg->CFG_ulBase    = (ULONG)pvBaseAddr;
        psiocfg->CFG_ulBaud    = 115200;
        psiocfg->CFG_ulXtal    = pciexar->EXAR_uiBaud * 4;
        psiocfg->CFG_pciHandle = hPciDevHandle;

        pciSioExarPortCfg(hIdEntry->PCIDEVID_uiDevice, pciexar, psiocfg);

        switch (hIdEntry->PCIDEVID_uiDevice) {
        
        case PCI_DEVICE_ID_EXAR_XR17V4354:
        case PCI_DEVICE_ID_EXAR_XR17V8354:
            if (i >= 4) {
                psiocfg->CFG_ulXtal = 62500000;
            }
            psiocfg->CFG_uiType = EXAR_TYPE_35X;
            break;
            
        case PCI_DEVICE_ID_EXAR_XR17V4358:
        case PCI_DEVICE_ID_EXAR_XR17V8358:
            if (i >= 8) {
                psiocfg->CFG_ulXtal = 62500000;
            }
            psiocfg->CFG_uiType = EXAR_TYPE_35X;
            break;

        default:
            break;
        }

        psio = pciSioExarChan(i, psiochan, psiocfg);

        for (iTtyNum = 0; iTtyNum < 512; iTtyNum++) {
            snprintf(cDevName, sizeof(cDevName),
                     PCI_SIO_EXAR_TTY_PERFIX "%d", iTtyNum);
            if (!API_IosDevMatchFull(cDevName)) {
                break;
            }
        }

        ttyDevCreate(cDevName, psio,
                     PCI_SIO_EXAR_TTY_RBUF_SZ, 
                     PCI_SIO_EXAR_TTY_SBUF_SZ);                         /*  add    tty   device         */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pciSioExarInit
** 功能描述: PCI EXAR 16c550 控制器驱动相关初始化
** 输　入  : NONE
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  pciSioExarInit (VOID)
{
    INT                iRet;
    PCI_DRV_CB         tPciDrvReg;
    PCI_DRV_HANDLE     hPciDrvReg = &tPciDrvReg;

    lib_bzero(hPciDrvReg, sizeof(PCI_DRV_CB));
    lib_strlcpy(hPciDrvReg->PCIDRV_cDrvName, "pci_exar", PCI_DRV_NAME_MAX);

    hPciDrvReg->PCIDRV_hDrvIdTable      = (PCI_DEV_ID_HANDLE)pciSioExarIdTbl;
    hPciDrvReg->PCIDRV_uiDrvIdTableSize = ARRAY_SIZE(pciSioExarIdTbl);

    hPciDrvReg->PCIDRV_pfuncDevProbe    = pciSioExarProbe;
    hPciDrvReg->PCIDRV_pfuncDevRemove   = pciSioExarRemove;

    iRet = API_PciDrvRegister(hPciDrvReg);
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
