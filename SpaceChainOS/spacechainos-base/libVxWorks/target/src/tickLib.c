/**
 * @file
 * tick library.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#define  __SYLIXOS_KERNEL
#include "tickLib.h"

/*
 * Announce a tick
 */
void tickAnnounce (void)
{
    API_KernelTicks();
}

/*
 * set tick
 */
void tickSet (ULONG ticks)
{
    errno = ENOSYS;
}

/*
 * get tick
 */
ULONG tickGet (void)
{
    return  (API_TimeGet());
}

/*
 * set tick 64bit
 */
void tick64Set(UINT64 ticks)
{
    errno = ENOSYS;
}

/*
 * get tick 64bit
 */
UINT64 tick64Get (void)
{
    return  ((UINT64)API_TimeGet64());
}

/*
 * add a tick hook
 */
STATUS tickAnnounceHookAdd (FUNCPTR pFunc)
{
    if (API_SystemHookAdd((LW_HOOK_FUNC)pFunc,
                          LW_OPTION_THREAD_TICK_HOOK)) {
        return  (ERROR);
    } else {
        return  (OK);
    }
}

/*
 * end
 */
