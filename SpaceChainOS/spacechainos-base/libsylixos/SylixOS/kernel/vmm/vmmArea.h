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
** 文   件   名: vmmArea.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 12 月 09 日
**
** 描        述: 平台无关虚拟内存管理, 地址空间管理. 提供地址反查功能.
                 地址反查使用哈希红黑树.
*********************************************************************************************************/

#ifndef __VMMAREA_H
#define __VMMAREA_H

/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
/*********************************************************************************************************
  地址空间
*********************************************************************************************************/

typedef struct __vmm_area {
    PLW_TREE_RB_ROOT     AREA_ptrbrHash;                                /*  hash rb tree 表             */
    ULONG                AREA_ulHashSize;                               /*  hash 表大小                 */
    addr_t               AREA_ulAreaAddr;                               /*  空间起始地址                */
    size_t               AREA_stAreaSize;                               /*  空间大小                    */
} LW_VMM_AREA;
typedef LW_VMM_AREA     *PLW_VMM_AREA;

/*********************************************************************************************************
  service
*********************************************************************************************************/
ULONG  __areaVirtualSpaceInit(LW_MMU_VIRTUAL_DESC   pvirdes[]);
ULONG  __areaPhysicalSpaceInit(LW_MMU_PHYSICAL_DESC  pphydesc[]);

VOID  __areaVirtualSpaceTraversal(VOIDFUNCPTR  pfuncCallback);
VOID  __areaPhysicalSpaceTraversal(ULONG  ulZoneIndex, VOIDFUNCPTR  pfuncCallback);

PLW_VMM_PAGE  __areaVirtualSearchPage(addr_t  ulAddr);
PLW_VMM_PAGE  __areaPhysicalSearchPage(ULONG  ulZoneIndex, addr_t  ulAddr);

VOID  __areaVirtualInsertPage(ULONG  ulAddr, PLW_VMM_PAGE  pvmpage);
VOID  __areaPhysicalInsertPage(ULONG  ulZoneIndex, addr_t  ulAddr, PLW_VMM_PAGE  pvmpage);

VOID  __areaVirtualUnlinkPage(ULONG  ulAddr, PLW_VMM_PAGE  pvmpage);
VOID  __areaPhysicalUnlinkPage(ULONG  ulZoneIndex, addr_t  ulAddr, PLW_VMM_PAGE  pvmpage);

#endif                                                                  /*  __VMM_H                     */
#endif                                                                  /*  __VMMAREA_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
