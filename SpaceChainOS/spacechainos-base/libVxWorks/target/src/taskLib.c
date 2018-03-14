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

#include <unistd.h>
#include <stdlib.h>
#include "taskLib.h"
#include "taskVarLib.h"
#include "taskWaitShow.h"

/*
 * vxWorks task parameter
 */
static int vxusrDefExcStackSize              = LW_CFG_PTHREAD_DEFAULT_STK_SIZE;
static int vxusrDefExcStackOverflowSize      = LW_CFG_THREAD_DEFAULT_GUARD_SIZE;
static int vxkernelDefExcStackOverflowSize   = LW_CFG_THREAD_DEFAULT_GUARD_SIZE;
static int vxusrDefExecStackOverflowSize     = LW_CFG_THREAD_DEFAULT_GUARD_SIZE;
static int vxusrDefExecStackUnderflowSize    = LW_CFG_THREAD_DEFAULT_GUARD_SIZE;
static int vxkernelDefExecStackOverflowSize  = LW_CFG_THREAD_DEFAULT_GUARD_SIZE;
static int vxkernelDefExecStackUnderflowSize = LW_CFG_THREAD_DEFAULT_GUARD_SIZE;

/*
 * vxWorks tcb pool
 */
WIND_TCB vxTcb[LW_CFG_MAX_THREADS];
static LW_HANDLE vxTcbLock;

/*
 * vxWorks tcb pool lock
 */
#define VX_TCB_LOCK()   API_SemaphoreMPend(vxTcbLock, LW_OPTION_WAIT_INFINITE)
#define VX_TCB_UNLOCK() API_SemaphoreMPost(vxTcbLock)

/*
 * vxworks compatibility layer task library init
 */
void  vxWorksTaskInit (void)
{
    LW_HANDLE task = API_ThreadIdSelf();
    int  tid = HANDLE_TO_TID(task);

    vxTcbLock = API_SemaphoreMCreate("vxWorks taskLock", LW_PRIO_DEF_CEILING,
                                     LW_OPTION_INHERIT_PRIORITY | LW_OPTION_DELETE_SAFE,
                                     LW_NULL);

    vxTcb[tid].entry = (FUNCPTR)vxWorksTaskInit;
    vxTcb[tid].id    = tid;
    vxTcb[tid].task  = task;
    vxTcb[tid].event = API_EventSetCreate("vxWorks Event", 0ul, LW_OPTION_DEFAULT, LW_NULL);

    API_ThreadGetName(task, vxTcb[tid].name);
}

/*
 * task library init
 */
STATUS  taskLibInit (int usrDefExcStackSize,
                     int usrDefExcStackOverflowSize,
                     int kernelDefExcStackOverflowSize,
                     int usrDefExecStackOverflowSize,
                     int usrDefExecStackUnderflowSize,
                     int kernelDefExecStackOverflowSize,
                     int kernelDefExecStackUnderflowSize)
{
    vxusrDefExcStackSize              = usrDefExcStackSize;
    vxusrDefExcStackOverflowSize      = usrDefExcStackOverflowSize;
    vxkernelDefExcStackOverflowSize   = kernelDefExcStackOverflowSize;
    vxusrDefExecStackOverflowSize     = usrDefExecStackOverflowSize;
    vxusrDefExecStackUnderflowSize    = usrDefExecStackUnderflowSize;
    vxkernelDefExecStackOverflowSize  = kernelDefExecStackOverflowSize;
    vxkernelDefExecStackUnderflowSize = kernelDefExecStackUnderflowSize;

    return  (OK);
}

/*
 * task create library init
 */
void  taskCreateLibInit (void)
{
}

/*
 * task open init
 */
void  taskOpenInit (void)
{
}

/*
 * task show init
 */
void taskShowInit (void)
{
}

/*
 * vxworks task wrapper
 */
LW_LIB_HOOK_STATIC void *taskWrapper (void *unuse)
{
    WIND_TCB *pTcb = &vxTcb[HANDLE_TO_TID(API_ThreadIdSelf())];

    if (pTcb->entry) {
        return  ((void *)pTcb->entry(pTcb->arg[0], pTcb->arg[1], pTcb->arg[2],
                         pTcb->arg[3], pTcb->arg[4], pTcb->arg[5], pTcb->arg[6],
                         pTcb->arg[7], pTcb->arg[8], pTcb->arg[9]));
    } else {
        return  ((void *)ERROR);
    }
}

/*
 * clear tcb
 */
static void taskClrTcb (int tid)
{
    LW_HANDLE   event;

    VX_TCB_LOCK();
    event = vxTcb[tid].event;
    vxTcb[tid].entry = LW_NULL;
    vxTcb[tid].id    = ERROR;
    vxTcb[tid].task  = LW_OBJECT_HANDLE_INVALID;
    vxTcb[tid].event = LW_OBJECT_HANDLE_INVALID;
    vxTcb[tid].name[0] = EOS;
    VX_TCB_UNLOCK();

    API_EventSetDelete(&event);
}

/*
 * task current
 */
int taskIdSelf (void)
{
    LW_HANDLE self = API_ThreadIdSelf();

    if (self) {
        return  (int)API_ObjectGetIndex(self);
    } else {
        return  (ERROR);
    }
}

/*
 * init a task
 */
STATUS  taskInit (WIND_TCB *pTcb, char *name, int priority,
                  int options, char *pStackBase, int stackSize,
                  FUNCPTR entryPt, int arg1, int arg2, int arg3,
                  int arg4, int arg5, int arg6, int arg7,
                  int arg8, int arg9, int arg10)
{
    LW_CLASS_THREADATTR  attr = API_ThreadAttrGetDefault();

    if (!pTcb || !name || !entryPt) {
        errno = EINVAL;
        return  (ERROR);
    }

    attr.THREADATTR_pstkLowAddr     = (PLW_STACK)pStackBase;
    attr.THREADATTR_pvArg           = LW_NULL;
    attr.THREADATTR_stStackByteSize = (size_t)stackSize;
    attr.THREADATTR_ucPriority      = (UINT8)priority;
    attr.THREADATTR_ulOption        = LW_OPTION_THREAD_STK_CHK;

    pTcb->entry  = entryPt;
    pTcb->arg[0] = arg1;
    pTcb->arg[1] = arg2;
    pTcb->arg[2] = arg3;
    pTcb->arg[3] = arg4;
    pTcb->arg[4] = arg5;
    pTcb->arg[5] = arg6;
    pTcb->arg[6] = arg7;
    pTcb->arg[7] = arg8;
    pTcb->arg[8] = arg9;
    pTcb->arg[9] = arg10;

    pTcb->event = API_EventSetCreate("vxWorks Event", 0ul, LW_OPTION_DEFAULT, LW_NULL);
    if (pTcb->event == LW_OBJECT_HANDLE_INVALID) {
        return  (ERROR);
    }

    pTcb->task = API_ThreadInit(name, taskWrapper, &attr, LW_NULL);
    if (pTcb->task == LW_OBJECT_HANDLE_INVALID) {
        API_EventSetDelete(&pTcb->event);
        return  (ERROR);
    }

    pTcb->id = HANDLE_TO_TID(pTcb->task);

    VX_TCB_LOCK();
    vxTcb[pTcb->id] = *pTcb;
    lib_strlcpy(vxTcb[pTcb->id].name, name, VX_TASK_NAME_LENGTH + 1);
    VX_TCB_UNLOCK();

    return  (OK);
}

/*
 * create and run a task
 */
int  taskSpawn (char *name, int priority, int options,
                int stackSize, FUNCPTR entryPt, int arg1,
                int arg2, int arg3, int arg4, int arg5,
                int arg6, int arg7, int arg8, int arg9,
                int arg10)
{
    WIND_TCB  tcb;
    STATUS    ret;

    ret = taskInit(&tcb, name, priority, options, LW_NULL, stackSize,
                   entryPt, arg1, arg2, arg3, arg4, arg5, arg6, arg7,
                   arg8, arg9, arg10);
    if (ret != OK) {
        return  (ERROR);
    }

    API_ThreadStart(tcb.task);

    return  (tcb.id);
}

/*
 * create a task
 */
int  taskCreate (char *name, int priority, int options,
                 int stackSize, FUNCPTR entryPt, int arg1,
                 int arg2, int arg3, int arg4, int arg5,
                 int arg6, int arg7, int arg8, int arg9,
                 int arg10)
{
    WIND_TCB  tcb;
    STATUS    ret;

    ret = taskInit(&tcb, name, priority, options, LW_NULL, stackSize,
                   entryPt, arg1, arg2, arg3, arg4, arg5, arg6, arg7,
                   arg8, arg9, arg10);
    if (ret != OK) {
        return  (ERROR);
    }

    return  (tcb.id);
}

/*
 * active a task
 */
STATUS  taskActivate (int tid)
{
    LW_HANDLE   task;

    TID_FIX(tid);

    task = TID_TO_HANDLE(tid);

    if (API_ThreadStart(task)) {
        return  (ERROR);
    } else {
        return  (OK);
    }
}

/*
 * delete a task
 */
STATUS  taskDelete (int tid)
{
    LW_HANDLE   task;

    TID_FIX(tid);

    task = TID_TO_HANDLE(tid);

    if (API_ThreadDelete(&task, LW_NULL)) {
        return  (ERROR);

    } else {
        taskClrTcb(tid);
        return  (OK);
    }
}

/*
 * force delete a task
 */
STATUS  taskDeleteForce (int tid)
{
    LW_HANDLE   task;

    TID_FIX(tid);

    task = TID_TO_HANDLE(tid);

    if (API_ThreadForceDelete(&task, LW_NULL)) {
        return  (ERROR);

    } else {
        taskClrTcb(tid);
        return  (OK);
    }
}

/*
 * set a task option
 */
STATUS  taskOptionsSet (int tid, int mask, int newOptions)
{
    return  (OK);
}

/*
 * get task tcb
 */
WIND_TCB *taskTcb (int tid)
{
    TID_FIX(tid);

    if (tid > 0) {
        return  (&vxTcb[tid]);
    } else {
        return  (LW_NULL);
    }
}

/*
 * get task id list
 */
int  taskIdListGet (int idList[], int maxTasks)
{
    int i, cnt = 0;

    VX_TCB_LOCK();
    for (i = 0; i < LW_CFG_MAX_THREADS; i++) {
        if (cnt >= maxTasks) {
            break;
        }
        if (vxTcb[i].task) {
            idList[cnt] = vxTcb[i].id;
            cnt++;
        }
    }
    VX_TCB_UNLOCK();

    return  (cnt);
}

/*
 * get task prio
 */
STATUS  taskPriNormalGet (int tid, int *priNormal)
{
    LW_HANDLE   task;
    UINT8       prio;

    if (!priNormal) {
        errno = EINVAL;
        return  (ERROR);
    }

    TID_FIX(tid);

    task = TID_TO_HANDLE(tid);

    if (API_ThreadGetPriority(task, &prio)) {
        return  (ERROR);
    }

    *priNormal = (int)prio;

    return  (OK);
}

/*
 * get task status string
 */
STATUS  taskStatusString (int tid, char *pString)
{
    size_t      len;
    LW_HANDLE   task;
    LW_CLASS_TCB_DESC  tcbdesc;

    if (!pString) {
        errno = EINVAL;
        return  (ERROR);
    }

    TID_FIX(tid);

    task = TID_TO_HANDLE(tid);

    if (API_ThreadDesc(task, &tcbdesc)) {
        return  (ERROR);
    }

    *pString = EOS;
    if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_SEM) {
        lib_strcat(pString, "SEM ");
    }
    if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_MSGQUEUE) {
        lib_strcat(pString, "MQUEUE ");
    }
    if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_SUSPEND) {
        lib_strcat(pString, "SUSP ");
    }
    if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_EVENTSET) {
        lib_strcat(pString, "ENTS ");
    }
    if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_SIGNAL) {
        lib_strcat(pString, "WSIG ");
    }
    if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_INIT) {
        lib_strcat(pString, "INIT ");
    }
    if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_WDEATH) {
        lib_strcat(pString, "WDEA ");
    }
    if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_DELAY) {
        lib_strcat(pString, "SLP ");
    }
    if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_STOP) {
        lib_strcat(pString, "STOP ");
    }
    if (tcbdesc.TCBD_usStatus == 0ul) {
        lib_strcpy(pString, "RDY ");
    }

    len = lib_strlen(pString);
    if (pString[len - 1] == ' ') {
        pString[len - 1] = EOS;
    }

    return  (OK);
}

/*
 * get task option string
 */
STATUS  taskOptionsString (int tid, char *pString)
{
    LW_HANDLE   task;
    LW_CLASS_TCB_DESC  tcbdesc;

    if (!pString) {
        errno = EINVAL;
        return  (ERROR);
    }

    TID_FIX(tid);

    task = TID_TO_HANDLE(tid);

    if (API_ThreadDesc(task, &tcbdesc)) {
        return  (ERROR);
    }

    if (tcbdesc.TCBD_ulOption & LW_OPTION_THREAD_USED_FP) {
        lib_strcpy(pString, "VX_FP_TASK");
    } else {
        *pString = EOS;
    }

    return  (OK);
}

/*
 * show a task
 */
STATUS  taskShow (int tid, int level)
{
    pid_t pid = getpid();

    if (pid > 0) {
        API_ThreadShowEx(pid);
    }

    return  (OK);
}

/*
 * lock a task Warning: SMP system do not use this api
 */
STATUS  taskLock (void)
{
    API_ThreadLock();
    return  (OK);
}

/*
 * same as taskLock()
 */
STATUS  taskCpuLock (void)
{
    API_ThreadLock();
    return  (OK);
}

/*
 * unlock a task Warning: SMP system do not use this api
 */
STATUS  taskUnlock (void)
{
    API_ThreadUnlock();
    return  (OK);
}

/*
 * same as taskUnlock()
 */
STATUS  taskCpuUnlock (void)
{
    API_ThreadUnlock();
    return  (OK);
}

/*
 * suspend a task
 */
STATUS  taskSuspend (int tid)
{
    LW_HANDLE   task;

    TID_FIX(tid);

    task = TID_TO_HANDLE(tid);

    if (API_ThreadSuspend(task)) {
        return  (ERROR);
    } else {
        return  (OK);
    }
}

/*
 * resume a task
 */
STATUS  taskResume (int tid)
{
    LW_HANDLE   task;

    TID_FIX(tid);

    task = TID_TO_HANDLE(tid);

    if (API_ThreadResume(task)) {
        return  (ERROR);
    } else {
        return  (OK);
    }
}

/*
 * restart a task
 */
STATUS  taskRestart (int tid)
{
    LW_HANDLE   task;

    TID_FIX(tid);

    task = TID_TO_HANDLE(tid);

    if (API_ThreadRestart(task, LW_NULL)) {
        return  (ERROR);
    } else {
        return  (OK);
    }
}

/*
 * set task prio
 */
STATUS  taskPrioritySet (int tid, int newPriority)
{
    LW_HANDLE   task;

    if ((newPriority < VX_TASK_PRIORITY_MIN) ||
        (newPriority > VX_TASK_PRIORITY_MAX)) {
        errno = S_taskLib_ILLEGAL_PRIORITY;
        return  (ERROR);
    }

    TID_FIX(tid);

    task = TID_TO_HANDLE(tid);

    if (API_ThreadSetPriority(task, (UINT8)newPriority)) {
        return  (ERROR);
    } else {
        return  (OK);
    }
}

/*
 * get task prio
 */
STATUS  taskPriorityGet (int tid, int *pPriority)
{
    LW_HANDLE   task;
    UINT8       prio;

    if (!pPriority) {
        errno = EINVAL;
        return  (ERROR);
    }

    TID_FIX(tid);

    task = TID_TO_HANDLE(tid);

    if (API_ThreadGetPriority(task, &prio)) {
        return  (ERROR);
    }

    *pPriority = (int)prio;
    return  (OK);
}

/*
 * get stack size
 */
STATUS  taskStackSizeGet (int tid, int *pStackSize)
{
    LW_HANDLE   task;
    LW_CLASS_TCB_DESC  tcbdesc;

    if (!pStackSize) {
        errno = EINVAL;
        return  (ERROR);
    }

    TID_FIX(tid);

    task = TID_TO_HANDLE(tid);

    if (API_ThreadDesc(task, &tcbdesc)) {
        return  (ERROR);
    }

    *pStackSize = tcbdesc.TCBD_stStackSize;
    return  (OK);
}

/*
 * get task name size
 */
char *taskName (int tid)
{
    LW_HANDLE   task;

    TID_FIX(tid);

    task = TID_TO_HANDLE(tid);

    if (API_ThreadVerify(task)) {
        return  (vxTcb[tid].name);
    } else {
        return  (LW_NULL);
    }
}

/*
 * get task id by name
 */
int  taskNameToId (char *name)
{
    int tid;

    if (!name) {
        errno = EINVAL;
        return  (ERROR);
    }

    VX_TCB_LOCK();
    for (tid = 1; tid < LW_CFG_MAX_THREADS; tid++) {
        if (lib_strcmp(vxTcb[tid].name, name) == 0) {
            break;
        }
    }
    VX_TCB_UNLOCK();

    if (tid < LW_CFG_MAX_THREADS) {
        return  (tid);
    } else {
        return  (ERROR);
    }
}

/*
 * check a task is valid
 */
STATUS  taskIdVerify (int tid)
{
    LW_HANDLE   task;

    TID_FIX(tid);

    task = TID_TO_HANDLE(tid);

    if (API_ThreadVerify(task)) {
        return  (OK);
    } else {
        return  (ERROR);
    }
}

/*
 * set default task id
 */
int  taskIdDefault (int tid /* user supplied task ID; if 0, return default */)
{
    static int defaultTaskId;   /* current default task ID */

    if (tid != 0) {
        defaultTaskId = tid;        /* update default */
    }

    return  (defaultTaskId);
}

/*
 * check task is ready
 */
BOOL  taskIsReady (int tid)
{
    LW_HANDLE   task;

    TID_FIX(tid);

    task = TID_TO_HANDLE(tid);

    if (API_ThreadIsReady(task)) {
        return  (TRUE);
    } else {
        return  (FALSE);
    }
}

/*
 * check task is suspended
 */
BOOL  taskIsSuspended (int tid)
{
    LW_HANDLE   task;

    TID_FIX(tid);

    task = TID_TO_HANDLE(tid);

    if (API_ThreadIsSuspend(task)) {
        return  (TRUE);
    } else {
        return  (FALSE);
    }
}

/*
 * check task is pend
 */
BOOL  taskIsPended (int tid)
{
    LW_HANDLE   task;
    LW_CLASS_TCB_DESC  tcbdesc;

    TID_FIX(tid);

    task = TID_TO_HANDLE(tid);

    if (API_ThreadDesc(task, &tcbdesc)) {
        return  (FALSE);
    }

    if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_PEND_ANY) {
        return  (TRUE);
    } else {
        return  (FALSE);
    }
}

/*
 * check task is delayed
 */
BOOL  taskIsDelayed (int tid, int *pDelayTicks)
{
    LW_HANDLE   task;
    LW_CLASS_TCB_DESC  tcbdesc;

    TID_FIX(tid);

    task = TID_TO_HANDLE(tid);

    if (API_ThreadDesc(task, &tcbdesc)) {
        return  (FALSE);
    }

    if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_DELAY) {
        if (pDelayTicks) {
            *pDelayTicks = (int)tcbdesc.TCBD_ulWakeupLeft;
        }
        return  (TRUE);
    } else {
        return  (FALSE);
    }
}

/*
 * get task infomation
 */
STATUS  taskInfoGet (int tid, TASK_DESC *pTaskDesc)
{
    LW_HANDLE   task;
    size_t      stack = 0;
    LW_CLASS_TCB_DESC  tcbdesc;

    if (!pTaskDesc) {
        errno = EINVAL;
        return  (ERROR);
    }

    TID_FIX(tid);

    task = TID_TO_HANDLE(tid);

    if (API_ThreadDesc(task, &tcbdesc)) {
        return  (ERROR);
    }

    API_ThreadStackCheck(task, LW_NULL, &stack, LW_NULL);

    pTaskDesc->td_id           = tid;                       /* task id */
    pTaskDesc->td_priority     = (int)tcbdesc.TCBD_ucPriority;   /* task priority */
    pTaskDesc->td_entry        = vxTcb[tid].entry;          /* original entry point of task */
    pTaskDesc->td_sp           = (char *)tcbdesc.TCBD_pstkStackNow; /* saved stack pointer */
    pTaskDesc->td_pStackBase   = (char *)tcbdesc.TCBD_pstkStackLowAddr; /* the bottom of the stack */
    pTaskDesc->td_pStackEnd    = (char *)tcbdesc.TCBD_pstkStackBottom;  /* the actual end of the stack */
    pTaskDesc->td_stackSize    = (int)tcbdesc.TCBD_stStackSize;      /* size of stack in bytes */
    pTaskDesc->td_stackCurrent = (int)stack; /* current stack usage in bytes */
    pTaskDesc->td_stackHigh    = (int)stack; /* maximum stack usage in bytes */
    pTaskDesc->td_stackMargin  = (int)stack; /* current stack margin in bytes */
    pTaskDesc->td_errorStatus  = (int)tcbdesc.TCBD_ulLastError;  /* most recent task error status */
    pTaskDesc->td_delay        = (int)tcbdesc.TCBD_ulWakeupLeft; /* delay/timeout ticks */
    pTaskDesc->td_rtpId        = getpid();       /* RTP owning the task */
    pTaskDesc->td_cpuIndex     = tcbdesc.TCBD_ulCPUId;
    lib_strlcpy(pTaskDesc->td_name, tcbdesc.TCBD_cThreadName, VX_TASK_NAME_LENGTH + 1);

    pTaskDesc->td_events.options  = 0;
    pTaskDesc->td_events.received = 0;
    pTaskDesc->td_events.wanted   = 0;

    if (tcbdesc.TCBD_ulOption & LW_OPTION_THREAD_USED_FP) {
        pTaskDesc->td_options = VX_FP_TASK;
    } else {
        pTaskDesc->td_options = 0;
    }

    pTaskDesc->td_status = WIND_READY;
    if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_SUSPEND) {
        pTaskDesc->td_status |= WIND_SUSPEND;
    }
    if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_PEND_ANY) {
        pTaskDesc->td_status |= WIND_PEND;
    }
    if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_DELAY) {
        pTaskDesc->td_status |= WIND_DELAY;
    }
    if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_WDEATH) {
        pTaskDesc->td_status |= WIND_DEAD;
    }
    if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_STOP) {
        pTaskDesc->td_status |= WIND_STOP;
    }
    if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_WSTAT) {
        pTaskDesc->td_status |= WIND_STATE_CHANGE;
    }

    return  (OK);
}

/*
 * get task option
 */
STATUS  taskOptionsGet (int tid, int *pOptions)
{
    LW_HANDLE   task;
    LW_CLASS_TCB_DESC  tcbdesc;

    if (!pOptions) {
        errno = EINVAL;
        return  (ERROR);
    }

    TID_FIX(tid);

    task = TID_TO_HANDLE(tid);

    if (API_ThreadDesc(task, &tcbdesc)) {
        return  (ERROR);
    }

    if (tcbdesc.TCBD_ulOption & LW_OPTION_THREAD_USED_FP) {
        *pOptions = VX_FP_TASK;
    } else {
        *pOptions = 0;
    }

    return  (OK);
}

/*
 * delay a task
 */
STATUS  taskDelay (int ticks)
{
    if (API_TimeSleepEx((ULONG)ticks, TRUE)) {
        return  (ERROR);
    } else {
        return  (OK);
    }
}

/*
 * make a task into safe mode
 */
STATUS  taskSafe (void)
{
    API_ThreadSafe();
    return  (OK);
}

/*
 * make a task into unsafe mode
 */
STATUS  taskUnsafe (void)
{
    API_ThreadUnsafe();
    return  (OK);
}

/*
 * make a task undelay
 */
STATUS  taskUndelay (int  tid)
{
    LW_HANDLE   task;

    TID_FIX(tid);

    task = TID_TO_HANDLE(tid);

    if (API_ThreadWakeup(task)) {
        return  (ERROR);
    }

    return  (OK);
}

#if LW_CFG_SMP_EN == 0

/*
 * task add a variable
 */
STATUS  taskVarAdd (int tid, int *pVar)
{
    LW_HANDLE   task;

    if (!pVar) {
        errno = EINVAL;
        return  (ERROR);
    }

    TID_FIX(tid);

    task = TID_TO_HANDLE(tid);

    if (API_ThreadVarAdd(task, (ULONG *)pVar)) {
        return  (ERROR);
    } else {
        return  (OK);
    }
}

/*
 * task delete a variable
 */
STATUS  taskVarDelete (int tid, int *pVar)
{
    LW_HANDLE   task;

    if (!pVar) {
        errno = EINVAL;
        return  (ERROR);
    }

    TID_FIX(tid);

    task = TID_TO_HANDLE(tid);

    if (API_ThreadVarDelete(task, (ULONG *)pVar)) {
        return  (ERROR);
    } else {
        return  (OK);
    }
}

/*
 * init task variable
 */
STATUS  taskVarInit (void)
{
    return  (OK);
}

/*
 * task variable set
 */
STATUS  taskVarSet (int tid, int *pVar, int value)
{
    LW_HANDLE   task;

    if (!pVar) {
        errno = EINVAL;
        return  (ERROR);
    }

    TID_FIX(tid);

    task = TID_TO_HANDLE(tid);

    if (API_ThreadVarSet(task, (ULONG *)pVar, (ULONG)value)) {
        return  (ERROR);
    } else {
        return  (OK);
    }
}

/*
 * task variable get
 */
int  taskVarGet (int tid, int *pVar)
{
    LW_HANDLE   task;

    if (!pVar) {
        errno = EINVAL;
        return  (ERROR);
    }

    TID_FIX(tid);

    task = TID_TO_HANDLE(tid);

    return  ((int)API_ThreadVarGet(task, (ULONG *)pVar));
}

#endif /* LW_CFG_SMP_EN == 0 */

/*
 * task pending show
 */
STATUS  taskWaitShow (int taskId, int level)
{
    pid_t pid = getpid();

    if (pid > 0) {
        API_ThreadPendShowEx(pid);
    }

    return  (OK);
}

/*
 * task set cpu affinity
 */
STATUS  taskCpuAffinitySet (int tid, cpuset_t newAffinity)
{
    LW_HANDLE   task;

    TID_FIX(tid);

    task = TID_TO_HANDLE(tid);

#if LW_CFG_SMP_EN > 0
    if (API_ThreadSetAffinity(task, sizeof(cpuset_t), &newAffinity)) {
        return  (ERROR);
    } else
#endif
    {
        return  (OK);
    }
    return  (OK);
}

/*
 * task get cpu affinity
 */
STATUS  taskCpuAffinityGet(int tid, cpuset_t* pAffinity)
{
    LW_HANDLE   task;

    if (!pAffinity) {
        errno = EINVAL;
        return  (ERROR);
    }

    TID_FIX(tid);

    task = TID_TO_HANDLE(tid);

    CPU_ZERO(pAffinity);

#if LW_CFG_SMP_EN > 0
    if (API_ThreadGetAffinity(task, sizeof(cpuset_t), pAffinity)) {
        return  (ERROR);
    } else
#endif
    {
        return  (OK);
    }
    return  (OK);
}

/*
 * end
 */
