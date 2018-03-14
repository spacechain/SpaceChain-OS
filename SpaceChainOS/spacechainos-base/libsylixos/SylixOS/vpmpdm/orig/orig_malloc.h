/*
 * SylixOS(TM)  LW : long wing
 * Copyright All Rights Reserved
 *
 * MODULE PRIVATE DYNAMIC MEMORY IN VPROCESS PATCH
 * this file is support current module malloc/free use his own heap in a vprocess.
 * 
 * Author: Han.hui <sylixos@gmail.com>
 */

#ifndef __ORIG_MALLOC_H
#define __ORIG_MALLOC_H

/*
 * update memory infomation
 */
#define VP_MEM_INFO(pheap)

/*
 * build a memory management
 */
#define VP_MEM_CTOR(pheap, mem, size) _HeapCtor(pheap, mem, size)

/*
 * destory memory management
 */
#define VP_MEM_DTOR(pheap) _HeapDtor(pheap, FALSE)

/*
 * add memory to management
 */
#define VP_MEM_ADD(pheap, mem, size) _HeapAddMemory(pheap, mem, size)

/*
 * allocate memory
 */
#define VP_MEM_ALLOC(pheap, nbytes) _HeapAllocate(pheap, nbytes, __func__)

/*
 * allocate memory align
 */
#define VP_MEM_ALLOC_ALIGN(pheap, nbytes, align) _HeapAllocateAlign(pheap, nbytes, align, __func__)

/*
 * re-allocate memory
 */
#define VP_MEM_REALLOC(pheap, ptr, new_size, do_check) _HeapRealloc(pheap, ptr, new_size, do_check, __func__)

/*
 * free memory
 */
#define VP_MEM_FREE(pheap, ptr, do_check) _HeapFree(pheap, ptr, do_check, __func__)

#endif /* __ORIG_MALLOC_H */

/*
 * end
 */
