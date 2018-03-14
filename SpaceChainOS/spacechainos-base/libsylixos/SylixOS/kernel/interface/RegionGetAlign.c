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
** 文   件   名: RegionGetAlign.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 06 月 17 日
**
** 描        述: 获得一个指定内存对齐关系的缓冲区.
                 主要用在特殊 DMA 设备和特殊内存算法. 例如某些 DMA 需要首地址 512 字节对齐的内存操作.
                 使用本函数就可以完成相关的操作, 可以在非 CACHE 区域建立一个内存堆...
                 
** BUG:
2009.03.26  升级对 iAlign 正确性判断的方法.
2011.03.05  分配内存时加入了跟踪函数名信息.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_RegionGetAlign
** 功能描述: 获得一个指定内存对齐关系的缓冲区
** 输　入  : 
**           ulId                         REGION 句柄
**           stByteSize                   分段大小
**           stAlign                      指定的内存对齐关系
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
#if (LW_CFG_REGION_EN > 0) && (LW_CFG_MAX_REGIONS > 0)

LW_API  
PVOID  API_RegionGetAlign (LW_OBJECT_HANDLE  ulId, size_t  stByteSize, size_t  stAlign)
{
    REGISTER PLW_CLASS_HEAP            pheap;
    REGISTER UINT16                    usIndex;
    REGISTER PVOID                     pvAllocate;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (LW_NULL);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!stByteSize) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "ulByteSize invalidate\r\n");
        _ErrorHandle(ERROR_REGION_SIZE);
        return  (LW_NULL);
    }
    
    if (!_ObjectClassOK(ulId, _OBJECT_REGION)) {                        /*  对象类型检查                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "region handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (LW_NULL);
    }
    
    if ((stAlign < sizeof(PVOID)) || (stAlign & (stAlign - 1))) {       /*  检查对齐关系                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "iAlign invalidate.\r\n");
        _ErrorHandle(ERROR_REGION_ALIGN);
        return  (LW_NULL);
    }
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (_Heap_Index_Invalid(usIndex)) {                                 /*  缓冲区索引检查              */
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "region handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (LW_NULL);
    }
#else
    __KERNEL_ENTER();                                                   /*  进入内核                    */
#endif

    pheap = &_K_heapBuffer[usIndex];

    __KERNEL_EXIT();                                                    /*  退出内核                    */

    pvAllocate = _HeapAllocateAlign(pheap, stByteSize, stAlign, __func__);
    
    if (!pvAllocate) {                                                  /*  是否分配成功                */
        _ErrorHandle(ERROR_REGION_NOMEM);
    }
    
    return  (pvAllocate);
}

#endif                                                                  /*  (LW_CFG_REGION_EN > 0) &&   */
                                                                        /*  (LW_CFG_MAX_REGIONS > 0)    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
