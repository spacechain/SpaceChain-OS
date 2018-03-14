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
#include <stddef.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <errno.h>
#include <string.h>

#include "xsiipc.h"

/*
 * msg node
 */
typedef struct {
    LW_LIST_RING    list;
    long            type;
    u_long          len;
    char            msg[IPC_MAX_MSG];
} ipc_msg_node_t;

#define MSG_RWAIT(mutex, msq)   API_SemaphorePostBPend(mutex, msq->msg_rlock, LW_OPTION_WAIT_INFINITE)
#define MSG_RFLUSH(msq)         API_SemaphoreBFlush(msq->msg_rlock, NULL)

#define MSG_WWAIT(mutex, msg)   API_SemaphorePostBPend(mutex, msq->msg_wlock, LW_OPTION_WAIT_INFINITE)
#define MSG_WPOST(msg)          API_SemaphoreBPost(msq->msg_wlock)

/*
 * init msg
 */
void ipc_msg_init (void)
{
}

/*
 * allocate a msg
 */
static int do_newmsg (key_t key, int msgflg)
{
    ipc_id_msg_t    *msq;
    ipc_msg_node_t  *msg_node;
    int idnum;
    int err = 0;
    int i;

    msq = (ipc_id_msg_t *)sys_malloc(sizeof(ipc_id_msg_t));
    if (msq == NULL) {
        errno = ENOMEM;
        return  (IPC_ERROR);
    }
    bzero(msq, sizeof(ipc_id_msg_t));

    msq->msg_buf = (void *)sys_malloc(sizeof(ipc_msg_node_t) * IPC_MAX_MSGS);
    if (msq->msg_buf == NULL) {
        err = 1;
        errno = ENOMEM;
        goto    __error;
    }

    msq->msg_wlock = API_SemaphoreBCreate("ipc_msg_wlock", TRUE, LW_OPTION_OBJECT_GLOBAL, NULL);
    if (msq->msg_wlock == LW_HANDLE_INVALID) {
        err = 2;
        errno = ENOSPC;
        goto    __error;

    }

    msq->msg_rlock = API_SemaphoreBCreate("ipc_msg_rlock", FALSE, LW_OPTION_OBJECT_GLOBAL, NULL);
    if (msq->msg_rlock == LW_HANDLE_INVALID) {
        err = 3;
        errno = ENOSPC;
        goto    __error;
    }

    msg_node = (ipc_msg_node_t *)msq->msg_buf;
    for (i = 0; i < IPC_MAX_MSGS; i++) {
        _List_Ring_Add_Ahead(&msg_node->list, &msq->msg_free);
        msg_node++;
    }

    msq->ipc.key  = key;
    msq->ipc.type = IPC_TYPE_MSG;

    msq->desc.msg_perm.mode = msgflg & 0666; /* no exec prem */
    msq->desc.msg_perm.uid  = geteuid();
    msq->desc.msg_perm.gid  = getegid();
    msq->desc.msg_perm.cuid = msq->desc.msg_perm.uid;
    msq->desc.msg_perm.cgid = msq->desc.msg_perm.gid;

    msq->desc.msg_qnum   = 0;
    msq->desc.msg_qbytes = IPC_MAX_MSG;
    msq->desc.msg_lspid  = 0;
    msq->desc.msg_lrpid  = 0;
    msq->desc.msg_stime  = 0;
    msq->desc.msg_rtime  = 0;
    msq->desc.msg_ctime  = time(NULL);

    idnum = ipc_insert_instance(&msq->ipc);
    if (idnum < 0) {
        err = 4;
        errno = ENOSPC;
        goto    __error;
    }

    return  (idnum);

__error:
    if (err > 3) {
        API_SemaphoreBDelete(&msq->msg_rlock);
    }
    if (err > 2) {
        API_SemaphoreBDelete(&msq->msg_wlock);
    }
    if (err > 1) {
        sys_free(msq->msg_buf);
    }
    if (err > 0) {
        sys_free(msq);
    }
    return  (IPC_ERROR);
}

/*
 * delete a msg
 */
static int do_deletemsg (int idnum)
{
    ipc_id_msg_t    *msq;
    uid_t            uid_cur = geteuid();

    msq = (ipc_id_msg_t *)ipc_get_instance(idnum, IPC_TYPE_MSG);
    if (msq == NULL) {
        errno = EINVAL;
        return  (IPC_ERROR);
    }

    if ((uid_cur != 0) &&
        (uid_cur != msq->desc.msg_perm.uid) &&
        (uid_cur != msq->desc.msg_perm.cuid)) {
        errno = EACCES;
        return  (IPC_ERROR);
    }

    ipc_remove_instance(&msq->ipc);

    API_SemaphoreBDelete(&msq->msg_rlock);
    API_SemaphoreBDelete(&msq->msg_wlock);
    sys_free(msq->msg_buf);
    sys_free(msq);

    return  (IPC_OK);
}

/*
 * get a msg stat
 */
static int do_getmsgstat (int idnum, struct msqid_ds *buf)
{
    ipc_id_msg_t    *msq;

    msq = (ipc_id_msg_t *)ipc_get_instance(idnum, IPC_TYPE_MSG);
    if (msq == NULL) {
        errno = EINVAL;
        return  (IPC_ERROR);
    }

    *buf = msq->desc;

    return  (IPC_OK);
}

/*
 * set a msg stat
 */
static int do_setmsgstat (int idnum, struct msqid_ds *buf)
{
    ipc_id_msg_t    *msq;
    uid_t            uid_cur = geteuid();

    msq = (ipc_id_msg_t *)ipc_get_instance(idnum, IPC_TYPE_MSG);
    if (msq == NULL) {
        errno = EINVAL;
        return  (IPC_ERROR);
    }

    if ((uid_cur != 0) &&
        (uid_cur != msq->desc.msg_perm.uid) &&
        (uid_cur != msq->desc.msg_perm.cuid)) {
        errno = EACCES;
        return  (IPC_ERROR);
    }

    msq->desc.msg_perm.uid  = buf->msg_perm.uid;
    msq->desc.msg_perm.gid  = buf->msg_perm.gid;
    msq->desc.msg_perm.mode = buf->msg_perm.mode;

    msq->desc.msg_ctime = time(NULL);

    return  (IPC_OK);
}

/*
 * recv a any type message (in ipc lock mode)
 */
static BOOL do_msgrecv_canrecv (ipc_id_msg_t *msq, long msgtyp)
{
    PLW_LIST_RING   tmp;
    ipc_msg_node_t  *msg_node;

    if (msq->desc.msg_qnum) {
        if (msgtyp == 0) {
            return  (LW_TRUE);
        } else {
            tmp = msq->msg_header; /* 'tmp' must do not a NULL pointer */
            do {
                msg_node = _LIST_ENTRY(tmp, ipc_msg_node_t, list);
                if (msgtyp > 0) {
                    if (msg_node->type == msgtyp) {
                        return  (LW_TRUE);
                    }
                } else {
                    if (msg_node->type <= (0 - msgtyp)) {
                        return  (LW_TRUE);
                    }
                }
                tmp = _list_ring_get_next(tmp);
            } while (tmp != msq->msg_header);
        }
    }

    return  (FALSE);
}

/*
 * recv a message (in ipc lock mode)
 */
static ssize_t do_msgrecv (int msqid, struct mymsg *msgp, size_t msgsz, long msgtyp, int msgflg)
{
    PLW_LIST_RING   tmp;
    ipc_id_msg_t    *msq;
    ipc_msg_node_t  *msg_node = NULL;
    u_long           real_len;
    int              first_get = 1;

    do {
        msq = (ipc_id_msg_t *)ipc_get_instance(msqid, IPC_TYPE_MSG);
        if (msq == NULL) {
            if (first_get) {
                errno = EINVAL;
            } else {
                errno = EIDRM;
            }
            return  (IPC_ERROR);
        }

        if (do_msgrecv_canrecv(msq, msgtyp)) {
            break;
        }

        if (msgflg & IPC_NOWAIT) {
            errno = ENOMSG;
            return  (IPC_ERROR);
        }

        first_get = 0;

        MSG_RWAIT(ipc_lock_mutex, msq); /* release ipc mutex and wait recv 'atomic' */

        ipc_lock();
    } while (1);

    tmp = msq->msg_header; /* 'tmp' must do not a NULL pointer */

    if (msgtyp == 0) {
        msg_node = _LIST_ENTRY(tmp, ipc_msg_node_t, list);

    } else {
        do {
            msg_node = _LIST_ENTRY(tmp, ipc_msg_node_t, list);
            if (msgtyp > 0) {
                if (msg_node->type == msgtyp) {
                    break;
                }
            } else {
                if (msg_node->type <= (0 - msgtyp)) {
                    break;
                }
            }
            tmp = _list_ring_get_next(tmp);
        } while (tmp != msq->msg_header);
    }

    if (msg_node == NULL) {
        errno = ENOMEM; /* must NOT run to here */
        return  (IPC_ERROR);
    }

    if ((msg_node->len > msgsz) && !(msgflg & MSG_NOERROR)) {
        errno = E2BIG;
        return  (IPC_ERROR);
    }

    real_len = (msg_node->len > msgsz) ? (msgsz) : (msg_node->len);

    msgp->mtype = msg_node->type;
    memcpy(msgp->mtext, msg_node->msg, real_len);

    _List_Ring_Del(&msg_node->list, &msq->msg_header);
    _List_Ring_Add_Ahead(&msg_node->list, &msq->msg_free);

    msq->desc.msg_qnum--;
    msq->desc.msg_lrpid = getpid();
    msq->desc.msg_rtime = time(NULL);

    MSG_WPOST(msq); /* notify can write to queue */

    return  ((ssize_t)real_len);
}

/*
 * send a message (in ipc lock mode)
 */
static ssize_t do_msgsend (int msqid, struct mymsg *msgp, size_t msgsz, int msgflg)
{
    PLW_LIST_RING   tmp;
    ipc_id_msg_t    *msq;
    ipc_msg_node_t  *msg_node = NULL;
    int              first_get = 1;

    do {
        msq = (ipc_id_msg_t *)ipc_get_instance(msqid, IPC_TYPE_MSG);
        if (msq == NULL) {
            if (first_get) {
                errno = EINVAL;
            } else {
                errno = EIDRM;
            }
            return  (IPC_ERROR);
        }

        if (msq->msg_free) {
            break;
        }

        if (msgflg & IPC_NOWAIT) {
            errno = EAGAIN;
            return  (IPC_ERROR);
        }

        first_get = 0;

        MSG_WWAIT(ipc_lock_mutex, msq); /* release ipc mutex and wait recv 'atomic' */

        ipc_lock();
    } while (1);

    tmp = msq->msg_free; /* 'tmp' must do not a NULL pointer */

    msg_node = _LIST_ENTRY(tmp, ipc_msg_node_t, list);

    msg_node->len = msgsz;
    msg_node->type = msgp->mtype;
    memcpy(msg_node->msg, msgp->mtext, msgsz);

    _List_Ring_Del(&msg_node->list, &msq->msg_free);
    _List_Ring_Add_Last(&msg_node->list, &msq->msg_header);

    msq->desc.msg_qnum++;
    msq->desc.msg_lspid = getpid();
    msq->desc.msg_stime = time(NULL);

    MSG_RFLUSH(msq); /* notify ALL recv thread/process */

    if (msq->desc.msg_qnum < IPC_MAX_MSGS) {
        MSG_WPOST(msq); /* notify can write to queue */
    }

    return  (IPC_OK);
}

/*
 * ipc message get
 */
LW_SYMBOL_EXPORT
int msgget (key_t key, int msgflg)
{
    ipc_id_msg_t    *msq;
    int msqid = IPC_ERROR;

    ipc_lock();

    if (key == IPC_PRIVATE) {
        msqid = do_newmsg(key, msgflg);
    } else {
        msq = (ipc_id_msg_t *)ipc_find_instance(key, IPC_TYPE_MSG);
        if (msq) {
            if ((msgflg & IPC_CREAT) && (msgflg & IPC_EXCL)) {
                errno = EEXIST;
            } else {
                if (ipc_perms(&msq->desc.msg_perm, msgflg) < 0) {
                    errno = EACCES;
                } else {
                    msqid = msq->ipc.idnum;
                }
            }
        } else {
            if (msgflg & IPC_CREAT) {
                msqid = do_newmsg(key, msgflg);
            } else {
                errno = ENOENT;
            }
        }
    }

    ipc_unlock();

    return  (msqid);
}

/*
 * ipc message control
 */
LW_SYMBOL_EXPORT
int msgctl (int msqid, int cmd, struct msqid_ds *buf)
{
    int ret = IPC_ERROR;

    switch (cmd) {

    case IPC_STAT:
        if (!buf) {
            errno = EINVAL;
        } else {
            ipc_lock();
            ret = do_getmsgstat(msqid, buf);
            ipc_unlock();
        }
        break;

    case IPC_SET:
        if (!buf) {
            errno = EINVAL;
        } else {
            ipc_lock();
            ret = do_setmsgstat(msqid, buf);
            ipc_unlock();
        }
        break;

    case IPC_RMID:
        ipc_lock();
        ret = do_deletemsg(msqid);
        ipc_unlock();
        break;

    default:
        errno = ENOSYS;
        break;
    }

    return  (ret);
}

/*
 * ipc message recv
 */
LW_SYMBOL_EXPORT
ssize_t msgrcv (int msqid, void *pvmsgp, size_t msgsz, long msgtyp, int msgflg)
{
    struct mymsg *msgp = (struct mymsg *)pvmsgp;
    ssize_t ret;

    if (msgp == NULL) {
        errno = EINVAL;
        return  (IPC_ERROR);
    }

    __THREAD_CANCEL_POINT();

    ipc_lock();
    ret = do_msgrecv(msqid, msgp, msgsz, msgtyp, msgflg);
    ipc_unlock();

    return  (ret);
}

/*
 * ipc message send
 */
LW_SYMBOL_EXPORT
int msgsnd (int msqid, const void *pvmsgp, size_t msgsz, int msgflg)
{
    struct mymsg *msgp = (struct mymsg *)pvmsgp;
    int ret;

    if ((msgp == NULL) || (msgsz > IPC_MAX_MSG)) {
        errno = EINVAL;
        return  (IPC_ERROR);
    }

    __THREAD_CANCEL_POINT();

    ipc_lock();
    ret = do_msgsend(msqid, msgp, msgsz, msgflg);
    ipc_unlock();

    return  (ret);
}

/*
 * hook
 */
void hook_msg_pid_remove (pid_t pid)
{
}
/*
 * end
 */
