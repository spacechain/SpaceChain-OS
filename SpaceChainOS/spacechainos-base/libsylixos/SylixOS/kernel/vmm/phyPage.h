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
** 文   件   名: phyPage.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 12 月 09 日
**
** 描        述: 物理页面管理.
*********************************************************************************************************/

#ifndef __PHYPAGE_H
#define __PHYPAGE_H

/*********************************************************************************************************
  ZONE
*********************************************************************************************************/

typedef struct __lw_vmm_zone_desc {
    addr_t                   ZONED_ulAddr;                              /*  起始地址                    */
    size_t                   ZONED_stSize;                              /*  区域长度                    */
    UINT                     ZONED_uiAttr;                              /*  区域属性                    */
} LW_VMM_ZONE_DESC;
typedef LW_VMM_ZONE_DESC    *PLW_VMM_ZONE_DESC;

/*********************************************************************************************************
  物理空间操作
*********************************************************************************************************/

ULONG           __vmmPhysicalCreate(LW_MMU_PHYSICAL_DESC  pphydesc[]);
PLW_VMM_PAGE    __vmmPhysicalPageAlloc(ULONG  ulPageNum, UINT  uiAttr, ULONG  *pulZoneIndex);
PLW_VMM_PAGE    __vmmPhysicalPageAllocZone(ULONG  ulZoneIndex, ULONG  ulPageNum, UINT  uiAttr);
PLW_VMM_PAGE    __vmmPhysicalPageAllocAlign(ULONG   ulPageNum, 
                                            size_t  stAlign, 
                                            UINT    uiAttr, 
                                            ULONG  *pulZoneIndex);
PLW_VMM_PAGE    __vmmPhysicalPageClone(PLW_VMM_PAGE  pvmpage);
PLW_VMM_PAGE    __vmmPhysicalPageRef(PLW_VMM_PAGE  pvmpage);
VOID            __vmmPhysicalPageFree(PLW_VMM_PAGE  pvmpage);
VOID            __vmmPhysicalPageFreeAll(PLW_VMM_PAGE  pvmpageVirtual);
VOID            __vmmPhysicalPageSetFlag(PLW_VMM_PAGE  pvmpage, ULONG  ulFlag);
VOID            __vmmPhysicalPageSetFlagAll(PLW_VMM_PAGE  pvmpageVirtual, ULONG  ulFlag);

#if LW_CFG_CACHE_EN > 0
VOID            __vmmPhysicalPageFlush(PLW_VMM_PAGE  pvmpage);
VOID            __vmmPhysicalPageFlushAll(PLW_VMM_PAGE  pvmpageVirtual);
VOID            __vmmPhysicalPageInvalidate(PLW_VMM_PAGE  pvmpage);
VOID            __vmmPhysicalPageInvalidateAll(PLW_VMM_PAGE  pvmpageVirtual);
VOID            __vmmPhysicalPageClear(PLW_VMM_PAGE  pvmpage);
VOID            __vmmPhysicalPageClearAll(PLW_VMM_PAGE  pvmpageVirtual);
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

ULONG           __vmmPhysicalGetZone(addr_t  ulAddr);
ULONG           __vmmPhysicalPageGetMinContinue(ULONG  *pulZoneIndex, UINT  uiAttr);
VOID            __vmmPhysicalGetKernelDesc(PLW_MMU_PHYSICAL_DESC  pphydescText, 
                                           PLW_MMU_PHYSICAL_DESC  pphydescData);

#endif                                                                  /*  __PHYPAGE_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
