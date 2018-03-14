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
** 文   件   名: virPage.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 12 月 09 日
**
** 描        述: 虚拟内存管理.

** BUG:
2013.05.24  加入虚拟空间对齐开辟.
*********************************************************************************************************/

#ifndef __VIRPAGE_H
#define __VIRPAGE_H

/*********************************************************************************************************
  虚拟空间操作
*********************************************************************************************************/

PLW_MMU_VIRTUAL_DESC    __vmmVirtualDesc(UINT32  uiType, ULONG  ulZoneIndex, ULONG  *pulFreePage);
addr_t                  __vmmVirtualSwitch(VOID);

BOOL                    __vmmVirtualIsInside(addr_t  ulAddr);
ULONG                   __vmmVirtualCreate(LW_MMU_VIRTUAL_DESC   pvirdes[]);

PLW_VMM_PAGE            __vmmVirtualPageAlloc(ULONG  ulPageNum);
PLW_VMM_PAGE            __vmmVirDevPageAlloc(ULONG  ulPageNum);

PLW_VMM_PAGE            __vmmVirtualPageAllocAlign(ULONG  ulPageNum, size_t  stAlign);
PLW_VMM_PAGE            __vmmVirDevPageAllocAlign(ULONG  ulPageNum, size_t  stAlign);

VOID                    __vmmVirtualPageFree(PLW_VMM_PAGE  pvmpage);
VOID                    __vmmVirDevPageFree(PLW_VMM_PAGE  pvmpage);

#endif                                                                  /*  __VIRPAGE_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
