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
** 文   件   名: _HeapInit.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 07 月 21 日
**
** 描        述: 堆内存初始化

** BUG
2007.01.10  没有初始化 index
2007.07.21  加入 _DebugHandle() 信息功能。
2007.11.07  将用户堆命名为内核堆.
2009.06.30  加入相关的裁剪限制.
2009.07.28  加入对自旋锁的初始化.
2009.11.23  加入名字.
2009.12.11  修正注释.
2013.11.14  使用对象资源管理器结构管理空闲资源.
2017.12.01  加入 cdump 初始化.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  使用自动堆配置时, 内核堆和系统堆的内存.
*********************************************************************************************************/
#if LW_CFG_MEMORY_HEAP_CONFIG_TYPE == 0
#if LW_CFG_MEMORY_KERNEL_HEAP_ADDRESS == 0
static LW_STACK _K_stkKernelHeap[LW_CFG_MEMORY_KERNEL_HEAP_SIZE_BYTE / sizeof(LW_STACK)];
#endif
#if LW_CFG_MEMORY_SYSTEM_HEAP_ADDRESS == 0
static LW_STACK _K_stkSystemHeap[LW_CFG_MEMORY_SYSTEM_HEAP_SIZE_BYTE / sizeof(LW_STACK)];
#endif
#endif                                                                  /*  LW_CFG_MEMORY_HEAP_CONFIG_..*/
/*********************************************************************************************************
** 函数名称: _HeapInit
** 功能描述: 堆内存初始化
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _HeapInit (VOID)
{
    REGISTER ULONG                  ulI;
    REGISTER PLW_LIST_MONO          pmonoTemp1;
    REGISTER PLW_LIST_MONO          pmonoTemp2;
    REGISTER PLW_CLASS_HEAP         heapTemp1;
    REGISTER PLW_CLASS_HEAP         heapTemp2;
    
    _K_resrcHeap.RESRC_pmonoFreeHeader = &_K_heapBuffer[0].HEAP_monoResrcList;
    
    heapTemp1 = &_K_heapBuffer[0];                                      /*  指向缓冲池首地址            */
    heapTemp2 = &_K_heapBuffer[1];

    for (ulI = 0; ulI < (LW_CFG_MAX_REGIONS + 1); ulI++) {              /*  LW_CFG_MAX_REGIONS + 2 个   */
        pmonoTemp1 = &heapTemp1->HEAP_monoResrcList;
        pmonoTemp2 = &heapTemp2->HEAP_monoResrcList;
        
        heapTemp1->HEAP_usIndex = (UINT16)ulI;
        LW_SPIN_INIT(&heapTemp1->HEAP_slLock);
        
        _LIST_MONO_LINK(pmonoTemp1, pmonoTemp2);
        
        heapTemp1++;
        heapTemp2++;
    }
    
    heapTemp1->HEAP_usIndex = (UINT16)ulI;
    LW_SPIN_INIT(&heapTemp1->HEAP_slLock);
    
    pmonoTemp1 = &heapTemp1->HEAP_monoResrcList;
    
    _INIT_LIST_MONO_HEAD(pmonoTemp1);
    
    _K_resrcHeap.RESRC_pmonoFreeTail = pmonoTemp1;
    
    _K_resrcHeap.RESRC_uiUsed    = 0;
    _K_resrcHeap.RESRC_uiMaxUsed = 0;
}
/*********************************************************************************************************
** 函数名称: _HeapKernelInit
** 功能描述: 内核堆内存初始化
** 输　入  : pvKernelHeapMem   内核堆的起始地址
**           stKernelHeapSize  内核堆的大小
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_MEMORY_HEAP_CONFIG_TYPE > 0
VOID  _HeapKernelInit (PVOID     pvKernelHeapMem,
                       size_t    stKernelHeapSize)
#else
VOID  _HeapKernelInit (VOID)
#endif                                                                  /*  LW_CFG_MEMORY_HEAP_...      */
{
    PVOID   pvHeap;
    size_t  stSize;

#if LW_CFG_MEMORY_HEAP_CONFIG_TYPE > 0
    pvHeap = pvKernelHeapMem;
    stSize = stKernelHeapSize;
    
    _K_pheapKernel = _HeapCreate(LW_KERNEL_HEAP_START(pvKernelHeapMem), 
                                 LW_KERNEL_HEAP_SIZE(stKernelHeapSize));
#else
#if LW_CFG_MEMORY_KERNEL_HEAP_ADDRESS == 0
    pvHeap = (PVOID)_K_stkKernelHeap;
    stSize = LW_CFG_MEMORY_KERNEL_HEAP_SIZE_BYTE;

    _K_pheapKernel = _HeapCreate(LW_KERNEL_HEAP_START(_K_stkKernelHeap), 
                                 LW_KERNEL_HEAP_SIZE(LW_CFG_MEMORY_KERNEL_HEAP_SIZE_BYTE));
#else
    pvHeap = (PVOID)LW_CFG_MEMORY_KERNEL_HEAP_ADDRESS;
    stSize = LW_CFG_MEMORY_KERNEL_HEAP_SIZE_BYTE;

    _K_pheapKernel = _HeapCreate(LW_KERNEL_HEAP_START(LW_CFG_MEMORY_KERNEL_HEAP_ADDRESS), 
                                 LW_KERNEL_HEAP_SIZE(LW_CFG_MEMORY_KERNEL_HEAP_SIZE_BYTE));
#endif
#endif                                                                  /*  LW_CFG_MEMORY_HEAP_...      */

#if (LW_CFG_CDUMP_EN > 0) && (LW_CFG_DEVICE_EN > 0)
    _CrashDumpSet(LW_KERNEL_CDUMP_START(pvHeap, stSize), LW_CFG_CDUMP_BUF_SIZE);
#endif                                                                  /*  LW_CFG_CDUMP_EN > 0         */
                                                                        /*  LW_CFG_DEVICE_EN > 0        */
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)
#if LW_CFG_ERRORMESSAGE_EN > 0 || LW_CFG_LOGMESSAGE_EN > 0
#if LW_CFG_MEMORY_HEAP_CONFIG_TYPE > 0
    _DebugFormat(__LOGMESSAGE_LEVEL, "kernel heap has been create 0x%lx (%zd Bytes).\r\n",
                 (addr_t)pvKernelHeapMem, stKernelHeapSize);
#else
    _DebugFormat(__LOGMESSAGE_LEVEL, "kernel heap has been create 0x%lx (%zd Bytes).\r\n",
                 (LW_CFG_MEMORY_KERNEL_HEAP_ADDRESS == 0) ? 
                 (addr_t)_K_stkKernelHeap : (addr_t)LW_CFG_MEMORY_KERNEL_HEAP_ADDRESS,
                 (size_t)LW_CFG_MEMORY_KERNEL_HEAP_SIZE_BYTE);
#endif                                                                  /*  LW_CFG_MEMORY_HEAP_...      */
#endif                                                                  /*  LW_CFG_ERRORMESSAGE_EN...   */
#endif                                                                  /*  LW_CFG_DEVICE_EN...         */

    lib_strcpy(_K_pheapKernel->HEAP_cHeapName, "kernel");
}
/*********************************************************************************************************
** 函数名称: _HeapSystemInit
** 功能描述: 系统堆内存初始化
** 输　入  : pvSystemHeapMem   系统堆的起始地址
**           stSystemHeapSize  系统堆的大小
** 输　出  : NONE
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_MEMORY_HEAP_CONFIG_TYPE > 0
VOID  _HeapSystemInit (PVOID     pvSystemHeapMem,
                       size_t    stSystemHeapSize)
#else
VOID  _HeapSystemInit (VOID)
#endif                                                                  /*  LW_CFG_MEMORY_HEAP_...      */
{
#if LW_CFG_MEMORY_HEAP_CONFIG_TYPE > 0
    if (pvSystemHeapMem && stSystemHeapSize) {
        _K_pheapSystem = _HeapCreate(pvSystemHeapMem, stSystemHeapSize);
    } else
#else
#if LW_CFG_MEMORY_SYSTEM_HEAP_ADDRESS == 0
    if (LW_CFG_MEMORY_SYSTEM_HEAP_SIZE_BYTE) {
        _K_pheapSystem = _HeapCreate((PVOID)_K_stkSystemHeap, LW_CFG_MEMORY_SYSTEM_HEAP_SIZE_BYTE);
    } else
#else
    if (LW_CFG_MEMORY_SYSTEM_HEAP_SIZE_BYTE) {
        _K_pheapSystem = _HeapCreate((PVOID)LW_CFG_MEMORY_SYSTEM_HEAP_ADDRESS, 
                                     LW_CFG_MEMORY_SYSTEM_HEAP_SIZE_BYTE);
    } else
#endif
#endif                                                                  /*  LW_CFG_MEMORY_HEAP_...      */
    {
        _K_pheapSystem = _K_pheapKernel;                                /*  只使用 kernel heap          */
    }

#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)
#if (LW_CFG_ERRORMESSAGE_EN > 0) || (LW_CFG_LOGMESSAGE_EN > 0)
#if LW_CFG_MEMORY_HEAP_CONFIG_TYPE > 0
    _DebugFormat(__LOGMESSAGE_LEVEL, "system heap has been create 0x%lx (%zd Bytes).\r\n",
                 (addr_t)pvSystemHeapMem, stSystemHeapSize);
#else
    _DebugFormat(__LOGMESSAGE_LEVEL, "system heap has been create 0x%lx (%zd Bytes).\r\n",
                 (LW_CFG_MEMORY_SYSTEM_HEAP_ADDRESS == 0) ? 
                 (addr_t)_K_stkSystemHeap : (addr_t)LW_CFG_MEMORY_SYSTEM_HEAP_ADDRESS,
                 (size_t)LW_CFG_MEMORY_SYSTEM_HEAP_SIZE_BYTE);
#endif                                                                  /*  LW_CFG_MEMORY_HEAP_...      */
#endif                                                                  /*  LW_CFG_ERRORMESSAGE_EN...   */
#endif                                                                  /*  LW_CFG_DEVICE_EN...         */

    if (_K_pheapSystem == _K_pheapKernel) {
        lib_strcpy(_K_pheapSystem->HEAP_cHeapName, "kersys");
    
    } else {
        lib_strcpy(_K_pheapSystem->HEAP_cHeapName, "system");
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
