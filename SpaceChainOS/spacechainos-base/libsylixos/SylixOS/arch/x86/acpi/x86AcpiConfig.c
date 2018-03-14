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
** 文   件   名: x86AcpiConfig.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2017 年 04 月 14 日
**
** 描        述: x86 体系构架 ACPI 配置相关.
*********************************************************************************************************/
#define  __SYLIXOS_PCI_DRV
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "x86AcpiLib.h"
#include "arch/x86/mpconfig/x86MpApic.h"
#include "sys/param.h"
/*********************************************************************************************************
  宏定义
*********************************************************************************************************/
#define DEBUG_LEVEL                 ACPI_DEBUG_ENABLED ? ACPI_LV_RESOURCES : 0

#define ROUNDUPLONG(arg)            ROUND_UP(arg, sizeof(LONG))
/*********************************************************************************************************
  Temporary storage size for pcMpLoApicIndexTbl and pMpApicBusTable
*********************************************************************************************************/
#define HOLD_SIZE                   255
/*********************************************************************************************************
  Local storage to save off PCI hierarchy to report the devices parent bus...
*********************************************************************************************************/
#define PCI_BUS_STACK_MAX           256
#define ACPI_MAX_DEVICE_ID_LENGTH   20
/*********************************************************************************************************
  全局变量定义
*********************************************************************************************************/
ACPI_MODULE_NAME("acpi_config")

static UINT32               _G_uiAcpiPciBusStack[PCI_BUS_STACK_MAX];
static UINT8                _G_ucAcpiMaxSubordinateBusId = 0;
static X86_MP_APIC_DATA    *_G_pAcpiMpApicData           = LW_NULL;

BOOL                        _G_bAcpiPcAtCompatible = LW_FALSE;
UINT8                       _G_ucAcpiIsaIntNr      = 0;
UINT8                       _G_ucAcpiIsaApic       = 0;
/*********************************************************************************************************
  List of IRQ base addresses for each IOAPIC
*********************************************************************************************************/
UINT8                      *_G_pucAcpiGlobalIrqBaseTable         = LW_NULL;
X86_IRQ_OVERRIDE         *(*_G_pfuncAcpiIrqOverride)(INT iIndex) = LW_NULL;
PVOID                     (*_G_pfuncAcpiIrqAppend)(INT iIndex)   = LW_NULL;
/*********************************************************************************************************
  外部函数声明
*********************************************************************************************************/
extern INT  x86PciConfigInByte(INT  iBus, INT  iSlot, INT  iFunc, INT  iOft, UINT8  *pucValue);
/*********************************************************************************************************
** 函数名称: __acpiShowIrqList
** 功能描述: 显示 IRQ 列表
** 输　入  : pucBuf        缓冲区
** 输　出  : IRQ 数目
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#ifdef ACPI_VERBOSE

static UINT32  __acpiShowIrqList (UINT8  *pucBuf)
{
    ACPI_PCI_ROUTING_TABLE  *pRoutingTable;
    UINT32                   uiCount;
    UINT32                   uiLocalAcpiDbgLevel;
    CHAR                     acBufStr[ACPI_MAX_STRING_PREFIX];

    /*
     * Show elements
     */
    ACPI_VERBOSE_PRINT(" \n");

    pRoutingTable = ACPI_CAST_PTR(ACPI_PCI_ROUTING_TABLE, pucBuf);
    for (uiCount = 0; pRoutingTable->Length; uiCount++) {
        strncpy(acBufStr, pRoutingTable->Source, (ACPI_MAX_STRING_PREFIX - 1));
        acBufStr[(ACPI_MAX_STRING_PREFIX - 1)] = 0;

        ACPI_VERBOSE_PRINT("+++\n");
        ACPI_VERBOSE_PRINT("   IRQ[%d].Length = %d\n",      uiCount, pRoutingTable->Length);
        ACPI_VERBOSE_PRINT("   IRQ[%d].Pin = %d\n",         uiCount, pRoutingTable->Pin);
        ACPI_VERBOSE_PRINT("   IRQ[%d].Address = %p\n",     uiCount, pRoutingTable->Address);
        ACPI_VERBOSE_PRINT("   IRQ[%d].SourceIndex = %d\n", uiCount, pRoutingTable->SourceIndex);
        if (!*acBufStr) {
            ACPI_VERBOSE_PRINT("   IRQ[%d].Source = [NULL NAMESTRING]\n", uiCount);
        } else {
            ACPI_VERBOSE_PRINT("   IRQ[%d].Source = %4s\n", uiCount, acBufStr);
        }
        pRoutingTable = ACPI_ADD_PTR(ACPI_PCI_ROUTING_TABLE, pRoutingTable, pRoutingTable->Length);
    }
    ACPI_VERBOSE_PRINT(" \n");

    /*
     * Dump elements (need to set debug level to see output)
     */
    uiLocalAcpiDbgLevel = AcpiDbgLevel;
    AcpiDbgLevel       |= DEBUG_LEVEL;
    AcpiRsDumpIrqList(pucBuf);
    AcpiDbgLevel        = uiLocalAcpiDbgLevel;

    return  (uiCount);
}
/*********************************************************************************************************
** 函数名称: __acpiShowMpApicBusTable
** 功能描述: 显示 MP APIC Bus 表
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __acpiShowMpApicBusTable (VOID)
{
    INT             i;
    CHAR            acBusTypeStr[6];
    X86_MP_BUS     *pMpApicBusTable;

    ACPI_VERBOSE_PRINT("\n  pMpApicBusTable\n");

    pMpApicBusTable = (X86_MP_BUS *)X86_MPAPIC_PHYS_TO_VIRT(_G_pAcpiMpApicData, MPAPIC_uiBtLoc);
    for (i = 0; i < _G_pAcpiMpApicData->MPAPIC_uiBusNr; i++) {
        lib_bcopy((CHAR *)pMpApicBusTable[i].BUS_aucBusType, acBusTypeStr, 3);
        acBusTypeStr[3] = '\0';

        ACPI_VERBOSE_PRINT("    %02d BusId: 0x%02x EntryType: %u BusType: %s\n",
                           i,
                           pMpApicBusTable[i].BUS_ucBusId,
                           pMpApicBusTable[i].BUS_ucEntryType,
                           acBusTypeStr);
    }
}

#endif                                                                  /*  ACPI_VERBOSE                */
/*********************************************************************************************************
** 函数名称: acpiConfigInit
** 功能描述: 初始化 ACPI 配置组件
** 输　入  : pcHeapBase      堆基地址
**           stHeapSize      堆大小
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  acpiConfigInit (PCHAR  pcHeapBase, size_t  stHeapSize)
{
    _G_pcAcpiOsHeapPtr  = pcHeapBase;
    _G_pcAcpiOsHeapBase = pcHeapBase;
    _G_pcAcpiOsHeapTop  = pcHeapBase + stHeapSize - 1;

    /*
     * Initialize PCI bus stack
     */
    lib_bzero((CHAR*)_G_uiAcpiPciBusStack, sizeof(_G_uiAcpiPciBusStack));
}
/*********************************************************************************************************
** 函数名称: acpiLibIntIsDuplicate
** 功能描述: 判断一个中断条目是否已经在表中
** 输　入  : ucSourceBusId     源 BUS ID
**           ucSourceBusIrq    IRQ 号
**           ucDestApicId      目的 APIC ID
**           ucDestApicIntIn   IRQ 号
** 输　出  : LW_TRUE OR LW_FALSE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static BOOL  acpiLibIntIsDuplicate (UINT8  ucSourceBusId,
                                    UINT8  ucSourceBusIrq,
                                    UINT8  ucDestApicId,
                                    UINT8  ucDestApicIntIn)
{
    X86_MP_INTERRUPT  *pMpApicInterruptTable;
    INT                iIrqIndex;

    pMpApicInterruptTable = (X86_MP_INTERRUPT *)X86_MPAPIC_PHYS_TO_VIRT(_G_pAcpiMpApicData, MPAPIC_uiItLoc);

    __ACPI_DEBUG_LOG("\n**** acpiLibIntIsDuplicate(0x%x, 0x%x, 0x%x, 0x%x) ****\n",
                     ucSourceBusId, ucSourceBusIrq, ucDestApicId, ucDestApicIntIn);

    for (iIrqIndex = 0; iIrqIndex < _G_pAcpiMpApicData->MPAPIC_uiIoIntNr; iIrqIndex++) {
        if ((pMpApicInterruptTable[iIrqIndex].INT_ucEntryType     == 3)              &&
            (pMpApicInterruptTable[iIrqIndex].INT_ucSourceBusId   == ucSourceBusId)  &&
            (pMpApicInterruptTable[iIrqIndex].INT_ucSourceBusIrq  == ucSourceBusIrq) &&
            (pMpApicInterruptTable[iIrqIndex].INT_ucDestApicId    == ucDestApicId)   &&
            (pMpApicInterruptTable[iIrqIndex].INT_ucDestApicIntIn == ucDestApicIntIn)) {
            __ACPI_DEBUG_LOG("**** Duplicate found ****\n");
            return  (LW_TRUE);
        }
    }

    return  (LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: acpiLibIoApicIndex
** 功能描述: 获得指定 IRQ 属于哪个 IOAPIC
** 输　入  : ucDestApicIntIn       IRQ 号
** 输　出  : IOAPIC ID
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  acpiLibIoApicIndex (UINT8  ucDestApicIntIn)
{
    INT  iIdIndex;

    for (iIdIndex = 0; iIdIndex < _G_pAcpiMpApicData->MPAPIC_uiIoApicNr; iIdIndex++) {
        __ACPI_DEBUG_LOG("\n**** acpiLibIoApicIndex checking 0x%x ****\n", iIdIndex);
        if (ucDestApicIntIn >= _G_pucAcpiGlobalIrqBaseTable[iIdIndex] &&
           (ucDestApicIntIn < (_G_pucAcpiGlobalIrqBaseTable[iIdIndex] + 24))) {
            break;
        }
    }

    __ACPI_DEBUG_LOG("**** acpiLibIoApicIndex returns 0x%x ****\n", iIdIndex);
    return  (iIdIndex);
}
/*********************************************************************************************************
** 函数名称: acpiLibIsaMap
** 功能描述: 为 ISA 中断向量初始化中断表
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  acpiLibIsaMap (VOID)
{
    X86_MP_INTERRUPT  *pMpApicInterruptTable;
    UINT8              ucIndex;

    __ACPI_DEBUG_LOG("\n**** acpiLibIsaMap ****\n");

    pMpApicInterruptTable = (X86_MP_INTERRUPT *)X86_MPAPIC_PHYS_TO_VIRT(_G_pAcpiMpApicData, MPAPIC_uiItLoc);
    lib_bzero((VOID *)pMpApicInterruptTable, (16 * sizeof(X86_MP_INTERRUPT)));

    _G_ucAcpiIsaIntNr = 16;

    /*
     * If dual-8259, prepend table with ISA IRQs
     */
    for (ucIndex = 0; ucIndex < 16; ucIndex++) {
        pMpApicInterruptTable[ucIndex].INT_ucEntryType     = 3;
        pMpApicInterruptTable[ucIndex].INT_ucSourceBusIrq  = ucIndex;
        pMpApicInterruptTable[ucIndex].INT_ucDestApicId    = _G_ucAcpiIsaApic;
        pMpApicInterruptTable[ucIndex].INT_ucDestApicIntIn = ucIndex;
        ACPI_VERBOSE_PRINT("  filling in pMpApicInterruptTable[%d]= 0x%08x 0x%08x\n",
                           ucIndex,
                           ((UINT32* )&pMpApicInterruptTable[ucIndex])[0],
                           ((UINT32* )&pMpApicInterruptTable[ucIndex])[1]);
    }

    _G_pAcpiMpApicData->MPAPIC_uiIoIntNr = 16;
}
/*********************************************************************************************************
** 函数名称: acpiLibIrqGet
** 功能描述: 获得 PCI link 设备的 IRQ
** 输　入  : pRoutingTable     PCI 路由表
** 输　出  : IRQ 号或 0xffffffff(失败)
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT32  acpiLibIrqGet (ACPI_PCI_ROUTING_TABLE  *pRoutingTable)
{
    ACPI_HANDLE     hObject = LW_NULL;
    ACPI_BUFFER     buf;
    ACPI_STATUS     status;
    ACPI_RESOURCE  *pResource;
    UINT32          uiType;
    UINT32          uiIrq = 0xffffffff;

    buf.Length  = ACPI_ALLOCATE_BUFFER;
    buf.Pointer = LW_NULL;

    status = AcpiGetHandle(ACPI_ROOT_OBJECT, pRoutingTable->Source, &hObject);
    if (ACPI_FAILURE(status)) {
        return  (0xffffffff);
    }

    status = AcpiGetCurrentResources(hObject, &buf);
    if (ACPI_FAILURE(status)) {
        return  (0xffffffff);
    }

    pResource = ACPI_CAST_PTR(ACPI_RESOURCE, buf.Pointer);
    do {
        uiType = pResource->Type;
        if (uiType == ACPI_RESOURCE_TYPE_IRQ) {
            uiIrq  = pResource->Data.Irq.Interrupts[0];
            break;
        }
        if (uiType == ACPI_RESOURCE_TYPE_EXTENDED_IRQ) {
            uiIrq  = pResource->Data.ExtendedIrq.Interrupts[0];
            break;
        }
        pResource = ACPI_ADD_PTR(ACPI_RESOURCE, pResource, pResource->Length);
    } while (uiType != ACPI_RESOURCE_TYPE_END_TAG);

    AcpiOsFree(buf.Pointer);

    return  (uiIrq);
}
/*********************************************************************************************************
** 函数名称: acpiLibIrqProcess
** 功能描述: 处理 IRQ 路由表
** 输　入  : pucBuf            缓冲区
**           ucSourceBusId     源 BUS ID
**           bFillinTable      是否填充到表中
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  acpiLibIrqProcess (UINT8  *pucBuf, UINT8  ucSourceBusId, BOOL  bFillinTable)
{
    ACPI_PCI_ROUTING_TABLE  *pRoutingTable;
    X86_MP_INTERRUPT        *pMpApicInterruptTable;
    UINT8                    ucSourceBusIrq, ucDestApicIntIn, ucDestApicId;
    UINT8                   *pucMpApicLogicalTable;
#ifdef ACPI_VERBOSE
    UINT32                   uiLocalAcpiDbgLevel;
#endif                                                                  /*  ACPI_VERBOSE                */
    UINT32                   uiIrq;
    INT                      iCount, iDestApicIndex;

    __ACPI_DEBUG_LOG("\n**** acpiLibIrqProcess(0x%x) ****\n", ucSourceBusId);

    if (bFillinTable) {
        pMpApicInterruptTable = (X86_MP_INTERRUPT *)X86_MPAPIC_PHYS_TO_VIRT(_G_pAcpiMpApicData, MPAPIC_uiItLoc);
        pucMpApicLogicalTable = (UINT8            *)X86_MPAPIC_PHYS_TO_VIRT(_G_pAcpiMpApicData, MPAPIC_uiLtLoc);
    }

    __ACPI_DEBUG_LOG("\n**** acpiLibIrqProcess(0x%x) ****\n", ucSourceBusId);

    pRoutingTable = ACPI_CAST_PTR(ACPI_PCI_ROUTING_TABLE, pucBuf);

    /*
     * Process elements, Exit on zero length element
     */
    for (iCount = 0; pRoutingTable->Length; iCount++) {
        ACPI_VERBOSE_PRINT("  acpiLibIrqProcess count = 0x%x"
                           "  pRoutingTable = %p \n", iCount, pRoutingTable);

        if (bFillinTable) {
            ucSourceBusIrq = (UINT8)(((pRoutingTable->Address >> 14) & 0xfc) |
                             (UINT32)( pRoutingTable->Pin & 3));
            iDestApicIndex = acpiLibIoApicIndex((UINT8)(pRoutingTable->SourceIndex));
            __ACPI_DEBUG_LOG("  iDestApicIndex = 0x%x\n", iDestApicIndex);

            ucDestApicId = pucMpApicLogicalTable[iDestApicIndex];

            /*
             * According to the ACPI spec, section 6.2.12, if the Source
             * field in a PCI Routing Table (_PRT) entry is just a zero
             * byte, then SourceIndex represents a global system interrupt
             * (GSI) to which the slot's interrupt pin is connected.
             * This value must be added to the IOAPIC's base GSI value to
             * obtain the IRQ. If Source is a string, then that string
             * specifies the PCI link device (LNKx) for the interrupt pin,
             * and SourceIndex specifies which resource from its current
             * resource list (_CRS) is the interrupt resource containing
             * the current IRQ.
             *
             * Note also that there are some bogus ACPI implementations
             * where Source is not a zero byte but SourceIndex is still a
             * non-zero value (GSI). Thus the correct heuristic to use is
             * to always check if SourceIndex is non-zero, since it will
             * always be zero for the PCI LNKx case.
             *
             * Previously, SylixOS only supported the GSI case, because it
             * is typically more common. However the PCI LNKx case, though
             * infrequently occuring, should not be ignored completely.
             * By always assuming the GSI case, we will always end up with
             * an IRQ of 0 for the PCI LNKx case, which renders the device
             * non-functional.
             */
            if (pRoutingTable->SourceIndex != 0) {
                ucDestApicIntIn = (UINT8)(pRoutingTable->SourceIndex - _G_pucAcpiGlobalIrqBaseTable[iDestApicIndex]);

            } else {
                uiIrq = acpiLibIrqGet(pRoutingTable);
                ucDestApicIntIn = (UINT8)(uiIrq & 0xff);
            }

            if (!acpiLibIntIsDuplicate(ucSourceBusId, ucSourceBusIrq, ucDestApicId, ucDestApicIntIn)) {
                pMpApicInterruptTable[_G_pAcpiMpApicData->MPAPIC_uiIoIntNr].INT_ucEntryType     = 3;
                pMpApicInterruptTable[_G_pAcpiMpApicData->MPAPIC_uiIoIntNr].INT_ucSourceBusId   = ucSourceBusId;
                pMpApicInterruptTable[_G_pAcpiMpApicData->MPAPIC_uiIoIntNr].INT_ucSourceBusIrq  = ucSourceBusIrq;
                pMpApicInterruptTable[_G_pAcpiMpApicData->MPAPIC_uiIoIntNr].INT_ucDestApicId    = \
                pucMpApicLogicalTable[iDestApicIndex];
                pMpApicInterruptTable[_G_pAcpiMpApicData->MPAPIC_uiIoIntNr].INT_ucDestApicIntIn = ucDestApicIntIn;

                ACPI_VERBOSE_PRINT("  Filling in pMpApicInterruptTable[%d]="
                                   " 0x%08x 0x%08x\n",              _G_pAcpiMpApicData->MPAPIC_uiIoIntNr,
                                   ((UINT32* )&pMpApicInterruptTable[_G_pAcpiMpApicData->MPAPIC_uiIoIntNr])[0],
                                   ((UINT32* )&pMpApicInterruptTable[_G_pAcpiMpApicData->MPAPIC_uiIoIntNr])[1]);
            }
        }

        _G_pAcpiMpApicData->MPAPIC_uiIoIntNr++;

        /*
         * Next record
         */
        pRoutingTable = ACPI_ADD_PTR(ACPI_PCI_ROUTING_TABLE, pRoutingTable, pRoutingTable->Length);
    }

#ifdef ACPI_VERBOSE
    /*
     * Dump elements (need to set debug level to see output)
     */
    uiLocalAcpiDbgLevel = AcpiDbgLevel;
    AcpiDbgLevel       |= DEBUG_LEVEL;
    AcpiRsDumpIrqList(pucBuf);
    AcpiDbgLevel        = uiLocalAcpiDbgLevel;
#endif                                                                  /*  ACPI_VERBOSE                */
}
/*********************************************************************************************************
** 函数名称: acpiLibGetBusNo
** 功能描述: 获得 ACPI 总线号
** 输　入  : hObject       对象句柄
**           puiBus        ACPI 总线号
** 输　出  : ACPI 状态
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ACPI_STATUS  acpiLibGetBusNo (ACPI_HANDLE  hObject, UINT32  *puiBus)
{
    ACPI_STATUS   status;
    ACPI_INTEGER  objRtn;

    /*
     * The PCI bus number comes from the _BBN method
     */
    status  = AcpiUtEvaluateNumericObject(METHOD_NAME__BBN, hObject, &objRtn);
    *puiBus = (UINT32)objRtn;

#ifdef ACPI_VERBOSE
    if (ACPI_FAILURE(status)) {
        ACPI_VERBOSE_PRINT("**** _BBN failed %s ****\n", AcpiUtValidateException(status));
    } else {
        ACPI_VERBOSE_PRINT("**** _BBN finds busId-%d ****\n", *puiBus);
    }
#endif                                                                  /*  ACPI_VERBOSE                */

    return  (status);
}
/*********************************************************************************************************
** 函数名称: acpiLibGetIsaIrqResources
** 功能描述: 收集 ISA 中断信息
** 输　入  : pResourceList     资源列表
**           bFillinTable      是否填充到表中
** 输　出  : ACPI 状态
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  acpiLibGetIsaIrqResources (ACPI_RESOURCE  *pResourceList, BOOL  bFillinTable)
{
    X86_MP_INTERRUPT   *pMpApicInterruptTable;
    ACPI_RSDUMP_INFO   *pTable;
    UINT32              uiType;
    UINT8              *ucTarget = LW_NULL;
    UINT8              *pucPreviousTarget;
    UINT8               ucCount, i, ucIndex = 0xff, ucTrig = 0x0, ucPol = 0x1;
    VOID               *pvResource;
    CHAR  __unused     *pcName;

    __ACPI_DEBUG_LOG("\n**** acpiLibGetIsaIrqResources ****\n");

    if (bFillinTable) {
        pMpApicInterruptTable = (X86_MP_INTERRUPT *)X86_MPAPIC_PHYS_TO_VIRT(_G_pAcpiMpApicData, MPAPIC_uiItLoc);
    }

    /*
     * Walk list and dump IRQ resource descriptor (END_TAG terminates)
     */
    do {
        /*
         * Validate Type before dispatch
         */
        uiType = pResourceList->Type;
        if (uiType > ACPI_RESOURCE_TYPE_MAX) {
            __ACPI_DEBUG_LOG("  Invalid descriptor type (%X) in resource list\n", pResourceList->Type);
            return;
        }

        if (uiType == ACPI_RESOURCE_TYPE_IRQ) {
            /*
             * Dump the resource descriptor
             */
            pvResource = &pResourceList->Data;
            pTable     = AcpiGbl_DumpResourceDispatch[uiType];

            /*
             * First table entry must contain the table length
             * (# of table entries)
             */
            ucCount = pTable->Offset;
            while (ucCount) {
                pucPreviousTarget = ucTarget;
                ucTarget          = ACPI_ADD_PTR(UINT8, pvResource, pTable->Offset);
                pcName            = pTable->Name;

                switch (pTable->Opcode) {

                case ACPI_RSD_TITLE:
                    /*
                     * Optional resource title
                     */
                    if (pTable->Name) {
                        __ACPI_DEBUG_LOG("  %s Resource\n", pcName);
                    }
                    break;

                case ACPI_RSD_UINT8:
                    __ACPI_DEBUG_LOG("  %27s : %2.2X\n", pcName, ACPI_GET8(ucTarget));
                    break;

                    /* Flags: 1-bit and 2-bit flags supported */
                case ACPI_RSD_1BITFLAG:
                    __ACPI_DEBUG_LOG("  %27s : %s\n", pcName,
                                     ACPI_CAST_PTR (CHAR, pTable->Pointer[ *ucTarget & 0x01]));
                    if (ucCount == 5) {
                        ucTrig = (UINT8)(*ucTarget & 0x01);
                    }

                    if (ucCount == 6) {
                        ucPol  = (UINT8)(*ucTarget & 0x01);
                    }
                    break;

                case ACPI_RSD_SHORTLIST:
                    /*
                     * Short byte list (single line output) for
                     * DMA and IRQ resources
                     *
                     * Note: The list length is obtained from
                     * the previous table entry
                     */
                    if (pucPreviousTarget) {
                        __ACPI_DEBUG_LOG("  %27s : ", pcName);
                        for (i = 0; i < *pucPreviousTarget; i++) {
                            __ACPI_DEBUG_LOG("  %X ", ucTarget[i]);
                            ucIndex = ucTarget[i];
                        }
                        __ACPI_DEBUG_LOG("\n");
                        ucIndex = ucTarget[0];
                    }
                    break;

                default:
                    __ACPI_DEBUG_LOG("\n---> Invalid table opcode [%X] <---\n", pTable->Opcode);
                    return;
                }

                pTable++;
                ucCount--;
            }

            /*
             * Only ISA if ActiveHigh/Edge-triggered/and less than 16
             */
            if ((ucIndex < 0x10) && (ucTrig == 0x01) && (ucPol == 0x00)) {
                if (bFillinTable &&
                    !acpiLibIntIsDuplicate((UINT8)_G_pAcpiMpApicData->MPAPIC_uiBusNr,
                                           ucIndex,
                                           _G_ucAcpiIsaApic,
                                           ucIndex)) {
                    pMpApicInterruptTable[_G_pAcpiMpApicData->MPAPIC_uiIoIntNr].INT_ucEntryType     = 3;
                    pMpApicInterruptTable[_G_pAcpiMpApicData->MPAPIC_uiIoIntNr].INT_ucSourceBusIrq  = ucIndex;
                    pMpApicInterruptTable[_G_pAcpiMpApicData->MPAPIC_uiIoIntNr].INT_ucDestApicId    = _G_ucAcpiIsaApic;
                    pMpApicInterruptTable[_G_pAcpiMpApicData->MPAPIC_uiIoIntNr].INT_ucDestApicIntIn = ucIndex;

                    /*
                     * Set to conform to specification of BUS
                     */
                    pMpApicInterruptTable[_G_pAcpiMpApicData->MPAPIC_uiIoIntNr].INT_usInterruptFlags= 0x0000;

                    ACPI_VERBOSE_PRINT("  Filling in pMpApicInterruptTable[%d]="
                                       " 0x%08x 0x%08x\n", _G_pAcpiMpApicData->MPAPIC_uiIoIntNr,
                                       ((UINT32* )&pMpApicInterruptTable[_G_pAcpiMpApicData->MPAPIC_uiIoIntNr])[0],
                                       ((UINT32* )&pMpApicInterruptTable[_G_pAcpiMpApicData->MPAPIC_uiIoIntNr])[1]);
                    _G_pAcpiMpApicData->MPAPIC_uiIoIntNr++;
                    _G_ucAcpiIsaIntNr++;

                } else {
                    _G_pAcpiMpApicData->MPAPIC_uiIoIntNr++;
                    _G_ucAcpiIsaIntNr++;
                }
            }

        }

        /*
         * Point to the next resource structure
         */
        pResourceList = ACPI_ADD_PTR(ACPI_RESOURCE, pResourceList, pResourceList->Length);

        /*
         * Exit when END_TAG descriptor is reached
         */
    } while (uiType != ACPI_RESOURCE_TYPE_END_TAG);
}
/*********************************************************************************************************
** 函数名称: __acpiIsaWalkRtn
** 功能描述: 为 ISA 中断扫描 ACPI 数据
** 输　入  : hObject           对象句柄
**           uiNestingLevel    嵌套层次
**           pvContext         上下文
**           ppvReturnValue    返回值
** 输　出  : ACPI 状态
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ACPI_STATUS  __acpiIsaWalkRtn (ACPI_HANDLE  hObject,
                                      UINT32       uiNestingLevel,
                                      VOID        *pvContext,
                                      VOID       **ppvReturnValue)
{
    ACPI_STATUS  status;
    ACPI_BUFFER  buf;

    buf.Length  = ACPI_ALLOCATE_BUFFER;
    buf.Pointer = LW_NULL;

    ACPI_VERBOSE_PRINT("\n**** __acpiIsaWalkRtn(%p) ****\n", hObject);

    status = AcpiGetCurrentResources(hObject, &buf);
    if (ACPI_FAILURE(status)) {
        ACPI_VERBOSE_PRINT("  No Current Resources for device found\n");
        if (buf.Pointer != (VOID *)LW_NULL) {
            AcpiOsFree(buf.Pointer);
        }
        return  (AE_OK);
    }

    acpiLibGetIsaIrqResources(ACPI_CAST_PTR(ACPI_RESOURCE, buf.Pointer), (BOOL)((size_t)pvContext));

    AcpiOsFree(buf.Pointer);

    return  (AE_OK);
}
/*********************************************************************************************************
** 函数名称: __acpiDeviceWalkRtn
** 功能描述: 为设备扫描 ACPI 数据
** 输　入  : hObject           对象句柄
**           uiNestingLevel    嵌套层次
**           pvContext         上下文
**           ppvReturnValue    返回值
** 输　出  : ACPI 状态
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ACPI_STATUS  __acpiDeviceWalkRtn (ACPI_HANDLE  hObject,
                                         UINT32       uiNestingLevel,
                                         VOID        *pvContext,
                                         VOID       **ppvReturnValue)
{
    ACPI_STATUS         status;
    ACPI_BUFFER         buf;
    ACPI_DEVICE_INFO   *pDevInfo = LW_NULL;
    UINT32              uiBusId;
    CHAR                acBufStr[ACPI_MAX_DEVICE_ID_LENGTH + 1];
    UINT8               ucSecBus = 0;
    BOOL                bBusFound = LW_FALSE;
    BOOL                bIsaBusFound = LW_FALSE;
    UINT8               ucBusNr;
    INT                 iDevIdLen;
    INT                 iBusNo;
    INT                 iDeviceNo;
    INT                 iFuncNo;
    UINT8               ucHeaderType;
    UINT8               ucSubBus;
#ifdef ACPI_VERBOSE
    UINT32              uiLocalAcpiDbgLevel;
#endif                                                                  /*  ACPI_VERBOSE                */
    X86_MP_BUS         *pMpApicBusTable;
    UINT8              *pMpLoApicIndexTable;
    UINT8              *pMpLoApicIndexTableOld;
    UINT8              *pcHoldMpLoApicIndexTbl;

    ACPI_VERBOSE_PRINT("\n**** __acpiDeviceWalkRtn(%p) ****\n", hObject);

    status = AcpiGetObjectInfo(hObject, &pDevInfo);
    if (ACPI_FAILURE (status) || (pDevInfo == LW_NULL)) {
        __ACPI_DEBUG_LOG("\n---> __acpiDeviceWalkRtn AcpiGetObjectInfo failure - %s <---\n",
                         AcpiFormatException(status));
        return  (AE_OK);
    }

    if (pDevInfo->Type != ACPI_TYPE_DEVICE) {
        __ACPI_DEBUG_LOG("*** Device.Type = %4s\n", pDevInfo->Type);
    }

    strncpy(acBufStr, (CHAR *)&pDevInfo->Name, 4);
    acBufStr[4] = 0;

    __ACPI_DEBUG_LOG("\n ----------- %s Device found (nest=%d) ----------- \n", acBufStr, uiNestingLevel);

    if (pDevInfo->Valid & ACPI_VALID_STA) {
        __ACPI_DEBUG_LOG("\tDevice.CurrentStatus = 0x%x\n", pDevInfo->CurrentStatus);
    }

    if (pDevInfo->Valid & ACPI_VALID_ADR) {
        __ACPI_DEBUG_LOG("\tDevice.Address = %p\n", pDevInfo->Address);

        if (_G_bAcpiPciConfigAccess) {
            if (uiNestingLevel >= PCI_BUS_STACK_MAX) {
                __ACPI_DEBUG_LOG("\tCannot get PCI bus for selected nesting level,"
                                 " please increase PCI_BUS_STACK_MAX\n");
                AcpiOsFree(pDevInfo);
                return  (AE_OK);
            }

            /*
             * If current device is behind bridge, we need to get
             * parent bridge bus number to get correct child device
             * configuration
             */
            iBusNo = (INT)_G_uiAcpiPciBusStack[uiNestingLevel];
            __ACPI_DEBUG_LOG("\tDevice parent bus = 0x%x\n", iBusNo);

            iDeviceNo = (INT)(pDevInfo->Address >> 16);
            iFuncNo   = (INT)(pDevInfo->Address & 0xffff);

            /*
             * Get PCI header type
             */
            if ((x86PciConfigInByte(iBusNo, iDeviceNo, iFuncNo,
                                    PCI_CFG_HEADER_TYPE, &ucHeaderType) == ERROR_NONE)) {
                /*
                 * Check to see if it is a bridge
                 */
                if (((ucHeaderType & PCI_HEADER_TYPE_MASK) == PCI_HEADER_TYPE_BRIDGE) ||
                    ((ucHeaderType & PCI_HEADER_TYPE_MASK) == PCI_HEADER_PCI_CARDBUS)) {
                    /*
                     * Get the secondary bus number downstream
                     */
                    if (x86PciConfigInByte(iBusNo, iDeviceNo, iFuncNo,
                                           PCI_CFG_SECONDARY_BUS, &ucSecBus) != PX_ERROR) {
                        /*
                         * Get the highest bus number downstream
                         */
                        if (x86PciConfigInByte(iBusNo, iDeviceNo, iFuncNo,
                                               PCI_CFG_SUBORDINATE_BUS, &ucSubBus) != PX_ERROR) {
                            /*
                             * Store highest bus number.
                             * Will be used to assign ISA bus number.
                             */
                            if (_G_ucAcpiMaxSubordinateBusId < ucSubBus) {
                                _G_ucAcpiMaxSubordinateBusId = ucSubBus;
                            }
                        }
                    } else {
                        ucSecBus = 0;
                    }
                }
            }
        }

        /*
         * if the device returns a non-zero secBus number
         */
        if (ucSecBus != 0) {
            bBusFound = LW_TRUE;                                        /*  We assume that it's a bus   */
            __ACPI_DEBUG_LOG("\tSecondary Bus  = 0x%x\n", ucSecBus);
        }
    }

    if (pDevInfo->Valid & ACPI_VALID_HID) {
        iDevIdLen = MIN(ACPI_MAX_DEVICE_ID_LENGTH, pDevInfo->HardwareId.Length);
        strncpy(acBufStr, (CHAR *)pDevInfo->HardwareId.String, iDevIdLen);
        acBufStr[iDevIdLen] = 0;
        __ACPI_DEBUG_LOG("\tDevice.HardwareId = %s\n", acBufStr);
    }

    if (pDevInfo->Valid & ACPI_VALID_UID) {
        iDevIdLen = MIN(ACPI_MAX_DEVICE_ID_LENGTH, pDevInfo->UniqueId.Length);
        strncpy(acBufStr, (CHAR *)pDevInfo->UniqueId.String, iDevIdLen);
        acBufStr[iDevIdLen] = 0;
        __ACPI_DEBUG_LOG("\tDevice.UniqueId = %s\n", acBufStr);
    }

    if (pDevInfo->Valid & ACPI_VALID_CID) {
        __ACPI_DEBUG_LOG("\tDevice.CompatibleIdList \n");
        __ACPI_DEBUG_LOG("\tDevice.CompatibleIdList.Count = 0x%x\n",    pDevInfo->CompatibleIdList.Count);
        __ACPI_DEBUG_LOG("\tDevice.CompatibleIdList.ListSize = 0x%x\n", pDevInfo->CompatibleIdList.ListSize);
        iDevIdLen = MIN(ACPI_MAX_DEVICE_ID_LENGTH, pDevInfo->CompatibleIdList.Ids[0].Length);
        strncpy(acBufStr, pDevInfo->CompatibleIdList.Ids[0].String, iDevIdLen);
        acBufStr[iDevIdLen] = 0;
        __ACPI_DEBUG_LOG("\tDevice.CompatibleIdList.Ids = %s\n", acBufStr);
    }

    if (pDevInfo->Valid & ACPI_VALID_SXDS) {
        __ACPI_DEBUG_LOG("\tDevice.HighestDstates = 0x%x, 0x%x, 0x%x 0x%x\n",
                         pDevInfo->HighestDstates[0],
                         pDevInfo->HighestDstates[1],
                         pDevInfo->HighestDstates[2],
                         pDevInfo->HighestDstates[3]);
    }

    buf.Length  = ACPI_ALLOCATE_BUFFER;
    buf.Pointer = LW_NULL;

    status = AcpiGetIrqRoutingTable(hObject, &buf);
    if (ACPI_FAILURE(status)) {
        __ACPI_DEBUG_LOG("\n---> AcpiGetIrqRoutingTable failed %s"
                         " buf.Length= %d <---\n",
                         AcpiUtValidateException(status), buf.Length);
        uiBusId = ucSecBus;                                             /*  It may still be a bus       */

    } else {
        bBusFound = LW_TRUE;                             /*  If it has a routing table, it must be a bus*/

        status = acpiLibGetBusNo(hObject, &uiBusId);                    /*  Try to get Bus Id           */
        if (ACPI_FAILURE(status)) {
            uiBusId = ucSecBus;
        }

        acpiLibIrqProcess((UINT8 *)((size_t)(buf.Pointer)),
                          (UINT8)uiBusId,
                          (BOOL)((size_t)pvContext));
#ifdef ACPI_VERBOSE
        __acpiShowIrqList(buf.Pointer);
#endif                                                                  /*  ACPI_VERBOSE                */
    }

    if (buf.Pointer != (VOID *)LW_NULL) {
        AcpiOsFree(buf.Pointer);
    }

    if ((BOOL)((size_t)pvContext)) {                                    /*  If second pass              */
        pMpApicBusTable = (X86_MP_BUS *)X86_MPAPIC_PHYS_TO_VIRT(_G_pAcpiMpApicData, MPAPIC_uiBtLoc);

        /*
         * Bus routing table size may change, save downstream tables for relocation
         */
        pcHoldMpLoApicIndexTbl = AcpiOsAllocate(HOLD_SIZE * sizeof(UINT8));
        if (pcHoldMpLoApicIndexTbl == LW_NULL) {
            __ACPI_DEBUG_LOG("\n---> AcpiOsAllocate pcHoldMpLoApicIndexTbl failure <---\n");
            AcpiOsFree(pDevInfo);
            return  (AE_ERROR);
        }

        pMpLoApicIndexTableOld = (UINT8 *)X86_MPAPIC_PHYS_TO_VIRT(_G_pAcpiMpApicData, MPAPIC_uiLaLoc);

        lib_bzero((VOID *)pcHoldMpLoApicIndexTbl, HOLD_SIZE * sizeof(UINT8));
        lib_bcopy((CHAR *)pMpLoApicIndexTableOld,
                  (CHAR *)pcHoldMpLoApicIndexTbl,
                  (UINT32)(_G_pAcpiMpApicData->MPAPIC_uiLaSize));
    }

    if (bBusFound) {
        bIsaBusFound = LW_FALSE;

        if ((BOOL)((size_t)pvContext)) {                                /*  If second pass              */
            for (ucBusNr = 0; ucBusNr < _G_pAcpiMpApicData->MPAPIC_uiBusNr; ucBusNr++) {
                if (pMpApicBusTable[ucBusNr].BUS_ucBusId == (UINT8)uiBusId) {
                    bIsaBusFound = LW_TRUE;
                    break;
                }
            }
        }

        if (!bIsaBusFound) {
            if ((BOOL)((size_t)pvContext)) {                            /*  If second pass              */
                pMpApicBusTable[_G_pAcpiMpApicData->MPAPIC_uiBusNr].BUS_ucBusId = (UINT8)uiBusId;
                pMpApicBusTable[_G_pAcpiMpApicData->MPAPIC_uiBusNr].BUS_ucEntryType = 1;
                /*
                 * Note:
                 * _G_pAcpiMpApicData->pMpApicBusTable[...].BUS_aucBusType
                 * is not to be LW_NULL terminated, size is always fixed to 3...
                 */
                lib_bcopy("PCI",
                          (CHAR *)pMpApicBusTable[_G_pAcpiMpApicData->MPAPIC_uiBusNr].BUS_aucBusType,
                          3);
            }

            __ACPI_DEBUG_LOG("  Found ISA bus at [0x%x] 0x%x\n", _G_pAcpiMpApicData->MPAPIC_uiBusNr, uiBusId);

            _G_pAcpiMpApicData->MPAPIC_uiBusNr++;

            /*
             * Save PCI bus on next nesting level
             */
            if (uiNestingLevel + 1 < PCI_BUS_STACK_MAX) {
                _G_uiAcpiPciBusStack[uiNestingLevel + 1] = uiBusId;
            }

            _G_pAcpiMpApicData->MPAPIC_uiBtSize = (UINT32)(_G_pAcpiMpApicData->MPAPIC_uiBusNr * sizeof(X86_MP_BUS));

            /*
             * Bus routing table size changed, downstream tables need
             * relocation
             */
            if (_G_pAcpiMpApicData->MPAPIC_uiBtLoc > 0) {
                _G_pAcpiMpApicData->MPAPIC_uiLaLoc = _G_pAcpiMpApicData->MPAPIC_uiBtLoc +
                                                     _G_pAcpiMpApicData->MPAPIC_uiBtSize;

                if (_G_pAcpiMpApicData->MPAPIC_uiLaSize > 0) {
                    if ((BOOL)((size_t)pvContext)) {                    /*  If second pass              */
                        pMpLoApicIndexTable = (UINT8 *)X86_MPAPIC_PHYS_TO_VIRT(_G_pAcpiMpApicData, MPAPIC_uiLaLoc);

                        lib_bcopy((CHAR *)pcHoldMpLoApicIndexTbl,
                                  (CHAR *)pMpLoApicIndexTable,
                                  (UINT32)_G_pAcpiMpApicData->MPAPIC_uiLaSize);
                    }
                }
            }
        }
    }

    if ((BOOL)((size_t)pvContext)) {                                    /*  If second pass              */
        AcpiOsFree(pcHoldMpLoApicIndexTbl);
    }
    AcpiOsFree(pDevInfo);

#ifdef ACPI_VERBOSE
    buf.Length  = ACPI_ALLOCATE_BUFFER;
    buf.Pointer = LW_NULL;

    status = AcpiGetCurrentResources(hObject, &buf);
    if (ACPI_FAILURE(status)) {
        __ACPI_DEBUG_LOG("  No Current Resources for device found\n");
        if (buf.Pointer != (VOID *)LW_NULL) {
            AcpiOsFree(buf.Pointer);
        }
        return  (AE_OK);
    }

    /*
     * Dump the _CRS resource list
     */
    ACPI_VERBOSE_PRINT("\n----------- Printing 'Current Resources' ----------- \n");

    uiLocalAcpiDbgLevel = AcpiDbgLevel;
    AcpiDbgLevel       |= DEBUG_LEVEL;
    AcpiRsDumpResourceList(ACPI_CAST_PTR(ACPI_RESOURCE, buf.Pointer));
    AcpiDbgLevel        = uiLocalAcpiDbgLevel;

    AcpiOsFree(buf.Pointer);

    buf.Length  = ACPI_ALLOCATE_BUFFER;
    buf.Pointer = LW_NULL;

    status = AcpiGetPossibleResources(hObject, &buf);
    if (ACPI_FAILURE(status)) {
        ACPI_VERBOSE_PRINT("  No 'Possible Resources' for device found\n");
        if (buf.Pointer != (VOID *)LW_NULL) {
            AcpiOsFree(buf.Pointer);
        }

        return  (AE_OK);
    }

    /*
     * Dump the _CRS resource list
     */
    ACPI_VERBOSE_PRINT("\n----------- Printing 'Possible Resources' ----------- \n");

    uiLocalAcpiDbgLevel = AcpiDbgLevel;
    AcpiDbgLevel       |= DEBUG_LEVEL;
    AcpiRsDumpResourceList(ACPI_CAST_PTR(ACPI_RESOURCE, buf.Pointer));
    AcpiDbgLevel        = uiLocalAcpiDbgLevel;

    AcpiOsFree(buf.Pointer);
#endif                                                                  /*  ACPI_VERBOSE                */

    return  (AE_OK);
}
/*********************************************************************************************************
** 函数名称: __acpiParseIrqOverride
** 功能描述: 分析 IRQ override
** 输　入  : ucSourceIrq       源 IRQ 号
**           uiGlobalIrq       全局 IRQ 号
**           usIntFlags        中断标志
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __acpiParseIrqOverride (UINT8   ucSourceIrq,
                                     UINT32  uiGlobalIrq,
                                     UINT16  usIntFlags,
                                     BOOL    bPrepend)
{
    X86_MP_INTERRUPT  *pMpApicInterruptTable;
    UINT8              ucIndex, ucGlobIrqIndex;
    BOOL               bFoundGlobIrq;

    pMpApicInterruptTable = (X86_MP_INTERRUPT *)X86_MPAPIC_PHYS_TO_VIRT(_G_pAcpiMpApicData, MPAPIC_uiItLoc);

    __ACPI_DEBUG_LOG("\n**** __acpiParseIrqOverride ****\n");
    __ACPI_DEBUG_LOG("\n   ucSourceIrq = 0x%x, uiGlobalIrq = 0x%x, usIntFlags = 0x%x bPrepend=0x0x\n",
                     ucSourceIrq, uiGlobalIrq, usIntFlags, bPrepend);

    /*
     * BSP generated overrides may be prepended to the list
     */
    if (bPrepend) {
        /*
         * Push elements up
         */
        for (ucIndex = (UINT8)(_G_pAcpiMpApicData->MPAPIC_uiIoIntNr); ucIndex > 0; ucIndex--) {
            pMpApicInterruptTable[ucIndex].INT_ucSourceBusIrq   = pMpApicInterruptTable[ucIndex - 1].INT_ucSourceBusIrq;
            pMpApicInterruptTable[ucIndex].INT_ucDestApicIntIn  = pMpApicInterruptTable[ucIndex - 1].INT_ucDestApicIntIn;
            pMpApicInterruptTable[ucIndex].INT_usInterruptFlags = pMpApicInterruptTable[ucIndex - 1].INT_usInterruptFlags;
            pMpApicInterruptTable[ucIndex].INT_ucEntryType      = pMpApicInterruptTable[ucIndex - 1].INT_ucEntryType;
            pMpApicInterruptTable[ucIndex].INT_ucDestApicId     = pMpApicInterruptTable[ucIndex - 1].INT_ucDestApicId;
        }

        /*
         * Add new entry to head of list
         */
        pMpApicInterruptTable[0].INT_ucSourceBusIrq    = ucSourceIrq;
        pMpApicInterruptTable[0].INT_ucDestApicIntIn   = (UINT8)uiGlobalIrq;
        pMpApicInterruptTable[0].INT_usInterruptFlags  = usIntFlags;
        pMpApicInterruptTable[0].INT_ucEntryType       = 3;
        pMpApicInterruptTable[0].INT_ucDestApicId      = _G_ucAcpiIsaApic;
        _G_pAcpiMpApicData->MPAPIC_uiIoIntNr++;

        ACPI_VERBOSE_PRINT("  filling in pMpApicInterruptTable[%d]= 0x%08x 0x%08x\n",
                           0,
                           ((UINT32* )&pMpApicInterruptTable[0])[0],
                           ((UINT32* )&pMpApicInterruptTable[0])[1]);
        return;
    }

    bFoundGlobIrq = LW_FALSE;                                           /*  Find modified GlobalIrq     */
    for (ucIndex = 0; ucIndex < _G_pAcpiMpApicData->MPAPIC_uiIoIntNr; ucIndex++) {
        if ((pMpApicInterruptTable[ucIndex].INT_ucSourceBusIrq == uiGlobalIrq) &&
            (pMpApicInterruptTable[ucIndex].INT_ucSourceBusIrq == pMpApicInterruptTable[ucIndex].INT_ucDestApicIntIn) &&
            (pMpApicInterruptTable[ucIndex].INT_ucEntryType    == 3)) {
            bFoundGlobIrq = LW_TRUE;
            ucGlobIrqIndex = ucIndex;
            ACPI_VERBOSE_PRINT("  Override found existing record 0x%x\n", ucGlobIrqIndex);
            break;
        }
    }

    if (bFoundGlobIrq) {
        /*
         * Modify entry where it is in list
         */
        pMpApicInterruptTable[ucGlobIrqIndex].INT_ucSourceBusIrq   = ucSourceIrq;
        pMpApicInterruptTable[ucGlobIrqIndex].INT_ucDestApicIntIn  = (UINT8)uiGlobalIrq;
        pMpApicInterruptTable[ucGlobIrqIndex].INT_usInterruptFlags = usIntFlags;
        pMpApicInterruptTable[ucGlobIrqIndex].INT_ucEntryType      = 3;
        pMpApicInterruptTable[ucGlobIrqIndex].INT_ucDestApicId     = _G_ucAcpiIsaApic;

    } else {
        /*
         * Override for new entry
         */
        pMpApicInterruptTable[_G_pAcpiMpApicData->MPAPIC_uiIoIntNr].INT_ucSourceBusIrq   = ucSourceIrq;
        pMpApicInterruptTable[_G_pAcpiMpApicData->MPAPIC_uiIoIntNr].INT_ucDestApicIntIn  = (UINT8)uiGlobalIrq;
        pMpApicInterruptTable[_G_pAcpiMpApicData->MPAPIC_uiIoIntNr].INT_usInterruptFlags = usIntFlags;
        pMpApicInterruptTable[_G_pAcpiMpApicData->MPAPIC_uiIoIntNr].INT_ucEntryType      = 3;
        pMpApicInterruptTable[_G_pAcpiMpApicData->MPAPIC_uiIoIntNr].INT_ucDestApicId     = _G_ucAcpiIsaApic;

        ACPI_VERBOSE_PRINT("  filling in pMpApicInterruptTable[%d]= 0x%08x 0x%08x\n",
                           _G_pAcpiMpApicData->MPAPIC_uiIoIntNr,
                           ((UINT32* )&pMpApicInterruptTable[_G_pAcpiMpApicData->MPAPIC_uiIoIntNr])[0],
                           ((UINT32* )&pMpApicInterruptTable[_G_pAcpiMpApicData->MPAPIC_uiIoIntNr])[1]);

        _G_pAcpiMpApicData->MPAPIC_uiIoIntNr++;
        _G_ucAcpiIsaIntNr++;
    }
}
/*********************************************************************************************************
** 函数名称: __acpiProcessIrqOverrides
** 功能描述: 处理 IRQ overrides
** 输　入  : pAcpiHeader       ACPI 表头
**           bParseEntries     分析条目
** 输　出  : ERROR_NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __acpiProcessIrqOverrides (ACPI_TABLE_HEADER  *pAcpiHeader, BOOL  bParseEntries)
{
    ACPI_MADT_INTERRUPT_OVERRIDE  *pIntOverride;
    ACPI_SUBTABLE_HEADER          *pApicEntry;
    ACPI_TABLE_MADT               *pMadt;
    X86_IRQ_OVERRIDE              *pIrqOveride;
    INT                            iIndex = 0;

    /*
     * Search through MADT subtable entries
     */
    pMadt = (ACPI_TABLE_MADT *)pAcpiHeader;
    for (pApicEntry = (ACPI_SUBTABLE_HEADER *)&pMadt[1];
         ((size_t)pApicEntry) < (size_t)pAcpiHeader + pAcpiHeader->Length;
         pApicEntry = (ACPI_SUBTABLE_HEADER *)((size_t)pApicEntry + pApicEntry->Length)) {

        /*
         * If entry is an override
         */
        if (pApicEntry->Type == ACPI_MADT_TYPE_INTERRUPT_OVERRIDE) {
            if (bParseEntries) {
                pIntOverride = (ACPI_MADT_INTERRUPT_OVERRIDE *)pApicEntry;
                __acpiParseIrqOverride(pIntOverride->SourceIrq,
                                       pIntOverride->GlobalIrq,
                                       pIntOverride->IntiFlags,
                                       LW_FALSE);
            } else {
                _G_pAcpiMpApicData->MPAPIC_uiIoIntNr++;
            }
        }
    }

    /*
     * If any BSP specific overrides are registered
     */
    if (_G_pfuncAcpiIrqOverride != LW_NULL) {
        while ((pIrqOveride = _G_pfuncAcpiIrqOverride(iIndex++)) != LW_NULL) {
            /*
             * Process any BSP specific overrides
             */
            if (bParseEntries) {
                __acpiParseIrqOverride(pIrqOveride->IRQ_ucSourceIrq,
                                       pIrqOveride->IRQ_uiGlobalIrq,
                                       pIrqOveride->IRQ_usIntFlags,
                                       pIrqOveride->IRQ_bPrepend);
            } else {
                _G_pAcpiMpApicData->MPAPIC_uiIoIntNr++;
            }
        }
    }

    /*
     * If any BSP specific append are registered
     */
    if (_G_pfuncAcpiIrqAppend != LW_NULL) {
        X86_MP_INTERRUPT  *pMpApicInterruptTable;
        X86_MP_INTERRUPT  *pMpInt;

        pMpApicInterruptTable = (X86_MP_INTERRUPT *)X86_MPAPIC_PHYS_TO_VIRT(_G_pAcpiMpApicData, MPAPIC_uiItLoc);

        iIndex = 0;

        while ((pMpInt = _G_pfuncAcpiIrqAppend(iIndex++)) != LW_NULL) {
            /*
             * Process any BSP specific append
             */
            if (bParseEntries) {
                pMpApicInterruptTable[_G_pAcpiMpApicData->MPAPIC_uiIoIntNr] = *pMpInt;
            }
            _G_pAcpiMpApicData->MPAPIC_uiIoIntNr++;
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __acpiParseMadt
** 功能描述: 分析 MADT 表
** 输　入  : pAcpiHeader       ACPI 表头
** 输　出  : ERROR_NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __acpiParseMadt (ACPI_TABLE_HEADER  *pAcpiHeader)
{
    ACPI_MADT_LOCAL_APIC        *pLocalApic;
    ACPI_MADT_IO_APIC           *pMadtToApic;
    ACPI_SUBTABLE_HEADER        *pApicEntry;
    ACPI_TABLE_MADT             *pMadt;
    X86_MP_INTERRUPT __unused   *pMpApicInterruptTable;
    UINT32                      *puiMpApicAddrTable;
    UINT8                       *pucMpApicLogicalTable;
    X86_MP_BUS __unused         *pMpApicBusTable;
    UINT8                       *pucMpLoApicIndexTable;
    UINT8                       ucTmp1, ucTmp2;
    UINT32                      uiTemp32;
    UINT32                      uiCpuIndex    = 0;                      /*  CPU counter                 */
    UINT32                      uiIoApicIndex = 0;                      /*  IO APIC counter             */
    INT                         iIndex1;
    INT                         iOuterIndex, iInnerIndex;

    __ACPI_DEBUG_LOG("\n**** parses MADT table... ****\n");

    pMadt = (ACPI_TABLE_MADT *)pAcpiHeader;

    _G_pAcpiMpApicData->MPAPIC_uiLocalApicBase = pMadt->Address;        /*  Default Local APIC addr     */

    /*
     * First, just count records
     * These records are found appended to the end of the MADT.
     */
    for (pApicEntry = (ACPI_SUBTABLE_HEADER *)&pMadt[1];
         ((size_t)pApicEntry) < (size_t)pAcpiHeader + pAcpiHeader->Length;
         pApicEntry = (ACPI_SUBTABLE_HEADER *)((size_t)pApicEntry + (UINT8)pApicEntry->Length)) {

        switch (pApicEntry->Type) {

        case (ACPI_MADT_TYPE_LOCAL_APIC):
            pLocalApic = (ACPI_MADT_LOCAL_APIC *)pApicEntry;
            if (pLocalApic->LapicFlags == 0) {                          /*  Non-operational APIC        */
                break;
            }
            _G_pAcpiMpApicData->MPAPIC_uiCpuCount++;
            break;

        case (ACPI_MADT_TYPE_IO_APIC):
            _G_pAcpiMpApicData->MPAPIC_uiIoApicNr++;
            break;

        default:
            break;
        }
    }

    /*
     * Allocate space for GlobalIrqBase table
     */
    _G_pucAcpiGlobalIrqBaseTable = AcpiOsAllocate(sizeof(UINT32) * _G_pAcpiMpApicData->MPAPIC_uiIoApicNr);

    /*
     * Set FPS header ID to _MP_
     */
    /*
     * Note: _G_pAcpiMpApicData->MPAPIC_Fps.signature is not to be LW_NULL
     * terminated, size is always fixed to 4...
     */
    lib_bcopy("_MP_", _G_pAcpiMpApicData->MPAPIC_Fps.FPS_acSignature, 4);

    _G_pAcpiMpApicData->MPAPIC_uiAtLoc  = _G_pAcpiMpApicData->MPAPIC_uiDataLoc + _G_pAcpiMpApicData->MPAPIC_uiDataSize;
    _G_pAcpiMpApicData->MPAPIC_uiAtSize = (UINT32)(_G_pAcpiMpApicData->MPAPIC_uiIoApicNr * sizeof(UINT32));
    puiMpApicAddrTable = (UINT32 *)X86_MPAPIC_PHYS_TO_VIRT(_G_pAcpiMpApicData, MPAPIC_uiAtLoc);

    _G_pAcpiMpApicData->MPAPIC_uiLtLoc  = _G_pAcpiMpApicData->MPAPIC_uiAtLoc + _G_pAcpiMpApicData->MPAPIC_uiAtSize;
    _G_pAcpiMpApicData->MPAPIC_uiLtSize = (UINT32)(ROUNDUPLONG(_G_pAcpiMpApicData->MPAPIC_uiIoApicNr * sizeof(UINT8)));
    pucMpApicLogicalTable = (UINT8 *)X86_MPAPIC_PHYS_TO_VIRT(_G_pAcpiMpApicData, MPAPIC_uiLtLoc);

    _G_pAcpiMpApicData->MPAPIC_uiItLoc  = _G_pAcpiMpApicData->MPAPIC_uiLtLoc + _G_pAcpiMpApicData->MPAPIC_uiLtSize;
    if (_G_bAcpiPcAtCompatible) {
        /*
         * If dual-8259, prepend table with ISA IRQs
         */
        _G_pAcpiMpApicData->MPAPIC_uiLoIntNr += 16;
    }
    _G_pAcpiMpApicData->MPAPIC_uiItSize = (UINT32)((_G_pAcpiMpApicData->MPAPIC_uiIoIntNr +
                                          _G_pAcpiMpApicData->MPAPIC_uiLoIntNr) * sizeof(X86_MP_INTERRUPT));
    pMpApicInterruptTable = (X86_MP_INTERRUPT *)X86_MPAPIC_PHYS_TO_VIRT(_G_pAcpiMpApicData, MPAPIC_uiItLoc);

    _G_pAcpiMpApicData->MPAPIC_uiBtLoc  = _G_pAcpiMpApicData->MPAPIC_uiItLoc +
                                          _G_pAcpiMpApicData->MPAPIC_uiItSize;
    _G_pAcpiMpApicData->MPAPIC_uiBtSize = (UINT32)(_G_pAcpiMpApicData->MPAPIC_uiBusNr * sizeof(X86_MP_BUS));
    pMpApicBusTable = (X86_MP_BUS *)X86_MPAPIC_PHYS_TO_VIRT(_G_pAcpiMpApicData, MPAPIC_uiBtLoc);

    _G_pAcpiMpApicData->MPAPIC_uiLaLoc  = _G_pAcpiMpApicData->MPAPIC_uiBtLoc +
                                          _G_pAcpiMpApicData->MPAPIC_uiBtSize;
    _G_pAcpiMpApicData->MPAPIC_uiLaSize = (UINT32)(_G_pAcpiMpApicData->MPAPIC_uiCpuCount * sizeof(UINT8));
    pucMpLoApicIndexTable = (UINT8 *)X86_MPAPIC_PHYS_TO_VIRT(_G_pAcpiMpApicData, MPAPIC_uiLaLoc);

    /*
     * Check sizeof current buffer
     */
    if ((_G_pAcpiMpApicData->MPAPIC_uiDataSize + _G_pAcpiMpApicData->MPAPIC_uiAtSize +
         _G_pAcpiMpApicData->MPAPIC_uiLtSize   + _G_pAcpiMpApicData->MPAPIC_uiItSize +
         _G_pAcpiMpApicData->MPAPIC_uiLaSize   + _G_pAcpiMpApicData->MPAPIC_uiBtSize) > MPAPIC_DATA_MAX_SIZE) {
        __ACPI_DEBUG_LOG("\n---> MpApicTable overrun <---\n");
        return  (PX_ERROR);
    }

    __ACPI_DEBUG_LOG("\n  _G_pAcpiMpApicData=%018p\n", _G_pAcpiMpApicData);
    __ACPI_DEBUG_LOG("  MPAPIC_uiDataLoc=0x%x\n",      _G_pAcpiMpApicData->MPAPIC_uiDataLoc);

    __ACPI_DEBUG_LOG("\n  MPAPIC_uiIoApicNr=%d\n",     _G_pAcpiMpApicData->MPAPIC_uiIoApicNr);
    __ACPI_DEBUG_LOG("  MPAPIC_uiIoIntNr=%d\n",        _G_pAcpiMpApicData->MPAPIC_uiIoIntNr);
    __ACPI_DEBUG_LOG("  MPAPIC_uiLoIntNr=%d\n",        _G_pAcpiMpApicData->MPAPIC_uiLoIntNr);
    __ACPI_DEBUG_LOG("  MPAPIC_uiBusNr=%d\n",          _G_pAcpiMpApicData->MPAPIC_uiBusNr);
    __ACPI_DEBUG_LOG("  MPAPIC_uiCpuCount=%d\n\n",     _G_pAcpiMpApicData->MPAPIC_uiCpuCount);

    __ACPI_DEBUG_LOG("  MPAPIC_uiAtSize=%d\n",         _G_pAcpiMpApicData->MPAPIC_uiAtSize);
    __ACPI_DEBUG_LOG("  MPAPIC_uiLtSize=%d\n",         _G_pAcpiMpApicData->MPAPIC_uiLtSize);
    __ACPI_DEBUG_LOG("  MPAPIC_uiItSize=%d\n",         _G_pAcpiMpApicData->MPAPIC_uiItSize);
    __ACPI_DEBUG_LOG("  MPAPIC_uiBtSize=%d\n",         _G_pAcpiMpApicData->MPAPIC_uiBtSize);
    __ACPI_DEBUG_LOG("  MPAPIC_uiLaSize=%d\n\n",       _G_pAcpiMpApicData->MPAPIC_uiLaSize);

    __ACPI_DEBUG_LOG("  MPAPIC_uiAtLoc=0x%x\n",        _G_pAcpiMpApicData->MPAPIC_uiAtLoc);
    __ACPI_DEBUG_LOG("  MPAPIC_uiLtLoc=0x%x\n",        _G_pAcpiMpApicData->MPAPIC_uiLtLoc);
    __ACPI_DEBUG_LOG("  MPAPIC_uiItLoc=0x%x\n",        _G_pAcpiMpApicData->MPAPIC_uiItLoc);
    __ACPI_DEBUG_LOG("  MPAPIC_uiBtLoc=0x%x\n",        _G_pAcpiMpApicData->MPAPIC_uiBtLoc);
    __ACPI_DEBUG_LOG("  MPAPIC_uiLaLoc=0x%x\n\n",      _G_pAcpiMpApicData->MPAPIC_uiLaLoc);

    /*
     * Now fill in local APIC logical and address tables
     */
    __ACPI_DEBUG_LOG("  Now fill in local APIC logical and address tables...\n");

    for (pApicEntry = (ACPI_SUBTABLE_HEADER *)&pMadt[1];
         ((size_t)pApicEntry) < (size_t)pAcpiHeader + pAcpiHeader->Length;
         pApicEntry = (ACPI_SUBTABLE_HEADER *)((size_t)pApicEntry + (UINT8)pApicEntry->Length)) {

        switch (pApicEntry->Type) {

        case (ACPI_MADT_TYPE_LOCAL_APIC):
            __ACPI_DEBUG_LOG("  ACPI_MADT_TYPE_LOCAL_APIC...\n");
            pLocalApic = (ACPI_MADT_LOCAL_APIC *)pApicEntry;
            if (pLocalApic->LapicFlags == 0) {
                break;                                                  /*  Non-operational APIC        */
            }

            /*
             * NEED TO ID BSP TO ADJUST TRANSLATION TABLES,
             * BSP should be only core running this code...
             */
            if (pLocalApic->Id == (UINT8)x86LocalApicId()) {
                _G_pAcpiMpApicData->MPAPIC_ucCpuBSP = pLocalApic->Id;
            }

            /*
             * Build CPU/Local Apic Id translation tbls
             */
            _G_pAcpiMpApicData->MPAPIC_aucCpuIndexTable[pLocalApic->Id] = (UINT8)uiCpuIndex;

            /*
             * Table order relevant, table is indexed logically,
             * must contain logical indicies...
             */
            ucTmp1 = pLocalApic->Id;
            for (iIndex1 = 0; iIndex1 < uiCpuIndex; iIndex1++) {
                if (ucTmp1 < pucMpLoApicIndexTable[iIndex1]) {
                    ucTmp2 = pucMpLoApicIndexTable[iIndex1];
                    pucMpLoApicIndexTable[iIndex1] = ucTmp1;
                    ucTmp1  = ucTmp2;
                }
            }
            pucMpLoApicIndexTable[uiCpuIndex] = ucTmp1;
            uiCpuIndex++;
            break;

        case (ACPI_MADT_TYPE_IO_APIC):
            __ACPI_DEBUG_LOG("  ACPI_MADT_TYPE_IO_APIC...\n");
            pMadtToApic                                 = (ACPI_MADT_IO_APIC *)pApicEntry;
            _G_pucAcpiGlobalIrqBaseTable[uiIoApicIndex] = (UINT8)pMadtToApic->GlobalIrqBase;
            puiMpApicAddrTable[uiIoApicIndex]           = (UINT32)pMadtToApic->Address;
            pucMpApicLogicalTable[uiIoApicIndex++]      = pMadtToApic->Id;
            break;

        default:
            break;
        }
    }

    /*
     * Now reorder the MPAPIC_aucCpuIndexTable so it is linear
     */
    for (iIndex1 = 0; iIndex1 < uiCpuIndex; iIndex1++) {
        /*
         * Get the Local Apic Id for CPU idx1
         */
        ucTmp1 = pucMpLoApicIndexTable[iIndex1];

        /*
         * Match the Local to index
         */
        _G_pAcpiMpApicData->MPAPIC_aucCpuIndexTable[ucTmp1] = (UINT8)iIndex1;
    }

    /*
     * Now reorder the IOAPIC entries in GlobalIrq sequence.
     * We keep these in order so that we can quicky determine
     * which IOAPIC an IRQ belongs to. (bubble sort)
     */
    __ACPI_DEBUG_LOG("  Now reorder the IOAPIC entries in GlobalIrq sequence...\n\n");

    for (iOuterIndex = 0; iOuterIndex < uiIoApicIndex; iOuterIndex++) {
        for (iInnerIndex = 0; iInnerIndex < uiIoApicIndex - 1; iInnerIndex++) {
            if (_G_pucAcpiGlobalIrqBaseTable[iInnerIndex] > _G_pucAcpiGlobalIrqBaseTable[iInnerIndex + 1]) {
                /*
                 * Swap entries
                 */
                ucTmp1 = _G_pucAcpiGlobalIrqBaseTable[iInnerIndex + 1];
                _G_pucAcpiGlobalIrqBaseTable[iInnerIndex + 1] = _G_pucAcpiGlobalIrqBaseTable[iInnerIndex];
                _G_pucAcpiGlobalIrqBaseTable[iInnerIndex]     = ucTmp1;

                uiTemp32 = puiMpApicAddrTable[iInnerIndex + 1];
                puiMpApicAddrTable[iInnerIndex + 1] = puiMpApicAddrTable[iInnerIndex];
                puiMpApicAddrTable[iInnerIndex]     = uiTemp32;

                ucTmp1 = pucMpApicLogicalTable[iInnerIndex + 1];
                pucMpApicLogicalTable[iInnerIndex + 1] = pucMpApicLogicalTable[iInnerIndex];
                pucMpApicLogicalTable[iInnerIndex]     = ucTmp1;
            }
        }
    }

    /*
     * Now that its sorted, we know that ISA IRQs are in first IOAPIC
     */
    _G_ucAcpiIsaApic = pucMpApicLogicalTable[0];

    __ACPI_DEBUG_LOG("**** parses MADT table completed... ****\n");

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: acpiLibDevScan
** 功能描述: 扫描 ACPI 设备
** 输　入  : bEarlyInit        是否早期初始化
**           bPciAccess        是否允许 PCI 访问
**           pcBuf             缓冲区
**           uiMpApicBufSize   缓冲区大小
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  acpiLibDevScan (BOOL  bEarlyInit, BOOL  bPciAccess, CHAR  *pcBuf, UINT32  uiMpApicBufSize)
{
    ACPI_STATUS             status, callbackStatus;
    ACPI_TABLE_HEADER      *pMadt;
#ifdef ACPI_VERBOSE
    ACPI_TABLE_HEADER      *pTable;
#endif                                                                  /*  ACPI_VERBOSE                */
    INT                     iIndex;
    X86_MP_INTERRUPT       *pMpApicInterruptTable;
    X86_MP_BUS             *pMpApicBusTable;
    X86_MP_BUS             *pMpApicBusTableOld;
    UINT8                  *pucMpLoApicIndexTable;
    UINT8                  *pucMpLoApicIndexTableOld;
    UINT32                 *puiHoldMpApicBusTbl;
    UINT8                  *pucHoldMpLoApicIndexTbl;
    INT                     i, j, k;
    UINT8                   ucTmp;

    _G_bAcpiEarlyAccess     = bEarlyInit;
    _G_bAcpiPciConfigAccess = bPciAccess;

    _G_ucAcpiIsaIntNr = 0;
    _G_ucAcpiIsaApic  = 0;

    __ACPI_DEBUG_LOG("\n**** acpiLibDevScan(%d, %d, %p, 0x%x) ****\n\n",
                     bEarlyInit, bPciAccess, pcBuf, uiMpApicBufSize);

    /*
     * Save pointer to static table
     */
    if (pcBuf == LW_NULL) {
        __ACPI_DEBUG_LOG("\n---> acpiLibDevScan failed, no acpiBuffer buffer <---\n");
        return  (PX_ERROR);

    } else {
        _G_pAcpiMpApicData = (X86_MP_APIC_DATA *)pcBuf;
    }

    __ACPI_DEBUG_LOG("\n  acpi get table: Madt\n");
    status = AcpiGetTable(ACPI_SIG_MADT, 0, &pMadt);
    if (ACPI_FAILURE(status)) {
        __ACPI_DEBUG_LOG("\n---> Unable to read MADT table <---\n");
        AcpiFormatException(status);
        return  (PX_ERROR);
    }

    /*
     * True if dual 8359, must fill in isa irqs
     */
    _G_bAcpiPcAtCompatible = ((ACPI_TABLE_MADT *)pMadt)->Flags;

    _G_pAcpiMpApicData->MPAPIC_uiLoIntNr = 2;                           /*  Fixed number, NMI and ExtInt*/

    /*
     * First pass, just count IO interrupts and busses
     */
    __ACPI_DEBUG_LOG("\n\n------- acpiLibDevScan (first pass) -------\n");

    if (_G_bAcpiPcAtCompatible) {
        /*
         * We'll hard-code ISA table entries
         */
        _G_ucAcpiIsaIntNr = 16;

    } else {
        /*
         * Scan ACPI database for ISA interrupts
         * third arg is flag. LW_FALSE indicates don't fill in table
         */
        __ACPI_DEBUG_LOG("\n  scan ACPI database for ISA interrupts\n");
        status = AcpiGetDevices(LW_NULL,
                                __acpiIsaWalkRtn,
                                (VOID *)LW_FALSE,
                                (VOID **)&callbackStatus);
        if (ACPI_FAILURE(status)) {
            __ACPI_DEBUG_LOG("unable to walk ISA tree %s\n", AcpiFormatException(status));
            return  (PX_ERROR);
        }
    }

    /*
     * Parse Irq Interrupt Override table
     */
    __ACPI_DEBUG_LOG("\n\n----count Irq Interrupt Override table----\n");

    /*
     * Count IRQ overrides.  At most, each will add one IOINT
     * return value of __acpiProcessIrqOverrides() not used
     */
    __acpiProcessIrqOverrides(pMadt, LW_FALSE);

    status = AcpiGetDevices(LW_NULL,
                            __acpiDeviceWalkRtn,
                            (VOID *)LW_FALSE,
                            (VOID **)&callbackStatus);
    if (ACPI_FAILURE(status)) {
        __ACPI_DEBUG_LOG("\n---> unable to walk device tree - %s <---\n", AcpiFormatException(status));
    }

    /*
     * This function allocates and fills in much of the mpApic data structure
     */
    if (__acpiParseMadt(pMadt) != ERROR_NONE) {
        __ACPI_DEBUG_LOG("\n---> Unable to process MADT table <---\n");
    }

#ifdef ACPI_VERBOSE
    status = AcpiGetTable(ACPI_SIG_FACS, 0, &pTable);
    if (ACPI_FAILURE(status)) {
        __ACPI_DEBUG_LOG("\n---> Unable to read FACS table <---\n");

    } else {
        AcpiShowFacs(pTable);
    }
#endif                                                                  /*  ACPI_VERBOSE                */

    __ACPI_DEBUG_LOG("\n\n------- acpiLibDevScan (second pass) -------\n");

    /*
     * Second pass, fill-in interrupt routing and busses
     *
     * At first gather ISA IRQ info...
     *  (No more than 16 interrupts will be reserved for ISA...)
     */
    _G_pAcpiMpApicData->MPAPIC_uiIoIntNr = 0;                           /*  Recomputed in second pass   */
    _G_pAcpiMpApicData->MPAPIC_uiBusNr   = 0;                           /*  Recomputed in second pass   */

    if (_G_bAcpiPcAtCompatible) {
        acpiLibIsaMap();                                                /*  Hard-code ISA table entries */

    } else {
        /*
         * Scan ACPI database for ISA interrupts
         * third arg is flag. LW_FALSE indicates don't fill in table
         */
        __ACPI_DEBUG_LOG("\n  scan ACPI database for ISA interrupts\n");
        status = AcpiGetDevices(LW_NULL,
                                __acpiIsaWalkRtn,
                                (VOID *)LW_TRUE,
                                (VOID **)&callbackStatus);
        if (ACPI_FAILURE(status)) {
            __ACPI_DEBUG_LOG("\n---> unable to walk ISA tree %s <---\n", AcpiFormatException(status));
            return  (PX_ERROR);
        }
    }

    /*
     * Parse Irq Interrupt Override table
     */
    __ACPI_DEBUG_LOG("\n\n----process Irq Interrupt Override table----\n");

    /*
     * Return value of __acpiProcessIrqOverrides() not used
     */
    __acpiProcessIrqOverrides(pMadt, LW_TRUE);

    status = AcpiGetDevices(LW_NULL,
                            __acpiDeviceWalkRtn,
                            (VOID *)LW_TRUE,
                            (VOID **)&callbackStatus);
    if (ACPI_FAILURE(status)) {
        __ACPI_DEBUG_LOG("\n---> unable to walk device tree - %s <---\n", AcpiFormatException(status));
    }

    /*
     * Interrupt routing table size may change, save downstream
     * tables for relocation
     */
    pucHoldMpLoApicIndexTbl = AcpiOsAllocate(HOLD_SIZE * sizeof(UINT8));
    if (pucHoldMpLoApicIndexTbl == LW_NULL) {
        __ACPI_DEBUG_LOG("\n---> AcpiOsAllocate pucHoldMpLoApicIndexTbl failure <---\n");
        return  (PX_ERROR);
    }

    puiHoldMpApicBusTbl = AcpiOsAllocate(HOLD_SIZE * sizeof(UINT32));
    if (puiHoldMpApicBusTbl == LW_NULL) {
        __ACPI_DEBUG_LOG("\n---> AcpiOsAllocate puiHoldMpApicBusTbl failure <---\n");
        AcpiOsFree(pucHoldMpLoApicIndexTbl);
        return  (PX_ERROR);
    }

    pucMpLoApicIndexTableOld = (UINT8 *)X86_MPAPIC_PHYS_TO_VIRT(_G_pAcpiMpApicData, MPAPIC_uiLaLoc);

    lib_bzero((VOID *)pucHoldMpLoApicIndexTbl, HOLD_SIZE * sizeof(UINT8));

    lib_bcopy((CHAR *)pucMpLoApicIndexTableOld,
              (CHAR *)pucHoldMpLoApicIndexTbl,
              (UINT32)(_G_pAcpiMpApicData->MPAPIC_uiLaSize));

    pMpApicBusTableOld = (X86_MP_BUS *)X86_MPAPIC_PHYS_TO_VIRT(_G_pAcpiMpApicData, MPAPIC_uiBtLoc);
    lib_bzero((VOID *)puiHoldMpApicBusTbl, HOLD_SIZE * sizeof(UINT32));
    lib_bcopy((CHAR *)pMpApicBusTableOld,
              (CHAR *)puiHoldMpApicBusTbl,
              (UINT32)(_G_pAcpiMpApicData->MPAPIC_uiBtSize));

    /*
     * Interrupt routing table size may have been changed by above
     * scans, recompute
     */
    _G_pAcpiMpApicData->MPAPIC_uiItSize = (UINT32)(((_G_pAcpiMpApicData->MPAPIC_uiIoIntNr +
                                          _G_pAcpiMpApicData->MPAPIC_uiLoIntNr) * sizeof(X86_MP_INTERRUPT)));

    _G_pAcpiMpApicData->MPAPIC_uiBtLoc  = _G_pAcpiMpApicData->MPAPIC_uiItLoc +
                                          _G_pAcpiMpApicData->MPAPIC_uiItSize;

    _G_pAcpiMpApicData->MPAPIC_uiLaLoc  = _G_pAcpiMpApicData->MPAPIC_uiBtLoc +
                                          _G_pAcpiMpApicData->MPAPIC_uiBtSize;

    if (_G_pAcpiMpApicData->MPAPIC_uiBtLoc > 0) {
        /*
         * Interrupt routing table size change, downstream tables need relocation
         */
        if (_G_pAcpiMpApicData->MPAPIC_uiLaSize > 0) {
            pucMpLoApicIndexTable = (UINT8 *)X86_MPAPIC_PHYS_TO_VIRT(_G_pAcpiMpApicData, MPAPIC_uiLaLoc);
            lib_bcopy((CHAR *)pucHoldMpLoApicIndexTbl,
                      (CHAR *)pucMpLoApicIndexTable,
                      (UINT32)_G_pAcpiMpApicData->MPAPIC_uiLaSize);
        }

        if (_G_pAcpiMpApicData->MPAPIC_uiBtSize > 0) {
            pMpApicBusTable = (X86_MP_BUS *)X86_MPAPIC_PHYS_TO_VIRT(_G_pAcpiMpApicData, MPAPIC_uiBtLoc);
            lib_bcopy((CHAR *)puiHoldMpApicBusTbl,
                      (CHAR *)pMpApicBusTable,
                      (UINT32)_G_pAcpiMpApicData->MPAPIC_uiBtSize);
        }
    }

    pMpApicInterruptTable = (X86_MP_INTERRUPT *)X86_MPAPIC_PHYS_TO_VIRT(_G_pAcpiMpApicData, MPAPIC_uiItLoc);

    /*
     * ExtINT
     */
    pMpApicInterruptTable[_G_pAcpiMpApicData->MPAPIC_uiIoIntNr].INT_ucEntryType      = 4;
    pMpApicInterruptTable[_G_pAcpiMpApicData->MPAPIC_uiIoIntNr].INT_ucInterruptType  = 3;
    pMpApicInterruptTable[_G_pAcpiMpApicData->MPAPIC_uiIoIntNr].INT_usInterruptFlags = 0;
    pMpApicInterruptTable[_G_pAcpiMpApicData->MPAPIC_uiIoIntNr].INT_ucSourceBusId    = 0;
    pMpApicInterruptTable[_G_pAcpiMpApicData->MPAPIC_uiIoIntNr].INT_ucSourceBusIrq   = 0;
    pMpApicInterruptTable[_G_pAcpiMpApicData->MPAPIC_uiIoIntNr].INT_ucDestApicId     = 0xff;
    pMpApicInterruptTable[_G_pAcpiMpApicData->MPAPIC_uiIoIntNr].INT_ucDestApicIntIn  = 0;

    /*
     * NMI
     */
    pMpApicInterruptTable[_G_pAcpiMpApicData->MPAPIC_uiIoIntNr + 1].INT_ucEntryType      = 4;
    pMpApicInterruptTable[_G_pAcpiMpApicData->MPAPIC_uiIoIntNr + 1].INT_ucInterruptType  = 1;
    pMpApicInterruptTable[_G_pAcpiMpApicData->MPAPIC_uiIoIntNr + 1].INT_usInterruptFlags = 0;
    pMpApicInterruptTable[_G_pAcpiMpApicData->MPAPIC_uiIoIntNr + 1].INT_ucSourceBusId    = 0;
    pMpApicInterruptTable[_G_pAcpiMpApicData->MPAPIC_uiIoIntNr + 1].INT_ucSourceBusIrq   = 0;
    pMpApicInterruptTable[_G_pAcpiMpApicData->MPAPIC_uiIoIntNr + 1].INT_ucDestApicId     = 0xff;
    pMpApicInterruptTable[_G_pAcpiMpApicData->MPAPIC_uiIoIntNr + 1].INT_ucDestApicIntIn  = 1;

    __ACPI_DEBUG_LOG("\n  Searching for ISA busses\n");

    pMpApicBusTable = (X86_MP_BUS *)X86_MPAPIC_PHYS_TO_VIRT(_G_pAcpiMpApicData, MPAPIC_uiBtLoc);

    /*
     * Bus routing table size may change, save downstream tables for relocation
     */
    pucMpLoApicIndexTableOld = (UINT8 *)X86_MPAPIC_PHYS_TO_VIRT(_G_pAcpiMpApicData, MPAPIC_uiLaLoc);
    lib_bzero((VOID *)pucHoldMpLoApicIndexTbl, HOLD_SIZE * sizeof(UINT8));
    lib_bcopy((CHAR *)pucMpLoApicIndexTableOld,
              (CHAR *)pucHoldMpLoApicIndexTbl,
              (UINT32)(_G_pAcpiMpApicData->MPAPIC_uiLaSize));

    /*
     * ISA bus number is _G_ucAcpiMaxSubordinateBusId + 1
     */
    __ACPI_DEBUG_LOG("    ++Found a bus!, busId: %d\n", _G_ucAcpiMaxSubordinateBusId + 1);

    __ACPI_DEBUG_LOG("  Found %d busses\n", _G_pAcpiMpApicData->MPAPIC_uiBusNr + 1);

    pMpApicBusTable[_G_pAcpiMpApicData->MPAPIC_uiBusNr].BUS_ucBusId     = (UINT8)(_G_ucAcpiMaxSubordinateBusId + 1);
    pMpApicBusTable[_G_pAcpiMpApicData->MPAPIC_uiBusNr].BUS_ucEntryType = 1;

    /*
     * Note: (mpApicBusTable[_G_pAcpiMpApicData->MPAPIC_uiBusNr].BUS_aucBusType) is not
     * to be LW_NULL terminated, size is always fixed to 3...
     */
    lib_bcopy("ISA", (CHAR *)(pMpApicBusTable[_G_pAcpiMpApicData->MPAPIC_uiBusNr].BUS_aucBusType), 3);

    /*
     * Fill-in ISA bus number
     */
    for (iIndex = 0; iIndex < _G_ucAcpiIsaIntNr; iIndex++) {
        pMpApicInterruptTable[iIndex].INT_ucSourceBusId = (UINT8)(_G_ucAcpiMaxSubordinateBusId + 1);
    }

    _G_pAcpiMpApicData->MPAPIC_uiBusNr++;                               /*  For ISA bus                 */
    _G_pAcpiMpApicData->MPAPIC_uiBtSize = (UINT32)((_G_pAcpiMpApicData->MPAPIC_uiBusNr) * sizeof(X86_MP_BUS));

    /*
     * Bus routing table size change, downstream tables need relocation
     */
    if (_G_pAcpiMpApicData->MPAPIC_uiBtLoc > 0) {
        _G_pAcpiMpApicData->MPAPIC_uiLaLoc = _G_pAcpiMpApicData->MPAPIC_uiBtLoc +
                                             _G_pAcpiMpApicData->MPAPIC_uiBtSize;
        if (_G_pAcpiMpApicData->MPAPIC_uiLaSize > 0) {
            pucMpLoApicIndexTable = (UINT8 *)X86_MPAPIC_PHYS_TO_VIRT(_G_pAcpiMpApicData, MPAPIC_uiLaLoc);
            lib_bcopy((CHAR *)pucHoldMpLoApicIndexTbl,
                      (CHAR *)pucMpLoApicIndexTable,
                      (UINT32)_G_pAcpiMpApicData->MPAPIC_uiLaSize);
        }
    }

    AcpiOsFree(pucHoldMpLoApicIndexTbl);
    AcpiOsFree(puiHoldMpApicBusTbl);

#ifdef ACPI_VERBOSE
    __acpiShowMpApicBusTable();
#endif                                                                  /*  ACPI_VERBOSE                */

    /*
     * Adjust translation tables if BIOS Setup utility has been modified
     * for BSP Selection or UP Boot Selection to relocate Boot Strap Processor.
     */
    pucMpLoApicIndexTable = (UINT8 *)X86_MPAPIC_PHYS_TO_VIRT(_G_pAcpiMpApicData, MPAPIC_uiLaLoc);
    ucTmp = pucMpLoApicIndexTable[0];
    if (ucTmp != _G_pAcpiMpApicData->MPAPIC_ucCpuBSP) {
        for (i = 1; i < _G_pAcpiMpApicData->MPAPIC_uiCpuCount; i++) {
            if (pucMpLoApicIndexTable[i] == _G_pAcpiMpApicData->MPAPIC_ucCpuBSP) {
                for (k = 0; k < i; k++) {
                    for (j = 1; j < _G_pAcpiMpApicData->MPAPIC_uiCpuCount; j++) {
                        pucMpLoApicIndexTable[j - 1] = pucMpLoApicIndexTable[j];
                    }
                    pucMpLoApicIndexTable[_G_pAcpiMpApicData->MPAPIC_uiCpuCount - 1] = ucTmp;
                    ucTmp = pucMpLoApicIndexTable[0];
                }
                break;
            }
        }

        for (i = 0; i < _G_pAcpiMpApicData->MPAPIC_uiCpuCount; i++) {
            _G_pAcpiMpApicData->MPAPIC_aucCpuIndexTable[pucMpLoApicIndexTable[i]] = (UINT8)i;
        }

        _G_uiX86BaseCpuPhysIndex = \
        (UINT)_G_pAcpiMpApicData->MPAPIC_aucCpuIndexTable[(UINT8)_G_pAcpiMpApicData->MPAPIC_ucCpuBSP];
    }

    /*
     * Dummy up defined boot structure, using ACPI as source
     */
    _G_pAcpiMpApicData->MPAPIC_uiBootOpt = ACPI_MP_STRUCT;

    __ACPI_DEBUG_LOG("**** acpiLibDevScan end ****\n");

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: x86AcpiMpTableShow
** 功能描述: 显示 ACPI 方法的 MP 配置表内容
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  x86AcpiMpTableShow (VOID)
{
    UINT32   MPAPIC_uiAtLoc, MPAPIC_uiLtLoc, MPAPIC_uiItLoc, MPAPIC_uiBtLoc, MPAPIC_uiLaLoc;
    UINT32   MPAPIC_uiAtSize, MPAPIC_uiLtSize, MPAPIC_uiItSize, MPAPIC_uiBtSize, MPAPIC_uiLaSize;
    UINT32  *MPAPIC_puiAddrTable;
    UINT32  *MPAPIC_puiLogicalTable;
    UINT32  *MPAPIC_puiInterruptTable;
    UINT32  *MPAPIC_puiBusTable;
    UINT32  *MPAPIC_puiLoApicIndexTable;
    UINT32  *puiTable;
    INT      iIndex, iEnd;

    puiTable = (UINT32 *)_G_pAcpiMpApicData;
    if (puiTable == LW_NULL) {
        return;
    }

    MPAPIC_uiAtSize = _G_pAcpiMpApicData->MPAPIC_uiAtSize;
    MPAPIC_uiLtSize = _G_pAcpiMpApicData->MPAPIC_uiLtSize;
    MPAPIC_uiItSize = _G_pAcpiMpApicData->MPAPIC_uiItSize;
    MPAPIC_uiBtSize = _G_pAcpiMpApicData->MPAPIC_uiBtSize;
    MPAPIC_uiLaSize = _G_pAcpiMpApicData->MPAPIC_uiLaSize;

    MPAPIC_uiAtLoc  = _G_pAcpiMpApicData->MPAPIC_uiAtLoc;
    MPAPIC_uiLtLoc  = _G_pAcpiMpApicData->MPAPIC_uiLtLoc;
    MPAPIC_uiItLoc  = _G_pAcpiMpApicData->MPAPIC_uiItLoc;
    MPAPIC_uiBtLoc  = _G_pAcpiMpApicData->MPAPIC_uiBtLoc;
    MPAPIC_uiLaLoc  = _G_pAcpiMpApicData->MPAPIC_uiLaLoc;

    MPAPIC_puiAddrTable        = (UINT32 *)((size_t)X86_MPAPIC_PHYS_TO_VIRT(_G_pAcpiMpApicData, MPAPIC_uiAtLoc));
    MPAPIC_puiLogicalTable     = (UINT32 *)((size_t)X86_MPAPIC_PHYS_TO_VIRT(_G_pAcpiMpApicData, MPAPIC_uiLtLoc));
    MPAPIC_puiInterruptTable   = (UINT32 *)((size_t)X86_MPAPIC_PHYS_TO_VIRT(_G_pAcpiMpApicData, MPAPIC_uiItLoc));
    MPAPIC_puiBusTable         = (UINT32 *)((size_t)X86_MPAPIC_PHYS_TO_VIRT(_G_pAcpiMpApicData, MPAPIC_uiBtLoc));
    MPAPIC_puiLoApicIndexTable = (UINT32 *)((size_t)X86_MPAPIC_PHYS_TO_VIRT(_G_pAcpiMpApicData, MPAPIC_uiLaLoc));

    /*
     * Print X86_MP_APIC_DATA data structure
     */
    printf("\n/* X86_MP_APIC_DATA data structure */\n\n");
    printf("_G_pAcpiMpApicData[] =\n{\n");

    printf("/* instance pointer */\n\n");
    printf("%p,\n\n", _G_pAcpiMpApicData);

    printf("/* MPAPIC_uiDataLoc: mem location of X86_MP_APIC_DATA */\n\n");
    printf("0x%08x,\n\n", *puiTable++);

    printf("/* MPAPIC_uiDataSize: size of X86_MP_APIC_DATA */\n\n");
    printf("0x%08x,\n\n", *puiTable++);

    printf("/* NONE/MP/ACPI/USR boot structure used */\n\n");
    printf("0x%08x,\n\n", *puiTable++);

    printf("/* MP Floating Pointer Structure */\n\n");
    printf("0x%08x, 0x%08x, 0x%08x, 0x%08x,\n\n", puiTable[0], puiTable[1], puiTable[2], puiTable[3]);
    puiTable += 4;

    printf("/* MP Configuration Table Header */\n\n");
    printf("0x%08x, 0x%08x, 0x%08x, 0x%08x,\n", puiTable[0], puiTable[1], puiTable[2], puiTable[3]);
    puiTable += 4;
    printf("0x%08x, 0x%08x, 0x%08x, 0x%08x,\n", puiTable[0], puiTable[1],puiTable[2], puiTable[3]);
    puiTable += 4;
    printf("0x%08x, 0x%08x, 0x%08x\n\n", puiTable[0], puiTable[1], puiTable[2]);
    puiTable += 3;

    printf("/* MPAPIC_uiLocalApicBase: Default Local APIC addr */\n\n");
    printf("0x%08x,\n\n", *puiTable++);

    printf("/* MPAPIC_uiIoApicNr: number of IO APICs */\n\n");
    printf("0x%08x,\n\n", *puiTable++);

    printf("/* Logical IO APIC ID to address mapping */\n\n");
    printf("MPAPIC_puiAddrTable, (this field not used under 64 bits) \n\n");
    puiTable++;

    printf("/* MPAPIC_uiAtLoc: mem location of MPAPIC_puiAddrTable */\n\n");
    printf("0x%08x,\n\n", *puiTable++);

    printf("/* MPAPIC_uiAtSize: size of MPAPIC_puiAddrTable */\n\n");
    printf("0x%08x,\n\n", *puiTable++);

    printf("/* recorded id mapping */\n\n");
    printf("MPAPIC_pucLogicalTable, (this field not used under 64 bits) \n\n");
    puiTable++;

    printf("/* MPAPIC_uiLtLoc: mem location of MPAPIC_pucLogicalTable */\n\n");
    printf("0x%08x,\n\n", *puiTable++);

    printf("/* MPAPIC_uiLtSize: size of MPAPIC_pucLogicalTable */\n\n");
    printf("0x%08x,\n\n", *puiTable++);

    printf("/* MPAPIC_uiIoIntNr: number of io interrupts (MP Table) */\n\n");
    printf("0x%08x,\n\n", *puiTable++);

    printf("/* MPAPIC_uiLoIntNr: number of local interrupts (MP Table) */\n\n");
    printf("0x%08x,\n\n", *puiTable++);

    printf("/* interrupt routing table */\n\n");
    printf("MPAPIC_pInterruptTable, (this field not used under 64 bits) \n\n");
    puiTable++;

    printf("/* MPAPIC_uiItLoc: mem location of MPAPIC_pInterruptTable */\n\n");
    printf("0x%08x,\n\n", *puiTable++);

    printf("/* MPAPIC_uiItSize: size of MPAPIC_pInterruptTable */\n\n");
    printf("0x%08x,\n\n", *puiTable++);

    printf("/* MPAPIC_uiBusNr: number of buses (MP Table) */\n\n");
    printf("0x%08x,\n\n", *puiTable++);

    printf("/* BUS routing table */\n\n");
    printf("MPAPIC_pBusTable, (this field not used under 64 bits) \n\n");
    puiTable++;

    printf("/* MPAPIC_uiBtLoc: mem location of MPAPIC_pBusTable */\n\n");
    printf("0x%08x,\n\n", *puiTable++);

    printf("/* MPAPIC_uiBtSize: size of MPAPIC_pBusTable */\n\n");
    printf("0x%08x,\n\n", *puiTable++);

    printf("/* Local APIC ID translation */\n\n");
    printf("MPAPIC_pucLoApicIndexTable, (this field not used under 64 bits) \n\n");
    puiTable++;

    printf("/* MPAPIC_uiLaLoc: mem location of MPAPIC_pucLoApicIndexTable */\n\n");
    printf("0x%08x,\n\n", *puiTable++);

    printf("/* MPAPIC_uiLaSize: size of MPAPIC_pucLoApicIndexTable */\n\n");
    printf("0x%08x,\n\n", *puiTable++);

    printf("\n");

    printf("/* CPU ID translation --Reserved for non-SMP images */\n\n");
    for (iIndex = 0; iIndex < 64; iIndex++) {
        printf("0x%08x, ", *puiTable++);
        if (iIndex % 4 == 3) {
            printf("\n");
        }
    }
    printf("\n");

    printf("/* CPU counter --Reserved for non-SMP images */\n\n");
    printf("0x%08x,\n\n", *puiTable++);

    printf("/* Boot Strap Processor local APIC ID */\n\n");
    printf("0x%08x\n\n};\n\n", *puiTable++);

    /*
     * Print variable sized tables
     */
    printf("\n\n/* MPAPIC_puiAddrTable table */\n");

    printf("\n   MPAPIC_uiAtLoc:  0x%08x\n", MPAPIC_uiAtLoc);
    printf("   MPAPIC_uiAtSize: 0x%08x\n",   MPAPIC_uiAtSize);

    iEnd = (INT)(ROUND_UP(MPAPIC_uiAtSize, 4) / 4);
    printf("\nUINT32 MPAPIC_puiAddrTable[] =\n{\n");
    for (iIndex = 0; iIndex < iEnd; iIndex++) {
        if (iIndex == (iEnd - 1)) {
            printf("0x%08x ",  MPAPIC_puiAddrTable[iIndex]);
        } else {
            printf("0x%08x, ", MPAPIC_puiAddrTable[iIndex]);
        }
        if (iIndex % 4 == 3) {
            printf("\n");
        }
    }

    printf("\n};\n");

    printf("\n\n/* MPAPIC_puiLogicalTable table */\n");

    printf("\n   MPAPIC_uiLtLoc:  0x%08x\n", MPAPIC_uiLtLoc);
    printf("   MPAPIC_uiLtSize: 0x%08x\n",   MPAPIC_uiLtSize);

    iEnd = (INT)(ROUND_UP(MPAPIC_uiLtSize, 4) / 4);
    printf("\nUINT32 MPAPIC_puiLogicalTable[] =\n{\n");

    for (iIndex = 0; iIndex < iEnd; iIndex++) {
        if (iIndex == (iEnd - 1)) {
            printf("0x%08x ",  MPAPIC_puiLogicalTable[iIndex]);
        } else {
            printf("0x%08x, ", MPAPIC_puiLogicalTable[iIndex]);
        }
        if (iIndex % 4 == 3) {
            printf("\n");
        }
    }

    printf("\n};\n");

    printf("\n\n/* MPAPIC_puiInterruptTable table */\n");

    printf("\n   MPAPIC_uiItLoc:  0x%08x\n", MPAPIC_uiItLoc);
    printf("   MPAPIC_uiItSize: 0x%08x\n",   MPAPIC_uiItSize);

    iEnd = (INT)(ROUND_UP(MPAPIC_uiItSize, 4) / 4);
    printf("\nUINT32 MPAPIC_puiInterruptTable[] =\n{\n");

    for (iIndex = 0; iIndex < iEnd; iIndex++) {
        if (iIndex == (iEnd - 1)) {
            printf("0x%08x ",  MPAPIC_puiInterruptTable[iIndex]);
        } else {
            printf("0x%08x, ", MPAPIC_puiInterruptTable[iIndex]);
        }
        if (iIndex % 4 == 3) {
            printf("\n");
        }
    }

    printf("\n};\n");

    printf("\n\n/* MPAPIC_puiBusTable table */\n");

    printf("\n   MPAPIC_uiBtLoc:  0x%08x\n", MPAPIC_uiBtLoc);
    printf("   MPAPIC_uiBtSize: 0x%08x\n",   MPAPIC_uiBtSize);

    iEnd = (INT)(ROUND_UP(MPAPIC_uiBtSize, 4) / 4);
    printf("\nUINT32 MPAPIC_puiBusTable[] =\n{\n");

    for (iIndex = 0; iIndex < iEnd; iIndex++) {
        if (iIndex == (iEnd - 1)) {
            printf("0x%08x ",  MPAPIC_puiBusTable[iIndex]);
        } else {
            printf("0x%08x, ", MPAPIC_puiBusTable[iIndex]);
        }
        if (iIndex % 4 == 3) {
            printf("\n");
        }
    }

    printf("};\n");

    printf("\n\n/* MPAPIC_puiLoApicIndexTable table */\n");

    printf("\n   MPAPIC_uiLaLoc:  0x%08x\n", MPAPIC_uiLaLoc);
    printf("   MPAPIC_uiLaSize: 0x%08x\n",   MPAPIC_uiLaSize);

    iEnd = (INT)(ROUND_UP(MPAPIC_uiLaSize, 4) / 4);
    printf("\nUINT32 MPAPIC_puiLoApicIndexTable[] =\n{\n");

    for (iIndex = 0; iIndex < iEnd; iIndex++) {
        if (iIndex == (iEnd - 1)) {
            printf("0x%08x ",  MPAPIC_puiLoApicIndexTable[iIndex]);
        } else {
            printf("0x%08x, ", MPAPIC_puiLoApicIndexTable[iIndex]);
        }
        if (iIndex % 4 == 3) {
            printf("\n");
        }
    }

    printf("};\n");
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
