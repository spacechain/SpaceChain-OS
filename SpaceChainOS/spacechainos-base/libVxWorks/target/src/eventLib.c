/**
 * @file
 * event library.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#include "eventLib.h"
#include "taskLib.h"

/*
 * vxWorks timeout to SylixOS
 */
static ULONG  vx2syEvtTo (int timeout)
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
 * send an event to task
 */
STATUS eventSend (int taskId, UINT32 events)
{
    LW_HANDLE event;

    TID_FIX(taskId);

    event = TID_TO_EVENT(taskId);

    if (API_EventSetSet(event, (ULONG)events, LW_OPTION_EVENTSET_SET)) {
        return  (ERROR);
    } else {
        return  (OK);
    }
}

/*
 * recv an event
 */
STATUS eventReceive (UINT32 events,
                     UINT8 options,
                     int timeout,
                     UINT32 *eventsReceived)
{
    int taskId = 0;
    LW_HANDLE event;
    ULONG ulOpt = LW_OPTION_EVENTSET_WAIT_SET_ALL;
    ULONG ulRecv;

    TID_FIX(taskId);

    event = TID_TO_EVENT(taskId);

    if (options & EVENTS_FETCH) {
        if (API_EventSetStatus(event, &ulRecv, LW_NULL)) {
            return  (ERROR);
        } else {
            if (eventsReceived) {
                *eventsReceived = (UINT32)ulRecv;
            }
            return  (OK);
        }
    }

    ulOpt = LW_OPTION_EVENTSET_WAIT_SET_ALL;

    if (options & EVENTS_WAIT_ANY) {
        ulOpt = LW_OPTION_EVENTSET_WAIT_SET_ANY;
    }

    if (options & EVENTS_RETURN_ALL) {
        ulOpt |= (LW_OPTION_EVENTSET_RETURN_ALL | LW_OPTION_EVENTSET_RESET_ALL);
    } else {
        if (options & EVENTS_KEEP_UNWANTED) {
            ulOpt |= LW_OPTION_EVENTSET_RESET;
        } else {
            ulOpt |= LW_OPTION_EVENTSET_RESET_ALL;
        }
    }

    if (API_EventSetGetEx(event, (ULONG)events, ulOpt, vx2syEvtTo(timeout), &ulRecv)) {
        return  (ERROR);
    } else {
        if (eventsReceived) {
            *eventsReceived = (UINT32)ulRecv;
        }
        return  (OK);
    }
}

/*
 * clear an event
 */
STATUS eventClear (void)
{
    int taskId = 0;
    LW_HANDLE event;

    TID_FIX(taskId);

    event = TID_TO_EVENT(taskId);

    if (API_EventSetSet(event, __ARCH_ULONG_MAX, LW_OPTION_EVENTSET_CLR)) {
        return  (ERROR);
    } else {
        return  (OK);
    }
}

/*
 * end
 */
