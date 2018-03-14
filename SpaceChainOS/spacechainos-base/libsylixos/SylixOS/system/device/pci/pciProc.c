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
** 文   件   名: pciProc.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 09 月 28 日
**
** 描        述: PCI 总线驱动模型 proc 文件系统信息.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/include/fs_fs.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PCI_EN > 0) && (LW_CFG_PROCFS_EN > 0)
#include "pciDev.h"
#include "pciLib.h"
#include "pciDb.h"
/*********************************************************************************************************
  PCI 主控器
*********************************************************************************************************/
extern  PCI_CTRL_HANDLE     *_G_hPciCtrlHandle;
#define PCI_CTRL             _G_hPciCtrlHandle
/*********************************************************************************************************
  pci proc 文件函数声明
*********************************************************************************************************/
static ssize_t  __procFsPciRead(PLW_PROCFS_NODE  p_pfsn, 
                                PCHAR            pcBuffer, 
                                size_t           stMaxBytes,
                                off_t            oft);
/*********************************************************************************************************
  pci proc 文件操作函数组
*********************************************************************************************************/
static LW_PROCFS_NODE_OP    _G_pfsnoPciFuncs = {
    __procFsPciRead,        LW_NULL
};
/*********************************************************************************************************
  pci proc 文件目录树
*********************************************************************************************************/
static LW_PROCFS_NODE       _G_pfsnPci[] = 
{
    LW_PROCFS_INIT_NODE("pci", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoPciFuncs, 
                        "p",
                        0),
};
/*********************************************************************************************************
  pci proc 遍历打印参数
*********************************************************************************************************/
typedef struct {
    PCHAR       PPA_pcFileBuffer;
    size_t      PPA_stTotalSize;
    size_t     *PPA_pstOft;
} PCI_PRINT_ARG;
/*********************************************************************************************************
** 函数名称: __procFsPciPrintDev
** 功能描述: 打印一个 PCI 设备
** 输　入  : 打印一个 PCI 事例
** 输　入  : iBus          总线号
**           iSlot         插槽
**           iFunc         功能号
**           p_pciparg     打印参数
**           p_pcidhdr     PCI 设备头
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __procFsPciPrintDev (INT iBus, INT iSlot, INT iFunc, 
                                  PCI_PRINT_ARG *p_pciparg, PCI_DEV_HDR *p_pcidhdr)
{   INT                 i;
    size_t             *pstOft;
    CHAR                cDevBasicInfo[128];
    PCI_DEV_HANDLE      hDevHandle;
    PCI_RESOURCE_HANDLE hRes;

    hDevHandle = API_PciDevHandleGet(iBus, iSlot, iFunc);

    API_PciDbGetClassStr(p_pcidhdr->PCID_ucClassCode,
                         p_pcidhdr->PCID_ucSubClass,
                         p_pcidhdr->PCID_ucProgIf,
                         cDevBasicInfo, sizeof(cDevBasicInfo));
    
    *p_pciparg->PPA_pstOft = bnprintf(p_pciparg->PPA_pcFileBuffer, 
                                      p_pciparg->PPA_stTotalSize, 
                                      *p_pciparg->PPA_pstOft, 
                                      "  %s\n"
                                      "    IRQ Line-%d Pin-%d\n"
                                      "    Latency=%d Min Gnt=%d Max lat=%d\n"
                                      "    Base0 %08x Base1 %08x Base2 %08x\n"
                                      "    Base3 %08x Base4 %08x Base5 %08x\n"
                                      "    Rom %x SubVendorID %x SubSystemID %x\n",
                                      cDevBasicInfo,
                                      p_pcidhdr->PCID_ucIntLine,
                                      p_pcidhdr->PCID_ucIntPin,
                                      p_pcidhdr->PCID_ucLatency,
                                      p_pcidhdr->PCID_ucMinGrant,
                                      p_pcidhdr->PCID_ucMaxLatency,
                                      p_pcidhdr->PCID_uiBase[PCI_BAR_INDEX_0],
                                      p_pcidhdr->PCID_uiBase[PCI_BAR_INDEX_1],
                                      p_pcidhdr->PCID_uiBase[PCI_BAR_INDEX_2],
                                      p_pcidhdr->PCID_uiBase[PCI_BAR_INDEX_3],
                                      p_pcidhdr->PCID_uiBase[PCI_BAR_INDEX_4],
                                      p_pcidhdr->PCID_uiBase[PCI_BAR_INDEX_5],
                                      p_pcidhdr->PCID_uiRomBase,
                                      p_pcidhdr->PCID_usSubVendorId,
                                      p_pcidhdr->PCID_usSubSystemId);

    for (i = 0; i <= PCI_ROM_RESOURCE; i++) {
        hRes = &hDevHandle->PCIDEV_tResource[i];

        if (hRes->PCIRS_stStart == hRes->PCIRS_stEnd) {
            continue;
        }

        pstOft = p_pciparg->PPA_pstOft;
        *pstOft = bnprintf(p_pciparg->PPA_pcFileBuffer,
                           p_pciparg->PPA_stTotalSize,
                          *pstOft,
                           "    Region %d: %s at %llx [Size %llu%s] [flags %08x] %s%s%s",
                           i,
                           (hRes->PCIRS_ulFlags & PCI_IORESOURCE_IO) ? "I/O ports" :
                           (hRes->PCIRS_ulFlags & PCI_IORESOURCE_READONLY) ? "Expansion ROM" : "Memory",
                           hRes->PCIRS_stStart,
                           API_PciSizeNumGet(hRes->PCIRS_stEnd - hRes->PCIRS_stStart + 1),
                           API_PciSizeNameGet(hRes->PCIRS_stEnd - hRes->PCIRS_stStart + 1),
                           (UINT32)hRes->PCIRS_ulFlags,
                           (hRes->PCIRS_ulFlags & PCI_IORESOURCE_IO) ? "\n" : "",
                           (hRes->PCIRS_ulFlags & PCI_IORESOURCE_MEM_64) ? " (64-bit" :
                           (hRes->PCIRS_ulFlags & PCI_IORESOURCE_MEM) ? " (32-bit" : "",
                           (hRes->PCIRS_ulFlags & PCI_IORESOURCE_IO) ? "" :
                           (hRes->PCIRS_ulFlags & PCI_IORESOURCE_PREFETCH) ? " pre)\n" : " non-pre)\n");
        p_pciparg->PPA_pstOft = pstOft;
    }

    hRes = &hDevHandle->PCIDEV_tResource[PCI_IRQ_RESOURCE];
    if (hRes->PCIRS_ulFlags & PCI_IORESOURCE_IRQ) {
        pstOft = p_pciparg->PPA_pstOft;
        *pstOft = bnprintf(p_pciparg->PPA_pcFileBuffer,
                           p_pciparg->PPA_stTotalSize,
                          *pstOft,
                           "    Irq start %llx end %llx [flags %08x] \n",
                           hRes->PCIRS_stStart,
                           hRes->PCIRS_stEnd,
                           (UINT32)hRes->PCIRS_ulFlags);
        p_pciparg->PPA_pstOft = pstOft;
    }
}
/*********************************************************************************************************
** 函数名称: __procFsPciPrintBridge
** 功能描述: 打印一个 PCI 桥
** 输　入  : 打印一个 PCI 桥
** 输　入  : iBus          总线号
**           iSlot         插槽
**           iFunc         功能号
**           p_pciparg     打印参数
**           p_pcibhdr     PCI Bridge header
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __procFsPciPrintBridge (INT iBus, INT iSlot, INT iFunc, 
                                     PCI_PRINT_ARG *p_pciparg, PCI_BRG_HDR *p_pcibhdr)
{

    CHAR    cBrgBasicInfo[128];
    
    API_PciDbGetClassStr(p_pcibhdr->PCIB_ucClassCode,
                         p_pcibhdr->PCIB_ucSubClass,
                         p_pcibhdr->PCIB_ucProgIf,
                         cBrgBasicInfo, sizeof(cBrgBasicInfo));
    
    *p_pciparg->PPA_pstOft = bnprintf(p_pciparg->PPA_pcFileBuffer, 
                                      p_pciparg->PPA_stTotalSize, 
                                      *p_pciparg->PPA_pstOft, 
                                      "  %s\n"
                                      "    IRQ Line-%d Pin-%d\n"
                                      "    Latency=%d SecLatency=%d PriBus=%d SecBus=%d SubBus=%d\n"
                                      "    I/O behind %08x Memory behind %08x\n"
                                      "    I/O %x-%x Mem %x-%x Rom %x\n",
                                      cBrgBasicInfo,
                                      p_pcibhdr->PCIB_ucIntLine,
                                      p_pcibhdr->PCIB_ucIntPin,
                                      p_pcibhdr->PCIB_ucLatency,
                                      p_pcibhdr->PCIB_ucSecLatency,
                                      p_pcibhdr->PCIB_ucPriBus,
                                      p_pcibhdr->PCIB_ucSecBus,
                                      p_pcibhdr->PCIB_ucSubBus,
                                      p_pcibhdr->PCIB_uiBase[PCI_BAR_INDEX_0],
                                      p_pcibhdr->PCIB_uiBase[PCI_BAR_INDEX_1],
                                      p_pcibhdr->PCIB_ucIoBase,
                                      p_pcibhdr->PCIB_ucIoBase + p_pcibhdr->PCIB_ucIoLimit,
                                      p_pcibhdr->PCIB_usMemBase,
                                      p_pcibhdr->PCIB_usMemBase + p_pcibhdr->PCIB_usMemLimit,
                                      p_pcibhdr->PCIB_uiRomBase);
}
/*********************************************************************************************************
** 函数名称: __procFsPciCardbus
** 功能描述: 打印一个 PCI cardbus bridge
** 输　入  : 打印一个 PCI cardbus bridge
** 输　入  : iBus          总线号
**           iSlot         插槽
**           iFunc         功能号
**           p_pciparg     打印参数
**           p_pcibhdr     PCI Cardbus Bridge header
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __procFsPciPrintCardbus (INT iBus, INT iSlot, INT iFunc, 
                                      PCI_PRINT_ARG *p_pciparg, PCI_CBUS_HDR *p_pcicbhdr)
{
    CHAR    cBrgBasicInfo[128];
    
    API_PciDbGetClassStr(p_pcicbhdr->PCICB_ucClassCode,
                         p_pcicbhdr->PCICB_ucSubClass,
                         p_pcicbhdr->PCICB_ucProgIf,
                         cBrgBasicInfo, sizeof(cBrgBasicInfo));
    
    *p_pciparg->PPA_pstOft = bnprintf(p_pciparg->PPA_pcFileBuffer, 
                                      p_pciparg->PPA_stTotalSize, 
                                      *p_pciparg->PPA_pstOft, 
                                      "  %s\n"
                                      "    IRQ Line-%d Pin-%d\n"
                                      "    Latency=%d SecLatency=%d PriBus=%d SecBus=%d SubBus=%d\n"
                                      "    Base0 %08x\n"
                                      "    I/O0 %08x-%08x I/O1 %08x-%08x\n"
                                      "    Mem0 %08x-%08x Mem1 %08x-%08x\n"
                                      "    SubVendorID %x SubSystemID %x\n",
                                      cBrgBasicInfo,
                                      p_pcicbhdr->PCICB_ucIntLine,
                                      p_pcicbhdr->PCICB_ucIntPin,
                                      p_pcicbhdr->PCICB_ucLatency,
                                      p_pcicbhdr->PCICB_ucSecLatency,
                                      p_pcicbhdr->PCICB_ucPriBus,
                                      p_pcicbhdr->PCICB_ucSecBus,
                                      p_pcicbhdr->PCICB_ucSubBus,
                                      p_pcicbhdr->PCICB_uiBase[PCI_BAR_INDEX_0],
                                      p_pcicbhdr->PCICB_uiIoBase0,
                                      p_pcicbhdr->PCICB_uiIoBase0 + p_pcicbhdr->PCICB_uiIoLimit0,
                                      p_pcicbhdr->PCICB_uiIoBase1,
                                      p_pcicbhdr->PCICB_uiIoBase1 + p_pcicbhdr->PCICB_uiIoLimit1,
                                      p_pcicbhdr->PCICB_uiMemBase0,
                                      p_pcicbhdr->PCICB_uiMemBase0 + p_pcicbhdr->PCICB_uiMemLimit0,
                                      p_pcicbhdr->PCICB_uiMemBase1,
                                      p_pcicbhdr->PCICB_uiMemBase1 + p_pcicbhdr->PCICB_uiMemLimit1,
                                      p_pcicbhdr->PCICB_usSubVendorId,
                                      p_pcicbhdr->PCICB_usSubSystemId);
                                      
}
/*********************************************************************************************************
** 函数名称: __procFsPciPrint
** 功能描述: 打印一个 PCI 事例
** 输　入  : iBus          总线号
**           iSlot         插槽
**           iFunc         功能号
**           p_pciparg     打印参数
** 输　出  : ERROR_NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __procFsPciPrint (INT iBus, INT iSlot, INT iFunc, PCI_PRINT_ARG *p_pciparg)
{
    PCI_HDR     pcihdr;
    
    API_PciGetHeader(iBus, iSlot, iFunc, &pcihdr);

    *p_pciparg->PPA_pstOft = bnprintf(p_pciparg->PPA_pcFileBuffer, 
                                      p_pciparg->PPA_stTotalSize, 
                                      *p_pciparg->PPA_pstOft, 
                                      "Bus%3d Slot%3d Function%2d VendorID %x DevieceID %x:\n",
                                      iBus, iSlot, iFunc, 
                                      pcihdr.PCIH_pcidHdr.PCID_usVendorId,
                                      pcihdr.PCIH_pcidHdr.PCID_usDeviceId);
    
    switch (pcihdr.PCIH_ucType & PCI_HEADER_TYPE_MASK) {
    
    case PCI_HEADER_TYPE_NORMAL:
        __procFsPciPrintDev(iBus, iSlot, iFunc, p_pciparg, &pcihdr.PCIH_pcidHdr);
        break;
        
    case PCI_HEADER_TYPE_BRIDGE:
        __procFsPciPrintBridge(iBus, iSlot, iFunc, p_pciparg, &pcihdr.PCIH_pcibHdr);
        break;
    
    case PCI_HEADER_TYPE_CARDBUS:
        __procFsPciPrintCardbus(iBus, iSlot, iFunc, p_pciparg, &pcihdr.PCIH_pcicbHdr);
        break;
    
    default:
        break;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __procFsPciGetCnt
** 功能描述: 获得 PCI 设备总数
** 输　入  : iBus          总线号
**           iSlot         插槽
**           iFunc         功能号
**           pstBufferSize 缓冲区大小
** 输　出  : ERROR_NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __procFsPciGetCnt (INT iBus, INT iSlot, INT iFunc, size_t  *pstNeedBufferSize)
{
    (VOID)iBus;
    (VOID)iSlot;
    (VOID)iFunc;
    
    *pstNeedBufferSize += 512;                                          /*  每个 PCI 事例占用 512 字节  */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __procFsPciRead
** 功能描述: procfs 读一个读取网络 pci 文件
** 输　入  : p_pfsn        节点控制块
**           pcBuffer      缓冲区
**           stMaxBytes    缓冲区大小
**           oft           文件指针
** 输　出  : 实际读取的数目
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __procFsPciRead (PLW_PROCFS_NODE  p_pfsn, 
                                 PCHAR            pcBuffer, 
                                 size_t           stMaxBytes,
                                 off_t            oft)
{
    const CHAR      cPciInfoHdr[] = "PCI info:\n";
          PCHAR     pcFileBuffer;
          size_t    stRealSize;                                         /*  实际的文件内容大小          */
          size_t    stCopeBytes;
    
    if (PCI_CTRL == LW_NULL) {
        return  (0);
    }
    
    /*
     *  由于预置内存大小为 0 , 所以打开后第一次读取需要手动开辟内存.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {                                      /*  还没有分配内存              */
        size_t          stNeedBufferSize = 0;
        PCI_PRINT_ARG   pciparg;
        
        API_PciTraversal(__procFsPciGetCnt, &stNeedBufferSize, PCI_MAX_BUS - 1);
        
        stNeedBufferSize += sizeof(cPciInfoHdr);
        
        if (API_ProcFsAllocNodeBuffer(p_pfsn, stNeedBufferSize)) {
            _ErrorHandle(ENOMEM);
            return  (0);
        }
        pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);             /*  重新获得文件缓冲区地址      */
        
        pciparg.PPA_pcFileBuffer = pcFileBuffer;
        pciparg.PPA_stTotalSize  = stNeedBufferSize;
        pciparg.PPA_pstOft       = &stRealSize;
        
        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, 0, cPciInfoHdr);
        
        API_PciTraversal(__procFsPciPrint, &pciparg, PCI_MAX_BUS - 1);

        API_ProcFsNodeSetRealFileSize(p_pfsn, stRealSize);
    } else {
        stRealSize = API_ProcFsNodeGetRealFileSize(p_pfsn);
    }
    if (oft >= stRealSize) {
        _ErrorHandle(ENOSPC);
        return  (0);
    }
    
    stCopeBytes = __MIN(stMaxBytes, (size_t)(stRealSize - oft));        /*  计算实际拷贝的字节数        */
    lib_memcpy(pcBuffer, (CPVOID)(pcFileBuffer + oft), (UINT)stCopeBytes);
    
    return  ((ssize_t)stCopeBytes);
}
/*********************************************************************************************************
** 函数名称: __procFsPciInit
** 功能描述: procfs 初始化 pci proc 文件
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __procFsPciInit (VOID)
{
    API_PciDbInit();
    
    API_ProcFsMakeNode(&_G_pfsnPci[0],  "/");
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PCI_EN > 0)         */
                                                                        /*  (LW_CFG_PROCFS_EN > 0)      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
