/**
 * @file
 * errno library.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#ifndef __VXWORKS_ERRNOLIB_H
#define __VXWORKS_ERRNOLIB_H

#include "vxWorksCommon.h"

/* status codes */

#define S_errnoLib_NO_STAT_SYM_TBL      -2

#ifdef __cplusplus
extern "C" {
#endif

STATUS   errnoOfTaskSet(int taskId, int errorValue);
STATUS   errnoSet(int errorValue);
int      errnoGet(void);
int      errnoOfTaskGet(int taskId);

#ifdef __cplusplus
}
#endif

#endif /* __VXWORKS_ERRNOLIB_H */
