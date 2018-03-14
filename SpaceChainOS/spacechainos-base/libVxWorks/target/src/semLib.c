/**
 * @file
 * semphore library.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#include <unistd.h>
#include <stdlib.h>
#include "semLib.h"
#include "taskLib.h"

/*
 * vxWorks option to SylixOS
 */
static ULONG  vx2sySemOpt (int options)
{
    ULONG  opt = 0;

    if (options & SEM_Q_PRIORITY) {
        opt |= LW_OPTION_WAIT_PRIORITY;
    }
    if (options & SEM_DELETE_SAFE) {
        opt |= LW_OPTION_DELETE_SAFE;
    }
    if (options & SEM_INTERRUPTIBLE) {
        opt |= LW_OPTION_SIGNAL_INTER;
    }

    return  (opt);
}

/*
 * SylixOS option to vxWorks
 */
static int  sy2vxSemOpt (ULONG  ulOpt)
{
    int opt = 0;

    if (ulOpt & LW_OPTION_WAIT_PRIORITY) {
        opt |= SEM_Q_PRIORITY;
    }
    if (ulOpt & LW_OPTION_DELETE_SAFE) {
        opt |= SEM_DELETE_SAFE;
    }
    if (ulOpt & LW_OPTION_SIGNAL_INTER) {
        opt |= SEM_INTERRUPTIBLE;
    }

    return  (opt);
}

/*
 * vxWorks timeout to SylixOS
 */
static ULONG  vx2sySemTo (int timeout)
{
    ULONG   ulTo;

    if (timeout == WAIT_FOREVER) {
        ulTo = LW_OPTION_WAIT_INFINITE;
    } else {
        ulTo = (ULONG)timeout;
    }

    return  (ulTo);
}

/*
 * sem show init
 */
void  semShowInit (void)
{
}

/*
 * create mutex
 */
SEM_ID  semMCreate (int options)
{
    ULONG  opt = vx2sySemOpt(options);

    opt |= LW_OPTION_INHERIT_PRIORITY;

    return  (API_SemaphoreMCreate("vxWorks Mutex", LW_PRIO_DEF_CEILING, opt, LW_NULL));
}

/*
 * create binary semaphore
 */
SEM_ID  semBCreate (int options, SEM_B_STATE initialState)
{
    ULONG  opt = vx2sySemOpt(options);

    return  (API_SemaphoreBCreate("vxWorks SemB", (BOOL)initialState, opt, LW_NULL));
}

/*
 * create count semaphore
 */
SEM_ID  semCCreate(int options, int initialCount)
{
    ULONG  opt = vx2sySemOpt(options);

    return  (API_SemaphoreCCreate("vxWorks SemC", (ULONG)initialCount, __ARCH_ULONG_MAX, opt, LW_NULL));
}

/*
 * create R/W semaphore
 */
SEM_ID  semRWCreate (int options, int maxReaders)
{
    ULONG  opt = vx2sySemOpt(options);

    return  (API_SemaphoreRWCreate("vxWorks SemRW", opt, LW_NULL));
}

/*
 * delete a semaphore
 */
STATUS  semDelete (SEM_ID semId)
{
    if (API_SemaphoreDelete(&semId)) {
        return  (ERROR);
    } else {
        return  (OK);
    }
}

/*
 * exchange two semaphore
 */
STATUS  semExchange (SEM_ID giveSemId, SEM_ID takeSemId, int timeout)
{
    ULONG   ulTo = vx2sySemTo(timeout);
    ULONG   ulError;

    switch (API_ObjectGetClass(takeSemId)) {

    case _OBJECT_SEM_B:
        ulError = API_SemaphorePostBPend(giveSemId, takeSemId, ulTo);
        break;

    case _OBJECT_SEM_C:
        ulError = API_SemaphorePostCPend(giveSemId, takeSemId, ulTo);
        break;

    default:
        errno = S_objLib_OBJ_OPERATION_UNSUPPORTED;
        return  (ERROR);
    }

    if (ulError) {
        if (ulError != S_objLib_OBJ_TIMEOUT) {
            errno = S_objLib_OBJ_UNAVAILABLE;
        }
        return  (ERROR);
    } else {
        return  (OK);
    }
}

/*
 * flush semaphore
 */
STATUS  semFlush (SEM_ID semId)
{
    ULONG   ulError = API_SemaphoreFlush(semId, LW_NULL);

    if (ulError) {
        errno = S_objLib_OBJ_UNAVAILABLE;
        return  (ERROR);
    } else {
        return  (OK);
    }
}

/*
 * give semaphore
 */
STATUS  semGive (SEM_ID semId)
{
    ULONG   ulError = API_SemaphorePost(semId);

    if (ulError) {
        errno = S_objLib_OBJ_UNAVAILABLE;
        return  (ERROR);
    } else {
        return  (OK);
    }
}

/*
 * take semaphore
 */
STATUS  semTake (SEM_ID semId, int timeout)
{
    ULONG   ulTo    = vx2sySemTo(timeout);
    ULONG   ulError = API_SemaphorePend(semId, ulTo);

    if (ulError) {
        if (ulError != S_objLib_OBJ_TIMEOUT) {
            errno = S_objLib_OBJ_UNAVAILABLE;
        }
        return  (ERROR);
    } else {
        return  (OK);
    }
}

/*
 * take semaphore
 */
STATUS  semRTake (SEM_ID semId, int timeout)
{
    ULONG   ulTo    = vx2sySemTo(timeout);
    ULONG   ulError = API_SemaphoreRWPendR(semId, ulTo);

    if (ulError) {
        if (ulError != S_objLib_OBJ_TIMEOUT) {
            errno = S_objLib_OBJ_UNAVAILABLE;
        }
        return  (ERROR);
    } else {
        return  (OK);
    }
}

/*
 * take semaphore
 */
STATUS  semWTake (SEM_ID semId, int timeout)
{
    ULONG   ulTo    = vx2sySemTo(timeout);
    ULONG   ulError = API_SemaphoreRWPendW(semId, ulTo);

    if (ulError) {
        if (ulError != S_objLib_OBJ_TIMEOUT) {
            errno = S_objLib_OBJ_UNAVAILABLE;
        }
        return  (ERROR);
    } else {
        return  (OK);
    }
}

/*
 * get semaphore infomation
 */
STATUS  semInfoGet (SEM_ID semId, SEM_INFO *pInfo)
{
    ULONG     ulError;
    ULONG     ulOption;
    ULONG     ulBlock;
    ULONG     ulWBlock;
    ULONG     ulCnt;
    LW_HANDLE ulOwn;

    if (!pInfo) {
        errno = EINVAL;
        return  (ERROR);
    }

    switch (API_ObjectGetClass(semId)) {

    case _OBJECT_SEM_B:
        pInfo->semType = SEM_TYPE_BINARY;
        ulError = API_SemaphoreBStatus(semId, &pInfo->state.full, &ulOption, &ulBlock);
        if (ulError == OK) {
            pInfo->options  = sy2vxSemOpt(ulOption);
            pInfo->numTasks = (UINT)ulBlock;
        }
        break;

    case _OBJECT_SEM_C:
        pInfo->semType = SEM_TYPE_COUNTING;
        ulError = API_SemaphoreCStatus(semId, &ulCnt, &ulOption, &ulBlock);
        if (ulError == OK) {
            pInfo->state.count = (UINT)ulCnt;
            pInfo->options  = sy2vxSemOpt(ulOption);
            pInfo->numTasks = (UINT)ulBlock;
        }
        break;

    case _OBJECT_SEM_M:
        pInfo->semType = SEM_TYPE_MUTEX;
        ulError = API_SemaphoreMStatusEx(semId, LW_NULL, &ulOption, &ulBlock, &ulOwn);
        if (ulError == OK) {
            if (ulOwn) {
                pInfo->state.owner = HANDLE_TO_TID(ulOwn);
            } else {
                pInfo->state.owner = -1;
            }
            pInfo->options  = sy2vxSemOpt(ulOption);
            pInfo->numTasks = (UINT)ulBlock;
        }
        break;

    case _OBJECT_SEM_RW:
        pInfo->semType = SEM_TYPE_RW;
        ulError = API_SemaphoreRWStatus(semId, LW_NULL,
                                        &ulBlock, &ulWBlock,
                                        &ulOption, &ulOwn);
        if (ulError == OK) {
            ulBlock += ulWBlock;
            if (ulOwn) {
                pInfo->state.owner = HANDLE_TO_TID(ulOwn);
            } else {
                pInfo->state.owner = -1;
            }
            pInfo->options  = sy2vxSemOpt(ulOption);
            pInfo->numTasks = (UINT)ulBlock;
        }
        break;

    default:
        errno = S_objLib_OBJ_UNAVAILABLE;
        return  (ERROR);
    }

    if (ulError) {
        errno = S_objLib_OBJ_UNAVAILABLE;
        return  (ERROR);
    } else {
        return  (OK);
    }
}

/*
 * show a sem
 */
STATUS  semShow (SEM_ID semId, int level)
{
    API_SemaphoreShow(semId);
    return  (OK);
}

/*
 * get sem block list
 */
int  semInfo (SEM_ID semId, int idList[], int maxTasks)
{
    errno = ENOSYS;
    return  (0);
}

/*
 * end
 */
