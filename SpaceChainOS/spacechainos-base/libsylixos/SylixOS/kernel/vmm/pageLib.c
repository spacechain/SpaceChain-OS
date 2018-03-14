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
** 文   件   名: pageLib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 12 月 09 日
**
** 描        述: 平台无关虚拟内存管理, 基础页分配与回收操作库.

** BUG
2008.12.23  修正 __pageFreeHashIndex() 返回值高于 LW_CFG_VMM_MAX_ORDER - 1 的 BUG.
2008.12.25  修正了 __pageFree() 对使用标志的赋值.
2009.06.23  加入了指定对齐关系页面开辟功能, 主要适用于一些 DMA 操作需要对内存对齐关系有要求的系统.
2009.07.03  修正了一些 GCC 警告.
2009.11.10  加入区域属性.
2009.11.13  升级获得最小连续页面的算法.
2010.03.10  修正了 page alloc 针对不满足大小的连续页面强行分配的 BUG.
            修正了 page alloc align 对满足页面条件判断的一个小 BUG.
2011.12.08  物理页面的页面控制块需要预先从 kheap 分配, 这样可以避免在 VMM_LOCK() 中等待 kheap lock.
2013.05.30  加入对新加入页面字段的初始化.
2013.06.03  加入物理页面与虚拟页面的连接关系处理.
2014.04.30  加入 split 与 expand 函数进行页面分离与扩展.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
/*********************************************************************************************************
  虚拟页面 hash 链
*********************************************************************************************************/
#define __VMM_PAGE_LINK_HASH_SIZE   32
#define __VMM_PAGE_LINK_HASH_MASK   (__VMM_PAGE_LINK_HASH_SIZE - 1)
#define __VMM_PAGE_LINK_HASH(key)   (((key) >> LW_CFG_VMM_PAGE_SHIFT) & __VMM_PAGE_LINK_HASH_MASK)
/*********************************************************************************************************
  物理页面控制块
*********************************************************************************************************/
typedef union {
    LW_LIST_MONO        PAGECB_monoFreeList;
    LW_VMM_PAGE         PAGECB_vmpage;
} LW_VMM_PAGE_CB;
static LW_LIST_MONO_HEADER  _K_pmonoPhyPageFreeList = LW_NULL;
/*********************************************************************************************************
** 函数名称: __pageCbInit
** 功能描述: 初始化页面控制块池
** 输　入  : ulPageNum     物理页面数量
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  __pageCbInit (ULONG  ulPageNum)
{
    INT                  i;
    LW_VMM_PAGE_CB      *pvmpagecb;
    
    pvmpagecb = (LW_VMM_PAGE_CB *)__KHEAP_ALLOC(sizeof(LW_VMM_PAGE_CB) * (size_t)ulPageNum);
    if (pvmpagecb == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "kernel low memory.\r\n");
        _ErrorHandle(ERROR_KERNEL_LOW_MEMORY);                          /*  缺少内核内存                */
        return  (ERROR_KERNEL_LOW_MEMORY);
    }
    
    for (i = 0; i < ulPageNum; i++) {
        _list_mono_free(&_K_pmonoPhyPageFreeList, &pvmpagecb[i].PAGECB_monoFreeList);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __pageCbAlloc
** 功能描述: 获取一个页面控制块
** 输　入  : iPageType     页面类型
** 输　出  : 页面控制块
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static PLW_VMM_PAGE  __pageCbAlloc (INT  iPageType)
{
    PLW_LIST_MONO    pmonoAlloc;
    size_t           stAllocSize;

    if (iPageType == __VMM_PAGE_TYPE_PHYSICAL) {
        if (_K_pmonoPhyPageFreeList) {
            pmonoAlloc = _list_mono_allocate(&_K_pmonoPhyPageFreeList);
        } else {
            pmonoAlloc = LW_NULL;
        }
        return  ((PLW_VMM_PAGE)pmonoAlloc);
    
    } else {
        stAllocSize  = ROUND_UP(sizeof(LW_VMM_PAGE), sizeof(LW_STACK));
        stAllocSize += (sizeof(LW_LIST_LINE_HEADER) * __VMM_PAGE_LINK_HASH_SIZE);
        return  ((PLW_VMM_PAGE)__KHEAP_ALLOC(stAllocSize));
    }
}
/*********************************************************************************************************
** 函数名称: __pageCbFree
** 功能描述: 获取一个页面控制块
** 输　入  : pvmpage       页面控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID __pageCbFree (PLW_VMM_PAGE  pvmpage)
{
    PLW_LIST_MONO    pmonoFree = (PLW_LIST_MONO)pvmpage;

    if (pvmpage->PAGE_iPageType == __VMM_PAGE_TYPE_PHYSICAL) {
        _list_mono_free(&_K_pmonoPhyPageFreeList, pmonoFree);
    
    } else {
        __KHEAP_FREE(pvmpage);
    }
}
/*********************************************************************************************************
** 函数名称: __pageInitLink
** 功能描述: 初始化物理页面 hash 链表
** 输　入  : pvmpage       页面控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __pageInitLink (PLW_VMM_PAGE  pvmpage)
{
    INT     i;
    size_t  stOffset = ROUND_UP(sizeof(LW_VMM_PAGE), sizeof(LW_STACK));
    
    pvmpage->PAGE_plinePhyLink = (LW_LIST_LINE_HEADER *)((PCHAR)pvmpage + stOffset);
    
    for (i = 0; i < __VMM_PAGE_LINK_HASH_SIZE; i++) {
        pvmpage->PAGE_plinePhyLink[i] = LW_NULL;
    }
}
/*********************************************************************************************************
** 函数名称: __pageFreeHashIndex
** 功能描述: 根据指定的页面大小选择 free area hash 表入口
** 输　入  : ulPageNum     需要获取的页面数量
** 输　出  : free area hash 表入口
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __pageFreeHashIndex (ULONG  ulPageNum)
{
    REGISTER INT    i;
    
    for (i = 0; (1ul << i) < ulPageNum; i++);                           /*  确定入口号                  */
    
    if (i >= LW_CFG_VMM_MAX_ORDER) {                                    /*  max LW_CFG_VMM_MAX_ORDER-1  */
        i =  LW_CFG_VMM_MAX_ORDER - 1;
    }
    
    return  (i);
}
/*********************************************************************************************************
** 函数名称: __pageAddToFreeHash
** 功能描述: 将一个页面描述符连接入空闲 hash 表
** 输　入  : pvmzone       指定的区域
**           pvmpage       需要链入的页面
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __pageAddToFreeHash (PLW_VMM_ZONE  pvmzone, PLW_VMM_PAGE  pvmpage)
{
    REGISTER INT              iHashIndex;
    REGISTER PLW_VMM_FREEAREA pvmfaEntry;
    
    iHashIndex = __pageFreeHashIndex(pvmpage->PAGE_ulCount);
    pvmfaEntry = &pvmzone->ZONE_vmfa[iHashIndex];                       /*  获得右分页段 hash 入口      */
    
    _List_Line_Add_Ahead(&pvmpage->PAGE_lineFreeHash,
                         &pvmfaEntry->FA_lineFreeHeader);               /*  插入链表                    */
    pvmfaEntry->FA_ulCount++;
}
/*********************************************************************************************************
** 函数名称: __pageDelFromFreeHash
** 功能描述: 将一个页面描述符从空闲 hash 表中删除.
** 输　入  : pvmzone       指定的区域
**           pvmpage       需要链入的页面
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __pageDelFromFreeHash (PLW_VMM_ZONE  pvmzone, PLW_VMM_PAGE  pvmpage)
{
    REGISTER INT              iHashIndex;
    REGISTER PLW_VMM_FREEAREA pvmfaEntry;
    
    iHashIndex = __pageFreeHashIndex(pvmpage->PAGE_ulCount);
    pvmfaEntry = &pvmzone->ZONE_vmfa[iHashIndex];                       /*  获得右分页段 hash 入口      */
    
    _List_Line_Del(&pvmpage->PAGE_lineFreeHash,
                   &pvmfaEntry->FA_lineFreeHeader);                     /*  从链表中删除                */
    pvmfaEntry->FA_ulCount--;
}
/*********************************************************************************************************
** 函数名称: __pageStructSeparate
** 功能描述: 将一块连续的页面描述块分离成两块
** 输　入  : pvmpage       主页面控制块
**           pvmpageNew    需要分离的页面控制块
**           ulPageNum     页面分离点
**           iPageType     页面类型
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __pageStructSeparate (PLW_VMM_PAGE  pvmpage, 
                                   PLW_VMM_PAGE  pvmpageNew, 
                                   ULONG         ulPageNum,
                                   INT           iPageType)
{
    PLW_LIST_LINE      plineHeader = &pvmpage->PAGE_lineManage;

    _List_Line_Add_Tail(&pvmpageNew->PAGE_lineManage, &plineHeader);    /*  加入邻居链表                */
    pvmpageNew->PAGE_ulCount      = pvmpage->PAGE_ulCount - ulPageNum;
    pvmpageNew->PAGE_ulPageAddr   = pvmpage->PAGE_ulPageAddr + (ulPageNum << LW_CFG_VMM_PAGE_SHIFT);
    pvmpageNew->PAGE_iPageType    = iPageType;
    pvmpageNew->PAGE_ulFlags      = pvmpage->PAGE_ulFlags;              /*  页面属性                    */
    pvmpageNew->PAGE_pvmzoneOwner = pvmpage->PAGE_pvmzoneOwner;         /*  记录所属区域                */
    
    pvmpage->PAGE_ulCount = ulPageNum;                                  /*  修改主页面控制块的大小      */
}
/*********************************************************************************************************
** 函数名称: __pageFree
** 功能描述: 向指定的 zone 释放页面
** 输　入  : pvmzone       指定的区域
**           pvmpage       需要归还的页面
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __pageFree (PLW_VMM_ZONE  pvmzone, PLW_VMM_PAGE  pvmpage)
{
             PLW_VMM_PAGE     pvmpageLeft;
             PLW_VMM_PAGE     pvmpageRight;
    
    REGISTER PLW_LIST_LINE    plineLeft;
    REGISTER PLW_LIST_LINE    plineRight;
    
             PLW_LIST_LINE    plineDummyHeader = LW_NULL;               /*  用于参数传递的头            */
             
             ULONG            ulFreePageNum = pvmpage->PAGE_ulCount;    /*  释放的页面数量              */
             
    
    plineLeft  = _list_line_get_prev(&pvmpage->PAGE_lineManage);        /*  左分页段                    */
    plineRight = _list_line_get_next(&pvmpage->PAGE_lineManage);        /*  右分页段                    */
    
    if (plineLeft) {
        pvmpageLeft = _LIST_ENTRY(plineLeft, LW_VMM_PAGE, 
                                  PAGE_lineManage);                     /*  左分页段地址                */
    } else {
        pvmpageLeft = LW_NULL;
    }
    
    if (plineRight) {
        pvmpageRight = _LIST_ENTRY(plineRight, LW_VMM_PAGE, 
                                   PAGE_lineManage);                    /*  右分页段地址                */
    } else {
        pvmpageRight = LW_NULL;
    }
    
    if (pvmpageLeft) {
        if (pvmpageLeft->PAGE_bUsed) {
            goto    __merge_right;                                      /*  进入右分页段聚合            */
        }
        
        __pageDelFromFreeHash(pvmzone, pvmpageLeft);                    /*  从空闲表中删除              */
        pvmpageLeft->PAGE_ulCount += pvmpage->PAGE_ulCount;             /*  合并分页段                  */
        
        _List_Line_Del(&pvmpage->PAGE_lineManage,
                       &plineDummyHeader);                              /*  从邻居链表中删除            */
        __pageCbFree(pvmpage);                                          /*  释放页面控制块内存          */

        pvmpage = pvmpageLeft;                                          /*  已经合并入左分页段          */
    }
    
__merge_right:
    if (pvmpageRight) {
        if (pvmpageRight->PAGE_bUsed) {
            goto    __right_merge_fail;                                 /*  进入右分页段聚合失败        */
        }
        
        __pageDelFromFreeHash(pvmzone, pvmpageRight);                   /*  从空闲表中删除              */
        pvmpage->PAGE_ulCount += pvmpageRight->PAGE_ulCount;
        
        _List_Line_Del(&pvmpageRight->PAGE_lineManage,
                       &plineDummyHeader);                              /*  从邻居链表中删除            */
        __pageCbFree(pvmpageRight);                                     /*  释放页面控制块内存          */
    }
    
__right_merge_fail:                                                     /*  右分页段合并错误            */
    pvmpage->PAGE_bUsed = LW_FALSE;                                     /*  没有使用                    */
    __pageAddToFreeHash(pvmzone, pvmpage);                              /*  插入空闲链表                */
    
    pvmzone->ZONE_ulFreePage += ulFreePageNum;                          /*  更新 zone 控制块            */
}
/*********************************************************************************************************
** 函数名称: __pageAllocate
** 功能描述: 从指定的 zone 中分配出连续的页面. (使用 best fit 法则) (之前需要确保 zone 内空闲页数量)
** 输　入  : pvmzone       指定的区域
**           ulPageNum     需要获取的页面数量
**           iPageType     页面类型
** 输　出  : 开辟出的页面控制块
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PLW_VMM_PAGE  __pageAllocate (PLW_VMM_ZONE  pvmzone, 
                              ULONG         ulPageNum, 
                              INT           iPageType)
{
    REGISTER INT                i;
    REGISTER INT                iHashIndex;
    REGISTER PLW_VMM_FREEAREA   pvmfaEntry;
    
             PLW_LIST_LINE      plineFree;
             PLW_VMM_PAGE       pvmpageFit = LW_NULL;
             PLW_VMM_PAGE       pvmpageNewFree;
             
             ULONG              ulTemp;
             ULONG              ulFit = ~0ul;
             
    
    iHashIndex = __pageFreeHashIndex(ulPageNum);                        /*  获得 hash 搜索起始点        */
    i = iHashIndex;
    
__check_hash:
    for (; i < LW_CFG_VMM_MAX_ORDER; i++) {                             /*  从小到大搜索                */
        pvmfaEntry = &pvmzone->ZONE_vmfa[i];                            /*  获得空闲 hash 表入口        */
        if (pvmfaEntry->FA_ulCount) {
            break;
        }
    }
    if (i >= LW_CFG_VMM_MAX_ORDER) {
        return  (LW_NULL);                                              /*  无法开辟新的连续空间        */
    }
    
    for (plineFree  = pvmfaEntry->FA_lineFreeHeader;
         plineFree != LW_NULL;
         plineFree  = _list_line_get_next(plineFree)) {                 /*  遍历链表                    */
        
        /*
         *  当 iHashIndex 区域入口表中有分页存在, 不能够保证连续分页一定大于 ulPageNum
         *  例如: iHashIndex = 5 是, 这个表项中连续分页应该在 16(2^4) 到 32(2^5)之间, 所以如果 ulPageNum
         *  为 29 的话,并不能保证所有 iHashIndex = 5 内的连续分页都大于 29! 所以这里加入判断!
         */
        if (((PLW_VMM_PAGE)plineFree)->PAGE_ulCount >= ulPageNum) {
            ulTemp = ((PLW_VMM_PAGE)plineFree)->PAGE_ulCount - ulPageNum;
                                                                        /*  找出最接近分配大小的连续分页*/
            if (ulTemp < ulFit) {
                pvmpageFit = (PLW_VMM_PAGE)plineFree;
                ulFit      = ulTemp;
            }
        }
    }
    
    if (pvmpageFit == LW_NULL) {                                        /*  没有大于 ulPageNum 连续分页 */
        i++;                                                            /*  进入更大的分页区搜索        */
        /*
         *  这里只可能发生一次, 即 i == iHashIndex 时, 才可能没有找到连续分页.
         */
        goto    __check_hash;
    }
    
    if (ulFit) {                                                        /*  需要打断                    */
        pvmpageNewFree = __pageCbAlloc(iPageType);
        if (pvmpageNewFree == LW_NULL) {
            _ErrorHandle(ERROR_KERNEL_LOW_MEMORY);                      /*  缺少内核内存                */
            return  (LW_NULL);
        }
        
        _List_Line_Del(&pvmpageFit->PAGE_lineFreeHash,
                       &pvmfaEntry->FA_lineFreeHeader);                 /*  从空闲表中删除              */
        pvmfaEntry->FA_ulCount--;
        
        pvmpageFit->PAGE_bUsed     = LW_TRUE;                           /*  正在使用的分页段            */
        pvmpageNewFree->PAGE_bUsed = LW_FALSE;                          /*  没有使用的分页段            */
        __pageStructSeparate(pvmpageFit, 
                             pvmpageNewFree, 
                             ulPageNum, 
                             iPageType);                                /*  页面分离                    */
                             
        __pageAddToFreeHash(pvmzone, pvmpageNewFree);                   /*  将剩余页面插入空闲 hash 表  */
        
    } else {                                                            /*  刚好匹配                    */
        _List_Line_Del(&pvmpageFit->PAGE_lineFreeHash,
                       &pvmfaEntry->FA_lineFreeHeader);                 /*  从空闲表中删除              */
        pvmfaEntry->FA_ulCount--;
        
        pvmpageFit->PAGE_bUsed = LW_TRUE;                               /*  正在使用的分页段            */
    }
    
    if (iPageType == __VMM_PAGE_TYPE_PHYSICAL) {
        pvmpageFit->PAGE_ulMapPageAddr = PAGE_MAP_ADDR_INV;             /*  没有映射关系                */
        pvmpageFit->PAGE_iChange       = 0;                             /*  没有变化过                  */
        pvmpageFit->PAGE_ulRef         = 1ul;                           /*  引用计数初始为 1            */
        pvmpageFit->PAGE_pvmpageReal   = LW_NULL;                       /*  真实页面                    */
    
    } else {
        pvmpageFit->PAGE_pvAreaCb = LW_NULL;
        __pageInitLink(pvmpageFit);
    }
    
    pvmzone->ZONE_ulFreePage -= ulPageNum;                              /*  更新 zone 控制块            */
    
    return  (pvmpageFit);
}
/*********************************************************************************************************
** 函数名称: __pageAllocateAlign
** 功能描述: 从指定的 zone 中分配出连续的页面, 同时满足指定的内存对齐关系. 
**           (使用 best fit 法则) (之前需要确保 zone 内空闲页数量)
** 输　入  : pvmzone       指定的区域
**           ulPageNum     需要获取的页面数量
**           stAlign       内存对齐关系 (必须是页面大小的整数倍)
**           iPageType     页面类型
** 输　出  : 开辟出的页面控制块
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PLW_VMM_PAGE  __pageAllocateAlign (PLW_VMM_ZONE  pvmzone, 
                                   ULONG         ulPageNum, 
                                   size_t        stAlign,
                                   INT           iPageType)
{
    REGISTER INT                i;
    REGISTER INT                iHashIndex;
    REGISTER PLW_VMM_FREEAREA   pvmfaEntry;
    
             PLW_LIST_LINE      plineFree;
             PLW_VMM_PAGE       pvmpageFit          = LW_NULL;
             PLW_VMM_PAGE       pvmpageNewFreeLeft  = LW_NULL;
             PLW_VMM_PAGE       pvmpageNewFreeRight = LW_NULL;
             
             addr_t             ulTemp;
             addr_t             ulFit = ~0;
             
             addr_t             ulAlignMask = (addr_t)(stAlign - 1);
             ULONG              ulLeftFreePageNum;
             ULONG              ulLeftFreePageNumFit = ~0ul;
             
    
    iHashIndex = __pageFreeHashIndex(ulPageNum);                        /*  获得 hash 搜索起始点        */
    
    for (i = iHashIndex; i < LW_CFG_VMM_MAX_ORDER; i++) {               /*  从小到大搜索                */
        pvmfaEntry = &pvmzone->ZONE_vmfa[i];                            /*  获得空闲 hash 表入口        */
        if (pvmfaEntry->FA_ulCount) {
            
            for (plineFree  = pvmfaEntry->FA_lineFreeHeader;
                 plineFree != LW_NULL;
                 plineFree  = _list_line_get_next(plineFree)) {         /*  遍历链表                    */
                 
                addr_t   ulPageAddr = ((PLW_VMM_PAGE)plineFree)->PAGE_ulPageAddr;
                
                if ((ulPageAddr & ulAlignMask) == 0) {                  /*  本身就满足对齐条件          */
                    ulLeftFreePageNum = 0;                              /*  左端不需要空余页面          */
                } else {
                    ulTemp = (ulPageAddr | ulAlignMask) + 1;
                    ulLeftFreePageNum = (ulTemp - ulPageAddr) >> LW_CFG_VMM_PAGE_SHIFT;
                }
                
                if (((PLW_VMM_PAGE)plineFree)->PAGE_ulCount >=
                    (ulPageNum + ulLeftFreePageNum)) {                  /*  此区域有足够大的空间        */
                    
                    ulTemp = ((PLW_VMM_PAGE)plineFree)->PAGE_ulCount - ulPageNum;  
                                                                        /*  找出最接近分配大小的连续分页*/
                    if (ulTemp < ulFit) {
                        pvmpageFit           = (PLW_VMM_PAGE)plineFree;
                        ulFit                = ulTemp;
                        ulLeftFreePageNumFit = ulLeftFreePageNum;
                    }
                }
            }
            if (ulFit != (~0ul)) {                                      /*  匹配成功 ?                  */
                break;
            }
        }
    }
    if (i >= LW_CFG_VMM_MAX_ORDER) {
        return  (LW_NULL);                                              /*  无法开辟新的连续空间        */
    }
    
    if (ulLeftFreePageNumFit) {                                         /*  需要左端打断                */
        pvmpageNewFreeLeft = __pageCbAlloc(iPageType);
        if (pvmpageNewFreeLeft == LW_NULL) {
            _ErrorHandle(ERROR_KERNEL_LOW_MEMORY);                      /*  缺少内核内存                */
            return  (LW_NULL);
        }
    }
    if (ulFit != ulLeftFreePageNumFit) {                                /*  右端也需要打断              */
        pvmpageNewFreeRight = __pageCbAlloc(iPageType);
        if (pvmpageNewFreeRight == LW_NULL) {
            if (pvmpageNewFreeLeft) {
                __pageCbFree(pvmpageNewFreeLeft);                       /*  释放左端控制块内存          */
            }
            _ErrorHandle(ERROR_KERNEL_LOW_MEMORY);                      /*  缺少内核内存                */
            return  (LW_NULL);
        }
    }
    
    /*
     *  将这个页面控制块从空闲 hash 表中删除.
     */
    _List_Line_Del(&pvmpageFit->PAGE_lineFreeHash,
                   &pvmfaEntry->FA_lineFreeHeader);                     /*  从空闲表中删除              */
    pvmfaEntry->FA_ulCount--;
    
    /*
     *  开始拆分操作, 先左端(低地址)后右端(高地址).
     */
    if (ulLeftFreePageNumFit) {                                         /*  需要左端打断                */
        
        pvmpageFit->PAGE_bUsed         = LW_FALSE;                      /*  左端保留分段                */
        pvmpageNewFreeLeft->PAGE_bUsed = LW_TRUE;                       /*  新的分段将被使用            */
        __pageStructSeparate(pvmpageFit, 
                             pvmpageNewFreeLeft, 
                             ulLeftFreePageNumFit,                      /*  左边忽略的页面              */
                             pvmpageFit->PAGE_iPageType);               /*  页面分离                    */
                             
        __pageAddToFreeHash(pvmzone, pvmpageFit);                       /*  将左端未使用区域加入空闲表  */
        
        pvmpageFit = pvmpageNewFreeLeft;                                /*  去掉左端的分页控制块        */
    }
    
    if (ulFit != ulLeftFreePageNumFit) {                                /*  需要右端打断                */
        
        pvmpageFit->PAGE_bUsed          = LW_TRUE;                      /*  正在使用的分页段            */
        pvmpageNewFreeRight->PAGE_bUsed = LW_FALSE;                     /*  没有使用的分页段            */
        __pageStructSeparate(pvmpageFit, 
                             pvmpageNewFreeRight, 
                             ulPageNum,                                 /*  需要开辟的页面数量          */
                             iPageType);                                /*  页面分离                    */
        
        __pageAddToFreeHash(pvmzone, pvmpageNewFreeRight);              /*  将剩余页面插入空闲 hash 表  */
    
    } else {
        pvmpageFit->PAGE_bUsed = LW_TRUE;                               /*  正在使用的分页段            */
    }
    
    if (iPageType == __VMM_PAGE_TYPE_PHYSICAL) {
        pvmpageFit->PAGE_ulMapPageAddr = PAGE_MAP_ADDR_INV;             /*  没有映射关系                */
        pvmpageFit->PAGE_iChange       = 0;                             /*  没有变化过                  */
        pvmpageFit->PAGE_ulRef         = 1ul;                           /*  引用计数初始为 1            */
        pvmpageFit->PAGE_pvmpageReal   = LW_NULL;                       /*  真实页面                    */
    
    } else {
        pvmpageFit->PAGE_pvAreaCb = LW_NULL;
        __pageInitLink(pvmpageFit);
    }
    
    pvmzone->ZONE_ulFreePage -= ulPageNum;                              /*  更新 zone 控制块            */
    
    return  (pvmpageFit);
}
/*********************************************************************************************************
** 函数名称: __pageExpand
** 功能描述: 将一个虚拟页面控制块扩大
** 输　入  : pvmpage              主页面控制块
**           ulExpPageNum         需要扩大的页面个数
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  __pageExpand (PLW_VMM_PAGE   pvmpage, 
                     ULONG          ulExpPageNum)
{
    PLW_LIST_LINE       plineRight;
    PLW_LIST_LINE       plineDummyHeader = LW_NULL;                     /*  用于参数传递的头            */
    
    PLW_VMM_PAGE        pvmpageRight;
    PLW_VMM_PAGE        pvmpageNewFree;
    
    INT                 iHashIndex;
    PLW_VMM_FREEAREA    pvmfaEntry;
    PLW_VMM_ZONE        pvmzone;
    
    if (pvmpage->PAGE_iPageType != __VMM_PAGE_TYPE_VIRTUAL) {           /*  只能拓展虚拟页面            */
        _ErrorHandle(ERROR_VMM_PAGE_INVAL);                             /*  缺少内核内存                */
        return  (ERROR_VMM_PAGE_INVAL);
    }
    
    plineRight = _list_line_get_next(&pvmpage->PAGE_lineManage);        /*  右分页段                    */
    if (plineRight) {
        pvmpageRight = _LIST_ENTRY(plineRight, LW_VMM_PAGE, 
                                   PAGE_lineManage);                    /*  右分页段地址                */
    } else {
        _ErrorHandle(ERROR_VMM_LOW_PAGE);                               /*  缺少可供扩展的页面          */
        return  (ERROR_VMM_LOW_PAGE);
    }
    
    if ((pvmpageRight->PAGE_bUsed) ||
        (pvmpageRight->PAGE_ulCount < ulExpPageNum)) {
        _ErrorHandle(ERROR_VMM_LOW_PAGE);                               /*  缺少可供扩展的页面          */
        return  (ERROR_VMM_LOW_PAGE);
    }
    
    pvmzone    = pvmpage->PAGE_pvmzoneOwner;
    iHashIndex = __pageFreeHashIndex(pvmpageRight->PAGE_ulCount);
    pvmfaEntry = &pvmzone->ZONE_vmfa[iHashIndex];                       /*  获得右分页段 hash 入口      */
    
    if (pvmpageRight->PAGE_ulCount > ulExpPageNum) {                    /*  右侧分段还有剩余            */
        
        pvmpageNewFree = __pageCbAlloc(pvmpageRight->PAGE_iPageType);
        if (pvmpageNewFree == LW_NULL) {
            _ErrorHandle(ERROR_KERNEL_LOW_MEMORY);                      /*  缺少内核内存                */
            return  (ERROR_KERNEL_LOW_MEMORY);
        }
        
        _List_Line_Del(&pvmpageRight->PAGE_lineFreeHash,
                       &pvmfaEntry->FA_lineFreeHeader);                 /*  从空闲表中删除              */
        pvmfaEntry->FA_ulCount--;
        
        pvmpageRight->PAGE_bUsed   = LW_TRUE;                           /*  正在使用的分页段            */
        pvmpageNewFree->PAGE_bUsed = LW_FALSE;                          /*  没有使用的分页段            */
        __pageStructSeparate(pvmpageRight, 
                             pvmpageNewFree, 
                             ulExpPageNum, 
                             pvmpageRight->PAGE_iPageType);             /*  页面分离                    */
                             
        __pageAddToFreeHash(pvmzone, pvmpageNewFree);                   /*  将剩余页面插入空闲 hash 表  */
    
    } else {
        _List_Line_Del(&pvmpageRight->PAGE_lineFreeHash,
                       &pvmfaEntry->FA_lineFreeHeader);                 /*  从空闲表中删除              */
        pvmfaEntry->FA_ulCount--;
        
        pvmpageRight->PAGE_bUsed = LW_TRUE;                             /*  正在使用的分页段            */
    }
    
    pvmzone->ZONE_ulFreePage -= ulExpPageNum;                           /*  更新 zone 控制块            */
    
    pvmpage->PAGE_ulCount += ulExpPageNum;                              /*  合并到 pvmpage 中           */
    
    _List_Line_Del(&pvmpageRight->PAGE_lineManage,
                   &plineDummyHeader);                                  /*  从邻居链表中删除            */
    __pageCbFree(pvmpageRight);                                         /*  释放页面控制块内存          */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __pageSplit
** 功能描述: 将一个虚拟页面控制块分离成两个
** 输　入  : pvmpage              主页面控制块
**           ppvmpageSplit        被分离出的页面控制块
**           ulPageNum            pvmpage 需要保留的页面个数, 剩余部分将分配给 pvmpageSplit.
**           pvAreaCb             新的 AreaCb.
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  __pageSplit (PLW_VMM_PAGE   pvmpage, 
                    PLW_VMM_PAGE  *ppvmpageSplit, 
                    ULONG          ulPageNum,
                    PVOID          pvAreaCb)
{
    PLW_VMM_PAGE    pvmpageSplit;

    if (pvmpage->PAGE_iPageType != __VMM_PAGE_TYPE_VIRTUAL) {           /*  只能拆分虚拟页面            */
        _ErrorHandle(ERROR_VMM_PAGE_INVAL);
        return  (ERROR_VMM_PAGE_INVAL);
    }

    if (pvmpage->PAGE_ulCount <= ulPageNum) {
        _ErrorHandle(ERROR_VMM_LOW_PAGE);                               /*  缺少内核内存                */
        return  (ERROR_VMM_LOW_PAGE);
    }
    
    pvmpageSplit = __pageCbAlloc(pvmpage->PAGE_iPageType);
    if (pvmpageSplit == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_LOW_MEMORY);                          /*  缺少内核内存                */
        return  (ERROR_KERNEL_LOW_MEMORY);
    }
    
    pvmpageSplit->PAGE_bUsed = LW_TRUE;                                 /*  正在使用的分页段            */
    __pageStructSeparate(pvmpage, 
                         pvmpageSplit, 
                         ulPageNum, 
                         pvmpage->PAGE_iPageType);                      /*  页面分离                    */
                         
    pvmpageSplit->PAGE_pvAreaCb = pvAreaCb;
    __pageInitLink(pvmpageSplit);
    
    *ppvmpageSplit = pvmpageSplit;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __pageMerge
** 功能描述: 合并两个连续的虚拟页面 (调用者需保证页面连续性)
** 输　入  : pvmpageL         低地址页面
**           pvmpageR         高地址页面
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  __pageMerge (PLW_VMM_PAGE   pvmpageL, 
                    PLW_VMM_PAGE   pvmpageR)
{
    PLW_LIST_LINE   plineDummyHeader = LW_NULL;                         /*  用于参数传递的头            */
    
    if ((pvmpageL->PAGE_iPageType != __VMM_PAGE_TYPE_VIRTUAL) ||
        (pvmpageR->PAGE_iPageType != __VMM_PAGE_TYPE_VIRTUAL)) {        /*  只能合并虚拟页面            */
        _ErrorHandle(ERROR_VMM_PAGE_INVAL);
        return  (ERROR_VMM_PAGE_INVAL);
    }
    
    pvmpageL->PAGE_ulCount += pvmpageR->PAGE_ulCount;                   /*  合并分页段                  */
    
    _List_Line_Del(&pvmpageR->PAGE_lineManage,
                   &plineDummyHeader);                                  /*  从邻居链表中删除            */
    
    __pageCbFree(pvmpageR);                                             /*  释放页面控制块内存          */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __pageLink
** 功能描述: 在虚拟空间中加入一个物理页面管理块
** 输　入  : pvmpageVirtual       虚拟地址分页
**           pvmpagePhysical      物理地址分页
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
** 注  意  : 物理页面 PAGE_ulMapPageAddr 字段必须有效.
*********************************************************************************************************/
VOID  __pageLink (PLW_VMM_PAGE  pvmpageVirtual, PLW_VMM_PAGE  pvmpagePhysical)
{
    ULONG   ulKey = __VMM_PAGE_LINK_HASH(pvmpagePhysical->PAGE_ulMapPageAddr);
    
    _List_Line_Add_Ahead(&pvmpagePhysical->PAGE_lineFreeHash,
                         &pvmpageVirtual->PAGE_plinePhyLink[ulKey]);
}
/*********************************************************************************************************
** 函数名称: __pageUnlink
** 功能描述: 取消虚拟页面和物理页面的连接关系
** 输　入  : pvmpageVirtual       虚拟地址分页
**           pvmpagePhysical      物理地址分页
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
** 注  意  : 物理页面 PAGE_ulMapPageAddr 字段必须有效.
*********************************************************************************************************/
VOID  __pageUnlink (PLW_VMM_PAGE  pvmpageVirtual, PLW_VMM_PAGE  pvmpagePhysical)
{
    ULONG   ulKey = __VMM_PAGE_LINK_HASH(pvmpagePhysical->PAGE_ulMapPageAddr);
    
    _List_Line_Del(&pvmpagePhysical->PAGE_lineFreeHash,
                   &pvmpageVirtual->PAGE_plinePhyLink[ulKey]);
}
/*********************************************************************************************************
** 函数名称: __pageFindLink
** 功能描述: 查询虚拟页面指定的物理页面
** 输　入  : pvmpageVirtual       虚拟地址分页
**           ulVirAddr            虚拟地址(页面对齐)
** 输　出  : 物理页面
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PLW_VMM_PAGE  __pageFindLink (PLW_VMM_PAGE  pvmpageVirtual, addr_t  ulVirAddr)
{
    ULONG           ulKey = __VMM_PAGE_LINK_HASH(ulVirAddr);
    PLW_LIST_LINE   plineTemp;
    PLW_VMM_PAGE    pvmpagePhysical;
    
    for (plineTemp  = pvmpageVirtual->PAGE_plinePhyLink[ulKey];
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
    
        pvmpagePhysical = _LIST_ENTRY(plineTemp, LW_VMM_PAGE, PAGE_lineFreeHash);
        if (pvmpagePhysical->PAGE_ulMapPageAddr == ulVirAddr) {
            break;
        }
    }
    
    if (plineTemp) {
        return  (pvmpagePhysical);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** 函数名称: __pageTraversalLink
** 功能描述: 遍历虚拟空间所有物理页面
** 输　入  : pvmpageVirtual       虚拟地址分页
**           pfunc                遍历函数
**           pvArg[0 ~ 5]         遍历函数参数
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
** 注  意  : 这里使用安全的遍历方式.
*********************************************************************************************************/
VOID  __pageTraversalLink (PLW_VMM_PAGE   pvmpageVirtual,
                           VOIDFUNCPTR    pfunc, 
                           PVOID          pvArg0,
                           PVOID          pvArg1,
                           PVOID          pvArg2,
                           PVOID          pvArg3,
                           PVOID          pvArg4,
                           PVOID          pvArg5)
{
    INT     i;
    
    for (i = 0; i < __VMM_PAGE_LINK_HASH_SIZE; i++) {
        PLW_LIST_LINE   plineTemp;
        PLW_VMM_PAGE    pvmpagePhysical;
        
        plineTemp = pvmpageVirtual->PAGE_plinePhyLink[i];
        while (plineTemp) {
            pvmpagePhysical = _LIST_ENTRY(plineTemp, LW_VMM_PAGE, PAGE_lineFreeHash);
            plineTemp       = _list_line_get_next(plineTemp);
        
            pfunc(pvmpagePhysical, pvArg0, pvArg1, pvArg2, pvArg3, pvArg4, pvArg5);
        }
    }
}
/*********************************************************************************************************
** 函数名称: __pageGetMinContinue
** 功能描述: 从指定的 zone 中获得最小的连续分页个数 (避免 hash 表内循环, 粗略估计)
** 输　入  : pvmzone       指定的区域
** 输　出  : 最小连续分页个数
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  __pageGetMinContinue (PLW_VMM_ZONE  pvmzone)
{
    REGISTER INT                i;
             BOOL               bOk = LW_FALSE;
    REGISTER PLW_VMM_FREEAREA   pvmfaEntry;
             PLW_VMM_PAGE       pvmpage;
    
    for (i = 0; i < LW_CFG_VMM_MAX_ORDER; i++) {
        pvmfaEntry = &pvmzone->ZONE_vmfa[i];                            /*  获得空闲 hash 表入口        */
        if (pvmfaEntry->FA_ulCount) {
            bOk = LW_TRUE;
            break;
        }
    }
    
    if (bOk) {
        /*
         *  获得此 hash 表入口的第一个页面控制块. 预估最小连续页面的个数.
         */
        pvmpage = (PLW_VMM_PAGE)pvmfaEntry->FA_lineFreeHeader;          /*  pvmpage 第一个元素是表指针  */
        return  (pvmpage->PAGE_ulCount);
    } else {
        return  (0);
    }
}
/*********************************************************************************************************
** 函数名称: __pageZoneCreate
** 功能描述: 创建一个 page zone
** 输　入  : pvmzone       需要填充的结构体
**           ulAddr        页面起始地址
**           stSize        页面大小
**           uiAttr        区域属性
**           iPageType     页面类型
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  __pageZoneCreate (PLW_VMM_ZONE   pvmzone,
                         addr_t         ulAddr, 
                         size_t         stSize, 
                         UINT           uiAttr,
                         INT            iPageType)
{
    PLW_VMM_PAGE       pvmpage;
    
    pvmpage = __pageCbAlloc(iPageType);
    if (pvmpage == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_LOW_MEMORY);                          /*  缺少内核内存                */
        return  (ERROR_KERNEL_LOW_MEMORY);
    }
    
    pvmzone->ZONE_ulFreePage = (ULONG)(stSize >> LW_CFG_VMM_PAGE_SHIFT);
    pvmzone->ZONE_ulAddr     = ulAddr;
    pvmzone->ZONE_stSize     = stSize;
    pvmzone->ZONE_uiAttr     = uiAttr;
    
    lib_bzero(pvmzone->ZONE_vmfa, sizeof(LW_VMM_FREEAREA) * LW_CFG_VMM_MAX_ORDER);
    
    _LIST_LINE_INIT_IN_CODE(pvmpage->PAGE_lineManage);                  /*  没有邻居                    */
    pvmpage->PAGE_ulCount       = pvmzone->ZONE_ulFreePage;
    pvmpage->PAGE_ulPageAddr    = ulAddr;
    pvmpage->PAGE_iPageType     = iPageType;
    pvmpage->PAGE_ulFlags       = 0;
    pvmpage->PAGE_bUsed         = LW_FALSE;
    pvmpage->PAGE_pvmzoneOwner  = pvmzone;                              /*  记录所属 zone               */
    
    if (iPageType == __VMM_PAGE_TYPE_PHYSICAL) {
        pvmpage->PAGE_ulMapPageAddr = PAGE_MAP_ADDR_INV;                /*  没有映射关系                */
        pvmpage->PAGE_iChange       = 0;                                /*  没有变化过                  */
        pvmpage->PAGE_ulRef         = 0ul;                              /*  引用计数初始为 0            */
        pvmpage->PAGE_pvmpageReal   = LW_NULL;                          /*  真实页面                    */
    
    } else {
        pvmpage->PAGE_pvAreaCb = LW_NULL;
        __pageInitLink(pvmpage);
    }
    
    __pageAddToFreeHash(pvmzone, pvmpage);                              /*  插入空闲表                  */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
