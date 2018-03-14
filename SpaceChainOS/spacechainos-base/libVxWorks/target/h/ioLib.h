/**
 * @file
 * I/O system library.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#ifndef __VXWORKS_IOLIB_H
#define __VXWORKS_IOLIB_H

#include "vxWorksCommon.h"

#define S_ioLib_NO_DRIVER               ENXIO       /* NOSTATTBL */
#define S_ioLib_UNKNOWN_REQUEST         ENOTSUP     /* NOSTATTBL */
#define S_ioLib_DEVICE_ERROR            EIO         /* NOSTATTBL */
#define S_ioLib_DEVICE_TIMEOUT          EIO         /* NOSTATTBL */
#define S_ioLib_WRITE_PROTECTED         EACCES      /* NOSTATTBL */
#define S_ioLib_DISK_NOT_PRESENT        ENXIO       /* NOSTATTBL */
#define S_ioLib_NO_FILENAME             ENOENT      /* NOSTATTBL */
#define S_ioLib_CANCELLED               ECANCELED   /* NOSTATTBL */
#define S_ioLib_NO_DEVICE_NAME_IN_PATH  ENODEV      /* NOSTATTBL */
#define S_ioLib_NAME_TOO_LONG           ENAMETOOLONG/* NOSTATTBL */
#define S_ioLib_UNFORMATED              EIO         /* NOSTATTBL */
#define S_ioLib_CANT_OVERWRITE_DIR      EISDIR      /* NOSTATTBL */

#endif /* __VXWORKS_IOLIB_H */
