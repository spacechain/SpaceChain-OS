/**
 * @file
 * task variable library.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#ifndef __VXWORKS_TASKVARLIB_H
#define __VXWORKS_TASKVARLIB_H

#include "taskLibCommon.h"

#ifdef __cplusplus
extern "C" {
#endif

#if LW_CFG_SMP_EN == 0 /* ONLY support task variable in UP mode */

STATUS   taskVarAdd(int tid, int *pVar);
STATUS   taskVarDelete(int tid, int *pVar);
STATUS   taskVarInit(void);
STATUS   taskVarSet(int tid, int *pVar, int value);
int      taskVarGet(int tid, int *pVar);

#endif /* #if LW_CFG_SMP_EN == 0 */

#ifdef __cplusplus
}
#endif

#endif /* __VXWORKS_TASKVARLIB_H */
