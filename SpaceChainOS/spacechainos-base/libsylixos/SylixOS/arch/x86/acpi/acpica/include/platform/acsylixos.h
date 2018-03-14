/*
 * acsylixos.h
 *
 *  Created on: 2016/9/27
 *      Author: Jiao.JinXing
 */

#ifndef ACSYLIXOS_H_
#define ACSYLIXOS_H_

#define __SYLIXOS_STDIO
#define __SYLIXOS_KERNEL
#include "SylixOS.h"

#include <platform/acgcc.h>
#include <string.h>
#include <stdio.h>

#define ACPI_MACHINE_WIDTH              LW_CFG_CPU_WORD_LENGHT

#define COMPILER_DEPENDENT_INT64        int64_t
#define COMPILER_DEPENDENT_UINT64       uint64_t

#define ACPI_UINTPTR_T                  uintptr_t

#define ACPI_MUTEX_TYPE                 ACPI_OSL_MUTEX

#define ACPI_USE_DO_WHILE_0
#define ACPI_USE_LOCAL_CACHE
#define ACPI_USE_NATIVE_DIVIDE
#define ACPI_USE_SYSTEM_CLIBRARY
#define ACPI_USE_STANDARD_HEADERS

#define ACPI_SPINLOCK                   VOID *
#define ACPI_CPU_FLAGS                  INTREG
#define ACPI_SEMAPHORE                  LW_HANDLE
#define ACPI_MUTEX                      LW_HANDLE

#ifdef ACPI_APPLICATION
#define ACPI_FLUSH_CPU_CACHE()
#else
#define ACPI_FLUSH_CPU_CACHE()          X86_WBINVD()
#endif

#endif /* ACSYLIXOS_H_ */
