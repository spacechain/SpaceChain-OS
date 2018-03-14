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
** 文   件   名: pageTable.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 12 月 09 日
**
** 描        述: 平台无关虚拟内存管理, 页表的相关操作库.

** BUG:
2009.02.24  键入内部全局映射 __vmmLibGlobalInit() 函数.
2009.03.03  修改 __vmmLibGlobalInit() 注释.
2009.03.04  MMU 全局映射关系由 __vmmLibGlobalMap() 全权负责, 
            __VMM_MMU_GLOBAL_INIT() 由驱动初始化体系相关关键信息.
2009.03.05  __vmmLibPageMap() 对页表操作后, 如果页表处于 cache 区域, 回写 cache 由 MMU 驱动程序处理.
            加入处理器设计为物理地址类型 cache 则不需要回写 cache , 例如 ARM1136.
2009.05.21  初始化时对 iErrNo 的判断有错误.
2009.11.10  __vmmLibVirtualToPhysical() 错误时返回页面无效代码.
2009.12.29  __vmmLibGlobalMap() 错误打印函数字串错误.
2010.08.13  __vmmLibInit() 中调用 __vmm_pgd_alloc() 地址为 0 .
            __vmmLibPageMap() 使用 __vmm_pgd_alloc() 获取一级页表对应项入口.
2011.03.02  加入 __vmmLibGetFlag() 函数.
2011.05.20  当 __vmmLibGetFlag() 获得非有效映射时, 必须返回映射无效错误.
2013.08.20  无效快表时, 需要通知其他的 CPU 无效快表.
2013.07.20  加入主从核分离的 MMU 初始化.
2014.11.09  VMM LIB 初始化不再启动 MMU.
2016.05.01  加入虚拟地址空间重叠判断.
2017.07.15  支持 L4 虚拟化 MMU 操作接口.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "virPage.h"
/*********************************************************************************************************
  体系结构函数声明
*********************************************************************************************************/
extern VOID   __ARCH_MMU_INIT(CPCHAR  pcMachineName);                   /*  BSP MMU 初始化函数          */
/*********************************************************************************************************
  内部全局映射函数声明
*********************************************************************************************************/
static INT    __vmmLibGlobalMap(PLW_MMU_CONTEXT   pmmuctx, LW_MMU_PHYSICAL_DESC  pphydesc[]);
/*********************************************************************************************************
** 函数名称: __vmmGetCurCtx
** 功能描述: 获得当前 MMU 上下文 (内部使用或驱动程序使用)
** 输　入  : NONE
** 输　出  : 当前正在执行的 MMU 上下文
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PLW_MMU_CONTEXT __vmmGetCurCtx (VOID)
{
    static LW_MMU_CONTEXT   mmuctxGlobal;                               /*  全局 MMU CTX                */

    return  (&mmuctxGlobal);                                            /*  目前不打算支持, 每一个任务  */
                                                                        /*  拥有自己的虚拟地址空间      */
}
/*********************************************************************************************************
** 函数名称: __vmmLibVirtualOverlap
** 功能描述: 虚拟空间地址冲突检查
** 输　入  : ulAddr         地址
**           stSize         长度
** 输　出  : 地址空间是否冲突
** 全局变量:
** 调用模块:
*********************************************************************************************************/
BOOL  __vmmLibVirtualOverlap (addr_t  ulAddr, size_t  stSize)
{
#define __ADDR_OVERLAP(pvirdesc, addr)    \
        if (((addr) >= pvirdesc->VIRD_ulVirAddr) && \
            ((addr) < (pvirdesc->VIRD_ulVirAddr +   \
                       pvirdesc->VIRD_stSize))) {   \
            return  (LW_TRUE);  \
        }

    INT                     i;
    PLW_MMU_VIRTUAL_DESC    pvirdescApp;
    PLW_MMU_VIRTUAL_DESC    pvirdescDev;

    for (i = 0; i < LW_CFG_VMM_VIR_NUM; i++) {
        pvirdescApp = __vmmVirtualDesc(LW_VIRTUAL_MEM_APP, i, LW_NULL);
        if (pvirdescApp->VIRD_stSize) {
            __ADDR_OVERLAP(pvirdescApp, ulAddr);
            __ADDR_OVERLAP(pvirdescApp, ulAddr + stSize - 1);
        }
    }

    pvirdescDev = __vmmVirtualDesc(LW_VIRTUAL_MEM_DEV, 0, LW_NULL);
    if (pvirdescDev->VIRD_stSize) {
        __ADDR_OVERLAP(pvirdescDev, ulAddr);
        __ADDR_OVERLAP(pvirdescDev, ulAddr + stSize - 1);
    }

    return  (LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: __vmmLibPrimaryInit
** 功能描述: 初始化 MMU 功能, CPU 构架相关。(多核模式下, 为主核 MMU 初始化)
** 输　入  : pphydesc          物理内存区描述表
**           pcMachineName     正在运行的机器名称
** 输　出  : BSP 函数返回值
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  __vmmLibPrimaryInit (LW_MMU_PHYSICAL_DESC  pphydesc[], CPCHAR  pcMachineName)
{
    static   BOOL             bIsInit          = LW_FALSE;
             BOOL             bIsNeedGlobalMap = LW_FALSE;

    REGISTER PLW_MMU_CONTEXT  pmmuctx = __vmmGetCurCtx();
    REGISTER INT              iError;
    REGISTER ULONG            ulError;
    
             INT              iErrLevel = 0;

    if (bIsInit == LW_FALSE) {
        __ARCH_MMU_INIT(pcMachineName);                                 /*  初始化 MMU 函数             */
        bIsInit          = LW_TRUE;
        bIsNeedGlobalMap = LW_TRUE;
    }
    
    iError = __VMM_MMU_MEM_INIT(pmmuctx);                               /*  创建页表内存缓冲区          */
    if (iError < ERROR_NONE) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "mmu memory init error.\r\n");
        return  (ERROR_KERNEL_MEMORY);
    }
    
    _DebugHandle(__LOGMESSAGE_LEVEL, "mmu initialize. start memory pagination...\r\n");
    
#if LW_CFG_VMM_L4_HYPERVISOR_EN > 0
    pmmuctx->MMUCTX_iProcId = 0;                                        /*  未来扩展接口                */
    
#else
    /*
     *  __vmm_pgd_alloc() 地址为 0 , 表示页表基址 + 0 偏移量, 所以返回的页表项就是页表基址.
     */
    pmmuctx->MMUCTX_pgdEntry = __vmm_pgd_alloc(pmmuctx, 0ul);           /*  创建 PGD 区域               */
    if (pmmuctx->MMUCTX_pgdEntry == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "mmu can not allocate pgd entry.\r\n");
        iErrLevel = 1;
        ulError   = ERROR_KERNEL_MEMORY;
        goto    __error_handle;
    }
#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */
    
    if (bIsNeedGlobalMap) {
        iError = __VMM_MMU_GLOBAL_INIT(pcMachineName);                  /*  初始化体系相关关键数据      */
        if (iError < ERROR_NONE) {
            iErrLevel = 2;
            ulError   = errno;
            goto    __error_handle;
        }
        
        iError = __vmmLibGlobalMap(pmmuctx, pphydesc);                  /*  全局内存关系映射            */
        if (iError < ERROR_NONE) {
            iErrLevel = 2;
            ulError   = errno;
            goto    __error_handle;
        }
    }
    
    __VMM_MMU_MAKE_CURCTX(pmmuctx);                                     /*  设置页表基地址              */
    KN_SMP_MB();
    
    return  (ERROR_NONE);
    
__error_handle:
#if LW_CFG_VMM_L4_HYPERVISOR_EN == 0
    if (iErrLevel > 1) {
        __vmm_pgd_free(pmmuctx->MMUCTX_pgdEntry);
    }
#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */
    
    _ErrorHandle(ulError);
    return  (ulError);
}
/*********************************************************************************************************
** 函数名称: __vmmLibSecondaryInit
** 功能描述: 初始化 MMU 功能, CPU 构架相关。(多核模式下, 为主核 MMU 初始化)
** 输　入  : pcMachineName     正在运行的机器名称
** 输　出  : BSP 函数返回值
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

ULONG  __vmmLibSecondaryInit (CPCHAR  pcMachineName)
{
    REGISTER PLW_MMU_CONTEXT  pmmuctx = __vmmGetCurCtx();
    REGISTER INT              iError;
    REGISTER ULONG            ulError;
    
    iError = __VMM_MMU_GLOBAL_INIT(pcMachineName);                      /*  初始化体系相关关键数据      */
    if (iError < ERROR_NONE) {
        ulError = errno;
        return  (ulError);
    }
    
    __VMM_MMU_MAKE_CURCTX(pmmuctx);                                     /*  设置页表位置                */
    KN_SMP_MB();
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_SMP_EN               */
/*********************************************************************************************************
** 函数名称: __vmmLibGlobalMap
** 功能描述: 全局页面映射关系设置
** 输　入  : pmmuctx        MMU 上下文
**           pphydesc       物理内存区描述表
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __vmmLibGlobalMap (PLW_MMU_CONTEXT   pmmuctx, LW_MMU_PHYSICAL_DESC  pphydesc[]) 
{
    INT     i;
    ULONG   ulError;
    ULONG   ulPageNum;

    for (i = 0; pphydesc[i].PHYD_stSize; i++) {
        if ((pphydesc[i].PHYD_uiType == LW_PHYSICAL_MEM_BUSPOOL) ||
            (pphydesc[i].PHYD_uiType == LW_PHYSICAL_MEM_APP)     ||
            (pphydesc[i].PHYD_uiType == LW_PHYSICAL_MEM_DMA)) {
            continue;
        }
        
        ulPageNum = (ULONG)(pphydesc[i].PHYD_stSize >> LW_CFG_VMM_PAGE_SHIFT);
        if (pphydesc[i].PHYD_stSize & ~LW_CFG_VMM_PAGE_MASK) {
            ulPageNum++;
        }
        
        _BugFormat(__vmmLibVirtualOverlap(pphydesc[i].PHYD_ulVirMap, 
                                          pphydesc[i].PHYD_stSize), LW_TRUE,
                   "global map vaddr 0x%08lx size: 0x%08zx overlap with virtual space.\r\n",
                   pphydesc[i].PHYD_ulVirMap, pphydesc[i].PHYD_stSize);
    
        switch (pphydesc[i].PHYD_uiType) {
        
        case LW_PHYSICAL_MEM_TEXT:
            ulError = __vmmLibPageMap(pphydesc[i].PHYD_ulPhyAddr, 
                                      pphydesc[i].PHYD_ulVirMap,
                                      ulPageNum, LW_VMM_FLAG_EXEC | LW_VMM_FLAG_RDWR);
            break;
            
        case LW_PHYSICAL_MEM_DATA:
            ulError = __vmmLibPageMap(pphydesc[i].PHYD_ulPhyAddr, 
                                      pphydesc[i].PHYD_ulVirMap,
                                      ulPageNum, LW_VMM_FLAG_RDWR);
            break;
            
        case LW_PHYSICAL_MEM_VECTOR:
            ulError = __vmmLibPageMap(pphydesc[i].PHYD_ulPhyAddr, 
                                      pphydesc[i].PHYD_ulVirMap,
                                      ulPageNum, LW_VMM_FLAG_EXEC);
            break;
            
        case LW_PHYSICAL_MEM_BOOTSFR:
            ulError = __vmmLibPageMap(pphydesc[i].PHYD_ulPhyAddr, 
                                      pphydesc[i].PHYD_ulVirMap,
                                      ulPageNum, LW_VMM_FLAG_DMA);
            break;
            
        default:
            ulError = ERROR_NONE;
            break;
        }
        
        if (ulError) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "vmm global map fail.\r\n");
            return  (PX_ERROR);
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
  TextUpdate Parameter
*********************************************************************************************************/
#if LW_CFG_VMM_L4_HYPERVISOR_EN == 0

#if LW_CFG_SMP_EN > 0

typedef struct {
    PLW_MMU_CONTEXT     FTLB_pmmuctx;
    addr_t              FTLB_ulPageAddr;
    ULONG               FTLB_ulPageNum;
} LW_VMM_FTLB_ARG;
/*********************************************************************************************************
** 函数名称: __vmmLibFlushTblTlb
** 功能描述: 刷新当前 CPU TLB (IPI 执行)
** 输　入  : pftlb     刷新参数
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __vmmLibFlushTlbIpi (LW_VMM_FTLB_ARG  *pftlb)
{
    INTREG      iregInterLevel;

    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */
    __VMM_MMU_INV_TLB(pftlb->FTLB_pmmuctx, 
                      pftlb->FTLB_ulPageAddr, 
                      pftlb->FTLB_ulPageNum);                           /*  无效快表                    */
    KN_INT_ENABLE(iregInterLevel);                                      /*  打开中断                    */
}

#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
** 函数名称: __vmmLibFlushTlb
** 功能描述: 刷新 TLB
** 输　入  : pmmuctx       MMU 上下文
**           ulPageAddr    页面虚拟地址
**           ulPageNum     页面个数
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __vmmLibFlushTlb (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulPageAddr, ULONG  ulPageNum)
{
    INTREG           iregInterLevel;
#if LW_CFG_SMP_EN > 0
    LW_VMM_FTLB_ARG  ftlb;
    BOOL             bLock;
#endif                                                                  /*  LW_CFG_SMP_EN               */
    
    ulPageAddr &= LW_CFG_VMM_PAGE_MASK;                                 /*  地址对齐                    */
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */
    __VMM_MMU_INV_TLB(pmmuctx, ulPageAddr, ulPageNum);                  /*  无效快表                    */
    KN_INT_ENABLE(iregInterLevel);                                      /*  打开中断                    */

#if LW_CFG_SMP_EN > 0
    if (LW_SYS_STATUS_IS_RUNNING() && 
        (__VMM_MMU_OPTION() & LW_VMM_MMU_FLUSH_TLB_MP) &&
        (LW_NCPUS > 1)) {                                               /*  需要通知其他 CPU            */
        ftlb.FTLB_pmmuctx    = pmmuctx;
        ftlb.FTLB_ulPageAddr = ulPageAddr;
        ftlb.FTLB_ulPageNum  = ulPageNum;
        
        bLock = __SMP_CPU_LOCK();                                       /*  锁定当前 CPU 执行           */
        _SmpCallFuncAllOther((FUNCPTR)__vmmLibFlushTlbIpi, &ftlb,
                             LW_NULL, LW_NULL, IPIM_OPT_NORMAL);        /*  通知其他的 CPU              */
        __SMP_CPU_UNLOCK(bLock);                                        /*  解锁当前 CPU 执行           */
    }
#endif                                                                  /*  LW_CFG_SMP_EN               */
}

#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */
/*********************************************************************************************************
** 函数名称: __vmmLibPageMap
** 功能描述: 将物理页面重新映射
** 输　入  : ulPhysicalAddr        物理页面地址
**           ulVirtualAddr         需要映射的虚拟地址
**           ulPageNum             需要映射的页面个数
**           ulFlag                页面标志
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  __vmmLibPageMap (addr_t  ulPhysicalAddr, 
                        addr_t  ulVirtualAddr, 
                        ULONG   ulPageNum, 
                        ULONG   ulFlag)
{
    ULONG                    i;
    PLW_MMU_CONTEXT          pmmuctx = __vmmGetCurCtx();
    
#if LW_CFG_VMM_L4_HYPERVISOR_EN == 0
    INTREG                   iregInterLevel;
    addr_t                   ulVirtualTlb = ulVirtualAddr;
    
    LW_PGD_TRANSENTRY       *p_pgdentry;
    LW_PMD_TRANSENTRY       *p_pmdentry;
#if LW_CFG_VMM_PAGE_4L_EN > 0
    LW_PTS_TRANSENTRY       *p_ptsentry;
#endif                                                                  /*  LW_CFG_VMM_PAGE_4L_EN > 0   */
    LW_PTE_TRANSENTRY       *p_pteentry;
#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */
    
#if LW_CFG_VMM_L4_HYPERVISOR_EN > 0
    if (ulFlag & LW_VMM_FLAG_ACCESS) {
        for (i = 0; i < ulPageNum; i++) {
            if (__VMM_MMU_PAGE_MAP(pmmuctx, ulPhysicalAddr, 
                                   ulVirtualAddr, ulFlag) < ERROR_NONE) {
                return  (ERROR_VMM_LOW_LEVEL);
            }
            ulPhysicalAddr += LW_CFG_VMM_PAGE_SIZE;
            ulVirtualAddr  += LW_CFG_VMM_PAGE_SIZE;
        }
    
    } else {
        for (i = 0; i < ulPageNum; i++) {
            if (__VMM_MMU_PAGE_UNMAP(pmmuctx, ulVirtualAddr) < ERROR_NONE) {
                return  (ERROR_VMM_LOW_LEVEL);
            }
            ulPhysicalAddr += LW_CFG_VMM_PAGE_SIZE;
            ulVirtualAddr  += LW_CFG_VMM_PAGE_SIZE;
        }
    }
    
#else
    for (i = 0; i < ulPageNum; i++) {
        p_pgdentry = __vmm_pgd_alloc(pmmuctx, ulVirtualAddr);
        if (p_pgdentry == LW_NULL) {
            return  (ERROR_VMM_LOW_LEVEL);
        }
        
        p_pmdentry = __vmm_pmd_alloc(pmmuctx, p_pgdentry, ulVirtualAddr);
        if (p_pmdentry == LW_NULL) {
            return  (ERROR_VMM_LOW_LEVEL);
        }
        
#if LW_CFG_VMM_PAGE_4L_EN > 0
        p_ptsentry = __vmm_pts_alloc(pmmuctx, p_pmdentry, ulVirtualAddr);
        if (p_ptsentry == LW_NULL) {
            return  (ERROR_VMM_LOW_LEVEL);
        }
        
        p_pteentry = __vmm_pte_alloc(pmmuctx, p_ptsentry, ulVirtualAddr);
#else
        p_pteentry = __vmm_pte_alloc(pmmuctx, p_pmdentry, ulVirtualAddr);
#endif                                                                  /*  LW_CFG_VMM_PAGE_4L_EN > 0   */

        if (p_pteentry == LW_NULL) {
            return  (ERROR_VMM_LOW_LEVEL);
        }
        
        iregInterLevel = KN_INT_DISABLE();                              /*  关闭中断                    */
        __VMM_MMU_MAKE_TRANS(pmmuctx, p_pteentry,
                             ulVirtualAddr,
                             ulPhysicalAddr, ulFlag);                   /*  创建映射关系                */
        KN_INT_ENABLE(iregInterLevel);                                  /*  打开中断                    */
        
        ulPhysicalAddr += LW_CFG_VMM_PAGE_SIZE;
        ulVirtualAddr  += LW_CFG_VMM_PAGE_SIZE;
    }
    
    __vmmLibFlushTlb(pmmuctx, ulVirtualTlb, ulPageNum);                 /*  同步刷新所有 CPU TLB        */
#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __vmmLibGetFlag
** 功能描述: 获取指定逻辑地址的访问权限
** 输　入  : ulVirtualAddr         需要映射的虚拟地址
**           pulFlag               页面标志
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  __vmmLibGetFlag (addr_t  ulVirtualAddr, ULONG  *pulFlag)
{
    PLW_MMU_CONTEXT     pmmuctx = __vmmGetCurCtx();
    ULONG               ulFlag;

    ulFlag = __VMM_MMU_FLAG_GET(pmmuctx, ulVirtualAddr);
    if (pulFlag) {
        *pulFlag = ulFlag;
    }
    
    if (ulFlag & LW_VMM_FLAG_VALID) {
        return  (ERROR_NONE);
    
    } else {
        _ErrorHandle(ERROR_VMM_PAGE_INVAL);
        return  (ERROR_VMM_PAGE_INVAL);
    }
}
/*********************************************************************************************************
** 函数名称: __vmmLibSetFlag
** 功能描述: 设置指定逻辑地址的访问权限
** 输　入  : ulVirtualAddr         需要映射的虚拟地址
**           ulPageNum             页面数量
**           ulFlag                页面标志
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  __vmmLibSetFlag (addr_t  ulVirtualAddr, ULONG   ulPageNum, ULONG  ulFlag)
{
    INTREG              iregInterLevel;
    INT                 i;
    PLW_MMU_CONTEXT     pmmuctx = __vmmGetCurCtx();
    INT                 iError;
    
#if LW_CFG_VMM_L4_HYPERVISOR_EN == 0
    addr_t              ulVirtualTlb = ulVirtualAddr;
#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */

    for (i = 0; i < ulPageNum; i++) {                                   /*  重新映射这些页面            */
        iregInterLevel = KN_INT_DISABLE();                              /*  关闭中断                    */
        iError = __VMM_MMU_FLAG_SET(pmmuctx, ulVirtualAddr, ulFlag);
        KN_INT_ENABLE(iregInterLevel);                                  /*  打开中断                    */
        
        _BugHandle((iError < 0), LW_FALSE, "set page flag error,\r\n");
        
        ulVirtualAddr += LW_CFG_VMM_PAGE_SIZE;
    }
    
#if LW_CFG_VMM_L4_HYPERVISOR_EN == 0
    __vmmLibFlushTlb(pmmuctx, ulVirtualTlb, ulPageNum);                 /*  同步刷新所有 CPU TLB        */
#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __vmmLibVirtualToPhysical
** 功能描述: 通过虚拟地址查询对应的物理地址
** 输　入  : ulVirtualAddr      虚拟地址
**           pulPhysicalAddr    返回的物理地址
** 输　出  : 虚拟地址, 错误返回 (void *)-1 地址.
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  __vmmLibVirtualToPhysical (addr_t  ulVirtualAddr, addr_t  *pulPhysicalAddr)
{
    INT                      iError;
    
#if LW_CFG_VMM_L4_HYPERVISOR_EN == 0
    PLW_MMU_CONTEXT          pmmuctx = __vmmGetCurCtx();
    LW_PGD_TRANSENTRY       *p_pgdentry;
    LW_PMD_TRANSENTRY       *p_pmdentry;
#if LW_CFG_VMM_PAGE_4L_EN > 0
    LW_PTS_TRANSENTRY       *p_ptsentry;
#endif                                                                  /*  LW_CFG_VMM_PAGE_4L_EN > 0   */
    LW_PTE_TRANSENTRY       *p_pteentry;
#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */
    
#if LW_CFG_VMM_L4_HYPERVISOR_EN > 0
    iError = __VMM_MMU_PHYS_GET(ulVirtualAddr, pulPhysicalAddr);
    if (iError < 0) {
        return  (ERROR_VMM_LOW_LEVEL);
    }
    
    return  (ERROR_NONE);

#else
    p_pgdentry = __VMM_MMU_PGD_OFFSET(pmmuctx, ulVirtualAddr);
    if (__VMM_MMU_PGD_NONE((*p_pgdentry))) {                            /*  判断 PGD 条目正确性         */
        goto    __error_handle;
    }
    
    p_pmdentry = __VMM_MMU_PMD_OFFSET(p_pgdentry, ulVirtualAddr);
    if (__VMM_MMU_PMD_NONE((*p_pmdentry))) {                            /*  判断 PMD 条目正确性         */
        goto    __error_handle;
    }
    
#if LW_CFG_VMM_PAGE_4L_EN > 0
    p_ptsentry = __VMM_MMU_PTS_OFFSET(p_pmdentry, ulVirtualAddr);
    if (__VMM_MMU_PTS_NONE((*p_ptsentry))) {                            /*  判断 PTS 条目正确性         */
        goto    __error_handle;
    }
    p_pteentry = __VMM_MMU_PTE_OFFSET(p_ptsentry, ulVirtualAddr);
#else
    p_pteentry = __VMM_MMU_PTE_OFFSET(p_pmdentry, ulVirtualAddr);
#endif                                                                  /*  LW_CFG_VMM_PAGE_4L_EN > 0   */
    
    if (__VMM_MMU_PTE_NONE((*p_pteentry))) {                            /*  判断 PTE 条目正确性         */
        goto    __error_handle;
    }
    
    iError = __VMM_MMU_PHYS_GET((*p_pteentry), pulPhysicalAddr);        /*  查询页面基地址              */
    if (iError < 0) {
        return  (ERROR_VMM_LOW_LEVEL);
    
    } else {
        *pulPhysicalAddr = (ulVirtualAddr & (LW_CFG_VMM_PAGE_SIZE - 1))
                         + (*pulPhysicalAddr);                          /*  加入页内偏移量              */
        return  (ERROR_NONE);
    }
    
__error_handle:
    _ErrorHandle(ERROR_VMM_PAGE_INVAL);
    return  (ERROR_VMM_PAGE_INVAL);
#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
