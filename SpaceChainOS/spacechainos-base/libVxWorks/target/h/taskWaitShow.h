/**
 * @file
 * task wait library.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#ifndef __VXWORKS_TASKWAITLIB_H
#define __VXWORKS_TASKWAITLIB_H

#include "taskLibCommon.h"

#ifdef __cplusplus
extern "C" {
#endif

STATUS taskWaitShow(int taskId, int level);

#ifdef __cplusplus
}
#endif

#endif /* __VXWORKS_TASKWAITLIB_H */
