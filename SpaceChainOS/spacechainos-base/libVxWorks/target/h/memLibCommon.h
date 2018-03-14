/**
 * @file
 * memory library.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#ifndef __VXWORKS_MEMLIBCOMMON_H
#define __VXWORKS_MEMLIBCOMMON_H

#include "vxWorksCommon.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef LW_HANDLE   PART_ID;

/* Partition statistics structure */

typedef struct {
    unsigned long numBytesFree;    /* Number of Free Bytes in Partition       */
    unsigned long numBlocksFree;   /* Number of Free Blocks in Partition      */
    unsigned long maxBlockSizeFree;/* Maximum block size that is free.        */
    unsigned long numBytesAlloc;   /* Number of Allocated Bytes in Partition  */
    unsigned long numBlocksAlloc;  /* Number of Allocated Blocks in Partition */
    unsigned long maxBytesAlloc;   /* Maximum number of Allocated Bytes at any*/
                                   /* time */
} MEM_PART_STATS;

#ifdef __cplusplus
}
#endif

#endif /* __VXWORKS_MEMLIBCOMMON_H */
