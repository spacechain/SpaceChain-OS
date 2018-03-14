/**
 * @file
 * task library.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#ifndef __VXWORKS_TASKLIB_H
#define __VXWORKS_TASKLIB_H

#include "taskLibCommon.h"

/* SylixOS handle to tid */

#define TID_FIX(tid)        { if (tid == 0) { tid = taskIdSelf(); } }
#define TID_TO_HANDLE(tid)  vxTcb[tid].task
#define TID_TO_EVENT(tid)   vxTcb[tid].event
#define HANDLE_TO_TID(h)    API_ObjectGetIndex(h)


/* miscellaneous */

#define TASK_PRIORITY_SELF  -1  /* passing normal Priority */

#define MAX_TASK_ARGS       10  /* max args passed to a task */

#define VX_MAX_TASK_SWITCH_RTNS 16  /* max task switch callout routines */
#define VX_MAX_TASK_SWAP_RTNS   16  /* max task swap callout routines */
#define VX_MAX_TASK_DELETE_RTNS 20  /* max task delete callout routines */
#define VX_MAX_TASK_CREATE_RTNS 20  /* max task create callout routines */

/* The MAX priority (lowest priority) that a public task can have */

#define VX_TASK_PRIORITY_MAX    254

/* The MIN priority (highest priority) that a public task can have */

#define VX_TASK_PRIORITY_MIN    1


/* kernel only task option bits (see taskLibCommon.h for common option bits) */

#define VX_UNBREAKABLE      0x0002  /* breakpoints ignored */
#define VX_STDIO            0x0010  /* OBSOLETE: need not be set for stdio*/
#define VX_ADA_DEBUG        0x0020  /* 1 = VADS debugger support */
#define VX_FORTRAN          0x0040  /* 1 = NKR FORTRAN support */
#define VX_SMP_INTLOCKED    0x0001  /* tasks holds global interrupt lock */

/* task status */

#define WIND_READY         0x00    /* ready to run */
#define WIND_SUSPEND       0x01    /* explicitly suspended */
#define WIND_PEND          0x02    /* pending on semaphore */
#define WIND_DELAY         0x04    /* task delay (or timeout) */
#define WIND_DEAD          0x08    /* dead task */
#define WIND_STOP          0x10    /* stopped by the debugger */
#define WIND_STATE_CHANGE  0x20    /* currently changing state */

/* TASK_LOCK()/UNLOCK() is now replaced by TASK_CPU_LOCK()/UNLOCK(() */

#define TASK_LOCK()     taskLock()
#define TASK_UNLOCK()   taskUnlock()

#define TASK_SAFE()     taskSafe()
#define TASK_UNSAFE()   taskUnsafe()

/* type define */

typedef struct windTcb {
    LW_HANDLE   task;
    LW_HANDLE   event;
    int         id;
    FUNCPTR     entry;
    int         arg[10];
    char        name[VX_TASK_NAME_LENGTH+1];
} WIND_TCB;

extern WIND_TCB vxTcb[];

#define taskExit(exitCode)  exit((exitCode))

#ifdef __cplusplus
extern "C" {
#endif

/* function declarations */

STATUS   taskLibInit(int usrDefExcStackSize,
                     int usrDefExcStackOverflowSize,
                     int kernelDefExcStackOverflowSize,
                     int usrDefExecStackOverflowSize,
                     int usrDefExecStackUnderflowSize,
                     int kernelDefExecStackOverflowSize,
                     int kernelDefExecStackUnderflowSize);
void     taskCreateLibInit(void);
void     taskOpenInit(void);
STATUS   taskInit(WIND_TCB *pTcb, char *name, int priority,
                   int options, char *pStackBase, int stackSize,
                   FUNCPTR entryPt, int arg1, int arg2, int arg3,
                   int arg4, int arg5, int arg6, int arg7,
                   int arg8, int arg9, int arg10);
STATUS   taskOptionsSet(int tid, int mask, int newOptions);
WIND_TCB *taskTcb(int tid);
int      taskIdListGet(int idList[], int maxTasks);
STATUS   taskPriNormalGet(int tid,int * priNormal);
STATUS   taskStatusString(int tid, char * pString);
STATUS   taskOptionsString(int tid, char * pString);
void     taskShowInit(void);
STATUS   taskShow(int tid, int level);
STATUS   taskLock(void);
STATUS   taskUnlock(void);
STATUS   taskCpuLock(void);
STATUS   taskCpuUnlock(void);

#ifdef __cplusplus
}
#endif

#endif /* __VXWORKS_TASKLIB_H */
