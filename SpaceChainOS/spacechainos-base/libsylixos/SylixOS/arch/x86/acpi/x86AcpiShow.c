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
** 文   件   名: x86AcpiShow.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2017 年 04 月 17 日
**
** 描        述: x86 体系构架 ACPI 信息显示.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "x86AcpiLib.h"
#include "arch/x86/mpconfig/x86MpApic.h"
/*********************************************************************************************************
  全局变量定义
*********************************************************************************************************/
ACPI_MODULE_NAME("acpi_show")
/*********************************************************************************************************
** 函数名称: __acpiPrintableString
** 功能描述: 将输入字符串变为可打印的字符串
** 输　入  : pcOutString       输出字符串
**           pcInString        输入字符串
**           iLength           长度
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#ifdef ACPI_VERBOSE

static VOID  __acpiPrintableString (CHAR  *pcOutString, const CHAR  *pcInString, INT  iLength)
{
    INT  i;

    for (i = 0; i < (iLength - 1); i++) {
        if (lib_isalnum(pcInString[i])) {
            pcOutString[i] = pcInString[i];
        } else {
            pcOutString[i] = ' ';
        }
    }
    pcOutString[i] = '\0';
}
/*********************************************************************************************************
** 函数名称: acpiShowFacs
** 功能描述: 显示 FACS
** 输　入  : pAcpiHeader       ACPI 表头
** 输　出  : FACS 长度
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  AcpiShowFacs (const ACPI_TABLE_HEADER  *pAcpiHeader)
{
    ACPI_TABLE_FACS  *pFacs = (ACPI_TABLE_FACS *)pAcpiHeader;
    CHAR              acString[10];

    if (LW_SYS_STATUS_IS_RUNNING()) {
        /*
         * FACS is a special case, doesn't have an ACPI header
         */
        __acpiPrintableString(acString, pFacs->Signature, 5);

        ACPI_VERBOSE_PRINT("\n\n  ACPI message (%p) %4s\n",      pFacs, acString);
        ACPI_VERBOSE_PRINT("    Length        %d\n",             pFacs->Length);
        ACPI_VERBOSE_PRINT("    HardwareSignature     0x%x\n",   pFacs->HardwareSignature);
        ACPI_VERBOSE_PRINT("    FirmwareWakingVec     0x%x\n",   pFacs->FirmwareWakingVector);
        ACPI_VERBOSE_PRINT("    GlobalLock    0x%x\n",           pFacs->GlobalLock);
        ACPI_VERBOSE_PRINT("    Flags         0x%x\n",           pFacs->Flags);
        ACPI_VERBOSE_PRINT("    XFirmwareWakingVect   0x%llx\n", pFacs->XFirmwareWakingVector);
        ACPI_VERBOSE_PRINT("    Version       0x%x\n",           pFacs->Version);
    }

    return  (pFacs->Length);
}

#endif                                                                  /*  ACPI_VERBOSE                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
