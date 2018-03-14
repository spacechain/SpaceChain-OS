/**
 * @file
 * timer library functions.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#ifndef __VXWORKS_WDLIB_H
#define __VXWORKS_WDLIB_H

#include "vxWorksCommon.h"

#ifdef __cplusplus
extern "C" {
#endif

/* sylixos recommend DO NOT use the following API, because pRoutine will call from interrupt level
 * please use POSIX timer or timerfd functions, if you want use high speed timer
 * please use hstimerfd functions*/

typedef LW_HANDLE WDOG_ID;

/* function declarations */

STATUS   wdLibInit(void);
void     wdCreateLibInit(void);
WDOG_ID  wdCreate(void);
STATUS   wdDelete(WDOG_ID wdId);
STATUS   wdStart(WDOG_ID wdId, int delay, FUNCPTR pRoutine,
                 int parameter);
STATUS   wdCancel(WDOG_ID wdId);
void     wdShowInit(void);
STATUS   wdShow(WDOG_ID wdId);

#ifdef __cplusplus
}
#endif

#endif /* __VXWORKS_WDLIB_H */
