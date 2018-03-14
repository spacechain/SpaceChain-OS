/**
 * @file
 * semphore common library.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#ifndef __VXWORKS_SEMLIBCOMMON_H
#define __VXWORKS_SEMLIBCOMMON_H

#include "vxWorksCommon.h"
#include "objLib.h"

/* generic status codes */

#define S_semLib_INVALID_STATE              ERROR_EVENT_WAS_DELETED
#define S_semLib_INVALID_OPTION             ERROR_EVENT_NULL
#define S_semLib_INVALID_QUEUE_TYPE         ERROR_EVENT_TYPE
#define S_semLib_INVALID_OPERATION          ERROR_EVENT_NOT_OWN
#define S_semLib_INVALID_INITIAL_COUNT      ERROR_EVENT_INIT_COUNTER
#define S_semLib_COUNT_OVERFLOW             ERROR_EVENT_FULL

/* semaphore options */

#define SEM_Q_FIFO                  0x00   /* first in first out queue */
#define SEM_Q_PRIORITY              0x01   /* priority sorted queue */
#define SEM_DELETE_SAFE             0x04   /* owner delete safe (mutex opt.) */
#define SEM_INVERSION_SAFE          0x08   /* no priority inversion (mutex opt.) */
#define SEM_EVENTSEND_ERR_NOTIFY    0x10   /* notify when eventRsrcSend fails */
#define SEM_INTERRUPTIBLE           0x20   /* interruptible on RTP signal */
#define SEM_NO_ID_VALIDATE          0x40   /* inline: no semaphore validation */
#define SEM_NO_ERROR_CHECK          0x80   /* inline: no error checking */
#define SEM_NO_EVENT_SEND           0x100  /* inline: do not send events */
#define SEM_NO_SYSTEM_VIEWER        0x200  /* inline: no system viewer events */
#define SEM_NO_RECURSE              0x400  /* inline (mutex opt.): no recursion */

#define SEM_BIN_OPTIONS_MASK    (SEM_Q_FIFO |   \
                                 SEM_Q_PRIORITY |   \
                                 SEM_EVENTSEND_ERR_NOTIFY | \
                                 SEM_INTERRUPTIBLE)

#define SEM_CNT_OPTIONS_MASK        SEM_BIN_OPTIONS_MASK

#define SEM_MUT_OPTIONS_MASK    (SEM_BIN_OPTIONS_MASK |     \
                                 SEM_DELETE_SAFE |      \
                                 SEM_INVERSION_SAFE)

#define SEM_RW_OPTIONS_MASK     (SEM_MUT_OPTIONS_MASK)

/* semaphore types */

typedef LW_HANDLE   SEM_ID;

typedef enum {                  /* SEM_TYPE */
    SEM_TYPE_BINARY,            /* 0: binary semaphore */
    SEM_TYPE_MUTEX,             /* 1: mutual exclusion semaphore */
    SEM_TYPE_COUNTING,          /* 2: counting semaphore */
    SEM_TYPE_OLD,               /* 3: 4.x style semaphore  (obsolete) */
    SEM_TYPE_RESERVED1,         /* 4: reserved for shared binary semaphore */
    SEM_TYPE_RESERVED2,         /* 5: reserved for shared counting semaphore */
    SEM_TYPE_RW,                /* 6: read/write semaphore */

    /*
     * The semType field of the semaphore structure is a single byte.
     *
     * Currently only 9 semaphore types exist, however, due to the use
     * of the SEM_TYPE_MASK macro as a quick means of determining the validity
     * of a semaphore type it is necessary to define the SEM_TYPE_MAX value
     * as 16 (the mask is now 4 bits, or 0xf).
     *
     * The SEM_TYPE_MASK macro is currently defined in the private semaphore
     * header file.  It must be updated if the number of semaphore types
     * reaches 17 or more.
     */
    SEM_TYPE_MAX = 16
} SEM_TYPE;

/* binary semaphore initial state */

typedef enum {          /* SEM_B_STATE */
    SEM_EMPTY,          /* 0: semaphore not available */
    SEM_FULL            /* 1: semaphore available */
} SEM_B_STATE;

/*
 * Information structure filled by semInfoGet.  The taskIdListMax
 * and taskIdList information is not provided in user land.
 */

typedef struct {            /* SEM_INFO */
    UINT    numTasks;       /* OUT: number of blocked tasks */
    SEM_TYPE    semType;    /* OUT: semaphore type */
    int     options;        /* OUT: options with which sem was created */
    union {
        UINT    count;      /* OUT: semaphore count (couting sems) */
        BOOL    full;       /* OUT: binary semaphore FULL? */
        int     owner;      /* OUT: task ID of mutex semaphore owner */
    } state;
} SEM_INFO;

#ifdef __cplusplus
extern "C" {
#endif

/* function declarations */

SEM_ID     semMCreate(int options);
SEM_ID     semBCreate(int options, SEM_B_STATE initialState);
SEM_ID     semCCreate(int options, int initialCount);
SEM_ID     semRWCreate(int options, int maxReaders);
STATUS     semDelete(SEM_ID semId);
STATUS     semExchange(SEM_ID giveSemId, SEM_ID takeSemId,
                       int timeout);
STATUS     semFlush(SEM_ID semId);
STATUS     semGive(SEM_ID semId);
STATUS     semTake(SEM_ID semId, int timeout);
STATUS     semRTake(SEM_ID semId, int timeout);
STATUS     semWTake(SEM_ID semId, int timeout);
STATUS     semInfoGet(SEM_ID semId, SEM_INFO *pInfo);

#ifdef __cplusplus
}
#endif

#endif /* __VXWORKS_SEMLIBCOMMON_H */
