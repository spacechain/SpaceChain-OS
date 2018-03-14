/**
 * @file
 * user login.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#include <userdb.h>
#include <shadow.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <crypt.h>
#include <pwd.h>
#include "loginLibCommon.h"

/*
 * login Init
 */
void  loginInit (void)
{
}

/*
 * login Default Encrypt
 */
STATUS  loginDefaultEncrypt (char *in, char *salt, size_t size, char *out)
{
#if LW_CFG_SHELL_PASS_CRYPT_EN > 0
    if (crypt_safe(in, salt, out, size)) {
        return  (OK);

    } else {
        return  (ERROR);
    }
#else
    lib_strlcpy(out, in, size);
#endif /* LW_CFG_SHELL_PASS_CRYPT_EN > 0 */
}

/*
 * Add VxWorks User
 */
STATUS  loginUserAdd (char name[MAX_LOGIN_NAME_LEN + 1],
                      char passwd[MAX_PASSWORD_LEN + 1],
                      char salt[2 * AUTH_SALT_LENGTH])
{
    uid_t  uid = VXWORKS_UID_OFFSET;
    CHAR   cHome[PATH_MAX + 1];

    struct passwd  pwd;
    struct passwd *ppwd;
    CHAR           buf[512];

    (VOID)salt;

    if (!name) {
        errno = EINVAL;
        return  (ERROR);
    }

    snprintf(cHome, PATH_MAX, "/home/%s", name);

    for (;;) {
        if (getpwuid_r(uid, &pwd, buf, sizeof(buf), &ppwd) < 0) {
            break;
        }
        uid++;
    }

    return  (user_db_uadd(name, passwd, 1, uid, VXWORKS_GID, "vxWorks-User", cHome));
}

/*
 * Delete VxWorks User
 */
STATUS  loginUserDelete (char *name, char *passwd)
{
    (VOID)passwd;

    if (!name) {
        errno = EINVAL;
        return  (ERROR);
    }

    if (geteuid()) {
        errno = EACCES;
        return  (ERROR);
    }

    return  (user_db_udel(name));
}

/*
 * Show All User
 */
void  loginUserShow (void)
{
    system("user");
}

/*
 * Show All User
 */
STATUS  loginUserVerify (char *name, char *passwd)
{
    return  (passwdcheck(name, passwd));
}

/*
 * end
 */
