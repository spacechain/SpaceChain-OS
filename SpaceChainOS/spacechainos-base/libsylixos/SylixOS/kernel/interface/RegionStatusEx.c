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
** 文   件   名: RegionStatusEx.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 04 月 12 日
**
** 描        述: 获得一个内存缓冲区状态:高级接口

** BUG
2008.01.13  加入 _DebugHandle() 功能。
2008.01.24  加入了新型内存管理的函数.
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock();
2009.05.21  加入对最大使用量的获取.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_RegionStatusEx
** 功能描述: 获得一个内存缓冲区状态高级接口
** 输　入  : 
**           ulId                         REGION 句柄
**           pstByteSize                  REGION 大小    可以为 NULL
**           pulSegmentCounter            分段数量       可以为 NULL
**           pstUsedByteSize              使用的字节数   可以为 NULL
**           pstFreeByteSize              空闲的字节数   可以为 NULL
**           pstMaxUsedByteSize           最大的分配量   可以为 NULL
**           psegmentList[]               分段头地址表   可以为 NULL
**           iMaxCounter                  分段头地址表最多可以保存的数量
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if (LW_CFG_REGION_EN > 0) && (LW_CFG_MAX_REGIONS > 0)

LW_API  
ULONG  API_RegionStatusEx (LW_OBJECT_HANDLE    ulId,
                           size_t             *pstByteSize,
                           ULONG              *pulSegmentCounter,
                           size_t             *pstUsedByteSize,
                           size_t             *pstFreeByteSize,
                           size_t             *pstMaxUsedByteSize,
                           PLW_CLASS_SEGMENT   psegmentList[],
                           INT                 iMaxCounter)
{
    REGISTER PLW_CLASS_HEAP            pheap;
    REGISTER UINT16                    usIndex;
    
    usIndex = _ObjectGetIndex(ulId);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_REGION)) {                        /*  对象类型检查                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "region handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (_Heap_Index_Invalid(usIndex)) {                                 /*  缓冲区索引检查              */
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "region handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
#else
    __KERNEL_ENTER();                                                   /*  进入内核                    */
#endif

    pheap = &_K_heapBuffer[usIndex];
    
    if (pstByteSize) {
        *pstByteSize = pheap->HEAP_stTotalByteSize;
    }
    
    if (pulSegmentCounter) {
        *pulSegmentCounter = pheap->HEAP_ulSegmentCounter;
    }
    
    if (pstUsedByteSize) {
        *pstUsedByteSize = pheap->HEAP_stUsedByteSize;
    }
    
    if (pstFreeByteSize) {
        *pstFreeByteSize = pheap->HEAP_stFreeByteSize;
    }
    
    if (pstMaxUsedByteSize) {
        *pstMaxUsedByteSize = pheap->HEAP_stMaxUsedByteSize;
    }

    if (psegmentList) {
        _HeapGetInfo(pheap, psegmentList, iMaxCounter);
    }
    
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_REGION_EN > 0)      */
                                                                        /*  (LW_CFG_MAX_REGIONS > 0)    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
