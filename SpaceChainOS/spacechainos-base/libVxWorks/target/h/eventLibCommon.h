/**
 * @file
 * event common library.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#ifndef __VXWORKS_EVENTLIBCOMMON_H
#define __VXWORKS_EVENTLIBCOMMON_H

#include "vxWorksCommon.h"
#include "objLib.h"

/* errnos */

#define S_eventLib_TIMEOUT                  ERROR_THREAD_WAIT_TIMEOUT
#define S_eventLib_NOT_ALL_EVENTS           ERROR_EVENTSET_OPTION
#define S_eventLib_ALREADY_REGISTERED       ERROR_EVENTSET_OPTION
#define S_eventLib_EVENTSEND_FAILED         ERROR_EVENTSET_OPTION
#define S_eventLib_ZERO_EVENTS              ERROR_EVENTSET_NULL
#define S_eventLib_TASK_NOT_REGISTERED      ERROR_EVENTSET_OPTION
#define S_eventLib_NULL_TASKID_AT_INT_LEVEL ERROR_EVENTSET_OPTION

/* options */

#define EVENTS_WAIT_ALL         0x00    /* wait for all events to occur */
#define EVENTS_WAIT_ANY         0x01    /* wait for one of many events  */
#define EVENTS_RETURN_ALL       0x02    /* return all events received   */
#define EVENTS_KEEP_UNWANTED    0x04    /* don't clear unwanted events  */
#define EVENTS_FETCH            0x80    /* return events received immediately */

/* resource-related options */

#define EVENTS_OPTIONS_NONE     0x00
#define EVENTS_SEND_ONCE        0x01  /* unregister events after eventSend   */
#define EVENTS_ALLOW_OVERWRITE  0x02  /* can overwrite previous registration */
#define EVENTS_SEND_IF_FREE     0x04  /* send events in xxxEvStart()         */

/* configuration options */

#define EVENTS_NONINTERRUPTIBLE 0x00    /* signals don't wake up RTP tasks */
#define EVENTS_INTERRUPTIBLE    0x01    /* signals wake up RTP tasks */

/*
 * Defines for all 24 events available to users. Events 25 to 32 are reserved
 * for system use and should never be used outside of that scope
 */

#define VXEV01 0x00000001
#define VXEV02 0x00000002
#define VXEV03 0x00000004
#define VXEV04 0x00000008
#define VXEV05 0x00000010
#define VXEV06 0x00000020
#define VXEV07 0x00000040
#define VXEV08 0x00000080
#define VXEV09 0x00000100
#define VXEV10 0x00000200
#define VXEV11 0x00000400
#define VXEV12 0x00000800
#define VXEV13 0x00001000
#define VXEV14 0x00002000
#define VXEV15 0x00004000
#define VXEV16 0x00008000
#define VXEV17 0x00010000
#define VXEV18 0x00020000
#define VXEV19 0x00040000
#define VXEV20 0x00080000
#define VXEV21 0x00100000
#define VXEV22 0x00200000
#define VXEV23 0x00400000
#define VXEV24 0x00800000

#define VXEV_USER_MASK 0x00ffffff

/* typedefs */

typedef struct events_desc {
    UINT32 wanted;      /* 0x00: events wanted          */
    UINT32 received;        /* 0x04: all events received        */
    UINT8  options;     /* 0x08: user options           */
} EVENTS_DESC;

#ifdef __cplusplus
extern "C" {
#endif

STATUS eventSend(int taskId, UINT32 events);
STATUS eventReceive(UINT32 events,
                    UINT8 options,
                    int timeout,
                    UINT32 *eventsReceived);
STATUS eventClear(void);

#ifdef __cplusplus
}
#endif

#endif /* __VXWORKS_EVENTLIBCOMMON_H */
