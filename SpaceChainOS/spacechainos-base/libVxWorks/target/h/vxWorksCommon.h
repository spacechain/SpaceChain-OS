/**
 * @file
 * system common header.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#ifndef __VXWORKS_VXWORKSCOMMON_H
#define __VXWORKS_VXWORKSCOMMON_H

#include <SylixOS.h>

#if !defined(EOF) || (EOF != (-1))
#define EOF     (-1)
#endif

#define NONE    (-1)    /* for times when NULL won't do */
#define EOS     '\0'    /* C string terminator */

/* return status values */

typedef int     TASK_ID;
typedef int     STATUS;
#define OK      0
#define ERROR   (-1)

/* timeout defines */

#define NO_WAIT         0
#define WAIT_FOREVER    (-1)

/* modes - must match O_RDONLY/O_WRONLY/O_RDWR in ioLib.h! */

#define VX_READ     0
#define VX_WRITE    1

/* common macros */

#define MSB(x)  (((x) >> 8) & 0xff)     /* most signif byte of 2-byte integer */
#define LSB(x)  ((x) & 0xff)            /* least signif byte of 2-byte integer*/
#define MSW(x)  (((x) >> 16) & 0xffff)  /* most signif word of 2-word integer */
#define LSW(x)  ((x) & 0xffff)          /* least signif byte of 2-word integer*/

/* swap the MSW with the LSW of a 32 bit integer */
#define WORDSWAP(x) (MSW(x) | (LSW(x) << 16))

#define LLSB(x)     ((x) & 0xff)        /* 32bit word byte/word swap macros */
#define LNLSB(x)    (((x) >> 8) & 0xff)
#define LNMSB(x)    (((x) >> 16) & 0xff)
#define LMSB(x)     (((x) >> 24) & 0xff)
#define LONGSWAP(x) ((LLSB(x)  << 24) | \
                     (LNLSB(x) << 16) | \
                     (LNMSB(x) << 8)  | \
                     (LMSB(x)))

#define OFFSET(structure, member)       /* byte offset of member in structure*/\
        ((int) &(((structure *) 0) -> member))

#define MEMBER_SIZE(structure, member)  /* size of a member of a structure */\
        (sizeof (((structure *) 0) -> member))

#define NELEMENTS(array)                /* number of elements in an array */ \
        (sizeof (array) / sizeof ((array) [0]))

#define FOREVER     for (;;)

/* storage class specifier definitions */

#define FAST    register
#define IMPORT  extern
#define LOCAL   static

#ifdef __cplusplus
extern "C" {
#endif

extern size_t allocAlignSize;
extern size_t stackAlignSize;
extern size_t cacheAlignSize;
extern size_t vmPageSize;

#ifdef __cplusplus
}
#endif

#ifdef _ALLOC_ALIGN_SIZE
#undef _ALLOC_ALIGN_SIZE
#endif /* _ALLOC_ALIGN_SIZE */
#define _ALLOC_ALIGN_SIZE       allocAlignSize

#ifdef _STACK_ALIGN_SIZE
#undef _STACK_ALIGN_SIZE
#endif /* _STACK_ALIGN_SIZE */
#define _STACK_ALIGN_SIZE       stackAlignSize

#ifdef _CACHE_ALIGN_SIZE
#undef _CACHE_ALIGN_SIZE
#endif /* _CACHE_ALIGN_SIZE */
#define _CACHE_ALIGN_SIZE       cacheAlignSize

#define VM_PAGE_SIZE_GET()      vmPageSize

#ifndef ROUND_UP
#define ROUND_UP(x, align)      (size_t)(((size_t)(x) +  (align - 1)) & ~(align - 1))
#endif

#ifndef ROUND_DOWN
#define ROUND_DOWN(x, align)    (size_t)( (size_t)(x) & ~(align - 1))
#endif

#ifndef ALIGNED
#define ALIGNED(x, align)       (((size_t)(x) & (align - 1)) == 0)
#endif

#define MEM_ROUND_UP(x)     ROUND_UP(x, _ALLOC_ALIGN_SIZE)
#define MEM_ROUND_DOWN(x)   ROUND_DOWN(x, _ALLOC_ALIGN_SIZE)
#define STACK_ROUND_UP(x)   ROUND_UP(x, _STACK_ALIGN_SIZE)
#define STACK_ROUND_DOWN(x) ROUND_DOWN(x, _STACK_ALIGN_SIZE)
#define MEM_ALIGNED(x)      ALIGNED(x, _ALLOC_ALIGN_SIZE)

/* default SMP max CPUs value, in case arch specific code did not define it */

#ifndef VX_MAX_SMP_CPUS
#define VX_MAX_SMP_CPUS   64
#endif

#if LW_CFG_CPU_WORD_LENGHT == 64
#define _WRS_CONFIG_LP64  1
#endif

#ifdef _WRS_KERNEL
#include "version.h"    /* kernel version numbers */
#endif

#ifdef __cplusplus
extern "C" {
#endif

STATUS  sysClkConnect(FUNCPTR routine, int arg);
void    sysClkDisable(void);
void    sysClkEnable(void);
int     sysClkRateGet(void);
STATUS  sysClkRateSet(int ticksPerSecond);

#ifdef __cplusplus
}
#endif

#endif /* __VXWORKS_VXWORKSCOMMON_H */
