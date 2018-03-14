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
** 文   件   名: RegionDelete.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 01 月 17 日
**
** 描        述: 删除一个内存分区

** BUG
2008.01.13  加入 _DebugHandle() 功能。
2008.02.05  使用新的_HeapDelete()来删除堆控制块.
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock();
2011.07.18  修正 log 打印错误.
2011.07.29  加入对象创建/销毁回调.
2012.12.06  加入强制删除功能.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_RegionDeleteEx
** 功能描述: 删除一个内存分区
** 输　入  : 
**           pulId                         REGION 句柄指针
**           bForce                        强制删除
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
#if (LW_CFG_REGION_EN > 0) && (LW_CFG_MAX_REGIONS > 0)

LW_API  
ULONG  API_RegionDeleteEx (LW_OBJECT_HANDLE   *pulId, BOOL  bForce)
{
    REGISTER PLW_CLASS_HEAP            pheap;
    REGISTER UINT16                    usIndex;
    REGISTER LW_OBJECT_HANDLE          ulId;
    
    ulId = *pulId;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
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
    
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    if (_HeapDelete(pheap, !bForce)) {                                  /*  释放堆控制块                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "region in use.\r\n");
        _ErrorHandle(ERROR_REGION_USED);
        return  (ERROR_REGION_USED);
    }
    
    _ObjectCloseId(pulId);                                              /*  清除句柄                    */
    
    __LW_OBJECT_DELETE_HOOK(ulId);
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "region \"%s\" has been delete.\r\n", pheap->HEAP_cHeapName);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_RegionDelete
** 功能描述: 删除一个内存分区
** 输　入  : 
**           pulId                         REGION 句柄指针
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API  
ULONG  API_RegionDelete (LW_OBJECT_HANDLE   *pulId)
{
    return  (API_RegionDeleteEx(pulId, LW_FALSE));
}

#endif                                                                  /*  (LW_CFG_REGION_EN > 0) &&   */
                                                                        /*  (LW_CFG_MAX_REGIONS > 0)    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
