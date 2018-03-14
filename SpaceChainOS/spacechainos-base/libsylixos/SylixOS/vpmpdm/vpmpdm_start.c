/*
 * SylixOS(TM)  LW : long wing
 * Copyright All Rights Reserved
 *
 * MODULE PRIVATE DYNAMIC MEMORY IN VPROCESS PATCH
 * this file is support current module malloc/free use his own heap in a vprocess.
 * 
 * Author: Han.hui <sylixos@gmail.com>
 */
 
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL /* need some kernel function */
#include <sys/cdefs.h>
#include <SylixOS.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <pwd.h>
#include <limits.h>

#ifdef __GNUC__
#if __GNUC__ < 2  || (__GNUC__ == 2 && __GNUC_MINOR__ < 5)
#error must use GCC include "__attribute__((...))" function.
#endif /*  __GNUC__ < 2  || ...        */
#endif /*  __GNUC__                    */

#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)
#define __LIB_PERROR(s) perror(s)
#else
#define __LIB_PERROR(s)
#endif /*  (LW_CFG_DEVICE_EN > 0)...   */

/*
 *  environ
 */
char **environ;
static char pwd[PATH_MAX + 1];

extern void __vp_patch_lock(void);
extern void __vp_patch_unlock(void);

#define __ENV_LOCK()      __vp_patch_lock()
#define __ENV_UNLOCK()    __vp_patch_unlock()

/*
 *  _init_env
 */
LW_CONSTRUCTOR_BEGIN
LW_LIB_HOOK_STATIC void __vp_patch_init_env (void)
{
    int  sysnum;
    int  dupnum;
    struct passwd   passwd;
    struct passwd  *ppasswd = LW_NULL;
    char user[LOGIN_NAME_MAX];
    
    sysnum = API_TShellVarGetNum();
    
    environ = malloc(sizeof(char *) * (sysnum + 1)); /* allocate environ space */
    if (!environ) {
        return;
    }
    
    dupnum = API_TShellVarDup(malloc, environ, sysnum);
    if (dupnum < 0) {
        dupnum = 0;
    }
    
    environ[dupnum] = NULL;

    if (!getpwuid_r(getuid(), &passwd, user, LOGIN_NAME_MAX, &ppasswd)) {
        setenv("USER", user, 1);
        setenv("HOME", ppasswd->pw_dir, 1);
    }
}
LW_CONSTRUCTOR_END(__vp_patch_init_env)

/*
 *  _deinit_env
 */
LW_DESTRUCTOR_BEGIN
LW_LIB_HOOK_STATIC void __vp_patch_deinit_env (void)
{
    int  i;
    
    if (environ) {
        for (i = 0; environ[i]; i++) {
            free(environ[i]);
        }
        free(environ);
    }
}
LW_DESTRUCTOR_END(__vp_patch_deinit_env)

/*
 *  _findenv
 */
static char *_find_env (const char *name, int *offset)
{
    size_t len;
    const char *np;
    char **p, *c;

    if (name == NULL || environ == NULL)
        return NULL;
    for (np = name; *np && *np != '='; ++np)
        continue;
    len = np - name;
    for (p = environ; (c = *p) != NULL; ++p)
        if (strncmp(c, name, len) == 0 && c[len] == '=') {
            *offset = p - environ;
            return c + len + 1;
        }
    return NULL;
}

/*
 *  vprocess entry point
 */
int _start (int argc, char **argv, char **env)
{
    int   i, idx, off, ret;
    int   oldnum = 0;
    int   addnum = 0;
    char  *tmp, *old, **new;

    extern FUNCPTR vprocGetMain(VOID);
    
    FUNCPTR pfuncMain  = vprocGetMain(); /* must use MAIN MODULE main() function! */
    ULONG   ulVersion  = API_KernelVersion();
    
    /*
     *  check sylixos version
     */
    if (ulVersion < __SYLIXOS_MAKEVER(1, 4, 0)) {
        fprintf(stderr, "sylixos version MUST equ or higher than 1.4.0\n");
        return  (EXIT_FAILURE);
    }
    
    /*
     * init command-line environ
     */
    if (env) {
        for (addnum = 0; env[addnum]; addnum++) {
            continue;
        }
        if (addnum > 0) {
            if (environ) {
                for (oldnum = 0; environ[oldnum]; oldnum++) {
                    continue;
                }
                new = realloc(environ, sizeof(char *) * (addnum + oldnum + 1));

            } else {
                new = malloc(sizeof(char *) * (addnum + 1));
                new[0] = NULL;
            }

            if (new) {
                environ = new;
                for (i = 0, idx = 0; i < addnum; i++) {
                    old = _find_env(env[i], &off);
                    if (old) {
                        tmp = strdup(env[i]); /* set new value */
                        if (tmp) {
                            free(environ[off]); /* free old value */
                            environ[off] = tmp;

                        } else {
                            fprintf(stderr, "environment variable initialization error: "
                                            "Not enough memory\n");
                        }

                    } else {
                        environ[oldnum + idx] = strdup(env[i]); /* add new value */
                        environ[oldnum + idx + 1] = NULL;
                        idx++;
                    }
                }
                environ[oldnum + idx] = NULL; /* end */
            }
        }
    }

    errno = 0;
    ret = pfuncMain(argc, argv, environ);
    exit(ret);
    
    return  (ret);
}

/*
 *  the following code is from BSD lib
 */
/*
 *  getenv
 */
char *getenv (const char *name)
{
    int offset;
    char *result;

    if (!name) {
        return (NULL);
    }

    if (lib_strcmp(name, "PWD") == 0) {
        getcwd(pwd, PATH_MAX + 1);
        pwd[PATH_MAX] = '\0';
        return (pwd);
    }

    __ENV_LOCK();
    result = _find_env(name, &offset);
    __ENV_UNLOCK();
    return result;
}

/*
 *  getenv_r
 */
int getenv_r (const char *name, char *buf, int len)
{
    int offset;
    char *result;
    int rv = -1;

    if (!name || !buf) {
        return (-1);
    }

    if (lib_strcmp(name, "PWD") == 0) {
        getcwd(pwd, PATH_MAX + 1);
        pwd[PATH_MAX] = '\0';
        if (strlcpy(buf, pwd, len) >= len) {
            errno = ERANGE;
            return (-1);
        }
        return (0);
    }

    __ENV_LOCK();
    result = _find_env(name, &offset);
    if (result == NULL) {
        errno = ENOENT;
        goto out;
    }
    if (strlcpy(buf, result, len) >= len) {
        errno = ERANGE;
        goto out;
    }
    rv = 0;
out:
    __ENV_UNLOCK();
    return rv;
}

/*
 *  setenv
 */
int setenv (const char *name, const char *value, int rewrite)
{
    static char **saveenv;    /* copy of previously allocated space */
    char *c, **newenv;
    const char *cc;
    size_t l_value, size;
    int offset;

    if (!name || !value) {
        return (-1);
    }

    if (*value == '=')            /* no `=' in value */
        ++value;
    l_value = strlen(value);
    __ENV_LOCK();
    /* find if already exists */
    if ((c = _find_env(name, &offset)) != NULL) {
        if (!rewrite)
            goto good;
        if (strlen(c) >= l_value)    /* old larger; copy over */
            goto copy;
    } else {                    /* create new slot */
        size_t cnt;

        for (cnt = 0; environ[cnt]; ++cnt)
            continue;
        size = (size_t)(sizeof(char *) * (cnt + 2));
        if (saveenv == environ) {        /* just increase size */
            if ((newenv = realloc(saveenv, size)) == NULL)
                goto bad;
            saveenv = newenv;
        } else {                /* get new space */
            free(saveenv);
            if ((saveenv = malloc(size)) == NULL)
                goto bad;
            (void)memcpy(saveenv, environ, cnt * sizeof(char *));
        }
        environ = saveenv;
        environ[cnt + 1] = NULL;
        offset = (int)cnt;
    }
    for (cc = name; *cc && *cc != '='; ++cc)    /* no `=' in name */
        continue;
    size = cc - name;
    /* name + `=' + value */
    if ((environ[offset] = malloc(size + l_value + 2)) == NULL)
        goto bad;
    c = environ[offset];
    (void)memcpy(c, name, size);
    c += size;
    *c++ = '=';
copy:
    (void)memcpy(c, value, l_value + 1);
good:
    __ENV_UNLOCK();
    return 0;
bad:
    __ENV_UNLOCK();
    return -1;
}

/*
 *  unsetenv
 */
int unsetenv (const char *name)
{
    char **p;
    int offset;

    if (name == NULL || *name == '\0' || strchr(name, '=') != NULL) {
        errno = EINVAL;
        return -1;
    }

    __ENV_LOCK();
    while (_find_env(name, &offset))    /* if set multiple times */
        for (p = &environ[offset];; ++p)
            if (!(*p = *(p + 1)))
                break;
    __ENV_UNLOCK();

    return 0;
}
/*
 * end
 */
