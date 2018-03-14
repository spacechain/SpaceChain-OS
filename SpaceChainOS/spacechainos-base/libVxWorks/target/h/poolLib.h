/**
 * @file
 * pool memory tree library.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#ifndef __VXWORKS_POOLLIB_H
#define __VXWORKS_POOLLIB_H

#include "vxWorks.h"
#include "dllLib.h"
#include "memLib.h"
#include "semLib.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pool_block {
    DL_NODE blockNode;  /* block node for pool */
    void   *pItmBlk;    /* ptr to start of item pool block */
    ULONG   itemCnt;    /* count of items in this block */
} POOL_BLOCK;

typedef struct pool {           /* Pool structure */
    DL_NODE     poolNode;       /* pool list for pond */
    const char *poolName;       /* pool name */
    SEM_ID      mutex;          /* Mutex to use for thread-safe pools */
    size_t      sizItem;        /* individual item size in bytes */
    size_t      sizItmReal;     /* actual item size, due to alignment */
    size_t      alignment;      /* individual item alignment in bytes */
    ULONG       numIncr;        /* nr. of incrementally allocated itm */
    ULONG       numTotl;        /* total number of items created */
    PART_ID     partId;         /* memory partition ID */
    ULONG       options;        /* pool options word */
    DL_LIST     blockList;      /* allocated block list */
    DL_LIST     freeItems;      /* free item list */
    ULONG       freeCount;      /* length of freeItems list */
    POOL_BLOCK  staticBlock;    /* for the first static block */
} POOL;

/* status codes */

#define M_poolLib                       3550            /* k_error.h partition + 50 */

#define S_poolLib_ARG_NOT_VALID         (M_poolLib | 1) /* invalid input arg */
#define S_poolLib_INVALID_POOL_ID       (M_poolLib | 2) /* invalid pool ID */
#define S_poolLib_NOT_POOL_ITEM         (M_poolLib | 3) /* unrecognized item */
#define S_poolLib_UNUSED_ITEM           (M_poolLib | 4) /* unused item */
#define S_poolLib_POOL_IN_USE           (M_poolLib | 5) /* can't delete pool */
#define S_poolLib_STATIC_POOL_EMPTY     (M_poolLib | 6) /* empty static pool */
#define S_poolLib_INVALID_BLK_ADDR      (M_poolLib | 7) /* invalid block addr */
#define S_poolLib_BLOCK_TOO_SMALL       (M_poolLib | 8) /* block too small */
#define S_poolLib_INT_CTX_POOL_EMPTY    (M_poolLib | 9) /* pool empty in ISR */

/* Pool option bits */

#define POOL_THREAD_SAFE    1   /* enable multi-task protection */
#define POOL_CHECK_ITEM     2   /* enable item check on return */

/* Typedefs */

typedef struct pool *POOL_ID;

/* Public function prototypes */

void    poolShow(POOL_ID poolId, int level);
POOL_ID poolCreate(const char * pName, size_t itmSize, size_t alignemnt,
                   ULONG initCnt, ULONG incrCnt, PART_ID partId,
                   ULONG options);
STATUS  poolDelete(POOL_ID poolId, BOOL force);
void   *poolItemGet(POOL_ID poolID);
STATUS  poolItemReturn(POOL_ID poolID, void *pItem);
STATUS  poolUnusedBlocksFree(POOL_ID poolId);
ULONG   poolTotalCount(POOL_ID poolId);
ULONG   poolFreeCount(POOL_ID poolId);
STATUS  poolIncrementSet(POOL_ID poolId, ULONG incrCnt);
ULONG   poolIncrementGet(POOL_ID poolId);
ULONG   poolBlockAdd(POOL_ID poolId, void * pBlock, size_t size);
int     poolIdListGet(POOL_ID * poolIdList, int listSize);

#ifdef __cplusplus
}
#endif

#endif /* __VXWORKS_POOLLIB_H */
