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
** 文   件   名: pciStorageAta.c
**
** 创   建   人: Han.hui (韩辉)
**
** 文件创建日期: 2016 年 08 月 03 日
**
** 描        述: ATA/IDE 驱动.
*********************************************************************************************************/
#define  __SYLIXOS_PCI_DRV
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "../SylixOS/config/driver/drv_cfg.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PCI_EN > 0) && (LW_CFG_ATA_EN > 0) && (LW_CFG_DRV_ATA_IDE > 0)
#include "pci_ids.h"
#include "pciStorageAta.h"
/*********************************************************************************************************
  板载类型.
*********************************************************************************************************/
enum {
    board_std = 0,
    board_res = 1
};
/*********************************************************************************************************
  驱动支持的设备 ID 表.
*********************************************************************************************************/
static const PCI_DEV_ID_CB  pciStorageAtaIdTbl[] = {
    {
        PCI_VDEVICE(INTEL, 0x27c0), 
        board_std
    },
    {
        PCI_VDEVICE(INTEL, 0x7010), 
        board_std
    },
    {
        PCI_VDEVICE(INTEL, 0x2850),                                 /* ICH8 Mobile PATA Controller      */
        board_res
    },
    {
        PCI_VDEVICE(INTEL, 0x2828),                                 /* Mobile SATA Controller IDE       */
        board_res
    },
    {
        PCI_VDEVICE(INTEL, 0x8c00),                                 /* SATA Controller IDE(Lynx Point)  */
        board_res
    },
    {
        PCI_VDEVICE(INTEL, 0x8c01),                                 /* SATA Controller IDE(Lynx Point)  */
        board_res
    },
    {
        PCI_VDEVICE(INTEL, 0x8c80),                                 /* SATA Controller IDE(9 Series)    */
        board_res
    },
    {
        PCI_VDEVICE(INTEL, 0x8c81),                                 /* SATA Controller IDE(9 Series)    */
        board_res
    },
    {
        PCI_VDEVICE(INTEL, 0x8d00),                                 /* SATA Controller IDE(Wellsburg)   */
        board_res
    },
    {
        PCI_VDEVICE(INTEL, 0x8d60),                                 /* SATA Controller IDE(Wellsburg)   */
        board_res
    },
    {
        PCI_VDEVICE(INTEL, 0x9c00),                                 /* SATA Controller IDE(Lynx PLP)    */
        board_res
    },
    {
        PCI_VDEVICE(INTEL, 0x9c01),                                 /* SATA Controller IDE(Lynx PLP)    */
        board_res
    },
    {
        PCI_VDEVICE(INTEL, 0x1f20),                                 /* SATA Controller IDE(Avoton)      */
        board_res
    },
    {
        PCI_VDEVICE(INTEL, 0x1f21),                                 /* SATA Controller IDE(Avoton)      */
        board_res
    },
    
    {   
        PCI_ANY_ID, PCI_ANY_ID, PCI_ANY_ID, PCI_ANY_ID, 
        PCI_CLASS_STORAGE_IDE << 8, 0xffffff00ul, board_std
    },

    { }                                                                 /* terminate list               */
};
/*********************************************************************************************************
  EXAR 芯片相关信息
*********************************************************************************************************/
typedef struct {
    UINT    ATAPORT_uiPort;
    UINT    ATAPORT_uiFlag;
#define ATAPORT_IO_PORT     1
} PCI_ATA_PORT;
/*********************************************************************************************************
  设备信息
*********************************************************************************************************/
static PCI_ATA_PORT  pciAtaPort[] = {
    {
        2, ATAPORT_IO_PORT
    },
    {
        2, 0
    }
};
/*********************************************************************************************************
  IDE 通道类型定义
*********************************************************************************************************/
typedef struct {
    ATA_DRV_FUNCS      *ATACHAN_pDrvFuncs;
    ATA_CALLBACK        ATACHAN_pfuncAtaDevChk;
    PVOID               ATACHAN_pvAtaDevChkArg;
    ATA_CALLBACK        ATACHAN_pfuncAtaWrite;
    PVOID               ATACHAN_pvAtaWriteArg;
} PCI_ATA_CHAN;
/*********************************************************************************************************
** 函数名称: __pciAtaIoctl
** 功能描述: 控制 ATA 通道
** 输　入  : patachan      ATA 通道
**           iCmd          命令
**           pvArg         参数
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __pciAtaIoctl (ATA_CHAN  *patachan, INT  iCmd, PVOID  pvArg)
{
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __pciAtaIoOutByte
** 功能描述: 输出一个字节
** 输　入  : patachan      ATA 通道
**           ulIoAddr      IO 地址
**           uiData        数据
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __pciAtaIoOutByte (ATA_CHAN  *patachan, ULONG  ulIoAddr, UINT  uiData)
{
    out8((UINT8)uiData, ulIoAddr);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __pciAtaIoInByte
** 功能描述: 输入一个字节
** 输　入  : patachan      ATA 通道
**           ulIoAddr      IO 地址
** 输　出  : 数据
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __pciAtaIoInByte (ATA_CHAN  *patachan,  ULONG  ulIoAddr)
{
    return  (in8(ulIoAddr));
}
/*********************************************************************************************************
** 函数名称: __pciAtaIoOutWordString
** 功能描述: 输出一个字串
** 输　入  : patachan      ATA 通道
**           ulIoAddr      IO 地址
**           psBuff        数据缓冲
**           iWord         字数
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __pciAtaIoOutWordString (ATA_CHAN  *patachan, ULONG  ulIoAddr, INT16  *psBuff, INT  iWord)
{
    outs16(ulIoAddr, psBuff, iWord);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __pciAtaIoInWordString
** 功能描述: 输入一个字串
** 输　入  : patachan      ATA 通道
**           ulIoAddr      IO 地址
**           psBuff        数据缓冲
**           iWord         字数
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __pciAtaIoInWordString (ATA_CHAN  *patachan,ULONG  ulIoAddr, INT16  *psBuff, INT  iWord)
{
    ins16(ulIoAddr, psBuff, iWord);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ideSysReset
** 功能描述: 复位 ATA 通道
** 输　入  : patachan      ATA 通道
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __pciAtaSysReset (ATA_CHAN  *patachan, INT  iDrive)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ideCallbackInstall
** 功能描述: 安装回调函数
** 输　入  : patachan      ATA 通道
**           iCallbackType 回调类型
**           callback      回调函数
**           pvCallbackArg 回调参数
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __pciAtaCallbackInstall (ATA_CHAN     *patachan,
                                     INT           iCallbackType,
                                     ATA_CALLBACK  callback,
                                     PVOID         pvCallbackArg)
{
    PCI_ATA_CHAN  *pciatachan = (PCI_ATA_CHAN *)patachan;

    switch (iCallbackType) {
    
    case ATA_CALLBACK_CHECK_DEV:
        pciatachan->ATACHAN_pfuncAtaDevChk = callback;
        pciatachan->ATACHAN_pvAtaDevChkArg = pvCallbackArg;
        break;


    case ATA_CALLBACK_WRITE_DATA:
        pciatachan->ATACHAN_pfuncAtaWrite = callback;
        pciatachan->ATACHAN_pvAtaWriteArg = pvCallbackArg;
        break;

    default:
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
  ATA 驱动函数
*********************************************************************************************************/
static ATA_DRV_FUNCS  pciStorageAtaDrvFuncs = {
    __pciAtaIoctl,
    __pciAtaIoOutByte,
    __pciAtaIoInByte,
    __pciAtaIoOutWordString,
    __pciAtaIoInWordString,
    __pciAtaSysReset,
    __pciAtaCallbackInstall
};
/*********************************************************************************************************
** 函数名称: pciStorageAtaCreateChan
** 功能描述: IDE 硬盘创建一个 ATA 通道
** 输　入  : NONE
** 输　出  : ATA 通道
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ATA_CHAN  *pciStorageAtaCreateChan (VOID)
{
    PCI_ATA_CHAN  *pciatachan = (PCI_ATA_CHAN *)__SHEAP_ALLOC(sizeof(PCI_ATA_CHAN));

    if (pciatachan) {
        pciatachan->ATACHAN_pDrvFuncs = &pciStorageAtaDrvFuncs;
    }
    
    return  ((ATA_CHAN *)pciatachan);
}
/*********************************************************************************************************
  ATAPI registers 定义
*********************************************************************************************************/
#define ATA_DATA(base0)         (base0 + 0)                             /* (RW) data register (16 bits) */
#define ATA_ERROR(base0)        (base0 + 1)                             /* (R)  error register          */
#define ATA_FEATURE(base0)      (base0 + 1)                             /* (W)  feature/precompensation */
#define ATA_SECCNT(base0)       (base0 + 2)                             /* (RW) sector count for ATA.   */
                                                                        /* R-Interrupt reason W-unused  */
#define ATA_SECTOR(base0)       (base0 + 3)                             /* (RW) first sector number.    */
                                                                        /* ATAPI- Reserved for SAMTAG   */
#define ATA_CYL_LO(base0)       (base0 + 4)                             /* (RW) cylinder low byte       */
                                                                        /* ATAPI - Byte count Low       */
#define ATA_CYL_HI(base0)       (base0 + 5)                             /* (RW) cylinder high byte      */
                                                                        /* ATAPI - Byte count High      */
#define ATA_SDH(base0)          (base0 + 6)                             /* (RW) sector size/drive/head  */
                                                                        /* ATAPI - drive select         */
#define ATA_COMMAND(base0)      (base0 + 7)                             /* (W)  command register        */
#define ATA_STATUS(base0)       (base0 + 7)                             /* (R)  immediate status        */

#define ATA_A_STATUS(ctl0)      (ctl0 + 2)                              /* (R)  alternate status        */
#define ATA_D_CONTROL(ctl0)     (ctl0 + 2)                              /* (W)  disk controller control */
#define ATA_D_ADDRESS(ctl0)     (ctl0 + 3)                              /* (R)  disk controller address */
/*********************************************************************************************************
** 函数名称: pciStorageAtaCreateDrv
** 功能描述: IDE 硬盘创建一个 ATA 设备
** 输　入  : ulIoBase      IO 基地址
**           ulCtrlBase    控制基地址
** 输　出  : 是否创建成功
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  pciStorageAtaCreateDrv (addr_t  ulIoBase, addr_t  ulCtrlBase)
{
    ATA_CHAN       *patachan = pciStorageAtaCreateChan();
    ATA_CHAN_PARAM  param;
    INT             iError;
    static INT      iCtrlNum = 0;
    
    if (!patachan) {
        return  (PX_ERROR);
    }
    
    param.ATACP_iCtrlNum         = iCtrlNum;                            /*  控制器号                    */
    param.ATACP_iDrives          = 2;                                   /*  设备个数 MASTER & SLAVE     */
    param.ATACP_iBytesPerSector  = 512;                                 /*  每扇区字节数                */
    param.ATACP_iConfigType      = ATA_PIO_MULTI | ATA_BITS_16;         /*  配置标志                    */
    param.ATACP_bIntEnable       = LW_FALSE;                            /*  系统中断使能标志            */
    param.ATACP_ulSyncSemTimeout = 2 * LW_OPTION_WAIT_A_SECOND;         /*  同步等待超时时间(系统时钟)  */
    param.ATACP_bPreadBeSwap     = LW_FALSE;                            /*  大端 CPU Pread 不需要翻转   */
    
    param.ATACP_atareg.ATAREG_ulData     = ATA_DATA(ulIoBase);
    param.ATACP_atareg.ATAREG_ulError    = ATA_ERROR(ulIoBase);
    param.ATACP_atareg.ATAREG_ulFeature  = ATA_FEATURE(ulIoBase);
    param.ATACP_atareg.ATAREG_ulSeccnt   = ATA_SECCNT(ulIoBase);
    param.ATACP_atareg.ATAREG_ulSector   = ATA_SECTOR(ulIoBase);
    param.ATACP_atareg.ATAREG_ulCylLo    = ATA_CYL_LO(ulIoBase);
    param.ATACP_atareg.ATAREG_ulCylHi    = ATA_CYL_HI(ulIoBase);
    param.ATACP_atareg.ATAREG_ulSdh      = ATA_SDH(ulIoBase);
    param.ATACP_atareg.ATAREG_ulCommand  = ATA_COMMAND(ulIoBase);
    param.ATACP_atareg.ATAREG_ulStatus   = ATA_STATUS(ulIoBase);
    param.ATACP_atareg.ATAREG_ulAStatus  = ATA_A_STATUS(ulCtrlBase);
    param.ATACP_atareg.ATAREG_ulDControl = ATA_D_CONTROL(ulCtrlBase);
    
    iError = API_AtaDrv(patachan, &param);
    if (iError < ERROR_NONE) {
        return  (PX_ERROR);
    }
    
    iCtrlNum++;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
  ATA Ctrl 定义
*********************************************************************************************************/
#define ATA_PRIMARY     0
#define ATA_SECONDARY   1
/*********************************************************************************************************
  ATA Drive 定义
*********************************************************************************************************/
#define ATA_MASTER      0
#define ATA_SLAVE       1
/*********************************************************************************************************
** 函数名称: pciStorageAtaCreateDev
** 功能描述: IDE 硬盘创建一个 ATA 设备
** 输　入  : iCtrl             控制器
**           iDrive            Drive
** 输　出  : 是否创建成功
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  pciStorageAtaCreateDev (INT  iCtrl, INT  iDrive)
{
    PLW_BLK_DEV     pblkdev;
    
    pblkdev = API_AtaDevCreate(iCtrl, iDrive, 0, 0);
    if (!pblkdev) {
        return  (PX_ERROR);
    }
    
    API_OemDiskMount("/media/hdd", pblkdev, LW_NULL, 512 * LW_CFG_KB_SIZE, 32);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pciStorageAtaDevIdTblGet
** 功能描述: 获取设备列表
** 输　入  : phPciDevId     设备 ID 列表句柄缓冲区
**           puiSzie        设备列表大小
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  pciStorageAtaDevIdTblGet (PCI_DEV_ID_HANDLE *phPciDevId, UINT32 *puiSzie)
{
    if ((!phPciDevId) ||
        (!puiSzie)) {
        return  (PX_ERROR);
    }

    *phPciDevId = (PCI_DEV_ID_HANDLE)pciStorageAtaIdTbl;
    *puiSzie    = sizeof(pciStorageAtaIdTbl) / sizeof(PCI_DEV_ID_CB);

    return (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pciStorageAtaDevRemove
** 功能描述: 移除 ATA 设备
** 输　入  : hDevHandle         PCI 设备控制块句柄
**           hIdTable           设备 ID 列表
** 输　出  : 控制器句柄
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
static VOID  pciStorageAtaDevRemove (PCI_DEV_HANDLE hHandle)
{
}
/*********************************************************************************************************
** 函数名称: pciStorageAtaDevProbe
** 功能描述: ATA 驱动探测到设备
** 输　入  : hDevHandle         PCI 设备控制块句柄
**           hIdEntry           匹配成功的设备 ID 条目(来自于设备 ID 表)
** 输　出  : 控制器句柄
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
static INT  pciStorageAtaDevProbe (PCI_DEV_HANDLE hPciDevHandle, const PCI_DEV_ID_HANDLE hIdEntry)
{
    PCI_ATA_PORT           *pataport;
    UINT                    uiChannel;
    addr_t                  ulBase, ulCtrl;                             /*  起始地址                    */
    PCI_RESOURCE_HANDLE     hResource;

    if ((!hPciDevHandle) || (!hIdEntry)) {                              /*  设备参数无效                */
        _ErrorHandle(EINVAL);                                           /*  标记错误                    */
        return  (PX_ERROR);                                             /*  错误返回                    */
    }
    
    if ((!hPciDevHandle) || (!hIdEntry)) {                              /*  设备参数无效                */
        _ErrorHandle(EINVAL);                                           /*  标记错误                    */
        return  (PX_ERROR);                                             /*  错误返回                    */
    }
    
    pataport = &pciAtaPort[hIdEntry->PCIDEVID_ulData];

    /*
     *  更新设备参数, 设备驱动版本信息及当前类型设备的索引等
     */
    hPciDevHandle->PCIDEV_uiDevVersion = ATA_PCI_DRV_VER_NUM;           /*  当前设备驱动版本号          */
    hPciDevHandle->PCIDEV_uiUnitNumber = 0;                             /*  本类设备索引                */
    
    for (uiChannel = 0; uiChannel < pataport->ATAPORT_uiPort; uiChannel++) {
        if (pataport->ATAPORT_uiFlag & ATAPORT_IO_PORT) {
            if (uiChannel) {
                pciStorageAtaCreateDrv(0x170, 0x374);
            
            } else {
                pciStorageAtaCreateDrv(0x1f0, 0x3f4);
            }
        
        } else {
            hResource = API_PciDevResourceGet(hPciDevHandle, PCI_IORESOURCE_IO, 2 * uiChannel);
            ulBase = (addr_t)(PCI_RESOURCE_START(hResource));

            hResource = API_PciDevResourceGet(hPciDevHandle, PCI_IORESOURCE_IO, 2 * uiChannel + 1);
            ulCtrl = (addr_t)(PCI_RESOURCE_START(hResource));

            if (pciStorageAtaCreateDrv(ulBase, ulCtrl) < 0) {
                continue;
            }
        }
        
        if (uiChannel) {
            pciStorageAtaCreateDev(ATA_SECONDARY, ATA_MASTER);
            pciStorageAtaCreateDev(ATA_SECONDARY, ATA_SLAVE);
        
        } else {
            pciStorageAtaCreateDev(ATA_PRIMARY, ATA_MASTER);
            pciStorageAtaCreateDev(ATA_PRIMARY, ATA_SLAVE);
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pciStorageAtaInit
** 功能描述: 设备驱动初始化
** 输　入  : NONE
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  pciStorageAtaInit (VOID)
{
    INT                 iRet;                                           /*  操作结果                    */
    PCI_DRV_CB          tPciDrv;                                        /*  驱动控制器用于注册驱动      */
    PCI_DRV_HANDLE      hPciDrv = &tPciDrv;                             /*  驱动控制块句柄              */

    lib_bzero(hPciDrv, sizeof(PCI_DRV_CB));                             /*  复位驱动控制块参数          */
    iRet = pciStorageAtaDevIdTblGet(&hPciDrv->PCIDRV_hDrvIdTable, &hPciDrv->PCIDRV_uiDrvIdTableSize);
    if (iRet != ERROR_NONE) {                                           /*  获取设备 ID 表失败          */
        return  (PX_ERROR);
    }
                                                                        /*  设置驱动名称                */
    lib_strlcpy(& hPciDrv->PCIDRV_cDrvName[0], ATA_PCI_DRV_NAME, PCI_DRV_NAME_MAX);
    hPciDrv->PCIDRV_pvPriv         = LW_NULL;                           /*  设备驱动的私有数据          */
    hPciDrv->PCIDRV_hDrvErrHandler = LW_NULL;                           /*  驱动错误处理                */
    hPciDrv->PCIDRV_pfuncDevProbe  = pciStorageAtaDevProbe;             /*  设备探测函数, 不能为空      */
    hPciDrv->PCIDRV_pfuncDevRemove = pciStorageAtaDevRemove;            /*  驱动移除函数, 不能为空      */

    iRet = API_PciDrvRegister(hPciDrv);                                 /*  注册 PCI 设备驱动           */
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PCI_EN > 0) &&      */
                                                                        /*  (LW_CFG_ATA_EN > 0)         */
                                                                        /*  (LW_CFG_DRV_ATA_IDE > 0)    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
