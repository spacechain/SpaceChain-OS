/**
 * @file
 * object common library.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#ifndef __VXWORKS_OBJLIBCOMMON_H
#define __VXWORKS_OBJLIBCOMMON_H

#include "vxWorksCommon.h"
#include "intLib.h"

/* Object creation mode options */

#define OM_CREATE               0x10000000
#define OM_EXCL                 0x20000000
#define OM_DELETE_ON_LAST_CLOSE 0x40000000

/* Error Status codes */

#define S_objLib_OBJ_ID_ERROR                   EINVAL
#define S_objLib_OBJ_UNAVAILABLE                ERROR_KERNEL_HANDLE_NULL
#define S_objLib_OBJ_DELETED                    ERROR_KERNEL_HANDLE_NULL
#define S_objLib_OBJ_TIMEOUT                    ERROR_THREAD_WAIT_TIMEOUT
#define S_objLib_OBJ_NO_METHOD                  ENOSYS
#define S_objLib_OBJ_NO_ACCESS_RIGHTS           EACCES
#define S_objLib_OBJ_NAME_TRUNCATED             ENAMETOOLONG
#define S_objLib_OBJ_NAME_CLASH                 EINVAL
#define S_objLib_OBJ_NOT_NAMED                  EINVAL
#define S_objLib_OBJ_ILLEGAL_CLASS_TYPE         ERROR_KERNEL_HANDLE_NULL
#define S_objLib_OBJ_OPERATION_UNSUPPORTED      ENOTSUP
#define S_objLib_OBJ_INVALID_OWNER              EPERM
#define S_objLib_OBJ_RENAME_NOT_ALLOWED         EACCES
#define S_objLib_OBJ_DESTROY_ERROR              EINVAL
#define S_objLib_OBJ_HANDLE_TBL_FULL            ERROR_THREAD_FULL
#define S_objLib_OBJ_NOT_FOUND                  ENOENT
#define S_objLib_OBJ_INVALID_ARGUMENT           EINVAL
#define S_objLib_OBJ_LIB_NOT_INITIALIZED        EINVAL

#endif /* __VXWORKS_OBJLIBCOMMON_H */
