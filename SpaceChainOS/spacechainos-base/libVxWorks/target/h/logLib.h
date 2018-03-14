/**
 * @file
 * Log library.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#ifndef __VXWORKS_LOGLIB_H
#define __VXWORKS_LOGLIB_H

#include "vxWorksCommon.h"

#ifdef __cplusplus
extern "C" {
#endif

extern INT      API_LogMsg(CPCHAR       pcFormat,
                           PVOID        pvArg0,
                           PVOID        pvArg1,
                           PVOID        pvArg2,
                           PVOID        pvArg3,
                           PVOID        pvArg4,
                           PVOID        pvArg5,
                           PVOID        pvArg6,
                           PVOID        pvArg7,
                           PVOID        pvArg8,
                           PVOID        pvArg9,
                           BOOL         bIsNeedHeader);

#ifndef __SYLIXOS_KERNEL
#define logMsg(fmt, a, b, c, d, e, f)   \
        do {    \
            API_LogMsg(fmt, (PVOID)a, (PVOID)b, (PVOID)c, \
                       (PVOID)d, (PVOID)e, (PVOID)f,    \
                       0, 0, 0, 0, FALSE);  \
        } while (0)
#endif /* __SYLIXOS_KERNEL */

extern STATUS   logFdAdd(int fd);
extern STATUS   logFdDelete(int fd);

#ifdef __cplusplus
}
#endif

#endif /* __VXWORKS_LOGLIB_H */
