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

#ifndef __XSIIPC_H
#define __XSIIPC_H

#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/shm.h>

/*
 * ipc version
 * 0.0.4    2013.11.28 add thread test point in msgrcv() and msgsnd()
 * 0.0.5    2016.04.13 fixed semctl() va_arg() bug.
 * 0.0.6    2016.04.20 fixed cache alias prob.
 * 0.0.7    2016.08.19 fixed module remove procfs node delete prob.
 */
#define IPC_VER         "0.0.7"

/*
 * ipc config
 */
#define IPC_MAX         1024   /* max ipc node semphore, message-queue, share-memory */

#define IPC_MAX_MSG     4096   /* max size of one message */
#define IPC_MAX_MSGS    8      /* max message in a msg queue */

#define IPC_MAX_NSEM    250    /* max nsem in one semaphore */
#define IPC_MAX_VALUE   32767  /* max value of semaphore */

/*
 * ipc return value
 */
#define IPC_ERROR       (-1)
#define IPC_OK          (0)

/*
 * ipc module type
 */
#define IPC_TYPE_SEM    0
#define IPC_TYPE_MSG    1
#define IPC_TYPE_SHM    2

/*
 * ipc_t structure
 */
typedef struct ipc_id {
    LW_LIST_LINE  list;
    key_t         key;
    int           type;
    int           idnum;
} ipc_id_t;

extern LW_HANDLE ipc_lock_mutex; /* IPC lock */
extern ipc_id_t *ipc_handle_table[IPC_MAX]; /* ipc id number table */
extern u_long    ipc_cnt[3];

void        ipc_lock(void);
void        ipc_unlock(void);
ipc_id_t   *ipc_find_instance(key_t  key, int type);
ipc_id_t   *ipc_get_instance(int  idnum, int type);
int         ipc_insert_instance(ipc_id_t *ipc_id);
void        ipc_remove_instance(ipc_id_t *ipc_id);
void        ipc_change_key(ipc_id_t *ipc_id, key_t new_key);
int         ipc_perms(struct ipc_perm *perm, int flag);

/*
 * ipc semaphore structure
 */
typedef struct ipc_id_sem {
    ipc_id_t            ipc;
    struct semid_ds     desc;
    struct sem         *semarry;
    LW_HANDLE           sem_notify;
} ipc_id_sem_t;

/*
 * ipc message structure
 */
typedef struct ipc_id_msg {
    ipc_id_t             ipc;
    struct msqid_ds      desc;
    void                *msg_buf;
    LW_HANDLE            msg_wlock;
    LW_HANDLE            msg_rlock;
    LW_LIST_RING_HEADER  msg_header;
    LW_LIST_RING_HEADER  msg_free;
} ipc_id_msg_t;

/*
 * ipc sharememory structure
 */
typedef struct ipc_id_shm {
    ipc_id_t            ipc;
    struct shmid_ds     desc;
    void               *phymem;
    int                 rmreq;
} ipc_id_shm_t;

/*
 * hook
 */
void hook_sem_pid_remove(pid_t pid);
void hook_msg_pid_remove(pid_t pid);
void hook_shm_pid_remove(pid_t pid);

/*
 * init
 */
void ipc_sem_init(void);
void ipc_msg_init(void);
void ipc_shm_init(void);

/*
 * procfs node
 */
void ipc_proc_init(void);
void ipc_proc_deinit(void);

#endif /* __XSIIPC_H */
/*
 * end
 */
