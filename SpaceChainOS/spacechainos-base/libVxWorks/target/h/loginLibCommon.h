/**
 * @file
 * login library.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#ifndef __VXWORKS_LOGINLIBCOMMON_H
#define __VXWORKS_LOGINLIBCOMMON_H

#include "vxWorksCommon.h"

#define S_loginLib_UNKNOWN_USER         ENOENT
#define S_loginLib_USER_ALREADY_EXISTS  EEXIST
#define S_loginLib_INVALID_PASSWORD     EBADMSG

#define VXWORKS_UID_OFFSET      100
#define VXWORKS_GID             0

#define MAX_LOGIN_NAME_LEN      64
#define MAX_PASSWORD_LEN        64
#define MAX_PASSWORD_LEN_OLD    MAX_PASSWORD_LEN
#define AUTH_SALT_LENGTH        8

/* Structure for passing login user info to sysctl (backward compatible) */

struct loginVerifyArgsOld {
    char name[MAX_LOGIN_NAME_LEN + 1];
    char passwd[MAX_PASSWORD_LEN_OLD + 1];
};

/* Structure for passing login user info to sysctl */

struct loginVerifyArgs {
    char name[MAX_LOGIN_NAME_LEN + 1];
    char passwd[MAX_PASSWORD_LEN + 1];
};

#ifdef __cplusplus
extern "C" {
#endif

STATUS  loginUserVerify(char *name, char *passwd);

#ifdef __cplusplus
}
#endif

#endif /* __VXWORKS_LOGINLIBCOMMON_H */
