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
** 文   件   名: _HeapLib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 07 月 25 日
**
** 描        述: 堆内存操作库

** BUG
2007.04.09  LINE 291 if (bIsMergeOk == LW_TRUE) {...} 改为：if (bIsMergeOk) {...}
2007.06.04  LINE 184 , 185 分配大小，应该是整个分段大大小。
2007.07.25  加入 _Heap_Verify() 在交还内存时，选择性的进行内存检查，KHEAP 和 SHEAP 不需要，其它操作均需要
2007.11.04  将所有 PCHAR 类型改为 UINT8 * 类型.
2007.11.13  使用链表库对链表操作进行完全封装.
2007.11.18  整理注释.
2008.01.17  完全进行代码风格重构, 而且对堆的互斥进行了革命性的改进.
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock();
2008.05.31  使用 __KERNEL_MODE_...().
2008.06.17  加入最优的指定对齐内存分区管理的函数.
2009.03.16  HEAP 控制块中 HEAP_bIsSemLock 没有存在的必要, LW_CFG_SEMB_EN -> LW_CFG_SEMM_EN.
            REALLOC 复制的数据长度错误. 只有当分配成功时才可以释放原有的内存.
2009.03.17  对齐内存分配时, 左边的分段无法再分时, 返回的地址将不是坐分段的数据区首地址, 这时, 在返回地址的
            上一个空间记录分段首部地址, 为 free 时提供快速的判断手段.
2009.04.08  加入 SMP 多核支持.
2009.05.21  加入对最大用量统计的功能.
2009.07.03  避过 GCC 一个无谓的警告.
2009.07.14  加入 TRACE 可以选择打印分配的内存地址. (内存泄露调试使用)
2009.07.28  自旋锁的初始化放在初始化所有的控制块中, 这里去除相关操作.
2009.11.23  在这里设置 3 个函数指针用于跟踪内存泄露.
2009.11.24  free 内存时, 如果没有在指定的内存堆中, 需要 _DebugHandle() 报错.
2010.07.10  修正 realloc 注释.
2010.08.19  _HeapRealloc() 支持处理 alloc_align 返回的内存.
            增强了内存跟踪接口.
2011.03.05  为了增强内存跟踪功能, 这里加入了内存分配用途说明.
2011.07.15  将内存堆构造, 析构与创建, 删除分离, 这样可以不需要从内核分配控制块就可以创建一个内部使用的堆.
2011.07.19  realloc() 加入对各种情况的判断.
2011.07.24  加入 _HeapZallocate() 方法.
2011.08.07  _HeapFree() 释放 NULL 时直接退出, 因为 linux kfree() 有对 null 的处理判断.
2011.11.03  heap 空闲分段使用环形链表管理, 优先使用 free 的内存, 而不是新建的分段.
            此方法主要是为了配合进程虚拟空间堆使用的缺页中断机制.
2012.11.09  _HeapRealloc() 判读如果是指定对齐关系的内存分配则不予支持, 没有实际意义.
2013.02.22  减少分段管理内存开销.
2013.05.01  加入内存越界检查.
2013.10.09  修正一些错误打印信息的内容.
2014.05.03  内存错误时, 打印堆的名字.
2014.07.03  可以给 heap 中添加内存.
            free 操作确保最右侧分段应该在 freelist 的最后, 为最不推荐分配的段.
2014.08.15  采用新的判断对齐内存释放算法.
2014.10.27  加入多操作系统通信接口.
2016.04.13  加入 _HeapGetMax() 获取最大内存空闲段大小.
2016.12.20  修正当 heap 添加内存后删除 heap 时, 使用情况判断错误.
2017.06.23  加入外部内存跟踪接口.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  TRACE
*********************************************************************************************************/
#if LW_CFG_SHELL_HEAP_TRACE_EN > 0

VOIDFUNCPTR _K_pfuncHeapTraceAlloc;
VOIDFUNCPTR _K_pfuncHeapTraceFree;

#define __HEAP_TRACE_ALLOC(pheap, pvAddr, stLen, pcPurpose)   do {          \
            if (_K_pfuncHeapTraceAlloc) {                                   \
                _K_pfuncHeapTraceAlloc(pheap, pvAddr, stLen, pcPurpose);    \
            }                                                               \
        } while (0)
#define __HEAP_TRACE_ALLOC_ALIGN(pheap, pvAddr, stLen, pcPurpose) do {      \
            if (_K_pfuncHeapTraceAlloc) {                                   \
                _K_pfuncHeapTraceAlloc(pheap, pvAddr, stLen, pcPurpose);    \
            }                                                               \
        } while (0)
#define __HEAP_TRACE_FREE(pheap, pvAddr)    do {                            \
            if (_K_pfuncHeapTraceFree) {                                    \
                _K_pfuncHeapTraceFree(pheap, pvAddr);                       \
            }                                                               \
        } while (0)
#else
#define __HEAP_TRACE_ALLOC(pheap, pvAddr, stLen, pcPurpose)
#define __HEAP_TRACE_ALLOC_ALIGN(pheap, pvAddr, stLen, pcPurpose)
#define __HEAP_TRACE_FREE(pheap, pvAddr)
#endif                                                                  /*  LW_CFG_SHELL_HEAP_TRACE_EN  */  
/*********************************************************************************************************
  锁信号量的相关选项
*********************************************************************************************************/
#define __HEAP_LOCK_OPT     (LW_OPTION_WAIT_PRIORITY | LW_OPTION_INHERIT_PRIORITY | \
                             LW_OPTION_DELETE_SAFE | LW_OPTION_OBJECT_DEBUG_UNPEND | \
                             LW_OPTION_OBJECT_GLOBAL)
/*********************************************************************************************************
  最大使用量统计
*********************************************************************************************************/
#define __HEAP_UPDATA_MAX_USED(pheap)       do {                                        \
            if ((pheap)->HEAP_stMaxUsedByteSize < (pheap)->HEAP_stUsedByteSize) {       \
                (pheap)->HEAP_stMaxUsedByteSize = (pheap)->HEAP_stUsedByteSize;         \
            }                                                                           \
        } while (0)
/*********************************************************************************************************
  allocate 新分段加入空闲链中 
  如果新的分段是高地址, 这里必须放在链表的最后, 使它尽量不要被使用, 这样如果使用了缺页分配机制, 
  则尽量节约物理页面的使用量.
*********************************************************************************************************/
#define __HEAP_ADD_NEW_SEG_TO_FREELIST(psegment, pheap)    do {                         \
            if (_list_line_get_next(&psegment->SEGMENT_lineManage)) {                   \
                _List_Ring_Add_Ahead(&psegment->SEGMENT_ringFreeList,                   \
                                     &pheap->HEAP_pringFreeSegment);                    \
            } else {                                                                    \
                _List_Ring_Add_Last(&psegment->SEGMENT_ringFreeList,                    \
                                    &pheap->HEAP_pringFreeSegment);                     \
            }                                                                           \
        } while (0)
/*********************************************************************************************************
  分段使用标志 (当在空闲链表中时, 分段为空闲分段, 不在空闲链表中, 则为正在使用的分段)
*********************************************************************************************************/
#define __HEAP_SEGMENT_IS_USED(psegment)    (!_list_ring_get_prev(&((psegment)->SEGMENT_ringFreeList)))
/*********************************************************************************************************
  分段是否有效
*********************************************************************************************************/
#define __HEAP_SEGMENT_IS_REAL(psegment)    ((psegment)->SEGMENT_stMagic == LW_SEG_MAGIC_REAL)
/*********************************************************************************************************
  分段数据指针
*********************************************************************************************************/
#define __HEAP_SEGMENT_DATA_PTR(psegment)   ((UINT8 *)(psegment) + __SEGMENT_BLOCK_SIZE_ALIGN)
/*********************************************************************************************************
  通过数据指针返回分段指针
*********************************************************************************************************/
#define __HEAP_SEGMENT_SEG_PTR(pvData)      ((PLW_CLASS_SEGMENT)((UINT8 *)(pvData) - \
                                                                 __SEGMENT_BLOCK_SIZE_ALIGN))
/*********************************************************************************************************
  理论上下一个分段地址
*********************************************************************************************************/
#define __HEAP_SEGMENT_NEXT_PTR(psegment)   ((PLW_CLASS_SEGMENT)(__HEAP_SEGMENT_DATA_PTR(psegment) + \
                                                                 psegment->SEGMENT_stByteSize))
/*********************************************************************************************************
  是否可以合并分段判断
*********************************************************************************************************/
#define __HEAP_SEGMENT_CAN_MR(psegment, psegmentRight)  \
        (__HEAP_SEGMENT_NEXT_PTR(psegment) == psegmentRight)
#define __HEAP_SEGMENT_CAN_ML(psegment, psegmentLeft)   \
        (__HEAP_SEGMENT_NEXT_PTR(psegmentLeft) == psegment)
/*********************************************************************************************************
  当出现需要归还的内存不在内存堆时, 需要打印如下信息
*********************************************************************************************************/
#define __DEBUG_MEM_ERROR(caller, heap, error, addr)  \
        _DebugFormat(__ERRORMESSAGE_LEVEL, "\'%s\' heap %s memory is %s, address %p.\r\n",  \
                     caller, heap, error, addr);
/*********************************************************************************************************
  需要内存越界检查
*********************************************************************************************************/
#define __HEAP_SEGMENT_MARK_FLAG            0xA5A54321

static LW_INLINE size_t __heap_crossbord_size (size_t  stSize)
{
    if (_K_bHeapCrossBorderEn) {
        return  (stSize + sizeof(size_t));
    } else {
        return  (stSize);
    }
}

static LW_INLINE VOID __heap_crossbord_mark (PLW_CLASS_SEGMENT psegment)
{
    size_t  *pstMark;
    
    if (_K_bHeapCrossBorderEn) {
        pstMark = (size_t *)(__HEAP_SEGMENT_DATA_PTR(psegment)
                + (psegment->SEGMENT_stByteSize - sizeof(size_t)));
        *pstMark = __HEAP_SEGMENT_MARK_FLAG;
    }
}

static LW_INLINE BOOL __heap_crossbord_check (PLW_CLASS_SEGMENT psegment)
{
    size_t  *pstMark;
    
    if (_K_bHeapCrossBorderEn) {
        pstMark = (size_t *)(__HEAP_SEGMENT_DATA_PTR(psegment)
                + (psegment->SEGMENT_stByteSize - sizeof(size_t)));
        if (*pstMark != __HEAP_SEGMENT_MARK_FLAG) {
            return  (LW_FALSE);
        }
    }
    
    return  (LW_TRUE);
}
/*********************************************************************************************************
** 函数名称: __heap_lock
** 功能描述: 锁定一个内存堆
** 输　入  : pheap                 内存堆
** 输　出  : 错误代码
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if (LW_CFG_SEMM_EN > 0) && (LW_CFG_MAX_EVENTS > 0)
static ULONG    __heap_lock (PLW_CLASS_HEAP  pheap)
{
    if (pheap->HEAP_ulLock) {
        return  (API_SemaphoreMPend(pheap->HEAP_ulLock,
                                    LW_OPTION_WAIT_INFINITE));
    } else {
        LW_SPIN_LOCK(&pheap->HEAP_slLock);                              /*  进入自旋锁保护资源          */
        return  (ERROR_NONE);
    }
}
#else 
static ULONG    __heap_lock (PLW_CLASS_HEAP  pheap)
{
    LW_SPIN_LOCK(&pheap->HEAP_slLock);                                  /*  进入自旋锁保护资源          */
    return  (ERROR_NONE);
}
#endif                                                                  /*  (LW_CFG_SEMM_EN > 0) &&     */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
** 函数名称: __heap_unlock
** 功能描述: 解锁一个内存堆
** 输　入  : pheap                 内存堆
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if (LW_CFG_SEMM_EN > 0) && (LW_CFG_MAX_EVENTS > 0)
static VOID    __heap_unlock (PLW_CLASS_HEAP  pheap)
{
    if (pheap->HEAP_ulLock) {
        API_SemaphoreMPost(pheap->HEAP_ulLock);
    } else {
        LW_SPIN_UNLOCK(&pheap->HEAP_slLock);                            /*  释放自旋锁                  */
    }
}
#else
static VOID    __heap_unlock (PLW_CLASS_HEAP  pheap)
{
    LW_SPIN_UNLOCK(&pheap->HEAP_slLock);                                /*  释放自旋锁                  */
}
#endif                                                                  /*  (LW_CFG_SEMM_EN > 0) &&     */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
** 函数名称: _HeapTraceAlloc
** 功能描述: 堆申请跟踪函数
** 输　入  : pheap              堆控制块
**           pvMem              分配指针
**           stByteSize         分配的字节数
**           pcPurpose          分配内存的用途
** 输　出  :
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  _HeapTraceAlloc (PLW_CLASS_HEAP  pheap, PVOID  pvMem, size_t  stByteSize, CPCHAR  cpcPurpose)
{
    __HEAP_TRACE_ALLOC(pheap, pvMem, stByteSize, cpcPurpose);
}
/*********************************************************************************************************
** 函数名称: _HeapTraceFree
** 功能描述: 堆释放跟踪函数
** 输　入  : pheap              堆控制块
**           pvMem              内存指针
** 输　出  :
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  _HeapTraceFree (PLW_CLASS_HEAP  pheap, PVOID  pvMem)
{
    __HEAP_TRACE_FREE(pheap, pvMem);
}
/*********************************************************************************************************
** 函数名称: _HeapCtor
** 功能描述: 构造一个内存堆
** 输　入  : pheapToBuild          需要创建的堆
**           pvStartAddress        起始内存地址
**           stByteSize            内存堆的大小
**           bIsMosHeap            是否为多操作系统内存堆
** 输　出  : 建立好的内存堆控制块
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PLW_CLASS_HEAP  _HeapCtorEx (PLW_CLASS_HEAP    pheapToBuild,
                             PVOID             pvStartAddress, 
                             size_t            stByteSize, 
                             BOOL              bIsMosHeap)
{
    REGISTER PLW_CLASS_SEGMENT  psegment;
    REGISTER addr_t             ulStart      = (addr_t)pvStartAddress;
    REGISTER addr_t             ulStartAlign = ROUND_UP(ulStart, LW_CFG_HEAP_ALIGNMENT);
    
    if (ulStartAlign > ulStart) {
        stByteSize -= (size_t)(ulStartAlign - ulStart);                 /*  去掉前面的不对齐长度        */
    }
    
    stByteSize = ROUND_DOWN(stByteSize, LW_CFG_HEAP_ALIGNMENT);         /*  分段大小对其                */
    psegment   = (PLW_CLASS_SEGMENT)ulStartAlign;                       /*  第一个分段起始地址          */
    
    _LIST_LINE_INIT_IN_CODE(psegment->SEGMENT_lineManage);              /*  初始化第一个分段            */
    _LIST_RING_INIT_IN_CODE(psegment->SEGMENT_ringFreeList);
    
    psegment->SEGMENT_stByteSize = stByteSize
                                 - __SEGMENT_BLOCK_SIZE_ALIGN;          /*  第一个分段的大小            */
    psegment->SEGMENT_stMagic    = LW_SEG_MAGIC_REAL;
                                         
    pheapToBuild->HEAP_pringFreeSegment  = LW_NULL;
    _List_Ring_Add_Ahead(&psegment->SEGMENT_ringFreeList, 
                         &pheapToBuild->HEAP_pringFreeSegment);         /*  加入空闲表                  */
    
    pheapToBuild->HEAP_pvStartAddress    = (PVOID)ulStartAlign;
    pheapToBuild->HEAP_ulSegmentCounter  = 1;                           /*  当前的分段数                */
    
    pheapToBuild->HEAP_stTotalByteSize   = stByteSize;                  /*  分区总大小                  */
    pheapToBuild->HEAP_stUsedByteSize    = __SEGMENT_BLOCK_SIZE_ALIGN;  /*  使用的字节数                */
    pheapToBuild->HEAP_stFreeByteSize    = stByteSize
                                         - __SEGMENT_BLOCK_SIZE_ALIGN;  /*  空闲的字节数                */
    pheapToBuild->HEAP_stMaxUsedByteSize = pheapToBuild->HEAP_stUsedByteSize;

    if (bIsMosHeap) {
        pheapToBuild->HEAP_ulLock = LW_OBJECT_HANDLE_INVALID;
        
    } else {
#if (LW_CFG_SEMM_EN > 0) && (LW_CFG_MAX_EVENTS > 0)
        pheapToBuild->HEAP_ulLock = API_SemaphoreMCreate("heap_lock",
                                                         LW_PRIO_DEF_CEILING, 
                                                         __HEAP_LOCK_OPT, 
                                                         LW_NULL);      /*  建立锁                      */
#endif                                                                  /*  (LW_CFG_SEMM_EN > 0) &&     */
    }                                                                   /*  (LW_CFG_MAX_EVENTS > 0)     */
    
    MONITOR_EVT_LONG3(MONITOR_EVENT_ID_REGION, MONITOR_EVENT_REGION_CREATE,
                      pheapToBuild, pvStartAddress, stByteSize, LW_NULL);
    
    return  (pheapToBuild);
}
/*********************************************************************************************************
** 函数名称: _HeapCtor
** 功能描述: 构造一个内存堆
** 输　入  : pheapToBuild          需要创建的堆
**           pvStartAddress        起始内存地址
**           stByteSize            内存堆的大小
** 输　出  : 建立好的内存堆控制块
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PLW_CLASS_HEAP  _HeapCtor (PLW_CLASS_HEAP    pheapToBuild,
                           PVOID             pvStartAddress, 
                           size_t            stByteSize)
{
    return  (_HeapCtorEx(pheapToBuild, pvStartAddress, stByteSize, LW_FALSE));
}
/*********************************************************************************************************
** 函数名称: _HeapCreate
** 功能描述: 建立一个内存堆
** 输　入  : pvStartAddress        起始内存地址
**           stByteSize            内存堆的大小
** 输　出  : 建立好的内存堆控制块
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PLW_CLASS_HEAP  _HeapCreate (PVOID             pvStartAddress, 
                             size_t            stByteSize)
{
    REGISTER PLW_CLASS_HEAP     pheapToBuild;

    __KERNEL_MODE_PROC(
        pheapToBuild = _Allocate_Heap_Object();                         /*  申请控制块                  */
    );
    
    if (!pheapToBuild) {                                                /*  失败                        */
        return  (LW_NULL);
    }
    
    return  (_HeapCtor(pheapToBuild, pvStartAddress, stByteSize));      /*  构造内存堆                  */
}
/*********************************************************************************************************
** 函数名称: _HeapDtor
** 功能描述: 析构一个内存堆
** 输　入  : pheap             已经建立的控制块
**           bIsCheckUsed      当没有释放完时, 是否析构此堆
** 输　出  : 删除成功返回 LW_NULL
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PLW_CLASS_HEAP _HeapDtor (PLW_CLASS_HEAP  pheap, BOOL  bIsCheckUsed)
{
    REGISTER ULONG              ulLockErr;
    REGISTER PLW_LIST_LINE      plineTemp;
    REGISTER PLW_CLASS_SEGMENT  psegment;

    ulLockErr = __heap_lock(pheap);
    if (ulLockErr) {                                                    /*  出现锁错误                  */
        return  (pheap);
    }
    if (bIsCheckUsed) {                                                 /*  检查分区是否空闲            */
        psegment = (PLW_CLASS_SEGMENT)pheap->HEAP_pvStartAddress;
        for (plineTemp  = &psegment->SEGMENT_lineManage;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {             /*  遍历分段                    */
            psegment = _LIST_ENTRY(plineTemp, LW_CLASS_SEGMENT, SEGMENT_lineManage);
            if (__HEAP_SEGMENT_IS_USED(psegment)) {
                __heap_unlock(pheap);
                return  (pheap);                                        /*  分区正在被使用              */
            }
        }
    }
    pheap->HEAP_stTotalByteSize = 0;                                    /*  防范互斥操作的漏洞          */
    pheap->HEAP_stFreeByteSize  = 0;                                    /*  将剩余的字节数置零          */
                                                                        /*  解锁后不可能在进行分配      */
#if (LW_CFG_SEMM_EN > 0) && (LW_CFG_MAX_EVENTS > 0)
    if (pheap->HEAP_ulLock) {                                           /*  是否建立了锁                */
        API_SemaphoreMDelete(&pheap->HEAP_ulLock);                      /*  删除锁                      */
    } else 
#endif                                                                  /*  (LW_CFG_SEMM_EN > 0) &&     */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
    {
        __heap_unlock(pheap);                                           /*  有 HEAP_bIsSemLock 确保不会 */
                                                                        /*  出现解锁方式的严重错误      */
    }
    
    MONITOR_EVT_LONG1(MONITOR_EVENT_ID_REGION, MONITOR_EVENT_REGION_DELETE,
                      pheap, LW_NULL);
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: _HeapDelete
** 功能描述: 删除一个内存堆
** 输　入  : pheap             已经建立的控制块
**           bIsCheckUsed      是否检查使用情况
** 输　出  : 删除成功返回 LW_NULL
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PLW_CLASS_HEAP  _HeapDelete (PLW_CLASS_HEAP  pheap, BOOL  bIsCheckUsed)
{
    if (_HeapDtor(pheap, bIsCheckUsed) == LW_NULL) {                    /*  必须是放完内存              */
        __KERNEL_MODE_PROC(
            _Free_Heap_Object(pheap);                                   /*  释放控制块                  */
        );
        return  (LW_NULL);
    
    } else {
        return  (pheap);
    }
}
/*********************************************************************************************************
** 函数名称: _HeapAddMemory
** 功能描述: 向一个堆内添加内存
** 输　入  : pheap                 堆控制块
**           pvMemory              需要添加的内存 (必须保证对其)
**           stSize                内存大小 (必须大于一个分段大小最小值)
** 输　出  : 是否添加成功
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  _HeapAddMemory (PLW_CLASS_HEAP  pheap, PVOID  pvMemory, size_t  stSize)
{
    REGISTER PLW_LIST_LINE      plineTemp;
    REGISTER PLW_CLASS_SEGMENT  psegmentLast;
    REGISTER PLW_CLASS_SEGMENT  psegmentNew;
    REGISTER ULONG              ulLockErr;
    REGISTER addr_t             ulStart      = (addr_t)pvMemory;
    REGISTER addr_t             ulStartAlign = ROUND_UP(ulStart, LW_CFG_HEAP_ALIGNMENT);
    REGISTER addr_t             ulEnd, ulSeg;
    
    if (ulStartAlign > ulStart) {
        stSize -= (size_t)(ulStartAlign - ulStart);                     /*  去掉前面的不对齐长度        */
    }
    
    stSize = ROUND_DOWN(stSize, LW_CFG_HEAP_ALIGNMENT);                 /*  分段大小对其                */
    ulEnd  = ulStartAlign + stSize - 1;

    ulLockErr = __heap_lock(pheap);
    if (ulLockErr) {                                                    /*  出现锁错误                  */
        return  (ulLockErr);
    }
    
    psegmentLast = (PLW_CLASS_SEGMENT)pheap->HEAP_pvStartAddress;
    plineTemp    = &psegmentLast->SEGMENT_lineManage;
    
    do {
        if (_list_line_get_next(plineTemp) == LW_NULL) {
            break;
        }
                                                                        /*  判断地址是否发生重叠        */
        psegmentLast = _LIST_ENTRY(plineTemp, LW_CLASS_SEGMENT, SEGMENT_lineManage);
        ulSeg = (addr_t)__HEAP_SEGMENT_DATA_PTR(psegmentLast);
        if ((ulStartAlign >= ulSeg) && 
            (ulStartAlign < (ulSeg + psegmentLast->SEGMENT_stByteSize))) {
            __heap_unlock(pheap);
            return  (EFAULT);
        }
        if ((ulEnd >= ulSeg) && 
            (ulEnd < (ulSeg + psegmentLast->SEGMENT_stByteSize))) {
            __heap_unlock(pheap);
            return  (EFAULT);
        }
        
        plineTemp = _list_line_get_next(plineTemp);
    } while (1);
    
    psegmentLast = _LIST_ENTRY(plineTemp, LW_CLASS_SEGMENT, SEGMENT_lineManage);
    
    psegmentNew = (PLW_CLASS_SEGMENT)ulStartAlign;
    psegmentNew->SEGMENT_stByteSize = stSize - __SEGMENT_BLOCK_SIZE_ALIGN;
    psegmentNew->SEGMENT_stMagic    = LW_SEG_MAGIC_REAL;
    
    _List_Line_Add_Right(&psegmentNew->SEGMENT_lineManage,
                         &psegmentLast->SEGMENT_lineManage);
                         
    __HEAP_ADD_NEW_SEG_TO_FREELIST(psegmentNew, pheap);
    
    pheap->HEAP_ulSegmentCounter++;
    pheap->HEAP_stTotalByteSize += stSize;
    pheap->HEAP_stUsedByteSize  += __SEGMENT_BLOCK_SIZE_ALIGN;
    pheap->HEAP_stFreeByteSize  += psegmentNew->SEGMENT_stByteSize;
    
    __heap_unlock(pheap);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _HeapGetInfo
** 功能描述: 获得一个堆的状态
** 输　入  : pheap                 堆控制块
**           psegmentList[]        保存分段指针的表格
**           iMaxCounter           表格的索引数量
** 输　出  : SEGMENT 的数量
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  _HeapGetInfo (PLW_CLASS_HEAP  pheap, PLW_CLASS_SEGMENT  psegmentList[], INT  iMaxCounter)
{
    REGISTER ULONG              ulI = 0;
    REGISTER PLW_LIST_LINE      plineSegmentList;
    REGISTER PLW_CLASS_SEGMENT  psegmentFrist;
    
    REGISTER ULONG              ulLockErr;
    
    psegmentFrist = (PLW_CLASS_SEGMENT)pheap->HEAP_pvStartAddress;
    
    ulLockErr = __heap_lock(pheap);
    if (ulLockErr) {                                                    /*  出现锁错误                  */
        return  (0);
    }
    
    for (plineSegmentList   = &psegmentFrist->SEGMENT_lineManage;
         (plineSegmentList != LW_NULL) && (iMaxCounter > 0);
         plineSegmentList   = _list_line_get_next(plineSegmentList)) {  /*  查询所有的分段              */
         
         psegmentList[ulI++] = _LIST_ENTRY(plineSegmentList, 
                                           LW_CLASS_SEGMENT, 
                                           SEGMENT_lineManage);         /*  获得分段控制块              */
         iMaxCounter--;
    }
    __heap_unlock(pheap);
    
    return  (ulI);
}
/*********************************************************************************************************
** 函数名称: _HeapGetMax
** 功能描述: 获得最大连续内存大小
** 输　入  : pheap                 堆控制块
** 输　出  : 最大空闲内存分段大小
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
size_t  _HeapGetMax (PLW_CLASS_HEAP  pheap)
{
             PLW_LIST_RING      pringFreeSegment;
    REGISTER PLW_CLASS_SEGMENT  psegment;
    REGISTER ULONG              ulLockErr;
             size_t             stMax = 0;
    
    ulLockErr = __heap_lock(pheap);
    if (ulLockErr) {                                                    /*  出现锁错误                  */
        return  (0);
    }
    
    pringFreeSegment = pheap->HEAP_pringFreeSegment;
    if (pringFreeSegment) {
        do {
            psegment = _LIST_ENTRY(pringFreeSegment, 
                                   LW_CLASS_SEGMENT, 
                                   SEGMENT_ringFreeList);
            if (stMax < psegment->SEGMENT_stByteSize) {
                stMax = psegment->SEGMENT_stByteSize;
            }
            
            pringFreeSegment = _list_ring_get_next(pringFreeSegment);
        } while (pringFreeSegment != pheap->HEAP_pringFreeSegment);
    }
    __heap_unlock(pheap);
    
    return  (stMax);
}
/*********************************************************************************************************
** 函数名称: _HeapVerify
** 功能描述: 检测是个内存地址是否在指定内存堆中并且已被占用(调用时已经进入护持状态)
** 输　入  : pheap                        堆控制块指针
**           pvStartAddress               内存起始地址
**           ppsegmentUsed                回写所在分段控制块起始地址
**           pcCaller                     打印 debug 信息时的函数名
** 输　出  : 检测是否成功
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
BOOL    _HeapVerify (PLW_CLASS_HEAP     pheap, 
                     PVOID              pvStartAddress, 
                     PLW_CLASS_SEGMENT *ppsegmentUsed,
                     CPCHAR             pcCaller)
{
    REGISTER PLW_CLASS_SEGMENT  psegment;
    REGISTER PLW_LIST_LINE      plineSegment;
    
    if ((pvStartAddress < pheap->HEAP_pvStartAddress) ||
        (pvStartAddress >= (PVOID)((UINT8 *)pheap->HEAP_pvStartAddress
                         + pheap->HEAP_stTotalByteSize))) {
        __DEBUG_MEM_ERROR(pcCaller, pheap->HEAP_cHeapName, 
                          "not in this heap", pvStartAddress);
        return  (LW_FALSE);
    }
    
    psegment = (PLW_CLASS_SEGMENT)pheap->HEAP_pvStartAddress;           /*  获得第一个分段              */
    
    for (plineSegment = &psegment->SEGMENT_lineManage;
         ;
         plineSegment = _list_line_get_next(plineSegment)) {            /*  扫描各个分段                */
         
        if (plineSegment == LW_NULL) {                                  /*  扫描完成                    */
            __DEBUG_MEM_ERROR(pcCaller, pheap->HEAP_cHeapName, 
                              "in heap control block", pvStartAddress);
            return  (LW_FALSE);
        }
        
        psegment = _LIST_ENTRY(plineSegment,  LW_CLASS_SEGMENT, SEGMENT_lineManage);
        
        if ((pvStartAddress >  (PVOID)psegment) &&
            (pvStartAddress <= (PVOID)(__HEAP_SEGMENT_DATA_PTR(psegment)
                             + psegment->SEGMENT_stByteSize))) {        /*  遍历分段                    */
            
            if (__HEAP_SEGMENT_IS_USED(psegment)) {
                *ppsegmentUsed = psegment;                              /*  找到分段                    */
                return  (LW_TRUE);
            } else {                                                    /*  分段使用标志错误            */
                __DEBUG_MEM_ERROR(pcCaller, pheap->HEAP_cHeapName, 
                                  "not in used or double free", pvStartAddress);
                return  (LW_FALSE);
            }
        }
    }
    
    return  (LW_FALSE);                                                 /*  不会运行到这里              */
}
/*********************************************************************************************************
** 函数名称: _HeapAllocate
** 功能描述: 从堆中申请字节池 (首次适应算法)
** 输　入  : pheap              堆控制块
**           stByteSize         分配的字节数
**           pcPurpose          分配内存的用途
** 输　出  : 分配的内存地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PVOID  _HeapAllocate (PLW_CLASS_HEAP  pheap, size_t  stByteSize, CPCHAR  pcPurpose)
{
    REGISTER size_t             stSize;
    REGISTER size_t             stNewSegSize;
    REGISTER PLW_CLASS_SEGMENT  psegment;
    REGISTER PLW_CLASS_SEGMENT  psegmentNew;
    
             PLW_LIST_LINE      plineHeader;
             PLW_LIST_RING      pringFreeSegment;
             
    REGISTER ULONG              ulLockErr;
    
    stByteSize = __heap_crossbord_size(stByteSize);                     /*  越界检查调整                */
    
    stSize = ROUND_UP(stByteSize, LW_CFG_HEAP_SEG_MIN_SIZE);            /*  获得页对齐内存大小          */
    
    if (pheap->HEAP_stFreeByteSize < stSize) {                          /*  没有空间                    */
        return  (LW_NULL);
    }
    
    ulLockErr = __heap_lock(pheap);
    if (ulLockErr) {                                                    /*  出现锁错误                  */
        return  (LW_NULL);
    }
    
    pringFreeSegment = pheap->HEAP_pringFreeSegment;                    /*  第一个空闲分段              */
    if (pringFreeSegment == LW_NULL) {
        __heap_unlock(pheap);
        return  (LW_NULL);                                              /*  没有空闲分段                */
    }
    
    do {
        psegment = _LIST_ENTRY(pringFreeSegment, 
                               LW_CLASS_SEGMENT, 
                               SEGMENT_ringFreeList);
        if (psegment->SEGMENT_stByteSize >=  stSize) {
            break;                                                      /*  找到分段                    */
        }
        
        pringFreeSegment = _list_ring_get_next(pringFreeSegment);
        
        if (pringFreeSegment == pheap->HEAP_pringFreeSegment) {         /*  遍历完毕, 没有合适的空闲分段*/
            __heap_unlock(pheap);
            return  (LW_NULL);
        }
    } while (1);
    
    _List_Ring_Del(&psegment->SEGMENT_ringFreeList, 
                   &pheap->HEAP_pringFreeSegment);                      /*  将当前分段从空闲段链中删除  */
    
    if ((psegment->SEGMENT_stByteSize - stSize) > 
        (__SEGMENT_BLOCK_SIZE_ALIGN + LW_CFG_HEAP_SEG_MIN_SIZE)) {      /*  是否可以分出新段            */
                     
        stNewSegSize = psegment->SEGMENT_stByteSize - stSize;           /*  计算新分段的总大小          */
        
        psegment->SEGMENT_stByteSize = stSize;                          /*  重新确定当前分段大小        */
        
        psegmentNew = (PLW_CLASS_SEGMENT)(__HEAP_SEGMENT_DATA_PTR(psegment)
                    + stSize);                                          /*  填写新分段的诸多信息        */
        psegmentNew->SEGMENT_stByteSize = stNewSegSize
                                        - __SEGMENT_BLOCK_SIZE_ALIGN;
        psegmentNew->SEGMENT_stMagic    = LW_SEG_MAGIC_REAL;
        
        plineHeader = &psegment->SEGMENT_lineManage;
        _List_Line_Add_Tail(&psegmentNew->SEGMENT_lineManage,
                            &plineHeader);                              /*  将新分段链入邻居链表        */
                            
        __HEAP_ADD_NEW_SEG_TO_FREELIST(psegmentNew, pheap);             /*  将新分段链接入空闲分段链表  */
                             
        pheap->HEAP_ulSegmentCounter++;                                 /*  回写内存堆控制块            */
        pheap->HEAP_stUsedByteSize += (stSize + __SEGMENT_BLOCK_SIZE_ALIGN);
        pheap->HEAP_stFreeByteSize -= (stSize + __SEGMENT_BLOCK_SIZE_ALIGN);
        
    } else {
        pheap->HEAP_stUsedByteSize += psegment->SEGMENT_stByteSize;
        pheap->HEAP_stFreeByteSize -= psegment->SEGMENT_stByteSize;
    }
    
    __HEAP_UPDATA_MAX_USED(pheap);                                      /*  更新统计变量                */
    __heap_unlock(pheap);
    
    __HEAP_TRACE_ALLOC(pheap, 
                       (PVOID)__HEAP_SEGMENT_DATA_PTR(psegment),
                       psegment->SEGMENT_stByteSize,
                       pcPurpose);                                      /*  打印跟踪信息                */
    
    __heap_crossbord_mark(psegment);                                    /*  加入内存越界标志            */
    
    MONITOR_EVT_LONG4(MONITOR_EVENT_ID_REGION, MONITOR_EVENT_REGION_ALLOC,
                      pheap, __HEAP_SEGMENT_DATA_PTR(psegment), 
                      psegment->SEGMENT_stByteSize, sizeof(LW_STACK), pcPurpose);
    
    return  ((PVOID)__HEAP_SEGMENT_DATA_PTR(psegment));                 /*  返回分配的内存首地址        */
}
/*********************************************************************************************************
** 函数名称: _HeapAllocateAlign
** 功能描述: 从堆中申请字节池 (首次适应算法) (指定内存对齐关系)
** 输　入  : pheap              堆控制块
**           stByteSize         分配的字节数
**           stAlign            内存对齐关系
**           pcPurpose          分配内存的用途
** 输　出  : 分配的内存地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PVOID  _HeapAllocateAlign (PLW_CLASS_HEAP  pheap, size_t  stByteSize, size_t  stAlign, CPCHAR  pcPurpose)
{
    REGISTER size_t             stSize;
    REGISTER size_t             stNewSegSize;
    REGISTER PLW_CLASS_SEGMENT  psegment;
    REGISTER PLW_CLASS_SEGMENT  psegmentNew;
             PLW_CLASS_SEGMENT  psegmentAlloc = LW_NULL;                /*  避过 GCC 一个无谓的警告     */
    
             UINT8             *pcAlign;
             addr_t             ulAlignMask = (addr_t)(stAlign - 1);
             size_t             stAddedSize;                            /*  前端补齐附加数据大小        */
             BOOL               bLeftNewFree;                           /*  左端的内存是否可以开辟新段  */
    
             PLW_LIST_LINE      plineHeader;
             PLW_LIST_RING      pringFreeSegment;
             
    REGISTER ULONG              ulLockErr;
    
    stByteSize = __heap_crossbord_size(stByteSize);                     /*  越界检查调整                */
    
    stSize = ROUND_UP(stByteSize, LW_CFG_HEAP_SEG_MIN_SIZE);            /*  获得页对齐内存大小          */
    
    if (pheap->HEAP_stFreeByteSize < stSize) {                          /*  没有空间                    */
        return  (LW_NULL);
    }
    
    ulLockErr = __heap_lock(pheap);
    if (ulLockErr) {                                                    /*  出现锁错误                  */
        return  (LW_NULL);
    }
    
    pringFreeSegment = pheap->HEAP_pringFreeSegment;                    /*  第一个空闲分段              */
    if (pringFreeSegment == LW_NULL) {
        __heap_unlock(pheap);
        return  (LW_NULL);                                              /*  没有空闲分段                */
    }
    
    do {
        psegment = _LIST_ENTRY(pringFreeSegment, 
                               LW_CLASS_SEGMENT, 
                               SEGMENT_ringFreeList);
        
        if (((size_t)__HEAP_SEGMENT_DATA_PTR(psegment) & ulAlignMask) == 0) {
            pcAlign     = __HEAP_SEGMENT_DATA_PTR(psegment);            /*  本身就满足对齐条件          */
            stAddedSize = 0;
        } else {                                                        /*  获取满则对齐条件的内存点    */
            pcAlign     = (UINT8 *)(((addr_t)__HEAP_SEGMENT_DATA_PTR(psegment) | ulAlignMask) + 1);
            stAddedSize = (size_t)(pcAlign - __HEAP_SEGMENT_DATA_PTR(psegment));
        }
        
        if (psegment->SEGMENT_stByteSize >= (stSize + stAddedSize)) {
            if (stAddedSize >= 
                (__SEGMENT_BLOCK_SIZE_ALIGN + LW_CFG_HEAP_SEG_MIN_SIZE)) {
                bLeftNewFree = LW_TRUE;                                 /*  左端可以开辟新段            */
            } else {
                bLeftNewFree = LW_FALSE;
                stSize += stAddedSize;                                  /*  使用整个分段                */
                psegmentAlloc = psegment;                               /*  记录本块内存                */
            }
            break;                                                      /*  找到分段                    */
        }
    
        pringFreeSegment = _list_ring_get_next(pringFreeSegment);
        
        if (pringFreeSegment == pheap->HEAP_pringFreeSegment) {         /*  遍历完毕, 没有合适的空闲分段*/
            __heap_unlock(pheap);
            return  (LW_NULL);
        }
    } while (1);
    
    if (bLeftNewFree) {                                                 /*  首先将这个段拆分            */
        psegmentNew = (PLW_CLASS_SEGMENT)(__HEAP_SEGMENT_DATA_PTR(psegment)
                    + stAddedSize - __SEGMENT_BLOCK_SIZE_ALIGN);
        psegmentNew->SEGMENT_stByteSize = psegment->SEGMENT_stByteSize
                                        - stAddedSize;
        psegmentNew->SEGMENT_stMagic    = LW_SEG_MAGIC_REAL;
                                               
        plineHeader = &psegment->SEGMENT_lineManage;
        _List_Line_Add_Tail(&psegmentNew->SEGMENT_lineManage,
                            &plineHeader);                              /*  将新分段链入邻居链表        */
                            
        __HEAP_ADD_NEW_SEG_TO_FREELIST(psegmentNew, pheap);             /*  将新分段链接入空闲分段链表  */
        
        psegment->SEGMENT_stByteSize = stAddedSize 
                                     - __SEGMENT_BLOCK_SIZE_ALIGN;
        
        psegment = psegmentNew;                                         /*  确定到分配的分区            */
        
        pheap->HEAP_ulSegmentCounter++;                                 /*  回写内存堆控制块            */
        pheap->HEAP_stUsedByteSize += __SEGMENT_BLOCK_SIZE_ALIGN;
        pheap->HEAP_stFreeByteSize -= __SEGMENT_BLOCK_SIZE_ALIGN;
    }
    
    _List_Ring_Del(&psegment->SEGMENT_ringFreeList, 
                   &pheap->HEAP_pringFreeSegment);                      /*  将当前分段从空闲段链中删除  */
    
    if ((psegment->SEGMENT_stByteSize - stSize) > 
        (__SEGMENT_BLOCK_SIZE_ALIGN + LW_CFG_HEAP_SEG_MIN_SIZE)) {      /*  是否可以分出新段            */
                     
        stNewSegSize = psegment->SEGMENT_stByteSize - stSize;           /*  计算新分段的总大小          */
        
        psegment->SEGMENT_stByteSize = stSize;                          /*  重新确定当前分段大小        */
        
        psegmentNew = (PLW_CLASS_SEGMENT)(__HEAP_SEGMENT_DATA_PTR(psegment)
                    + stSize);                                          /*  填写新分段的诸多信息        */
        psegmentNew->SEGMENT_stByteSize = stNewSegSize
                                        - __SEGMENT_BLOCK_SIZE_ALIGN;
        psegmentNew->SEGMENT_stMagic    = LW_SEG_MAGIC_REAL;
                                        
        plineHeader = &psegment->SEGMENT_lineManage;
        _List_Line_Add_Tail(&psegmentNew->SEGMENT_lineManage,
                            &plineHeader);                              /*  将新分段链入邻居链表        */
               
        __HEAP_ADD_NEW_SEG_TO_FREELIST(psegmentNew, pheap);             /*  将新分段链接入空闲分段链表  */
                             
        pheap->HEAP_ulSegmentCounter++;                                 /*  回写内存堆控制块            */
        pheap->HEAP_stUsedByteSize += (stSize + __SEGMENT_BLOCK_SIZE_ALIGN);
        pheap->HEAP_stFreeByteSize -= (stSize + __SEGMENT_BLOCK_SIZE_ALIGN);
        
    } else {
        pheap->HEAP_stUsedByteSize += psegment->SEGMENT_stByteSize;
        pheap->HEAP_stFreeByteSize -= psegment->SEGMENT_stByteSize;
    }
    
    if ((bLeftNewFree == LW_FALSE) && stAddedSize) {                    /*  左端有些内存没有使用        */
        PLW_CLASS_SEGMENT    psegmentFake = __HEAP_SEGMENT_SEG_PTR(pcAlign);
        size_t              *pstLeft      = (((size_t *)pcAlign) - 1);
        *pstLeft = (size_t)psegmentAlloc;                               /*  记录真正分段控制块位置      */
        if (pstLeft != &psegmentFake->SEGMENT_stMagic) {
            psegmentFake->SEGMENT_stMagic = ~LW_SEG_MAGIC_REAL;         /*  不能识别本分段头            */
        }
    }
    
    __HEAP_UPDATA_MAX_USED(pheap);                                      /*  更新统计变量                */
    __heap_unlock(pheap);
    
    __HEAP_TRACE_ALLOC_ALIGN(pheap, 
                             pcAlign, 
                             psegment->SEGMENT_stByteSize,
                             pcPurpose);                                /*  打印跟踪信息                */
    
    __heap_crossbord_mark(psegment);                                    /*  加入内存越界标志            */
    
    MONITOR_EVT_LONG4(MONITOR_EVENT_ID_REGION, MONITOR_EVENT_REGION_ALLOC,
                      pheap, pcAlign, psegment->SEGMENT_stByteSize, 
					  stAlign, pcPurpose);
    
    return  (pcAlign);                                                  /*  返回分配的内存首地址        */
}
/*********************************************************************************************************
** 函数名称: _HeapZallocate
** 功能描述: 从堆中申请字节池并清零 (首次适应算法)
** 输　入  : pheap              堆控制块
**           stByteSize         分配的字节数 (max bytes is INT_MAX)
**           pcPurpose          分配内存的用途
** 输　出  : 分配的内存地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PVOID  _HeapZallocate (PLW_CLASS_HEAP  pheap, size_t  stByteSize, CPCHAR  pcPurpose)
{
    PVOID   pvMem = _HeapAllocate(pheap, stByteSize, pcPurpose);
    
    if (pvMem) {
        lib_bzero(pvMem, stByteSize);
    }
    
    return  (pvMem);
}
/*********************************************************************************************************
** 函数名称: _HeapFree
** 功能描述: 将申请的空间释放回堆, 且下一次优先被分配! (立即聚合算法)
** 输　入  : 
**           pheap              堆控制块
**           pvStartAddress     归还的地址
**           bIsNeedVerify      是否需要安全性检查
**           pcPurpose          谁释放内存
** 输　出  : 正确返回 LW_NULL 否则返回 pvStartAddress
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PVOID  _HeapFree (PLW_CLASS_HEAP  pheap, PVOID  pvStartAddress, BOOL  bIsNeedVerify, CPCHAR  pcPurpose)
{
    REGISTER BOOL               bIsMergeOk = LW_FALSE;
    REGISTER size_t             stSegmentByteSizeFree;
             PLW_CLASS_SEGMENT  psegment;
    REGISTER PLW_LIST_LINE      plineLeft;
    REGISTER PLW_LIST_LINE      plineRight;
    REGISTER PLW_CLASS_SEGMENT  psegmentLeft;                           /*  左分段                      */
    REGISTER PLW_CLASS_SEGMENT  psegmentRight;                          /*  右分段                      */
    
             PLW_LIST_LINE      plineDummyHeader = LW_NULL;             /*  用于参数传递的头            */
             
    REGISTER BOOL               bVerifyOk;                              /*  是否检查成功                */
    REGISTER ULONG              ulLockErr;
    
    if (pvStartAddress == LW_NULL) {
        return  (pvStartAddress);
    }
    
    ulLockErr = __heap_lock(pheap);
    if (ulLockErr) {                                                    /*  出现锁错误                  */
        return  (pvStartAddress);
    }
    if (bIsNeedVerify) {                                                /*  是否需要安全性检查          */
        bVerifyOk = _HeapVerify(pheap, pvStartAddress, 
                                &psegment, pcPurpose);                  /*  检查                        */
        if (bVerifyOk == LW_FALSE) {
            __heap_unlock(pheap);
            return  (pvStartAddress);                                   /*  检查错误                    */
        }
    } else {                                                            /*  不需要安全性检查            */
        psegment = __HEAP_SEGMENT_SEG_PTR(pvStartAddress);              /*  查找段控制块地址            */
        if (!__HEAP_SEGMENT_IS_REAL(psegment)) {                        /*  控制块地址不真实            */
            psegment = (PLW_CLASS_SEGMENT)(*((size_t *)pvStartAddress - 1));
        }
        
        if (!__HEAP_SEGMENT_IS_REAL(psegment) ||
            !__HEAP_SEGMENT_IS_USED(psegment)) {                        /*  段控制块无效或已经被释放    */
            __heap_unlock(pheap);
            __DEBUG_MEM_ERROR(pcPurpose, pheap->HEAP_cHeapName, 
                              "not in used or double free", pvStartAddress);
            return  (pvStartAddress);
        }
    }
    
    if (__heap_crossbord_check(psegment) == LW_FALSE) {                 /*  内存越界                    */
        __DEBUG_MEM_ERROR(pcPurpose, pheap->HEAP_cHeapName, 
                          "cross-border", pvStartAddress);
    }
    
    __HEAP_TRACE_FREE(pheap, pvStartAddress);                           /*  打印跟踪信息                */
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_REGION, MONITOR_EVENT_REGION_FREE,
                      pheap, pvStartAddress, LW_NULL);
    
    plineLeft  = _list_line_get_prev(&psegment->SEGMENT_lineManage);    /*  左分段                      */
    plineRight = _list_line_get_next(&psegment->SEGMENT_lineManage);    /*  右分段                      */
    
    if (plineLeft) {
        psegmentLeft  = _LIST_ENTRY(plineLeft,  
                                    LW_CLASS_SEGMENT, 
                                    SEGMENT_lineManage);                /*  左分段控制块                */
    } else {
        psegmentLeft  = LW_NULL;
    }
    if (plineRight) {
        psegmentRight = _LIST_ENTRY(plineRight, 
                                    LW_CLASS_SEGMENT, 
                                    SEGMENT_lineManage);                /*  右分段控制块                */
    } else {
        psegmentRight = LW_NULL;
    }
    
    stSegmentByteSizeFree = psegment->SEGMENT_stByteSize;               /*  将要释放的分段内容大小      */
        
    if (psegmentLeft) {
        if (__HEAP_SEGMENT_IS_USED(psegmentLeft) ||
            !__HEAP_SEGMENT_CAN_ML(psegment, psegmentLeft)) {           /*  不能聚合                    */
            goto    __merge_right;                                      /*  进入右段聚合                */
        }
        
        psegmentLeft->SEGMENT_stByteSize += (stSegmentByteSizeFree
                                          + __SEGMENT_BLOCK_SIZE_ALIGN);
        
        _List_Line_Del(&psegment->SEGMENT_lineManage, 
                       &plineDummyHeader);                              /*  从邻居链表中删除当前分段    */
        
        pheap->HEAP_ulSegmentCounter--;
        pheap->HEAP_stUsedByteSize -= (__SEGMENT_BLOCK_SIZE_ALIGN + stSegmentByteSizeFree);
        pheap->HEAP_stFreeByteSize += (__SEGMENT_BLOCK_SIZE_ALIGN + stSegmentByteSizeFree);
        
        psegment = psegmentLeft;                                        /*  当前分段变成左分段          */
    
        if (plineRight == LW_NULL) {                                    /*  最右侧分段                  */
            _List_Ring_Del(&psegment->SEGMENT_ringFreeList, 
                           &pheap->HEAP_pringFreeSegment);
            __HEAP_ADD_NEW_SEG_TO_FREELIST(psegment, pheap);            /*  重新插入适当的位置(节约内存)*/
        }
    
        bIsMergeOk = LW_TRUE;                                           /*  成功进行了左端合并          */
    }
    
__merge_right:
    
    if (psegmentRight) {                                                /*  右分段聚合                  */
        if (__HEAP_SEGMENT_IS_USED(psegmentRight) ||
            !__HEAP_SEGMENT_CAN_MR(psegment, psegmentRight)) {          /*  不能聚合                    */
            goto    __right_merge_fail;                                 /*  进入右段聚合失败            */
        }
        
        psegment->SEGMENT_stByteSize += (psegmentRight->SEGMENT_stByteSize
                                      + __SEGMENT_BLOCK_SIZE_ALIGN);
        
        _List_Ring_Del(&psegmentRight->SEGMENT_ringFreeList, 
                       &pheap->HEAP_pringFreeSegment);                  /*  将右分段从空闲表中删除      */
        
        _List_Line_Del(&psegmentRight->SEGMENT_lineManage, 
                       &plineDummyHeader);                              /*  将右分段从邻居链表中删除    */
        
        pheap->HEAP_ulSegmentCounter--;                                 /*  分区分段数量--              */
        
        if (bIsMergeOk == LW_FALSE) {                                   /*  当没有产生左合并时          */
            __HEAP_ADD_NEW_SEG_TO_FREELIST(psegment, pheap);            /*  这里插到空闲表头            */
            
            pheap->HEAP_stUsedByteSize -= (__SEGMENT_BLOCK_SIZE_ALIGN + stSegmentByteSizeFree);
            pheap->HEAP_stFreeByteSize += (__SEGMENT_BLOCK_SIZE_ALIGN + stSegmentByteSizeFree);
        
        } else {                                                        /*  左分段合并成功              */
            if (!_list_line_get_next(&psegment->SEGMENT_lineManage)) {  /*  合并后为最右侧分段          */
                _List_Ring_Del(&psegment->SEGMENT_ringFreeList, 
                               &pheap->HEAP_pringFreeSegment);
                __HEAP_ADD_NEW_SEG_TO_FREELIST(psegment, pheap);        /*  重新插入适当的位置(节约内存)*/
            }
        
            pheap->HEAP_stUsedByteSize -= (__SEGMENT_BLOCK_SIZE_ALIGN);
            pheap->HEAP_stFreeByteSize += (__SEGMENT_BLOCK_SIZE_ALIGN);
        }
        
        __heap_unlock(pheap);
        return  (LW_NULL);
    }
    
__right_merge_fail:                                                     /*  右分段合并错误              */
    
    if (bIsMergeOk == LW_FALSE) {                                       /*  左分段合并失败              */
        __HEAP_ADD_NEW_SEG_TO_FREELIST(psegment, pheap);                /*  这里插到空闲表头            */
        
        pheap->HEAP_stUsedByteSize -= (stSegmentByteSizeFree);
        pheap->HEAP_stFreeByteSize += (stSegmentByteSizeFree);
    }
    
    __heap_unlock(pheap);
    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: _HeapRealloc
** 功能描述: 从堆中重新申请字节池
** 输　入  : pheap              堆控制块
**           pvStartAddress     原始的内存缓冲区 (可能需要释放)
**           stNewByteSize      需要新的内存大小
**           bIsNeedVerify      是否需要安全检查
**           pcPurpose          分配内存的用途
** 输　出  : 分配的内存地址
** 全局变量: 
** 调用模块: 
** 注  意  : 这里的 pvStartAddress 和 ulNewByteSize 必须真实有效, 这个函数是内部使用的.
*********************************************************************************************************/
PVOID  _HeapRealloc (PLW_CLASS_HEAP  pheap, 
                     PVOID           pvStartAddress, 
                     size_t          stNewByteSize,
                     BOOL            bIsNeedVerify,
                     CPCHAR          pcPurpose)
{
    REGISTER size_t             stSize;
    REGISTER size_t             stNewSegSize;
             PLW_CLASS_SEGMENT  psegment;
    REGISTER PLW_CLASS_SEGMENT  psegmentNew;
    
             PLW_LIST_LINE      plineHeader;
             
    REGISTER BOOL               bVerifyOk;                              /*  是否检查成功                */
    REGISTER ULONG              ulLockErr;
    
    if (stNewByteSize == 0) {
        if (pvStartAddress) {
            return  (_HeapFree(pheap, pvStartAddress, 
                               bIsNeedVerify, pcPurpose));              /*  释放先前分配的内存即可      */
        } else {
            return  (LW_NULL);
        }
    }
    
    if (pvStartAddress == LW_NULL) {
        return  (_HeapAllocate(pheap, stNewByteSize, pcPurpose));       /*  仅分配新内存即可            */
    }
    
    stNewByteSize = __heap_crossbord_size(stNewByteSize);               /*  越界检查调整                */
    
    stSize = ROUND_UP(stNewByteSize, LW_CFG_HEAP_SEG_MIN_SIZE);         /*  获得页对齐内存大小          */
    
    ulLockErr = __heap_lock(pheap);
    if (ulLockErr) {                                                    /*  出现锁错误                  */
        return  (LW_NULL);
    }
    if (bIsNeedVerify) {                                                /*  是否需要安全性检查          */
        bVerifyOk = _HeapVerify(pheap, pvStartAddress, 
                                &psegment, __func__);                   /*  检查                        */
        if (bVerifyOk == LW_FALSE) {
            __heap_unlock(pheap);
            return  (LW_NULL);                                          /*  检查错误                    */
        }
    } else {                                                            /*  不需要安全性检查            */
        psegment = __HEAP_SEGMENT_SEG_PTR(pvStartAddress);              /*  查找段控制块地址            */
        if (!__HEAP_SEGMENT_IS_REAL(psegment)) {                        /*  控制块地址不真实            */
            psegment = (PLW_CLASS_SEGMENT)(*((size_t *)pvStartAddress - 1));
        }
        
        if (!__HEAP_SEGMENT_IS_REAL(psegment) ||
            !__HEAP_SEGMENT_IS_USED(psegment)) {                        /*  段控制块无效或已经被释放    */
            __heap_unlock(pheap);
            __DEBUG_MEM_ERROR(pcPurpose, pheap->HEAP_cHeapName, 
                              "not in used or double free", pvStartAddress);
            return  (LW_NULL);
        }
    }
    
    if (__heap_crossbord_check(psegment) == LW_FALSE) {                 /*  内存越界                    */
        __DEBUG_MEM_ERROR(pcPurpose, pheap->HEAP_cHeapName, 
                          "cross-border", pvStartAddress);
    }
    
    if (pvStartAddress != __HEAP_SEGMENT_DATA_PTR(psegment)) {          /*  realloc 不支持带有对齐特性  */
        __heap_unlock(pheap);
        __DEBUG_MEM_ERROR(pcPurpose, pheap->HEAP_cHeapName,
                          "bigger aligned", pvStartAddress);
        return  (LW_NULL);
    }
    
    if (stSize == psegment->SEGMENT_stByteSize) {                       /*  大小没有改变                */
        __heap_unlock(pheap);
        return  (pvStartAddress);
    }
    
    if (stSize > psegment->SEGMENT_stByteSize) {                        /*  希望分配到更大的空间        */
        REGISTER PLW_LIST_LINE      plineRight;
        REGISTER PLW_CLASS_SEGMENT  psegmentRight;                      /*  右分段                      */
                 PLW_LIST_LINE      plineDummyHeader = LW_NULL;         /*  用于参数传递的头            */
        
        plineRight = _list_line_get_next(&psegment->SEGMENT_lineManage);/*  右分段                      */
        if (plineRight) {
            psegmentRight = _LIST_ENTRY(plineRight, 
                                        LW_CLASS_SEGMENT, 
                                        SEGMENT_lineManage);            /*  右分段控制块                */
            if (!__HEAP_SEGMENT_IS_USED(psegmentRight) &&
                __HEAP_SEGMENT_CAN_MR(psegment, psegmentRight)) {
                                                                        /*  右分段没有使用              */
                if ((psegmentRight->SEGMENT_stByteSize + 
                     __SEGMENT_BLOCK_SIZE_ALIGN) >=
                    (stSize - psegment->SEGMENT_stByteSize)) {          /*  可以将右分段与本段合并      */
                    
                    _List_Ring_Del(&psegmentRight->SEGMENT_ringFreeList, 
                                   &pheap->HEAP_pringFreeSegment);      /*  将右分段从空闲表中删除      */
                    _List_Line_Del(&psegmentRight->SEGMENT_lineManage, 
                                   &plineDummyHeader);                  /*  将右分段从邻居链表中删除    */
                    
                    psegment->SEGMENT_stByteSize += 
                        (psegmentRight->SEGMENT_stByteSize +
                         __SEGMENT_BLOCK_SIZE_ALIGN);                   /*  更新本段大小                */
                                   
                    pheap->HEAP_ulSegmentCounter--;                     /*  分区分段数量--              */
                    pheap->HEAP_stUsedByteSize += psegmentRight->SEGMENT_stByteSize;
                    pheap->HEAP_stFreeByteSize -= psegmentRight->SEGMENT_stByteSize;
                    goto    __split_segment;
                }
            }
        }
        
        __heap_unlock(pheap);
        pvStartAddress = _HeapAllocate(pheap, stSize, pcPurpose);       /*  重新开辟                    */
        if (pvStartAddress) {                                           /*  分配成功                    */
            lib_memcpy(pvStartAddress, 
                       __HEAP_SEGMENT_DATA_PTR(psegment), 
                       (UINT)psegment->SEGMENT_stByteSize);             /*  复制原始信息                */
            _HeapFree(pheap, 
                      __HEAP_SEGMENT_DATA_PTR(psegment), 
                      LW_FALSE, pcPurpose);                             /*  释放本块空间                */
        }
        
        ulLockErr = __heap_lock(pheap);                                 /*  这里 heap 绝对不能被删除    */
        __HEAP_UPDATA_MAX_USED(pheap);                                  /*  更新统计变量                */
        __heap_unlock(pheap);
        return  (pvStartAddress);
    }
    
__split_segment:
    stNewSegSize = psegment->SEGMENT_stByteSize - stSize;               /*  获取剩余的字节数            */
    
    if (stNewSegSize < 
        (__SEGMENT_BLOCK_SIZE_ALIGN + LW_CFG_HEAP_SEG_MIN_SIZE)) {      /*  剩余空间太小, 无法分配新段  */
        __HEAP_UPDATA_MAX_USED(pheap);                                  /*  更新统计变量                */
        __heap_unlock(pheap);
        
        __heap_crossbord_mark(psegment);                                /*  加入内存越界标志            */
        
        MONITOR_EVT_LONG4(MONITOR_EVENT_ID_REGION, MONITOR_EVENT_REGION_REALLOC,
                          pheap, pvStartAddress, 
                          pvStartAddress, psegment->SEGMENT_stByteSize, pcPurpose);
                      
        return  (pvStartAddress);
    }
    
    psegment->SEGMENT_stByteSize = stSize;                              /*  更新原始分段的大小          */
    
    psegmentNew = (PLW_CLASS_SEGMENT)(__HEAP_SEGMENT_DATA_PTR(psegment)
                + stSize);                                              /*  确定新分段的地点            */
                
    psegmentNew->SEGMENT_stByteSize = stNewSegSize 
                                    - __SEGMENT_BLOCK_SIZE_ALIGN;
    psegmentNew->SEGMENT_stMagic    = LW_SEG_MAGIC_REAL;
                                           
    _INIT_LIST_RING_HEAD(&psegmentNew->SEGMENT_ringFreeList);           /*  正在使用的分段              */
                                           
    plineHeader = &psegment->SEGMENT_lineManage;
    _List_Line_Add_Tail(&psegmentNew->SEGMENT_lineManage,
                        &plineHeader);                                  /*  加入左右邻居链表连接        */
    
    pheap->HEAP_ulSegmentCounter++;                                     /*  堆中多了一个分段            */
    __heap_unlock(pheap);
    
    __heap_crossbord_mark(psegmentNew);                                 /*  加入内存越界标志            */
    
    _HeapFree(pheap, __HEAP_SEGMENT_DATA_PTR(psegmentNew), 
              LW_FALSE, pcPurpose);                                     /*  进行内存重组                */
    
    ulLockErr = __heap_lock(pheap);                                     /*  这里 heap 绝对不能被删除    */
    __HEAP_UPDATA_MAX_USED(pheap);                                      /*  更新统计变量                */
    __heap_unlock(pheap);
    
    __heap_crossbord_mark(psegment);                                    /*  加入内存越界标志            */
    
    MONITOR_EVT_LONG4(MONITOR_EVENT_ID_REGION, MONITOR_EVENT_REGION_REALLOC,
                      pheap, __HEAP_SEGMENT_DATA_PTR(psegment), 
                      pvStartAddress, stSize, pcPurpose);
    
    return  ((PVOID)__HEAP_SEGMENT_DATA_PTR(psegment));                 /*  返回原始内存点              */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
