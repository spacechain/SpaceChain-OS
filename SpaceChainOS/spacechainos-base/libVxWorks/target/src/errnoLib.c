/**
 * @file
 * errno library.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#include "errnoLib.h"
#include "taskLib.h"

/*
 * set task errno
 */
STATUS  errnoOfTaskSet (int taskId, int errorValue)
{
    LW_HANDLE  task;

    TID_FIX(taskId);

    task = TID_TO_HANDLE(taskId);

    if (API_SetLastErrorEx(task, (ULONG)errorValue)) {
        return  (ERROR);
    } else {
        return  (OK);
    }
}

/*
 * set current task errno
 */
STATUS  errnoSet(int errorValue)
{
    API_SetLastError((ULONG)errorValue);

    return  (OK);
}

/*
 * get current task errno
 */
int  errnoGet (void)
{
    return  (errno);
}

/*
 * get task errno
 */
int  errnoOfTaskGet (int taskId)
{
    LW_HANDLE  task;
    ULONG      ulError;

    TID_FIX(taskId);

    task = TID_TO_HANDLE(taskId);

    if (API_GetLastErrorEx(task, &ulError)) {
        return  (ERROR);
    } else {
        return  ((int)ulError);
    }
}

/*
 * end
 */
