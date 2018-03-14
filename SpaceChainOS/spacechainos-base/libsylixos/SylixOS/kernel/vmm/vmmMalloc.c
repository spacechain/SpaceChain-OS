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
** 文   件   名: vmmMalloc.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2011 年 05 月 16 日
**
** 描        述: 平台无关虚拟内存分配与回收管理.
**
** 注        意: 当发生缺页中断时, 所有的填充I/O操作(交换区或mmap填充)都是在 vmm lock 下进行的!
                 SylixOS 处理缺页异常是在任务上下文中, 通过陷阱程序完成, 所以, 不影响无关任务的调度与运行.
                 但是也带来了诸多问题. 如多任务可能在同一地址发生异常等等, 这里就是用的一些方法基本解决了
                 这个问题.

** BUG:
2011.05.19  当发生内存访问失效时, 如果物理内存存在, 则判断读写权限, 如读写权限正常则直接退出.
            (有可能为多线程同时访问同一段内存, 一个线程已经分配的物理内存, 另一个则直接退出即可).
            当读写权限错误, 则直接非法内存访问处理.
2011.07.28  加入 API_VmmRemapArea() 函数, 类似于 linux remap_pfn_range() 方法, 驱动程序可以使用此函数实现
            mmap 接口.
2011.11.09  加入 API_VmmPCountInArea() 可查看虚拟空间的缺页中断分配情况.
2011.11.18  加入对 dtrace 的支持.
2011.12.08  中断中发生异常, 直接重新启动操作系统.
            异常处理如果在不合时宜的时间发生, 此时需要打印诸多调试信息.
2011.12.10  在取消映射关系或者改变映射关系属性时, 需要回写并无效 cache.
2012.04.26  当产生错误异常时, 打印寄存器情况.
2012.05.07  使用 OSStkSetFp() 函数类同步 fp 指针, 保证 unwind 堆栈的正确性.
2012.09.05  将异常处理相关代码放入 vmmAbort.c 文件中.
2013.09.13  API_VmmShareArea() 如果存在的共享页面发生改变或者并且具有 MAP_PRIVATE 选项, 则应该重启分配.
2013.09.14  加入 API_VmmSetFindShare() 功能.
2013.12.20  API_VmmShareArea() 目标页面不存在时, 将开辟新的物理页面.
2015.05.26  加入 API_VmmMergeArea() 合并虚拟空间功能.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "vmmSwap.h"
#include "phyPage.h"
#include "virPage.h"
#ifndef printk
#define printk
#endif                                                                  /*  printk                      */
/*********************************************************************************************************
** 函数名称: __vmmPCountInAreaHook
** 功能描述: API_VmmCounterArea 回调函数
** 输　入  : pvmpagePhysical   pvmpageVirtual 内的一个物理页面控制块
**           pulCounter        计数变量
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __vmmPCountInAreaHook (PLW_VMM_PAGE  pvmpagePhysical,
                                    ULONG        *pulCounter)
{
    PLW_VMM_PAGE  pvmpageReal;
    
    if (LW_VMM_PAGE_IS_FAKE(pvmpagePhysical)) {
        pvmpageReal  = LW_VMM_PAGE_GET_REAL(pvmpagePhysical);
        if (pvmpageReal->PAGE_ulRef == 1) {
            *pulCounter += pvmpagePhysical->PAGE_ulCount;
        }
    } else {
        *pulCounter += pvmpagePhysical->PAGE_ulCount;
    }
}
/*********************************************************************************************************
** 函数名称: __vmmInvalidateAreaHook
** 功能描述: API_VmmInvalidateArea 回调函数
** 输　入  : pvmpagePhysical   pvmpageVirtual 内的一个物理页面控制块
**           pvmpageVirtual    虚拟页面控制块
**           ulSubMem          内部起始地址
**           stSize            大小
**           pulCount          释放的个数
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __vmmInvalidateAreaHook (PLW_VMM_PAGE  pvmpagePhysical,
                                      PLW_VMM_PAGE  pvmpageVirtual, 
                                      addr_t        ulSubMem,
                                      size_t        stSize,
                                      ULONG        *pulCount)
{
    if ((pvmpagePhysical->PAGE_ulMapPageAddr >= ulSubMem) &&
        (pvmpagePhysical->PAGE_ulMapPageAddr < (ulSubMem + stSize))) {  /*  判断对应的虚拟内存范围      */
        
#if LW_CFG_CACHE_EN > 0
        __vmmPhysicalPageClear(pvmpagePhysical);
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
        __vmmLibPageMap(pvmpagePhysical->PAGE_ulMapPageAddr,
                        pvmpagePhysical->PAGE_ulMapPageAddr,
                        pvmpagePhysical->PAGE_ulCount,                  /*  一定是 1                    */
                        LW_VMM_FLAG_FAIL);                              /*  对应的虚拟内存不允许访问    */
                        
        __pageUnlink(pvmpageVirtual, pvmpagePhysical);                  /*  解除连接关系                */
                       
        __vmmPhysicalPageFree(pvmpagePhysical);                         /*  释放物理页面                */
        
        if (pulCount) {
            (*pulCount)++;
        }
    }
}
/*********************************************************************************************************
** 函数名称: __vmmSplitAreaHook
** 功能描述: API_VmmSplitArea 回调函数
** 输　入  : pvmpagePhysical       pvmpageVirtual 内的一个物理页面控制块
**           pvmpageVirtual        原始虚拟分区
**           pvmpageVirtualSplit   新拆分出的虚拟分区
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __vmmSplitAreaHook (PLW_VMM_PAGE  pvmpagePhysical,
                                 PLW_VMM_PAGE  pvmpageVirtual,
                                 PLW_VMM_PAGE  pvmpageVirtualSplit)
{
    REGISTER addr_t     ulSubMem = pvmpageVirtualSplit->PAGE_ulPageAddr;

    if (pvmpagePhysical->PAGE_ulMapPageAddr >= ulSubMem) {              /*  判断对应的虚拟内存范围      */
        __pageUnlink(pvmpageVirtual, pvmpagePhysical);                  /*  解除连接关系                */
        __pageLink(pvmpageVirtualSplit, pvmpagePhysical);
    }
}
/*********************************************************************************************************
** 函数名称: __vmmMergeAreaHook
** 功能描述: API_VmmMergeArea 回调函数
** 输　入  : pvmpagePhysical       pvmpageVirtualR 内的一个物理页面控制块
**           pvmpageVirtualL       虚拟分区低地址段
**           pvmpageVirtualR       虚拟分区高地址段
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __vmmMergeAreaHook (PLW_VMM_PAGE  pvmpagePhysical,
                                 PLW_VMM_PAGE  pvmpageVirtualL,
                                 PLW_VMM_PAGE  pvmpageVirtualR)
{
    __pageUnlink(pvmpageVirtualR, pvmpagePhysical);                     /*  解除连接关系                */
    __pageLink(pvmpageVirtualL, pvmpagePhysical);
}
/*********************************************************************************************************
** 函数名称: __vmmMoveAreaHook
** 功能描述: API_VmmMoveArea 回调函数
** 输　入  : pvmpagePhysical       pvmpageVirtualFrom 内的一个物理页面控制块
**           pvmpageVirtualL       目标虚拟分区
**           pvmpageVirtualR       源虚拟分区
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __vmmMoveAreaHook (PLW_VMM_PAGE  pvmpagePhysical,
                                PLW_VMM_PAGE  pvmpageVirtualTo,
                                PLW_VMM_PAGE  pvmpageVirtualFrom)
{
    addr_t  ulNewMap;
    ULONG   ulOff;
    
    __pageUnlink(pvmpageVirtualFrom, pvmpagePhysical);                  /*  解除连接关系                */
    
    ulOff = pvmpagePhysical->PAGE_ulMapPageAddr
          - pvmpageVirtualFrom->PAGE_ulPageAddr;                        /*  物理页面偏移量              */
          
    ulNewMap = pvmpageVirtualTo->PAGE_ulPageAddr
             + ulOff;                                                   /*  新的映射地址                */
             
    __vmmLibPageMap(pvmpagePhysical->PAGE_ulPageAddr, 
                    ulNewMap, 1, pvmpageVirtualTo->PAGE_ulFlags);       /*  建立新的映射关系            */
                    
    pvmpagePhysical->PAGE_ulMapPageAddr = ulNewMap;                     /*  记录新的映射关系            */
                    
    __pageLink(pvmpageVirtualTo, pvmpagePhysical);                      /*  连接到目标虚拟分区          */
}
/*********************************************************************************************************
** 函数名称: API_VmmMalloc
** 功能描述: 分配逻辑上连续, 物理可能不连续的内存.
** 输　入  : stSize     需要分配的内存大小
** 输　出  : 虚拟内存首地址
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
PVOID  API_VmmMalloc (size_t  stSize)
{
    return  (API_VmmMallocAlign(stSize, LW_CFG_VMM_PAGE_SIZE, LW_VMM_FLAG_RDWR));
}
/*********************************************************************************************************
** 函数名称: API_VmmMallocEx
** 功能描述: 分配逻辑上连续, 物理可能不连续的内存.
** 输　入  : stSize     需要分配的内存大小
**           ulFlag     访问属性. (必须为 LW_VMM_FLAG_TEXT 或 LW_VMM_FLAG_DATA)
** 输　出  : 虚拟内存首地址
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
PVOID  API_VmmMallocEx (size_t  stSize, ULONG  ulFlag)
{
    return  (API_VmmMallocAlign(stSize, LW_CFG_VMM_PAGE_SIZE, ulFlag));
}
/*********************************************************************************************************
** 函数名称: API_VmmMallocAlign
** 功能描述: 分配逻辑上连续, 物理可能不连续的内存. (虚拟地址满足对齐关系)
** 输　入  : stSize     需要分配的内存大小
**           stAlign    对齐关系
**           ulFlag     访问属性. (必须为 LW_VMM_FLAG_TEXT 或 LW_VMM_FLAG_DATA)
** 输　出  : 虚拟内存首地址
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
PVOID  API_VmmMallocAlign (size_t  stSize, size_t  stAlign, ULONG  ulFlag)
{
    REGISTER PLW_VMM_PAGE   pvmpageVirtual;
    REGISTER PLW_VMM_PAGE   pvmpagePhysical;
    
    REGISTER ULONG          ulPageNum = (ULONG) (stSize >> LW_CFG_VMM_PAGE_SHIFT);
    REGISTER size_t         stExcess  = (size_t)(stSize & ~LW_CFG_VMM_PAGE_MASK);
    
             ULONG          ulZoneIndex;
             ULONG          ulPageNumTotal = 0;
             ULONG          ulVirtualAddr;
             ULONG          ulError;
    
    if (stAlign & (stAlign - 1)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "iAlign invalidate.\r\n");
        _ErrorHandle(ERROR_VMM_ALIGN);
        return  (LW_NULL);
    }
    if (stAlign < LW_CFG_VMM_PAGE_SIZE) {
        stAlign = LW_CFG_VMM_PAGE_SIZE;
    }
    
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
    if (stAlign > LW_CFG_VMM_PAGE_SIZE) {
        pvmpageVirtual = __vmmVirtualPageAllocAlign(ulPageNum, stAlign);/*  分配连续虚拟页面            */
    } else {
        pvmpageVirtual = __vmmVirtualPageAlloc(ulPageNum);              /*  分配连续虚拟页面            */
    }
    if (pvmpageVirtual == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);
        return  (LW_NULL);
    }
    
    ulVirtualAddr = pvmpageVirtual->PAGE_ulPageAddr;                    /*  起始虚拟内存地址            */
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
                                  ulFlag);                              /*  映射为连续虚拟地址          */
        if (ulError) {                                                  /*  映射错误                    */
            __vmmPhysicalPageFree(pvmpagePhysical);
            _ErrorHandle(ulError);
            goto    __error_handle;
        }
        
        pvmpagePhysical->PAGE_ulMapPageAddr = ulVirtualAddr;
        pvmpagePhysical->PAGE_ulFlags = ulFlag;
        
        __pageLink(pvmpageVirtual, pvmpagePhysical);                    /*  将物理页面连接入虚拟空间    */
        
        ulPageNumTotal += ulPageNumOnce;
        ulVirtualAddr  += (ulPageNumOnce << LW_CFG_VMM_PAGE_SHIFT);
        
    } while (ulPageNumTotal < ulPageNum);
    
    pvmpageVirtual->PAGE_ulFlags = ulFlag;
    __areaVirtualInsertPage(pvmpageVirtual->PAGE_ulPageAddr, 
                            pvmpageVirtual);                            /*  插入逻辑空间反查表          */
    __VMM_UNLOCK();
    
    MONITOR_EVT_LONG4(MONITOR_EVENT_ID_VMM, MONITOR_EVENT_VMM_ALLOC,
                      pvmpageVirtual->PAGE_ulPageAddr, 
                      stSize, stAlign, ulFlag, LW_NULL);
    
    return  ((PVOID)pvmpageVirtual->PAGE_ulPageAddr);                   /*  返回虚拟地址                */
        
__error_handle:                                                         /*  出现错误                    */
    __vmmPhysicalPageFreeAll(pvmpageVirtual);                           /*  释放页面链表                */
    __vmmVirtualPageFree(pvmpageVirtual);                               /*  释放虚拟地址空间            */
    __VMM_UNLOCK();
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: API_VmmFree
** 功能描述: 释放连续虚拟内存
** 输　入  : pvVirtualMem    连续虚拟地址
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_VmmFree (PVOID  pvVirtualMem)
{
    API_VmmFreeArea(pvVirtualMem);
}
/*********************************************************************************************************
** 函数名称: API_VmmMallocArea
** 功能描述: 仅开辟虚拟空间, 当出现第一次访问时, 将通过缺页中断分配物理内存, 
             当缺页中断中无法分配物理页面时, 将收到 SIGSEGV 信号并结束线程. 
** 输　入  : stSize        需要分配的内存大小
**           pfuncFiller   当出现缺页时, 获取物理分页后的填充函数, 一般为 NULL.
**           pvArg         填充函数参数.
** 输　出  : 虚拟内存首地址
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
PVOID  API_VmmMallocArea (size_t  stSize, FUNCPTR  pfuncFiller, PVOID  pvArg)
{
    return  (API_VmmMallocAreaAlign(stSize, LW_CFG_VMM_PAGE_SIZE, 
                                    pfuncFiller, pvArg, LW_VMM_PRIVATE_CHANGE, LW_VMM_FLAG_RDWR));
}
/*********************************************************************************************************
** 函数名称: API_VmmMallocAreaEx
** 功能描述: 仅开辟虚拟空间, 当出现第一次访问时, 将通过缺页中断分配物理内存, 
             当缺页中断中无法分配物理页面时, 将收到 SIGSEGV 信号并结束线程. 
** 输　入  : stSize        需要分配的内存大小
**           pfuncFiller   当出现缺页时, 获取物理分页后的填充函数, 一般为 NULL.
**           pvArg         填充函数参数.
**           iFlag         MAP_SHARED or MAP_PRIVATE
**           ulFlag        访问属性. (必须为 LW_VMM_FLAG_TEXT 或 LW_VMM_FLAG_DATA)
** 输　出  : 虚拟内存首地址
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
PVOID  API_VmmMallocAreaEx (size_t  stSize, FUNCPTR  pfuncFiller, PVOID  pvArg, INT  iFlags, ULONG  ulFlag)
{
    return  (API_VmmMallocAreaAlign(stSize, LW_CFG_VMM_PAGE_SIZE, pfuncFiller, pvArg, iFlags, ulFlag));
}
/*********************************************************************************************************
** 函数名称: API_VmmMallocAreaAlign
** 功能描述: 仅开辟虚拟空间, 当出现第一次访问时, 将通过缺页中断分配物理内存, 
             当缺页中断中无法分配物理页面时, 将收到 SIGSEGV 信号并结束线程. 
** 输　入  : stSize        需要分配的内存大小
**           stAlign       对齐关系
**           pfuncFiller   当出现缺页时, 获取物理分页后的填充函数, 一般为 NULL.
**           pvArg         填充函数参数.
**           iFlag         MAP_SHARED or MAP_PRIVATE
**           ulFlag        访问属性. (必须为 LW_VMM_FLAG_TEXT 或 LW_VMM_FLAG_DATA)
** 输　出  : 虚拟内存首地址
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
PVOID  API_VmmMallocAreaAlign (size_t  stSize, size_t  stAlign, 
                               FUNCPTR  pfuncFiller, PVOID  pvArg, INT  iFlags, ULONG  ulFlag)
{
    REGISTER PLW_VMM_PAGE           pvmpageVirtual;
    REGISTER PLW_VMM_PAGE_PRIVATE   pvmpagep;
    
    REGISTER ULONG          ulPageNum = (ULONG) (stSize >> LW_CFG_VMM_PAGE_SHIFT);
    REGISTER size_t         stExcess  = (size_t)(stSize & ~LW_CFG_VMM_PAGE_MASK);
             
             ULONG          ulError;
    
    if (stAlign & (stAlign - 1)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "iAlign invalidate.\r\n");
        _ErrorHandle(ERROR_VMM_ALIGN);
        return  (LW_NULL);
    }
    if (stAlign < LW_CFG_VMM_PAGE_SIZE) {
        stAlign = LW_CFG_VMM_PAGE_SIZE;
    }
    
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
    if (stAlign > LW_CFG_VMM_PAGE_SIZE) {
        pvmpageVirtual = __vmmVirtualPageAllocAlign(ulPageNum, stAlign);/*  分配连续虚拟页面            */
    } else {
        pvmpageVirtual = __vmmVirtualPageAlloc(ulPageNum);              /*  分配连续虚拟页面            */
    }
    if (pvmpageVirtual == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);
        return  (LW_NULL);
    }
    
    pvmpagep = (PLW_VMM_PAGE_PRIVATE)__KHEAP_ALLOC(sizeof(LW_VMM_PAGE_PRIVATE));
    if (pvmpagep == LW_NULL) {
        __vmmVirtualPageFree(pvmpageVirtual);
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_KERNEL_LOW_MEMORY);
        return  (LW_NULL);
    }
    
    ulError = __vmmLibPageMap(pvmpageVirtual->PAGE_ulPageAddr,
                              pvmpageVirtual->PAGE_ulPageAddr,
                              ulPageNum, 
                              LW_VMM_FLAG_FAIL);                        /*  此段内存空间访问失效        */
    if (ulError) {                                                      /*  映射错误                    */
        __KHEAP_FREE(pvmpagep);
        __vmmVirtualPageFree(pvmpageVirtual);                           /*  释放虚拟地址空间            */
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);
        return  (LW_NULL);
    }
    
    pvmpagep->PAGEP_pfuncFiller = pfuncFiller;
    pvmpagep->PAGEP_pvArg       = pvArg;
    
    pvmpagep->PAGEP_pfuncFindShare = LW_NULL;
    pvmpagep->PAGEP_pvFindArg      = LW_NULL;
    
    pvmpagep->PAGEP_iFlags = iFlags;
    
    pvmpagep->PAGEP_pvmpageVirtual = pvmpageVirtual;                    /*  建立连接关系                */
    pvmpageVirtual->PAGE_pvAreaCb  = (PVOID)pvmpagep;                   /*  记录私有数据结构            */
    
    _List_Line_Add_Ahead(&pvmpagep->PAGEP_lineManage, 
                         &_K_plineVmmVAddrSpaceHeader);                 /*  连入缺页中断查找表          */
    
    pvmpageVirtual->PAGE_ulFlags = ulFlag;                              /*  记录内存类型                */
    __areaVirtualInsertPage(pvmpageVirtual->PAGE_ulPageAddr, 
                            pvmpageVirtual);                            /*  插入逻辑空间反查表          */
    __VMM_UNLOCK();
    
    MONITOR_EVT_LONG5(MONITOR_EVENT_ID_VMM, MONITOR_EVENT_VMM_ALLOC_A,
                      pvmpageVirtual->PAGE_ulPageAddr, 
                      stSize, stAlign, iFlags, ulFlag, LW_NULL);
    
    return  ((PVOID)pvmpageVirtual->PAGE_ulPageAddr);
}
/*********************************************************************************************************
** 函数名称: API_VmmFreeArea
** 功能描述: 释放连续虚拟内存
** 输　入  : pvVirtualMem    连续虚拟地址
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_VmmFreeArea (PVOID  pvVirtualMem)
{
    REGISTER PLW_VMM_PAGE           pvmpageVirtual;
    REGISTER PLW_VMM_PAGE_PRIVATE   pvmpagep;
             addr_t                 ulAddr = (addr_t)pvVirtualMem;
    
    __VMM_LOCK();
    pvmpageVirtual = __areaVirtualSearchPage(ulAddr);
    if (pvmpageVirtual == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  无法反向查询虚拟页面控制块  */
        return;
    }
    
    pvmpagep = (PLW_VMM_PAGE_PRIVATE)pvmpageVirtual->PAGE_pvAreaCb;
    if (pvmpagep) {
        _List_Line_Del(&pvmpagep->PAGEP_lineManage, &_K_plineVmmVAddrSpaceHeader);
        pvmpageVirtual->PAGE_pvAreaCb = LW_NULL;
        __KHEAP_FREE(pvmpagep);                                         /*  释放缺页中断管理模块        */
    }
    
#if LW_CFG_CACHE_EN > 0
    __vmmPhysicalPageClearAll(pvmpageVirtual);
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
    
    __vmmLibPageMap(pvmpageVirtual->PAGE_ulPageAddr,
                    pvmpageVirtual->PAGE_ulPageAddr,
                    pvmpageVirtual->PAGE_ulCount, 
                    LW_VMM_FLAG_FAIL);                                  /*  不允许访问                  */
    
    __vmmPhysicalPageFreeAll(pvmpageVirtual);                           /*  释放物理页面                */
    
    __areaVirtualUnlinkPage(pvmpageVirtual->PAGE_ulPageAddr,
                            pvmpageVirtual);
    
    __vmmVirtualPageFree(pvmpageVirtual);                               /*  删除虚拟页面                */
    __VMM_UNLOCK();
    
    MONITOR_EVT_LONG1(MONITOR_EVENT_ID_VMM, MONITOR_EVENT_VMM_FREE,
                      pvVirtualMem, LW_NULL);
}
/*********************************************************************************************************
** 函数名称: API_VmmExpandArea
** 功能描述: 扩展连续虚拟内存
** 输　入  : pvVirtualMem    连续虚拟地址
**           stExpSize       需要扩大的大小
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_VmmExpandArea (PVOID  pvVirtualMem, size_t  stExpSize)
{
    REGISTER PLW_VMM_PAGE   pvmpageVirtual;
             addr_t         ulAddr = (addr_t)pvVirtualMem;
             ULONG          ulError;
             
    REGISTER ULONG          ulExpPageNum = (ULONG) (stExpSize >> LW_CFG_VMM_PAGE_SHIFT);
    REGISTER size_t         stExcess     = (size_t)(stExpSize & ~LW_CFG_VMM_PAGE_MASK);
    
    if (stExcess) {
        ulExpPageNum++;                                                 /*  确定分页数量                */
    }
    
    if (ulExpPageNum < 1) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    __VMM_LOCK();
    pvmpageVirtual = __areaVirtualSearchPage(ulAddr);
    if (pvmpageVirtual == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  无法反向查询虚拟页面控制块  */
        return  (ERROR_VMM_VIRTUAL_PAGE);
    }
    
    ulError = __pageExpand(pvmpageVirtual, ulExpPageNum);               /*  拓展虚拟页面                */
    if (ulError) {
        __VMM_UNLOCK();
        return  (ulError);
    }
    __VMM_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_VmmSplitArea
** 功能描述: 拆分连续虚拟内存
** 输　入  : pvVirtualMem    连续虚拟地址
**           stSize          pvVirtualMem 虚拟段需要保留的大小.
** 输　出  : 新的连续虚拟地址
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
PVOID  API_VmmSplitArea (PVOID  pvVirtualMem, size_t  stSize)
{
    REGISTER PLW_VMM_PAGE           pvmpageVirtual;
             PLW_VMM_PAGE           pvmpageVirtualSplit;
    REGISTER PLW_VMM_PAGE_PRIVATE   pvmpagep;
    REGISTER PLW_VMM_PAGE_PRIVATE   pvmpagepSplit;
    
             addr_t         ulAddr = (addr_t)pvVirtualMem;
             ULONG          ulError;
             
    REGISTER ULONG          ulPageNum = (ULONG) (stSize >> LW_CFG_VMM_PAGE_SHIFT);
    REGISTER size_t         stExcess  = (size_t)(stSize & ~LW_CFG_VMM_PAGE_MASK);
    
    
    if (stExcess) {
        ulPageNum++;                                                    /*  确定分页数量                */
    }
    
    if (ulPageNum < 1) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    pvmpagepSplit = (PLW_VMM_PAGE_PRIVATE)__KHEAP_ALLOC(sizeof(LW_VMM_PAGE_PRIVATE));
    if (pvmpagepSplit == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_LOW_MEMORY);
        return  (LW_NULL);
    }
    
    __VMM_LOCK();
    pvmpageVirtual = __areaVirtualSearchPage(ulAddr);
    if (pvmpageVirtual == LW_NULL) {
        __VMM_UNLOCK();
        __KHEAP_FREE(pvmpagepSplit);
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  无法反向查询虚拟页面控制块  */
        return  (LW_NULL);
    }
    
    pvmpagep = (PLW_VMM_PAGE_PRIVATE)pvmpageVirtual->PAGE_pvAreaCb;
    if (pvmpagep == LW_NULL) {
        __VMM_UNLOCK();
        __KHEAP_FREE(pvmpagepSplit);
        _ErrorHandle(ERROR_VMM_PAGE_INVAL);
        return  (LW_NULL);
    }
    
    ulError = __pageSplit(pvmpageVirtual, &pvmpageVirtualSplit, ulPageNum, pvmpagepSplit);
    if (ulError) {
        __VMM_UNLOCK();
        __KHEAP_FREE(pvmpagepSplit);
        return  (LW_NULL);
    }
    
    __pageTraversalLink(pvmpageVirtual, 
                        __vmmSplitAreaHook, 
                        pvmpageVirtual,
                        pvmpageVirtualSplit,
                        LW_NULL,
                        LW_NULL,
                        LW_NULL,
                        LW_NULL);                                       /*  遍历对应的物理页面          */
                        
    *pvmpagepSplit = *pvmpagep;
    pvmpagepSplit->PAGEP_pvmpageVirtual = pvmpageVirtualSplit;          /*  回指向新的虚拟地址控制块    */
    
    _List_Line_Add_Ahead(&pvmpagepSplit->PAGEP_lineManage, 
                         &_K_plineVmmVAddrSpaceHeader);                 /*  连入缺页中断查找表          */
                         
    __areaVirtualInsertPage(pvmpageVirtualSplit->PAGE_ulPageAddr, 
                            pvmpageVirtualSplit);                       /*  插入逻辑空间反查表          */
    __VMM_UNLOCK();

    return  ((PVOID)pvmpageVirtualSplit->PAGE_ulPageAddr);
}
/*********************************************************************************************************
** 函数名称: API_VmmMergeArea
** 功能描述: 合并虚拟空间
** 输　入  : pvVirtualMem1   虚拟地址区域 1
**           pvVirtualMem2   虚拟地址区域 2
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_VmmMergeArea (PVOID  pvVirtualMem1, PVOID  pvVirtualMem2)
{
    PLW_VMM_PAGE    pvmpageVirtualL;
    PLW_VMM_PAGE    pvmpageVirtualR;
    
    PLW_VMM_PAGE_PRIVATE   pvmpagep;
    
    addr_t          ulAddrL;
    addr_t          ulAddrR;
    
    if (pvVirtualMem1 < pvVirtualMem2) {
        ulAddrL = (addr_t)pvVirtualMem1;
        ulAddrR = (addr_t)pvVirtualMem2;
    
    } else {
        ulAddrL = (addr_t)pvVirtualMem2;
        ulAddrR = (addr_t)pvVirtualMem1;
    }
    
    __VMM_LOCK();
    pvmpageVirtualL = __areaVirtualSearchPage(ulAddrL);
    if (pvmpageVirtualL == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  无法反向查询虚拟页面控制块  */
        return  (ERROR_VMM_VIRTUAL_PAGE);
    }
    
    pvmpageVirtualR = __areaVirtualSearchPage(ulAddrR);
    if (pvmpageVirtualR == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  无法反向查询虚拟页面控制块  */
        return  (ERROR_VMM_VIRTUAL_PAGE);
    }
    
    if ((pvmpageVirtualL->PAGE_ulPageAddr + pvmpageVirtualL->PAGE_ulCount) != 
         pvmpageVirtualR->PAGE_ulPageAddr) {                            /*  非连续页面不能进行合并      */
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_ADDR);
        return  (ERROR_VMM_PAGE_INVAL);
    }
    
    pvmpagep = (PLW_VMM_PAGE_PRIVATE)pvmpageVirtualR->PAGE_pvAreaCb;
    if (pvmpagep == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_PAGE_INVAL);
        return  (ERROR_VMM_PAGE_INVAL);
    }
    
    __pageTraversalLink(pvmpageVirtualR, 
                        __vmmMergeAreaHook, 
                        pvmpageVirtualL,
                        pvmpageVirtualR,
                        LW_NULL,
                        LW_NULL,
                        LW_NULL,
                        LW_NULL);                                       /*  遍历对应的物理页面          */

    __areaVirtualUnlinkPage(pvmpageVirtualR->PAGE_ulPageAddr,
                            pvmpageVirtualR);
                            
    _List_Line_Del(&pvmpagep->PAGEP_lineManage, &_K_plineVmmVAddrSpaceHeader);
    __KHEAP_FREE(pvmpagep);                                             /*  释放缺页中断管理模块        */
    
    __pageMerge(pvmpageVirtualL, pvmpageVirtualR);                      /*  合并页面                    */
    __VMM_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_VmmMoveArea
** 功能描述: 将一块虚拟页面的物理内存移动到另一块虚拟内存
** 输　入  : pvVirtualTo     目标虚拟地址区域
**           pvVirtualFrom   源虚拟地址区域
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_VmmMoveArea (PVOID  pvVirtualTo, PVOID  pvVirtualFrom)
{
    PLW_VMM_PAGE    pvmpageVirtualTo;
    PLW_VMM_PAGE    pvmpageVirtualFrom;
    
    addr_t          ulAddrTo   = (addr_t)pvVirtualTo;
    addr_t          ulAddrFrom = (addr_t)pvVirtualFrom;
    
    __VMM_LOCK();
    pvmpageVirtualTo = __areaVirtualSearchPage(ulAddrTo);
    if (pvmpageVirtualTo == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  无法反向查询虚拟页面控制块  */
        return  (ERROR_VMM_VIRTUAL_PAGE);
    }
    
    pvmpageVirtualFrom = __areaVirtualSearchPage(ulAddrFrom);
    if (pvmpageVirtualFrom == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  无法反向查询虚拟页面控制块  */
        return  (ERROR_VMM_VIRTUAL_PAGE);
    }
    
    if (pvmpageVirtualTo->PAGE_ulCount < 
        pvmpageVirtualFrom->PAGE_ulCount) {                             /*  目标虚拟区间太小            */
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_LOW_PAGE);
        return  (ERROR_VMM_LOW_PAGE);
    }
    
#if LW_CFG_CACHE_EN > 0
    __vmmPhysicalPageClearAll(pvmpageVirtualFrom);
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
    
    __vmmLibPageMap(pvmpageVirtualFrom->PAGE_ulPageAddr,
                    pvmpageVirtualFrom->PAGE_ulPageAddr,
                    pvmpageVirtualFrom->PAGE_ulCount, 
                    LW_VMM_FLAG_FAIL);                                  /*  不允许访问                  */
    
    __pageTraversalLink(pvmpageVirtualFrom, 
                        __vmmMoveAreaHook, 
                        pvmpageVirtualTo,
                        pvmpageVirtualFrom,
                        LW_NULL,
                        LW_NULL,
                        LW_NULL,
                        LW_NULL);                                       /*  遍历对应的物理页面          */
    __VMM_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_VmmSetFiller
** 功能描述: 设置虚拟空间的填充函数
**           此函数仅向 loader 提供服务, 用于代码段共享.
** 输　入  : pvVirtualMem    连续虚拟地址  (必须为 vmmMallocArea 返回地址)
**           pfuncFiller     当出现缺页时, 获取物理分页后的填充函数, 一般为 NULL.
**           pvArg           填充函数参数.
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
ULONG  API_VmmSetFiller (PVOID  pvVirtualMem, FUNCPTR  pfuncFiller, PVOID  pvArg)
{
    REGISTER PLW_VMM_PAGE           pvmpageVirtual;
    REGISTER PLW_VMM_PAGE_PRIVATE   pvmpagep;
             addr_t                 ulAddr = (addr_t)pvVirtualMem;
    
    __VMM_LOCK();
    pvmpageVirtual = __areaVirtualSearchPage(ulAddr);
    if (pvmpageVirtual == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  无法反向查询虚拟页面控制块  */
        return  (ERROR_VMM_VIRTUAL_PAGE);
    }
    
    pvmpagep = (PLW_VMM_PAGE_PRIVATE)pvmpageVirtual->PAGE_pvAreaCb;
    if (!pvmpagep) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  无法反向查询虚拟页面控制块  */
        return  (ERROR_VMM_VIRTUAL_PAGE);
    }
    
    pvmpagep->PAGEP_pfuncFiller = pfuncFiller;
    pvmpagep->PAGEP_pvArg       = pvArg;
    
    __VMM_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_VmmSetFindShare
** 功能描述: 设置虚拟空间查询共享页面函数.
** 输　入  : pvVirtualMem    连续虚拟地址  (必须为 vmmMallocArea 返回地址)
**           pfuncFindShare  当出现缺页时, 如果页面属性为共享, 则查询是否有可以共享的页面
**           pvArg           函数参数.
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
ULONG  API_VmmSetFindShare (PVOID  pvVirtualMem, PVOIDFUNCPTR  pfuncFindShare, PVOID  pvArg)
{
    REGISTER PLW_VMM_PAGE           pvmpageVirtual;
    REGISTER PLW_VMM_PAGE_PRIVATE   pvmpagep;
             addr_t                 ulAddr = (addr_t)pvVirtualMem;
    
    __VMM_LOCK();
    pvmpageVirtual = __areaVirtualSearchPage(ulAddr);
    if (pvmpageVirtual == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  无法反向查询虚拟页面控制块  */
        return  (ERROR_VMM_VIRTUAL_PAGE);
    }
    
    pvmpagep = (PLW_VMM_PAGE_PRIVATE)pvmpageVirtual->PAGE_pvAreaCb;
    if (!pvmpagep) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  无法反向查询虚拟页面控制块  */
        return  (ERROR_VMM_VIRTUAL_PAGE);
    }
    
    pvmpagep->PAGEP_pfuncFindShare = pfuncFindShare;
    pvmpagep->PAGEP_pvFindArg      = pvArg;
    
    __VMM_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_VmmPCountInArea
** 功能描述: API_VmmMallocAreaEx 分配的连续虚拟内存中包含的物理页面个数 (此物理内存为缺页中断分配)
** 输　入  : pvVirtualMem    连续虚拟地址 (必须为 vmmMallocArea?? 返回地址)
**           pulPageNum      返回物理页面的个数
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_VmmPCountInArea (PVOID  pvVirtualMem, ULONG  *pulPageNum)
{
    REGISTER PLW_VMM_PAGE           pvmpageVirtual;
             addr_t                 ulAddr    = (addr_t)pvVirtualMem;
             ULONG                  ulCounter = 0ul;
             
    if (pulPageNum == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    __VMM_LOCK();
    pvmpageVirtual = __areaVirtualSearchPage(ulAddr);
    if (pvmpageVirtual == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  无法反向查询虚拟页面控制块  */
        return  (ERROR_VMM_VIRTUAL_PAGE);
    }
    
    __pageTraversalLink(pvmpageVirtual, 
                        __vmmPCountInAreaHook, 
                        (PVOID)&ulCounter,
                        LW_NULL,
                        LW_NULL,
                        LW_NULL,
                        LW_NULL,
                        LW_NULL);                                       /*  遍历对应的物理页面          */
    __VMM_UNLOCK();
    
    *pulPageNum = ulCounter;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_VmmInvalidateArea
** 功能描述: 释放 vmmMallocArea 分配出的物理内存, 仅保留虚拟内存空间. 
** 输　入  : pvVirtualMem    连续虚拟地址  (必须为 vmmMalloc?? 返回地址)
**           pvSubMem        需要 invalidate 的地址
**           stSize          需要 invalidate 的大小
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
ULONG  API_VmmInvalidateArea (PVOID  pvVirtualMem, PVOID  pvSubMem, size_t  stSize)
{
    REGISTER PLW_VMM_PAGE           pvmpageVirtual;
    REGISTER PLW_VMM_PAGE_PRIVATE   pvmpagep;
             addr_t                 ulAddr = (addr_t)pvVirtualMem;
    
    __VMM_LOCK();
    pvmpageVirtual = __areaVirtualSearchPage(ulAddr);
    if (pvmpageVirtual == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  无法反向查询虚拟页面控制块  */
        return  (ERROR_VMM_VIRTUAL_PAGE);
    }
    
    pvmpagep = (PLW_VMM_PAGE_PRIVATE)pvmpageVirtual->PAGE_pvAreaCb;
    if (!pvmpagep) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  无法反向查询虚拟页面控制块  */
        return  (ERROR_VMM_VIRTUAL_PAGE);
    }
    
    __pageTraversalLink(pvmpageVirtual, 
                        __vmmInvalidateAreaHook, 
                        (PVOID)pvmpageVirtual,
                        pvSubMem,
                        (PVOID)stSize,
                        LW_NULL,
                        LW_NULL,
                        LW_NULL);                                       /*  遍历对应的物理页面          */
    __VMM_UNLOCK();
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_VMM, MONITOR_EVENT_VMM_INVAL_A,
                      pvSubMem, stSize, LW_NULL);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_VmmPreallocArea
** 功能描述: 向 vmmMallocArea 分配出的虚拟内存空间填充物理页面
**           此函数仅向 loader 提供服务, 提高装载速度.
** 输　入  : pvVirtualMem    连续虚拟地址  (必须为 vmmMalloc?? 返回地址)
**           pvSubMem        需要填充的地址
**           stSize          需要填充的大小
**           ulFlag          填充属性
**           pfuncTempFiller 临时填充函数, 和标准的 filler 相同, 如果存在则使用临时填充函数, 如果不存在,
                             则使用 PLW_VMM_PAGE_PRIVATE 设置的填充函数.
**           pvTempArg       临时填充函数参数.
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
ULONG  API_VmmPreallocArea (PVOID       pvVirtualMem, 
                            PVOID       pvSubMem, 
                            size_t      stSize, 
                            FUNCPTR     pfuncTempFiller, 
                            PVOID       pvTempArg,
                            ULONG       ulFlag)
{
    REGISTER PLW_VMM_PAGE           pvmpageVirtual;
    REGISTER PLW_VMM_PAGE           pvmpagePhysical;
    REGISTER PLW_VMM_PAGE_PRIVATE   pvmpagep;
             ULONG                  ulZoneIndex;
    
             ULONG                  i;
             addr_t                 ulAddr = (addr_t)pvVirtualMem;
             addr_t                 ulAddrPage;
             ULONG                  ulPageNum;
             size_t                 stRealSize;
             ULONG                  ulOldFlag;
             ULONG                  ulError;
    
    __VMM_LOCK();
    pvmpageVirtual = __areaVirtualSearchPage(ulAddr);
    if (pvmpageVirtual == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  无法反向查询虚拟页面控制块  */
        return  (ERROR_VMM_VIRTUAL_PAGE);
    }
    
    if ((pvSubMem < pvVirtualMem) ||
        (((addr_t)pvSubMem + stSize) > 
        ((addr_t)pvVirtualMem + (pvmpageVirtual->PAGE_ulCount << LW_CFG_VMM_PAGE_SHIFT)))) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_ADDR);                           /*  地址超出范围                */
        return  (ERROR_VMM_VIRTUAL_ADDR);
    }
    
    pvmpagep = (PLW_VMM_PAGE_PRIVATE)pvmpageVirtual->PAGE_pvAreaCb;
    if (!pvmpagep) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  无法反向查询虚拟页面控制块  */
        return  (ERROR_VMM_VIRTUAL_PAGE);
    }
    
    ulAddrPage = (addr_t)pvSubMem & LW_CFG_VMM_PAGE_MASK;               /*  页面对齐地址                */
    stRealSize = stSize + (size_t)((addr_t)pvSubMem - ulAddrPage);
    stRealSize = ROUND_UP(stRealSize, LW_CFG_VMM_PAGE_SIZE);
    ulPageNum  = (ULONG)(stRealSize >> LW_CFG_VMM_PAGE_SHIFT);          /*  需要预分配的物理页面个数    */
    
    for (i = 0; i < ulPageNum; i++) {
        if (__vmmLibGetFlag(ulAddrPage, &ulOldFlag) == ERROR_NONE) {
            if (ulOldFlag != ulFlag) {
                __vmmLibSetFlag(ulAddrPage, 1, ulFlag);                 /*  存在物理页面仅设置flag即可  */
            }
        
        } else {
            pvmpagePhysical = __vmmPhysicalPageAlloc(1, LW_ZONE_ATTR_NONE, &ulZoneIndex);
            if (pvmpagePhysical == LW_NULL) {
                ulError = ERROR_VMM_LOW_PHYSICAL_PAGE;
                printk(KERN_CRIT "kernel no more physical page.\n");    /*  系统无法分配物理页面        */
                goto    __error_handle;
            }
            ulError = __vmmLibPageMap(pvmpagePhysical->PAGE_ulPageAddr,
                                      ulAddrPage, 1, ulFlag);           /*  这里直接映射到需要的地址    */
            if (ulError) {
                __vmmPhysicalPageFree(pvmpagePhysical);
                printk(KERN_CRIT "kernel physical page map error.\n");  /*  系统无法映射物理页面        */
                goto    __error_handle;
            }
            
            /*
             *  注意: 填充函数直接向目标地址填充数据, 所以多任务访问时由于抢占, 可能访问的数据还没有经过
             *  填充, 所以此函数应该仅作为初始化阶段的操作, 其他的分配, 使用缺页中断分配即可.
             */
            if (pfuncTempFiller) {
                pfuncTempFiller(pvTempArg, ulAddrPage, ulAddrPage, 1);
            } else {
                if (pvmpagep->PAGEP_pfuncFiller) {
                    pvmpagep->PAGEP_pfuncFiller(pvmpagep->PAGEP_pvArg, 
                                                ulAddrPage, ulAddrPage, 1);
                }
            }
            
            pvmpagePhysical->PAGE_ulMapPageAddr = ulAddrPage;
            pvmpagePhysical->PAGE_ulFlags       = ulFlag;
            
            __pageLink(pvmpageVirtual, pvmpagePhysical);                /*  建立连接关系                */
        }
        ulAddrPage += LW_CFG_VMM_PAGE_SIZE;
    }
    __VMM_UNLOCK();
    
    MONITOR_EVT_LONG3(MONITOR_EVENT_ID_VMM, MONITOR_EVENT_VMM_PREALLOC_A,
                      pvSubMem, stSize, ulFlag, LW_NULL);
    
    return  (ERROR_NONE);
    
__error_handle:
    __VMM_UNLOCK();
    
    _ErrorHandle(ulError);
    return  (ulError);
}
/*********************************************************************************************************
** 函数名称: API_VmmShareArea
** 功能描述: 将两块 area 内存内某一部分共享一块物理内存, 
**           此函数仅向 loader 提供服务, 用于代码段共享.
** 输　入  : pvVirtualMem1   连续虚拟地址  (必须为 vmmMallocArea 返回地址)
**           pvVirtualMem2   连续虚拟地址  (必须为 vmmMallocArea 返回地址)
**           stStartOft1     内存区1共享起始偏移量
**           stStartOft2     内存区2共享起始偏移量
**           stSize          共享区域长度
**           bExecEn         区域是否可执行
**           pfuncTempFiller 临时填充函数, 和标准的 filler 相同, 如果存在则使用临时填充函数, 如果不存在,
                             则使用 PLW_VMM_PAGE_PRIVATE 设置的填充函数.
**           pvTempArg       临时填充函数参数.
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
** 注  意  : 如果, pvVirtualMem2 指定的区域已经存在物理页面, 则释放对应的物理页面, 然后建立 fake 物理页面
             与 pvVirtualMem1 共享相关区域, 同时 ulStartOft1 与 ulStartOft2 必须页面对齐, ulSize 也必须页
             面对齐, 
             如果遇到 pvVirtualMem1 中有被更改的页面, pvVirtualMem2 则新开辟物理页面并填充.
                                           API 函数
*********************************************************************************************************/
LW_API 
ULONG  API_VmmShareArea (PVOID      pvVirtualMem1, 
                         PVOID      pvVirtualMem2,
                         size_t     stStartOft1, 
                         size_t     stStartOft2, 
                         size_t     stSize,
                         BOOL       bExecEn,
                         FUNCPTR    pfuncTempFiller, 
                         PVOID      pvTempArg)
{
    REGISTER PLW_VMM_PAGE           pvmpageVirtual1;
    REGISTER PLW_VMM_PAGE           pvmpageVirtual2;
    REGISTER PLW_VMM_PAGE           pvmpagePhysical1;
    REGISTER PLW_VMM_PAGE           pvmpagePhysical2;
    REGISTER PLW_VMM_PAGE_PRIVATE   pvmpagep;
             ULONG                  ulZoneIndex;
    
             ULONG                  i;
             addr_t                 ulAddr1 = (addr_t)pvVirtualMem1;
             addr_t                 ulAddr2 = (addr_t)pvVirtualMem2;
             ULONG                  ulPageNum;
             ULONG                  ulError;
             ULONG                  ulFlag;
    
    
    if (!ALIGNED(stStartOft1, LW_CFG_VMM_PAGE_SIZE) ||
        !ALIGNED(stStartOft2, LW_CFG_VMM_PAGE_SIZE) ||
        !ALIGNED(stSize,      LW_CFG_VMM_PAGE_SIZE)) {                  /*  偏移量和长度必须页面对齐    */
        _ErrorHandle(ERROR_VMM_ALIGN);
        return  (ERROR_VMM_ALIGN);
    }
    
    if (bExecEn) {
        ulFlag = LW_VMM_FLAG_EXEC | LW_VMM_FLAG_READ;
    } else {
        ulFlag = LW_VMM_FLAG_READ;
    }
    
    ulPageNum = (ULONG)(stSize >> LW_CFG_VMM_PAGE_SHIFT);
    
    __VMM_LOCK();
    pvmpageVirtual1 = __areaVirtualSearchPage(ulAddr1);
    if (pvmpageVirtual1 == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  无法反向查询虚拟页面控制块  */
        return  (ERROR_VMM_VIRTUAL_PAGE);
    }
    
    pvmpageVirtual2 = __areaVirtualSearchPage(ulAddr2);
    if (pvmpageVirtual2 == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  无法反向查询虚拟页面控制块  */
        return  (ERROR_VMM_VIRTUAL_PAGE);
    }
    
    if (!pvmpageVirtual1->PAGE_pvAreaCb) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  无法反向查询虚拟页面控制块  */
        return  (ERROR_VMM_VIRTUAL_PAGE);
    }
    
    pvmpagep = (PLW_VMM_PAGE_PRIVATE)pvmpageVirtual2->PAGE_pvAreaCb;
    if (!pvmpagep) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  无法反向查询虚拟页面控制块  */
        return  (ERROR_VMM_VIRTUAL_PAGE);
    }
    
    if ((stStartOft1 > (pvmpageVirtual1->PAGE_ulCount << LW_CFG_VMM_PAGE_SHIFT)) ||
        (stStartOft2 > (pvmpageVirtual2->PAGE_ulCount << LW_CFG_VMM_PAGE_SHIFT))) {
        __VMM_UNLOCK();
        _ErrorHandle(EINVAL);                                           /*  偏移量越界                  */
        return  (EINVAL);
    }
    
    ulAddr1 += stStartOft1;
    ulAddr2 += stStartOft2;
    for (i = 0; i < ulPageNum; i++) {
        pvmpagePhysical1 = __pageFindLink(pvmpageVirtual1, ulAddr1);    /*  获取共享目标物理页面        */
        
        if ((pvmpagePhysical1 == LW_NULL) ||                            /*  共享页面不存在              */
            ((pvmpagePhysical1->PAGE_iChange) &&
             (pvmpagep->PAGEP_iFlags & LW_VMM_PRIVATE_CHANGE))) {       /*  共享物理页面发生了改变      */
            
            pvmpagePhysical2 = __vmmPhysicalPageAlloc(1, LW_ZONE_ATTR_NONE, &ulZoneIndex);
            if (pvmpagePhysical2 == LW_NULL) {                          /*  物理页面分配                */
                ulError = ERROR_KERNEL_LOW_MEMORY;
                goto    __error_handle;
            }
            pvmpagePhysical2->PAGE_ulFlags = LW_VMM_FLAG_RDWR;          /*  读写                        */
        
        } else {
            pvmpagePhysical2 = __vmmPhysicalPageRef(pvmpagePhysical1);  /*  页面引用                    */
            if (pvmpagePhysical2 == LW_NULL) {
                ulError = ERROR_KERNEL_LOW_MEMORY;
                goto    __error_handle;
            }
            pvmpagePhysical2->PAGE_ulFlags = ulFlag;
        }
        
        ulError = __vmmLibPageMap(pvmpagePhysical2->PAGE_ulPageAddr,
                                  ulAddr2, 1, 
                                  pvmpagePhysical2->PAGE_ulFlags);      /*  这里直接映射到需要的地址    */
        if (ulError) {
            __vmmPhysicalPageFree(pvmpagePhysical2);
            printk(KERN_CRIT "kernel physical page map error.\n");      /*  系统无法映射物理页面        */
            goto    __error_handle;
        }
        
        if (!LW_VMM_PAGE_IS_FAKE(pvmpagePhysical2)) {                   /*  不是共享物理内存            */
            if (pfuncTempFiller) {
                pfuncTempFiller(pvTempArg, ulAddr2, ulAddr2, 1);        /*  使用临时填充函数            */
            } else {
                if (pvmpagep->PAGEP_pfuncFiller) {
                    pvmpagep->PAGEP_pfuncFiller(pvmpagep->PAGEP_pvArg, 
                                                ulAddr2, ulAddr2, 1);   /*  填充文件数据                */
                }
            }
            pvmpagePhysical2->PAGE_ulFlags = ulFlag;
            
            __vmmLibSetFlag(ulAddr2, 1, ulFlag);                        /*  设置最终模式                */
        }
        
        pvmpagePhysical2->PAGE_ulMapPageAddr = ulAddr2;                 /*  保存对应的映射虚拟地址      */
        
        __pageLink(pvmpageVirtual2, pvmpagePhysical2);                  /*  建立连接关系                */
        
        ulAddr1 += LW_CFG_VMM_PAGE_SIZE;
        ulAddr2 += LW_CFG_VMM_PAGE_SIZE;
    }
    __VMM_UNLOCK();
    
    MONITOR_EVT_LONG5(MONITOR_EVENT_ID_VMM, MONITOR_EVENT_VMM_SHARE_A,
                      pvVirtualMem1, pvVirtualMem2, 
                      stStartOft1, stStartOft2, 
                      stSize, LW_NULL);
    
    return  (ERROR_NONE);
    
__error_handle:
    __VMM_UNLOCK();
    
    _ErrorHandle(ulError);
    return  (ulError);
}
/*********************************************************************************************************
** 函数名称: API_VmmRemapArea
** 功能描述: 将连续虚拟内存映射到连续的物理地址上 (驱动程序 mmap 将使用此函数做映射)
** 输　入  : pvVirtualAddr   连续虚拟地址 (必须为 vmmMallocArea?? 返回地址)
**           pvPhysicalAddr  物理地址
**           stSize          映射长度
**           ulFlag          映射 flag 标志
**           pfuncFiller     缺页中断填充函数 (一般为 LW_NULL)
**           pvArg           缺页中断填充函数首参数
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_VmmRemapArea (PVOID      pvVirtualAddr, 
                         PVOID      pvPhysicalAddr, 
                         size_t     stSize, 
                         ULONG      ulFlag, 
                         FUNCPTR    pfuncFiller, 
                         PVOID      pvArg)
{
    REGISTER addr_t ulVirtualAddr  = (addr_t)pvVirtualAddr;
    REGISTER addr_t ulPhysicalAddr = (addr_t)pvPhysicalAddr;
    
    REGISTER ULONG  ulPageNum = (ULONG) (stSize >> LW_CFG_VMM_PAGE_SHIFT);
    REGISTER size_t stExcess  = (size_t)(stSize & ~LW_CFG_VMM_PAGE_MASK);
             
    REGISTER PLW_VMM_PAGE           pvmpageVirtual;
    REGISTER PLW_VMM_PAGE_PRIVATE   pvmpagep;
             
    REGISTER ULONG  ulError;
    
    if (stExcess) {
        ulPageNum++;                                                    /*  确定分页数量                */
    }

    if (ulPageNum < 1) {
        _ErrorHandle(EINVAL);
        return  (ERROR_VMM_VIRTUAL_ADDR);
    }
    
    if (!ALIGNED(pvVirtualAddr,  LW_CFG_VMM_PAGE_SIZE) ||
        !ALIGNED(pvPhysicalAddr, LW_CFG_VMM_PAGE_SIZE)) {
        _ErrorHandle(EINVAL);
        return  (ERROR_VMM_ALIGN);
    }
    
    __VMM_LOCK();
    pvmpageVirtual = __areaVirtualSearchPage(ulVirtualAddr);
    if (pvmpageVirtual == LW_NULL) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  无法反向查询虚拟页面控制块  */
        return  (ERROR_VMM_VIRTUAL_PAGE);
    }
    
    pvmpagep = (PLW_VMM_PAGE_PRIVATE)pvmpageVirtual->PAGE_pvAreaCb;
    if (!pvmpagep) {
        __VMM_UNLOCK();
        _ErrorHandle(ERROR_VMM_VIRTUAL_PAGE);                           /*  无法反向查询虚拟页面控制块  */
        return  (ERROR_VMM_VIRTUAL_PAGE);
    }
    
    pvmpagep->PAGEP_pfuncFiller = pfuncFiller;
    pvmpagep->PAGEP_pvArg       = pvArg;
                                   
    ulError = __vmmLibPageMap(ulPhysicalAddr, ulVirtualAddr, ulPageNum, ulFlag);
    if (ulError == ERROR_NONE) {
        pvmpageVirtual->PAGE_ulFlags = ulFlag;                          /*  记录内存类型                */
    }
    __VMM_UNLOCK();
    
    if (ulError == ERROR_NONE) {
        MONITOR_EVT_LONG4(MONITOR_EVENT_ID_VMM, MONITOR_EVENT_VMM_REMAP_A,
                          pvVirtualAddr, pvPhysicalAddr, 
                          stSize, ulFlag, LW_NULL);
    }
    
    return  (ulError);
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
