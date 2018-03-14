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

#ifndef __VXWORKS_LOGINLIB_H
#define __VXWORKS_LOGINLIB_H

#include "loginLibCommon.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void    loginInit(void);
extern STATUS  loginDefaultEncrypt(char *in, char *salt, size_t size, char *out);
extern STATUS  loginUserAdd(char name[MAX_LOGIN_NAME_LEN + 1],
                            char passwd[MAX_PASSWORD_LEN + 1],
                            char salt[2 * AUTH_SALT_LENGTH]);
extern STATUS  loginUserDelete(char *name, char *passwd);
extern void    loginUserShow(void);

#ifdef __cplusplus
}
#endif

#endif /* __VXWORKS_LOGINLIB_H */
