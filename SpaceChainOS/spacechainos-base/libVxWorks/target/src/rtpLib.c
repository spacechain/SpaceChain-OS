/**
 * @file
 * real time process library.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#define  __SYLIXOS_KERNEL
#include <unistd.h>
#include <sched.h>
#include <spawn.h>
#include <vproc.h>
#include <loader/include/loader_vppatch.h>
#include "rtpLib.h"
#include "taskLib.h"

/*
 * create a process
 */
RTP_ID  rtpSpawn (const char * rtpFileName, const char * argv[],
                  const char * envp[], int priority, int uStackSize,
                  int options, int taskOptions)
{
    posix_spawnattr_t attr;
    posix_spawnopt_t opt;
    struct sched_param par;
    RTP_ID process;
    int ret;

    par.sched_priority  = PX_PRIORITY_CONVERT(priority);
    opt.SPO_iSigNo      = 0;
    opt.SPO_ulId        = 0;
    opt.SPO_stStackSize = (size_t)uStackSize;

    if (options & RTP_CPU_AFFINITY_NONE) {
        opt.SPO_ulMainOption = LW_OPTION_THREAD_NO_AFFINITY;
    } else {
        opt.SPO_ulMainOption = 0;
    }

    posix_spawnattr_init(&attr);
    posix_spawnattr_setschedparam(&attr, &par);
    posix_spawnattr_setopt(&attr, &opt);

    ret = posix_spawnp(&process, rtpFileName, LW_NULL, &attr, (char **)argv, (char **)envp);

    posix_spawnattr_destroy(&attr);

    if (ret < 0) {
        return  (-1);
    } else {
        return  (process);
    }
}

/*
 * get process info
 */
STATUS   rtpInfoGet (RTP_ID rtpId, RTP_DESC *rtpStruct)
{
    ULONG   cnt;

    if (!rtpStruct) {
        errno = EINVAL;
        return  (ERROR);
    }

    if (vprocGetPath(rtpId, rtpStruct->pathName, MAX_FILENAME_LENGTH)) {
        errno = S_rtpLib_INVALID_RTP_ID;
        return  (ERROR);
    }

    rtpStruct->status     = RTP_STATE_NORMAL;
    rtpStruct->options    = RTP_LOCAL_SYMBOLS;
    rtpStruct->entrAddr   = LW_NULL;
    rtpStruct->initTaskId = HANDLE_TO_TID(vprocMainThread(rtpId));

    if (vprocThreadNum(rtpId, &cnt)) {
        errno = S_rtpLib_INVALID_RTP_ID;
        return  (ERROR);
    }

    rtpStruct->taskCnt  = (INT32)cnt;
    rtpStruct->parentId = vprocGetFather(rtpId);
    if (rtpStruct->parentId == 0) {
        rtpStruct->parentId = -1;
    }

    return  (OK);
}

/*
 * delete process
 */
STATUS  rtpDelete (RTP_ID id, int options, int status)
{
    if (options & RTP_DEL_FORCE) {
        vprocExitModeSet(id, LW_VPROC_EXIT_FORCE);

    } else {
        vprocExitModeSet(id, LW_VPROC_EXIT_NORMAL);
    }

    if (getpid() == id) {
        exit(status);

    } else {
        kill(id, SIGKILL);
    }

    return  (OK);
}

/*
 * kill a signal
 */
int  rtpKill (RTP_ID rtpId,  int signo)
{
    return  (kill(rtpId, signo));
}

/*
 * sigqueue a signal
 */
int  rtpSigqueue (RTP_ID rtpId,  int signo,
                  const union sigval value)
{
    return  (sigqueue(rtpId, signo, value));
}

/*
 * kill a signal to task
 */
int  rtpTaskKill(TASK_ID tid, int signo)
{
    TID_FIX(tid);

    return  (kill(TID_TO_HANDLE(tid), signo));
}

/*
 * sigqueue a signal to task
 */
int  rtpTaskSigqueue (TASK_ID tid, int signo,
                      const union sigval value)
{
    TID_FIX(tid);

    return  (sigqueue(TID_TO_HANDLE(tid), signo, value));
}

/*
 * end
 */
