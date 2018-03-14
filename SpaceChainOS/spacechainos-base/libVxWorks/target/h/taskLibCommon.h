/**
 * @file
 * task common library.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#ifndef __VXWORKS_TASKLIBCOMMON_H
#define __VXWORKS_TASKLIBCOMMON_H

#include "eventLibCommon.h"
#include "rtpLibCommon.h"
#include "objLib.h"
#include "vxCpuLib.h"
#include "sys/taskDefs.h"

/* generic status codes */

#define S_taskLib_NAME_NOT_FOUND            ERROR_THREAD_NULL
#define S_taskLib_TASK_HOOK_TABLE_FULL      ERROR_KERNEL_HOOK_NULL
#define S_taskLib_TASK_HOOK_NOT_FOUND       ERROR_KERNEL_HOOK_NULL
#define S_taskLib_TASK_SWAP_HOOK_REFERENCED ERROR_KERNEL_HOOK_NULL
#define S_taskLib_TASK_SWAP_HOOK_SET        ERROR_KERNEL_HOOK_NULL
#define S_taskLib_TASK_SWAP_HOOK_CLEAR      ERROR_KERNEL_HOOK_NULL
#define S_taskLib_TASK_VAR_NOT_FOUND        ERROR_THERAD_VAR_NULL
#define S_taskLib_TASK_UNDELAYED            ERROR_THREAD_NOT_SLEEP
#define S_taskLib_ILLEGAL_PRIORITY          ERROR_THREAD_PRIORITY_WRONG
#define S_taskLib_ILLEGAL_OPTIONS           ERROR_THREAD_OPTION
#define S_taskLib_NO_TCB                    ERROR_THREAD_FULL
#define S_taskLib_ILLEGAL_OPERATION         ERROR_THREAD_NULL
#define S_taskLib_ILLEGAL_STACK_INFO        ERROR_THREAD_OPTION

/* miscellaneous */

#define VX_TASK_NAME_LENGTH LW_CFG_OBJECT_NAME_SIZE  /* max length of name in TASK_DESC */

/*
 * Task option bits.  BE SURE to modify either of the
 * VX_USR_TASK_OPTIONS_BASE or VX_USR_TASK_OPTIONS definitions
 * below when modifying/adding individual task options.
 */

#define VX_FP_TASK          0x0008
#define VX_PRIVATE_ENV      0x0080  /* 1 = private environment variables SYLIXOS INVAL! */
#define VX_NO_STACK_FILL    0x0100  /* 1 = avoid stack fill of 0xee SYLIXOS INVAL! */
#define VX_TASK_NOACTIVATE  0x2000  /* taskOpen() does not taskActivate() */
#define VX_NO_STACK_PROTECT 0x4000  /* no over/underflow stack protection,*/
                                    /* stack space remains executable     */

/* define for all valid user task options */

#define VX_USR_TASK_OPTIONS_BASE    (VX_PRIVATE_ENV         | \
                                     VX_NO_STACK_FILL       | \
                                     VX_TASK_NOACTIVATE     | \
                                     VX_NO_STACK_PROTECT)

typedef struct {                        /* TASK_DESC - information structure */
    int         td_id;                  /* task id */
    int         td_priority;            /* task priority */
    int         td_status;              /* task status */
    int         td_options;             /* task option bits (see below) */
    FUNCPTR     td_entry;               /* original entry point of task */
    char *      td_sp;                  /* saved stack pointer */
    char *      td_pStackBase;          /* the bottom of the stack */
    char *      td_pStackEnd;           /* the actual end of the stack */
    int         td_stackSize;           /* size of stack in bytes */
    int         td_stackCurrent;        /* current stack usage in bytes */
    int         td_stackHigh;           /* maximum stack usage in bytes */
    int         td_stackMargin;         /* current stack margin in bytes */
    int         td_errorStatus;         /* most recent task error status */
    int         td_delay;               /* delay/timeout ticks */
    EVENTS_DESC td_events;              /* VxWorks events information */
    char        td_name[VX_TASK_NAME_LENGTH + 1];   /* name of task */
    RTP_ID      td_rtpId;               /* RTP owning the task */
    int         td_cpuIndex;            /* cpu index running on (if any) */
} TASK_DESC;

#ifdef __cplusplus
extern "C" {
#endif

/* function declarations */

int     taskSpawn(char * name, int priority, int options,
                  int stackSize, FUNCPTR entryPt, int arg1,
                  int arg2, int arg3, int arg4, int arg5,
                  int arg6, int arg7, int arg8, int arg9,
                  int arg10);
int     taskCreate(char * name, int priority, int options,
                   int stackSize, FUNCPTR entryPt, int arg1,
                   int arg2, int arg3, int arg4, int arg5,
                   int arg6, int arg7, int arg8, int arg9,
                   int arg10);
STATUS  taskActivate(int tid);
STATUS  taskDelete(int tid);
STATUS  taskDeleteForce(int tid);
STATUS  taskSuspend(int tid);
STATUS  taskResume(int tid);
STATUS  taskRestart(int tid);
STATUS  taskPrioritySet(int tid, int newPriority);
STATUS  taskPriorityGet(int tid, int * pPriority);
STATUS  taskStackSizeGet(int tid, int * pStackSize);
char   *taskName(int tid);
int     taskNameToId(char * name);
int     taskIdSelf(void);
STATUS  taskIdVerify(int tid);
int     taskIdDefault(int tid);
BOOL    taskIsReady(int tid);
BOOL    taskIsSuspended(int tid);
BOOL    taskIsPended(int tid);
BOOL    taskIsDelayed(int tid, int * pDelayTicks);
STATUS  taskInfoGet(int tid, TASK_DESC * pTaskDesc);
STATUS  taskOptionsGet(int tid, int * pOptions);
STATUS  taskDelay(int ticks);
STATUS  taskSafe(void);
STATUS  taskUnsafe(void);
STATUS  taskUndelay(int tid);
STATUS  taskCpuAffinitySet(int tid, cpuset_t newAffinity);
STATUS  taskCpuAffinityGet(int tid, cpuset_t* pAffinity);

#ifdef __cplusplus
}
#endif

#endif /* __VXWORKS_TASKLIBCOMMON_H */
