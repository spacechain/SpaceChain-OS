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

#include <stdio.h>
#include <stdlib.h>
#include "memLibCommon.h"

/*
 * create memory pool
 */
PART_ID  memPartCreate (char *pPool, size_t poolSize)
{
    if (!pPool || !poolSize) {
        errno = EINVAL;
        return  (LW_HANDLE_INVALID);
    }

    return  (API_RegionCreate("vxWorks mem", pPool, poolSize, LW_OPTION_DEFAULT, LW_NULL));
}

/*
 * delete memory pool
 */
STATUS   memPartDelete (PART_ID partId)
{
    ULONG   ulRet;

    if (partId == LW_HANDLE_INVALID) {
        errno = EINVAL;
        return  (ERROR);
    }

    ulRet = API_RegionDelete(&partId);
    if (ulRet) {
        return  (ERROR);
    }

    return  (OK);
}

/*
 * init memory pool
 */
STATUS   memPartInit (PART_ID partId, char *pPool, size_t poolSize)
{
    printf("Not support memPartInit() please use memPartCreate() instead.\n");
    errno = ENOSYS;
    return  (ERROR);
}

/*
 * Add memory to pool
 */
STATUS   memPartAddToPool (PART_ID partId, char *pPool, size_t  poolSize)
{
    if (!pPool || !poolSize) {
        errno = EINVAL;
        return  (ERROR);
    }

    if (API_RegionAddMem(partId, pPool, poolSize)) {
        return  (ERROR);
    }

    return  (OK);
}

/*
 * Add memory to system pool
 */
STATUS   memAddToPool (char *pPool, size_t poolSize)
{
    if (!pPool || !poolSize) {
        errno = EINVAL;
        return  (ERROR);
    }

    printf("Not support memAddToPool() add memory to system heap.\n");
    errno = ENOSYS;
    return  (ERROR);
}

/*
 * Allocate memory from pool
 */
void    *memPartAlloc (PART_ID partId, size_t nBytes)
{
    void  *ret;

    if (!nBytes) {
        errno = EINVAL;
        return  (LW_NULL);
    }

    ret = API_RegionGet(partId, nBytes);

    return  (ret);
}

/*
 * Allocate memory from pool with aligned specify.
 */
void    *memPartAlignedAlloc (PART_ID partId, size_t nBytes, size_t alignment)
{
    void  *ret;

    if (!nBytes || !alignment) {
        errno = EINVAL;
        return  (LW_NULL);
    }

    if (alignment & (alignment - 1)) {
        errno = ERROR_REGION_ALIGN;
        return  (LW_NULL);
    }

    ret = API_RegionGetAlign(partId, nBytes, alignment);

    return  (ret);
}

/*
 * Free memory to pool
 */
STATUS   memPartFree (PART_ID partId, char * pBlock)
{
    void  *ret;

    if (!pBlock) {
        errno = EINVAL;
        return  (ERROR);
    }

    ret = API_RegionPut(partId, pBlock);
    if (ret) {
        return  (ERROR);
    }

    return  (OK);

}

/*
 * Set options to memory pool
 */
STATUS   memPartOptionsSet (PART_ID partId, unsigned options)
{
    errno = ENOSYS;
    return  (ERROR);
}

/*
 * Get options from memory pool
 */
STATUS   memPartOptionsGet (PART_ID partId, UINT * pOptions)
{
    errno = ENOSYS;
    return  (ERROR);
}

/*
 * Find max segment size
 */
ssize_t  memFindMax (void)
{
    return  (LW_CFG_MB_SIZE);
}

/*
 * Find max segment size from memory pool
 */
ssize_t  memPartFindMax (PART_ID partId)
{
    size_t  max;

    if (API_RegionGetMax(partId, &max)) {
        return  ((ssize_t)ERROR);
    }

    return  ((ssize_t)max);
}

/*
 * Realloc from memory pool
 */
void    *memPartRealloc (PART_ID partId, char * pBlock, size_t nBytes)
{
    void  *ret;

    if (!nBytes) {
        errno = EINVAL;
        return  (LW_NULL);
    }

    ret = API_RegionReget(partId, pBlock, nBytes);

    return  (ret);
}

/*
 * Set options to memory pool
 */
STATUS   memOptionsSet (unsigned options)
{
    errno = ENOSYS;
    return  (ERROR);
}

/*
 * Get options from memory pool
 */
STATUS   memOptionsGet (UINT * pOptions)
{
    errno = ENOSYS;
    return  (ERROR);
}

/*
 * This function should never be used.  Use free(3) instead.
 */
STATUS   cfree (char * pBlock)
{
    printf("This function should never be used.  Use free(3) instead.\n");
    errno = ENOSYS;
    return  (ERROR);
}

/*
 * Show system memory init.
 */
void     memShowInit (void)
{
}

/*
 * Show system memory info.
 */
STATUS   memShow (int type)
{
    (VOID)type;
    return  (lib_system("free"));
}

/*
 * Show memory pool info.
 */
STATUS   memPartShow (PART_ID partId, int type)
{
    size_t  stByteSize;
    ULONG   ulSegmentCounter;
    size_t  stUsedByteSize;
    size_t  stFreeByteSize;
    size_t  stMaxUsedByteSize;

    (VOID)type;
    if (API_RegionStatus(partId, &stByteSize, &ulSegmentCounter,
                         &stUsedByteSize, &stFreeByteSize, &stMaxUsedByteSize)) {
        return  (ERROR);
    }

    printf("memory pool    : 0x%lx\n", partId);
    printf("  segment      : %lu\n",   ulSegmentCounter);
    printf("  total size   : 0x%zx\n", stByteSize);
    printf("  used  size   : 0x%zx\n", stUsedByteSize);
    printf("  max used size: 0x%zx\n", stMaxUsedByteSize);

    return  (OK);
}

/*
 * Get memory pool info
 */
STATUS   memPartInfoGet (PART_ID partId, MEM_PART_STATS *ppartStats)
{
    size_t  stByteSize;
    ULONG   ulSegmentCounter;
    size_t  stUsedByteSize;
    size_t  stFreeByteSize;
    size_t  stMaxUsedByteSize;

    if (!ppartStats) {
        errno = EINVAL;
        return  (ERROR);
    }

    if (API_RegionStatus(partId, &stByteSize, &ulSegmentCounter,
                         &stUsedByteSize, &stFreeByteSize, &stMaxUsedByteSize)) {
        return  (ERROR);
    }

    ppartStats->numBytesFree      = (unsigned long)stFreeByteSize;
    ppartStats->numBlocksFree     = (unsigned long)1;
    ppartStats->maxBlockSizeFree  = (unsigned long)stFreeByteSize;
    ppartStats->numBytesAlloc     = (unsigned long)stUsedByteSize;
    ppartStats->numBlocksAlloc    = (unsigned long)ulSegmentCounter;
    ppartStats->maxBytesAlloc     = (unsigned long)stMaxUsedByteSize;

    return  (OK);
}

/*
 * Get system memory info
 */
STATUS   memInfoGet (MEM_PART_STATS * ppartStats)
{
    size_t  stByteSize;
    ULONG   ulSegmentCounter;
    size_t  stUsedByteSize;
    size_t  stFreeByteSize;
    size_t  stMaxUsedByteSize;

    if (!ppartStats) {
        errno = EINVAL;
        return  (ERROR);
    }

    if (API_KernelHeapInfo(LW_OPTION_HEAP_SYSTEM, &stByteSize, &ulSegmentCounter,
                           &stUsedByteSize, &stFreeByteSize, &stMaxUsedByteSize)) {
        return  (ERROR);
    }

    ppartStats->numBytesFree      = (unsigned long)stFreeByteSize;
    ppartStats->numBlocksFree     = (unsigned long)1;
    ppartStats->maxBlockSizeFree  = (unsigned long)stFreeByteSize;
    ppartStats->numBytesAlloc     = (unsigned long)stUsedByteSize;
    ppartStats->numBlocksAlloc    = (unsigned long)ulSegmentCounter;
    ppartStats->maxBytesAlloc     = (unsigned long)stMaxUsedByteSize;

    return  (OK);
}

#ifdef _WRS_CONFIG_LP64
/*
 * memory allocate
 */
void    *kProxHeapAlloc (size_t size)
{
    return  (malloc(size));
}

/*
 * memory free
 */
STATUS   kProxHeapFree (void * ptr)
{
    if (!ptr) {
        errno = EINVAL;
        return  (ERROR);
    }

    free(ptr);

    return  (OK);
}

/*
 * allocate aligned
 */
void    *kProxHeapAlignedAlloc (size_t size, size_t alignment)
{
    return  (mallocalign(size, alignment));
}

/*
 * reallocate
 */
void    *kProxHeapRealloc (void * pNewBlock, size_t newSize)
{
    return  (realloc(pNewBlock, newSize));
}
#endif /* _WRS_CONFIG_LP64 */
/*
 * end
 */
