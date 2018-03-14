/**
 * @file
 * select library.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#ifndef __VXWORKS_SELECTLIB_H
#define __VXWORKS_SELECTLIB_H

#include <sys/select.h>

/* error codes are shared */

#define S_selectLib_NO_SELECT_SUPPORT_IN_DRIVER     ERROR_IO_SELECT_UNSUPPORT_IN_DRIVER
#define S_selectLib_NO_SELECT_CONTEXT               ERROR_IO_SELECT_CONTEXT
#define S_selectLib_WIDTH_OUT_OF_RANGE              ERROR_IO_SELECT_WIDTH

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif /* __VXWORKS_SELECTLIB_H */
