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
** 描        述: 平台无关虚拟内存管理, 基础页操作库.

** BUG:
2009.09.30  显示 virtuals 时, 更加详细.
2009.11.13  加入 DMA 域打印与利用率打印.
2011.05.17  加入缺页中断信息显示.
2011.08.03  当虚拟页面没有 Link 物理页面时, 需要使用页表查询来确定映射的物理地址.
2013.06.04  显示虚拟空间信息时, 不再显示物理地址.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_FIO_LIB_EN > 0
#if LW_CFG_VMM_EN > 0
#include "../SylixOS/kernel/vmm/phyPage.h"
#include "../SylixOS/kernel/vmm/virPage.h"
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
#if LW_CFG_CPU_WORD_LENGHT == 64
static const CHAR   _G_cZoneInfoHdr[] = "\n\
ZONE     PHYSICAL         SIZE     PAGESIZE       PGD        FREEPAGE  DMA  USED\n\
---- ---------------- ------------ -------- ---------------- -------- ----- ----\n";
static const CHAR   _G_cAreaInfoHdr[] = "\n\
     VIRTUAL          SIZE        WRITE CACHE\n\
---------------- ---------------- ----- -----\n";
#else
static const CHAR   _G_cZoneInfoHdr[] = "\n\
ZONE PHYSICAL   SIZE   PAGESIZE    PGD   FREEPAGE  DMA  USED\n\
---- -------- -------- -------- -------- -------- ----- ----\n";
static const CHAR   _G_cAreaInfoHdr[] = "\n\
VIRTUAL    SIZE   WRITE CACHE\n\
-------- -------- ----- -----\n";
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT adj  */
/*********************************************************************************************************
** 函数名称: API_VmmPhysicalShow
** 功能描述: 显示 vmm 物理存储器信息
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_VmmPhysicalShow (VOID)
{
    REGISTER INT                    i;
             addr_t                 ulPhysicalAddr;
             LW_MMU_PHYSICAL_DESC   phydescKernel[2];
             PCHAR                  pcDma;
             UINT                   uiAttr;
             ULONG                  ulFreePage;
             addr_t                 ulPgd;
             size_t                 stSize;
             size_t                 stUsed;
             
             size_t                 stTotalSize = 0;
             size_t                 stFreeSize  = 0;

    printf("vmm physical zone show >>\n");
    printf(_G_cZoneInfoHdr);                                            /*  打印欢迎信息                */
    
    for (i = 0; i < LW_CFG_VMM_ZONE_NUM; i++) {
        if (API_VmmZoneStatus(i, &ulPhysicalAddr, &stSize, 
                              &ulPgd, &ulFreePage, &uiAttr)) {
            continue;
        }
        if (!stSize) {
            continue;
        }
        
        pcDma  = (uiAttr & LW_ZONE_ATTR_DMA) ? "true" : "false";
        stUsed = stSize - (ulFreePage << LW_CFG_VMM_PAGE_SHIFT);
        stUsed = (stUsed / (stSize / 100));                             /*  防止溢出                    */
        
#if LW_CFG_CPU_WORD_LENGHT == 64
        printf("%4d %16lx %12zx %8zx %16lx %8ld %-5s %3zd%%\n",
#else
        printf("%4d %08lx %8zx %8zx %08lx %8ld %-5s %3zd%%\n",
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT adj  */
               i, ulPhysicalAddr, stSize, (size_t)LW_CFG_VMM_PAGE_SIZE,
               ulPgd, ulFreePage, pcDma, stUsed);
               
        stTotalSize += stSize;
        stFreeSize  += (ulFreePage << LW_CFG_VMM_PAGE_SHIFT);
    }
    
    API_VmmPhysicalKernelDesc(&phydescKernel[0], &phydescKernel[1]);
    
    printf("\n"
           "ALL-Physical memory size: %zu MBytes (%zu Bytes)\n"
           "VMM-Physical memory size: %zu MBytes (%zu Bytes)\n"
           "VMM-Physical memory free: %zu MBytes (%zu Bytes)\n",
           (phydescKernel[0].PHYD_stSize +
            phydescKernel[1].PHYD_stSize +
            stTotalSize) / LW_CFG_MB_SIZE,
           (phydescKernel[0].PHYD_stSize +
            phydescKernel[1].PHYD_stSize +
            stTotalSize),
           stTotalSize / LW_CFG_MB_SIZE, stTotalSize,
           stFreeSize / LW_CFG_MB_SIZE, stFreeSize);
}
/*********************************************************************************************************
** 函数名称: __vmmVirtualPrint
** 功能描述: 打印信息回调函数
** 输　入  : pvmpage  页面信息控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __vmmVirtualPrint (PLW_VMM_PAGE  pvmpage)
{
    addr_t  ulVirtualAddr  = pvmpage->PAGE_ulPageAddr;
    
#if LW_CFG_CPU_WORD_LENGHT == 64
    printf("%16lx %16lx ", ulVirtualAddr, (pvmpage->PAGE_ulCount << LW_CFG_VMM_PAGE_SHIFT));
#else
    printf("%08lx %8lx ", ulVirtualAddr, (pvmpage->PAGE_ulCount << LW_CFG_VMM_PAGE_SHIFT));
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT adj  */

    if (pvmpage->PAGE_ulFlags & LW_VMM_FLAG_WRITABLE) {
        printf("true  ");
    } else {
        printf("false ");
    }
    
    if (pvmpage->PAGE_ulFlags & LW_VMM_FLAG_CACHEABLE) {
        printf("true\n");
    } else {
        printf("false\n");
    }
}
/*********************************************************************************************************
** 函数名称: API_VmmVirtualShow
** 功能描述: 显示 vmm 虚拟存储器信息
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_VmmVirtualShow (VOID)
{
    INT     i;
    addr_t  ulVirtualAddr;
    size_t  stSize;
    ULONG   ulFreePage;
    size_t  stUsed;
    
    printf("vmm virtual area show >>\n");
    
    for (i = 0; i < LW_CFG_VMM_VIR_NUM; i++) {
        if (API_VmmVirtualStatus(LW_VIRTUAL_MEM_APP, i, &ulVirtualAddr, &stSize, &ulFreePage)) {
            continue;
        }
        if (!stSize) {
            continue;
        }
        
        stUsed = stSize - (ulFreePage << LW_CFG_VMM_PAGE_SHIFT);
        stUsed = (stUsed / (stSize / 100));
        
        printf("vmm virtual program from: 0x%08lx, size: 0x%08zx, used: %zd%%\n", 
               ulVirtualAddr, stSize, stUsed);
    }

    if (API_VmmVirtualStatus(LW_VIRTUAL_MEM_DEV, 0, &ulVirtualAddr, &stSize, &ulFreePage)) {
        return;
    }
    
    stUsed = stSize - (ulFreePage << LW_CFG_VMM_PAGE_SHIFT);
    stUsed = (stUsed / (stSize / 100));
    
    printf("vmm virtual ioremap from: 0x%08lx, size: 0x%08zx, used: %zd%%\n", 
           ulVirtualAddr, stSize, stUsed);
    
    printf("vmm virtual area usage as follow:\n");
    
    printf(_G_cAreaInfoHdr);                                            /*  打印欢迎信息                */
    
    __VMM_LOCK();
    __areaVirtualSpaceTraversal(__vmmVirtualPrint);                     /*  遍历虚拟空间树, 打印信息    */
    __VMM_UNLOCK();
    
    printf("\n");
}
/*********************************************************************************************************
** 函数名称: API_VmmAbortShow
** 功能描述: 显示 vmm 访问中止信息
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_VmmAbortShow (VOID)
{
    LW_VMM_STATUS  vmms;
    
    API_VmmAbortStatus(&vmms);

    printf("vmm abort statistics infomation show >>\n");
    printf("vmm abort (memory access error) counter : %lld\n", vmms.VMMS_i64AbortCounter);
    printf("vmm page fail (alloc success) counter   : %lld\n", vmms.VMMS_i64PageFailCounter);
    printf("vmm alloc physical page error counter   : %lld\n", vmms.VMMS_i64PageLackCounter);
    printf("vmm page map error counter              : %lld\n", vmms.VMMS_i64MapErrCounter);
    
    printf("\n");
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
#endif                                                                  /*  LW_CFG_FIO_LIB_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
