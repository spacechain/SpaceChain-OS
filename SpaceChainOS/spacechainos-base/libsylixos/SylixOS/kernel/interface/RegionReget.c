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
** 文   件   名: RegionReget.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 02 月 05 日
**
** 描        述: 重新获得一个内存缓冲区, 类似于 realloc() 操作

** BUG
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock();
2008.09.05  完全符合 realloc 规范.
2011.03.05  分配内存时加入了跟踪函数名信息.
2011.07.19  对参数的判断完全交由 _HeapRealloc() 处理.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_RegionReget
** 功能描述: 重新获得一个内存缓冲区
** 输　入  : 
**           ulId                         REGION 句柄
**           pvOldMem                     原先分配的内存
**           stNewByteSize                新的分段大小
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
#if (LW_CFG_REGION_EN > 0) && (LW_CFG_MAX_REGIONS > 0)

LW_API  
PVOID  API_RegionReget (LW_OBJECT_HANDLE    ulId, 
                        PVOID               pvOldMem, 
                        size_t              stNewByteSize)
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
    if (!_ObjectClassOK(ulId, _OBJECT_REGION)) {                        /*  对象类型检查                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "region handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
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
    
    pvAllocate = _HeapRealloc(pheap, pvOldMem, stNewByteSize, LW_TRUE, __func__);
    
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
