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

#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

#include "xsiipc.h"

/*
 *  proc file read functions
 */
static ssize_t  ipc_proc_ver_read(PLW_PROCFS_NODE  p_pfsn,
                                  PCHAR            pcBuffer,
                                  size_t           stMaxBytes,
                                  off_t            oft);
static ssize_t  ipc_proc_sem_read(PLW_PROCFS_NODE  p_pfsn,
                                  PCHAR            pcBuffer,
                                  size_t           stMaxBytes,
                                  off_t            oft);
static ssize_t  ipc_proc_msg_read(PLW_PROCFS_NODE  p_pfsn,
                                  PCHAR            pcBuffer,
                                  size_t           stMaxBytes,
                                  off_t            oft);
static ssize_t  ipc_proc_shm_read(PLW_PROCFS_NODE  p_pfsn,
                                  PCHAR            pcBuffer,
                                  size_t           stMaxBytes,
                                  off_t            oft);

/*
 *  proc file function set
 */
static LW_PROCFS_NODE_OP    ipc_proc_ver_funcs = {
        ipc_proc_ver_read, NULL
};
static LW_PROCFS_NODE_OP    ipc_proc_sem_funcs = {
        ipc_proc_sem_read, NULL
};
static LW_PROCFS_NODE_OP    ipc_proc_msg_funcs = {
        ipc_proc_msg_read, NULL
};
static LW_PROCFS_NODE_OP    ipc_proc_shm_funcs = {
        ipc_proc_shm_read, NULL
};

/*
 *  static proc file node
 */
static LW_PROCFS_NODE   ipc_proc[] = {
        LW_PROCFS_INIT_NODE("sysvipc",
                            (S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH),
                            NULL, NULL, 0),
        LW_PROCFS_INIT_NODE("ver",
                            (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH),
                            &ipc_proc_ver_funcs, NULL, 32),
        LW_PROCFS_INIT_NODE("sem",
                            (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH),
                            &ipc_proc_sem_funcs, NULL, 0),
        LW_PROCFS_INIT_NODE("msg",
                            (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH),
                            &ipc_proc_msg_funcs, NULL, 0),
        LW_PROCFS_INIT_NODE("shm",
                            (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH),
                            &ipc_proc_shm_funcs, NULL, 0),
};

/*
 *  sysvipc/ver file read
 */
static ssize_t  ipc_proc_ver_read (PLW_PROCFS_NODE  p_pfsn,
                                   PCHAR            pcBuffer,
                                   size_t           stMaxBytes,
                                   off_t            oft)
{
    PCHAR        pcFileBuffer;
    size_t       stRealSize;
    size_t       stCopeBytes;

    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (0);
    }

    stRealSize = API_ProcFsNodeGetRealFileSize(p_pfsn);
    if (stRealSize == 0) {
        stRealSize = bnprintf(pcFileBuffer, 32, 0,
                              "%s", IPC_VER);
        API_ProcFsNodeSetRealFileSize(p_pfsn, stRealSize);
    }

    if (oft >= stRealSize) {
        _ErrorHandle(ENOSPC);
        return  (0);
    }

    stCopeBytes  = __MIN(stMaxBytes, (size_t)(stRealSize - oft));
    lib_memcpy(pcBuffer, (CPVOID)(pcFileBuffer + oft), (UINT)stCopeBytes);

    return  ((ssize_t)stCopeBytes);
}

/*
 *  sysvipc/sem file read
 */
static ssize_t  ipc_proc_sem_read (PLW_PROCFS_NODE  p_pfsn,
                                   PCHAR            pcBuffer,
                                   size_t           stMaxBytes,
                                   off_t            oft)
{
    static const CHAR     hdr[] = \
            "             key semid perms nsems    uid    gid   cuid   cgid        otime        ctime\n";

    PCHAR        pcFileBuffer;
    size_t       stRealSize;
    size_t       stCopeBytes;

    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) { /* no buffer */
        size_t          stNeedBufferSize = sizeof(hdr);
        ipc_id_sem_t   *sem;
        int i, cnt = 0;

        ipc_lock();
        cnt = ipc_cnt[IPC_TYPE_SEM];
        stNeedBufferSize += (cnt * 128);
        ipc_unlock();

        if (API_ProcFsAllocNodeBuffer(p_pfsn, stNeedBufferSize)) {
            errno = ENOMEM;
            return  (0);
        }

        pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);

        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, 0, hdr);

        ipc_lock();
        for (i = 0; (i < IPC_MAX) && (cnt > 0); i++) {
            ipc_id_t *ipc = ipc_handle_table[i];
            if (ipc && (ipc->type == IPC_TYPE_SEM)) {
                sem = (ipc_id_sem_t *)ipc;
                stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize,
                                      "%16llx %5d %5o %5d %6d %6d %6d %6d %12lld %12lld\n",
                                      sem->ipc.key,
                                      sem->ipc.idnum,
                                      sem->desc.sem_perm.mode,
                                      sem->desc.sem_nsems,
                                      sem->desc.sem_perm.uid,
                                      sem->desc.sem_perm.gid,
                                      sem->desc.sem_perm.cuid,
                                      sem->desc.sem_perm.cgid,
                                      sem->desc.sem_otime,
                                      sem->desc.sem_ctime);
                cnt--;
            }
        }
        ipc_unlock();

        API_ProcFsNodeSetRealFileSize(p_pfsn, stRealSize);
    } else {
        stRealSize = API_ProcFsNodeGetRealFileSize(p_pfsn);
    }

    if (oft >= stRealSize) {
        _ErrorHandle(ENOSPC);
        return  (0);
    }

    stCopeBytes  = min(stMaxBytes, (size_t)(stRealSize - oft));
    memcpy(pcBuffer, (CPVOID)(pcFileBuffer + oft), (UINT)stCopeBytes);

    return  ((ssize_t)stCopeBytes);
}

/*
 *  sysvipc/msg file read
 */
static ssize_t  ipc_proc_msg_read (PLW_PROCFS_NODE  p_pfsn,
                                   PCHAR            pcBuffer,
                                   size_t           stMaxBytes,
                                   off_t            oft)
{
    static const CHAR     hdr[] = \
            "             key msqid perms   qnum  lspid  lrpid    uid    gid   cuid   cgid"
            "        stime        rtime        ctime\n";

    PCHAR        pcFileBuffer;
    size_t       stRealSize;
    size_t       stCopeBytes;

    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) { /* no buffer */
        size_t          stNeedBufferSize = sizeof(hdr);
        ipc_id_msg_t   *msg;
        int i, cnt = 0;

        ipc_lock();
        cnt = ipc_cnt[IPC_TYPE_MSG];
        stNeedBufferSize += (cnt * 256);
        ipc_unlock();

        if (API_ProcFsAllocNodeBuffer(p_pfsn, stNeedBufferSize)) {
            errno = ENOMEM;
            return  (0);
        }

        pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);

        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, 0, hdr);

        ipc_lock();
        for (i = 0; (i < IPC_MAX) && (cnt > 0); i++) {
            ipc_id_t *ipc = ipc_handle_table[i];
            if (ipc && (ipc->type == IPC_TYPE_MSG)) {
                msg = (ipc_id_msg_t *)ipc;
                stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize,
                                      "%16llx %5d %5o %6ld %6d %6d %6d %6d %6d %6d "
                                      "%12lld %12lld %12lld\n",
                                      msg->ipc.key,
                                      msg->ipc.idnum,
                                      msg->desc.msg_perm.mode,
                                      msg->desc.msg_qnum,
                                      msg->desc.msg_lspid,
                                      msg->desc.msg_lrpid,
                                      msg->desc.msg_perm.uid,
                                      msg->desc.msg_perm.gid,
                                      msg->desc.msg_perm.cuid,
                                      msg->desc.msg_perm.cgid,
                                      msg->desc.msg_stime,
                                      msg->desc.msg_rtime,
                                      msg->desc.msg_ctime);
                cnt--;
            }
        }
        ipc_unlock();

        API_ProcFsNodeSetRealFileSize(p_pfsn, stRealSize);
    } else {
        stRealSize = API_ProcFsNodeGetRealFileSize(p_pfsn);
    }

    if (oft >= stRealSize) {
        _ErrorHandle(ENOSPC);
        return  (0);
    }

    stCopeBytes  = min(stMaxBytes, (size_t)(stRealSize - oft));
    memcpy(pcBuffer, (CPVOID)(pcFileBuffer + oft), (UINT)stCopeBytes);

    return  ((ssize_t)stCopeBytes);
}

/*
 *  sysvipc/shm file read
 */
static ssize_t  ipc_proc_shm_read (PLW_PROCFS_NODE  p_pfsn,
                                   PCHAR            pcBuffer,
                                   size_t           stMaxBytes,
                                   off_t            oft)
{
    static const CHAR     hdr[] = \
            "             key shmid perms       size nattch   cpid   lpid    uid    gid   cuid   cgid"
            "        atime        dtime        ctime\n";

    PCHAR        pcFileBuffer;
    size_t       stRealSize;
    size_t       stCopeBytes;

    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) { /* no buffer */
        size_t          stNeedBufferSize = sizeof(hdr);
        ipc_id_shm_t   *shm;
        int i, cnt = 0;

        ipc_lock();
        cnt = ipc_cnt[IPC_TYPE_SHM];
        stNeedBufferSize += (cnt * 256);
        ipc_unlock();

        if (API_ProcFsAllocNodeBuffer(p_pfsn, stNeedBufferSize)) {
            errno = ENOMEM;
            return  (0);
        }

        pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);

        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, 0, hdr);

        ipc_lock();
        for (i = 0; (i < IPC_MAX) && (cnt > 0); i++) {
            ipc_id_t *ipc = ipc_handle_table[i];
            if (ipc && (ipc->type == IPC_TYPE_SHM)) {
                shm = (ipc_id_shm_t *)ipc;
                stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize,
                                      "%16llx %5d %5o %10zu %6d %6d %6d %6d %6d %6d %6d "
                                      "%12lld %12lld %12lld\n",
                                      shm->ipc.key,
                                      shm->ipc.idnum,
                                      shm->desc.shm_perm.mode,
                                      shm->desc.shm_segsz,
                                      shm->desc.shm_nattch,
                                      shm->desc.shm_cpid,
                                      shm->desc.shm_lpid,
                                      shm->desc.shm_perm.uid,
                                      shm->desc.shm_perm.gid,
                                      shm->desc.shm_perm.cuid,
                                      shm->desc.shm_perm.cgid,
                                      shm->desc.shm_atime,
                                      shm->desc.shm_dtime,
                                      shm->desc.shm_ctime);
                cnt--;
            }
        }
        ipc_unlock();

        API_ProcFsNodeSetRealFileSize(p_pfsn, stRealSize);
    } else {
        stRealSize = API_ProcFsNodeGetRealFileSize(p_pfsn);
    }

    if (oft >= stRealSize) {
        _ErrorHandle(ENOSPC);
        return  (0);
    }

    stCopeBytes  = min(stMaxBytes, (size_t)(stRealSize - oft));
    memcpy(pcBuffer, (CPVOID)(pcFileBuffer + oft), (UINT)stCopeBytes);

    return  ((ssize_t)stCopeBytes);
}

/*
 *  proc file init
 */
void ipc_proc_init (void)
{
    API_ProcFsMakeNode(&ipc_proc[0], "/");
    API_ProcFsMakeNode(&ipc_proc[1], "/sysvipc");
    API_ProcFsMakeNode(&ipc_proc[2], "/sysvipc");
    API_ProcFsMakeNode(&ipc_proc[3], "/sysvipc");
    API_ProcFsMakeNode(&ipc_proc[4], "/sysvipc");
}

/*
 *  proc file deinit
 */
void ipc_proc_deinit (void)
{
    if (API_ProcFsRemoveNode(&ipc_proc[4], NULL)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Serious error: /proc/sysvipc/shm file busy now, "
                                           "the system will become unstable!\r\n");
    }

    if (API_ProcFsRemoveNode(&ipc_proc[3], NULL)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Serious error: /proc/sysvipc/msg file busy now, "
                                           "the system will become unstable!\r\n");
    }

    if (API_ProcFsRemoveNode(&ipc_proc[2], NULL)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Serious error: /proc/sysvipc/sem file busy now, "
                                           "the system will become unstable!\r\n");
    }

    if (API_ProcFsRemoveNode(&ipc_proc[1], NULL)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Serious error: /proc/sysvipc/ver file busy now, "
                                           "the system will become unstable!\r\n");
    }

    if (API_ProcFsRemoveNode(&ipc_proc[0], NULL)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Serious error: /proc/sysvipc directory busy now, "
                                           "the system will become unstable!\r\n");
    }
}

/*
 * end
 */
