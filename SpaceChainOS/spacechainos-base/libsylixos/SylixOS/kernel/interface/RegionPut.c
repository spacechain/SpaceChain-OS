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
** 文   件   名: RegionPut.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 01 月 22 日
**
** 描        述: 释放一个内存缓冲区

** BUG
2007.07.25  加入强制内存检查
2008.01.13  加入 _DebugHandle() 功能。
2008.01.24  加入新型的内存管理.
2008.02.05  提前了解锁调度器的时间.
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock();
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_RegionPut
** 功能描述: 释放一个内存缓冲区
** 输　入  : 
**           ulId                         REGION 句柄
**           pvSegmentData                分段指针
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if (LW_CFG_REGION_EN > 0) && (LW_CFG_MAX_REGIONS > 0)

LW_API  
PVOID  API_RegionPut (LW_OBJECT_HANDLE  ulId, PVOID  pvSegmentData)
{
    REGISTER PLW_CLASS_HEAP            pheap;
    REGISTER UINT16                    usIndex;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (pvSegmentData);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!pvSegmentData) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pvSegmentData invalidate\r\n");
        _ErrorHandle(ERROR_REGION_NULL);
        return  (pvSegmentData);
    }
    
    if (!_ObjectClassOK(ulId, _OBJECT_REGION)) {                        /*  对象类型检查                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "region handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (pvSegmentData);
    }
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (_Heap_Index_Invalid(usIndex)) {                                 /*  缓冲区索引检查              */
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "region handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (pvSegmentData);
    }
#else
    __KERNEL_ENTER();                                                   /*  进入内核                    */
#endif

    pheap = &_K_heapBuffer[usIndex];
    
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
#if LW_CFG_USER_HEAP_SAFETY > 0
    pvSegmentData = _HeapFree(pheap, pvSegmentData, LW_TRUE, __func__); /*  带有内存检查的交还          */
#else
    pvSegmentData = _HeapFree(pheap, pvSegmentData, LW_FALSE, __func__);
#endif                                                                  /*  LW_CFG_USER_HEAP_SAFETY     */
    
    if (pvSegmentData) {
        _ErrorHandle(ERROR_REGION_NULL);
    }
    
    return  (pvSegmentData);
}

#endif                                                                  /*  (LW_CFG_REGION_EN > 0) &&   */
                                                                        /*  (LW_CFG_MAX_REGIONS > 0)    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
