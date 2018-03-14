/**
 * @file
 * kernel library.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#ifndef __VXWORKS_KERNELLIB_H
#define __VXWORKS_KERNELLIB_H

#include "vxWorksCommon.h"

#define _KERNEL_INIT_PARAMS_VERSION 0x6600  /* sanity check */

#ifdef __cplusplus
extern "C" {
#endif

extern char *   kernelVersion(void);
extern STATUS   kernelTimeSlice(int ticks);
extern STATUS   kernelTimeSliceGet(ULONG *pTimeSlice);
extern STATUS   kernelRoundRobinInstall(void);
extern STATUS   kernelCpuEnable(unsigned int cpuToEnable);
extern BOOL     kernelIsCpuIdle(unsigned int cpu);
extern BOOL     kernelIsSystemIdle(void);

#ifdef __cplusplus
}
#endif

#endif /* __VXWORKS_KERNELLIB_H */
