/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: mosipc_heap.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2014 年 10 月 28 日
**
** 描        述: 多操作系统通信内存堆.
*********************************************************************************************************/

#ifndef __MOSIPC_HEAP_H
#define __MOSIPC_HEAP_H

/*********************************************************************************************************
  多操作系统通信内存堆管理
*********************************************************************************************************/

INT   __mosipcHeapInit(PLW_CLASS_HEAP pheapToBuild, PVOID pvStartAddress, size_t stByteSize);
PVOID __mosipcHeapAlloc(PLW_CLASS_HEAP pheap, size_t stByteSize, size_t stAlign, CPCHAR pcPurpose);
INT   __mosipcHeapFree(PLW_CLASS_HEAP pheap, PVOID pvStartAddress, CPCHAR pcPurpose);

#define MOSIPC_HEAP_INIT(pheap, addr, size)     __mosipcHeapInit(pheap, addr, size)
#define MOSIPC_HEAP_ALLOC(pheap, size, align)   __mosipcHeapAlloc(pheap, size, align, __func__)
#define MOSIPC_HEAP_FREE(pheap, addr)           __mosipcHeapFree(pheap, addr, __func__)

#endif                                                                  /*  __MOSIPC_HEAP_H             */
/*********************************************************************************************************
  END
*********************************************************************************************************/
