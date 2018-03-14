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
** 文   件   名: vmmStack.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2017 年 06 月 01 日
**
** 描        述: 应用程序堆栈管理.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "phyPage.h"
#include "virPage.h"
/*********************************************************************************************************
** 函数名称: API_VmmStackAlloc
** 功能描述: 分配任务堆栈内存.
** 输　入  : stSize     需要分配的内存大小
** 输　出  : 虚拟内存首地址
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
PVOID  API_VmmStackAlloc (size_t  stSize)
{
    REGISTER PLW_VMM_PAGE   pvmpageVirtual;
    REGISTER PLW_VMM_PAGE   pvmpagePhysical;
    
    REGISTER ULONG          ulPageNum = (ULONG) (stSize >> LW_CFG_VMM_PAGE_SHIFT);
    REGISTER size_t         stExcess  = (size_t)(stSize & ~LW_CFG_VMM_PAGE_MASK);
             size_t         stAlign   = LW_CFG_VMM_PAGE_SIZE;
    
             ULONG          ulZoneIndex;
             ULONG          ulPageNumTotal = 0;
             ULONG          ulVirtualAddr;
             ULONG          ulError;
    
#if LW_CFG_CACHE_EN > 0
    if (API_CacheAliasProb() && 
        (stAlign < API_CacheWaySize(DATA_CACHE))) {                     /*  有限修正 cache alias        */
        stAlign = API_CacheWaySize(DATA_CACHE);
    }
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
    
    if (stExcess) {
        ulPageNum++;                                                    /*  确定分页数量                */
    }

    if (ulPageNum < 1) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }
    
    __VMM_LOCK();
    if (stAlign > LW_CFG_VMM_PAGE_SIZE) {                               /*  多分配一个虚拟页面作为保护  */
        pvmpageVirtual = __vmmVirtualPageAllocAlign(ulPageNum + 1, stAlign);
    } else {
        pvmpageVirtual = __vmmVirtualPageAlloc(ulPageNum + 1);
    }
    if (pvmpageVirtual == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);
        return  (LW_NULL);
    }
    
#if CPU_STK_GROWTH == 0
    ulVirtualAddr = pvmpageVirtual->PAGE_ulPageAddr;                    /*  起始虚拟内存地址            */
#else
    ulVirtualAddr = pvmpageVirtual->PAGE_ulPageAddr + LW_CFG_VMM_PAGE_SIZE;
#endif

    do {
        ULONG   ulPageNumOnce = ulPageNum - ulPageNumTotal;
        ULONG   ulMinContinue = __vmmPhysicalPageGetMinContinue(&ulZoneIndex, LW_ZONE_ATTR_NONE);
                                                                        /*  优先分配碎片页面            */
    
        if (ulPageNumOnce > ulMinContinue) {                            /*  选择适当的页面长度          */
            ulPageNumOnce = ulMinContinue;
        }
    
        pvmpagePhysical = __vmmPhysicalPageAllocZone(ulZoneIndex, ulPageNumOnce, LW_ZONE_ATTR_NONE);
        if (pvmpagePhysical == LW_NULL) {
            _ErrorHandle(ERROR_VMM_LOW_PHYSICAL_PAGE);
            goto    __error_handle;
        }
        
        ulError = __vmmLibPageMap(pvmpagePhysical->PAGE_ulPageAddr,     /*  使用 CACHE                  */
                                  ulVirtualAddr,
                                  ulPageNumOnce, 
                                  LW_VMM_FLAG_RDWR);                    /*  映射为连续虚拟地址          */
        if (ulError) {                                                  /*  映射错误                    */
            __vmmPhysicalPageFree(pvmpagePhysical);
            _ErrorHandle(ulError);
            goto    __error_handle;
        }
        
        pvmpagePhysical->PAGE_ulMapPageAddr = ulVirtualAddr;
        pvmpagePhysical->PAGE_ulFlags = LW_VMM_FLAG_RDWR;
        
        __pageLink(pvmpageVirtual, pvmpagePhysical);                    /*  将物理页面连接入虚拟空间    */
        
        ulPageNumTotal += ulPageNumOnce;
        ulVirtualAddr  += (ulPageNumOnce << LW_CFG_VMM_PAGE_SHIFT);
        
    } while (ulPageNumTotal < ulPageNum);
    
    pvmpageVirtual->PAGE_ulFlags = LW_VMM_FLAG_RDWR;
    __areaVirtualInsertPage(pvmpageVirtual->PAGE_ulPageAddr, 
                            pvmpageVirtual);                            /*  插入逻辑空间反查表          */
    __VMM_UNLOCK();
    
    MONITOR_EVT_LONG4(MONITOR_EVENT_ID_VMM, MONITOR_EVENT_VMM_ALLOC,
                      pvmpageVirtual->PAGE_ulPageAddr, 
                      stSize, stAlign, LW_VMM_FLAG_RDWR, LW_NULL);
    
#if CPU_STK_GROWTH == 0
    return  ((PVOID)(pvmpageVirtual->PAGE_ulPageAddr));                 /*  返回虚拟地址                */
#else
    return  ((PVOID)(pvmpageVirtual->PAGE_ulPageAddr + LW_CFG_VMM_PAGE_SIZE));
#endif

__error_handle:                                                         /*  出现错误                    */
    __vmmPhysicalPageFreeAll(pvmpageVirtual);                           /*  释放页面链表                */
    __vmmVirtualPageFree(pvmpageVirtual);                               /*  释放虚拟地址空间            */
    __VMM_UNLOCK();
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: API_VmmStackFree
** 功能描述: 释放任务堆栈内存
** 输　入  : pvVirtualMem    连续虚拟地址
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_VmmStackFree (PVOID  pvVirtualMem)
{
#if CPU_STK_GROWTH == 0
    API_VmmFreeArea(pvVirtualMem);
#else
    API_VmmFreeArea((PVOID)((addr_t)pvVirtualMem - LW_CFG_VMM_PAGE_SIZE));
#endif
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
