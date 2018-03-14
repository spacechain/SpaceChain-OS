/**
 * @file
 * signal library.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#ifndef __VXWORKS_SIGLIB_H
#define __VXWORKS_SIGLIB_H

#include "vxWorksCommon.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NUM_SIGNALS 64

#define SIGIOT      SIGABRT
#define SIGCLD      SIGCHLD

struct sigcontext {
    ARCH_REG_CTX    sc_regs;
};

typedef struct sigvec       SIGVEC;
typedef struct sigcontext   SIGCONTEXT;

#ifdef __cplusplus
}
#endif

#endif /* __VXWORKS_SIGLIB_H */
