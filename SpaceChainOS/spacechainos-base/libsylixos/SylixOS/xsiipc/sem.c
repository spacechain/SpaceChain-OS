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

#define  __SYLIXOS_STDARG
#define  __SYLIXOS_KERNEL
#include <module.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <errno.h>
#include <string.h>

#include "xsiipc.h"

/*
 * sem undo structure
 */
typedef struct {
    LW_LIST_LINE    list;
    int             sem_id;
    pid_t           sem_pid;
    short          *sem_adj;
} ipc_sem_undo_t;

static LW_LIST_LINE_HEADER  sem_undo_header = NULL;

#define SEM_WAIT(mutex, sem, timeout)   API_SemaphorePostBPend(mutex, sem->sem_notify, timeout)
#define SEM_NOTIFY(sem)                 API_SemaphoreBFlush(sem->sem_notify, NULL)

/*
 * init sem
 */
void ipc_sem_init (void)
{
}

/*
 * allocate a undo struct
 */
static ipc_sem_undo_t *semundo_alloc (int semid, int nsems, pid_t pid)
{
    ipc_sem_undo_t *semundo;
    size_t  size = sizeof(ipc_sem_undo_t) + (sizeof(short) * nsems);

    semundo = (ipc_sem_undo_t *)sys_malloc(size);
    if (semundo == NULL) {
        return  (NULL);
    }
    bzero(semundo, size);

    semundo->sem_id  = semid;
    semundo->sem_pid = pid;
    semundo->sem_adj = (short *)((char *)semundo + sizeof(ipc_sem_undo_t));

    _List_Line_Add_Tail(&semundo->list, &sem_undo_header);

    return  (semundo);
}

/*
 * free a undo struct
 */
static void semundo_free (ipc_sem_undo_t *free)
{
    if (free) {
        _List_Line_Del(&free->list, &sem_undo_header);
        sys_free(free);
    }
}

/*
 * free undo struct by id
 */
static void semundo_free_semid (int semid)
{
    PLW_LIST_LINE    tmp;
    ipc_sem_undo_t  *semundo;

    tmp = sem_undo_header;
    while (tmp) {
        semundo = _LIST_ENTRY(tmp, ipc_sem_undo_t, list);
        tmp = _list_line_get_next(tmp);
        if (semundo->sem_id == semid) {
            semundo_free(semundo);
        }
    }
}

/*
 * find a undo struct
 */
static ipc_sem_undo_t *semundo_find (int semid, pid_t pid)
{
    PLW_LIST_LINE    tmp;
    ipc_sem_undo_t  *semundo;

    for (tmp  = sem_undo_header;
         tmp != NULL;
         tmp  = _list_line_get_next(tmp)) {

        semundo = _LIST_ENTRY(tmp, ipc_sem_undo_t, list);
        if ((semundo->sem_id == semid) &&
            (semundo->sem_pid == pid)) {
            return  (semundo);
        }
    }

    return  (NULL);
}

/*
 * allocate a new sem
 */
static int do_newsem (key_t key, int nsems, int semflg)
{
    ipc_id_sem_t   *sem;
    size_t  size;
    int idnum;
    int err = 0;

    if (!nsems) {
        errno = EINVAL;
        return  (IPC_ERROR);
    }

    sem = (ipc_id_sem_t *)sys_malloc(sizeof(ipc_id_sem_t));
    if (sem == NULL) {
        errno = ENOMEM;
        return  (IPC_ERROR);
    }
    bzero(sem, sizeof(ipc_id_sem_t));

    size = sizeof(struct sem) * nsems;
    sem->semarry = (struct sem *)sys_malloc(size);
    if (sem->semarry == NULL) {
        err = 1;
        errno = ENOMEM;
        goto    __error;
    }
    bzero(sem->semarry, size);

    sem->sem_notify = API_SemaphoreBCreate("ipc_sem_notify", FALSE, LW_OPTION_OBJECT_GLOBAL, NULL);
    if (sem->sem_notify == LW_HANDLE_INVALID) {
        err = 2;
        errno = ENOSPC;
        goto    __error;
    }

    sem->ipc.key  = key;
    sem->ipc.type = IPC_TYPE_SEM;

    sem->desc.sem_perm.mode = semflg & 0666; /* no exec prem */
    sem->desc.sem_perm.uid  = geteuid();
    sem->desc.sem_perm.gid  = getegid();
    sem->desc.sem_perm.cuid = sem->desc.sem_perm.uid;
    sem->desc.sem_perm.cgid = sem->desc.sem_perm.gid;

    sem->desc.sem_nsems = nsems;
    sem->desc.sem_otime = 0;
    sem->desc.sem_ctime = time(NULL);

    idnum = ipc_insert_instance(&sem->ipc);
    if (idnum < 0) {
        err = 3;
        errno = ENOSPC;
        goto    __error;
    }

    return  (idnum);

__error:
    if (err > 2) {
        API_SemaphoreBDelete(&sem->sem_notify);
    }
    if (err > 1) {
        sys_free(sem->semarry);
    }
    if (err > 0) {
        sys_free(sem);
    }
    return  (IPC_ERROR);
}

/*
 * notify waiting sem
 */
static void do_semupdate (ipc_id_sem_t *sem)
{
    SEM_NOTIFY(sem);
}

/*
 * delete a sem
 */
static int do_deletesem (int idnum)
{
    ipc_id_sem_t    *sem;
    uid_t            uid_cur = geteuid();

    sem = (ipc_id_sem_t *)ipc_get_instance(idnum, IPC_TYPE_SEM);
    if (sem == NULL) {
        errno = EINVAL;
        return  (IPC_ERROR);
    }

    if ((uid_cur != 0) &&
        (uid_cur != sem->desc.sem_perm.uid) &&
        (uid_cur != sem->desc.sem_perm.cuid)) {
        errno = EACCES;
        return  (IPC_ERROR);
    }

    ipc_remove_instance(&sem->ipc);

    semundo_free_semid(idnum);

    API_SemaphoreBDelete(&sem->sem_notify);
    sys_free(sem->semarry);
    sys_free(sem);

    return  (IPC_OK);
}

/*
 * get a sem stat
 */
static int do_getsemstat (int idnum, struct semid_ds *buf)
{
    ipc_id_sem_t    *sem;

    sem = (ipc_id_sem_t *)ipc_get_instance(idnum, IPC_TYPE_SEM);
    if (sem == NULL) {
        errno = EINVAL;
        return  (IPC_ERROR);
    }

    *buf = sem->desc;

    return  (IPC_OK);
}

/*
 * set a sem stat
 */
static int do_setsemstat (int idnum, struct semid_ds *buf)
{
    ipc_id_sem_t    *sem;
    uid_t            uid_cur = geteuid();

    sem = (ipc_id_sem_t *)ipc_get_instance(idnum, IPC_TYPE_SEM);
    if (sem == NULL) {
        errno = EINVAL;
        return  (IPC_ERROR);
    }

    if ((uid_cur != 0) &&
        (uid_cur != sem->desc.sem_perm.uid) &&
        (uid_cur != sem->desc.sem_perm.cuid)) {
        errno = EACCES;
        return  (IPC_ERROR);
    }

    sem->desc.sem_perm.uid  = buf->sem_perm.uid;
    sem->desc.sem_perm.gid  = buf->sem_perm.gid;
    sem->desc.sem_perm.mode = buf->sem_perm.mode;

    sem->desc.sem_ctime = time(NULL);

    return  (IPC_OK);
}

/*
 * do_semgetval
 */
static int do_semgetval (int semid, int semnum)
{
    ipc_id_sem_t    *sem;

    sem = (ipc_id_sem_t *)ipc_get_instance(semid, IPC_TYPE_SEM);
    if (sem == NULL) {
        errno = EINVAL;
        return  (IPC_ERROR);
    }

    if ((semnum < 0) || (semnum >= sem->desc.sem_nsems)) {
        errno = EINVAL;
        return  (IPC_ERROR);
    }

    return  ((int)sem->semarry[semnum].semval);
}

/*
 * do_semgetpid
 */
static int do_semgetpid (int semid, int semnum)
{
    ipc_id_sem_t    *sem;

    sem = (ipc_id_sem_t *)ipc_get_instance(semid, IPC_TYPE_SEM);
    if (sem == NULL) {
        errno = EINVAL;
        return  (IPC_ERROR);
    }

    if ((semnum < 0) || (semnum >= sem->desc.sem_nsems)) {
        errno = EINVAL;
        return  (IPC_ERROR);
    }

    return  ((int)sem->semarry[semnum].sempid);
}

/*
 * do_semgetncnt
 */
static int do_semgetncnt (int semid, int semnum)
{
    ipc_id_sem_t    *sem;

    sem = (ipc_id_sem_t *)ipc_get_instance(semid, IPC_TYPE_SEM);
    if (sem == NULL) {
        errno = EINVAL;
        return  (IPC_ERROR);
    }

    if ((semnum < 0) || (semnum >= sem->desc.sem_nsems)) {
        errno = EINVAL;
        return  (IPC_ERROR);
    }

    return  ((int)sem->semarry[semnum].semncnt);
}

/*
 * do_semgetzcnt
 */
static int do_semgetzcnt (int semid, int semnum)
{
    ipc_id_sem_t    *sem;

    sem = (ipc_id_sem_t *)ipc_get_instance(semid, IPC_TYPE_SEM);
    if (sem == NULL) {
        errno = EINVAL;
        return  (IPC_ERROR);
    }

    if ((semnum < 0) || (semnum >= sem->desc.sem_nsems)) {
        errno = EINVAL;
        return  (IPC_ERROR);
    }

    return  ((int)sem->semarry[semnum].semzcnt);
}

/*
 * do_semgetall
 */
static int do_semgetall (int semid, unsigned short array[])
{
    ipc_id_sem_t    *sem;
    int i;

    sem = (ipc_id_sem_t *)ipc_get_instance(semid, IPC_TYPE_SEM);
    if (sem == NULL) {
        errno = EINVAL;
        return  (IPC_ERROR);
    }

    for (i = 0; i < sem->desc.sem_nsems; i++) {
        array[i] = sem->semarry[i].semval;
    }

    return  (IPC_OK);
}

/*
 * do_semsetall
 */
static int do_semsetval (int semid, int semnum, int val)
{
    ipc_id_sem_t    *sem;

    sem = (ipc_id_sem_t *)ipc_get_instance(semid, IPC_TYPE_SEM);
    if (sem == NULL) {
        errno = EINVAL;
        return  (IPC_ERROR);
    }

    if ((semnum < 0) || (semnum >= sem->desc.sem_nsems)) {
        errno = EINVAL;
        return  (IPC_ERROR);
    }

    if ((val < 0) || (val > IPC_MAX_VALUE)) {
        errno = ERANGE;
        return  (IPC_ERROR);
    }

    sem->semarry[semnum].semval = (unsigned short)val;
    sem->semarry[semnum].sempid = getpid();
    sem->desc.sem_ctime = time(NULL);

    do_semupdate(sem);

    return  (IPC_OK);
}
/*
 * do_semsetall
 */
static int do_semsetall (int semid, unsigned short array[])
{
    ipc_id_sem_t    *sem;
    pid_t pid = getpid();
    int i;

    sem = (ipc_id_sem_t *)ipc_get_instance(semid, IPC_TYPE_SEM);
    if (sem == NULL) {
        errno = EINVAL;
        return  (IPC_ERROR);
    }

    for (i = 0; i < sem->desc.sem_nsems; i++) {
        if ((array[i] < 0) || (array[i] > IPC_MAX_VALUE)) {
            errno = ERANGE;
            return  (IPC_ERROR);
        }
    }

    for (i = 0; i < sem->desc.sem_nsems; i++) {
        sem->semarry[i].semval = array[i];
        sem->semarry[i].sempid = pid;
    }

    sem->desc.sem_ctime = time(NULL);

    do_semupdate(sem);

    return  (IPC_OK);
}

/*
 * Determine whether a sequence of semaphore operations would succeed
 * all at once. Return 0 if yes, 1 if need to sleep, else return error code.
 */
static int do_try_semop (ipc_id_sem_t *sem, struct sembuf sops[],
                         size_t nsops, ipc_sem_undo_t *un, pid_t pid)
{
    int result, sem_op;
    struct sembuf *sop;
    struct sem *curr;

    for (sop = sops; sop < sops + nsops; sop++) {
        curr = &sem->semarry[sop->sem_num];
        sem_op = sop->sem_op;
        result = curr->semval;

        if (!sem_op && result) {
            goto would_block;
        }

        result += sem_op;
        if (result < 0) {
            goto would_block;
        }

        if (result > IPC_MAX_VALUE) {
            goto out_of_range;
        }

        if (sop->sem_flg & SEM_UNDO) {
            int undo = un->sem_adj[sop->sem_num] - sem_op;
            if (undo < (-IPC_MAX_VALUE - 1) || undo > IPC_MAX_VALUE) {
                goto out_of_range;
            }
        }
        curr->semval = result;
    }

    sop--;
    while (sop >= sops) {
        sem->semarry[sop->sem_num].sempid = pid;
        if (sop->sem_flg & SEM_UNDO) {
            un->sem_adj[sop->sem_num] -= sop->sem_op;
        }
        sop--;
    }

    sem->desc.sem_otime = time(NULL);

    return  (0);

out_of_range:
    result = -ERANGE;
    goto undo;

would_block:
    if (sop->sem_flg & IPC_NOWAIT) {
        result = -EAGAIN;
    } else {
        result = 1;
    }

undo:
    sop--;
    while (sop >= sops) {
        sem->semarry[sop->sem_num].semval -= sop->sem_op;
        sop--;
    }

    return  (result);
}

/*
 * semaphore operate
 */
static int do_semop (int semid, struct sembuf *tsops, size_t nsops, u_long timeout)
{
    ipc_id_sem_t    *sem;
    pid_t pid = getpid();
    int result;
    int first_get = 1;
    ipc_sem_undo_t *semundo = NULL;
    u_long to;
    u_long org_time;

    do {
        sem = (ipc_id_sem_t *)ipc_get_instance(semid, IPC_TYPE_SEM);
        if (sem == NULL) {
            if (first_get) {
                errno = EINVAL;
            } else {
                errno = EIDRM;
            }
            return  (IPC_ERROR);
        }

        if (semundo == NULL) {
            semundo = semundo_find(semid, pid);
            if (semundo == NULL) {
                semundo = semundo_alloc(semid, sem->desc.sem_nsems, pid);
                if (semundo == NULL) {
                    errno = ENOMEM;
                    return  (IPC_ERROR);
                }
            }
        }

        result = do_try_semop(sem, tsops, nsops, semundo, pid);
        if (result == 0) {
            break;
        } else if (result < 0) {
            errno = -result;
            return  (IPC_ERROR);
        }

        first_get = 0;

        __KERNEL_TIME_GET(org_time, u_long); /* get kernel time now */

        to = SEM_WAIT(ipc_lock_mutex, sem, timeout);

        ipc_lock();

        if (to == ERROR_THREAD_WAIT_TIMEOUT) {
            errno = EAGAIN;
            return  (IPC_ERROR);
        }

        timeout = _sigTimeoutRecalc(org_time, timeout); /* recalculate timeout */
    } while (1);

    SEM_NOTIFY(sem);

    return  (IPC_OK);
}

/*
 * ipc semaphore get
 */
LW_SYMBOL_EXPORT
int semget (key_t key, int nsems, int semflg)
{
    ipc_id_sem_t   *sem;
    int semid = IPC_ERROR;

    if ((nsems < 0) || (nsems > IPC_MAX_NSEM)) {
        errno = EINVAL;
        return  (semid);
    }

    ipc_lock();

    if (key == IPC_PRIVATE) {
        semid = do_newsem(key, nsems, semflg);
    } else {
        sem = (ipc_id_sem_t *)ipc_find_instance(key, IPC_TYPE_SEM);
        if (sem) {
            if ((semflg & IPC_CREAT) && (semflg & IPC_EXCL)) {
                errno = EEXIST;
            } else {
                if (nsems > sem->desc.sem_nsems) {
                    errno = EINVAL;
                } else if (ipc_perms(&sem->desc.sem_perm, semflg) < 0) {
                    errno = EACCES;
                } else {
                    semid = sem->ipc.idnum;
                }
            }
        } else {
            if (semflg & IPC_CREAT) {
                semid = do_newsem(key, nsems, semflg);
            } else {
                errno = ENOENT;
            }
        }
    }

    ipc_unlock();

    return  (semid);
}

/*
 * ipc semaphore control
 */
LW_SYMBOL_EXPORT
int semctl (int semid, int semnum, int cmd, ...)
{
    int ret = IPC_ERROR;
    union semun arg;
    va_list varlist;

    va_start(varlist, cmd);

    switch (cmd) {

    case IPC_STAT:
        arg = va_arg(varlist, union semun);
        if (!arg.buf) {
            errno = EINVAL;
        } else {
            ipc_lock();
            ret = do_getsemstat(semid, arg.buf);
            ipc_unlock();
        }
        break;

    case IPC_SET:
        arg = va_arg(varlist, union semun);
        if (!arg.buf) {
            errno = EINVAL;
        } else {
            ipc_lock();
            ret = do_setsemstat(semid, arg.buf);
            ipc_unlock();
        }
        break;

    case IPC_RMID:
        ipc_lock();
        ret = do_deletesem(semid);
        ipc_unlock();
        break;

    case GETVAL:
        ipc_lock();
        ret = do_semgetval(semid, semnum);
        ipc_unlock();
        break;

    case SETVAL:
        arg = va_arg(varlist, union semun);
        ipc_lock();
        ret = do_semsetval(semid, semnum, arg.val);
        ipc_unlock();
        break;

    case GETPID:
        ipc_lock();
        ret = do_semgetpid(semid, semnum);
        ipc_unlock();
        break;

    case GETNCNT:
        ipc_lock();
        ret = do_semgetncnt(semid, semnum);
        ipc_unlock();
        break;

    case GETZCNT:
        ipc_lock();
        ret = do_semgetzcnt(semid, semnum);
        ipc_unlock();
        break;

    case GETALL:
        arg = va_arg(varlist, union semun);
        if (!arg.array) {
            errno = EINVAL;
        } else {
            ipc_lock();
            ret = do_semgetall(semid, arg.array);
            ipc_unlock();
        }
        break;

    case SETALL:
        arg = va_arg(varlist, union semun);
        if (!arg.array) {
            errno = EINVAL;
        } else {
            ipc_lock();
            ret = do_semsetall(semid, arg.array);
            ipc_unlock();
        }
        break;

    default:
        errno = ENOSYS;
        break;
    }

    va_end(varlist);

    return  (ret);
}

/*
 * ipc semaphore operate
 */
LW_SYMBOL_EXPORT
int semop (int semid, struct sembuf *tsops, size_t nsops)
{
    return  (semtimedop(semid, tsops, nsops, NULL));
}

/*
 * ipc semaphore operate with timeout
 */
LW_SYMBOL_EXPORT
int semtimedop (int semid, struct sembuf *tsops, size_t nsops, const struct timespec *timeout)
{
    u_long  timeout_tick = LW_OPTION_WAIT_INFINITE;
    int ret;

    if ((tsops == NULL) || (nsops > IPC_MAX_NSEM)) {
        errno = EINVAL;
        return  (IPC_ERROR);
    }

    if (timeout) {
        timeout_tick = __timespecToTick(timeout);
    }

    ipc_lock();
    ret = do_semop(semid, tsops, nsops, timeout_tick);
    ipc_unlock();

    return  (ret);
}

/*
 * hook
 */
void hook_sem_pid_remove (pid_t pid)
{
    ipc_id_sem_t   *sem;
    PLW_LIST_LINE   tmp;
    ipc_sem_undo_t  *semundo;

    ipc_lock();

    tmp = sem_undo_header;
    while (tmp) {
        semundo = _LIST_ENTRY(tmp, ipc_sem_undo_t, list);
        tmp = _list_line_get_next(tmp);

        if (semundo->sem_pid == pid) {
            sem = (ipc_id_sem_t *)ipc_get_instance(semundo->sem_id, IPC_TYPE_SEM);
            if (sem) {
                int i;
                for (i = 0; i < sem->desc.sem_nsems; i++) {
                    if (semundo->sem_adj[i]) {
                        struct sem *curr = &sem->semarry[i];
                        curr->semval += semundo->sem_adj[i];
                        /*
                         * Range checks of the new semaphore value,
                         * not defined by sus:
                         * - Some unices ignore the undo entirely
                         *   (e.g. HP UX 11i 11.22, Tru64 V5.1)
                         * - some cap the value (e.g. FreeBSD caps
                         *   at 0, but doesn't enforce SEMVMX)
                         *
                         * Linux caps the semaphore value, both at 0
                         * and at IPC_MAX_VALUE.
                         *
                         *  Manfred <manfred@colorfullife.com>
                         */
                        if (curr->semval < 0) {
                            curr->semval = 0;
                        }
                        if (curr->semval > IPC_MAX_VALUE) {
                            curr->semval = IPC_MAX_VALUE;
                        }
                        curr->sempid = pid;
                    }
                }
                sem->desc.sem_otime = time(NULL);
                do_semupdate(sem);
            }
            semundo_free(semundo);
        }
    }

    ipc_unlock();
}

/*
 * end
 */
