/**
 * @file
 * CPU library.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#define  __SYLIXOS_KERNEL
#include <unistd.h>
#include "vxCpuLib.h"

/*
 * get current CPU index.
 */
unsigned int vxCpuIndexGet (void)
{
    INTREG  ireg;
    ULONG   id;

    API_InterLock(&ireg);
    id = LW_CPU_GET_CUR_ID();
    API_InterUnlock(ireg);

    return  ((unsigned int)id);
}

/*
 * get enable CPU index.
 */
cpuset_t vxCpuEnabledGet (void)
{
    static cpuset_t set;

#if LW_CFG_SMP_EN > 0
    int i;
    int max = (int)sysconf(_SC_NPROCESSORS_CONF);
#endif

    CPU_ZERO(&set);

#if LW_CFG_SMP_EN > 0
    for (i = 0; i < max; i++) {
        if (API_CpuIsUp(i)) {
            CPU_SET(i, &set);
        }
    }
#else
    CPU_SET(0, &set);
#endif

    return  (set);
}

/*
 * get CPU count.
 */
unsigned int vxCpuConfiguredGet (void)
{
    return  ((unsigned int)sysconf(_SC_NPROCESSORS_CONF));
}

/*
 * end
 */
