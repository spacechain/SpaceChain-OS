/**
 * @file
 * ACPI sylixos.
 */

/*
 * Copyright (c) 2006-2016 SylixOS Group.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * Author: Jiao.JinXing <jiaojinxing@acoinfo.com>
 *
 */

#define  __SYLIXOS_PCI_DRV
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "stdio.h"
#include <linux/compat.h>
#include "x86AcpiLib.h"
#include "arch/x86/mpconfig/x86MpApic.h"

ACPI_MODULE_NAME("acpi_sylixos")

/*
 * Globals
 */
static volatile VOID *_G_pvAcpiRedirectionTarget = LW_NULL;
static ACPI_SIZE      _G_stAcpiOsHeapUsedByte  = 0;
static size_t         _G_stAcpiOsHeapMaxAlloc  = 0;
static ULONG          _G_ulApciOsHeapAllocFail = 0;

PCHAR  _G_pcAcpiOsHeapPtr  = LW_NULL;
PCHAR  _G_pcAcpiOsHeapBase = LW_NULL;
PCHAR  _G_pcAcpiOsHeapTop  = LW_NULL;

/*
 * Marcos
 */
#define __IS_PTR_IN_HEAP(pvPtr) \
    ((_G_pcAcpiOsHeapBase != LW_NULL) && \
    (((size_t)pvPtr) >= ((size_t)_G_pcAcpiOsHeapBase)) && \
    (((size_t)pvPtr) <= ((size_t)_G_pcAcpiOsHeapTop)))

/******************************************************************************
*
* FUNCTION:    AcpiOsInitialize
*
* PARAMETERS:  None
*
* RETURN:      Status
*
* DESCRIPTION: Init this OSL
*
*****************************************************************************/

ACPI_STATUS  AcpiOsInitialize (VOID)
{
	_G_pvAcpiRedirectionTarget = LW_NULL;

	return  (AE_OK);
}

/******************************************************************************
*
* FUNCTION:    AcpiOsInfoShow
*
* PARAMETERS:  None
*
* RETURN:      None
*
* DESCRIPTION: Show info about OS
*
*****************************************************************************/

VOID  AcpiOsInfoShow (VOID)
{
    ACPI_VERBOSE_PRINT("\n  ACPI heap max allocated size = %ld\n", _G_stAcpiOsHeapMaxAlloc);
    ACPI_VERBOSE_PRINT("  ACPI heap allocated = %ld bytes\n",      _G_stAcpiOsHeapUsedByte);

    if (_G_ulApciOsHeapAllocFail) {
        ACPI_VERBOSE_PRINT("  ACPI heap allocated failed = %ld times\n", _G_ulApciOsHeapAllocFail);
        ACPI_VERBOSE_PRINT("  ACPI_HEAP_SIZE = %ld is too small (%ld)\n",
                (((size_t)_G_pcAcpiOsHeapTop) - ((size_t)_G_pcAcpiOsHeapBase) + 1) / 1024,
                 ((size_t)_G_pcAcpiOsHeapTop) - ((size_t)_G_pcAcpiOsHeapBase));
    }
}

/******************************************************************************
*
* FUNCTION:    AcpiOsTerminate
*
* PARAMETERS:  None
*
* RETURN:      Status
*
* DESCRIPTION: Nothing to do for SylixOS
*
*****************************************************************************/

ACPI_STATUS  AcpiOsTerminate (VOID)
{
	return  (AE_OK);
}

/******************************************************************************
*
* FUNCTION:    AcpiOsGetRootPointer
*
* PARAMETERS:  None
*
* RETURN:      RSDP physical address
*
* DESCRIPTION: Gets the root pointer (RSDP)
*
*****************************************************************************/

ACPI_PHYSICAL_ADDRESS  AcpiOsGetRootPointer (VOID)
{
    ACPI_TABLE_RSDP  *pRsdp;
    ACPI_SIZE         pTableAddress;

    /*
     * Check to see if EFI left us a pointer
     */
    if (_G_pcAcpiRsdpPtr != LW_NULL) {
        pRsdp = (ACPI_TABLE_RSDP *)_G_pcAcpiRsdpPtr;
        return  ((ACPI_PHYSICAL_ADDRESS)pRsdp);
    }

    /*
     * See if the generic ACPI code can find the root pointer
     */
    if (AcpiFindRootPointer(&pTableAddress) != AE_OK) {
        /*
         * If not, see if we can scan memory for it
         */
        return  ((ACPI_PHYSICAL_ADDRESS)acpiFindRsdp());
    }

    return  ((ACPI_PHYSICAL_ADDRESS)pTableAddress);
}

/******************************************************************************
*
* FUNCTION:    AcpiOsPredefinedOverride
*
* PARAMETERS:  InitVal             - Initial value of the predefined object
*              NewVal              - The new value for the object
*
* RETURN:      Status, pointer to value. LW_NULL pointer returned if not
*              overriding.
*
* DESCRIPTION: Allow the OS to override predefined names
*
*****************************************************************************/

ACPI_STATUS  AcpiOsPredefinedOverride (const ACPI_PREDEFINED_NAMES  *InitVal,
			                           ACPI_STRING                  *NewVal)
{
	if (!InitVal || !NewVal) {
		return  (AE_BAD_PARAMETER);
	}

	*NewVal = LW_NULL;
	return  (AE_OK);
}

/******************************************************************************
*
* FUNCTION:    AcpiOsTableOverride
*
* PARAMETERS:  ExistingTable       - Header of current table (probably firmware)
*              NewTable            - Where an entire new table is returned.
*
* RETURN:      Status, pointer to new table. LW_NULL pointer returned if no
*              table is available to override
*
* DESCRIPTION: Return a different version of a table if one is available
*
*****************************************************************************/

ACPI_STATUS  AcpiOsTableOverride (ACPI_TABLE_HEADER  *ExistingTable,
			                      ACPI_TABLE_HEADER  **NewTable)
{
	if (!ExistingTable || !NewTable) {
		return  (AE_BAD_PARAMETER);
	}

	*NewTable = LW_NULL;
	return  (AE_NO_ACPI_TABLES);
}

/******************************************************************************
*
* FUNCTION:    AcpiOsMapMemory
*
* PARAMETERS:  Where               - Physical address of memory to be mapped
*              Length              - How much memory to map
*
* RETURN:      Pointer to mapped memory. LW_NULL on error.
*
* DESCRIPTION: Map physical memory into caller's address space
*
*****************************************************************************/

VOID  *AcpiOsMapMemory (ACPI_PHYSICAL_ADDRESS  Where,
                        ACPI_SIZE              Length)
{ 
    if (LW_SYS_STATUS_IS_RUNNING()) {
        addr_t  ulPhyBase = ROUND_DOWN(Where, LW_CFG_VMM_PAGE_SIZE);
        addr_t  ulOffset  = Where - ulPhyBase;
        addr_t  ulVirBase;

        Length   += ulOffset;
        Length    = ROUND_UP(Length, LW_CFG_VMM_PAGE_SIZE);

        ulVirBase = (addr_t)API_VmmIoRemapNocache((PVOID)ulPhyBase, Length);
        if (ulVirBase) {
            return  (VOID *)(ulVirBase + ulOffset);
        } else {
            return  (VOID *)(LW_NULL);
        }
    } else {
        return  (VOID *)(Where);
    }
}

/******************************************************************************
*
* FUNCTION:    AcpiOsUnmapMemory
*
* PARAMETERS:  Where               - Logical address of memory to be unmapped
*              Length              - How much memory to unmap
*
* RETURN:      None.
*
* DESCRIPTION: Delete a previously created mapping. Where and Length must
*              correspond to a previous mapping exactly.
*
*****************************************************************************/

VOID  AcpiOsUnmapMemory (VOID  *Where, ACPI_SIZE  Length)
{
    if (LW_SYS_STATUS_IS_RUNNING()) {
        API_VmmIoUnmap(Where);
    }
}

/******************************************************************************
*
* FUNCTION:    AcpiOsAllocate
*
* PARAMETERS:  Size                - Amount to allocate, in bytes
*
* RETURN:      Pointer to the new allocation. LW_NULL on error.
*
* DESCRIPTION: Allocate memory. Algorithm is dependent on the OS.
*
*****************************************************************************/

VOID  *AcpiOsAllocate (ACPI_SIZE  Size)
{
    if (LW_SYS_STATUS_IS_RUNNING()) {
        VOID  *pvRet;

        pvRet = sys_malloc(Size);
        if (pvRet) {
            lib_bzero(pvRet, Size);
        }

        return  (pvRet);
    } else {
        CHAR  *pcRet;
        CHAR  *pcNewPtr;

        /*
         * 保证返回的每个 Pointer 2 倍 sizeof(size_t) 大小字节对齐
         */
        Size = ROUND_UP(Size, (2 * sizeof(size_t)));

        if (Size > _G_stAcpiOsHeapMaxAlloc) {
            _G_stAcpiOsHeapMaxAlloc = Size;
        }

        /*
         * allocate a buffer from the static buffer area
         */
        if (_G_pcAcpiOsHeapPtr == LW_NULL) {
            __ACPI_ERROR_LOG("\n---> AcpiOsAllocate no buffer <---\r\n");
            return  (LW_NULL);
        }

        pcRet    = _G_pcAcpiOsHeapPtr;
        pcNewPtr = _G_pcAcpiOsHeapPtr + Size;

        /*
         * if we didn't run off the end of the buffer
         */
        if (__IS_PTR_IN_HEAP(pcNewPtr)) {
            _G_stAcpiOsHeapUsedByte += Size;
            _G_pcAcpiOsHeapPtr       = pcNewPtr;
            return  (VOID *)pcRet;
        }

        _G_ulApciOsHeapAllocFail++;

        __ACPI_ERROR_LOG("\n---> AcpiOsAllocate out buffer space <---\r\n");
        return  (LW_NULL);
    }
}

/******************************************************************************
*
* FUNCTION:    AcpiOsFree
*
* PARAMETERS:  Mem                 - Pointer to previously allocated memory
*
* RETURN:      None.
*
* DESCRIPTION: Free memory allocated via AcpiOsAllocate
*
*****************************************************************************/

VOID  AcpiOsFree (VOID  *Mem)
{
    if (__IS_PTR_IN_HEAP(Mem)) {
        return;
    }
    if (LW_SYS_STATUS_IS_RUNNING()) {
        sys_free(Mem);
    }
}

/******************************************************************************
*
* FUNCTION:    AcpiOsInstallInterruptHandler
*
* PARAMETERS:  InterruptNumber     - Level handler should respond to.
*              ServiceRoutine      - Address of the ACPI interrupt handler
*              Context             - User context
*
* RETURN:      Handle to the newly installed handler.
*
* DESCRIPTION: Install an interrupt handler. Used to install the ACPI
*              OS-independent handler.
*
*****************************************************************************/

UINT32  AcpiOsInstallInterruptHandler (UINT32            InterruptNumber,
                                       ACPI_OSD_HANDLER  ServiceRoutine,
                                       VOID             *Context)
{
    if (LW_SYS_STATUS_IS_RUNNING()) {
        API_InterVectorSetFlag(InterruptNumber, LW_IRQ_FLAG_QUEUE);

        API_InterVectorConnect(InterruptNumber,
                               (PINT_SVR_ROUTINE)ServiceRoutine,
                               (PVOID)Context,
                               "acpi_isr");

        API_InterVectorEnable(InterruptNumber);
    }
	return (AE_OK);
}

/******************************************************************************
*
* FUNCTION:    AcpiOsRemoveInterruptHandler
*
* PARAMETERS:  Handle              - Returned when handler was installed
*
* RETURN:      Status
*
* DESCRIPTION: Uninstalls an interrupt handler.
*
*****************************************************************************/

ACPI_STATUS  AcpiOsRemoveInterruptHandler (UINT32            InterruptNumber,
			                               ACPI_OSD_HANDLER  ServiceRoutine)
{
    if (LW_SYS_STATUS_IS_RUNNING()) {
        API_InterVectorDisable(InterruptNumber);
    }
	return  (AE_OK);
}

/******************************************************************************
*
* FUNCTION:    AcpiOsStall
*
* PARAMETERS:  Microseconds        - Time to stall
*
* RETURN:      None. Blocks until stall is completed.
*
* DESCRIPTION: Sleep at microsecond granularity (1 Milli = 1000 Micro)
*
*****************************************************************************/

VOID  AcpiOsStall (UINT32  Microseconds)
{
    if (LW_SYS_STATUS_IS_RUNNING()) {
        API_TimeMSleep((Microseconds / 1000) + 1);
    }
}

/******************************************************************************
*
* FUNCTION:    AcpiOsSleep
*
* PARAMETERS:  Milliseconds        - Time to sleep
*
* RETURN:      None. Blocks until sleep is completed.
*
* DESCRIPTION: Sleep at millisecond granularity
*
*****************************************************************************/

VOID  AcpiOsSleep (UINT64  Milliseconds)
{
    if (LW_SYS_STATUS_IS_RUNNING()) {
        API_TimeMSleep(Milliseconds);
    }
}

/*********************************************************************************************************
  PCI 主控器配置地址 PCI_MECHANISM_1 2
*********************************************************************************************************/
#define PCI_CONFIG_ADDR0()          PCI_CONFIG_ADDR
#define PCI_CONFIG_ADDR1()          PCI_CONFIG_DATA
/*********************************************************************************************************
  PCI 主控器配置 IO 操作 PCI_MECHANISM_1 2
*********************************************************************************************************/
#define PCI_IN_BYTE(addr)           in8((addr))
#define PCI_IN_WORD(addr)           in16((addr))
#define PCI_IN_DWORD(addr)          in32((addr))
#define PCI_OUT_BYTE(addr, data)    out8((UINT8)(data), (addr))
#define PCI_OUT_WORD(addr, data)    out16((UINT16)(data), (addr))
#define PCI_OUT_DWORD(addr, data)   out32((UINT32)(data), (addr))

INT  x86PciConfigInByte (INT  iBus, INT  iSlot, INT  iFunc, INT  iOft, UINT8  *pucValue)
{
    UINT8   ucRet = 0;
    INT     iRetVal = PX_ERROR;

    PCI_OUT_DWORD(PCI_CONFIG_ADDR0(), (PCI_PACKET(iBus, iSlot, iFunc) | (iOft & 0xfc) | 0x80000000));
    ucRet = PCI_IN_BYTE(PCI_CONFIG_ADDR1() + (iOft & 0x3));
    iRetVal = ERROR_NONE;

    if (pucValue) {
        *pucValue = ucRet;
    }

    return  (iRetVal);
}

INT  x86PciConfigInWord (INT  iBus, INT  iSlot, INT  iFunc, INT  iOft, UINT16  *pusValue)
{
    UINT16  usRet = 0;
    INT     iRetVal = PX_ERROR;

    if (iOft & 0x01) {
        return  (PX_ERROR);
    }

    PCI_OUT_DWORD(PCI_CONFIG_ADDR0(), (PCI_PACKET(iBus, iSlot, iFunc) | (iOft & 0xfc) | 0x80000000));
    usRet = PCI_IN_WORD(PCI_CONFIG_ADDR1() + (iOft & 0x2));
    iRetVal = ERROR_NONE;

    if (pusValue) {
        *pusValue = usRet;
    }

    return  (iRetVal);
}

INT  x86PciConfigInDword (INT  iBus, INT  iSlot, INT  iFunc, INT  iOft, UINT32  *puiValue)
{
    UINT32  uiRet   = 0;
    INT     iRetVal = PX_ERROR;

    if (iOft & 0x03) {
        return  (PX_ERROR);
    }

    PCI_OUT_DWORD(PCI_CONFIG_ADDR0(), (PCI_PACKET(iBus, iSlot, iFunc) | (iOft & 0xfc) | 0x80000000));
    uiRet = PCI_IN_DWORD(PCI_CONFIG_ADDR1());
    iRetVal = ERROR_NONE;

    if (puiValue) {
        *puiValue = uiRet;
    }

    return  (iRetVal);
}

INT  x86PciConfigOutByte (INT  iBus, INT  iSlot, INT  iFunc, INT  iOft, UINT8  ucValue)
{
    PCI_OUT_DWORD(PCI_CONFIG_ADDR0(), (PCI_PACKET(iBus, iSlot, iFunc) | (iOft & 0xfc) | 0x80000000));
    PCI_OUT_BYTE((PCI_CONFIG_ADDR1() + (iOft & 0x3)), ucValue);

    return  (ERROR_NONE);
}

INT  x86PciConfigOutWord (INT  iBus, INT  iSlot, INT  iFunc, INT  iOft, UINT16  usValue)
{
    PCI_OUT_DWORD(PCI_CONFIG_ADDR0(), (PCI_PACKET(iBus, iSlot, iFunc) | (iOft & 0xfc) | 0x80000000));
    PCI_OUT_WORD((PCI_CONFIG_ADDR1() + (iOft & 0x2)), usValue);

    return  (ERROR_NONE);
}

INT  x86PciConfigOutDword (INT  iBus, INT  iSlot, INT  iFunc, INT  iOft, UINT32  uiValue)
{
    PCI_OUT_DWORD(PCI_CONFIG_ADDR0(), (PCI_PACKET(iBus, iSlot, iFunc) | (iOft & 0xfc) | 0x80000000));
    PCI_OUT_DWORD(PCI_CONFIG_ADDR1(), uiValue);

    return  (ERROR_NONE);
}

/******************************************************************************
*
* FUNCTION:    AcpiOsReadPciConfiguration
*
* PARAMETERS:  PciId               - Seg/Bus/Dev
*              Register            - Device Register
*              Value               - Buffer where value is placed
*              Width               - Number of bits
*
* RETURN:      Status
*
* DESCRIPTION: Read data from PCI configuration space
*
*****************************************************************************/

ACPI_STATUS  AcpiOsReadPciConfiguration (ACPI_PCI_ID  *PciId,
                                         UINT32        Register,
                                         UINT64       *Value,
                                         UINT32        Width)
{
	switch (Width) {
    case 8: {
        x86PciConfigInByte(PciId->Bus, PciId->Device, PciId->Function, Register, (UINT8 *)Value);
    } break;

    case 16: {
        x86PciConfigInWord(PciId->Bus, PciId->Device, PciId->Function, Register, (UINT16 *)Value);
    } break;

    case 32: {
        x86PciConfigInDword(PciId->Bus, PciId->Device, PciId->Function, Register, (UINT32 *)Value);
    } break;

    default:
        return  (AE_ERROR);
	}

	return  (AE_OK);
}

/******************************************************************************
*
* FUNCTION:    AcpiOsWritePciConfiguration
*
* PARAMETERS:  PciId               - Seg/Bus/Dev
*              Register            - Device Register
*              Value               - Value to be written
*              Width               - Number of bits
*
* RETURN:      Status
*
* DESCRIPTION: Write data to PCI configuration space
*
*****************************************************************************/

ACPI_STATUS  AcpiOsWritePciConfiguration (ACPI_PCI_ID  *PciId,
                                          UINT32        Register,
                                          UINT64        Value,
                                          UINT32        Width)
{
	switch (Width) {
    case 8: {
        x86PciConfigOutByte(PciId->Bus, PciId->Device, PciId->Function, Register, (UINT8)Value);
    } break;

    case 16: {
        x86PciConfigOutWord(PciId->Bus, PciId->Device, PciId->Function, Register, (UINT16)Value);
    } break;

    case 32: {
        x86PciConfigOutDword(PciId->Bus, PciId->Device, PciId->Function, Register, (UINT32)Value);
    } break;

    default:
        return  (AE_ERROR);
	}

	return  (AE_OK);
}

/******************************************************************************
*
* FUNCTION:    AcpiOsReadPort
*
* PARAMETERS:  Address             - Address of I/O port/register to read
*              Value               - Where value is placed
*              Width               - Number of bits
*
* RETURN:      Value read from port
*
* DESCRIPTION: Read data from an I/O port or register
*
*****************************************************************************/

ACPI_STATUS  AcpiOsReadPort (ACPI_IO_ADDRESS    Address,
                             UINT32            *Value,
                             UINT32             Width)
{
	switch (Width) {
	case 8:
		*Value = in8((UINT16)Address);
		break;

	case 16:
		*Value = in16((UINT16)Address);
		break;

	case 32:
		*Value = in32((UINT16)Address);
		break;

	default:
		ACPI_ERROR((AE_INFO, "Bad width parameter: %X", Width));
		return  (AE_BAD_PARAMETER);
	}

	return  (AE_OK);
}

/******************************************************************************
*
* FUNCTION:    AcpiOsWritePort
*
* PARAMETERS:  Address             - Address of I/O port/register to write
*              Value               - Value to write
*              Width               - Number of bits
*
* RETURN:      None
*
* DESCRIPTION: Write data to an I/O port or register
*
*****************************************************************************/

ACPI_STATUS  AcpiOsWritePort (ACPI_IO_ADDRESS   Address,
                              UINT32            Value,
                              UINT32            Width)
{
    switch (Width) {
    case 8:
        out8((UINT8)Value, (UINT16)Address);
        break;

    case 16:
        out16((UINT16)Value, (UINT16)Address);
        break;

    case 32:
        out32((UINT32)Value, (UINT16)Address);
        break;

    default:
        ACPI_ERROR((AE_INFO, "Bad width parameter: %X", Width));
        return  (AE_BAD_PARAMETER);
    }

    return  (AE_OK);
}

/******************************************************************************
*
* FUNCTION:    AcpiOsReadMemory
*
* PARAMETERS:  Address             - Physical Memory Address to read
*              Value               - Where value is placed
*              Width               - Number of bits (8,16,32, or 64)
*
* RETURN:      Value read from physical memory address. Always returned
*              as a 64-bit integer, regardless of the read width.
*
* DESCRIPTION: Read data from a physical memory address
*
*****************************************************************************/

ACPI_STATUS  AcpiOsReadMemory (ACPI_PHYSICAL_ADDRESS    Address,
                               UINT32                  *Value,
                               UINT32                   Width)
{
    switch (Width) {
    case 8:
        *Value = read8(Address);
        break;

    case 16:
        *Value = read16(Address);
        break;

    case 32:
        *Value = read32(Address);
        break;

    default:
        ACPI_ERROR((AE_INFO, "Bad width parameter: %X", Width));
        return  (AE_BAD_PARAMETER);
    }

    return  (AE_OK);
}

/******************************************************************************
*
* FUNCTION:    AcpiOsWriteMemory
*
* PARAMETERS:  Address             - Physical Memory Address to write
*              Value               - Value to write
*              Width               - Number of bits (8,16,32, or 64)
*
* RETURN:      None
*
* DESCRIPTION: Write data to a physical memory address
*
*****************************************************************************/

ACPI_STATUS  AcpiOsWriteMemory (ACPI_PHYSICAL_ADDRESS   Address,
                                UINT32                  Value,
                                UINT32                  Width)
{
    switch (Width) {
    case 8:
        write8((UINT8)Value, Address);
        break;

    case 16:
        write16((UINT16)Value, Address);
        break;

    case 32:
        write32((UINT32)Value, Address);
        break;

    default:
        ACPI_ERROR((AE_INFO, "Bad width parameter: %X", Width));
        return  (AE_BAD_PARAMETER);
    }

    return  (AE_OK);
}

/******************************************************************************
*
* FUNCTION:    AcpiOsSignal
*
* PARAMETERS:  Function            - ACPICA signal function code
*              Info                - Pointer to function-dependent structure
*
* RETURN:      Status
*
* DESCRIPTION: Miscellaneous functions. Example implementation only.
*
*****************************************************************************/

ACPI_STATUS  AcpiOsSignal (UINT32   Function,
                           VOID    *Info)
{
    switch (Function) {
    case ACPI_SIGNAL_FATAL:
        break;

    case ACPI_SIGNAL_BREAKPOINT:
        break;
    }

    return  (AE_OK);
}

/******************************************************************************
*
* FUNCTION:    AcpiOsGetThreadId
*
* PARAMETERS:  None
*
* RETURN:      Id of the running thread
*
* DESCRIPTION: Get the Id of the current (running) thread
*
*****************************************************************************/

ACPI_THREAD_ID  AcpiOsGetThreadId (VOID)
{
    if (LW_SYS_STATUS_IS_RUNNING()) {
        return  (API_ThreadIdSelf());
    } else {
        return  (1);
    }
}

/******************************************************************************
*
* FUNCTION:    AcpiOsExecute
*
* PARAMETERS:  Type                - Type of execution
*              Function            - Address of the function to execute
*              Context             - Passed as a parameter to the function
*
* RETURN:      Status
*
* DESCRIPTION: Execute a new thread
*
*****************************************************************************/

ACPI_STATUS  AcpiOsExecute (ACPI_EXECUTE_TYPE       Type,
                            ACPI_OSD_EXEC_CALLBACK  Function,
                            VOID                   *Context)
{
    if (LW_SYS_STATUS_IS_RUNNING()) {
        LW_CLASS_THREADATTR  threadattr = API_ThreadAttrGetDefault();

        threadattr.THREADATTR_pvArg = Context;

        API_ThreadCreate("t_acpi", (PTHREAD_START_ROUTINE)Function, &threadattr, LW_NULL);
    } else {
        Function(Context);
    }

	return  (AE_OK);
}

/******************************************************************************
*
* FUNCTION:    AcpiOsPrintf
*
* PARAMETERS:  Fmt, ...            - Standard printf format
*
* RETURN:      None
*
* DESCRIPTION: Formatted output
*
*****************************************************************************/

VOID ACPI_INTERNAL_VAR_XFACE AcpiOsPrintf (const CHAR  *Fmt, ...)
{
	va_list  Args;
	
	va_start(Args, Fmt);
	AcpiOsVprintf(Fmt, Args);
	va_end(Args);
}

/******************************************************************************
*
* FUNCTION:    AcpiOsVprintf
*
* PARAMETERS:  Fmt                 - Standard printf format
*              Args                - Argument list
*
* RETURN:      None
*
* DESCRIPTION: Formatted output with argument list pointer
*
*****************************************************************************/

VOID  AcpiOsVprintf (const CHAR  *Fmt, va_list  Args)
{
    CHAR  Buffer[256];

    vsnprintf(Buffer, sizeof(Buffer), Fmt, Args);

    if (LW_SYS_STATUS_IS_RUNNING()) {
        printk("ACPI: %s", Buffer);
    }
}

/******************************************************************************
*
* FUNCTION:    AcpiOsGetLine
*
* PARAMETERS:  Buffer              - Where to return the command line
*              BufferLength        - Maximum length of Buffer
*              BytesRead           - Where the actual byte count is returned
*
* RETURN:      Status and actual bytes read
*
* DESCRIPTION: Formatted input with argument list pointer
*
*****************************************************************************/

UINT32  AcpiOsGetLine (CHAR  *Buffer)
{
    if (LW_SYS_STATUS_IS_RUNNING()) {
        INT     Temp = EOF;
        UINT32  i;

        for (i = 0;; i++) {
            if ((Temp = getchar()) == EOF) {
                return  (AE_ERROR);
            }

            if (!Temp || Temp == '\n') {
                break;
            }

            Buffer[i] = (CHAR)Temp;
        }

        /*
         * LW_NULL terminate the buffer
         */
        Buffer[i] = 0;

        return  (i);
    } else {
        return  (0);
    }
}

/******************************************************************************
*
* FUNCTION:    Spinlock interfaces
*
* DESCRIPTION: Map these interfaces to semaphore interfaces
*
*****************************************************************************/

ACPI_STATUS  AcpiOsCreateLock (ACPI_SPINLOCK  *OutHandle)
{
    if (LW_SYS_STATUS_IS_RUNNING()) {
        spinlock_t  *pLock = (spinlock_t *)AcpiOsAllocate(sizeof(spinlock_t));

        if (pLock) {
            LW_SPIN_INIT(pLock);
            *OutHandle = (ACPI_SPINLOCK)pLock;
            return  (AE_OK);

        } else {
            *OutHandle = LW_NULL;
            return  (AE_ERROR);
        }
    } else {
        *OutHandle = (ACPI_SPINLOCK)0;
        return  (AE_OK);
    }
}

VOID  AcpiOsDeleteLock (ACPI_SPINLOCK  Handle)
{
    if (LW_SYS_STATUS_IS_RUNNING() && Handle) {
        sys_free(Handle);
    }
}

ACPI_CPU_FLAGS  AcpiOsAcquireLock (ACPI_SPINLOCK  Handle)
{
    if (LW_SYS_STATUS_IS_RUNNING() && Handle) {
        ACPI_CPU_FLAGS  intreg;

        LW_SPIN_LOCK_IRQ((spinlock_t *)Handle, &intreg);
        return  (intreg);
    } else {
        return  (0);
    }
}

VOID  AcpiOsReleaseLock (ACPI_SPINLOCK  Handle, ACPI_CPU_FLAGS  Flags)
{
    if (LW_SYS_STATUS_IS_RUNNING() && Handle) {
        LW_SPIN_UNLOCK_IRQ((spinlock_t *)Handle, Flags);
    }
}

/******************************************************************************
*
* FUNCTION:    Semaphore functions
*
* DESCRIPTION: Stub functions used for single-thread applications that do
*              not require semaphore synchronization. Full implementations
*              of these functions appear after the stubs.
*
*****************************************************************************/

ACPI_STATUS  AcpiOsCreateSemaphore (UINT32          MaxUnits,
                                    UINT32          InitialUnits,
                                    ACPI_SEMAPHORE *OutHandle)
{
    if (LW_SYS_STATUS_IS_RUNNING()) {
        *OutHandle = (ACPI_SEMAPHORE)API_SemaphoreCCreate("sem_acpi", InitialUnits, MaxUnits,
                     (LW_OPTION_WAIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL), LW_NULL);
        if (*OutHandle != LW_HANDLE_INVALID) {
            return  (AE_OK);
        } else {
            return  (AE_ERROR);
        }
    } else {
        *OutHandle = (ACPI_SEMAPHORE)LW_HANDLE_INVALID;
        return  (AE_OK);
    }
}

ACPI_STATUS  AcpiOsDeleteSemaphore (ACPI_SEMAPHORE  Handle)
{
    if (LW_SYS_STATUS_IS_RUNNING()) {
        if (Handle) {
            API_SemaphoreCDelete((LW_HANDLE *)&Handle);
        }
    }

	return  (AE_OK);
}

ACPI_STATUS  AcpiOsWaitSemaphore (ACPI_SEMAPHORE      Handle,
                                  UINT32              Units,
                                  UINT16              Timeout)
{
    if (LW_SYS_STATUS_IS_RUNNING()) {
        if (Handle) {
            API_SemaphoreCPend((LW_HANDLE)Handle,
                               Timeout == ACPI_DO_NOT_WAIT ? LW_OPTION_NOT_WAIT : LW_OPTION_WAIT_INFINITE);
        }
    }

	return  (AE_OK);
}

ACPI_STATUS  AcpiOsSignalSemaphore (ACPI_SEMAPHORE      Handle,
                                    UINT32              Units)
{
    if (LW_SYS_STATUS_IS_RUNNING()) {
        if (Handle) {
            API_SemaphoreCRelease((LW_HANDLE)Handle, Units, LW_NULL);
        }
    }

	return  (AE_OK);
}

ACPI_STATUS  AcpiOsCreateMutex (ACPI_MUTEX  *OutHandle)
{
    if (LW_SYS_STATUS_IS_RUNNING()) {
        *OutHandle = (ACPI_MUTEX)API_SemaphoreMCreate("mutex_acpi", LW_PRIO_CRITICAL,
                     (LW_OPTION_WAIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL |
                      LW_OPTION_DELETE_SAFE | LW_OPTION_INHERIT_PRIORITY), LW_NULL);
        if (*OutHandle != LW_HANDLE_INVALID) {
            return  (AE_OK);
        } else {
            return  (AE_ERROR);
        }
    } else {
        *OutHandle = (ACPI_MUTEX)LW_HANDLE_INVALID;
        return  (AE_OK);
    }
}

VOID  AcpiOsDeleteMutex (ACPI_MUTEX  Handle)
{
    if (LW_SYS_STATUS_IS_RUNNING()) {
        if (Handle) {
            API_SemaphoreMDelete((LW_HANDLE *)&Handle);
        }
    }
}

ACPI_STATUS  AcpiOsAcquireMutex (ACPI_MUTEX     Handle,
                                 UINT16         Timeout)
{
    if (LW_SYS_STATUS_IS_RUNNING()) {
        if (Handle) {
            API_SemaphoreMPend((LW_HANDLE)Handle,
                               Timeout == ACPI_DO_NOT_WAIT ?
                               LW_OPTION_NOT_WAIT : LW_OPTION_WAIT_INFINITE);
        }
    }

    return  (AE_OK);
}

VOID  AcpiOsReleaseMutex (ACPI_MUTEX    Handle)
{
    if (LW_SYS_STATUS_IS_RUNNING()) {
        if (Handle) {
            API_SemaphoreMPost((LW_HANDLE)Handle);
        }
    }
}

/******************************************************************************
*
* FUNCTION:    AcpiOsGetTimer
*
* PARAMETERS:  None
*
* RETURN:      Current ticks in 100-nanosecond units
*
* DESCRIPTION: Get the value of a system timer
*
******************************************************************************/

UINT64  AcpiOsGetTimer (VOID)
{
    if (LW_SYS_STATUS_IS_RUNNING()) {
        return  (API_TimeGet64() * 10000000 / LW_TICK_HZ);
    } else {
        static UINT64  ui64Time = 0;

        return  (++ui64Time);
    }
}

/******************************************************************************
*
* FUNCTION:    AcpiOsReadable
*
* PARAMETERS:  Pointer             - Area to be verified
*              Length              - Size of area
*
* RETURN:      TRUE if readable for entire length
*
* DESCRIPTION: Verify that a pointer is valid for reading
*
*****************************************************************************/

BOOLEAN  AcpiOsReadable (VOID  *Pointer, ACPI_SIZE  Length)
{
	return  (TRUE);
}

/******************************************************************************
*
* FUNCTION:    AcpiOsWritable
*
* PARAMETERS:  Pointer             - Area to be verified
*              Length              - Size of area
*
* RETURN:      TRUE if writable for entire length
*
* DESCRIPTION: Verify that a pointer is valid for writing
*
*****************************************************************************/

BOOLEAN  AcpiOsWritable (VOID  *Pointer, ACPI_SIZE  Length)
{
	return  (TRUE);
}

/******************************************************************************
*
* FUNCTION:    AcpiOsRedirectOutput
*
* PARAMETERS:  Destination         - An open file handle/pointer
*
* RETURN:      None
*
* DESCRIPTION: Causes redirect of AcpiOsPrintf and AcpiOsVprintf
*
*****************************************************************************/

VOID  AcpiOsRedirectOutput (VOID  *Destination)
{
	_G_pvAcpiRedirectionTarget = Destination;
}

/******************************************************************************
*
* FUNCTION:    AcpiOsWaitEventsComplete
*
* PARAMETERS:  None
*
* RETURN:      None
*
* DESCRIPTION: Wait for all asynchronous events to complete. This
*              implementation does nothing.
*
*****************************************************************************/

VOID AcpiOsWaitEventsComplete (VOID  *Context)
{
}
