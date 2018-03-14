/**
 * @file
 * semphore library.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#ifndef __VXWORKS_SEMLIB_H
#define __VXWORKS_SEMLIB_H

#include "semLibCommon.h"

/* semaphore options */

#define SEM_Q_MASK       0x03   /* q-type mask */

/* macro used to statically declare semaphores */

#define VX_SEMAPHORE(name) \
        LW_HANDLE name

#define VX_BINARY_SEMAPHORE   VX_SEMAPHORE

#define VX_COUNTING_SEMAPHORE VX_SEMAPHORE

#define VX_MUTEX_SEMAPHORE    VX_SEMAPHORE

#define VX_READ_WRITE_SEMAPHORE(name, numReaders) \
        LW_HANDLE name

#ifdef __cplusplus
extern "C" {
#endif

/* function declarations */

void     semShowInit(void);
STATUS   semShow(SEM_ID semId, int level);
int      semInfo(SEM_ID semId, int idList[], int maxTasks);

#ifdef __cplusplus
}
#endif

#endif /* __VXWORKS_SEMLIB_H */
