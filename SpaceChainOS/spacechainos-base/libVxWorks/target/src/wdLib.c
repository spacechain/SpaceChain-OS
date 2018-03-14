/**
 * @file
 * timer library functions.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#include "wdLib.h"

/*
 * vxWorks timeout to SylixOS
 */
static ULONG  vx2syWdTo (int timeout)
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
 * init watch dog lib
 */
STATUS  wdLibInit (void)
{
    return  (OK);
}

/*
 * init watch dog lib
 */
void  wdCreateLibInit (void)
{
}

/*
 * watch dog show init
 */
void  wdShowInit (void)
{
}

/*
 * create a watch dog
 */
WDOG_ID  wdCreate (void)
{
    WDOG_ID wd;

    wd = API_TimerCreate("vxWorks wd", LW_OPTION_HTIMER, LW_NULL);

    return  (wd);
}

/*
 * delete a watch dog
 */
STATUS  wdDelete (WDOG_ID wdId)
{
    if (API_TimerDelete(&wdId)) {
        return  (ERROR);
    } else {
        return  (OK);
    }
}

/*
 * start a watch dog
 */
STATUS  wdStart (WDOG_ID wdId, int delay, FUNCPTR pRoutine,
                 int parameter)
{
    ULONG  ulError;

    ulError = API_TimerStart(wdId, vx2syWdTo(delay), LW_OPTION_MANUAL_RESTART,
                            (PTIMER_CALLBACK_ROUTINE)pRoutine, (PVOID)parameter);
    if (ulError) {
        return  (ERROR);
    } else {
        return  (OK);
    }
}

/*
 * cancel a watch dog
 */
STATUS  wdCancel (WDOG_ID wdId)
{
    if (API_TimerCancel(wdId)) {
        return  (ERROR);
    } else {
        return  (OK);
    }
}

/*
 * show a watch dog
 */
STATUS  wdShow (WDOG_ID wdId)
{
    API_TimerShow(wdId);
    return  (OK);
}

/*
 * end
 */
