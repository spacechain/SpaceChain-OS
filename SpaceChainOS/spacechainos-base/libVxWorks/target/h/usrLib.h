/**
 * @file
 * user library functions.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#ifndef __VXWORKS_USRLIB_H
#define __VXWORKS_USRLIB_H

#include "vxWorksCommon.h"

#ifdef __cplusplus
extern "C" {
#endif

void w(int taskNameOrId);
void tw(int taskNameOrId);
void checkStack(int taskNameOrId);
void i(int taskNameOrId);
void ti(int taskNameOrId);
void printLogo(void);
void printErrno(int errNo);

#ifdef __cplusplus
}
#endif

#endif /* __VXWORKS_USRLIB_H */
