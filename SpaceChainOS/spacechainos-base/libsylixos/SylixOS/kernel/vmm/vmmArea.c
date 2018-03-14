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
** 文   件   名: vmmArea.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 12 月 09 日
**
** 描        述: 平台无关虚拟内存管理, 地址空间管理. 提供地址反查功能.
                 地址反查使用哈希红黑树.
** BUG
2008.12.23  加入虚拟空间遍历的功能.
2009.03.03  红黑树相关内容完全使用库函数.
2016.08.19  __areaPhysicalSpaceInit() 支持多次调用添加物理内存.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
/*********************************************************************************************************
  全局地址空间 hash 大小设定
*********************************************************************************************************/
static const ULONG  _G_ulVmmAreaHashSizeTbl[][2] = {
/*********************************************************************************************************
        AREA SIZE      HASH SIZE
        (address)       (entry)
*********************************************************************************************************/
    {   (   8 * LW_CFG_MB_SIZE),       5       },
    {   (  16 * LW_CFG_MB_SIZE),       7       },
    {   (  32 * LW_CFG_MB_SIZE),      13       },
    {   (  64 * LW_CFG_MB_SIZE),      29       },
    {   ( 128 * LW_CFG_MB_SIZE),     157       },
    {   ( 256 * LW_CFG_MB_SIZE),     269       },
    {   ( 512 * LW_CFG_MB_SIZE),     557       },
    {   (1024 * LW_CFG_MB_SIZE),    1039       },
    {                         0,    1999       },
};
/*********************************************************************************************************
  全局地址空间
*********************************************************************************************************/
static LW_VMM_AREA  _G_vmareaZoneSpace[LW_CFG_VMM_ZONE_NUM];            /*  物理页面区域                */
/*********************************************************************************************************
  hash 操作
*********************************************************************************************************/
#define __VMM_AREA_HASH_INDEX(pvmarea, ulAddr)      \
        ((ulAddr >> LW_CFG_VMM_PAGE_SHIFT) % (pvmarea)->AREA_ulHashSize)
/*********************************************************************************************************
** 函数名称: __areaSpaceRbTreeTraversal
** 功能描述: 遍历一颗红黑数
** 输　入  : ptrbn          红黑数根节点
**           pfuncCallback  回调函数
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __areaSpaceRbTreeTraversal (PLW_TREE_RB_NODE   ptrbn, VOIDFUNCPTR  pfuncCallback)
{
    PLW_VMM_PAGE       pvmpage;
    
    if (ptrbn) {
        __areaSpaceRbTreeTraversal(ptrbn->TRBN_ptrbnLeft, pfuncCallback);
        pvmpage = _TREE_ENTRY(ptrbn, LW_VMM_PAGE, PAGE_trbnNode);
        pfuncCallback(pvmpage);
        __areaSpaceRbTreeTraversal(ptrbn->TRBN_ptrbnRight, pfuncCallback);
    }
}
/*********************************************************************************************************
** 函数名称: __areaSpaceTraversal
** 功能描述: 空间遍历
** 输　入  : pvmarea        地址空间管理块
**           pfuncCallback  回调函数
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __areaSpaceTraversal (PLW_VMM_AREA  pvmarea, VOIDFUNCPTR  pfuncCallback)
{
    REGISTER INT                i;
    REGISTER PLW_TREE_RB_ROOT   ptrbr;
    REGISTER PLW_TREE_RB_NODE   ptrbn;
    
    for (i = 0; i < pvmarea->AREA_ulHashSize; i++) {
        ptrbr = &pvmarea->AREA_ptrbrHash[i];
        ptrbn = ptrbr->TRBR_ptrbnNode;
        
        if (ptrbn) {
            __areaSpaceRbTreeTraversal(ptrbn, pfuncCallback);
        }
    }
}
/*********************************************************************************************************
** 函数名称: __areaVirtualSpaceInit
** 功能描述: 遍历虚拟地址空间管理块
** 输　入  : pfuncCallback  回调函数
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __areaVirtualSpaceTraversal (VOIDFUNCPTR  pfuncCallback)
{
    REGISTER PLW_MMU_CONTEXT  pmmuctx = __vmmGetCurCtx();
    
    __areaSpaceTraversal(&pmmuctx->MMUCTX_vmareaVirSpace, pfuncCallback);
}
/*********************************************************************************************************
** 函数名称: __areaPhysicalSpaceTraversal
** 功能描述: 物理空间遍历
** 输　入  : ulZoneIndex    物理地址 zone 下标
**           pfuncCallback  回调函数
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __areaPhysicalSpaceTraversal (ULONG  ulZoneIndex, VOIDFUNCPTR  pfuncCallback)
{
    __areaSpaceTraversal(&_G_vmareaZoneSpace[ulZoneIndex], pfuncCallback);
}
/*********************************************************************************************************
** 函数名称: __areaSpaceInit
** 功能描述: 初始化地址空间管理块
** 输　入  : pvmarea       地址空间管理块
**           ulAddr        起始地址
**           stSize        大小
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ULONG  __areaSpaceInit (PLW_VMM_AREA  pvmarea, addr_t  ulAddr, size_t  stSize)
{
             INT    i;
    REGISTER ULONG  ulHashSize;
    
    for (i = 0; ; i++) {
        if (_G_ulVmmAreaHashSizeTbl[i][0] == 0) {
            ulHashSize = _G_ulVmmAreaHashSizeTbl[i][1];                 /*  最大表大小                  */
            break;
        
        } else {
            if (stSize >= _G_ulVmmAreaHashSizeTbl[i][0]) {
                continue;
            
            } else {
                ulHashSize = _G_ulVmmAreaHashSizeTbl[i][1];             /*  确定                        */
                break;
            }
        } 
    }
    
    pvmarea->AREA_ulAreaAddr = ulAddr;
    pvmarea->AREA_stAreaSize = stSize;
    pvmarea->AREA_ulHashSize = ulHashSize;
    
    pvmarea->AREA_ptrbrHash = 
        (PLW_TREE_RB_ROOT)__KHEAP_ALLOC(sizeof(PLW_TREE_RB_ROOT) * (size_t)ulHashSize);
    if (pvmarea->AREA_ptrbrHash == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "kernel low memory.\r\n");
        _ErrorHandle(ERROR_KERNEL_LOW_MEMORY);                          /*  缺少内核内存                */
        return  (ERROR_KERNEL_LOW_MEMORY);
    }
    lib_bzero(pvmarea->AREA_ptrbrHash,
              (size_t)(sizeof(PLW_TREE_RB_ROOT) * ulHashSize));         /*  清空 hash 表                */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __areaVirtualSpaceInit
** 功能描述: 初始化虚拟地址空间管理块
** 输　入  : pvirdes       虚拟空间描述
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  __areaVirtualSpaceInit (LW_MMU_VIRTUAL_DESC   pvirdes[])
{
    REGISTER PLW_MMU_CONTEXT  pmmuctx = __vmmGetCurCtx();
             INT              i;
             addr_t           ulAddr  = __ARCH_ULONG_MAX;
             addr_t           ulEnd   = 0;
    
    for (i = 0; i < LW_CFG_VMM_VIR_NUM; i++) {
        if (pvirdes[i].VIRD_stSize == 0) {
            break;
        }
        if (ulAddr > pvirdes[i].VIRD_ulVirAddr) {
            ulAddr = pvirdes[i].VIRD_ulVirAddr;
        }
        if (ulEnd < (pvirdes[i].VIRD_ulVirAddr + pvirdes[i].VIRD_stSize)) {
            ulEnd = (pvirdes[i].VIRD_ulVirAddr + pvirdes[i].VIRD_stSize);
        }
    }
    
    if (ulAddr == __ARCH_ULONG_MAX) {
        _ErrorHandle(ENOSPC);
        return  (ENOSPC);
    }
    
    return  (__areaSpaceInit(&pmmuctx->MMUCTX_vmareaVirSpace, ulAddr, (size_t)(ulEnd - ulAddr)));
}
/*********************************************************************************************************
** 函数名称: __areaPhysicalSpaceInit
** 功能描述: 初始化物理地址空间管理块
** 输　入  : pphydesc      物理空间描述
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  __areaPhysicalSpaceInit (LW_MMU_PHYSICAL_DESC  pphydesc[])
{
    REGISTER ULONG  ulError = ERROR_NONE;
    static   ULONG  ulZone  = 0;                                        /*  可多次追尾添加内存          */
             INT    i;
             
    for (i = 0; ; i++) {
        if (pphydesc[i].PHYD_stSize == 0) {
            break;
        }
        
        switch (pphydesc[i].PHYD_uiType) {
        
        case LW_PHYSICAL_MEM_DMA:
        case LW_PHYSICAL_MEM_APP:
            if (ulZone < LW_CFG_VMM_ZONE_NUM) {
                ulError = __areaSpaceInit(&_G_vmareaZoneSpace[ulZone], 
                                          pphydesc[i].PHYD_ulPhyAddr, 
                                          pphydesc[i].PHYD_stSize);
                if (ulError) {
                    _ErrorHandle(ulError);
                    return  (ulError);
                }
                ulZone++;
            }
            break;
            
        default:
            break;
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __areaSearchPage
** 功能描述: 通过地址反向查询页面控制块
** 输　入  : pvmarea       地址空间管理块
**           ulAddr        起始地址
** 输　出  : 页面控制块
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static PLW_VMM_PAGE  __areaSearchPage (PLW_VMM_AREA  pvmarea, addr_t  ulAddr)
{
             ULONG              ulHashIndex;
    REGISTER PLW_TREE_RB_ROOT   ptrbr;
    REGISTER PLW_TREE_RB_NODE   ptrbn;
             PLW_VMM_PAGE       pvmpage;
             
    ulHashIndex = __VMM_AREA_HASH_INDEX(pvmarea, ulAddr);
    ptrbr       = &pvmarea->AREA_ptrbrHash[ulHashIndex];                /*  确定 hash 表位置            */
    ptrbn       = ptrbr->TRBR_ptrbnNode;
    
    while (ptrbn) {                                                     /*  开始从树根搜索              */
        pvmpage = _TREE_ENTRY(ptrbn, LW_VMM_PAGE, PAGE_trbnNode);
        
        if (ulAddr < pvmpage->PAGE_ulPageAddr) {
            ptrbn = _tree_rb_get_left(ptrbn);                           /*  查找左子树                  */
        } else if (ulAddr > pvmpage->PAGE_ulPageAddr) {
            ptrbn = _tree_rb_get_right(ptrbn);                          /*  查找右子树                  */
        } else {
            return  (pvmpage);
        }
    }
    
    return  (LW_NULL);                                                  /*  没有找到                    */
}
/*********************************************************************************************************
** 函数名称: __areaVirtualSearchPage
** 功能描述: 在虚拟地址空间中, 通过地址反向查询页面控制块
** 输　入  : ulAddr        起始地址
** 输　出  : 页面控制块
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PLW_VMM_PAGE  __areaVirtualSearchPage (addr_t  ulAddr)
{
    REGISTER PLW_MMU_CONTEXT  pmmuctx = __vmmGetCurCtx();

    return  (__areaSearchPage(&pmmuctx->MMUCTX_vmareaVirSpace, ulAddr));
}
/*********************************************************************************************************
** 函数名称: __areaPhysicalSearchPage
** 功能描述: 在物理地址空间中, 通过地址反向查询页面控制块
** 输　入  : ulZoneIndex   物理地址 zone 下标
**           ulAddr        起始地址
** 输　出  : 页面控制块
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PLW_VMM_PAGE  __areaPhysicalSearchPage (ULONG  ulZoneIndex, addr_t  ulAddr)
{
    return  (__areaSearchPage(&_G_vmareaZoneSpace[ulZoneIndex], ulAddr));
}
/*********************************************************************************************************
** 函数名称: __areaInsertPage
** 功能描述: 向地址空间中注册一个页面控制块
** 输　入  : pvmarea       地址空间管理块
**           ulAddr        起始地址
**           pvmpage       页面控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __areaInsertPage (PLW_VMM_AREA  pvmarea, addr_t  ulAddr, PLW_VMM_PAGE  pvmpage)
{
             ULONG              ulHashIndex;
    REGISTER PLW_TREE_RB_ROOT   ptrbr;
    REGISTER PLW_TREE_RB_NODE  *pptrbnRoot;
    REGISTER PLW_TREE_RB_NODE   ptrbnParent = LW_NULL;
             PLW_VMM_PAGE       pvmpageTemp;
    
    ulHashIndex = __VMM_AREA_HASH_INDEX(pvmarea, ulAddr);
    ptrbr       = &pvmarea->AREA_ptrbrHash[ulHashIndex];                /*  确定 hash 表位置            */
    
    pptrbnRoot = &ptrbr->TRBR_ptrbnNode;
    
    while (*pptrbnRoot) {
        ptrbnParent = *pptrbnRoot;
        
        pvmpageTemp = _TREE_ENTRY(ptrbnParent, LW_VMM_PAGE, PAGE_trbnNode);
        
        if (ulAddr < pvmpageTemp->PAGE_ulPageAddr) {
            pptrbnRoot = _tree_rb_get_left_addr(*pptrbnRoot);
        } else if (ulAddr > pvmpageTemp->PAGE_ulPageAddr) {
            pptrbnRoot = _tree_rb_get_right_addr(*pptrbnRoot);
        } else {
            return;                                                     /*  不可能运行到这里            */
        }
    }
    
    _tree_rb_link_node(&pvmpage->PAGE_trbnNode, 
                       ptrbnParent, 
                       pptrbnRoot);                                     /*  插入到树中                  */
                       
    _Tree_Rb_Insert_Color(&pvmpage->PAGE_trbnNode, 
                          &pvmarea->AREA_ptrbrHash[ulHashIndex]);       /*  维持平衡                    */
}
/*********************************************************************************************************
** 函数名称: __areaVirtualInsertPage
** 功能描述: 向虚拟地址空间中注册一个页面控制块
** 输　入  : ulAddr        起始地址
**           pvmpage       页面控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __areaVirtualInsertPage (addr_t  ulAddr, PLW_VMM_PAGE  pvmpage)
{
    REGISTER PLW_MMU_CONTEXT  pmmuctx = __vmmGetCurCtx();

    __areaInsertPage(&pmmuctx->MMUCTX_vmareaVirSpace, ulAddr, pvmpage);
}
/*********************************************************************************************************
** 函数名称: __areaPhysicalInsertPage
** 功能描述: 向物理地址空间中注册一个页面控制块
** 输　入  : ulZoneIndex   物理地址 zone 下标
**           ulAddr        起始地址
**           pvmpage       页面控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __areaPhysicalInsertPage (ULONG  ulZoneIndex, addr_t  ulAddr, PLW_VMM_PAGE  pvmpage)
{
    __areaInsertPage(&_G_vmareaZoneSpace[ulZoneIndex], ulAddr, pvmpage);
}
/*********************************************************************************************************
** 函数名称: __areaUnlinkPage
** 功能描述: 从地址空间中解链一个页面控制块
** 输　入  : pvmarea       地址空间管理块
**           ulAddr        起始地址
**           pvmpage       页面控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __areaUnlinkPage (PLW_VMM_AREA  pvmarea, addr_t  ulAddr, PLW_VMM_PAGE  pvmpage)
{
             ULONG              ulHashIndex;
    REGISTER PLW_TREE_RB_ROOT   ptrbr;
    REGISTER PLW_TREE_RB_NODE   ptrbn = &pvmpage->PAGE_trbnNode;
    
    ulHashIndex = __VMM_AREA_HASH_INDEX(pvmarea, ulAddr);
    ptrbr       = &pvmarea->AREA_ptrbrHash[ulHashIndex];                /*  确定 hash 表位置            */
    
    _Tree_Rb_Erase(ptrbn, ptrbr);
}
/*********************************************************************************************************
** 函数名称: __areaVirtualUnlinkPage
** 功能描述: 从虚拟地址空间中解链一个页面控制块
** 输　入  : ulAddr        起始地址
**           pvmpage       页面控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __areaVirtualUnlinkPage (addr_t  ulAddr, PLW_VMM_PAGE  pvmpage)
{
    REGISTER PLW_MMU_CONTEXT  pmmuctx = __vmmGetCurCtx();

    __areaUnlinkPage(&pmmuctx->MMUCTX_vmareaVirSpace, ulAddr, pvmpage);
}
/*********************************************************************************************************
** 函数名称: __areaPhysicalUnlinkPage
** 功能描述: 从物理地址空间中解链一个页面控制块
** 输　入  : pvmarea       地址空间管理块
**           ulAddr        起始地址
**           pvmpage       页面控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __areaPhysicalUnlinkPage (ULONG  ulZoneIndex, addr_t  ulAddr, PLW_VMM_PAGE  pvmpage)
{
    __areaUnlinkPage(&_G_vmareaZoneSpace[ulZoneIndex], ulAddr, pvmpage);
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
