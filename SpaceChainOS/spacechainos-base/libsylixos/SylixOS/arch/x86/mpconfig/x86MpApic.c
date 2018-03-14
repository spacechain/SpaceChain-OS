/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: x86MpApic.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 07 月 29 日
**
** 描        述: x86 体系构架 MP configuration table.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_STDIO
#include "SylixOS.h"
#include "arch/x86/acpi/x86AcpiLib.h"
#include "x86MpApic.h"
/*********************************************************************************************************
  内部变量定义
*********************************************************************************************************/
static INT8                 *_G_pcX86EbdaStart      = LW_NULL;          /*  Extended BIOS Data Area     */
static INT8                 *_G_pcX86EbdaEnd        = LW_NULL;

static INT8                 *_G_pcX86BiosRomStart   = LW_NULL;          /*  BIOS ROM space              */
static INT8                 *_G_pcX86BiosRomEnd     = LW_NULL;

static BOOL                  _G_bX86MpApicDrvInited = LW_FALSE;         /*  X86_MP_APIC_DATA 是否初始化 */
static X86_MP_APIC_DATA     *_G_pX86MpApicData      = LW_NULL;          /*  X86_MP_APIC_DATA 指针       */
/*********************************************************************************************************
  全局变量定义
*********************************************************************************************************/
UINT32              _G_uiX86CpuCount            = 0;                    /*  逻辑处理器数目              */

UINT32              _G_uiX86MpApicIoIntNr       = 0;                    /*  IO 中断数目                 */
UINT32              _G_uiX86MpApicLoIntNr       = 0;                    /*  Local 中断数目              */
UINT8               _G_ucX86MpApicIoBaseId      = 0;                    /*  Base IOAPIC Id              */

X86_MP_INTERRUPT   *_G_pX86MpApicInterruptTable = LW_NULL;              /*  中断表                      */

UINT8               _G_aucX86CpuIndexTable[X86_CPUID_MAX_NUM_CPUS];     /*  Local APIC ID->逻辑处理器 ID*/
UINT                _G_uiX86BaseCpuPhysIndex    = 0;                    /*  Base CPU Phy index          */

UINT                _G_uiX86MpApicBootOpt       = ACPI_MP_STRUCT;       /*  启动选项                    */
addr_t              _G_ulX86MpApicDataAddr      = MPAPIC_DATA_START;    /*  MPAPIC 数据开始地址         */
VOID              (*_G_pfuncX86MpApicDataGet)(X86_MP_APIC_DATA *) = LW_NULL;
/*********************************************************************************************************
  内部函数前置声明
*********************************************************************************************************/
static VOID   x86MpApicDataLocInfoSet(X86_MP_APIC_DATA   *pMpApicData);
static INT    x86MpApicConfigTableInit(X86_MP_APIC_DATA  *pMpApicData);
static INT    x86MpApicConfigTableCheck(X86_MP_HEADER    *pHeader);
static INT8  *x86MpApicScan(INT8  *pcMatch, INT8  *pcStart, INT8  *pcEnd);
static PVOID  x86MpApicIrqAppend(INT  iIndex);
/*********************************************************************************************************
  外部函数声明
*********************************************************************************************************/
extern VOID   x86CpuTopologyInit(X86_MP_APIC_DATA  *pMpApicData);
/*********************************************************************************************************
 ** 函数名称: x86MpApicInstInit
 ** 功能描述: 初始化 MPAPIC 实例
 ** 输　入  : NONE
 ** 输　出  : ERROR CODE
 ** 全局变量:
 ** 调用模块:
*********************************************************************************************************/
INT  x86MpApicInstInit (VOID)
{
    UINT8  *pucMpApicLogicalTable;
    INT     i;

__begin:
    _G_pX86MpApicData = (X86_MP_APIC_DATA *)_G_ulX86MpApicDataAddr;
#if LW_CFG_CPU_WORD_LENGHT == 32
    _G_pX86MpApicData->MPAPIC_uiDataLoc = _G_ulX86MpApicDataAddr;
#else
    _G_pX86MpApicData->MPAPIC_uiDataLoc = 0;
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT == 32*/
    _G_pX86MpApicData->MPAPIC_uiBootOpt = _G_uiX86MpApicBootOpt;

    if (_G_pcX86EbdaStart == 0) {
        _G_pcX86EbdaStart = (INT8 *)EBDA_START;
        _G_pcX86EbdaEnd   = _G_pcX86EbdaStart + (EBDA_SIZE - 1);
    }

    if (_G_pcX86BiosRomStart == 0) {
        _G_pcX86BiosRomStart = (INT8 *)BIOS_ROM_START;
        _G_pcX86BiosRomEnd   = _G_pcX86BiosRomStart + (BIOS_ROM_SIZE - 1);
    }

    switch (_G_uiX86MpApicBootOpt) {

    case MP_MP_STRUCT:
        x86MpApicConfigTableInit(_G_pX86MpApicData);
        break;

    case ACPI_MP_STRUCT:
    {
        INT  iError;

        lib_bzero((CHAR *)_G_ulX86MpApicDataAddr, MPAPIC_DATA_MAX_SIZE);
#if LW_CFG_CPU_WORD_LENGHT == 32
        _G_pX86MpApicData->MPAPIC_uiDataLoc  = _G_ulX86MpApicDataAddr;
#else
        _G_pX86MpApicData->MPAPIC_uiDataLoc  = 0;
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT == 32*/
        _G_pX86MpApicData->MPAPIC_uiDataSize = sizeof(X86_MP_APIC_DATA);
        _G_pX86MpApicData->MPAPIC_uiBootOpt  = _G_uiX86MpApicBootOpt;

        /*
         * 查找 ACPI 的表并映射它们到内存
         */
        iError = acpiTableInit();
        if (iError != ERROR_NONE) {
            _G_uiX86MpApicBootOpt = MP_MP_STRUCT;                       /*  非 ACPI 计算机, 用 MP 方式  */
            goto    __begin;                                            /*  重新初始化                  */
        }

        /*
         * 初始化 ACPI 配置组件
         * MPAPIC_DATA 后面是 ACPI 堆
         */
        acpiConfigInit((PCHAR)(_G_ulX86MpApicDataAddr + MPAPIC_DATA_MAX_SIZE), ACPI_HEAP_SIZE);

        /*
         * 早期初始化 ACPI 库
         */
        iError = acpiLibInit(LW_TRUE);
        if (iError != ERROR_NONE) {
            _G_uiX86MpApicBootOpt = MP_MP_STRUCT;
            goto    __begin;
        }

        /*
         * 设置 IRQ 信息追加函数
         */
        _G_pfuncAcpiIrqAppend = x86MpApicIrqAppend;

        /*
         * 早期扫描 ACPI 设备,
         * 允许 PCI 访问(因为 ACPI SylixOS 移植的 PCI 函数不依赖于 BSP 的 PCI 驱动),
         * 将填充完整的中断路由表和总线拓扑, 后面无须再调用一次 acpiLibDevScan
         */
        iError = acpiLibDevScan(LW_TRUE,
                                LW_TRUE,
                                (CHAR *)_G_ulX86MpApicDataAddr,
                                MPAPIC_DATA_MAX_SIZE);
        if (iError != ERROR_NONE) {
            _G_uiX86MpApicBootOpt = MP_MP_STRUCT;
            acpiLibDisable();
            goto    __begin;
        }

        /*
         * ACPI 库关闭
         * 系统初始化完成后，会重新初始化 ACPI，安装 ACPI 的中断服务程序和事件通知函数
         */
        acpiLibDisable();
    }
        break;

    case USR_MP_STRUCT:
        _G_pfuncX86MpApicDataGet(_G_pX86MpApicData);
        break;

    case NO_MP_STRUCT:
        break;

    default:
        x86MpApicConfigTableInit(_G_pX86MpApicData);
        break;
    }

    /*
     * Number of IO interrupts
     */
    _G_uiX86MpApicIoIntNr = _G_pX86MpApicData->MPAPIC_uiIoIntNr;
    if (_G_uiX86MpApicIoIntNr == 0) {
        return  (PX_ERROR);
    }

    /*
     * Number of Local interrupts
     */
    _G_uiX86MpApicLoIntNr = _G_pX86MpApicData->MPAPIC_uiLoIntNr;
    if (_G_uiX86MpApicLoIntNr == 0) {
        return  (PX_ERROR);
    }

    _G_pX86MpApicInterruptTable = (X86_MP_INTERRUPT *)X86_MPAPIC_PHYS_TO_VIRT(_G_pX86MpApicData, MPAPIC_uiItLoc);

    /*
     * Init proper number of cores
     */
    _G_uiX86CpuCount = (UINT)((LW_CFG_MAX_PROCESSORS < _G_pX86MpApicData->MPAPIC_uiCpuCount) ?
                               LW_CFG_MAX_PROCESSORS : _G_pX86MpApicData->MPAPIC_uiCpuCount);

    lib_bcopy((CHAR *)_G_pX86MpApicData->MPAPIC_aucCpuIndexTable,
              (CHAR *)_G_aucX86CpuIndexTable,
              sizeof(_G_pX86MpApicData->MPAPIC_aucCpuIndexTable));

    /*
     * Set the Base CPU Phy index
     */
    _G_uiX86BaseCpuPhysIndex = x86CpuPhysIndexGet();

    /*
     * Establish IO APIC Base Id
     */
    pucMpApicLogicalTable  = (UINT8 *)X86_MPAPIC_PHYS_TO_VIRT(_G_pX86MpApicData, MPAPIC_uiLtLoc);
    _G_ucX86MpApicIoBaseId = pucMpApicLogicalTable[0];
    for (i = 1; i < _G_pX86MpApicData->MPAPIC_uiIoApicNr; i++) {
        if (_G_ucX86MpApicIoBaseId > pucMpApicLogicalTable[i]) {
            _G_ucX86MpApicIoBaseId = pucMpApicLogicalTable[i];
        }
    }

    /*
     * Init CPU Topology
     */
    x86CpuTopologyInit(_G_pX86MpApicData);

    _G_bX86MpApicDrvInited = LW_TRUE;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
 ** 函数名称: x86MpApicDataLocInfoSet
 ** 功能描述: 初始化 X86_MP_APIC_DATA 结构内地址信息
 ** 输　入  : pMpApicData      X86_MP_APIC_DATA 结构
 ** 输　出  : NONE
 ** 全局变量:
 ** 调用模块:
*********************************************************************************************************/
static VOID  x86MpApicDataLocInfoSet (X86_MP_APIC_DATA  *pMpApicData)
{
    pMpApicData->MPAPIC_uiDataSize = sizeof(X86_MP_APIC_DATA);

    pMpApicData->MPAPIC_uiAtLoc  = pMpApicData->MPAPIC_uiDataLoc + pMpApicData->MPAPIC_uiDataSize;
    pMpApicData->MPAPIC_uiAtSize = (UINT32)(pMpApicData->MPAPIC_uiIoApicNr * sizeof(UINT32));

    pMpApicData->MPAPIC_uiLtLoc  = pMpApicData->MPAPIC_uiAtLoc + pMpApicData->MPAPIC_uiAtSize;
    pMpApicData->MPAPIC_uiLtSize = pMpApicData->MPAPIC_uiIoApicNr * sizeof(UINT8);

    pMpApicData->MPAPIC_uiItLoc  = pMpApicData->MPAPIC_uiLtLoc + pMpApicData->MPAPIC_uiLtSize;
    pMpApicData->MPAPIC_uiItSize = (UINT32)((pMpApicData->MPAPIC_uiIoIntNr
                                   + pMpApicData->MPAPIC_uiLoIntNr) * sizeof(X86_MP_INTERRUPT));

    pMpApicData->MPAPIC_uiBtLoc  = pMpApicData->MPAPIC_uiItLoc + pMpApicData->MPAPIC_uiItSize;
    pMpApicData->MPAPIC_uiBtSize = (UINT32)(pMpApicData->MPAPIC_uiBusNr * sizeof(X86_MP_BUS));

    pMpApicData->MPAPIC_uiLaLoc  = pMpApicData->MPAPIC_uiBtLoc + pMpApicData->MPAPIC_uiBtSize;
    pMpApicData->MPAPIC_uiLaSize = pMpApicData->MPAPIC_uiCpuCount * sizeof(UINT8);
}
/*********************************************************************************************************
 ** 函数名称: x86MpApicConfigTableCheck
 ** 功能描述: 检查 MP configuration table 是否有效
 ** 输　入  : pHeader          MP_HEADER 结构
 ** 输　出  : ERROR CODE
 ** 全局变量:
 ** 调用模块:
*********************************************************************************************************/
static INT  x86MpApicConfigTableCheck (X86_MP_HEADER  *pHeader)
{
    UINT8  *pucByte = (UINT8*)pHeader;
    UINT8   ucCheckSum = 0;
    INT     iError = PX_ERROR;
    UINT32  i;

    /*
     * Since 8bit checksum is not fully reliable, here we also add table length sanity check.
     */
    if (pHeader->HDR_usTableLength > 0xf000) {
        return  (iError);
    }

    for (i = 0; i < pHeader->HDR_usTableLength; i++) {
        ucCheckSum = (UINT8)(ucCheckSum + (*pucByte));
        pucByte++;
    }

    if (ucCheckSum == 0) {
        iError = ERROR_NONE;
    }

    return  (iError);
}
/*********************************************************************************************************
 ** 函数名称: x86MpApicConfigTableInit
 ** 功能描述: 初始化 X86_MP_APIC_DATA 结构
 ** 输　入  : pMpApicData      X86_MP_APIC_DATA 结构
 ** 输　出  : ERROR CODE
 ** 全局变量:
 ** 调用模块:
*********************************************************************************************************/
static INT  x86MpApicConfigTableInit (X86_MP_APIC_DATA  *pMpApicData)
{
    INT32               i, iEntrieNr, iIntNr;
    UINT32             *puiAddrTable;
    UINT8              *pucLogicalTable;
    UINT8              *pucLoApicIndexTable;
    UINT8               ucTemp1, ucTemp2;
    UINT8              *pucPtr;
    INT                 iIndex1;
    X86_MP_INTERRUPT   *pInterruptTable;
    X86_MP_BUS         *pBusTable;
    X86_MP_HEADER      *pHdr;
    X86_MP_CPU         *pCpu;
    X86_MP_IOAPIC      *pIoApic;
    X86_MP_FPS         *pFps;

    /*
     * Scan for the MP Floating Point Structure
     */
    pFps = (X86_MP_FPS *)x86MpApicScan((INT8 *)"_MP_", _G_pcX86EbdaStart, _G_pcX86EbdaEnd);
    if (pFps == LW_NULL) {
        pFps = (X86_MP_FPS *)x86MpApicScan((INT8 *)"_MP_", _G_pcX86BiosRomStart, _G_pcX86BiosRomEnd);
    }
    if (pFps == LW_NULL) {
        return  (PX_ERROR);
    }

    /*
     * Copy FPS into X86_MP_APIC_DATA structure
     */
    lib_bcopy((CHAR *)pFps, (CHAR *)&pMpApicData->MPAPIC_Fps, sizeof(X86_MP_FPS));

    /*
     * If featurebyte1 (array element 0) is non-zero, then we use
     * standard configuration return PX_ERROR and let the error handle use
     * standard addresses and Local and IO APIC ID's.
     */
    if ((pMpApicData->MPAPIC_Fps.FPS_aucFeatureByte[0] != 0) ||
        (pMpApicData->MPAPIC_Fps.FPS_uiConfigTableAddr == 0)) {
        return  (PX_ERROR);
    }

    /*
     * Get MP header pointer
     */
    if ((pMpApicData->MPAPIC_Fps.FPS_uiConfigTableAddr >= EBDA_START) &&
        (pMpApicData->MPAPIC_Fps.FPS_uiConfigTableAddr  < EBDA_END)) {
        pHdr = (X86_MP_HEADER *)((ULONG)_G_pcX86EbdaStart +
               ((ULONG)pMpApicData->MPAPIC_Fps.FPS_uiConfigTableAddr - EBDA_START));
    } else {
        pHdr = (X86_MP_HEADER *)((ULONG)_G_pcX86BiosRomStart +
               ((ULONG)pMpApicData->MPAPIC_Fps.FPS_uiConfigTableAddr - BIOS_ROM_START));
    }

    /*
     * Sanity check of the MP Configuration Table
     */
    if (x86MpApicConfigTableCheck(pHdr) != ERROR_NONE) {
        return  (PX_ERROR);
    }

    lib_bcopy((CHAR *)pHdr, (CHAR *)&pMpApicData->MPAPIC_Header, sizeof(X86_MP_HEADER));

    /*
     * Initialize counters
     */
    pMpApicData->MPAPIC_uiCpuCount = 0;
    pMpApicData->MPAPIC_uiIoApicNr = 0;
    pMpApicData->MPAPIC_uiBusNr    = 0;
    pMpApicData->MPAPIC_uiIoIntNr  = 0;
    pMpApicData->MPAPIC_uiLoIntNr  = 0;

    /*
     * First pass count the number of each important entry
     */
    pucPtr = (UINT8 *)pHdr + sizeof(X86_MP_HEADER);

    for (i = 0; i < pHdr->HDR_usEntryCount; i++) {
        switch (*pucPtr) {

        case MP_ENTRY_CPU:                                          /*  Processor Configuration Entry   */
            pCpu = (X86_MP_CPU *)pucPtr;

            if (pCpu->CPU_ucCpuFlags & CPU_EN) {
                pMpApicData->MPAPIC_uiCpuCount++;                   /*  Increment the CPU counter       */
            }

            pucPtr += sizeof(X86_MP_CPU);
            break;

        case MP_ENTRY_IOAPIC:                                       /*  IO Apic Configuration Entry     */
            pMpApicData->MPAPIC_uiIoApicNr++;                       /*  Increment the IO APIC counter   */
            pucPtr += sizeof(X86_MP_IOAPIC);
            break;

        case MP_ENTRY_BUS:
            pMpApicData->MPAPIC_uiBusNr++;
            pucPtr += sizeof(X86_MP_BUS);
            break;

        case MP_ENTRY_IOINTERRUPT:                                  /*  IO Interrupt Entry              */
            pMpApicData->MPAPIC_uiIoIntNr++;
            pucPtr += sizeof(X86_MP_INTERRUPT);
            break;

        case MP_ENTRY_LOINTERRUPT:                                  /*  Local Interrupt Entry           */
            pMpApicData->MPAPIC_uiLoIntNr++;
            pucPtr += sizeof(X86_MP_INTERRUPT);
            break;

        default:
            /*
             * "Maybe" it is 8 bytes till the next entry
             */
            pucPtr += 8;
            break;
        }
    }

    /*
     * Set the X86_MP_APIC_DATA local info
     */
    x86MpApicDataLocInfoSet(pMpApicData);

    puiAddrTable        = (UINT32           *)X86_MPAPIC_PHYS_TO_VIRT(_G_pX86MpApicData, MPAPIC_uiAtLoc);
    pucLogicalTable     = (UINT8            *)X86_MPAPIC_PHYS_TO_VIRT(_G_pX86MpApicData, MPAPIC_uiLtLoc);
    pInterruptTable     = (X86_MP_INTERRUPT *)X86_MPAPIC_PHYS_TO_VIRT(_G_pX86MpApicData, MPAPIC_uiItLoc);
    pBusTable           = (X86_MP_BUS       *)X86_MPAPIC_PHYS_TO_VIRT(_G_pX86MpApicData, MPAPIC_uiBtLoc);
    pucLoApicIndexTable = (UINT8            *)X86_MPAPIC_PHYS_TO_VIRT(_G_pX86MpApicData, MPAPIC_uiLaLoc);

    /*
     * Reset the counters
     */
    iEntrieNr                       = i;
    pMpApicData->MPAPIC_ucCpuBSP    = 0;
    pMpApicData->MPAPIC_uiCpuCount  = 0;
    pMpApicData->MPAPIC_uiIoApicNr  = 0;
    pMpApicData->MPAPIC_uiBusNr     = 0;
    pMpApicData->MPAPIC_uiIoIntNr   = 0;
    pMpApicData->MPAPIC_uiLoIntNr   = 0;
    iIntNr                          = 0;

    /*
     * Clear the CPU Index Table
     */
    lib_bzero((CHAR *)pMpApicData->MPAPIC_aucCpuIndexTable,
              sizeof(pMpApicData->MPAPIC_aucCpuIndexTable));

    /*
     * Second pass takes action on each important entry
     */
    pucPtr = (UINT8 *)pHdr + sizeof(X86_MP_HEADER);

    for (i = 0; i < iEntrieNr; i++) {
        switch (*pucPtr) {

        case MP_ENTRY_CPU:                                          /*  Processor Configuration Entry   */
            pCpu = (X86_MP_CPU *)pucPtr;

            if ((pCpu->CPU_ucCpuFlags & CPU_EN) == 0) {
                pucPtr += sizeof(X86_MP_CPU);
                break;
            }

            /*
             * Build CPU/Local APIC Id translation tables
             */
            pMpApicData->MPAPIC_aucCpuIndexTable[pCpu->CPU_ucLocalApicId] = \
                    (UINT8)pMpApicData->MPAPIC_uiCpuCount;

            /*
             * Table order relevant, table is indexed logically,
             * must contain logical indicies...
             */
            ucTemp1 = pCpu->CPU_ucLocalApicId;
            for (iIndex1 = 0; iIndex1 < pMpApicData->MPAPIC_uiCpuCount; iIndex1++) {
                if (ucTemp1 < pucLoApicIndexTable[iIndex1]) {
                    ucTemp2 = pucLoApicIndexTable[iIndex1];
                    pucLoApicIndexTable[iIndex1] = ucTemp1;
                    ucTemp1 = ucTemp2;
                }
            }
            pucLoApicIndexTable[pMpApicData->MPAPIC_uiCpuCount] = ucTemp1;

            pMpApicData->MPAPIC_uiCpuCount++;                       /*  Increment the CPU counter       */
            if (pCpu->CPU_ucCpuFlags & CPU_BP) {
                pMpApicData->MPAPIC_ucCpuBSP = pCpu->CPU_ucLocalApicId;
            }
            pucPtr += sizeof(X86_MP_CPU);
            break;

        case MP_ENTRY_IOAPIC:                                       /*  IO APIC Configuration Entry     */
            pIoApic = (X86_MP_IOAPIC *)pucPtr;
            if ((pIoApic->IOAPIC_ucIoApicFlags & 0x1) && (pMpApicData->MPAPIC_uiIoApicNr < IOAPIC_MAX_NR)) {
                pucLogicalTable[pMpApicData->MPAPIC_uiIoApicNr] = pIoApic->IOAPIC_ucIoApicId;
                puiAddrTable[pMpApicData->MPAPIC_uiIoApicNr] = (UINT32)pIoApic->IOAPIC_uiIoApicBaseAddress;
                pMpApicData->MPAPIC_uiIoApicNr++;                   /*  Increment the IO APIC counter   */
            }
            pucPtr += sizeof(X86_MP_IOAPIC);
            break;

        case MP_ENTRY_BUS:                                          /*  Bus Identifier Entry            */
            lib_bcopy((CHAR *)pucPtr,
                      (CHAR *)&pBusTable[pMpApicData->MPAPIC_uiBusNr],
                      sizeof(X86_MP_BUS));
            pMpApicData->MPAPIC_uiBusNr++;
            pucPtr += sizeof(X86_MP_BUS);
            break;

        case MP_ENTRY_IOINTERRUPT:                                  /*  IO Interrupt Entry              */
            lib_bcopy((CHAR *)pucPtr,
                      (CHAR *)&pInterruptTable[iIntNr],
                      sizeof(X86_MP_INTERRUPT));
            pMpApicData->MPAPIC_uiIoIntNr++;
            iIntNr++;
            pucPtr += sizeof(X86_MP_INTERRUPT);
            break;

        case MP_ENTRY_LOINTERRUPT:                                  /*  Local Interrupt Entry           */
            lib_bcopy((CHAR *)pucPtr,
                      (CHAR *)&pInterruptTable[iIntNr],
                      sizeof(X86_MP_INTERRUPT));
            pMpApicData->MPAPIC_uiLoIntNr++;
            iIntNr++;
            pucPtr += sizeof(X86_MP_INTERRUPT);
            break;

        default:
            /*
             * "Maybe" it is 8 bytes till the next entry
             */
            pucPtr += 8;
            break;
        }
    }

    /*
     * Set up the Local APIC base address
     */
    pMpApicData->MPAPIC_uiLocalApicBase = pHdr->HDR_uiLocalApicBaseAddr;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
 ** 函数名称: x86MpApicIrqAppend
 ** 功能描述: 追加 mpconfig 中的 IO 中断信息
 ** 输　入  : iIndex           索引
 ** 输　出  : IRQ 信息结构指针
 ** 全局变量:
 ** 调用模块:
*********************************************************************************************************/
static PVOID  x86MpApicIrqAppend (INT  iIndex)
{
    static X86_MP_HEADER     *pHdr;
    INT32                     i;
    UINT8                    *pucPtr;
    X86_MP_FPS               *pFps;

    if (pHdr == LW_NULL) {
        /*
         * Scan for the MP Floating Point Structure
         */
        pFps = (X86_MP_FPS *)x86MpApicScan((INT8 *)"_MP_", _G_pcX86EbdaStart, _G_pcX86EbdaEnd);
        if (pFps == LW_NULL) {
            pFps = (X86_MP_FPS *)x86MpApicScan((INT8 *)"_MP_", _G_pcX86BiosRomStart, _G_pcX86BiosRomEnd);
        }
        if (pFps == LW_NULL) {
            return  (LW_NULL);
        }

        /*
         * If featurebyte1 (array element 0) is non-zero, then we use
         * standard configuration return PX_ERROR and let the error handle use
         * standard addresses and Local and IO APIC ID's.
         */
        if ((pFps->FPS_aucFeatureByte[0] != 0) ||
            (pFps->FPS_uiConfigTableAddr == 0)) {
            return  (LW_NULL);
        }

        /*
         * Get MP header pointer
         */
        if ((pFps->FPS_uiConfigTableAddr >= EBDA_START) &&
            (pFps->FPS_uiConfigTableAddr  < EBDA_END)) {
            pHdr = (X86_MP_HEADER *)((ULONG)_G_pcX86EbdaStart +
                   ((ULONG)pFps->FPS_uiConfigTableAddr - EBDA_START));
        } else {
            pHdr = (X86_MP_HEADER *)((ULONG)_G_pcX86BiosRomStart +
                   ((ULONG)pFps->FPS_uiConfigTableAddr - BIOS_ROM_START));
        }

        /*
         * Sanity check of the MP Configuration Table
         */
        if (x86MpApicConfigTableCheck(pHdr) != ERROR_NONE) {
            return  (LW_NULL);
        }
    }

    /*
     * First pass count the number of each important entry
     */
    pucPtr = (UINT8 *)pHdr + sizeof(X86_MP_HEADER);

    for (i = 0; i < pHdr->HDR_usEntryCount; i++) {
        switch (*pucPtr) {

        case MP_ENTRY_CPU:                                          /*  Processor Configuration Entry   */
            pucPtr += sizeof(X86_MP_CPU);
            break;

        case MP_ENTRY_IOAPIC:                                       /*  IO Apic Configuration Entry     */
            pucPtr += sizeof(X86_MP_IOAPIC);
            break;

        case MP_ENTRY_BUS:
            pucPtr += sizeof(X86_MP_BUS);
            break;

        case MP_ENTRY_IOINTERRUPT:                                  /*  IO Interrupt Entry              */
            if (iIndex-- == 0) {
                return  (pucPtr);
            }
            pucPtr += sizeof(X86_MP_INTERRUPT);
            break;

        case MP_ENTRY_LOINTERRUPT:                                  /*  Local Interrupt Entry           */
            pucPtr += sizeof(X86_MP_INTERRUPT);
            break;

        default:
            /*
             * "Maybe" it is 8 bytes till the next entry
             */
            pucPtr += 8;
            break;
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
 ** 函数名称: x86MpApicScan
 ** 功能描述: 从给出的内存地址范围里查找匹配的字符串
 ** 输　入  : pcMatch      匹配的字符串
 **           pcStart      开始地址
 **           pcEnd        结束地址
 ** 输　出  : 匹配的地址 或 LW_NULL
 ** 全局变量:
 ** 调用模块:
*********************************************************************************************************/
static INT8  *x86MpApicScan (INT8  *pcMatch, INT8  *pcStart, INT8  *pcEnd)
{
    INT8   *pcPtr;
    UINT32  uiCS;
    INT8    i;

    for (pcPtr = pcStart; (pcPtr + 16) < pcEnd; pcPtr += 16) {
        if (lib_strncmp((CHAR *)pcPtr, (CHAR *)pcMatch, lib_strlen((CHAR *)pcMatch)) == 0) {
            /*
             * Verify MP floating pointer struture checksum
             * the bytewise sum of the 16 byte struture should be 0
             */
            uiCS = 0;
            for (i = 0; i < 16; i++) {
                uiCS += pcPtr[i];
            }

            if ((uiCS & 0xff) == 0) {
                return  (pcPtr);
            }
        }
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
 ** 函数名称: x86MpApicLoBaseGet
 ** 功能描述: 获得 Local APIC 基地址
 ** 输　入  : ppcLocalApicBase     Local APIC 基地址
 ** 输　出  : ERROR CODE
 ** 全局变量:
 ** 调用模块:
*********************************************************************************************************/
INT  x86MpApicLoBaseGet (CHAR  **ppcLocalApicBase)
{
    if ((ppcLocalApicBase == LW_NULL) || (!_G_bX86MpApicDrvInited)) {
        return  (PX_ERROR);
    }

    *ppcLocalApicBase = (CHAR *)((ULONG)_G_pX86MpApicData->MPAPIC_uiLocalApicBase);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
 ** 函数名称: x86MpApicIoApicNrGet
 ** 功能描述: 获得 IO APIC 数目
 ** 输　入  : puiIoApicNr          IO APIC 数目
 ** 输　出  : ERROR CODE
 ** 全局变量:
 ** 调用模块:
*********************************************************************************************************/
INT  x86MpApicIoApicNrGet (UINT32  *puiIoApicNr)
{
    if ((puiIoApicNr == LW_NULL) || (!_G_bX86MpApicDrvInited)) {
        return  (PX_ERROR);
    }

    *puiIoApicNr = _G_pX86MpApicData->MPAPIC_uiIoApicNr;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
 ** 函数名称: x86MpApicIoIntNrGet
 ** 功能描述: 获得 IO 中断数目
 ** 输　入  : puiIoApicNr          IO 中断数目
 ** 输　出  : ERROR CODE
 ** 全局变量:
 ** 调用模块:
*********************************************************************************************************/
INT  x86MpApicIoIntNrGet (UINT32  *puiIoIntNr)
{
    if ((puiIoIntNr == LW_NULL) || (!_G_bX86MpApicDrvInited)) {
        return  (PX_ERROR);
    }

    *puiIoIntNr = _G_pX86MpApicData->MPAPIC_uiIoIntNr;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
 ** 函数名称: x86MpApicLoIntNrGet
 ** 功能描述: 获得 Local 中断数目
 ** 输　入  : puiLoIntNr           Local APIC 数目
 ** 输　出  : ERROR CODE
 ** 全局变量:
 ** 调用模块:
*********************************************************************************************************/
INT   x86MpApicLoIntNrGet(UINT32  *puiLoIntNr)
{
    if ((puiLoIntNr == LW_NULL) || (!_G_bX86MpApicDrvInited)) {
        return  (PX_ERROR);
    }

    *puiLoIntNr = _G_pX86MpApicData->MPAPIC_uiLoIntNr;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
 ** 函数名称: x86MpApicBusNrGet
 ** 功能描述: 获得 BUS 路由数目
 ** 输　入  : puiBusNr             BUS 路由数目
 ** 输　出  : ERROR CODE
 ** 全局变量:
 ** 调用模块:
*********************************************************************************************************/
INT  x86MpApicBusNrGet (UINT32  *puiBusNr)
{
    if ((puiBusNr == LW_NULL) || (!_G_bX86MpApicDrvInited)) {
        return  (PX_ERROR);
    }

    *puiBusNr = _G_pX86MpApicData->MPAPIC_uiBusNr;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
 ** 函数名称: x86MpApicCpuNrGet
 ** 功能描述: 获得 CPU 数目
 ** 输　入  : puiCpuNr             CPU 数目
 ** 输　出  : ERROR CODE
 ** 全局变量:
 ** 调用模块:
*********************************************************************************************************/
INT  x86MpApicCpuNrGet (UINT32  *puiCpuNr)
{
    if ((puiCpuNr == LW_NULL) || (!_G_bX86MpApicDrvInited)) {
        return  (PX_ERROR);
    }

    *puiCpuNr = _G_pX86MpApicData->MPAPIC_uiCpuCount;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
 ** 函数名称: x86MpApicInterruptTableGet
 ** 功能描述: 获得中断路由表地址
 ** 输　入  : ppInterruptTable     中断路由表地址
 ** 输　出  : ERROR CODE
 ** 全局变量:
 ** 调用模块:
*********************************************************************************************************/
INT   x86MpApicInterruptTableGet (X86_MP_INTERRUPT  **ppInterruptTable)
{
    if ((ppInterruptTable == LW_NULL) || (!_G_bX86MpApicDrvInited)) {
        return  (PX_ERROR);
    }

    *ppInterruptTable = (X86_MP_INTERRUPT *)X86_MPAPIC_PHYS_TO_VIRT(_G_pX86MpApicData, MPAPIC_uiItLoc);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
 ** 函数名称: x86MpApicBusTableGet
 ** 功能描述: 获得 BUS 路由表地址
 ** 输　入  : ppBusTable           BUS 路由表地址
 ** 输　出  : ERROR CODE
 ** 全局变量:
 ** 调用模块:
*********************************************************************************************************/
INT  x86MpApicBusTableGet (X86_MP_BUS  **ppBusTable)
{
    if ((ppBusTable == LW_NULL) || (!_G_bX86MpApicDrvInited)) {
        return  (PX_ERROR);
    }

    *ppBusTable = (X86_MP_BUS *)X86_MPAPIC_PHYS_TO_VIRT(_G_pX86MpApicData, MPAPIC_uiBtLoc);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
 ** 函数名称: x86MpApicFpsGet
 ** 功能描述: 获得 FPS 结构地址
 ** 输　入  : ppFps                FPS 结构地址
 ** 输　出  : ERROR CODE
 ** 全局变量:
 ** 调用模块:
*********************************************************************************************************/
INT  x86MpApicFpsGet (X86_MP_FPS  **ppFps)
{
    if ((ppFps == LW_NULL) || (!_G_bX86MpApicDrvInited)) {
        return  (PX_ERROR);
    }

    *ppFps = (X86_MP_FPS *)(&_G_pX86MpApicData->MPAPIC_Fps);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
 ** 函数名称: x86MpApicAddrTableGet
 ** 功能描述: 获得 IOAPIC 地址表地址
 ** 输　入  : ppuiAddrTable       IOAPIC 地址表地址
 ** 输　出  : ERROR CODE
 ** 全局变量:
 ** 调用模块:
*********************************************************************************************************/
INT  x86MpApicAddrTableGet (UINT32  **ppuiAddrTable)
{
    if ((ppuiAddrTable == LW_NULL) || (!_G_bX86MpApicDrvInited)) {
        return  (PX_ERROR);
    }

    *ppuiAddrTable = (UINT32 *)X86_MPAPIC_PHYS_TO_VIRT(_G_pX86MpApicData, MPAPIC_uiAtLoc);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
 ** 函数名称: x86MpApicLogicalTableGet
 ** 功能描述: 获得 IOAPIC ID 表地址
 ** 输　入  : ppucLogicalTable     IOAPIC ID 表地址
 ** 输　出  : ERROR CODE
 ** 全局变量:
 ** 调用模块:
*********************************************************************************************************/
INT  x86MpApicLogicalTableGet (UINT8   **ppucLogicalTable)
{
    if ((ppucLogicalTable == LW_NULL) || (!_G_bX86MpApicDrvInited)) {
        return  (PX_ERROR);
    }

    *ppucLogicalTable = (UINT8 *)X86_MPAPIC_PHYS_TO_VIRT(_G_pX86MpApicData, MPAPIC_uiLtLoc);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
 ** 函数名称: x86MpApicLoIndexTableGet
 ** 功能描述: 获得 Local APIC ID 表地址
 ** 输　入  : ppucLoApicIndexTable   Local APIC ID 表地址
 ** 输　出  : ERROR CODE
 ** 全局变量:
 ** 调用模块:
*********************************************************************************************************/
INT  x86MpApicLoIndexTableGet (UINT8   **ppucLoApicIndexTable)
{
    if ((ppucLoApicIndexTable == LW_NULL) || (!_G_bX86MpApicDrvInited)) {
        return  (PX_ERROR);
    }

    *ppucLoApicIndexTable = (UINT8 *)X86_MPAPIC_PHYS_TO_VIRT(_G_pX86MpApicData, MPAPIC_uiLaLoc);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
 ** 函数名称: x86MpApicCpuIndexTableGet
 ** 功能描述: 获得 CPU ID 表地址
 ** 输　入  : ppucCpuIndexTable    CPU ID 表地址
 ** 输　出  : ERROR CODE
 ** 全局变量:
 ** 调用模块:
*********************************************************************************************************/
INT  x86MpApicCpuIndexTableGet (UINT8  **ppucCpuIndexTable)
{
    if ((ppucCpuIndexTable == LW_NULL) || (!_G_bX86MpApicDrvInited)) {
        return  (PX_ERROR);
    }

    *ppucCpuIndexTable = (UINT8 *)(_G_pX86MpApicData->MPAPIC_aucCpuIndexTable);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
 ** 函数名称: x86MpApicDataShow
 ** 功能描述: 显示 MP 配置数据
 ** 输　入  : NONE
 ** 输　出  : ERROR CODE
 ** 全局变量:
 ** 调用模块:
*********************************************************************************************************/
INT  x86MpApicDataShow (VOID)
{
    X86_MP_INTERRUPT   *pMpApicInterruptTable;
    X86_MP_BUS         *pMpApicBusTable;
    UINT8              *pucMpApicLogicalTable;
    UINT8              *pucMpLoApicIndexTable;
    UINT32             *puiMpApicAddrTable;
    INT                 i;

    if (!_G_bX86MpApicDrvInited) {
        return  (PX_ERROR);
    }

    /*
     * Retrieve the data
     */
    puiMpApicAddrTable      = (UINT32           *)X86_MPAPIC_PHYS_TO_VIRT(_G_pX86MpApicData, MPAPIC_uiAtLoc);
    pucMpApicLogicalTable   = (UINT8            *)X86_MPAPIC_PHYS_TO_VIRT(_G_pX86MpApicData, MPAPIC_uiLtLoc);
    pMpApicInterruptTable   = (X86_MP_INTERRUPT *)X86_MPAPIC_PHYS_TO_VIRT(_G_pX86MpApicData, MPAPIC_uiItLoc);
    pMpApicBusTable         = (X86_MP_BUS       *)X86_MPAPIC_PHYS_TO_VIRT(_G_pX86MpApicData, MPAPIC_uiBtLoc);
    pucMpLoApicIndexTable   = (UINT8            *)X86_MPAPIC_PHYS_TO_VIRT(_G_pX86MpApicData, MPAPIC_uiLaLoc);

    printf("MP Configuration Data acquired by MPAPIC driver\n\n");

    printf("  MPAPIC_uiLocalApicBase        = %p\n\n", (CHAR *)(ULONG)_G_pX86MpApicData->MPAPIC_uiLocalApicBase);

    printf("  MPAPIC_uiIoApicNr             = 0x%08x\n",   _G_pX86MpApicData->MPAPIC_uiIoApicNr);
    printf("  MPAPIC_uiBusNr                = 0x%08x\n\n", _G_pX86MpApicData->MPAPIC_uiBusNr);

    printf("  MPAPIC_uiIoIntNr              = 0x%08x\n",   _G_pX86MpApicData->MPAPIC_uiIoIntNr);
    printf("  MPAPIC_uiLoIntNr              = 0x%08x\n\n", _G_pX86MpApicData->MPAPIC_uiLoIntNr);

    printf("  MPAPIC_Fps                    = %p\n", &_G_pX86MpApicData->MPAPIC_Fps);

    printf("    MPAPIC_Fps.FPS_acSignature          = ");
    for (i = 0; i < 4; i++) {
        printf("%c", _G_pX86MpApicData->MPAPIC_Fps.FPS_acSignature[i]);
    }

    printf("\n    MPAPIC_Fps.FPS_uiConfigTableAddr    = 0x%08x\n", _G_pX86MpApicData->MPAPIC_Fps.FPS_uiConfigTableAddr);

    printf("    MPAPIC_Fps.FPS_ucLength             = 0x%02x\n",  _G_pX86MpApicData->MPAPIC_Fps.FPS_ucLength);

    printf("    MPAPIC_Fps.FPS_ucSpecRev            = 0x%02x\n",  _G_pX86MpApicData->MPAPIC_Fps.FPS_ucSpecRev);

    printf("    MPAPIC_Fps.FPS_ucChecksum           = 0x%02x\n",  _G_pX86MpApicData->MPAPIC_Fps.FPS_ucChecksum);

    for (i = 0; i < 5; i++) {
        printf("    MPAPIC_Fps.FPS_aucFeatureByte[%d]    = 0x%02x\n", i, _G_pX86MpApicData->MPAPIC_Fps.FPS_aucFeatureByte[i]);
    }

    printf("\n  puiMpApicAddrTable:                     \n");
    for (i = 0; i < _G_pX86MpApicData->MPAPIC_uiIoApicNr; i++) {
        printf("    puiMpApicAddrTable[%d]         = 0x%08x\n", i, puiMpApicAddrTable[i]);
    }

    printf("\n  pucMpApicLogicalTable:                  \n");
    for (i = 0; i < _G_pX86MpApicData->MPAPIC_uiIoApicNr; i++) {
        printf("    pucMpApicLogicalTable[%d]      = 0x%08x\n", i, pucMpApicLogicalTable[i]);
    }

    printf("\n  MPAPIC_uiCpuCount                  = 0x%08x\n", _G_pX86MpApicData->MPAPIC_uiCpuCount);
    printf("\n  Boot Strap Processor local APIC ID = 0x%08x\n", _G_pX86MpApicData->MPAPIC_ucCpuBSP);

    printf("\n  MPAPIC_aucCpuIndexTable (first 10 entries):     \n");
    /*
     * Max for this is actually X86_CPUID_MAX_NUM_CPUS
     */
    for (i = 0; i < 24; i++) {
        printf("    MPAPIC_aucCpuIndexTable[%d]   = 0x%08x\n", i, _G_aucX86CpuIndexTable[i]);
    }

    printf("\n  pucMpLoApicIndexTable:                  \n");
    for (i = 0; i < _G_pX86MpApicData->MPAPIC_uiCpuCount; i++) {
        printf("    pucMpLoApicIndexTable[%d]     = 0x%08x\n", i, pucMpLoApicIndexTable[i]);
    }

    printf("\n  pMpApicInterruptTable:");
    for (i = 0; i < (_G_pX86MpApicData->MPAPIC_uiIoIntNr + _G_pX86MpApicData->MPAPIC_uiLoIntNr); i++) {
        printf("\n    pMpApicInterruptTable[%d].INT_ucEntryType      = 0x%08x",   i, pMpApicInterruptTable[i].INT_ucEntryType);
        printf("\n    pMpApicInterruptTable[%d].INT_ucInterruptType  = 0x%08x",   i, pMpApicInterruptTable[i].INT_ucInterruptType);
        printf("\n    pMpApicInterruptTable[%d].INT_usInterruptFlags = 0x%08x",   i, pMpApicInterruptTable[i].INT_usInterruptFlags);
        printf("\n    pMpApicInterruptTable[%d].INT_ucSourceBusId    = 0x%08x",   i, pMpApicInterruptTable[i].INT_ucSourceBusId);
        printf("\n    pMpApicInterruptTable[%d].INT_ucSourceBusIrq   = 0x%08x",   i, pMpApicInterruptTable[i].INT_ucSourceBusIrq);
        printf("\n    pMpApicInterruptTable[%d].INT_ucDestApicId     = 0x%08x",   i, pMpApicInterruptTable[i].INT_ucDestApicId);
        printf("\n    pMpApicInterruptTable[%d].INT_ucDestApicIntIn  = 0x%08x\n", i, pMpApicInterruptTable[i].INT_ucDestApicIntIn);
    }

    printf("\n  pMpApicBusTable:");
    for (i = 0; i < (_G_pX86MpApicData->MPAPIC_uiBusNr); i++) {
        printf("\n    pMpApicBusTable[%d].BUS_ucEntryType            = 0x%08x",   i, pMpApicBusTable[i].BUS_ucEntryType);
        printf("\n    pMpApicBusTable[%d].BUS_ucBusId                = 0x%08x",   i, pMpApicBusTable[i].BUS_ucBusId);
        printf("\n    pMpApicBusTable[%d].BUS_aucBusType[0]          = 0x%08x",   i, pMpApicBusTable[i].BUS_aucBusType[0]);
        printf("\n    pMpApicBusTable[%d].BUS_aucBusType[1]          = 0x%08x",   i, pMpApicBusTable[i].BUS_aucBusType[1]);
        printf("\n    pMpApicBusTable[%d].BUS_aucBusType[2]          = 0x%08x",   i, pMpApicBusTable[i].BUS_aucBusType[2]);
        printf("\n    pMpApicBusTable[%d].BUS_aucBusType[3]          = 0x%08x",   i, pMpApicBusTable[i].BUS_aucBusType[3]);
        printf("\n    pMpApicBusTable[%d].BUS_aucBusType[4]          = 0x%08x",   i, pMpApicBusTable[i].BUS_aucBusType[4]);
        printf("\n    pMpApicBusTable[%d].BUS_aucBusType[5]          = 0x%08x\n", i, pMpApicBusTable[i].BUS_aucBusType[5]);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
 ** 函数名称: x86MpBiosShow
 ** 功能描述: 显示 MP 配置表
 ** 输　入  : NONE
 ** 输　出  : NONE
 ** 全局变量:
 ** 调用模块:
*********************************************************************************************************/
VOID  x86MpBiosShow (VOID)
{
    UINT8          *pucPtr;
    X86_MP_FPS     *pFps;
    X86_MP_HEADER  *pHdr;
    X86_MP_CPU     *pCpu;
    X86_MP_IOAPIC  *pIoApic;
    X86_MP_BUS     *pBus;
    INT             iX, iY;

    /*
     * MP Floating Point Structure
     */
    pFps = (X86_MP_FPS *)x86MpApicScan((INT8 *)"_MP_", _G_pcX86EbdaStart, _G_pcX86EbdaEnd);
    if (pFps == LW_NULL) {
        pFps = (X86_MP_FPS *)x86MpApicScan((INT8 *)"_MP_", _G_pcX86BiosRomStart, _G_pcX86BiosRomEnd);
    }
    if (pFps == LW_NULL) {
        printf("Not MP Compliant!\n");
        return;
    }

    printf("MP Floating Point Structure\n");
    printf("  Address Pointer       = 0x%08x\n", pFps->FPS_uiConfigTableAddr);
    printf("  Spec Version          = 0x%02x\n", pFps->FPS_ucSpecRev);
    printf("  Feature byte-1        = 0x%02x\n", pFps->FPS_aucFeatureByte[0]);
    printf("  Feature byte-2        = 0x%02x\n", pFps->FPS_aucFeatureByte[1]);
    printf("  Feature byte-3        = 0x%02x\n", pFps->FPS_aucFeatureByte[2]);
    printf("  Feature byte-4        = 0x%02x\n", pFps->FPS_aucFeatureByte[3]);
    printf("  Feature byte-5        = 0x%02x\n", pFps->FPS_aucFeatureByte[4]);

    if ((pFps->FPS_aucFeatureByte[0] != 0) || (pFps->FPS_uiConfigTableAddr == 0)) {
        printf("MP Configuration Table does not exist!\n");
        return;
    }

    /*
     * Show MP Configuration Table Header
     */
    if ((pFps->FPS_uiConfigTableAddr >= EBDA_START) && (pFps->FPS_uiConfigTableAddr < EBDA_END)) {
        pHdr = (X86_MP_HEADER *)((ULONG)_G_pcX86EbdaStart + ((ULONG)pFps->FPS_uiConfigTableAddr - EBDA_START));
    } else {
        pHdr = (X86_MP_HEADER *)((ULONG)_G_pcX86BiosRomStart + ((ULONG)pFps->FPS_uiConfigTableAddr - BIOS_ROM_START));
    }

    printf("MP Configuration Table\n");
    printf("  Base Table Length     = 0x%04x\n", pHdr->HDR_usTableLength);

    printf("  Base Table Checksum ");
    if (x86MpApicConfigTableCheck(pHdr) != ERROR_NONE) {
        printf("Fail! \n");
    } else {
        printf("OK. \n");
    }
    printf("  OEM ID String         = ");
    for (iX = 0; iX < sizeof(pHdr->HDR_acOemId); iX++) {
        printf("%c", pHdr->HDR_acOemId[iX]);
    }
    printf("\n");
    printf("  Product ID String     = ");
    for (iX = 0; iX < sizeof(pHdr->HDR_acProdId); iX++) {
        printf("%c", pHdr->HDR_acProdId[iX]);
    }
    printf("\n");
    printf("  OEM Table Pointer     = 0x%08x\n", pHdr->HDR_uiOemTablePtr);
    printf("  OEM Table Size        = 0x%04x\n", pHdr->HDR_usOemTableSize);
    printf("  Entry Count           = 0x%04x\n", pHdr->HDR_usEntryCount);
    printf("  Address of local APIC = 0x%08x\n", pHdr->HDR_uiLocalApicBaseAddr);
    printf("  Extended Table Length = 0x%04x\n", pHdr->HDR_usExtendedTableLength);

    /*
     * Show MP Configuration Table Entry
     */
    pucPtr = (UINT8 *)pHdr + sizeof(X86_MP_HEADER);
    for (iX = 0; iX < pHdr->HDR_usEntryCount; iX++) {
        switch (*pucPtr) {

        case MP_ENTRY_CPU:                                              /*  Processor Entry             */
            pCpu = (X86_MP_CPU *)pucPtr;
            printf("Processor Entry\n");
            printf("  Local Apic ID         = 0x%02x\n", pCpu->CPU_ucLocalApicId);
            printf("  Local Apic Ver        = 0x%02x\n", pCpu->CPU_ucLocalApicVersion);
            printf("  CPU Flags             = %s|%s\n",
                    (pCpu->CPU_ucCpuFlags & 0x01) ? "EN" : "~EN",
                    (pCpu->CPU_ucCpuFlags & 0x02) ? "BP" : "~BP");
            printf("  CPU Signature         = 0x%08x\n", pCpu->CPU_ucCpuSig);
            printf("  Feature Flags         = 0x%08x\n", pCpu->CPU_uiFeatureFlags);
            pucPtr += sizeof(X86_MP_CPU);
            break;

        case MP_ENTRY_BUS:                                              /*  Bus Entry                   */
            pBus = (X86_MP_BUS *)pucPtr;
            {
                INT  iBusId = pBus->BUS_ucBusId;
                printf("Bus Entry\n");
                printf("  Bus ID                = 0x%02x\n", iBusId);
            }
            printf("  Bus Type String       = ");
            for (iY = 0; iY < 6; iY++) {
                printf("%c", pBus->BUS_aucBusType[iY]);
            }
            printf("\n");
            pucPtr += 8;
            break;

        case MP_ENTRY_IOAPIC:                                           /*  IO APIC Entry               */
            pIoApic = (X86_MP_IOAPIC *)pucPtr;
            printf("IO Apic Entry\n");
            printf("  IO Apic ID            = 0x%02x\n", pIoApic->IOAPIC_ucIoApicId);
            printf("  IO Apic Ver           = 0x%02x\n", pIoApic->IOAPIC_ucIoApicVersion);
            printf("  IO Apic Flags         = %s\n",    (pIoApic->IOAPIC_ucIoApicFlags & 0x01) ? "EN" : "~EN");
            printf("  IO Apic Address       = 0x%08x\n", pIoApic->IOAPIC_uiIoApicBaseAddress);
            pucPtr += sizeof(X86_MP_IOAPIC);
            break;

        case MP_ENTRY_IOINTERRUPT:                                      /*  IO Interrupt Entry          */
            printf("IO Interrupt Entry\n");
            printf("  Interrupt Type        = 0x%02x\n",       *(UINT8  *)(pucPtr + 1));
            printf("  IO Interrupt Flag     = 0x%04x InPol(", (*(UINT16 *)(pucPtr + 2)) & 0x03);
            switch ((*(UINT16 *)(pucPtr + 2)) & 0x03) {

            case 0:
                printf("Bus");
                break;
            case 1:
                printf("AH");
                break;
            case 2:
                printf("RSV");
                break;
            case 3:
                printf("AL");
                break;
            }
            printf(")|Trig(");
            switch (((*(UINT16 *)(pucPtr + 2)) >> 2) & 0x03) {

            case 0:
                printf("Bus");
                break;
            case 1:
                printf("Edge");
                break;
            case 2:
                printf("RSV");
                break;
            case 3:
                printf("Lvl");
                break;
            }
            printf(")\n");
            printf("  Source Bus ID         = 0x%02x\n", *(UINT8 *)(pucPtr + 4));
            printf("  Source Bus IRQ        = 0x%02x\n", *(UINT8 *)(pucPtr + 5));
            printf("  Dest IO Apic ID       = 0x%02x\n", *(UINT8 *)(pucPtr + 6));
            printf("  Dest IO Apic INTIN    = 0x%02x\n", *(UINT8 *)(pucPtr + 7));
            pucPtr += 8;
            break;

        case MP_ENTRY_LOINTERRUPT:                                      /*  Local Interrupt Entry       */
            printf("Local Interrupt Entry\n");
            printf("  Interrupt Type        = 0x%02x\n",       *(UINT8  *)(pucPtr + 1));
            printf("  IO Interrupt Flag     = 0x%04x InPol(", (*(UINT16 *)(pucPtr + 2)) & 0x03);
            switch ((*(UINT16 *)(pucPtr + 2)) & 0x03) {

            case 0:
                printf("Bus");
                break;
            case 1:
                printf("AH");
                break;
            case 2:
                printf("RSV");
                break;
            case 3:
                printf("AL");
                break;
            }
            printf(")|Trig(");
            switch (((*(UINT16 *)(pucPtr + 2)) >> 2) & 0x03) {

            case 0:
                printf("Bus");
                break;
            case 1:
                printf("Edge");
                break;
            case 2:
                printf("RSV");
                break;
            case 3:
                printf("Lvl");
                break;
            }
            printf(")\n");
            printf("  Source Bus ID         = 0x%02x\n", *(UINT8 *)(pucPtr + 4));
            printf("  Source Bus IRQ        = 0x%02x\n", *(UINT8 *)(pucPtr + 5));
            printf("  Dest Local Apic ID    = 0x%02x\n", *(UINT8 *)(pucPtr + 6));
            printf("  Dest Local Apic INTIN = 0x%02x\n", *(UINT8 *)(pucPtr + 7));
            pucPtr += 8;
            break;

        default:                                                        /*  Unknown Entry               */
            pucPtr += 8;                                                /*  Wild guess                  */
            break;
        }
    }

    /*
     * pucPtr now pointing to the begining of the extended MP configuration table
     */
    iX = 0;
    while (iX < (INT)pHdr->HDR_usExtendedTableLength) {
        switch ((UINT8)*pucPtr) {

        case EXT_MP_ENTRY_SASM:                                 /*  System Address Space Map Entry      */
            printf("System Address Space Map Entry\n");
            printf("  Entry Length          = 0x%02x\n", *(UINT8  *)(pucPtr + 1));
            printf("  Bus ID                = 0x%02x\n", *(UINT8  *)(pucPtr + 2));
            printf("  Address Type          = 0x%02x\n", *(UINT8  *)(pucPtr + 3));
            printf("  Address Base (LOW)    = 0x%08x\n", *(UINT32 *)(pucPtr + 4));
            printf("  Address Base (HIGH)   = 0x%08x\n", *(UINT32 *)(pucPtr + 8));
            printf("  Address Length (LOW)  = 0x%08x\n", *(UINT32 *)(pucPtr + 12));
            printf("  Address Length (HIGH) = 0x%08x\n", *(UINT32 *)(pucPtr + 16));
            pucPtr += 20;
            iX     += 20;
            break;

        case EXT_MP_ENTRY_BHD:                                  /*  Bus Hierarchy Descriptor Entry      */
            printf("Bus Hierarchy Descriptor Entry\n");
            printf("  Entry Length          = 0x%02x\n", *(UINT8 *)(pucPtr + 1));
            printf("  Bus ID                = 0x%02x\n", *(UINT8 *)(pucPtr + 2));
            printf("  Bus Info              = 0x%02x\n", *(UINT8 *)(pucPtr + 3));
            printf("  Parent Bus            = 0x%02x\n", *(UINT8 *)(pucPtr + 4));
            pucPtr += 8;
            iX     += 8;
            break;

        case EXT_MP_ENTRY_CBASM:                                /*  Comp Address Space Modifier Entry   */
            printf("Comp Address Space Modifier\n");
            printf("  Entry Length          = 0x%02x\n", *(UINT8  *)(pucPtr + 1));
            printf("  Bus ID                = 0x%02x\n", *(UINT8  *)(pucPtr + 2));
            printf("  Address Mod           = 0x%02x\n", *(UINT8  *)(pucPtr + 3));
            printf("  Predefined Range List = 0x%08x\n", *(UINT32 *)(pucPtr + 4));
            pucPtr += 8;
            iX     += 8;
            break;

        default:                                                /*  Unknown Entry                       */
            pucPtr += 8;                                        /*  Wild guess                          */
            iX     += 8;                                        /*  Wild guess                          */
            break;
        }
    }
}
/*********************************************************************************************************
 ** 函数名称: x86MpBiosIoIntMapShow
 ** 功能描述: 显示 MP IO 中断映射
 ** 输　入  : NONE
 ** 输　出  : NONE
 ** 全局变量:
 ** 调用模块:
*********************************************************************************************************/
VOID  x86MpBiosIoIntMapShow (VOID)
{
    UINT8          *pucPtr;
    X86_MP_FPS     *pFps;
    X86_MP_HEADER  *pHdr;
    INT             i;

    /*
     * MP Floating Point Structure
     */
    pFps = (X86_MP_FPS *)x86MpApicScan((INT8 *)"_MP_", _G_pcX86EbdaStart, _G_pcX86EbdaEnd);
    if (pFps == LW_NULL) {
        pFps = (X86_MP_FPS *)x86MpApicScan((INT8 *)"_MP_", _G_pcX86BiosRomStart, _G_pcX86BiosRomEnd);
    }
    if (pFps == LW_NULL) {
        printf("Not MP Compliant!\n");
        return;
    }

    if ((pFps->FPS_aucFeatureByte[0] != 0) || (pFps->FPS_uiConfigTableAddr == 0)) {
        printf("MP Configuration Table does not exist!\n");
        return;
    }

    /*
     * Show MP Configuration Table Header
     */
    if ((pFps->FPS_uiConfigTableAddr >= EBDA_START) && (pFps->FPS_uiConfigTableAddr < EBDA_END)) {
        pHdr = (X86_MP_HEADER *)((ULONG)_G_pcX86EbdaStart + ((ULONG)pFps->FPS_uiConfigTableAddr - EBDA_START));
    } else {
        pHdr = (X86_MP_HEADER *)((ULONG)_G_pcX86BiosRomStart + ((ULONG)pFps->FPS_uiConfigTableAddr - BIOS_ROM_START));
    }

    /*
     * Show Interrupt Configuration Table Entry
     */
    printf("BIOS I/O Interrupt Configuration Table\n");
    printf("  Type    Flags   Bus ID    Bus IRQ   APIC ID APIC INT In  Org Dev  INT_n#\n");
    printf("  ====   ======   ======    =======   ======= ===========  =======  ======\n");
    pucPtr = (UINT8 *)pHdr + sizeof(X86_MP_HEADER);
    for (i = 0; i < pHdr->HDR_usEntryCount; i++) {
        switch (*pucPtr) {

        case MP_ENTRY_CPU:                                              /*  Processor Entry             */
            pucPtr += sizeof(X86_MP_CPU);
            break;

        case MP_ENTRY_BUS:                                              /*  Bus Entry                   */
            pucPtr += 8;
            break;

        case MP_ENTRY_IOAPIC:                                           /*  IO APIC Entry               */
            pucPtr += sizeof(X86_MP_IOAPIC);
            break;

        case MP_ENTRY_IOINTERRUPT:                                      /*  IO Interrupt Entry          */
            printf("  0x%02x   0x%04x     0x%02x       0x%02x      0x%02x        0x%02x     0x%02x  ",
                    *(pucPtr + 1), *(UINT16 *)(pucPtr + 2), *(pucPtr + 4), *(pucPtr + 5), *(pucPtr + 6),
                    *(pucPtr + 7), (*(pucPtr + 5)) >> 2);

            switch (*(pucPtr + 5) & 0x3) {
            case 0:
                printf("A (0x%x & 0x3 -> %d),\n", *(pucPtr + 5), *(pucPtr + 5) & 0x3);
                break;

            case 1:
                printf("B (0x%x & 0x3 -> %d),\n", *(pucPtr + 5), *(pucPtr + 5) & 0x3);
                break;

            case 2:
                printf("C (0x%x & 0x3 -> %d),\n", *(pucPtr + 5), *(pucPtr + 5) & 0x3);
                break;

            case 3:
                printf("D (0x%x & 0x3 -> %d),\n", *(pucPtr + 5), *(pucPtr + 5) & 0x3);
                break;
            }
            pucPtr += 8;
            break;

        case MP_ENTRY_LOINTERRUPT:                                      /*  Local Interrupt Entry       */
            pucPtr += 8;
            break;

        default:                                                        /*  Unknown Entry               */
            pucPtr += 8;                                                /*  Wild guess                  */
            break;
        }
    }
}
/*********************************************************************************************************
 ** 函数名称: x86MpBiosLocalIntMapShow
 ** 功能描述: 显示 MP local 中断映射
 ** 输　入  : NONE
 ** 输　出  : NONE
 ** 全局变量:
 ** 调用模块:
*********************************************************************************************************/
VOID  x86MpBiosLocalIntMapShow (VOID)
{
    UINT8          *pucPtr;
    X86_MP_FPS     *pFps;
    X86_MP_HEADER  *pHdr;
    INT             i;

    /*
     * MP Floating Point Structure
     */
    pFps = (X86_MP_FPS *)x86MpApicScan((INT8 *)"_MP_", _G_pcX86EbdaStart, _G_pcX86EbdaEnd);
    if (pFps == LW_NULL) {
        pFps = (X86_MP_FPS *)x86MpApicScan((INT8 *)"_MP_", _G_pcX86BiosRomStart, _G_pcX86BiosRomEnd);
    }
    if (pFps == LW_NULL) {
        printf("Not MP Compliant!\n");
        return;
    }

    if ((pFps->FPS_aucFeatureByte[0] != 0) || (pFps->FPS_uiConfigTableAddr == 0)) {
        printf("MP Configuration Table does not exist!\n");
        return;
    }

    /*
     * Show MP Configuration Table Header
     */
    if ((pFps->FPS_uiConfigTableAddr >= EBDA_START) && (pFps->FPS_uiConfigTableAddr < EBDA_END)) {
        pHdr = (X86_MP_HEADER *)((ULONG)_G_pcX86EbdaStart + ((ULONG)pFps->FPS_uiConfigTableAddr - EBDA_START));
    } else {
        pHdr = (X86_MP_HEADER *)((ULONG)_G_pcX86BiosRomStart + ((ULONG)pFps->FPS_uiConfigTableAddr - BIOS_ROM_START));
    }

    /*
     * Show Interrupt Configuration Table Entry
     */
    printf("BIOS Local Interrupt Configuration Table\n");
    printf("  Type    Flags   Bus ID    Bus IRQ     APIC ID   APIC INT In\n");
    printf("  ====   ======   ======    =======   ==========  ===========\n");
    pucPtr = (UINT8 *)pHdr + sizeof(X86_MP_HEADER);
    for (i = 0; i < pHdr->HDR_usEntryCount; i++) {
        switch (*pucPtr) {

        case MP_ENTRY_CPU:                                              /*  Processor Entry             */
            pucPtr += sizeof(X86_MP_CPU);
            break;

        case MP_ENTRY_BUS:                                              /*  Bus Entry                   */
            pucPtr += 8;
            break;

        case MP_ENTRY_IOAPIC:                                           /*  IO APIC Entry               */
            pucPtr += sizeof(X86_MP_IOAPIC);
            break;

        case MP_ENTRY_IOINTERRUPT:                                      /*  IO Interrupt Entry          */
            pucPtr += 8;
            break;

        case MP_ENTRY_LOINTERRUPT:                                      /*  Local Interrupt Entry       */
            printf("  0x%02x   0x%04x     0x%02x       0x%02x         0x%02x         0x%02x\n",
                   *(pucPtr + 1), *(UINT16 *)(pucPtr + 2), *(pucPtr + 4), *(pucPtr + 5), *(pucPtr + 6),
                   *(pucPtr + 7));
            pucPtr += 8;
            break;

        default:                                                        /*  Unknown Entry               */
            pucPtr += 8;                                                /*  Wild guess                  */
            break;
        }
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
