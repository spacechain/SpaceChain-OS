/**
 * @file
 * CPU library header.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#ifndef __VXWORKS_VXCPULIB_H
#define __VXWORKS_VXCPULIB_H

#include "vxWorksCommon.h"
#include <sched.h>

/*
 * CPU set
 */
typedef cpu_set_t    cpuset_t;

#ifdef __cplusplus
extern "C" {
#endif

/* function declarations */

unsigned int vxCpuIndexGet(void);
cpuset_t     vxCpuEnabledGet(void);
unsigned int vxCpuConfiguredGet(void);

#ifdef __cplusplus
}
#endif

#endif /* __VXWORKS_VXCPULIB_H */
