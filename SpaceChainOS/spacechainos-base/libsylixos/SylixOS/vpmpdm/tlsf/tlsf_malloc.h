/*
 * SylixOS(TM)  LW : long wing
 * Copyright All Rights Reserved
 *
 * MODULE PRIVATE DYNAMIC MEMORY IN VPROCESS PATCH
 * this file is support current module malloc/free use his own heap in a vprocess.
 * 
 * Author: Han.hui <sylixos@gmail.com>
 */

#ifndef __TLSF_MALLOC_H
#define __TLSF_MALLOC_H

#include "tlsf.h"

/*
 * tlsf lock
 */
#if LW_CFG_VP_TLSF_LOCK_TYPE == 0
void __vp_patch_lock(void);
void __vp_patch_unlock(void);

#define TLSF_LOCK_INIT()
#define TLSF_LOCK_DEINIT()
#define TLSF_LOCK()         __vp_patch_lock()
#define TLSF_UNLOCK()       __vp_patch_unlock()

#else
#include <pthread.h>

static pthread_spinlock_t   tlsflock;

#define TLSF_LOCK_INIT()    pthread_spin_init(&tlsflock, 0)
#define TLSF_LOCK_DEINIT()  pthread_spin_destroy(&tlsflock)
#define TLSF_LOCK()         pthread_spin_lock(&tlsflock)
#define TLSF_UNLOCK()       pthread_spin_unlock(&tlsflock)

#endif /* LW_CFG_VP_TLSF_LOCK_TYPE */

/*
 * tlsf handle
 */
#define TLSF_HANDLE(pheap)  ((pheap)->HEAP_pvStartAddress)

/*
 * update memory infomation
 */
#define VP_MEM_INFO(pheap)  \
        do {    \
            (pheap)->HEAP_stTotalByteSize = 0;  \
        } while (0)
        
/*
 * build a memory management
 */
#define VP_MEM_CTOR(pheap, mem, size)   \
        do {    \
            TLSF_HANDLE(pheap) = (tlsf_t)tlsf_create_with_pool(mem, size);  \
            TLSF_LOCK_INIT();   \
        } while (0)
        
/*
 * destory memory management
 */
#define VP_MEM_DTOR(pheap)  \
        do {    \
            tlsf_destroy((tlsf_t)TLSF_HANDLE(pheap));   \
            TLSF_LOCK_DEINIT(); \
        } while (0)

/*
 * add memory to management
 */
#if LW_CFG_VP_TLSF_LOCK_TYPE == 0
#define VP_MEM_ADD(pheap, mem, size) tlsf_add_pool((tlsf_t)TLSF_HANDLE(pheap), mem, size)
#else
#define VP_MEM_ADD(pheap, mem, size) \
		do {	\
			TLSF_LOCK();	\
			tlsf_add_pool((tlsf_t)TLSF_HANDLE(pheap), mem, size);	\
			TLSF_UNLOCK();	\
		} while (0)
#endif /* LW_CFG_VP_TLSF_LOCK_TYPE */

/*
 * allocate memory
 */
static LW_INLINE void *VP_MEM_ALLOC (PLW_CLASS_HEAP  pheap, size_t nbytes)
{
    REGISTER void *ret;
    
    TLSF_LOCK();
    ret = tlsf_malloc((tlsf_t)TLSF_HANDLE(pheap), nbytes);
    TLSF_UNLOCK();
    
    if (ret) {
        _HeapTraceAlloc(pheap, ret, nbytes, "mem alloc");
    }

    return  (ret);
}

/*
 * allocate memory align
 */
static LW_INLINE void *VP_MEM_ALLOC_ALIGN (PLW_CLASS_HEAP pheap, size_t nbytes, size_t align)
{
    REGISTER void *ret;
    
    TLSF_LOCK();
    ret = tlsf_memalign((tlsf_t)TLSF_HANDLE(pheap), align, nbytes);
    TLSF_UNLOCK();
    
    if (ret) {
        _HeapTraceAlloc(pheap, ret, nbytes, "mem align");
    }

    return  (ret);
}


/*
 * re-allocate memory
 */
static LW_INLINE void *VP_MEM_REALLOC (PLW_CLASS_HEAP pheap, void *ptr, size_t new_size, int do_check)
{
    REGISTER void *ret;
    
    TLSF_LOCK();
    ret = tlsf_realloc((tlsf_t)TLSF_HANDLE(pheap), ptr, new_size);
    TLSF_UNLOCK();
    
    if (ptr) {
        if (ptr != ret) {
            _HeapTraceFree(pheap, ptr);
            _HeapTraceAlloc(pheap, ret, new_size, "mem realloc");
        }
    } else {
        _HeapTraceAlloc(pheap, ret, new_size, "mem realloc");
    }

    return  (ret);
}

/*
 * free memory
 */
#define VP_MEM_FREE(pheap, ptr, do_check)   \
        do {    \
            _HeapTraceFree(pheap, ptr); \
            TLSF_LOCK();  \
            tlsf_free((tlsf_t)TLSF_HANDLE(pheap), ptr);  \
            TLSF_UNLOCK();  \
        } while (0)

#endif /* __TLSF_MALLOC_H */

/*
 * end
 */
