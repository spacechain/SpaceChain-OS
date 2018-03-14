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
** 文   件   名: mosipc_heap.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2014 年 10 月 28 日
**
** 描        述: 多操作系统通信内存堆.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if LW_CFG_MOSIPC_EN > 0
/*********************************************************************************************************
** 函数名称: __mosipcHeapInit
** 功能描述: 初始化 MOSIPC 内存堆.
** 输　入  : pheapToBuild          需要创建的堆
**           pvStartAddress        起始内存地址
**           stByteSize            内存堆的大小
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __mosipcHeapInit (PLW_CLASS_HEAP    pheapToBuild,
                       PVOID             pvStartAddress, 
                       size_t            stByteSize)
{
    if (_HeapCtorEx(pheapToBuild, pvStartAddress, stByteSize, LW_TRUE)) {
        return  (ERROR_NONE);
    
    } else {
        return  (PX_ERROR);
    }
    
    lib_strlcpy(pheapToBuild->HEAP_cHeapName, "mosipc_heap", LW_CFG_OBJECT_NAME_SIZE);
}
/*********************************************************************************************************
** 函数名称: __mosipcHeapAlloc
** 功能描述: 从 MOSIPC 内存堆分配内存.
** 输　入  : pheap              内存堆
**           stByteSize         分配的字节数
**           stAlign            内存对齐关系
**           pcPurpose          分配内存的用途
** 输　出  : 分配的内存
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PVOID  __mosipcHeapAlloc (PLW_CLASS_HEAP    pheap,
                          size_t            stByteSize, 
                          size_t            stAlign, 
                          CPCHAR            pcPurpose)
{
    return  (_HeapAllocateAlign(pheap, stByteSize, stAlign, pcPurpose));
}
/*********************************************************************************************************
** 函数名称: __mosipcHeapFree
** 功能描述: 向 MOSIPC 内存堆释放内存.
** 输　入  : pheap              堆控制块
**           pvStartAddress     归还的地址
**           pcPurpose          谁释放内存
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __mosipcHeapFree (PLW_CLASS_HEAP    pheap,
                       PVOID             pvStartAddress,
                       CPCHAR            pcPurpose)
{
    if (_HeapFree(pheap, pvStartAddress, LW_FALSE, pcPurpose)) {
        return  (PX_ERROR);
    
    } else {
        return  (ERROR_NONE);
    }
}

#endif                                                                  /*  LW_CFG_MOSIPC_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
