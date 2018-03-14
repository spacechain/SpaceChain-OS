/**
 * @file
 * Real-Time process common library.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#ifndef __VXWORKS_RTPLIBCOMMON_H
#define __VXWORKS_RTPLIBCOMMON_H

#include "vxWorksCommon.h"
#include "objLib.h"

/* creation mode options */

#define RTP_GLOBAL_SYMBOLS      0x01    /* register global symbols for RTP */
#define RTP_LOCAL_SYMBOLS       0x02    /* idem for local symbols */
#define RTP_ALL_SYMBOLS         (RTP_GLOBAL_SYMBOLS | RTP_LOCAL_SYMBOLS)
#define RTP_DEBUG               0x10    /* set RTP in debug mode when created */
#define RTP_BUFFER_VAL_OFF      0x20    /* disable buffer validation for all */
                                        /* system calls issued from the RTP  */
#define RTP_LOADED_WAIT         0x40    /* Wait until the RTP is loaded */
#define RTP_CPU_AFFINITY_NONE   0x80    /* Remove any CPU affinity (SMP) */

/* Error Status codes */

#define S_rtpLib_INVALID_FILE                   EINVAL
#define S_rtpLib_INVALID_OPTION                 EINVAL
#define S_rtpLib_ACCESS_DENIED                  EACCES
#define S_rtpLib_INVALID_RTP_ID                 ESRCH
#define S_rtpLib_NO_SYMBOL_TABLE                ERROR_LOADER_NO_SYMBOL
#define S_rtpLib_INVALID_SEGMENT_START_ADDRESS  ERROR_LOADER_FORMAT
#define S_rtpLib_INVALID_SYMBOL_REGISTR_POLICY  ERROR_LOADER_EXPORT_SYM
#define S_rtpLib_INSTANTIATE_FAILED             ERROR_LOADER_NO_MODULE
#define S_rtpLib_INVALID_TASK_OPTION            ERROR_THREAD_OPTION
#define S_rtpLib_RTP_NAME_LENGTH_EXCEEDED       ENAMETOOLONG /* rtpInfoGet */

#define VX_RTP_NAME_LENGTH  PATH_MAX /* max name length for diplay */

/*
 * The 'status' field (32 bit integer) of a RTP holds the RTP state and status.
 *
 * NOTE: RTP_STATE_GET()   : read the RTP state(s)
 *       RTP_STATE_PUT()   : write the RTP state(s)
 *       RTP_STATE_SET()   : set a RTP state
 *       RTP_STATE_UNSET() : unset a RTP state
 *
 *       RTP_STATUS_GET()   : read the RTP status
 *       RTP_STATUS_PUT()   : write the RTP status
 *       RTP_STATUS_SET()   : set a RTP status
 *       RTP_STATUS_UNSET() : unset a RTP status
 *
 * The PUT/SET/UNSET macros are available only in the kernel headers.
 */

/* RTP states */

#define RTP_STATE_CREATE           0x0001   /* RTP is under construction */
#define RTP_STATE_NORMAL           0x0002   /* RTP is ready */
#define RTP_STATE_DELETE           0x0004   /* RTP is being deleted */

#define RTP_STATUS_STOP            0x0100   /* RTP has recieved stopped signal */
#define RTP_STATUS_ELECTED_DELETER 0x0200   /* RTP delete has started */

#define RTP_STATE_MASK            (RTP_STATE_CREATE | RTP_STATE_NORMAL | \
                                   RTP_STATE_DELETE)
#define RTP_STATUS_MASK           (RTP_STATUS_STOP | RTP_STATUS_ELECTED_DELETER)

#define RTP_STATE_GET(value)    ((value) & RTP_STATE_MASK)
#define RTP_STATUS_GET(value)   ((value) & RTP_STATUS_MASK)

/* IS_RTP_ macros */

#define IS_RTP_STATE_NORMAL(value) ((RTP_STATE_GET (value) & RTP_STATE_NORMAL) \
                                    == RTP_STATE_NORMAL)
#define IS_RTP_STATE_CREATE(value) ((RTP_STATE_GET (value) & RTP_STATE_CREATE) \
                                    == RTP_STATE_CREATE)
#define IS_RTP_STATE_DELETE(value) ((RTP_STATE_GET (value) & RTP_STATE_DELETE) \
                                    == RTP_STATE_DELETE)
#define IS_RTP_STATUS_STOP(value) ((RTP_STATUS_GET (value) & \
                                    RTP_STATUS_STOP) == RTP_STATUS_STOP)
#define IS_RTP_STATUS_ELECTED_DELETER(value) ((RTP_STATUS_GET (value) & \
                                              RTP_STATUS_ELECTED_DELETER) == RTP_STATUS_ELECTED_DELETER)

/* typedefs */

typedef pid_t   RTP_ID;

typedef struct {            /* RTP_DESC - information structure */
    char    pathName[VX_RTP_NAME_LENGTH + 1]; /* path to RTP executable */
    int     status;         /* the status field of the RTP */
    UINT32  options;        /* option bits, e.g. debug, symtable */
    void *  entrAddr;       /* entry point of ELF file */
    int     initTaskId;     /* the initial task ID */
    INT32   taskCnt;        /* number of tasks in the RTP */
    RTP_ID  parentId;       /* RTP ID of the parent */
} RTP_DESC;

#ifdef __cplusplus
extern "C" {
#endif

RTP_ID   rtpSpawn(const char * rtpFileName, const char * argv[],
                  const char * envp[], int priority, int uStackSize,
                  int options, int taskOptions);

STATUS   rtpInfoGet(RTP_ID rtpId,
                    RTP_DESC *rtpStruct);

#ifdef __cplusplus
}
#endif

#endif /* __VXWORKS_RTPLIBCOMMON_H */
