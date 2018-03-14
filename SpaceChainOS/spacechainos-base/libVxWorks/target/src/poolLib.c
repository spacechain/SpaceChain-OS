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

#include "poolLib.h"
#include <stdlib.h>

/* Private macros */

#define POOL_SEM_CREATE_ATTRIB \
    (SEM_Q_PRIORITY | SEM_INVERSION_SAFE | SEM_DELETE_SAFE)

#define POOL_SEM_TAKE_ATTRIB    WAIT_FOREVER

/* indicator of a block that is provided by user */

#define BLOCK_DO_NOT_FREE   0x00000001

#define POOL_LIST_LOCK()    semTake(poolListMutex, POOL_SEM_TAKE_ATTRIB)
#define POOL_LIST_UNLOCK()  semGive(poolListMutex)
#define POOL_LOCK(poolId)   semTake((poolId)->mutex, POOL_SEM_TAKE_ATTRIB)
#define POOL_UNLOCK(poolId) semGive((poolId)->mutex)

/* mem alloc*/

#define MEM_PART_ALLOC(partId, size)                \
        ((partId) ? memPartAlloc(partId, size) : malloc(size))
#define MEM_PART_ALLOC_ALIGN(partId, size, align)   \
        ((partId) ? memPartAlignedAlloc(partId, size, align) : memalign(align, size))
#define MEM_PART_FREE(partId, ptr)                  \
        ((partId) ? memPartFree(partId, ptr) : free(ptr))

/* private functions */

POOL_ID poolInitialize(char * pPool, const char * pName, size_t itmSize,
                       size_t alignemnt, ULONG initCnt, ULONG incrCnt,
                       PART_ID partId, ULONG options);
BOOL    poolFindItem(DL_LIST * pList, DL_NODE * pItem);

/* local variables */

LOCAL SEM_ID poolListMutex; /* Mutex used in pool list change */

/* globals */

DL_LIST poolListGbl = {NULL, NULL};  /* global pool list */

/* forward declarations */

LOCAL STATUS poolBlockBuild(POOL_ID poolId, ULONG itmSize, void * pBlock);
LOCAL BOOL   poolItemIsValid(POOL_ID poolId, void *  pItem);
LOCAL BOOL   poolBlockIsFree(POOL_ID poolId, POOL_BLOCK * pBlk);

/*
 * poolLibInit - initialize the poolLib.
 */
LW_CONSTRUCTOR_BEGIN
LW_LIB_HOOK_STATIC void poolLibInit (void)
{
    if (!poolListMutex) {
        poolListMutex = semMCreate(POOL_SEM_CREATE_ATTRIB);
    }
}
LW_CONSTRUCTOR_END(poolLibInit)

/*
 * poolCreate - create a pool
 */
POOL_ID poolCreate (const char *pName,     /* optional name to assign to pool */
                    size_t      itmSize,   /* size in bytes of a pool item */
                    size_t      alignment, /* alignment of a pool item */
                                           /* (must be power of 2, or 0) */
                    ULONG       initCnt,   /* initial number of items to put in pool */
                    ULONG       incrCnt,   /* min no of items to add to pool dynamically */
                                           /* (if 0, no pool expansion is done) */
                    PART_ID     partId,    /* memory partition ID */
                    ULONG       options)   /* initial options for pool */
{
    FAST POOL_ID poolId;
    char        *pPool;

    if (alignment == 0) {
        alignment = _ALLOC_ALIGN_SIZE;
    }

    /* Validate input args */

    if ((itmSize == 0) || ((alignment & (alignment - 1)) != 0)) {
        errno = S_poolLib_ARG_NOT_VALID;
        return  (NULL);
    }

    if ((pPool = malloc(sizeof(POOL))) == NULL) {
        return  (NULL);
    }

    poolId = poolInitialize(pPool, pName, itmSize, alignment,
                            initCnt, incrCnt, partId, options);

    if (poolId == NULL) {
        free(pPool);
    }

    return  (poolId);
}

/*
 * poolInitialize - initialize a pool
 */
POOL_ID poolInitialize (char       *pPool,      /* pool structure memory */
                        const char *pName,      /* optional name to assign to pool */
                        size_t      itmSize,    /* size in bytes of a pool item */
                        size_t      alignment,  /* alignment of a pool item */
                                                /* (must be power of 2, or 0) */
                        ULONG       initCnt,    /* initial number of items to put in pool */
                        ULONG       incrCnt,    /* min no of items to add to pool dynamically */
                                                /* (if 0, no pool expansion is done) */
                        PART_ID     partId,     /* memory partition ID */
                        ULONG       options)    /* initial options for pool */
{
    POOL_ID     poolId = (POOL_ID) pPool;
    POOL_BLOCK *pBlk;

    poolId->sizItem    = itmSize;   /* record item size */
    poolId->alignment  = alignment; /* record item alignment */
    poolId->options    = options;   /* pool options */
    poolId->numIncr    = incrCnt;   /* pool increment */
    poolId->poolName   = pName;     /* save name */
    poolId->numTotl    = 0;
    poolId->freeCount  = 0;

    /* actual item size must be multiple of alignment, larger than NODE size */

    if (itmSize < sizeof(DL_NODE)) {
        itmSize = sizeof(DL_NODE);
    }

    poolId->sizItmReal = ROUND_UP(itmSize, alignment);
    poolId->partId     = partId;

    /* initialize lists */

    dllInit(&poolId->freeItems);
    dllInit(&poolId->blockList);

    /* mark first static block unused */

    poolId->staticBlock.pItmBlk = NULL;

    /*
     * allocate initial block that has a POOL_BLOCK and a POOL structure
     * at the end.
     */

    if (initCnt != 0) {
        if (poolBlockBuild(poolId, initCnt, NULL) == ERROR) {
            return  (NULL);
        }
    }

    /* create a semaphore for this pool if needed */

    if ((poolId->options & POOL_THREAD_SAFE) == POOL_THREAD_SAFE) {
        poolId->mutex = semMCreate(POOL_SEM_CREATE_ATTRIB);
        if (!poolId->mutex) {
            goto    poolInit_2;
        }
    }

    /* if pool built successfully, add it to pond */

    if (POOL_LIST_LOCK() != OK) {
        goto    poolInit_1;
    }

    /* Add this pool to the pool list */

    dllAdd(&poolListGbl, &poolId->poolNode);

    /* unlock access */

    POOL_LIST_UNLOCK();

    return  (poolId);

    /* else, free allocated memory before returning NULL */

poolInit_1:
    if ((poolId->options & POOL_THREAD_SAFE) == POOL_THREAD_SAFE) {
        semDelete(poolId->mutex);
        poolId->mutex = LW_HANDLE_INVALID;
    }

poolInit_2:
    if ((pBlk = (POOL_BLOCK *)dllGet(&poolId->blockList)) != NULL) {
        MEM_PART_FREE(poolId->partId, pBlk->pItmBlk);
    }

    return  (NULL);
}

/*
 * poolDelete - delete a pool
 */
STATUS poolDelete (POOL_ID  poolId,    /* ID of pool to delete */
                   BOOL     force)     /* force deletion if there are items in use */
{
    STATUS      result = ERROR;
    POOL_BLOCK *pBlk;

    /* verify valid, unused pool ID */

    if (poolId == NULL) {
        errno = S_poolLib_INVALID_POOL_ID;
        return  (result);
    }

    /* Reserve exclusive access to this pool */

    if ((poolId->options & POOL_THREAD_SAFE) == POOL_THREAD_SAFE) {
        if (POOL_LOCK(poolId) == ERROR) {
            return  (result);   /* ERROR */
        }
    }

    if (!force && (poolId->freeCount != poolId->numTotl)) {
        errno = S_poolLib_POOL_IN_USE;

        if ((poolId->options & POOL_THREAD_SAFE) == POOL_THREAD_SAFE) {
            POOL_UNLOCK(poolId);
        }

        return  (result);
    }

    /* remove from global pool list */

    if (POOL_LIST_LOCK() != OK) {
        if ((poolId->options & POOL_THREAD_SAFE) == POOL_THREAD_SAFE) {
            POOL_UNLOCK(poolId);
        }
        return  (result);   /* ERROR */
    }

    dllRemove(&poolListGbl, &poolId->poolNode);

    POOL_LIST_UNLOCK();

    /* now clean up all allocated stuff */

    result = OK;

    /* free all blocks */

    while ((pBlk = (POOL_BLOCK *) dllGet (&poolId->blockList)) != NULL) {
        if (((ULONG) pBlk->pItmBlk & BLOCK_DO_NOT_FREE) == 0) {
            MEM_PART_FREE(poolId->partId, pBlk->pItmBlk);

        } else {
           if (pBlk != &poolId->staticBlock) {
               MEM_PART_FREE(poolId->partId, (char *)pBlk);
           }
        }
    }

    /* delete semaphore */

    if ((poolId->options & POOL_THREAD_SAFE) == POOL_THREAD_SAFE) {
        semDelete(poolId->mutex);
        poolId->mutex = LW_HANDLE_INVALID;
    }

    /* free the pool structure */

    free(poolId);

    return  (result);
}

/*
 * poolBlockAdd - add an item block to the pool
 */
ULONG poolBlockAdd (POOL_ID poolId, /* ID of pool to delete */
                    void   *pBlock, /* base address of block to add */
                    size_t  size)   /* size of block to add */
{
    void    *pBlockAligned;
    size_t   itmCnt;

    /* verify valid, unused pool ID */

    if (poolId == NULL) {
        errno = S_poolLib_INVALID_POOL_ID;
        return  (0);
    }

    if ((pBlock == NULL)) {
        errno = S_poolLib_INVALID_BLK_ADDR;
        return  (0);
    }

    /* do alignment and size adjustments */

    pBlockAligned = (void *)ROUND_UP(pBlock, poolId->alignment);
    itmCnt        = (size - ((char *) pBlockAligned - (char *) pBlock))
                  / poolId->sizItmReal;

    if (itmCnt < 1) {    /* add at least one item */
        errno = S_poolLib_BLOCK_TOO_SMALL;
        return (0);
    }

    if ((poolId->options & POOL_THREAD_SAFE) == POOL_THREAD_SAFE) {
        if (POOL_LOCK(poolId) == ERROR) {
            return  (0);
        }
    }

    if (poolBlockBuild(poolId, itmCnt, pBlockAligned) == ERROR) {
        itmCnt = 0;
    }

    if ((poolId->options & POOL_THREAD_SAFE) == POOL_THREAD_SAFE) {
        POOL_UNLOCK(poolId);
    }

    return  ((ULONG)itmCnt);
}

/*
 * poolUnusedBlocksFree -  free blocks that have all items unused
 */
STATUS poolUnusedBlocksFree (POOL_ID poolId) /* ID of pool to free blocks */
{
    POOL_BLOCK * pBlkDel;
    POOL_BLOCK * pBlk;

    /* verify valid pool ID */

    if (poolId == NULL) {
        errno = S_poolLib_INVALID_POOL_ID;
        return  (ERROR);
    }

    /* Reserve exclusive access to this pool */

    if ((poolId->options & POOL_THREAD_SAFE) == POOL_THREAD_SAFE) {
        if (POOL_LOCK(poolId) == ERROR) {
            return  (ERROR);
        }
    }

    /* find and remove any blocks that are all free */

    pBlk = (POOL_BLOCK *)DLL_FIRST(&poolId->blockList);

    while (pBlk != NULL) {
        pBlkDel = pBlk;
        pBlk    = (POOL_BLOCK *)DLL_NEXT(&pBlk->blockNode);

        if (poolBlockIsFree (poolId, pBlkDel) &&
            (((ULONG) pBlkDel->pItmBlk & BLOCK_DO_NOT_FREE) == 0)) {
            DL_NODE  *pItem;
            ULONG     ix;
            ULONG     actualItemSize = poolId->sizItmReal;

            pItem = (DL_NODE *)((ULONG)pBlkDel->pItmBlk &
                                (ULONG)~BLOCK_DO_NOT_FREE);

            /* delete all items from free list */

            for (ix = 0; ix < pBlkDel->itemCnt; ix++) {
                dllRemove(&poolId->freeItems, pItem);
                poolId->freeCount--;
                pItem = (DL_NODE *)((char *)pItem + actualItemSize);
            }

            poolId->numTotl -= pBlkDel->itemCnt;

            /* delete block from blockList */

            dllRemove(&poolId->blockList, &pBlkDel->blockNode);

            /* free memory associated to block */

            MEM_PART_FREE(poolId->partId, pBlkDel->pItmBlk);
        }
    }

    /* exit exlusive access section */

    if ((poolId->options & POOL_THREAD_SAFE) == POOL_THREAD_SAFE) {
        POOL_UNLOCK(poolId);
    }

    return  (OK);
}

/*
 * poolItemGet - get next free item from pool and return a pointer to it
 */
void *poolItemGet (POOL_ID poolId) /* ID of pool from which to get item */
{
    BOOL   cont  = TRUE;
    void  *pItem = NULL;

    /* verify valid pool ID */

    if (poolId == NULL) {
        errno = S_poolLib_INVALID_POOL_ID;
        return  (NULL);
    }

    /* Reserve exclusive access to this pool */

    if ((poolId->options & POOL_THREAD_SAFE) == POOL_THREAD_SAFE) {
        if (POOL_LOCK(poolId) == ERROR) {
            return  (NULL);
        }
    }

    if (DLL_FIRST(&poolId->freeItems) == NULL) {
        /*
         * Current block empty. Allocate another one if not fixed size and not
         * called from ISR
         */
        if (poolId->numIncr == 0) {
            errno = S_poolLib_STATIC_POOL_EMPTY;
            cont = FALSE;

        } else if (poolBlockBuild(poolId, poolId->numIncr, NULL) == ERROR) {
            /* keep errno from alloc error */
            cont = FALSE;
        }
    }

    if (cont == TRUE) {
        pItem = (void *)dllGet(&poolId->freeItems);
        poolId->freeCount--;
    }

    if ((poolId->options & POOL_THREAD_SAFE) == POOL_THREAD_SAFE) {
        POOL_UNLOCK(poolId);
    }

    return  (pItem);
}

/*
 * poolItemReturn - return an item to the pool
 */
STATUS poolItemReturn (POOL_ID poolId, /* ID of pool to which to return item */
                       void   *pItem)   /* pointer to item to return */
{
    STATUS  result = ERROR;
    BOOL    cont   = TRUE;

    /* verify valid pool ID */

    if (poolId == NULL){
        errno = S_poolLib_INVALID_POOL_ID;
        return  (result);
    }

    if (pItem == NULL) {
        errno = S_poolLib_NOT_POOL_ITEM;
        return  (result);
    }

    /* Reserve exclusive access to this pool */

    if ((poolId->options & POOL_THREAD_SAFE) == POOL_THREAD_SAFE) {
        if (POOL_LOCK (poolId) == ERROR) {
            return  (result);
        }
    }

    if ((poolId->options & POOL_CHECK_ITEM) == POOL_CHECK_ITEM) {
        /*
         * verify that pool belongs to one of the blocks and it is
         * not in free list.
         */
        if (poolFindItem(&poolId->freeItems, (DL_NODE *) pItem) == TRUE) {
            errno = S_poolLib_UNUSED_ITEM;
            cont  = FALSE;

        } else if (!poolItemIsValid(poolId, pItem)) {
            errno = S_poolLib_NOT_POOL_ITEM;
            cont = FALSE;
        }
    }

    if (cont) {
        dllAdd(&poolId->freeItems, (DL_NODE *)pItem);
        poolId->freeCount++;
        result = OK;
    }

    /* exit exlusive access section */

    if ((poolId->options & POOL_THREAD_SAFE) == POOL_THREAD_SAFE) {
        POOL_UNLOCK(poolId);
    }

    return  (result);
}

/*
 * poolIncrementSet - set the increment value used to grow the pool
 */
STATUS poolIncrementSet (POOL_ID poolId,  /* ID of pool */
                         ULONG   incrCnt) /* new increment value */
{
    /* verify valid pool ID */

    if (poolId == NULL) {
        errno = S_poolLib_INVALID_POOL_ID;
        return  (ERROR);
    }

    poolId->numIncr = incrCnt;

    return  (OK);
}

/*
 * poolIncrementGet - get the increment value used to grow the pool
 */
ULONG poolIncrementGet (POOL_ID poolId) /* ID of pool */
{
    /* verify valid pool ID */

    if (poolId == NULL) {
        errno = S_poolLib_INVALID_POOL_ID;
        return  (0);
    }

    return  (poolId->numIncr);
}

/*
 * poolTotalCount - return total number of items in pool
 */
ULONG poolTotalCount (POOL_ID poolId) /* ID of pool */
{
    if (poolId == NULL) {
        errno = S_poolLib_INVALID_POOL_ID;
        return  (0);
    }

    return  (poolId->numTotl);
}

/*
 * poolFreeCount - return number of free items in pool
 */
ULONG poolFreeCount (POOL_ID poolId) /* ID of pool */
{
    if (poolId == NULL) {
        errno = S_poolLib_INVALID_POOL_ID;
        return  (0);
    }

    return  (poolId->freeCount);
}

/*
 * poolIdListGet - get a list of pools in the system
 */
int poolIdListGet (POOL_ID poolIdList[],
                   int     listSize)
{
    POOL_ID poolId;
    int     count = 0;

    if (POOL_LIST_LOCK() != OK) {
        return  (ERROR);
    }

    poolId = (POOL_ID)DLL_FIRST(&poolListGbl);

    while ((count < listSize) && (poolId != NULL)) {
        poolIdList[count] = poolId;
        poolId = (POOL_ID)DLL_NEXT(&poolId->poolNode);
        count++;
    }

    POOL_LIST_UNLOCK();

    return  (count);
}

/*
 * poolBlockBuild - Add a block of items to a specified pool
 */
LOCAL STATUS poolBlockBuild (POOL_ID  poolId,    /* ID of pool to which to add block */
                             ULONG    itemCnt,   /* number of items in block */
                             void    *pUsrBlk)
{
    size_t      actualItemSize = poolId->sizItmReal;
    ULONG       noFree = 0;
    size_t      blockSize;
    POOL_BLOCK *pBlk;
    int         ix;

    if (pUsrBlk == NULL) {
        /* allocate user block and POOL_BLOCK in one chunk */

        blockSize = actualItemSize * itemCnt + sizeof (POOL_BLOCK);

        pUsrBlk = MEM_PART_ALLOC_ALIGN(poolId->partId, blockSize,
                                       poolId->alignment);

        if (pUsrBlk == NULL) {
            return  (ERROR);
        }

        /* POOL_BLOCK is at the end of the allocated chunk */

        pBlk = (POOL_BLOCK *)((char *)pUsrBlk + actualItemSize * itemCnt);

    } else {
        /*
         * first block with user-provided memory will use the embedded
         * block structure
         */

        if (poolId->staticBlock.pItmBlk == NULL) {
            pBlk = &poolId->staticBlock;

        } else {
            pBlk = MEM_PART_ALLOC(poolId->partId, sizeof (POOL_BLOCK));
            if (pBlk == NULL) {
                return  (ERROR);
            }
        }

        /* prevent freeing of this block */

        noFree = BLOCK_DO_NOT_FREE;
    }

    pBlk->pItmBlk = (void *) ((ULONG) pUsrBlk | noFree);
    pBlk->itemCnt = itemCnt;

    for (ix = 0; ix < itemCnt; ix++) {
        dllAdd(&poolId->freeItems, (DL_NODE *)pUsrBlk);
        poolId->freeCount++;
        pUsrBlk = (void *)((char *)pUsrBlk + actualItemSize);
    }

    dllAdd(&poolId->blockList, &pBlk->blockNode);

    poolId->numTotl += itemCnt;

    return  (OK);
}

/*
 * poolItemIsValid - check validity of an item
 */
LOCAL BOOL poolItemIsValid (POOL_ID  poolId,
                            void    *pItem)
{
    POOL_BLOCK * pBlk;
    size_t       actualItemSize = poolId->sizItmReal;

    /* find the block this item belongs to */

    pBlk = (POOL_BLOCK *)DLL_FIRST(&poolId->blockList);

    while (pBlk != NULL) {
        char * pItmBlk = (char *)((ULONG)pBlk->pItmBlk &
                                  (ULONG)~BLOCK_DO_NOT_FREE);

        if (((char *)pItem >= (char *)pItmBlk) &&
            ((char *)pItem < ((char *)pItmBlk + actualItemSize * pBlk->itemCnt))) {
            /* pointer within block limits */

            ULONG   offset = ((char *)pItem - pItmBlk);

            /* make sure offset is multiple of item size */

            if (((offset / actualItemSize) * actualItemSize) == offset) {
                return  (TRUE);

            } else {
                return  (FALSE);
            }
        }

        pBlk = (POOL_BLOCK *)DLL_NEXT(&pBlk->blockNode);
    }

    /* item not in pool */

    return  (FALSE);
}

/*
 * poolBlockIsFree - test if items in a block are free
 */
LOCAL BOOL poolBlockIsFree (POOL_ID      poolId,    /* pool the block belongs to */
                            POOL_BLOCK  *pBlk)      /* block to check */
{
    BOOL     blkFree = TRUE;
    size_t   actualItemSize = poolId->sizItmReal;
    void    *pItem;
    ULONG    ix;

    /* block is free if all items are in free list */

    pItem = (void *)((ULONG)pBlk->pItmBlk & (ULONG)~BLOCK_DO_NOT_FREE);

    for (ix = 0; ix < pBlk->itemCnt; ix++) {
        if (poolFindItem(&poolId->freeItems, (DL_NODE *)pItem) == FALSE) {
            blkFree = FALSE;
            break;
        }

        pItem = (void *)((char *)pItem + actualItemSize);
    }

    return  (blkFree);
}

/*
 * poolFindItem - support routine to find if item is in DL list.
 */
BOOL poolFindItem (DL_LIST *pList,
                   DL_NODE *pItem)
{
    FAST DL_NODE *pNode = DLL_FIRST(pList);

    while ((pNode != NULL) && (pNode != pItem)) {
        pNode = DLL_NEXT(pNode);
    }

    if (pNode == NULL) {
        return  (FALSE);

    } else {
        return  (TRUE);
    }
}

/*
 * end
 */
