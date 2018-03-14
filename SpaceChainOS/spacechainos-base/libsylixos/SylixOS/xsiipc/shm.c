/**
 * @file
 * SylixOS XSI IPC support kernel module.
 */

/*
 * Copyright (c) 2006-2012 SylixOS Group.
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
#include <module.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <errno.h>
#include <time.h>
#include <string.h>

#include "xsiipc.h"

typedef struct shm_map {
    LW_LIST_LINE    list;
    void           *virmem;
    pid_t           pid;
    ipc_id_shm_t   *shm;
} shm_map_t;

static LW_LIST_LINE_HEADER map_header = NULL;

/*
 * init shm
 */
void ipc_shm_init (void)
{
}

/*
 * allocate a shm
 */
static int do_newshm (key_t key, size_t size, int shmflg)
{
    ipc_id_shm_t    *shm;
    int idnum;

    if (size == 0) {
        errno = EINVAL;
        return  (IPC_ERROR);
    }

    shm = (ipc_id_shm_t *)sys_malloc(sizeof(ipc_id_shm_t));
    if (shm == NULL) {
        errno = ENOMEM;
        return  (IPC_ERROR);
    }
    bzero(shm, sizeof(ipc_id_shm_t));

    shm->ipc.key  = key;
    shm->ipc.type = IPC_TYPE_SHM;

    shm->desc.shm_perm.mode = shmflg & 0666; /* no exec prem */
    shm->desc.shm_perm.uid  = geteuid();
    shm->desc.shm_perm.gid  = getegid();
    shm->desc.shm_perm.cuid = shm->desc.shm_perm.uid;
    shm->desc.shm_perm.cgid = shm->desc.shm_perm.gid;

    shm->desc.shm_atime    = 0;
    shm->desc.shm_cpid     = getpid();
    shm->desc.shm_ctime    = time(NULL);
    shm->desc.shm_dtime    = 0;
    shm->desc.shm_internal = NULL;
    shm->desc.shm_lpid     = 0;
    shm->desc.shm_nattch   = 0;
    shm->desc.shm_segsz    = ROUND_UP(size, LW_CFG_VMM_PAGE_SIZE);

    shm->phymem = NULL;
    shm->rmreq  = 0;

    idnum = ipc_insert_instance(&shm->ipc);
    if (idnum < 0) {
        sys_free(shm);
        errno = ENOSPC;
        return  (IPC_ERROR);
    }

    return  (idnum);
}

/*
 * delete a shm
 */
static int do_deleteshm (int idnum)
{
    ipc_id_shm_t    *shm;
    shm_map_t       *map_node;
    uid_t            uid_cur = geteuid();
    PLW_LIST_LINE    tmp;

    shm = (ipc_id_shm_t *)ipc_get_instance(idnum, IPC_TYPE_SHM);
    if (shm == NULL) {
        errno = EINVAL;
        return  (IPC_ERROR);
    }

    if ((uid_cur != 0) &&
        (uid_cur != shm->desc.shm_perm.uid) &&
        (uid_cur != shm->desc.shm_perm.cuid)) {
        errno = EACCES;
        return  (IPC_ERROR);
    }

    if (shm->desc.shm_nattch) { /* can not remove this shm */
        shm->rmreq = 1; /* remove require */
        ipc_change_key(&shm->ipc, IPC_PRIVATE); /* Do not find it any more */
        return  (IPC_OK);
    }

    tmp = map_header;
    while (tmp) {
        map_node = _LIST_ENTRY(tmp, shm_map_t, list);
        tmp = _list_line_get_next(tmp);
        if (map_node->shm == shm) {
            _List_Line_Del(&map_node->list, &map_header);
            API_VmmFreeArea(map_node->virmem);
            sys_free(map_node);
        }
    }

    if (shm->phymem) {
        API_VmmPhyFree(shm->phymem);
    }

    ipc_remove_instance(&shm->ipc);

    sys_free(shm);

    return  (IPC_OK);
}

/*
 * get a shm stat
 */
static int do_getshmstat (int idnum, struct shmid_ds *buf)
{
    ipc_id_shm_t    *shm;

    shm = (ipc_id_shm_t *)ipc_get_instance(idnum, IPC_TYPE_SHM);
    if (shm == NULL) {
        errno = EINVAL;
        return  (IPC_ERROR);
    }

    *buf = shm->desc;

    return  (IPC_OK);
}

/*
 * set a shm stat
 */
static int do_setshmstat (int idnum, struct shmid_ds *buf)
{
    ipc_id_shm_t    *shm;
    uid_t            uid_cur = geteuid();

    shm = (ipc_id_shm_t *)ipc_get_instance(idnum, IPC_TYPE_SHM);
    if (shm == NULL) {
        errno = EINVAL;
        return  (IPC_ERROR);
    }

    if ((uid_cur != 0) &&
        (uid_cur != shm->desc.shm_perm.uid) &&
        (uid_cur != shm->desc.shm_perm.cuid)) {
        errno = EACCES;
        return  (IPC_ERROR);
    }

    shm->desc.shm_perm.uid  = buf->shm_perm.uid;
    shm->desc.shm_perm.gid  = buf->shm_perm.gid;
    shm->desc.shm_perm.mode = buf->shm_perm.mode;

    shm->desc.shm_ctime = time(NULL);

    return  (IPC_OK);
}

/*
 * lock a shm into memory
 */
static int do_lockshm (int idnum, int lock)
{
    ipc_id_shm_t    *shm;
    uid_t            uid_cur = geteuid();
    pid_t            pid_cur = getpid();
    shm_map_t       *map_node;
    PLW_LIST_LINE    tmp;
    int              ret;

    shm = (ipc_id_shm_t *)ipc_get_instance(idnum, IPC_TYPE_SHM);
    if (shm == NULL) {
        errno = EINVAL;
        return  (IPC_ERROR);
    }

    if (uid_cur != 0) {
        errno = EACCES;
        return  (IPC_ERROR);
    }

    for (tmp  = map_header;
         tmp != NULL;
         tmp  = _list_line_get_next(tmp)) {

        map_node = _LIST_ENTRY(tmp, shm_map_t, list);
        if ((map_node->pid == pid_cur) &&
            (map_node->shm == shm)) {

            if (lock) {
                ret = mlock(map_node->virmem, shm->desc.shm_segsz);
            } else {
                ret = munlock(map_node->virmem, shm->desc.shm_segsz);
            }

            if (ret < 0) {
                return  (ret);
            }
        }
    }

    return  (IPC_OK);
}

/*
 * attach shm
 */
static void *do_shmat (int idnum, int flag)
{
    ipc_id_shm_t    *shm;
    pid_t            pid_cur = getpid();
    shm_map_t       *map_node;
    u_long           map_flag;
    int              err = 0;

    shm = (ipc_id_shm_t *)ipc_get_instance(idnum, IPC_TYPE_SHM);
    if (shm == NULL) {
        errno = EINVAL;
        return  (MAP_FAILED);
    }

    if (shm->phymem == NULL) { /* attach == 0 */
        shm->phymem = API_VmmPhyAlloc(shm->desc.shm_segsz);
        if (shm->phymem == NULL) {
            errno = ENOMEM;
            return  (MAP_FAILED);
        }
    }

    map_node = (shm_map_t *)sys_malloc(sizeof(shm_map_t));
    if (map_node == NULL) {
        errno = ENOMEM;
        goto    error;
    }

    if (flag & SHM_RDONLY) {
        map_flag = LW_VMM_FLAG_READ;
    } else {
        map_flag = LW_VMM_FLAG_RDWR;
    }

    if (API_CacheAliasProb()) {
        map_flag &= ~(LW_VMM_FLAG_CACHEABLE | LW_VMM_FLAG_BUFFERABLE); /* MAP_SHARED (non-cache) */
    }

    map_node->shm    = shm;
    map_node->pid    = pid_cur;
    map_node->virmem = API_VmmMallocAreaEx(shm->desc.shm_segsz, NULL, NULL, MAP_SHARED, map_flag);
    if (map_node->virmem == NULL) {
        errno = ENOMEM;
        err = 1;
        goto    error;
    }

    if (API_VmmRemapArea(map_node->virmem, shm->phymem, shm->desc.shm_segsz, map_flag, NULL, NULL)) {
        err = 2;
        goto    error;
    }

    _List_Line_Add_Ahead(&map_node->list, &map_header);

    shm->desc.shm_nattch++;
    shm->desc.shm_lpid  = pid_cur;
    shm->desc.shm_atime = time(NULL);

    return  (map_node->virmem);

error:
    if (err > 1) {
        API_VmmFreeArea(map_node->virmem);
    }
    if (err > 0) {
        sys_free(map_node);
    }
    if ((shm->desc.shm_nattch <= 0) && shm->phymem) {
        API_VmmPhyFree(shm->phymem);
        shm->phymem = NULL;
    }
    return  (MAP_FAILED);
}

/*
 * dettach shm
 */
static int do_shmdt (const void *mem)
{
    ipc_id_shm_t    *shm;
    pid_t            pid_cur = getpid();
    shm_map_t       *map_node;
    PLW_LIST_LINE    tmp;

    for (tmp  = map_header;
         tmp != NULL;
         tmp  = _list_line_get_next(tmp)) {

        map_node = _LIST_ENTRY(tmp, shm_map_t, list);
        if ((map_node->pid == pid_cur) &&
            (map_node->virmem == mem)) {
            break;
        }
    }

    if (tmp == NULL) {
        errno = EINVAL;
        return  (IPC_ERROR);
    }

    shm = map_node->shm;

    _List_Line_Del(&map_node->list, &map_header);

    API_VmmFreeArea(map_node->virmem);

    sys_free(map_node);

    shm->desc.shm_nattch--;
    shm->desc.shm_lpid  = pid_cur;
    shm->desc.shm_dtime = time(NULL);

    if ((shm->desc.shm_nattch <= 0) && shm->phymem) {
        API_VmmPhyFree(shm->phymem);
        shm->phymem = NULL;

        if (shm->rmreq) { /* required to remove this shm */
            ipc_remove_instance(&shm->ipc);
            sys_free(shm);
        }
    }

    return  (IPC_OK);
}

/*
 * ipc sharememory get
 */
LW_SYMBOL_EXPORT
int shmget (key_t key, size_t size, int shmflg)
{
    ipc_id_shm_t    *shm;
    int shmid = IPC_ERROR;

    ipc_lock();

    if (key == IPC_PRIVATE) {
        shmid = do_newshm(key, size, shmflg);
    } else {
        shm = (ipc_id_shm_t *)ipc_find_instance(key, IPC_TYPE_SHM);
        if (shm) {
            if ((shmflg & IPC_CREAT) && (shmflg & IPC_EXCL)) {
                errno = EEXIST;
            } else {
                if (size > shm->desc.shm_segsz) {
                    errno = EINVAL;
                } else if (ipc_perms(&shm->desc.shm_perm, shmflg) < 0) {
                    errno = EACCES;
                } else {
                    shmid = shm->ipc.idnum;
                }
            }
        } else {
            if (shmflg & IPC_CREAT) {
                shmid = do_newshm(key, size, shmflg);
            } else {
                errno = ENOENT;
            }
        }
    }

    ipc_unlock();

    return  (shmid);
}

/*
 * ipc sharememory control
 */
LW_SYMBOL_EXPORT
int shmctl (int shmid, int cmd, struct shmid_ds *buf)
{
    int ret = IPC_ERROR;

    switch (cmd) {

    case IPC_STAT:
        if (!buf) {
            errno = EINVAL;
        } else {
            ipc_lock();
            ret = do_getshmstat(shmid, buf);
            ipc_unlock();
        }
        break;

    case IPC_SET:
        if (!buf) {
            errno = EINVAL;
        } else {
            ipc_lock();
            ret = do_setshmstat(shmid, buf);
            ipc_unlock();
        }
        break;

    case IPC_RMID:
        ipc_lock();
        ret = do_deleteshm(shmid);
        ipc_unlock();
        break;

    case SHM_LOCK:
        ipc_lock();
        ret = do_lockshm(shmid, 1);
        ipc_unlock();
        break;

    case SHM_UNLOCK:
        ipc_lock();
        ret = do_lockshm(shmid, 0);
        ipc_unlock();
        break;

    default:
        errno = ENOSYS;
        break;
    }

    return  (ret);
}

/*
 * ipc sharememory attach
 */
LW_SYMBOL_EXPORT
void *shmat (int shmid, const void *shmaddr, int shmflg)
{
    void *mem;

    if (shmaddr != LW_NULL) {
        errno = EINVAL;
        return  (MAP_FAILED);
    }

    ipc_lock();
    mem = do_shmat(shmid, shmflg);
    ipc_unlock();

    return  (mem);
}

/*
 * ipc sharememory dettach
 */
LW_SYMBOL_EXPORT
int shmdt (const void *addr)
{
    int ret;

    if ((addr == LW_NULL) || (addr == MAP_FAILED)) {
        errno = EINVAL;
        return  (IPC_ERROR);
    }

    ipc_lock();
    ret = do_shmdt(addr);
    ipc_unlock();

    return  (ret);
}

/*
 * hook
 */
void hook_shm_pid_remove (pid_t pid)
{
    ipc_id_shm_t    *shm;
    shm_map_t       *map_node;
    PLW_LIST_LINE    tmp;

    ipc_lock();

    tmp = map_header;
    while (tmp) {
        map_node = _LIST_ENTRY(tmp, shm_map_t, list);
        tmp = _list_line_get_next(tmp);

        if (map_node->pid == pid) {
            shm = map_node->shm;

            _List_Line_Del(&map_node->list, &map_header);
            API_VmmFreeArea(map_node->virmem);
            sys_free(map_node);

            shm->desc.shm_nattch--;
            shm->desc.shm_lpid  = pid;
            shm->desc.shm_dtime = time(NULL);

            if ((shm->desc.shm_nattch <= 0) && shm->phymem) {
                API_VmmPhyFree(shm->phymem);
                shm->phymem = NULL;

                if (shm->rmreq) { /* required to remove this shm */
                    ipc_remove_instance(&shm->ipc);
                    sys_free(shm);
                }
            }
        }
    }

    ipc_unlock();
}

/*
 * end
 */
