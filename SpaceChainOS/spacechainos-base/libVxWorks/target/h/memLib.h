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

#ifndef __VXWORKS_MEMLIB_H
#define __VXWORKS_MEMLIB_H

#include "memLibCommon.h"
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* status codes */

#define S_memLib_NOT_ENOUGH_MEMORY          ERROR_REGION_NOMEM
#define S_memLib_INVALID_NBYTES             ERROR_REGION_SIZE
#define S_memLib_BLOCK_ERROR                ERROR_REGION_USED
#define S_memLib_NO_PARTITION_DESTROY       ERROR_REGION_FULL
#define S_memLib_PAGE_SIZE_UNAVAILABLE      ERROR_REGION_SIZE
#define S_memLib_WRONG_PART_ID              ERROR_REGION_NULL
#define S_memLib_INVALID_ADDRESS            ERROR_REGION_NULL
#define S_memLib_INVALID_ALIGNMENT          ERROR_REGION_ALIGN
#define S_memLib_FUNC_NOT_AVAILABLE         ENOSYS


/* partition options */

/* optional check for bad blocks */

#define MEM_BLOCK_CHECK     0x10

/* function declarations */

extern PART_ID  memPartCreate(char * pPool, size_t poolSize);
extern STATUS   memPartDelete(PART_ID partId);
extern STATUS   memPartInit(PART_ID partId, char * pPool,
                            size_t poolSize);
extern STATUS   memPartAddToPool(PART_ID partId, char * pPool,
                                 size_t  poolSize);
extern STATUS   memAddToPool(char * pPool, size_t poolSize);
extern void    *memPartAlloc(PART_ID partId, size_t nBytes);
extern void    *memPartAlignedAlloc(PART_ID partId, size_t nBytes,
                                    size_t alignment);
extern void    *memalign(size_t alignment, size_t size);
extern void    *valloc(size_t size);
extern STATUS   memPartFree(PART_ID partId, char * pBlock);
extern STATUS   memPartOptionsSet(PART_ID partId, unsigned options);
extern STATUS   memPartOptionsGet(PART_ID partId, UINT * pOptions);
extern ssize_t  memFindMax(void);
extern ssize_t  memPartFindMax(PART_ID partId);
extern void    *memPartRealloc(PART_ID partId, char * pBlock,
                               size_t nBytes);
extern STATUS   memOptionsGet(UINT * pOptions);
extern STATUS   memOptionsSet(unsigned options);
extern STATUS   cfree(char * pBlock);
extern void     memShowInit(void);
extern STATUS   memShow(int type);
extern STATUS   memPartShow(PART_ID partId, int type);
extern STATUS   memPartInfoGet(PART_ID partId,
                               MEM_PART_STATS * ppartStats);
extern STATUS   memInfoGet(MEM_PART_STATS * ppartStats);

#ifdef _WRS_CONFIG_LP64
extern void    *kProxHeapAlloc(size_t size);
extern STATUS   kProxHeapFree(void * ptr);
extern void    *kProxHeapAlignedAlloc(size_t size, size_t alignment);
extern void    *kProxHeapRealloc(void * pNewBlock, size_t newSize);
#endif /* _WRS_CONFIG_LP64 */

#ifdef __cplusplus
}
#endif

#endif /* __VXWORKS_MEMLIB_H */
