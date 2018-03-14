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
** 文   件   名: virPage.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 12 月 09 日
**
** 描        述: 虚拟内存管理.

** BUG:
2013.05.24  加入虚拟空间对齐开辟.
2014.08.08  加入 __vmmVirtualDesc() 函数.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "phyPage.h"
#include "virPage.h"
/*********************************************************************************************************
  地址空间冲突检查
*********************************************************************************************************/
extern BOOL     __vmmLibVirtualOverlap(addr_t  ulAddr, size_t  stSize);
/*********************************************************************************************************
  虚拟空间描述
*********************************************************************************************************/
static LW_MMU_VIRTUAL_DESC  _G_vmvirDescApp[LW_CFG_VMM_VIR_NUM];        /*  应用程序                    */
static LW_MMU_VIRTUAL_DESC  _G_vmvirDescDev;                            /*  虚拟设备                    */
/*********************************************************************************************************
  设备虚拟空间 zone 控制块
*********************************************************************************************************/
static LW_VMM_ZONE          _G_vmzoneVirApp[LW_CFG_VMM_VIR_NUM];
static LW_VMM_ZONE          _G_vmzoneVirDev;
/*********************************************************************************************************
  切换通道
*********************************************************************************************************/
static addr_t               _G_ulVmmSwitchAddr = (addr_t)PX_ERROR;
/*********************************************************************************************************
** 函数名称: __vmmVirtualDesc
** 功能描述: 获得虚拟空间区域.
** 输　入  : uiType        类型
**           ulZoneIndex   虚拟区间下标
**           pulFreePage   剩余空间页面数
** 输　出  : 虚拟空间描述
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PLW_MMU_VIRTUAL_DESC  __vmmVirtualDesc (UINT32  uiType, ULONG  ulZoneIndex, ULONG  *pulFreePage)
{
    if (ulZoneIndex >= LW_CFG_VMM_VIR_NUM) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    if (uiType == LW_VIRTUAL_MEM_APP) {
        if (pulFreePage) {
            *pulFreePage = _G_vmzoneVirApp[ulZoneIndex].ZONE_ulFreePage;
        }
        return  (&_G_vmvirDescApp[ulZoneIndex]);
    
    } else {
        if (pulFreePage) {
            *pulFreePage = _G_vmzoneVirDev.ZONE_ulFreePage;
        }
        return  (&_G_vmvirDescDev);
    }
}
/*********************************************************************************************************
** 函数名称: __vmmVirtualSwitch
** 功能描述: 获得虚拟空间区域交换区.
** 输　入  : NONE
** 输　出  : 虚拟空间描述
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
addr_t  __vmmVirtualSwitch (VOID)
{
    return  (_G_ulVmmSwitchAddr);
}
/*********************************************************************************************************
** 函数名称: __vmmVirtualGetZone
** 功能描述: 确定虚拟 zone 下标.
** 输　入  : ulAddr        地址
** 输　出  : zone 下标.
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static BOOL  __vmmVirtualGetZone (addr_t  ulAddr)
{
             INT            i;
    REGISTER PLW_VMM_ZONE   pvmzone;

    for (i = 0; i < LW_CFG_VMM_VIR_NUM; i++) {
        pvmzone = &_G_vmzoneVirApp[i];
        if (!pvmzone->ZONE_stSize) {                                    /*  无效 zone                   */
            break;
        }
        if ((ulAddr >= pvmzone->ZONE_ulAddr) &&
            (ulAddr <  pvmzone->ZONE_ulAddr + pvmzone->ZONE_stSize)) {
            return  ((ULONG)i);
        }
    }
    
    return  (LW_CFG_VMM_VIR_NUM);
}
/*********************************************************************************************************
** 函数名称: __vmmVirtualIsInside
** 功能描述: 判断地址是否在虚拟空间中.
** 输　入  : ulAddr        地址
** 输　出  : 是否在 VMM 虚拟空间中
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
BOOL  __vmmVirtualIsInside (addr_t  ulAddr)
{
    ULONG   ulZoneIndex = __vmmVirtualGetZone(ulAddr);
    
    if (ulZoneIndex >= LW_CFG_VMM_VIR_NUM) {
        return  (LW_FALSE);
    }
    
    return  (LW_TRUE);
}
/*********************************************************************************************************
** 函数名称: __vmmVirtualCreate
** 功能描述: 创建虚拟空间区域.
** 输　入  : pvirdes       虚拟空间描述
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  __vmmVirtualCreate (LW_MMU_VIRTUAL_DESC   pvirdes[])
{
    REGISTER ULONG  ulError = ERROR_NONE;
             ULONG  ulZone  = 0;
             addr_t ulAddr;
             INT    i;
    
    for (i = 0; ; i++) {
        if (pvirdes[i].VIRD_stSize == 0) {
            break;
        }
        
        _BugFormat(!ALIGNED(pvirdes[i].VIRD_ulVirAddr, LW_CFG_VMM_PAGE_SIZE), LW_TRUE,
                   "virtual zone vaddr 0x%08lx not page aligned.\r\n", 
                   pvirdes[i].VIRD_ulVirAddr);
        
        _BugFormat(__vmmLibVirtualOverlap(pvirdes[i].VIRD_ulVirAddr, 
                                          pvirdes[i].VIRD_stSize), LW_TRUE,
                   "virtual zone vaddr 0x%08lx size: 0x%08zx overlap with virtual space.\r\n",
                   pvirdes[i].VIRD_ulVirAddr, pvirdes[i].VIRD_stSize);
                   
        switch (pvirdes[i].VIRD_uiType) {
        
        case LW_VIRTUAL_MEM_APP:
            _BugHandle((pvirdes[i].VIRD_ulVirAddr == (addr_t)LW_NULL), LW_TRUE,
                       "virtual APP area can not use NULL address, you can move offset page.\r\n");
                                                                        /*  目前不支持 NULL 起始地址    */
            if (ulZone < LW_CFG_VMM_VIR_NUM) {
                _G_vmvirDescApp[ulZone] = pvirdes[i];
                if (_G_ulVmmSwitchAddr == (addr_t)PX_ERROR) {
                    _G_ulVmmSwitchAddr =  pvirdes[i].VIRD_ulVirAddr;
                    ulAddr =  _G_ulVmmSwitchAddr + LW_CFG_VMM_PAGE_SIZE;
                
                } else {
                    ulAddr =  pvirdes[i].VIRD_ulVirAddr;
                }
                ulError = __pageZoneCreate(&_G_vmzoneVirApp[ulZone], 
                                           ulAddr, pvirdes[i].VIRD_stSize, 
                                           LW_ZONE_ATTR_NONE,
                                           __VMM_PAGE_TYPE_VIRTUAL);
                if (ulError) {
                    _DebugHandle(__ERRORMESSAGE_LEVEL, "kernel low memory.\r\n");
                    _ErrorHandle(ulError);
                    return  (ulError);
                }
                ulZone++;
            }
            break;
            
        case LW_VIRTUAL_MEM_DEV:
            _BugHandle((pvirdes[i].VIRD_ulVirAddr == (addr_t)LW_NULL), LW_TRUE,
                       "virtual device area can not use NULL address, you can move offset page.\r\n");
                                                                        /*  目前不支持 NULL 起始地址    */
            _BugHandle((_G_vmvirDescDev.VIRD_stSize), LW_TRUE,
                       "there are ONLY one virtual section allowed used to device.\r\n");
                       
            _G_vmvirDescDev = pvirdes[i];
            ulError = __pageZoneCreate(&_G_vmzoneVirDev, 
                                       pvirdes[i].VIRD_ulVirAddr, 
                                       pvirdes[i].VIRD_stSize, 
                                       LW_ZONE_ATTR_NONE,
                                       __VMM_PAGE_TYPE_VIRTUAL);
            if (ulError) {
                _DebugHandle(__ERRORMESSAGE_LEVEL, "kernel low memory.\r\n");
                _ErrorHandle(ulError);
                return  (ulError);
            }
            break;
            
        default:
            break;
        }
    }
    
    _BugHandle((_G_ulVmmSwitchAddr == (addr_t)PX_ERROR), LW_TRUE, 
               "virtual switich page invalidate.\r\n");
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __vmmVirtualPageAlloc
** 功能描述: 分配指定的连续虚拟页面
** 输　入  : ulPageNum     需要分配的虚拟页面个数
** 输　出  : 页面控制块
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PLW_VMM_PAGE  __vmmVirtualPageAlloc (ULONG  ulPageNum)
{
             INT            i;
    REGISTER PLW_VMM_ZONE   pvmzone;
    REGISTER PLW_VMM_PAGE   pvmpage = LW_NULL;
    
    for (i = 0; i < LW_CFG_VMM_VIR_NUM; i++) {
        pvmzone = &_G_vmzoneVirApp[i];
        if (!pvmzone->ZONE_stSize) {                                    /*  无效 zone                   */
            break;
        }
        if (pvmzone->ZONE_ulFreePage >= ulPageNum) {
            pvmpage = __pageAllocate(pvmzone, ulPageNum, __VMM_PAGE_TYPE_VIRTUAL);
            if (pvmpage) {
                return  (pvmpage);
            }
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: __vmmVirDevPageAlloc
** 功能描述: 分配指定的连续虚拟设备页面
** 输　入  : ulPageNum     需要分配的虚拟页面个数
** 输　出  : 页面控制块
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PLW_VMM_PAGE  __vmmVirDevPageAlloc (ULONG  ulPageNum)
{
    return  (__pageAllocate(&_G_vmzoneVirDev, ulPageNum, __VMM_PAGE_TYPE_VIRTUAL));
}
/*********************************************************************************************************
** 函数名称: __vmmVirtualPageAllocAlign
** 功能描述: 分配指定的连续虚拟页面 (指定对齐关系)
** 输　入  : ulPageNum     需要分配的虚拟页面个数
**           stAlign       内存对齐关系
** 输　出  : 页面控制块
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PLW_VMM_PAGE  __vmmVirtualPageAllocAlign (ULONG  ulPageNum, size_t  stAlign)
{
             INT            i;
    REGISTER PLW_VMM_ZONE   pvmzone;
    REGISTER PLW_VMM_PAGE   pvmpage = LW_NULL;
    
    for (i = 0; i < LW_CFG_VMM_VIR_NUM; i++) {
        pvmzone = &_G_vmzoneVirApp[i];
        if (!pvmzone->ZONE_stSize) {                                    /*  无效 zone                   */
            break;
        }
        if (pvmzone->ZONE_ulFreePage >= ulPageNum) {
            pvmpage = __pageAllocateAlign(pvmzone, ulPageNum, stAlign, __VMM_PAGE_TYPE_VIRTUAL);
            if (pvmpage) {
                return  (pvmpage);
            }
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: __vmmVirDevPageAllocAlign
** 功能描述: 分配指定的连续虚拟设备页面 (指定对齐关系)
** 输　入  : ulPageNum     需要分配的虚拟页面个数
**           stAlign       内存对齐关系
** 输　出  : 页面控制块
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PLW_VMM_PAGE  __vmmVirDevPageAllocAlign (ULONG  ulPageNum, size_t  stAlign)
{
    return  (__pageAllocateAlign(&_G_vmzoneVirDev, ulPageNum, 
                                 stAlign, __VMM_PAGE_TYPE_VIRTUAL));
}
/*********************************************************************************************************
** 函数名称: __vmmVirtualPageFree
** 功能描述: 回收指定的连虚拟页面
** 输　入  : pvmpage       页面控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __vmmVirtualPageFree (PLW_VMM_PAGE  pvmpage)
{
    ULONG   ulZoneIndex = __vmmVirtualGetZone(pvmpage->PAGE_ulPageAddr);

    __pageFree(&_G_vmzoneVirApp[ulZoneIndex], pvmpage);
}
/*********************************************************************************************************
** 函数名称: __vmmVirDevPageFree
** 功能描述: 回收指定的连虚拟设备页面
** 输　入  : pvmpage       页面控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __vmmVirDevPageFree (PLW_VMM_PAGE  pvmpage)
{
    __pageFree(&_G_vmzoneVirDev, pvmpage);
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
