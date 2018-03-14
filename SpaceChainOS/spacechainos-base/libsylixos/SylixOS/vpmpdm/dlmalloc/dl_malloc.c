/*
 * SylixOS(TM)  LW : long wing
 * Copyright All Rights Reserved
 *
 * MODULE PRIVATE DYNAMIC MEMORY IN VPROCESS PATCH
 * this file is support current module malloc/free use his own heap in a vprocess.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#define  __SYLIXOS_KERNEL /* need some kernel function */
#include <unistd.h>
#include <sys/mman.h>

/*
 * sbrk lock
 */
void __vp_patch_lock(void);
void __vp_patch_unlock(void);
void __vp_patch_sbrk(BOOL lock);

/*
 * pre allocate physical memory
 */
void  __vp_pre_alloc_phy(const void *pmem, size_t nbytes);

/*
 * global heap
 */
static PLW_CLASS_HEAP  pctx_heap;

/*
 * dlmalloc_init
 */
void  dlmalloc_init (PLW_CLASS_HEAP  pheap, void  *mem, size_t  size)
{
    _HeapCtor(pheap, mem, size);
    pctx_heap = pheap;
}

/*
 * dlmalloc_sbrk
 */
void *dlmalloc_sbrk (int  size)
{
    void *mem;
    int mextern = 0;
    static void *previous_top = NULL;

    if (size == 0) {
        return  (previous_top);

    } else if (size < 0) {
        return  ((void *)(-1));
    }

__re_try:
    __vp_patch_lock();
    mem = _HeapAllocate(pctx_heap, (size_t)size, __func__);
    if ((mem == NULL) && mextern) {
        __vp_patch_unlock();
        return  ((void *)(-1));
    }

    if (mem) {
        previous_top = (void *)((size_t)mem + (size_t)size);
        __vp_patch_unlock();

    } else {
        mextern = 1;
        __vp_patch_sbrk(FALSE);
        __vp_patch_unlock();
        goto  __re_try;
    }

    return  (mem);
}

/*
 * dlmalloc_mmap
 */
void *dlmalloc_mmap (size_t  stLen)
{
    void  *mem;

    mem = mmap(NULL, stLen, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem != MAP_FAILED) {
        __vp_pre_alloc_phy(mem, stLen);
    }

    return  (mem);
}

/*
 * dlmalloc_mremap
 */
void  *dlmalloc_mremap (void *pvAddr, size_t stOldSize, size_t stNewSize, int mv)
{
    void  *mem;
    int    flag = 0;

    if (mv) {
        flag = MREMAP_MAYMOVE;
    }

    mem = mremap(pvAddr, stOldSize, stNewSize, flag);
    if (mem != MAP_FAILED) {
        __vp_pre_alloc_phy(mem, stNewSize);
    }

    return  (mem);
}

/*
 * end
 */
