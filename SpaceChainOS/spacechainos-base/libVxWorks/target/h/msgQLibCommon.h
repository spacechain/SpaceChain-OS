/**
 * @file
 * message queue common library.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#ifndef __VXWORKS_MSGQLIBCOMMON_H
#define __VXWORKS_MSGQLIBCOMMON_H

#include "vxWorksCommon.h"
#include "msgQLibCommon.h"
#include "objLib.h"

/* generic status codes */

#define S_msgQLib_INVALID_MSG_LENGTH            (ERROR_MSGQUEUE_MSG_LEN)
#define S_msgQLib_NON_ZERO_TIMEOUT_AT_INT_LEVEL (ERROR_MSGQUEUE_OPTION)
#define S_msgQLib_INVALID_QUEUE_TYPE            (ERROR_MSGQUEUE_TYPE)
#define S_msgQLib_ILLEGAL_OPTIONS               (ERROR_MSGQUEUE_OPTION)
#define S_msgQLib_ILLEGAL_PRIORITY              (ERROR_MSGQUEUE_OPTION)
#define S_msgQLib_UNSUPPORTED_OPERATION         (ENOSYS)
#define S_msgQLib_INVALID_MSG_COUNT             (ERROR_MSGQUEUE_MAX_COUNTER_NULL)

/* message queue options */

#define MSG_Q_TYPE_MASK             0x01    /* mask for pend queue type in options */
#define MSG_Q_FIFO                  0x00    /* tasks wait in FIFO order */
#define MSG_Q_PRIORITY              0x01    /* tasks wait in PRIORITY order */
#define MSG_Q_EVENTSEND_ERR_NOTIFY  0x02    /* notify when eventRsrcSend fails */
#define MSG_Q_INTERRUPTIBLE         0x04    /* interruptible on RTP signal */

/* message send priorities */

#define MSG_PRI_NORMAL  0   /* normal priority message */
#define MSG_PRI_URGENT  1   /* urgent priority message */

/* message queue typedefs */
typedef struct {            /* MSG_Q_INFO */
    int     numMsgs;        /* OUT: number of messages queued */
    int     numTasks;       /* OUT: number of tasks waiting on msg q */

    int     sendTimeouts;   /* OUT: count of send timeouts */
    int     recvTimeouts;   /* OUT: count of receive timeouts */

    int     options;        /* OUT: options with which msg q was created */
    int     maxMsgs;        /* OUT: max messages that can be queued */
    int     maxMsgLength;   /* OUT: max byte length of each message */
} MSG_Q_INFO;

typedef struct {
    LW_HANDLE   reserved;
    LW_HANDLE   syMsgq;
} MSG_Q_HDL;
typedef MSG_Q_HDL *MSG_Q_ID;

/* function declarations */

#ifdef __cplusplus
extern "C" {
#endif

MSG_Q_ID        msgQCreate(int maxMsgs, int maxMsgLength, int options);
STATUS          msgQDelete(MSG_Q_ID msgQId);
STATUS          msgQSend(MSG_Q_ID msgQId, char *buffer, UINT nBytes,
                         int timeout, int priority);
int             msgQReceive(MSG_Q_ID msgQId, char *buffer, UINT maxNBytes,
                            int timeout);
STATUS          msgQInfoGet(MSG_Q_ID msgQId, MSG_Q_INFO *pInfo);
int             msgQNumMsgs(MSG_Q_ID msgQId);

#ifdef __cplusplus
}
#endif

#endif /* __VXWORKS_MSGQLIBCOMMON_H */
