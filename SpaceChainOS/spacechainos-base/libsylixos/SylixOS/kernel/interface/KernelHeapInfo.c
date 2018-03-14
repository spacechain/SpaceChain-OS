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
** 文   件   名: KernelHeapInfo.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 14 日
**
** 描        述: 系统内核堆状态查询

** BUG
2007.11.07  将 LW_OPTION_HEAP_USR 给为 LW_OPTION_HEAP_KERNEL.
            将 用户堆命名为 内核堆.
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock();
2009.09.28  修改注释.
2013.03.30  将 HeapInfoEx 扩展接口放在此处.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_KernelHeapInfo
** 功能描述: 系统内核堆状态查询
** 输　入  : 
**           ulOption             堆选择选项 LW_OPTION_HEAP_KERNEL LW_OPTION_HEAP_SYSTEM
**           pstByteSize          堆总大小，字节数
**           pulSegmentCounter    堆分段数
**           pstUsedByteSize      堆使用的字节数
**           pstFreeByteSize      堆空闲的字节数
**           pstMaxUsedByteSize   堆空闲最大使用量
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_KernelHeapInfo (ULONG    ulOption, 
                           size_t  *pstByteSize,
                           ULONG   *pulSegmentCounter,
                           size_t  *pstUsedByteSize,
                           size_t  *pstFreeByteSize,
                           size_t  *pstMaxUsedByteSize)
{
    switch (ulOption) {
    
    case LW_OPTION_HEAP_KERNEL:
        __KERNEL_ENTER();                                               /*  进入内核                    */
        if (pstByteSize) {
            *pstByteSize = _K_pheapKernel->HEAP_stTotalByteSize;
        }
        if (pulSegmentCounter) {
            *pulSegmentCounter = _K_pheapKernel->HEAP_ulSegmentCounter;
        }
        if (pstUsedByteSize) {
            *pstUsedByteSize = _K_pheapKernel->HEAP_stUsedByteSize;
        }
        if (pstFreeByteSize) {
            *pstFreeByteSize = _K_pheapKernel->HEAP_stFreeByteSize;
        }
        if (pstMaxUsedByteSize) {
            *pstMaxUsedByteSize = _K_pheapKernel->HEAP_stMaxUsedByteSize;
        }
        __KERNEL_EXIT();                                                /*  退出内核                    */
        return  (ERROR_NONE);
        
    case LW_OPTION_HEAP_SYSTEM:
        if (_K_pheapKernel == _K_pheapSystem) {
            _ErrorHandle(ERROR_KERNEL_OPTION);
            return  (ERROR_KERNEL_OPTION);
        }
        __KERNEL_ENTER();                                               /*  进入内核                    */
        if (pstByteSize) {
            *pstByteSize = _K_pheapSystem->HEAP_stTotalByteSize;
        }
        if (pulSegmentCounter) {
            *pulSegmentCounter = _K_pheapSystem->HEAP_ulSegmentCounter;
        }
        if (pstUsedByteSize) {
            *pstUsedByteSize = _K_pheapSystem->HEAP_stUsedByteSize;
        }
        if (pstFreeByteSize) {
            *pstFreeByteSize = _K_pheapSystem->HEAP_stFreeByteSize;
        }
        if (pstMaxUsedByteSize) {
            *pstMaxUsedByteSize = _K_pheapSystem->HEAP_stMaxUsedByteSize;
        }
        __KERNEL_EXIT();                                                /*  退出内核                    */
        return  (ERROR_NONE);
        
    default:
        _ErrorHandle(ERROR_KERNEL_OPTION);
        return  (ERROR_KERNEL_OPTION);
    }
}
/*********************************************************************************************************
** 函数名称: KernelHeapInfoEx
** 功能描述: 系统内核堆状态查询:高级接口
** 输　入  : 
**           ulOption             堆选择选项 LW_OPTION_HEAP_KERNEL LW_OPTION_HEAP_SYSTEM
**           pstByteSize          堆总大小，字节数
**           pulSegmentCounter    堆分段数
**           pstUsedByteSize      堆使用的字节数
**           pstFreeByteSize      堆空闲的字节数
**           psegmentList[]       分段头地点表
**           iMaxCounter          分段头地点表所能保存的最大分段数.
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_KernelHeapInfoEx (ULONG                ulOption, 
                             size_t              *pstByteSize,
                             ULONG               *pulSegmentCounter,
                             size_t              *pstUsedByteSize,
                             size_t              *pstFreeByteSize,
                             size_t              *pstMaxUsedByteSize,
                             PLW_CLASS_SEGMENT    psegmentList[],
                             INT                  iMaxCounter)
{
    switch (ulOption) {
    
    case LW_OPTION_HEAP_KERNEL:
        __KERNEL_ENTER();                                               /*  进入内核                    */
        if (pstByteSize) {
            *pstByteSize = _K_pheapKernel->HEAP_stTotalByteSize;
        }
        if (pulSegmentCounter) {
            *pulSegmentCounter = _K_pheapKernel->HEAP_ulSegmentCounter;
        }
        if (pstUsedByteSize) {
            *pstUsedByteSize = _K_pheapKernel->HEAP_stUsedByteSize;
        }
        if (pstFreeByteSize) {
            *pstFreeByteSize = _K_pheapKernel->HEAP_stFreeByteSize;
        }
        if (pstMaxUsedByteSize) {
            *pstMaxUsedByteSize = _K_pheapKernel->HEAP_stMaxUsedByteSize;
        }
        __KERNEL_EXIT();                                                /*  退出内核                    */
        
        if (psegmentList) {
            (VOID)_HeapGetInfo(_K_pheapKernel, psegmentList, iMaxCounter);
        }
        return  (ERROR_NONE);
        
    case LW_OPTION_HEAP_SYSTEM:
        if (_K_pheapKernel == _K_pheapSystem) {
            _ErrorHandle(ERROR_KERNEL_OPTION);
            return  (ERROR_KERNEL_OPTION);
        }
        __KERNEL_ENTER();                                               /*  进入内核                    */
        if (pstByteSize) {
            *pstByteSize = _K_pheapSystem->HEAP_stTotalByteSize;
        }
        if (pulSegmentCounter) {
            *pulSegmentCounter = _K_pheapSystem->HEAP_ulSegmentCounter;
        }
        if (pstUsedByteSize) {
            *pstUsedByteSize = _K_pheapSystem->HEAP_stUsedByteSize;
        }
        if (pstFreeByteSize) {
            *pstFreeByteSize = _K_pheapSystem->HEAP_stFreeByteSize;
        }
        if (pstMaxUsedByteSize) {
            *pstMaxUsedByteSize = _K_pheapSystem->HEAP_stMaxUsedByteSize;
        }
        __KERNEL_EXIT();                                                /*  退出内核                    */
        
        if (psegmentList) {
            (VOID)_HeapGetInfo(_K_pheapSystem, psegmentList, iMaxCounter);
        }
        return  (ERROR_NONE);
        
    default:
        _ErrorHandle(ERROR_KERNEL_OPTION);
        return  (ERROR_KERNEL_OPTION);
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
