/**
 * @file
 * user db setting.
 */

/*
 * Copyright (c) 2006-2016 SylixOS Group.
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 * 4. This code has been or is applying for intellectual property protection 
 *    and can only be used with acoinfo software products.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 * 
 * Author: Han.hui <sylixos@gmail.com>
 *
 */

#define  __SYLIXOS_KERNEL
#include "stdio.h"
#include "pwd.h"
#include "grp.h"
#include "shadow.h"
#include "errno.h"
#include "unistd.h"
#include "stdlib.h"
#include "string.h"
#include "limits.h"
#include "ctype.h"
#include "crypt.h"

extern void init_etc_shadow(void);
extern void init_etc_passwd_group(void);

/*
 * User database lock
 */
#define USER_DB_LOCK_FILE   "/etc/usr.lock"
 
/*
 * User parameter max len
 */
#define USER_PARAM_LEN      512

/*
 * User sync max retry times
 */
#define USER_SYNC_RETRY_MAX     3
#define USER_SYNC_RETRY_DELAY   100

/*
 * Resource
 */
#if LW_CFG_MODULELOADER_EN > 0
static LW_RESOURCE_RAW  udb_resraw;
#endif
 
/*
 * User
 */
typedef struct {
    LW_LIST_LINE     list;
    char            *name;
    char            *passwd;
    uid_t            uid;
    gid_t            gid;
    char            *comment;
    char            *dir;
} user_t;
static BOOL                 user_need_sync;
static LW_LIST_LINE_HEADER  user_in;
static LW_LIST_LINE_HEADER  user_out;

/*
 * Group
 */
typedef struct {
    LW_LIST_LINE     list;
    char            *name;
    char            *passwd;
    gid_t            gid;
    char            *allmember;
} group_t;
static BOOL                 group_need_sync;
static LW_LIST_LINE_HEADER  group_in;
static LW_LIST_LINE_HEADER  group_out;

/*
 * Password
 */
typedef struct {
    LW_LIST_LINE     list;
    char            *name;
    char            *passwd;
} shadow_t;
static BOOL                 shadow_need_sync;
static LW_LIST_LINE_HEADER  shadow_in;
static LW_LIST_LINE_HEADER  shadow_out;

/*
 * dup string
 */
static int load_dup_string (FILE *fp, char **p, int flag)
{
    char  line[USER_PARAM_LEN];
    
    char *buf = line;
    int   len = USER_PARAM_LEN;
    int   c;
           
    for (;;) {
        c = getc(fp);
        if (c == ':') {
            if (flag) {
                return  (0);
            }
            break;
        }
        if (c == '\n') {
            if (!flag) {
                return  (0);
            }
            break;
        }
        if (c == EOF) {
            return  (0);
        }
        if (len < 2) {
            return  (0);
        }
        *buf++ = (char)c;
        len--;
    }
    
    *buf++ = PX_EOS;
    len--;
    
    if (p) {
        *p = lib_strdup(line);
        if (*p == NULL) {
            return  (0);
        }
    }
    
    return  (1);
}

/*
 * dup string
 */
static int load_dup_int (FILE *fp, unsigned int *p)
{
    unsigned int i     = 0;
    unsigned int limit = INT_MAX;
    int   c;
    int   sign = 0;
    int   d;
    
    for (;;) {
        c = getc(fp);
        if (c == ':') {
            break;
        }
        if (sign == 0) {
            if (c == '-') {
                sign = -1;
                limit++;
                continue;
            }
            sign = 1;
        }
        if (!lib_isdigit(c)) {
            return  (0);
        }
        d = c - '0';
        if ((i > (limit / 10)) ||
            ((i == (limit / 10)) && (d > (limit % 10)))) {
            return  (0);
        }
        i = i * 10 + d;
    }
    
    if (sign == 0) {
        return  0;
    }
    
    if (p) {
        *p = i * sign;
    }
    
    return  (1);
}

/*
 * walk_to_line_end
 */
static void walk_to_line_end (FILE *fp)
{
    int   c;
    
    for (;;) {
        c = getc(fp);
        if ((c == '\n') || (c == '\0')) {
            break;
        }
    }
}

/*
 * User info load one
 */
static int passwd_load_one (FILE *fp, user_t  *puser)
{
    if (!load_dup_string(fp, &puser->name, 0)    ||
        !load_dup_string(fp, &puser->passwd, 0)  ||
        !load_dup_int(fp, &puser->uid)           ||
        !load_dup_int(fp, &puser->gid)           ||
        !load_dup_string(fp, &puser->comment, 0) ||
        !load_dup_string(fp, NULL, 0)            ||
        !load_dup_string(fp, &puser->dir, 0)     ||
        !load_dup_string(fp, NULL, 1)) {
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}

/*
 * Group info load one
 */
static int group_load_one (FILE *fp, group_t  *pgrp)
{
    if (!load_dup_string(fp, &pgrp->name, 0)   ||
        !load_dup_string(fp, &pgrp->passwd, 0) ||
        !load_dup_int(fp, &pgrp->gid)          ||
        !load_dup_string(fp, &pgrp->allmember, 1)) {
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}

/*
 * shadow info load one
 */
static int shadow_load_one (FILE *fp, shadow_t  *pshadow)
{
    if (!load_dup_string(fp, &pshadow->name, 0) ||
        !load_dup_string(fp, &pshadow->passwd, 0)) {
        return  (PX_ERROR);    
    }
    
    walk_to_line_end(fp);
    
    return  (ERROR_NONE);
}

/*
 * User info unload one
 */
static void passwd_unload_one (user_t  *puser)
{
    if (puser->name) {
        lib_free(puser->name);
        puser->name = NULL;
    }
    
    if (puser->passwd) {
        lib_free(puser->passwd);
        puser->passwd = NULL;
    }
    
    if (puser->comment) {
        lib_free(puser->comment);
        puser->comment = NULL;
    }
    
    if (puser->dir) {
        lib_free(puser->dir);
        puser->dir = NULL;
    }
}

/*
 * Group info unload one
 */
static void group_unload_one (group_t  *pgrp)
{
    if (pgrp->name) {
        lib_free(pgrp->name);
        pgrp->name = NULL;
    }
    
    if (pgrp->passwd) {
        lib_free(pgrp->passwd);
        pgrp->passwd = NULL;
    }
    
    if (pgrp->allmember) {
        lib_free(pgrp->allmember);
        pgrp->allmember = NULL;
    }
}

/*
 * shadow info unload one
 */
static void shadow_unload_one (shadow_t  *pshadow)
{
    if (pshadow->name) {
        lib_free(pshadow->name);
        pshadow->name = NULL;
    }
    
    if (pshadow->passwd) {
        lib_free(pshadow->passwd);
        pshadow->passwd = NULL;
    }
}

/*
 * User info load
 */
static int passwd_load (void)
{
    int      ret = ERROR_NONE;
    FILE    *userfp;
    user_t  *puser;
    
    userfp = fopen(_PATH_PASSWD, "r");
    if (!userfp) {
        return  (PX_ERROR);
    }
    
    for (;;) {
        puser = (user_t *)lib_malloc(sizeof(user_t));
        if (!puser) {
            ret = PX_ERROR;
            break;
        }
        lib_bzero(puser, sizeof(user_t));
        
        if (passwd_load_one(userfp, puser)) {
            passwd_unload_one(puser);
            lib_free(puser);
            break;
        }
        
        _List_Line_Add_Ahead(&puser->list, &user_in);
        if (!user_out) {
            user_out = user_in;
        }
    }

    fclose(userfp);
    
    user_need_sync = FALSE;
    
    return  (ret);
}

/*
 * Group info load
 */
static int group_load (void)
{
    int       ret = ERROR_NONE;
    FILE     *grpfp;
    group_t  *pgrp;
    
    grpfp = fopen(_PATH_GROUP, "r");
    if (!grpfp) {
        return  (PX_ERROR);
    }
    
    for (;;) {
        pgrp = (group_t *)lib_malloc(sizeof(group_t));
        if (!pgrp) {
            ret = PX_ERROR;
            break;
        }
        lib_bzero(pgrp, sizeof(group_t));
        
        if (group_load_one(grpfp, pgrp)) {
            group_unload_one(pgrp);
            lib_free(pgrp);
            break;
        }
        
        _List_Line_Add_Ahead(&pgrp->list, &group_in);
        if (!group_out) {
            group_out = group_in;
        }
    }
    
    fclose(grpfp);
    
    group_need_sync = FALSE;
    
    return  (ret);
}

/*
 * Shadow info load
 */
static int shadow_load (void)
{
    int       ret = ERROR_NONE;
    FILE     *shdfp;
    shadow_t *pshadow;
    
    shdfp = fopen(_PATH_SHADOW, "r");
    if (!shdfp) {
        return  (PX_ERROR);
    }
    
    for (;;) {
        pshadow = (shadow_t *)lib_malloc(sizeof(shadow_t));
        if (!pshadow) {
            ret = PX_ERROR;
            break;
        }
        lib_bzero(pshadow, sizeof(shadow_t));
        
        if (shadow_load_one(shdfp, pshadow)) {
            shadow_unload_one(pshadow);
            lib_free(pshadow);
            break;
        }
        
        _List_Line_Add_Ahead(&pshadow->list, &shadow_in);
        if (!shadow_out) {
            shadow_out = shadow_in;
        }
    }
    
    fclose(shdfp);
    
    shadow_need_sync = FALSE;
    
    return  (ret);
}

/*
 * User info unload
 */
static void passwd_unload (void)
{
    user_t  *puser;
    
    while (user_in) {
        puser   = _LIST_ENTRY(user_in, user_t, list);
        user_in = _list_line_get_next(user_in);
        
        passwd_unload_one(puser);
        lib_free(puser);
    }
    
    user_out = NULL;
}

/*
 * Group info unload
 */
static void group_unload (void)
{
    group_t  *pgrp;
    
    while (group_in) {
        pgrp     = _LIST_ENTRY(group_in, group_t, list);
        group_in = _list_line_get_next(group_in);
        
        group_unload_one(pgrp);
        lib_free(pgrp);
    }
    
    group_out = NULL;
}

/*
 * Shadow info unload
 */
static void shadow_unload (void)
{
    shadow_t  *pshadow;
    
    while (shadow_in) {
        pshadow   = _LIST_ENTRY(shadow_in, shadow_t, list);
        shadow_in = _list_line_get_next(shadow_in);
        
        shadow_unload_one(pshadow);
        lib_free(pshadow);
    }
    
    shadow_out = NULL;
}

/*
 * User info sync to file
 */
static int passwd_sync (void)
{
    int  fd;
    int  ret = ERROR_NONE;
    
    PLW_LIST_LINE  pline;
    user_t        *puser;

    fd = open(_PATH_PASSWD ".update", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        return  (PX_ERROR);
    }
    
    for (pline  = user_out;
         pline != NULL;
         pline  = _list_line_get_prev(pline)) {
        
        puser = _LIST_ENTRY(pline, user_t, list);
        fdprintf(fd, "%s:%s:%d:%d:%s::%s:/bin/sh\n", 
                 puser->name, puser->passwd, 
                 puser->uid, puser->gid, puser->comment,
                 puser->dir);
    }
    
    close(fd);
    
    chmod(_PATH_PASSWD ".bak", 0600);
    unlink(_PATH_PASSWD ".bak");
    
    if (rename(_PATH_PASSWD, _PATH_PASSWD ".bak")) {
        ret = PX_ERROR;
        goto    out;
    }
    
    if (rename(_PATH_PASSWD ".update", _PATH_PASSWD)) {
        ret = PX_ERROR;
        goto    out;
    }
    
out:
    unlink(_PATH_PASSWD ".update");
    
    if (ret == ERROR_NONE) {
        user_need_sync = FALSE;
    }
    
    return  (ret);
}

/*
 * Group info sync to file
 */
static int group_sync (void)
{
    int  fd;
    int  ret = ERROR_NONE;
    
    PLW_LIST_LINE   pline;
    group_t        *pgrp;

    fd = open(_PATH_GROUP ".update", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        return  (PX_ERROR);
    }
    
    for (pline  = group_out;
         pline != NULL;
         pline  = _list_line_get_prev(pline)) {
        
        pgrp = _LIST_ENTRY(pline, group_t, list);
        fdprintf(fd, "%s:%s:%d:%s\n", 
                 pgrp->name, pgrp->passwd, 
                 pgrp->gid, pgrp->allmember);
    }
    
    close(fd);
    
    chmod(_PATH_GROUP ".bak", 0600);
    unlink(_PATH_GROUP ".bak");
    
    if (rename(_PATH_GROUP, _PATH_GROUP ".bak")) {
        ret = PX_ERROR;
        goto    out;
    }
    
    if (rename(_PATH_GROUP ".update", _PATH_GROUP)) {
        ret = PX_ERROR;
        goto    out;
    }
    
out:
    unlink(_PATH_GROUP ".update");
    
    if (ret == ERROR_NONE) {
        group_need_sync = FALSE;
    }
    
    return  (ret);
}

/*
 * Shadow info sync to file
 */
static int shadow_sync (void)
{
    int  fd;
    int  ret = ERROR_NONE;
    
    PLW_LIST_LINE   pline;
    shadow_t       *pshadow;

    fd = open(_PATH_SHADOW ".update", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        return  (PX_ERROR);
    }
    
    for (pline  = shadow_out;
         pline != NULL;
         pline  = _list_line_get_prev(pline)) {
        
        pshadow = _LIST_ENTRY(pline, shadow_t, list);
        fdprintf(fd, "%s:%s:0:0:99999:7:::\n", 
                 pshadow->name, pshadow->passwd);
    }
    
    close(fd);
    
    chmod(_PATH_SHADOW ".bak", 0600);
    unlink(_PATH_SHADOW ".bak");
    
    if (rename(_PATH_SHADOW, _PATH_SHADOW ".bak")) {
        ret = PX_ERROR;
        goto    out;
    }
    
    if (rename(_PATH_SHADOW ".update", _PATH_SHADOW)) {
        ret = PX_ERROR;
        goto    out;
    }
    
out:
    unlink(_PATH_SHADOW ".update");
    
    if (ret == ERROR_NONE) {
        shadow_need_sync = FALSE;
    }
    
    return  (ret);
}

/*
 * User find
 */
static user_t *passwd_find (const char *name, uid_t  uid)
{
    PLW_LIST_LINE  pline;
    user_t        *puser;
    
    for (pline  = user_out;
         pline != NULL;
         pline  = _list_line_get_prev(pline)) {
        
        puser = _LIST_ENTRY(pline, user_t, list);
        if (name && !lib_strcmp(puser->name, name)) {
            return  (puser);
        
        } else if (puser->uid == uid) {
            return  (puser);
        }
    }
    
    return  (NULL);
}

/*
 * Group find
 */
static group_t *group_find (const char *name, gid_t  gid)
{
    PLW_LIST_LINE  pline;
    group_t       *pgrp;
    
    for (pline  = group_out;
         pline != NULL;
         pline  = _list_line_get_prev(pline)) {
        
        pgrp = _LIST_ENTRY(pline, group_t, list);
        if (name && !lib_strcmp(pgrp->name, name)) {
            return  (pgrp);
        
        } else if (pgrp->gid == gid) {
            return  (pgrp);
        }
    }
    
    return  (NULL);
}

/*
 * Shadow find
 */
static shadow_t *shadow_find (const char *name)
{
    PLW_LIST_LINE  pline;
    shadow_t      *pshadow;
    
    for (pline  = shadow_out;
         pline != NULL;
         pline  = _list_line_get_prev(pline)) {
        
        pshadow = _LIST_ENTRY(pline, shadow_t, list);
        if (!lib_strcmp(pshadow->name, name)) {
            return  (pshadow);
        }
    }
    
    return  (NULL);
}

/*
 * User set
 */
static int passwd_set (user_t *puser, uid_t  uid, gid_t  gid, const char *comment, const char *home)
{
    char *newcomment = NULL;
    char *newhome    = NULL;
    
    if (comment) {
        newcomment = lib_strdup(comment);
        if (!newcomment) {
            return  (PX_ERROR);
        }
        if (puser->comment) {
            lib_free(puser->comment);
        }
        puser->comment = newcomment;
    }
    
    if (home) {
        newhome = lib_strdup(home);
        if (!newhome) {
            return  (PX_ERROR);
        }
        if (puser->dir) {
            lib_free(puser->dir);
        }
        puser->dir = newhome;
    }
    
    puser->uid = uid;
    puser->gid = gid;
    
    return  (ERROR_NONE);
}

/*
 * Group set
 */
static int group_set (group_t *pgrp, int  op, const char *member)
{
#define GROUP_SET_ADD_MEM   0
#define GROUP_SET_DEL_MEM   1

    char   *newallmember = NULL;
    char   *tmp;
    char   *cpy;
    
    size_t  mem_len;
    size_t  old_len;
    size_t  tot_len;
    
    BOOL    haved = FALSE;
    
    if (member) {
        mem_len = lib_strlen(member);
        old_len = lib_strlen(pgrp->allmember);
        tmp     = lib_strstr(pgrp->allmember, member);
        
        if (tmp) {
            if ((tmp[mem_len] == ',') || (tmp[mem_len] == '\0')) {
                if (tmp == pgrp->allmember) {
                    haved = TRUE;
                }
                if (tmp[-1] == ',') {
                    haved = TRUE;
                }
            }
        }
        
        if (op == GROUP_SET_ADD_MEM) {
            if (haved) {
                return  (ERROR_NONE);
            }
        
            if (old_len == 0) {
                tot_len =  mem_len + 1;
            
            } else {
                tot_len =  old_len + mem_len + 2;
            }
            
            newallmember = (char *)lib_malloc(tot_len);
            if (!newallmember) {
                return  (PX_ERROR);
            }
            
            if (old_len == 0) {
                lib_strcpy(newallmember, member);
            
            } else {
                lib_strcpy(newallmember, pgrp->allmember);
                lib_strcat(newallmember, ",");
                lib_strcat(newallmember, member);
            }
            
            if (pgrp->allmember) {
                lib_free(pgrp->allmember);
            }
            
            pgrp->allmember = newallmember;
        
        } else {
            if (!haved) {
                return  (ERROR_NONE);
            }
            
            cpy = tmp + mem_len;
            if (*cpy == ',') {
                cpy++;
            
            } else {
                *tmp = '\0';
                if (tmp != pgrp->allmember) {
                    tmp[-1] = '\0';
                    return  (ERROR_NONE);
                }
            }
            
            while (*cpy) {
                *tmp++ = *cpy++;
            }
            
            *tmp = '\0';
        }
    }
    
    return  (ERROR_NONE);
}

/*
 * Shadow set
 */
static int shadow_set (shadow_t *pshadow, const char *pass)
{
    char *newpass = NULL;
    
    if (pass) {
        newpass = lib_strdup(pass);
        if (!newpass) {
            return  (PX_ERROR);
        }
        
        if (pshadow->passwd) {
            lib_free(pshadow->passwd);
        }
        
        pshadow->passwd = newpass;
    }
    
    return  (ERROR_NONE);
}

/*
 * User info unload
 */
static int user_load (void)
{
    user_need_sync   = FALSE;
    group_need_sync  = FALSE;
    shadow_need_sync = FALSE;
    
    if (!passwd_load()) {
        if (!group_load()) {
            if (!shadow_load()) {
                return  (ERROR_NONE);
            }
        }
    }
    
    passwd_unload();
    group_unload();
    shadow_unload();
    
    return  (PX_ERROR);
}

/*
 * User info unload
 */
static void user_unload (void)
{
    passwd_unload();
    group_unload();
    shadow_unload();
}

/*
 * User info sync
 */
static int user_sync (void)
{
    int  i;
    
    if (user_need_sync) {
        for (i = 0; i < USER_SYNC_RETRY_MAX; i++) {
            if (passwd_sync() == ERROR_NONE) {
                break;
            }
            usleep(USER_SYNC_RETRY_DELAY * 1000);
        }
        if (i >= USER_SYNC_RETRY_MAX) {
            return  (PX_ERROR);
        }
    }
    
    if (group_need_sync) {
        for (i = 0; i < USER_SYNC_RETRY_MAX; i++) {
            if (group_sync() == ERROR_NONE) {
                break;
            }
            usleep(USER_SYNC_RETRY_DELAY * 1000);
        }
        if (i >= USER_SYNC_RETRY_MAX) {
            return  (PX_ERROR);
        }
    }
    
    if (shadow_need_sync) {
        for (i = 0; i < USER_SYNC_RETRY_MAX; i++) {
            if (shadow_sync() == ERROR_NONE) {
                break;
            }
            usleep(USER_SYNC_RETRY_DELAY * 1000);
        }
        if (i >= USER_SYNC_RETRY_MAX) {
            return  (PX_ERROR);
        }
    }
    
    sync(); /* sync to disk! */
    
    return  (ERROR_NONE);
}

/*
 * User make password
 */
static void user_mkpass (const char *passwd, char *pass)
{
#if LW_CFG_SHELL_PASS_CRYPT_EN > 0
    int     i;
    char    salt[14] = "$1$........$";
#endif /* LW_CFG_SHELL_PASS_CRYPT_EN > 0 */
    
    if (passwd && lib_strlen(passwd)) {
#if LW_CFG_SHELL_PASS_CRYPT_EN > 0
        for (i = 3; i < 12; i++) {
            int  type = (lib_rand() % 3);
            switch (type) {
            case 0: salt[i] = (char)((lib_rand() % 10) + '0'); break;
            case 1: salt[i] = (char)((lib_rand() % 26) + 'A'); break;
            case 2: salt[i] = (char)((lib_rand() % 26) + 'a'); break;
            }
        }
        crypt_safe(passwd, salt, pass, PASS_MAX);
#else
        lib_strlcpy(pass, passwd, PASS_MAX);
#endif /* LW_CFG_SHELL_PASS_CRYPT_EN > 0 */
    
    } else {
        lib_strcpy(pass, "!!");
    }
}

/*
 * User db end
 */
static void user_db_close (void)
{
    __resDelRawHook(&udb_resraw);
    
    user_unload();
    
    user_need_sync   = FALSE;
    group_need_sync  = FALSE;
    shadow_need_sync = FALSE;
    
    unlink(USER_DB_LOCK_FILE);
}

/*
 * User db start
 */
static int user_db_open (void)
{
    int  lock;
    
    init_etc_passwd_group();
    init_etc_shadow();
    
    if (geteuid() != 0) {
        errno = EACCES;
        return  (PX_ERROR);
    }
    
    lock = open(USER_DB_LOCK_FILE, O_CREAT | O_EXCL | O_WRONLY, 0400);
    if (lock <= 0) {
        errno = EBUSY;
        return  (PX_ERROR);
    }
    
    close(lock);
    
    if (user_load()) {
        unlink(USER_DB_LOCK_FILE);
        return  (PX_ERROR);
    }
    
    __resAddRawHook(&udb_resraw, user_db_close, 0, 0, 0, 0, 0, 0);
    
    return  (ERROR_NONE);
}

/*
 * User add 
 */
int  user_db_uadd (const char  *user, const char  *passwd, int enable,
                   uid_t  uid, gid_t  gid, 
                   const char *comment, const char  *home)
{
    user_t      *puser;
    group_t     *pgrp;
    shadow_t    *pshadow;
    
    BOOL         free_shadow = FALSE;
    char         pass[PASS_MAX];
    
    if (!user) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    /*
     *  link db base
     */
    if (user_db_open()) {
        return  (PX_ERROR);
    }
    
    puser = passwd_find(user, uid);
    if (puser) {
        user_db_close();
        errno = EEXIST;
        return  (PX_ERROR);
    }
    
    pgrp = group_find(NULL, gid);
    if (!pgrp) {
        errno = EINVAL;
        goto    error;
    }
    
    /*
     *  set user struct
     */
    puser = (user_t *)lib_malloc(sizeof(user_t));
    if (!puser) {
        goto    error;
    }
    lib_bzero(puser, sizeof(user_t));
    
    puser->name = lib_strdup(user);
    if (!puser->name) {
        goto    error;
    }
    
    if (enable) {
        puser->passwd = lib_strdup("x");
        if (!puser->passwd) {
            goto    error;
        }
    } else {
        puser->passwd = lib_strdup("!");
        if (!puser->passwd) {
            goto    error;
        }
    }
    
    if (passwd_set(puser, uid, gid, comment, home)) {
        goto    error;
    }
    
    /*
     *  set group struct
     */
    if (group_set(pgrp, GROUP_SET_ADD_MEM, puser->name)) {
        goto    error;
    }
    
    /*
     *  set shadow struct
     */
    user_mkpass(passwd, pass);
    
    pshadow = shadow_find(user);
    if (!pshadow) {
        pshadow = (shadow_t *)lib_malloc(sizeof(shadow_t));
        if (!pshadow) {
            goto    error;
        }
        lib_bzero(pshadow, sizeof(shadow_t));
        
        free_shadow = TRUE;
        
        pshadow->name = lib_strdup(user);
        if (!pshadow->name) {
            goto    error;
        }
    }
    
    if (shadow_set(pshadow, pass)) {
        goto    error;
    }
    
    /*
     * add user to update list
     */
    _List_Line_Add_Ahead(&puser->list, &user_in);
    if (!user_out) {
        user_out = user_in;
    }
    
    /*
     * add shadow to update list
     */
    if (free_shadow) {
        _List_Line_Add_Ahead(&pshadow->list, &shadow_in);
        if (!shadow_out) {
            shadow_out = shadow_in;
        }
    }
    
    user_need_sync   = TRUE;
    group_need_sync  = TRUE;
    shadow_need_sync = TRUE;
    
    /*
     * update
     */
    user_sync();
    user_db_close();
    
    return  (ERROR_NONE);
    
error:
    if (puser) {
        passwd_unload_one(puser);
        lib_free(puser);
    }
    
    if (free_shadow) {
        shadow_unload_one(pshadow);
        lib_free(pshadow);
    }
    
    user_db_close();
    
    return  (PX_ERROR);
}

/*
 * User modify 
 */
int  user_db_umod (const char  *user, int enable,
                   const char *comment, const char  *home)
{
    user_t  *puser;
    char    *newpass = NULL;
    
    if (!user) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    /*
     *  link db base
     */
    if (user_db_open()) {
        return  (PX_ERROR);
    }
    
    puser = passwd_find(user, (uid_t)-1);
    if (!puser) {
        errno = ENOENT;
        goto    error;
    }
    
    if (enable && lib_strcmp(puser->passwd, "x")) {
        newpass = lib_strdup("x");
        if (!newpass) {
            goto    error;
        }
        
    } else if (!enable && !lib_strcmp(puser->passwd, "x")) {
        newpass = lib_strdup("!");
        if (!newpass) {
            goto    error;
        }
    }
    
    if (newpass) {
        if (puser->passwd) {
            lib_free(puser->passwd);
        }
        puser->passwd = newpass;
    }

    if (passwd_set(puser, puser->uid, puser->gid, comment, home)) {
        goto    error;
    }
    
    user_need_sync = TRUE;
    
    /*
     * update
     */
    user_sync();
    user_db_close();
    
    return  (ERROR_NONE);
    
error:
    user_db_close();
    
    return  (PX_ERROR);
}

/*
 *  get user info
 */
int  user_db_uget (const char  *user, int *enable,
                   uid_t *uid, gid_t *gid, 
                   char *comment, size_t sz_com, char  *home, size_t sz_home)
{
    user_t  *puser;
    
    if (!user) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    /*
     *  link db base
     */
    if (user_db_open()) {
        return  (PX_ERROR);
    }
    
    puser = passwd_find(user, (uid_t)-1);
    if (!puser) {
        errno = ENOENT;
        goto    error;
    }
    
    if (enable) {
        if (lib_strcmp(puser->passwd, "x")) {
            *enable = 0;
        } else {
            *enable = 1;
        }
    }
    
    if (uid) {
        *uid = puser->uid;
    }
    
    if (gid) {
        *gid = puser->gid;
    }
    
    if (comment) {
        if (sz_com < (lib_strlen(puser->comment) + 1)) {
            errno = ENOSPC;
            goto    error;
        }
        lib_strcpy(comment, puser->comment);
    }
    
    if (home) {
        if (sz_home < (lib_strlen(puser->dir) + 1)) {
            errno = ENOSPC;
            goto    error;
        }
        lib_strcpy(home, puser->dir);
    }
    
    user_db_close();
    
    return  (ERROR_NONE);
    
error:
    user_db_close();
    
    return  (PX_ERROR);
}

/*
 *  user delete
 */
int  user_db_udel (const char  *user)
{
    user_t        *puser;
    group_t       *pgrp;
    shadow_t      *pshadow;
    PLW_LIST_LINE  pline;
    
    if (!user) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (lib_strcmp(user, "root") == 0) {
        errno = EACCES;
        return  (PX_ERROR);
    }
    
    /*
     *  link db base
     */
    if (user_db_open()) {
        return  (PX_ERROR);
    }
    
    puser = passwd_find(user, (uid_t)-1);
    if (!puser) {
        errno = ENOENT;
        goto    error;
    }
    
    if (getuid() == puser->uid) {
        errno = EBUSY;
        goto    error;
    }
    
    /*
     * remove user from list
     */
    if (user_out == &puser->list) {
        user_out =  _list_line_get_prev(user_out);
    }
    _List_Line_Del(&puser->list, &user_in);
    
    /*
     * remove user from group
     */
    for (pline  = group_in;
         pline != NULL;
         pline  = _list_line_get_next(pline)) {
        
        pgrp = _LIST_ENTRY(pline, group_t, list);
        group_set(pgrp, GROUP_SET_DEL_MEM, puser->name);
    }
    
    /*
     * remove shadow from list
     */
    pshadow = shadow_find(puser->name);
    if (pshadow) {
        if (shadow_out == &pshadow->list) {
            shadow_out =  _list_line_get_prev(shadow_out);
        }
        _List_Line_Del(&pshadow->list, &shadow_in);
        
        shadow_unload_one(pshadow);
        lib_free(pshadow);
    }
    
    /*
     * free puser
     */
    passwd_unload_one(puser);
    lib_free(puser);
    
    user_need_sync   = TRUE;
    group_need_sync  = TRUE;
    shadow_need_sync = TRUE;
    
    /*
     * update
     */
    user_sync();
    user_db_close();
    
    return  (ERROR_NONE);
    
error:
    user_db_close();
    
    return  (PX_ERROR);
}

/*
 * Add group
 */
int  user_db_gadd (const char  *group, gid_t  gid)
{
    group_t  *pgrp;
    
    if (!group) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    /*
     *  link db base
     */
    if (user_db_open()) {
        return  (PX_ERROR);
    }
    
    pgrp = group_find(group, gid);
    if (pgrp) {
        user_db_close();
        errno = EEXIST;
        return  (PX_ERROR);
    }
    
    /*
     *  set user struct
     */
    pgrp = (group_t *)lib_malloc(sizeof(group_t));
    if (!pgrp) {
        goto    error;
    }
    lib_bzero(pgrp, sizeof(group_t));
    
    pgrp->name = lib_strdup(group);
    if (!pgrp->name) {
        goto    error;
    }
    
    pgrp->passwd = lib_strdup("x");
    if (!pgrp->passwd) {
        goto    error;
    }
    
    pgrp->allmember = lib_strdup("");
    if (!pgrp->allmember) {
        goto    error;
    }
    
    pgrp->gid = gid;
    
    /*
     * add group to update list
     */
    _List_Line_Add_Ahead(&pgrp->list, &group_in);
    if (!group_out) {
        group_out = group_in;
    }
    
    group_need_sync = TRUE;
    
    /*
     * update
     */
    user_sync();
    user_db_close();
    
    return  (ERROR_NONE);
    
error:
    if (pgrp) {
        group_unload_one(pgrp);
        lib_free(pgrp);
    }
    
    user_db_close();
    
    return  (PX_ERROR);
}

/*
 * Group get
 */
int  user_db_gget (const char  *group, gid_t *gid)
{
    group_t  *pgrp;
    
    if (!group) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    /*
     *  link db base
     */
    if (user_db_open()) {
        return  (PX_ERROR);
    }
    
    pgrp = group_find(group, (gid_t)-1);
    if (!pgrp) {
        errno = ENOENT;
        goto    error;
    }
    
    if (gid) {
        *gid = pgrp->gid;
    }
    
    user_db_close();
    
    return  (ERROR_NONE);
    
error:
    user_db_close();
    
    return  (PX_ERROR);
}

/*
 * Group delete
 */
int  user_db_gdel (const char  *group)
{
    group_t  *pgrp;
    
    if (!group) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    /*
     *  link db base
     */
    if (user_db_open()) {
        return  (PX_ERROR);
    }
    
    pgrp = group_find(group, (gid_t)-1);
    if (!pgrp) {
        errno = ENOENT;
        goto    error;
    }
    
    if (lib_strlen(pgrp->allmember)) {
        errno = ENOTEMPTY;
        goto    error;
    }
    
    /*
     * remove group from list
     */
    if (group_out == &pgrp->list) {
        group_out =  _list_line_get_prev(group_out);
    }
    _List_Line_Del(&pgrp->list, &group_in);
    
    group_unload_one(pgrp);
    lib_free(pgrp);
    
    group_need_sync = TRUE;
    
    /*
     * update
     */
    user_sync();
    user_db_close();
    
    return  (ERROR_NONE);
    
error:
    user_db_close();
    
    return  (PX_ERROR);
}

/*
 * Change password
 */
int  user_db_pmod (const char  *user, const char  *passwd_old, const char  *passwd_new)
{
    shadow_t *pshadow;
    char      pass[PASS_MAX];
    
    if (!user) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (getuid() != 0) { /* 'root' user do not check old password */
        if (passwdcheck(user, passwd_old)) {
            errno = EACCES;
            return  (PX_ERROR);
        }
    }
    
    /*
     *  link db base
     */
    if (user_db_open()) {
        return  (PX_ERROR);
    }
    
    /*
     *  set shadow struct
     */
    user_mkpass(passwd_new, pass);
    
    pshadow = shadow_find(user);
    if (!pshadow) {
        errno = EINVAL;
        goto    error;
    }
    
    if (shadow_set(pshadow, pass)) {
        goto    error;
    }
    
    shadow_need_sync = TRUE;
    
    /*
     * update
     */
    user_sync();
    user_db_close();
    
    return  (ERROR_NONE);
    
error:
    user_db_close();
    
    return  (PX_ERROR);
}

/*
 * end
 */