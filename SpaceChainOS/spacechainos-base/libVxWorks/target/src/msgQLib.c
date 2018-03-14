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

#include <stdlib.h>
#include "msgQLib.h"

/*
 * vxWorks option to SylixOS
 */
static ULONG  vx2syMsgQOpt (int options)
{
    ULONG  opt = 0;

    if (options & MSG_Q_PRIORITY) {
        opt |= LW_OPTION_WAIT_PRIORITY;
    }
    if (options & MSG_Q_INTERRUPTIBLE) {
        opt |= LW_OPTION_SIGNAL_INTER;
    }

    return  (opt);
}

/*
 * SylixOS option to vxWorks
 */
static int  sy2vxMsgQOpt (ULONG  ulOpt)
{
    int opt = 0;

    if (ulOpt & LW_OPTION_WAIT_PRIORITY) {
        opt |= MSG_Q_PRIORITY;
    }
    if (ulOpt & LW_OPTION_SIGNAL_INTER) {
        opt |= MSG_Q_INTERRUPTIBLE;
    }

    return  (opt);
}

/*
 * vxWorks timeout to SylixOS
 */
static ULONG  vx2syMsgQTo (int timeout)
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
 * init msg queue
 */
STATUS  msgQLibInit (void)
{
    return  (OK);
}

/*
 * msg queue create init
 */
void  msgQCreateLibInit (void)
{
}

/*
 * msg queue show init
 */
void  msgQShowInit (void)
{
}

/*
 * show msg queue
 */
STATUS  msgQShow (MSG_Q_ID msgQId, int level)
{
    if (!msgQId) {
        errno = S_objLib_OBJ_UNAVAILABLE;
        return  (ERROR);
    }

    API_MsgQueueShow(msgQId->syMsgq);

    return  (OK);
}

/*
 * create a msg queue
 */
MSG_Q_ID  msgQCreate (int maxMsgs, int maxMsgLength, int options)
{
    MSG_Q_ID  qid;
    ULONG     ulOpt;

    qid = (MSG_Q_ID)malloc(sizeof(MSG_Q_HDL));
    if (!qid) {
        errno = ENOMEM;
        return  (LW_NULL);
    }

    ulOpt = vx2syMsgQOpt(options);

    qid->syMsgq = API_MsgQueueCreate("vxWorks MsgQ", (ULONG)maxMsgs,
                                     (ULONG)maxMsgLength, ulOpt, LW_NULL);
    if (qid->syMsgq == LW_OBJECT_HANDLE_INVALID) {
        free(qid);
        return  (LW_NULL);
    }

    return  (qid);
}

/*
 * delete a msg queue
 */
STATUS  msgQDelete (MSG_Q_ID msgQId)
{
    if (!msgQId) {
        errno = S_objLib_OBJ_UNAVAILABLE;
        return  (ERROR);
    }

    if (API_MsgQueueDelete(&msgQId->syMsgq)) {
        return  (ERROR);
    }

    free(msgQId);

    return  (OK);
}

/*
 * send a msg to msgqueue
 */
STATUS  msgQSend (MSG_Q_ID msgQId, char *buffer, UINT nBytes,
                  int timeout, int priority)
{
    ULONG   ulError;

    if (!msgQId) {
        errno = S_objLib_OBJ_UNAVAILABLE;
        return  (ERROR);
    }

    if (priority == MSG_PRI_URGENT) {
        ulError = API_MsgQueueSendEx2(msgQId->syMsgq, buffer, nBytes,
                                      vx2syMsgQTo(timeout), LW_OPTION_URGENT);
    } else {
        ulError = API_MsgQueueSend2(msgQId->syMsgq, buffer, nBytes, vx2syMsgQTo(timeout));
    }

    if (ulError == ERROR_NONE) {
        return  (OK);

    } else {
        return  (ERROR);
    }
}

/*
 * recv a msg from msgqueue
 */
int  msgQReceive (MSG_Q_ID msgQId, char *buffer, UINT maxNBytes, int timeout)
{
    ULONG   ulError;
    size_t  stLen;

    if (!msgQId) {
        errno = S_objLib_OBJ_UNAVAILABLE;
        return  (ERROR);
    }

    ulError = API_MsgQueueReceive(msgQId->syMsgq, buffer, maxNBytes, &stLen, vx2syMsgQTo(timeout));
    if (ulError) {
        if (ulError != S_objLib_OBJ_TIMEOUT) {
            errno = S_objLib_OBJ_UNAVAILABLE;
        }
        return  (ERROR);

    } else {
        return  (OK);
    }
}

STATUS  msgQInfoGet (MSG_Q_ID msgQId, MSG_Q_INFO *pInfo)
{
    ULONG   ulError;
    ULONG   ulMaxMsgNum;
    ULONG   ulCounter;
    size_t  stMsgLen;
    ULONG   ulOption;
    ULONG   ulThreadBlockNum;
    size_t  stMaxMsgLen;

    if (!pInfo) {
        errno = EINVAL;
        return  (ERROR);
    }

    if (!msgQId) {
        errno = S_objLib_OBJ_UNAVAILABLE;
        return  (ERROR);
    }

    ulError = API_MsgQueueStatusEx(msgQId->syMsgq,
                                   &ulMaxMsgNum,
                                   &ulCounter,
                                   &stMsgLen,
                                   &ulOption,
                                   &ulThreadBlockNum,
                                   &stMaxMsgLen);
    if (ulError) {
        errno = S_objLib_OBJ_UNAVAILABLE;
        return  (ERROR);

    } else {
        pInfo->maxMsgLength = (int)stMaxMsgLen;
        pInfo->maxMsgs      = (int)ulMaxMsgNum;
        pInfo->numMsgs      = (int)ulCounter;
        pInfo->numTasks     = (int)ulThreadBlockNum;
        pInfo->options      = sy2vxMsgQOpt(ulOption);
        pInfo->recvTimeouts = 0;
        pInfo->sendTimeouts = 0;
        return  (OK);
    }
}

/*
 * get msgqueue msg num
 */
int  msgQNumMsgs (MSG_Q_ID msgQId)
{
    ULONG   ulError;
    ULONG   ulMsg;

    if (!msgQId) {
        errno = S_objLib_OBJ_UNAVAILABLE;
        return  (ERROR);
    }

    ulError = API_MsgQueueStatus(msgQId->syMsgq, LW_NULL, &ulMsg, LW_NULL, LW_NULL, LW_NULL);
    if (ulError) {
        errno = S_objLib_OBJ_UNAVAILABLE;
        return  (ERROR);

    } else {
        return  ((int)ulMsg);
    }
}

/*
 * end
 */
