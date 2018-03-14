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
#include <sys/stat.h>
#include <errno.h>

#include "xsiipc.h"

#define IPC_HASH_SIZE   32
#define IPC_HASH_MASK   (IPC_HASH_SIZE - 1)

LW_HANDLE ipc_lock_mutex; /* IPC lock */
ipc_id_t *ipc_handle_table[IPC_MAX]; /* ipc id number table */
u_long    ipc_cnt[3];

/*
 * ipc private lock and ipc id
 */
static LW_LIST_LINE_HEADER ipc_id_hash_header[IPC_HASH_MASK]; /* ipc key hash table */

/*
 * ipc exit hook
 */
static void hook_pid_remove (pid_t pid)
{
    extern void hook_shm_pid_remove(pid_t pid);

    hook_sem_pid_remove(pid);
    hook_msg_pid_remove(pid);
    hook_shm_pid_remove(pid);
}

/*
 * ipc lock
 */
void ipc_lock (void)
{
    API_SemaphoreMPend(ipc_lock_mutex, LW_OPTION_WAIT_INFINITE);
}

/*
 * ipc unlock
 */
void ipc_unlock (void)
{
    API_SemaphoreMPost(ipc_lock_mutex);
}

/*
 * find ipc_id
 */
ipc_id_t *ipc_find_instance (key_t  key, int type)
{
    int index = key & IPC_HASH_MASK;
    PLW_LIST_LINE tmp;
    ipc_id_t *ipc_id;

    for (tmp = ipc_id_hash_header[index];
         tmp != NULL;
         tmp = _list_line_get_next(tmp)) {

        ipc_id = _LIST_ENTRY(tmp, ipc_id_t, list);
        if ((ipc_id->key == key) && (ipc_id->type == type)) {
            return  (ipc_id);
        }
    }

    return  (NULL);
}

/*
 * get ipc_id by index
 */
ipc_id_t *ipc_get_instance (int  idnum, int type)
{
    ipc_id_t *ipc_id;

    if ((idnum < 0) || (idnum > IPC_MAX)) {
        return  (NULL);
    }

    ipc_id = ipc_handle_table[idnum];

    if (ipc_id->type != type) {
        return  (NULL);
    }

    return  (ipc_id);
}

/*
 * insert ipc_id
 * return ipc id number
 */
int ipc_insert_instance (ipc_id_t *ipc_id)
{
    static int idnum = 0; /* static var */
    int index = ipc_id->key & IPC_HASH_MASK;
    int i;

    for (i = 0; i < IPC_MAX; i++) {
        if (idnum >= IPC_MAX) {
            idnum =  0;
        }
        if (ipc_handle_table[idnum] == NULL) {
            ipc_handle_table[idnum] =  ipc_id;
            ipc_id->idnum = idnum;
            idnum++;
            break;
        } else {
            idnum++;
        }
    }

    if (i >= IPC_MAX) {
        return  (IPC_ERROR);
    }

    _List_Line_Add_Ahead(&ipc_id->list, &ipc_id_hash_header[index]);

    ipc_cnt[ipc_id->type]++;

    return  (ipc_id->idnum);
}

/*
 * remove ipc_id
 * return ipc id number
 */
void ipc_remove_instance (ipc_id_t *ipc_id)
{
    int index = ipc_id->key & IPC_HASH_MASK;

    ipc_handle_table[ipc_id->idnum] = NULL;

    _List_Line_Del(&ipc_id->list, &ipc_id_hash_header[index]);

    ipc_cnt[ipc_id->type]--;
}

/*
 * change ipc_id key number
 */
void ipc_change_key (ipc_id_t *ipc_id, key_t new_key)
{
    int index = ipc_id->key & IPC_HASH_MASK;

    _List_Line_Del(&ipc_id->list, &ipc_id_hash_header[index]);

    ipc_id->key = new_key;

    index = ipc_id->key & IPC_HASH_MASK;

    _List_Line_Add_Ahead(&ipc_id->list, &ipc_id_hash_header[index]);
}

/*
 * check IPC permissions
 */
int ipc_perms (struct ipc_perm *perm, int flag)
{
    if (_IosCheckPermissions(O_RDWR, FALSE, perm->mode, perm->uid, perm->gid) < 0) {
        return  (_IosCheckPermissions(O_RDWR, FALSE, perm->mode, perm->cuid, perm->cgid));
    }

    return  (IPC_OK);
}

/*
 * get key by path & id
 * (key_t in sylixos is 64bits)
 */
LW_SYMBOL_EXPORT
key_t  ftok(const char*  path, int  id)
{
    struct stat   st;
    key_t   key;

    if (lstat(path, &st) < 0) {
        return  ((key_t)-1);
    }

    key  = (((key_t)(id & 255)) << 48);
    key |= (((key_t)(st.st_dev & 0xffffff)) << 32);
    key |= (key_t)st.st_ino & 0xffffffff;

    return  (key);
}

/*
 * ipc module init
 */
int module_init (void)
{
    ipc_lock_mutex = API_SemaphoreMCreate("ipc_lock", LW_PRIO_DEF_CEILING,
                                          LW_OPTION_WAIT_PRIORITY | LW_OPTION_INHERIT_PRIORITY |
                                          LW_OPTION_DELETE_SAFE |
                                          LW_OPTION_OBJECT_GLOBAL,
                                          LW_NULL);
    if (ipc_lock_mutex == LW_HANDLE_INVALID) {
        printk(KERN_ERR "ipc lock create error.\n");
    }

    ipc_cnt[IPC_TYPE_SEM] = 0;
    ipc_cnt[IPC_TYPE_MSG] = 0;
    ipc_cnt[IPC_TYPE_SHM] = 0;

    API_SystemHookAdd(hook_pid_remove, LW_OPTION_VPROC_DELETE_HOOK);

    ipc_sem_init();
    ipc_msg_init();
    ipc_shm_init();
    ipc_proc_init(); /* init procfs file node */
	
	return  (ERROR_NONE);
}

/*
 * ipc module exit
 */
void module_exit (void)
{
    ipc_proc_deinit();

    if (ipc_lock_mutex != LW_HANDLE_INVALID) {
        API_SemaphoreMDelete(&ipc_lock_mutex);
    }

    API_SystemHookDelete(hook_pid_remove, LW_OPTION_VPROC_DELETE_HOOK);
}

/*
 * end
 */
