/**
 * @file
 * tick library.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#ifndef __VXWORKS_TICKLIB_H
#define __VXWORKS_TICKLIB_H

#include "vxWorksCommon.h"

#ifdef __cplusplus
extern "C" {
#endif

/* function declarations */

void     tickAnnounce(void);
void     tickSet(ULONG ticks);
ULONG    tickGet(void);
void     tick64Set(UINT64 ticks);
UINT64   tick64Get(void);
STATUS   tickAnnounceHookAdd(FUNCPTR pFunc);

#ifdef __cplusplus
}
#endif

#endif /* __VXWORKS_TICKLIB_H */
