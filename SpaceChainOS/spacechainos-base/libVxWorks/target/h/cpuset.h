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

#ifndef __VXWORKS_CPUSET_H
#define __VXWORKS_CPUSET_H

#include "vxWorksCommon.h"
#include "vxCpuLib.h"

#define CPUSET_SET(p, n)   LW_CPU_SET(n, p)
#define CPUSET_CLR(p, n)   LW_CPU_CLR(n, p)
#define CPUSET_ISSET(p, n) LW_CPU_ISSET(n, p)
#define CPUSET_ZERO(p)     LW_CPU_ZERO(p)

#endif /* __VXWORKS_CPUSET_H */
