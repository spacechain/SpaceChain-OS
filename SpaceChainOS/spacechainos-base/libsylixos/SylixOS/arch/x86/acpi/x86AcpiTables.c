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
** 文   件   名: x86AcpiTables.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2017 年 04 月 14 日
**
** 描        述: x86 体系构架 ACPI 表相关.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "x86AcpiLib.h"
#include "arch/x86/mpconfig/x86MpApic.h"
/*********************************************************************************************************
  全局变量定义
*********************************************************************************************************/
ACPI_MODULE_NAME("acpi_tables")

static CHAR         *_G_pcAcpiTableStart = LW_NULL;
static CHAR         *_G_pcAcpiTableEnd   = LW_NULL;

CHAR                *_G_pcAcpiRsdpPtr    = LW_NULL;
ACPI_TABLE_HPET     *_G_pAcpiHpet        = LW_NULL;
ACPI_TABLE_MADT     *_G_pAcpiMadt        = LW_NULL;
ACPI_TABLE_FACS     *_G_pAcpiFacs        = LW_NULL;
ACPI_TABLE_RSDT     *_G_pAcpiRsdt        = LW_NULL;
ACPI_TABLE_XSDT     *_G_pAcpiXsdt        = LW_NULL;
ACPI_TABLE_FADT     *_G_pAcpiFadt        = LW_NULL;
/*********************************************************************************************************
** 函数名称: acpiChecksum
** 功能描述: 计算一个 ACPI 内存块的校验和
** 输　入  : pucBuffer         缓冲区开始地址
**           uiLength          缓冲区长度
** 输　出  : 8 位校验和
** 全局变量:
** 调用模块:
*********************************************************************************************************/
UINT8  acpiChecksum (const UINT8  *pucBuffer, UINT32  uiLength)
{
    const UINT8 *pucEnd = pucBuffer + uiLength;
          UINT8  ucSum  = 0;

    while (pucBuffer < pucEnd) {
        ucSum = (UINT8)(ucSum + (UINT8)*pucBuffer);
        pucBuffer++;
    }

    return  (ucSum);
}
/*********************************************************************************************************
** 函数名称: acpiScan
** 功能描述: 搜索表头签名
** 输　入  : pcStart       开始地址
**           pcEnd         结束地址
**           pcSignature   签名
** 输　出  : 表头签名的地址或 LW_NULL
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static CHAR  *acpiScan (CHAR  *pcStart, CHAR  *pcEnd, CHAR  *pcSignature)
{
    /*
     * Align start address
     */
    pcStart = (CHAR *)((ULONG)pcStart & ~0xf);

    __ACPI_DEBUG_LOG("\n**** acpiScan (%p, %p, %s) strlen=%d ****\n",
            pcStart, pcEnd, pcSignature, lib_strlen(pcSignature));

    pcEnd = (CHAR *)((ULONG)pcEnd - lib_strlen(pcSignature));
    if (pcStart <= pcEnd) {
        do  {
            if (lib_memcmp(pcStart, pcSignature, lib_strlen(pcSignature))) {
                continue;
            }
            __ACPI_DEBUG_LOG("**** acpiScan returns %p\n ****", pcStart);
            return  (pcStart);
        } while ((pcStart += 16) < pcEnd);
    }

    __ACPI_DEBUG_LOG("\n---> acpiScan unable to find table <---\n");
    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: acpiTableValidate
** 功能描述: 判断 ACPI 表内容是否有效
** 输　入  : pucBuffer     缓冲区开始地址
**           uiLength      长度
**           pcSignature   签名
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  acpiTableValidate (UINT8  *pucBuffer, UINT32  uiLength, const CHAR  *pcSignature)
{
    ACPI_TABLE_HEADER  *pAcpiHeader = (ACPI_TABLE_HEADER *)pucBuffer;

    __ACPI_DEBUG_LOG("\n**** acpiTableValidate ****\n");

    if (pAcpiHeader == LW_NULL) {
        __ACPI_DEBUG_LOG("\n---> acpiTableValidate NULL pointer <---\n");
        return  (PX_ERROR);
    }

    if ((pcSignature != LW_NULL) &&
        (lib_strncmp(pcSignature, pAcpiHeader->Signature, lib_strlen(pcSignature)) != 0)) {
        __ACPI_DEBUG_LOG("\n---> acpiTableValidate wrong signature %s <---\n", pcSignature);
        return  (PX_ERROR);
    }

    /*
     * Verify checksum (not checked for FACS)
     */
    if (!(acpiIsFacsTable(pAcpiHeader))) {
        uiLength = pAcpiHeader->Length;
        /*
         * If table size is bogus
         */
        if ((uiLength < sizeof(ACPI_TABLE_HEADER)) || (uiLength > 0x10000)) {
            __ACPI_DEBUG_LOG("\n---> acpiTableValidate bad size 0x%x <---\n", uiLength);
            return  (PX_ERROR);
        }

        /*
         * Table has a standard header and is not bogus - get checksum
         */
        if (acpiChecksum((UINT8 *)pAcpiHeader, uiLength) != 0) {
            INT  i;
            __ACPI_DEBUG_LOG("\n---> acpiTableValidate bad checksum for 0x%x <---\n", pAcpiHeader);
            for (i = 0; i < uiLength; i++) {
                __ACPI_DEBUG_LOG("0x%8x\n", ((UINT32 * )pAcpiHeader)[i]);
            }
            return  (PX_ERROR);
        }
    }

    __ACPI_DEBUG_LOG("**** acpiTableValidate completed: pAcpiHeader %p, pSignature %s ****\n",
            pAcpiHeader, pcSignature);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: acpiRsdpScanBlock
** 功能描述: 在内存块里查找根系统描述符
** 输　入  : pcStart       开始地址
**           uiBufLength   缓冲区长度
** 输　出  : 描述符的开始地址或 LW_NULL
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT8  *acpiRsdpScanBlock (CHAR  *pcStart, UINT32  uiBufLength)
{
    ACPI_TABLE_RSDP  *pRsdp;
    UINT32            uiLength;
    CHAR             *pcEnd = pcStart + uiBufLength;

    __ACPI_DEBUG_LOG("\n**** acpiRsdpScanBlock ****\n");

    /*
     * Search from given start address for the requested length
     */
    __ACPI_DEBUG_LOG("  Find the RSDP header\n");
    do {
        /*
         * Find the RSDP header
         */
        pRsdp = (ACPI_TABLE_RSDP *)acpiScan(pcStart, pcEnd, ACPI_SIG_RSDP);
        if (pRsdp == LW_NULL) {
            break;
        }

        uiLength = pRsdp->Revision < 2 ? ACPI_RSDP_CHECKSUM_LENGTH : ACPI_RSDP_XCHECKSUM_LENGTH;
        if (acpiChecksum((UINT8 *)pRsdp, uiLength) == 0) {
            break;
        }

        pcStart = ((CHAR *)pRsdp) + lib_strlen(ACPI_SIG_RSDP);
        __ACPI_DEBUG_LOG("  acpiRsdpScanBlock finds invalid block %p\n", pRsdp);
    } while (pcStart < pcEnd);

    __ACPI_DEBUG_LOG("**** acpiRsdpScanBlock returns %p ****\n", pRsdp);
    return  ((UINT8 *)pRsdp);
}
/*********************************************************************************************************
** 函数名称: acpiFindRsdp
** 功能描述: 查找根系统描述符指针
** 输　入  : NONE
** 输　出  : 根系统描述符指针或 LW_NULL
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ACPI_TABLE_RSDP  *acpiFindRsdp (VOID)
{
    ACPI_TABLE_RSDP  *pRsdp = LW_NULL;
    __ACPI_DEBUG_LOG("\n**** acpiFindRsdp ****\n");

    /*
     * TODO: If already provided by EFI
     */
#if 0
    pRsdp = LW_NULL;
    pcEfi = (CHAR *)ACPI_UEFI_RSDP_SIG;
    if (lib_memcmp(pcEfi, ACPI_SIG_RSDP, strlen(ACPI_SIG_RSDP)) == 0) {
        pRsdp = (ACPI_TABLE_RSDP *)pcEfi;
    }

    if (pRsdp != LW_NULL) {
        _G_pcRsdpPtr = *((CHAR **)ACPI_EFI_RSDP);
        pRsdp = (ACPI_TABLE_RSDP *)_G_pcRsdpPtr;
        __ACPI_DEBUG_LOG("**** acpiFindRsdp returns %p ****\n", pRsdp);
        return  (pRsdp);
    }
#endif

    /*
     * Extended BIOS Data Area (EBDA)
     */
    __ACPI_DEBUG_LOG("  Scan Extended BIOS Data Area (EBDA)\n");
    pRsdp = (ACPI_TABLE_RSDP *)
            acpiRsdpScanBlock((CHAR *)ACPI_EBDA_PTR_LOCATION,
                              ACPI_EBDA_PTR_LENGTH);

    if (pRsdp != LW_NULL) {
        return  (pRsdp);
    }

    /*
     * Search upper memory: 16-byte boundaries in E0000h-FFFFFh
     */
    if (pRsdp == LW_NULL) {
        __ACPI_DEBUG_LOG("\n  Scan upper memory: 16-byte boundaries in E0000h-FFFFFh\n");
        pRsdp = (ACPI_TABLE_RSDP *)
                acpiRsdpScanBlock((CHAR *)ACPI_HI_RSDP_WINDOW_BASE,
                                  ACPI_HI_RSDP_WINDOW_SIZE);
    }

    _G_pcAcpiRsdpPtr = (CHAR *)pRsdp;

    __ACPI_DEBUG_LOG("**** acpiFindRsdp returns %p ****\n", pRsdp);

    return  (pRsdp);
}
/*********************************************************************************************************
** 函数名称: acpiIsAmlTable
** 功能描述: 判断是否为 AML 表
** 输　入  : NONE
** 输　出  : LW_TRUE OR LW_FALSE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  acpiIsAmlTable (const ACPI_TABLE_HEADER  *pTable)
{
    /*
     * These are the only tables that contain executable AML
     */
    if (ACPI_COMPARE_NAME(pTable->Signature, ACPI_SIG_DSDT) ||
        ACPI_COMPARE_NAME(pTable->Signature, ACPI_SIG_PSDT) ||
        ACPI_COMPARE_NAME(pTable->Signature, ACPI_SIG_SSDT)) {
        return  (LW_TRUE);
    }

    return  (LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: acpiIsFacsTable
** 功能描述: 判断是否为 FACS 表
** 输　入  : NONE
** 输　出  : LW_TRUE OR LW_FALSE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  acpiIsFacsTable (const ACPI_TABLE_HEADER  *pTable)
{
    if (ACPI_COMPARE_NAME(pTable->Signature, ACPI_SIG_FACS)) {
        return  (LW_TRUE);
    }

    return  (LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: acpiIsRsdpTable
** 功能描述: 判断是否为 RSDP 表
** 输　入  : NONE
** 输　出  : LW_TRUE OR LW_FALSE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  acpiIsRsdpTable (const ACPI_TABLE_RSDP  *pTable)
{
    if (ACPI_COMPARE_NAME(pTable->Signature, ACPI_SIG_RSDP)) {
        return  (LW_TRUE);
    }

    return  (LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: acpiGetTableSize
** 功能描述: 获得表的大小
** 输　入  : NONE
** 输　出  : 表的大小(0 为表有误)
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ACPI_SIZE  acpiGetTableSize (ACPI_PHYSICAL_ADDRESS  where)
{
    ACPI_TABLE_HEADER  *pAcpiHeader;
    ACPI_TABLE_FACS    *pFacs = LW_NULL;
    ACPI_TABLE_RSDP    *pRsdp = LW_NULL;
    ACPI_SIZE           size = 0;

    /*
     * Map a 4K page to have access to the table header
     */
    pAcpiHeader = AcpiOsMapMemory((ACPI_PHYSICAL_ADDRESS)where, 0x1000);
    if (pAcpiHeader == LW_NULL) {
        return  (0);
    }

    /*
     * FACS does not conform to the standard header format
     */
    if (acpiIsFacsTable(pAcpiHeader)) {
        pFacs = (ACPI_TABLE_FACS *)pAcpiHeader;
        size  = pFacs->Length;

    } else if (acpiIsRsdpTable((ACPI_TABLE_RSDP *)pAcpiHeader)) {
        /*
         * RSDP table header is different than the standard table header
         */
        pRsdp = (ACPI_TABLE_RSDP *)pAcpiHeader;
        /*
         * Careful! Only with ACPI 2.0 and higher does
         * the RSDP have a length field. If the revision field
         * indicates the RSDP is earlier than 2.0, we have to
         * return a hard coded size.
         */
        if (pRsdp->Revision >= 2) {
            size = pRsdp->Length;
        } else {
            size = sizeof(ACPI_TABLE_RSDP);
        }

    } else {
        /*
         * For standard Headers
         */
        size = pAcpiHeader->Length;
    }

    /*
     * Filter out tables with invalid lengths
     * Example: The FACS table on the NORCO is not valid.
     */
    if (size < sizeof(ACPI_TABLE_HEADER) || (size > 0x20000)) {
        /*
         * Return 0 length for all bogus tables, so that we
         * know not to allocate space for it.
         */
        size = 0;
    }

    /*
     * Now unmap the 4K page again
     */
    AcpiOsUnmapMemory(pAcpiHeader, 0x1000);

    return  (size);
}
/*********************************************************************************************************
** 函数名称: acpiTableRegister
** 功能描述: 通过地址范围注册一个 ACPI 表
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  acpiTableRegister (ACPI_TABLE_HEADER  *pAcpiHeader,
                                ACPI_TABLE_HEADER  *pPhysAcpiHeader,
                                CHAR               **ppcTableStart,
                                CHAR               **ppcTableEnd)
{
    CHAR  *pcStart;
    CHAR  *pcEnd;

    __ACPI_DEBUG_LOG("\n**** acpiTableRegister for address %p ****\n", pAcpiHeader);

    /*
     * If the table address looks valid
     */
    if ((*ppcTableStart == LW_NULL) ||
        (((ULONG)pAcpiHeader    - (ULONG)*ppcTableStart) < 0x100000) ||
        (((ULONG)*ppcTableStart - (ULONG)pAcpiHeader)    < 0x100000)) {
        /*
         * Recompute global start and end addresses
         */
        pcStart = (CHAR *)pAcpiHeader;
        pcEnd   = pcStart + pAcpiHeader->Length;

        if ((*ppcTableStart == LW_NULL) || (pcStart < *ppcTableStart)) {
            *ppcTableStart  = pcStart;
        }

        if ((*ppcTableEnd == LW_NULL) || (pcEnd > *ppcTableEnd)) {
            *ppcTableEnd  = pcEnd;
        }

        /*
         * There are a few tables that we might want to access quickly at a
         * later time.  We save these table address pointers.
         */
        if (ACPI_NAME_COMPARE(pAcpiHeader->Signature, ACPI_SIG_HPET)) {
            _G_pAcpiHpet = (ACPI_TABLE_HPET *)pPhysAcpiHeader;

        } else if (ACPI_NAME_COMPARE(pAcpiHeader->Signature, ACPI_SIG_MADT)) {
            _G_pAcpiMadt = (ACPI_TABLE_MADT *)pPhysAcpiHeader;

        } else if (ACPI_NAME_COMPARE(pAcpiHeader->Signature, ACPI_SIG_FACS)) {
            _G_pAcpiFacs = (ACPI_TABLE_FACS *)pPhysAcpiHeader;

        } else if (ACPI_NAME_COMPARE(pAcpiHeader->Signature, ACPI_SIG_FADT)) {
            _G_pAcpiFadt = (ACPI_TABLE_FADT *)pPhysAcpiHeader;

        } else if (ACPI_NAME_COMPARE(pAcpiHeader->Signature, ACPI_SIG_RSDT)) {
            _G_pAcpiRsdt = (ACPI_TABLE_RSDT *)pPhysAcpiHeader;

        } else if (ACPI_NAME_COMPARE(pAcpiHeader->Signature, ACPI_SIG_XSDT)) {
            _G_pAcpiXsdt = (ACPI_TABLE_XSDT *)pPhysAcpiHeader;
        }

    } else {
        __ACPI_DEBUG_LOG("\n---> acpiTableRegister address invalid %p <---\n", pAcpiHeader);
    }
}
/*********************************************************************************************************
** 函数名称: acpiTableInit
** 功能描述: 查找 ACPI 的表并映射它们到内存
** 输　入  : NONE
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  acpiTableInit (VOID)
{
    CHAR               *pcAcpiTStart    = LW_NULL;
    ACPI_TABLE_HEADER  *pAcpiHeader     = LW_NULL;
    ACPI_TABLE_HEADER  *pPhysAcpiHeader = LW_NULL;
    ACPI_TABLE_RSDP    *pRsdp           = LW_NULL;
    ACPI_TABLE_RSDT    *pRsdt           = LW_NULL;
    ACPI_TABLE_XSDT    *pXsdt           = LW_NULL;
    ACPI_TABLE_FADT    *pFadt           = LW_NULL;
    ACPI_TABLE_FACS    *pFacs           = LW_NULL;
    ACPI_TABLE_FACS    *pDsdt           = LW_NULL;
    ACPI_SIZE           stLength;
    INT                 iTableEntriesNr, i;
    INT                 iError          = PX_ERROR;

    _G_bAcpiEarlyAccess = LW_TRUE;

    __ACPI_DEBUG_LOG("\n\n**** acpiTableInit entered ****\n");

    /*
     * Get the root pointer
     */
    pRsdp = acpiFindRsdp();
    if (pRsdp == LW_NULL) {
        __ACPI_ERROR_LOG("\nACPI: NULL root pointer!\n");
        return  (PX_ERROR);
    }

    pRsdt = ACPI_TO_POINTER(pRsdp->RsdtPhysicalAddress);
    if (pRsdp->Revision > 0) {
        pXsdt = ACPI_TO_POINTER(pRsdp->XsdtPhysicalAddress);
    }

    __ACPI_DEBUG_LOG("\npRsdp = 0x%x\n", pRsdp);
    __ACPI_DEBUG_LOG("pRsdt = 0x%x\n",   pRsdt);
    __ACPI_DEBUG_LOG("pXsdt = 0x%x\n",   pXsdt);

    /*
     * Convert from physical to virtual
     */
    pPhysAcpiHeader = (ACPI_TABLE_HEADER *)pRsdt;

    stLength     = acpiGetTableSize((ACPI_PHYSICAL_ADDRESS)pRsdt);
    pcAcpiTStart = (CHAR *)((ULONG)pRsdt);
    pRsdt        = AcpiOsMapMemory((ACPI_PHYSICAL_ADDRESS)pcAcpiTStart, stLength);

    /*
     * If RSDT table is valid
     */
    if (pRsdt != LW_NULL) {
        __ACPI_DEBUG_LOG("\n  Checking Rsdt table %p, length %d\n", pRsdt, (pRsdt->Header.Length));
    }

    if ((pRsdt != LW_NULL) &&
        (acpiTableValidate((UINT8 *)pRsdt,
                           (UINT32)(pRsdt->Header.Length),
                           ACPI_SIG_RSDT) == ERROR_NONE)) {

        __ACPI_DEBUG_LOG("\n  Rsdt acpiTableRegister %p\n", pRsdt);

        acpiTableRegister((ACPI_TABLE_HEADER *)pRsdt,
                          pPhysAcpiHeader,
                          &_G_pcAcpiTableStart,
                          &_G_pcAcpiTableEnd);

        /*
         * Compute number of tables
         */
        iTableEntriesNr = (INT)((pRsdt->Header.Length - sizeof(ACPI_TABLE_HEADER)) / sizeof(UINT32));

        __ACPI_DEBUG_LOG("\n  Rsdt numTableEntries 0x%x\n", iTableEntriesNr);

        /*
         * Register each table
         */
        for (i = 0; i < iTableEntriesNr; i++) {
            pAcpiHeader     = ACPI_TO_POINTER(pRsdt->TableOffsetEntry[i]);
            pPhysAcpiHeader = pAcpiHeader;

            stLength      = acpiGetTableSize((ACPI_PHYSICAL_ADDRESS)pAcpiHeader);
            pcAcpiTStart  = (CHAR *)pAcpiHeader;

            pAcpiHeader = AcpiOsMapMemory((ACPI_PHYSICAL_ADDRESS)pcAcpiTStart, stLength);
            if (pAcpiHeader != LW_NULL) {
                __ACPI_DEBUG_LOG("\n  Rsdt checking table %018p, length %d\n",
                        pAcpiHeader, (pAcpiHeader->Length));
            }

            if ((pAcpiHeader == LW_NULL) ||
                (acpiTableValidate((UINT8 *)pAcpiHeader,
                                   (UINT32)(pAcpiHeader->Length),
                                   LW_NULL) != ERROR_NONE)) {
                continue;
            }

            __ACPI_DEBUG_LOG("\n  Rsdt acpiTableRegister2 %p\n", pAcpiHeader);

            acpiTableRegister((ACPI_TABLE_HEADER *)pAcpiHeader,
                              pPhysAcpiHeader,
                              &_G_pcAcpiTableStart,
                              &_G_pcAcpiTableEnd);

            __ACPI_DEBUG_LOG("  * Rsdt ACPI_NAME_COMPARE...\n");

            if (ACPI_NAME_COMPARE(pAcpiHeader->Signature, ACPI_SIG_FADT)) {
                pFadt = (ACPI_TABLE_FADT *)pAcpiHeader;

                __ACPI_DEBUG_LOG("\n  Rsdt check Facs...\n");

                _G_pAcpiFadt = (ACPI_TABLE_FADT *)pFadt;

                pFacs           = ACPI_TO_POINTER(pFadt->Facs);
                pPhysAcpiHeader = (ACPI_TABLE_HEADER *)pFacs;

                stLength     = acpiGetTableSize((ACPI_PHYSICAL_ADDRESS)pFacs);
                pcAcpiTStart = (CHAR *)((ULONG)pFacs);

                pFacs = AcpiOsMapMemory((ACPI_PHYSICAL_ADDRESS)pcAcpiTStart, stLength);
                if (pFacs == LW_NULL) {
                    __ACPI_DEBUG_LOG("\n  Mapping Facs failed!\n");
                    __ACPI_ERROR_LOG("\nACPI: Facs Mapping failed!\n");
                    return  (PX_ERROR);
                }

                if (acpiTableValidate((UINT8 *)((size_t)pFacs),
                                      (UINT32)(pFacs->Length),
                                      ACPI_SIG_FACS) == ERROR_NONE) {
                    acpiTableRegister((ACPI_TABLE_HEADER *)pFacs,
                                      pPhysAcpiHeader,
                                      &_G_pcAcpiTableStart,
                                      &_G_pcAcpiTableEnd);
                }

                __ACPI_DEBUG_LOG("\n  Rsdt check Dsdt...\n");
                pDsdt           = ACPI_TO_POINTER(pFadt->Dsdt);
                pPhysAcpiHeader = (ACPI_TABLE_HEADER *)pDsdt;

                stLength     = acpiGetTableSize((ACPI_PHYSICAL_ADDRESS)pDsdt);
                pcAcpiTStart = (CHAR *)((ULONG)pDsdt);

                pDsdt = AcpiOsMapMemory((ACPI_PHYSICAL_ADDRESS)pcAcpiTStart, stLength);
                if (pDsdt == LW_NULL) {
                    __ACPI_DEBUG_LOG("\n  Mapping Dsdt failed!\n");
                    __ACPI_ERROR_LOG("\nACPI: Dsdt Mapping failed!\n");
                    return  (PX_ERROR);
                }

                if (acpiTableValidate((UINT8 *)((size_t)pDsdt),
                                      (UINT32)(pDsdt->Length),
                                      ACPI_SIG_DSDT) == ERROR_NONE) {
                    acpiTableRegister((ACPI_TABLE_HEADER *)pDsdt,
                                      pPhysAcpiHeader,
                                      &_G_pcAcpiTableStart,
                                      &_G_pcAcpiTableEnd);
                }
            }
        }
    }

    if (pXsdt != LW_NULL) {
        pPhysAcpiHeader = (ACPI_TABLE_HEADER *)pXsdt;
        stLength        = acpiGetTableSize((ACPI_PHYSICAL_ADDRESS)pXsdt);

        pcAcpiTStart    = (CHAR *)((ULONG)pXsdt);
        pXsdt           = AcpiOsMapMemory((ACPI_PHYSICAL_ADDRESS)pcAcpiTStart, stLength);
        if (pXsdt != LW_NULL) {
            __ACPI_DEBUG_LOG("\n  Checking Xsdt table %p, length %d\n", pXsdt, (pXsdt->Header.Length));
        }

        if ((pXsdt != LW_NULL) &&
            (acpiTableValidate((UINT8 *)pXsdt,
                               (UINT32)(pXsdt->Header.Length),
                               ACPI_SIG_XSDT) == ERROR_NONE)) {

            __ACPI_DEBUG_LOG("\n  Xsdt acpiTableRegister %p\n", pXsdt);

            acpiTableRegister((ACPI_TABLE_HEADER *)pXsdt,
                              pPhysAcpiHeader,
                              &_G_pcAcpiTableStart,
                              &_G_pcAcpiTableEnd);

            /*
             * Compute number of tables
             */
            iTableEntriesNr = (INT)((pXsdt->Header.Length - sizeof(ACPI_TABLE_HEADER)) / sizeof(UINT64));

            __ACPI_DEBUG_LOG("\n  Xsdt numTableEntries %p\n", iTableEntriesNr);

            /*
             * Register each table
             */
            for (i = 0; i < iTableEntriesNr; i++) {
                pAcpiHeader     = ACPI_TO_POINTER(pXsdt->TableOffsetEntry[i]);
                pPhysAcpiHeader = pAcpiHeader,

                stLength     = acpiGetTableSize((ACPI_PHYSICAL_ADDRESS)pAcpiHeader);
                pcAcpiTStart = (CHAR *)((ULONG)pAcpiHeader);

                pAcpiHeader  = AcpiOsMapMemory((ACPI_PHYSICAL_ADDRESS)pcAcpiTStart, stLength);
                if (pAcpiHeader != LW_NULL) {
                    __ACPI_DEBUG_LOG("\n  Xsdt checking table %p, length %d\n",
                            pAcpiHeader, (pAcpiHeader->Length));
                }

                if ((pAcpiHeader == LW_NULL) ||
                    (acpiTableValidate((UINT8 *)pAcpiHeader,
                                       (UINT32)(pAcpiHeader->Length),
                                       LW_NULL) != ERROR_NONE)) {
                    continue;
                }

                acpiTableRegister(pAcpiHeader,
                                  pPhysAcpiHeader,
                                  &_G_pcAcpiTableStart,
                                  &_G_pcAcpiTableEnd);

                __ACPI_DEBUG_LOG("  * Xsdt ACPI_NAME_COMPARE...\n");

                if (ACPI_NAME_COMPARE(pAcpiHeader->Signature, ACPI_SIG_FADT)) {
                    pFadt = (ACPI_TABLE_FADT *)pAcpiHeader;

                    __ACPI_DEBUG_LOG("\n  Xsdt check Facs...\n");

                    pFacs           = ACPI_TO_POINTER(pFadt->XFacs);
                    pPhysAcpiHeader = (ACPI_TABLE_HEADER *)pFacs;

                    stLength     = acpiGetTableSize((ACPI_PHYSICAL_ADDRESS)pFacs);
                    pcAcpiTStart = (CHAR *)((ULONG)pFacs);

                    pFacs = AcpiOsMapMemory((ACPI_PHYSICAL_ADDRESS)pcAcpiTStart, stLength);
                    if (acpiTableValidate((UINT8 *)((size_t)pFacs),
                                          (UINT32)(stLength),
                                          ACPI_SIG_FACS) == ERROR_NONE) {
                        acpiTableRegister((ACPI_TABLE_HEADER *)(pFacs),
                                           pPhysAcpiHeader,
                                           &_G_pcAcpiTableStart,
                                           &_G_pcAcpiTableEnd);
                    }

                    __ACPI_DEBUG_LOG("\n  Xsdt check Dsdt...\n");

                    pDsdt           = ACPI_TO_POINTER(pFadt->XDsdt);
                    pPhysAcpiHeader = (ACPI_TABLE_HEADER *)pDsdt;

                    stLength     = acpiGetTableSize((ACPI_PHYSICAL_ADDRESS)pDsdt);
                    pcAcpiTStart = (CHAR *)((ULONG)pDsdt);

                    pDsdt = AcpiOsMapMemory((ACPI_PHYSICAL_ADDRESS)pcAcpiTStart, stLength);
                    if (acpiTableValidate((UINT8 *)((size_t)pDsdt),
                                          (UINT32)(stLength),
                                          ACPI_SIG_DSDT) == ERROR_NONE) {
                        acpiTableRegister((ACPI_TABLE_HEADER *)(pDsdt),
                                          pPhysAcpiHeader,
                                          &_G_pcAcpiTableStart,
                                          &_G_pcAcpiTableEnd);
                    }
                }
            }
        }
    }

    iError = ERROR_NONE;

    __ACPI_DEBUG_LOG("**** acpiTableInit returns 0x%x ****\n", iError);

    return  (iError);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
