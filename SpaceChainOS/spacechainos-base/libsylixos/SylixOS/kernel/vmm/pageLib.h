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
** 文   件   名: pageLib.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 12 月 09 日
**
** 描        述: 平台无关虚拟内存管理, 基础页操作库.

** BUG:
2009.11.10  LW_VMM_ZONE 中加入 DMA 区域判断符, 表明该区域是否可供 DMA 使用.
2013.05.30  页面控制块中加入引用次数和指向真实物理页面控制块的指针, 用于共享物理页面.
*********************************************************************************************************/

#ifndef __PAGELIB_H
#define __PAGELIB_H

/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
/*********************************************************************************************************
  buddy free area struct (diff than linux)
*********************************************************************************************************/

typedef struct __lw_vmm_freearea {
    PLW_LIST_LINE           FA_lineFreeHeader;                          /*  空闲物理页面链表            */
    ULONG                   FA_ulCount;                                 /*  当前 hash 入口节点数量      */
} LW_VMM_FREEAREA;
typedef LW_VMM_FREEAREA    *PLW_VMM_FREEAREA;

/*********************************************************************************************************
  physical zone struct 
*********************************************************************************************************/

typedef struct __lw_vmm_zone {
    ULONG                   ZONE_ulFreePage;                            /*  空闲页面的个数              */
    addr_t                  ZONE_ulAddr;                                /*  分区起始地址                */
    size_t                  ZONE_stSize;                                /*  分区大小                    */
    UINT                    ZONE_uiAttr;                                /*  区域属性                    */
    LW_VMM_FREEAREA         ZONE_vmfa[LW_CFG_VMM_MAX_ORDER];            /*  空闲页面 hash 表            */
} LW_VMM_ZONE;
typedef LW_VMM_ZONE        *PLW_VMM_ZONE;

/*********************************************************************************************************
  page struct (当页面为物理页面时 PAGE_lineFreeHash 在使用时复用做物理页面链表, hash key 为虚拟地址)
*********************************************************************************************************/

typedef struct __lw_vmm_page {
    LW_LIST_LINE            PAGE_lineFreeHash;                          /*  free area hash 中的链表     */
    LW_LIST_LINE            PAGE_lineManage;                            /*  左右邻居指针                */
    LW_TREE_RB_NODE         PAGE_trbnNode;                              /*  红黑树节点                  */
    
    ULONG                   PAGE_ulCount;                               /*  页面个数                    */
    addr_t                  PAGE_ulPageAddr;                            /*  页面地址                    */
    
#define __VMM_PAGE_TYPE_PHYSICAL      0                                 /*  物理空间页面                */
#define __VMM_PAGE_TYPE_VIRTUAL       1                                 /*  虚拟空间页面                */
    INT                     PAGE_iPageType;                             /*  页面类型                    */
    ULONG                   PAGE_ulFlags;                               /*  页面属性                    */
    BOOL                    PAGE_bUsed;                                 /*  页面是否在使用              */
    PLW_VMM_ZONE            PAGE_pvmzoneOwner;                          /*  页面所属区域                */
    
    union {
        struct {
            /*
             *  以下字段只在此页面为物理页面时有效.
             *  如果 PAGE_pvmpageReal 有效, 则此物理页面控制块为伪造, 真实的物理页面控制块为 
             *  PAGE_pvmpageReal 指针的目标. PAGE_iChange 表示共享区域发生改变, 
             */
            addr_t                  PPAGE_ulMapPageAddr;                /*  页面被映射的地址 物理->虚拟 */
            INT                     PPAGE_iChange;                      /*  物理页面已经变化            */
            ULONG                   PPAGE_ulRef;                        /*  物理页面引用次数            */
            struct __lw_vmm_page   *PPAGE_pvmpageReal;                  /*  指向真实物理页面            */
        } __phy_page_status;
        
        struct {
            /*
             *  以下字段只在此页面为虚拟页面时有效. 
             *  PAGE_pvPrivate 数据结构仅对缺页中断管理部分有效.
             *  如页面是虚拟页面, 则 VPAGE_pvAreaCb 指向 LW_VMM_PAGE_PRIVATE 数据结构
             */
            PVOID                   VPAGE_pvAreaCb;                     /*  缺页中断区域控制块          */
            LW_LIST_LINE_HEADER    *VPAGE_plinePhyLink;                 /*  物理页面 hash 链表          */
        } __vir_page_status;
    
    } __lw_vmm_page_status;
} LW_VMM_PAGE;
typedef LW_VMM_PAGE        *PLW_VMM_PAGE;

#define PAGE_ulMapPageAddr  __lw_vmm_page_status.__phy_page_status.PPAGE_ulMapPageAddr
#define PAGE_iChange        __lw_vmm_page_status.__phy_page_status.PPAGE_iChange
#define PAGE_ulRef          __lw_vmm_page_status.__phy_page_status.PPAGE_ulRef
#define PAGE_pvmpageReal    __lw_vmm_page_status.__phy_page_status.PPAGE_pvmpageReal

#define PAGE_pvAreaCb       __lw_vmm_page_status.__vir_page_status.VPAGE_pvAreaCb
#define PAGE_plinePhyLink   __lw_vmm_page_status.__vir_page_status.VPAGE_plinePhyLink

#define LW_VMM_PAGE_IS_FAKE(pvmpagePhysical)    (pvmpagePhysical->PAGE_pvmpageReal ? LW_TRUE : LW_FALSE)
#define LW_VMM_PAGE_GET_REAL(pvmpagePhysical)   (pvmpagePhysical->PAGE_pvmpageReal)

/*********************************************************************************************************
  映射地址无效
*********************************************************************************************************/

#define PAGE_MAP_ADDR_INV   ((addr_t)PX_ERROR)

/*********************************************************************************************************
  init
*********************************************************************************************************/

ULONG         __pageCbInit(ULONG  ulPageNum);

/*********************************************************************************************************
  service
*********************************************************************************************************/

ULONG         __pageZoneCreate(PLW_VMM_ZONE   pvmzone,
                               addr_t         ulAddr, 
                               size_t         stSize, 
                               UINT           uiAttr,
                               INT            iPageType);               /*  创建区域                    */
PLW_VMM_PAGE  __pageAllocate(PLW_VMM_ZONE  pvmzone, 
                             ULONG         ulPageNum, 
                             INT           iPageType);                  /*  分配连续分页                */
PLW_VMM_PAGE  __pageAllocateAlign(PLW_VMM_ZONE  pvmzone, 
                                  ULONG         ulPageNum, 
                                  size_t        stAlign,
                                  INT           iPageType);             /*  分配指定对齐关系连续分页    */
VOID          __pageFree(PLW_VMM_ZONE   pvmzone, 
                         PLW_VMM_PAGE   pvmpage);                       /*  回收连续分页                */
ULONG         __pageSplit(PLW_VMM_PAGE   pvmpage, 
                          PLW_VMM_PAGE  *ppvmpageSplit, 
                          ULONG          ulPageNum,
                          PVOID          pvAreaCb);                     /*  页面拆分                    */
ULONG         __pageExpand(PLW_VMM_PAGE   pvmpage, 
                           ULONG          ulExpPageNum);                /*  页面扩展                    */
ULONG         __pageMerge(PLW_VMM_PAGE   pvmpageL, 
                          PLW_VMM_PAGE   pvmpageR);                     /*  页面合并                    */
VOID          __pageLink(PLW_VMM_PAGE   pvmpageVirtual, 
                         PLW_VMM_PAGE   pvmpagePhysical);               /*  页面连接                    */
VOID          __pageUnlink(PLW_VMM_PAGE  pvmpageVirtual, 
                           PLW_VMM_PAGE  pvmpagePhysical);              /*  解除页面连接                */
PLW_VMM_PAGE  __pageFindLink(PLW_VMM_PAGE  pvmpageVirtual, 
                             addr_t        ulVirAddr);                  /*  查找页面连接                */
VOID          __pageTraversalLink(PLW_VMM_PAGE   pvmpageVirtual,
                                  VOIDFUNCPTR    pfunc, 
                                  PVOID          pvArg0,
                                  PVOID          pvArg1,
                                  PVOID          pvArg2,
                                  PVOID          pvArg3,
                                  PVOID          pvArg4,
                                  PVOID          pvArg5);               /*  遍历页面连接                */
ULONG         __pageGetMinContinue(PLW_VMM_ZONE  pvmzone);              /*  获得当前最小连续分页个数    */

#endif                                                                  /*  __VMM_H                     */
#endif                                                                  /*  __PAGELIB_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
