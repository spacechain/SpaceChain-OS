/**
 * @file
 * VxWorks system library.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#ifndef __VXWORKS_SYSLIB_H
#define __VXWORKS_SYSLIB_H

#include "vxWorksCommon.h"

#ifdef __SYLIXOS_KERNEL
#include <io.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* system restart types */

#define BOOT_NORMAL         LW_REBOOT_WARM    /* normal reboot with countdown */
#define BOOT_NO_AUTOBOOT    LW_REBOOT_WARM    /* no autoboot if set */
#define BOOT_CLEAR          LW_REBOOT_WARM    /* clear memory if set */
#define BOOT_QUICK_AUTOBOOT LW_REBOOT_COLD    /* fast autoboot if set */

/* for backward compatibility */

#define BOOT_WARM_AUTOBOOT          LW_REBOOT_WARM
#define BOOT_WARM_NO_AUTOBOOT       LW_REBOOT_WARM
#define BOOT_WARM_QUICK_AUTOBOOT    LW_REBOOT_WARM
#define BOOT_COLD                   LW_REBOOT_COLD

#ifdef __SYLIXOS_KERNEL
#define sysInByte(port)     (UINT8)in8((addr_t)(port))
#define sysInWord(port)     (UINT16)in16((addr_t)(port))
#ifdef _WRS_CONFIG_LP64
#define sysInLong(port)     (UINT32)in32((addr_t)(port))
#else
#define sysInLong(port)     (UINT64)in64((addr_t)(port))
#endif /* _WRS_CONFIG_LP64 */

#define sysInByteString(port, data, cnt)        ins8((addr_t)(port), (PVOID)(data), (size_t)(cnt))
#define sysInWordString(port, data, cnt)        ins16((addr_t)(port), (PVOID)(data), (size_t)(cnt))
#ifdef _WRS_CONFIG_LP64
#define sysInLongString(port, data, cnt)        ins32((addr_t)(port), (PVOID)(data), (size_t)(cnt))
#else
#define sysInLongString(port, data, cnt)        ins64((addr_t)(port), (PVOID)(data), (size_t)(cnt))
#endif /* _WRS_CONFIG_LP64 */

#define sysOutByte(port, data)      out8((UINT8)data, (addr_t)(port))
#define sysOutWord(port, data)      out16((UINT16)data, (addr_t)(port))
#ifdef _WRS_CONFIG_LP64
#define sysOutLong(port, data)      out32((UINT32)data, (addr_t)(port))
#else
#define sysOutLong(port, data)      out64((UINT64)data, (addr_t)(port))
#endif /* _WRS_CONFIG_LP64 */

#define sysOutByteString(port, data, cnt)       outs8((addr_t)(port), (PVOID)(data), (size_t)(cnt))
#define sysOutWordString(port, data, cnt)       outs16((addr_t)(port), (PVOID)(data), (size_t)(cnt))
#ifdef _WRS_CONFIG_LP64
#define sysOutLongString(port, data, cnt)       outs32((addr_t)(port), (PVOID)(data), (size_t)(cnt))
#else
#define sysOutLongString(port, data, cnt)       outs64((addr_t)(port), (PVOID)(data), (size_t)(cnt))
#endif /* _WRS_CONFIG_LP64 */
#endif /* __SYLIXOS_KERNEL */

#ifdef __cplusplus
}
#endif

#endif /* __VXWORKS_SYSLIB_H */
