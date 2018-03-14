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
** 文   件   名: pageTable.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 12 月 09 日
**
** 描        述: 平台无关虚拟内存管理, 页表的相关操作库.

** BUG:
2009.06.18  加入虚拟地址到物理地址的查询 API 和 BSP 接口.
2013.12.21  加入是否可执行选项.
*********************************************************************************************************/

#ifndef __PAGETABLE_H
#define __PAGETABLE_H

/*********************************************************************************************************
  mmu 页面标志
*********************************************************************************************************/

#define LW_VMM_FLAG_VALID               0x01                            /*  映射有效                    */
#define LW_VMM_FLAG_UNVALID             0x00                            /*  映射无效                    */

#define LW_VMM_FLAG_ACCESS              0x02                            /*  可以访问                    */
#define LW_VMM_FLAG_UNACCESS            0x00                            /*  不能访问                    */

#define LW_VMM_FLAG_WRITABLE            0x04                            /*  可以写操作                  */
#define LW_VMM_FLAG_UNWRITABLE          0x00                            /*  不可以写操作                */

#define LW_VMM_FLAG_EXECABLE            0x08                            /*  可以执行代码                */
#define LW_VMM_FLAG_UNEXECABLE          0x00                            /*  不可以执行代码              */

#define LW_VMM_FLAG_CACHEABLE           0x10                            /*  可以缓冲                    */
#define LW_VMM_FLAG_UNCACHEABLE         0x00                            /*  不可以缓冲                  */

#define LW_VMM_FLAG_BUFFERABLE          0x20                            /*  可以写缓冲                  */
#define LW_VMM_FLAG_UNBUFFERABLE        0x00                            /*  不可以写缓冲                */

#define LW_VMM_FLAG_GUARDED             0x40                            /*  进行严格的权限检查          */
#define LW_VMM_FLAG_UNGUARDED           0x00                            /*  不进行严格的权限检查        */

/*********************************************************************************************************
  默认页面标志
*********************************************************************************************************/

#define LW_VMM_FLAG_EXEC                (LW_VMM_FLAG_VALID |        \
                                         LW_VMM_FLAG_ACCESS |       \
                                         LW_VMM_FLAG_EXECABLE |     \
                                         LW_VMM_FLAG_CACHEABLE |    \
                                         LW_VMM_FLAG_BUFFERABLE |   \
                                         LW_VMM_FLAG_GUARDED)           /*  可执行区域                  */

#define LW_VMM_FLAG_READ                (LW_VMM_FLAG_VALID |        \
                                         LW_VMM_FLAG_ACCESS |       \
                                         LW_VMM_FLAG_CACHEABLE |    \
                                         LW_VMM_FLAG_BUFFERABLE |   \
                                         LW_VMM_FLAG_GUARDED)           /*  只读区域                    */
                                         
#define LW_VMM_FLAG_RDWR                (LW_VMM_FLAG_VALID |        \
                                         LW_VMM_FLAG_ACCESS |       \
                                         LW_VMM_FLAG_WRITABLE |     \
                                         LW_VMM_FLAG_CACHEABLE |    \
                                         LW_VMM_FLAG_BUFFERABLE |   \
                                         LW_VMM_FLAG_GUARDED)           /*  读写区域                    */

#define LW_VMM_FLAG_DMA                 (LW_VMM_FLAG_VALID |        \
                                         LW_VMM_FLAG_ACCESS |       \
                                         LW_VMM_FLAG_WRITABLE |     \
                                         LW_VMM_FLAG_GUARDED)           /*  物理硬件映射 (CACHE 一致的) */
                                         
#define LW_VMM_FLAG_FAIL                (LW_VMM_FLAG_VALID |        \
                                         LW_VMM_FLAG_UNACCESS |     \
                                         LW_VMM_FLAG_GUARDED)           /*  不允许访问区域              */

/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
/*********************************************************************************************************
  mmu 执行功能
*********************************************************************************************************/

#if LW_CFG_VMM_L4_HYPERVISOR_EN > 0

typedef struct {
    FUNCPTR                  MMUOP_pfuncMemInit;                        /*  初始化内存, (页表和目录项)  */
    FUNCPTR                  MMUOP_pfuncGlobalInit;                     /*  初始化全局映射关系          */
    
    ULONGFUNCPTR             MMUOP_pfuncFlagGet;                        /*  获得页面标志                */
    FUNCPTR                  MMUOP_pfuncFlagSet;                        /*  设置页面标志 (当前未使用)   */
    
    FUNCPTR                  MMUOP_pfuncPageMap;                        /*  映射内存页面                */
    FUNCPTR                  MMUOP_pfuncPageUnmap;                      /*  释放映射关系                */
    FUNCPTR                  MMUOP_pfuncVirToPhy;                       /*  获得物理地址                */
} LW_MMU_OP;

#else                                                                   /* !LW_CFG_VMM_L4_HYPERVISOR_EN */

#ifdef __cplusplus
typedef LW_PGD_TRANSENTRY  *(*PGDFUNCPTR)(...);
typedef LW_PMD_TRANSENTRY  *(*PMDFUNCPTR)(...);
#if LW_CFG_VMM_PAGE_4L_EN > 0
typedef LW_PTS_TRANSENTRY  *(*PTSFUNCPTR)(...);
#endif                                                                  /*  LW_CFG_VMM_PAGE_4L_EN > 0   */
typedef LW_PTE_TRANSENTRY  *(*PTEFUNCPTR)(...);

#else
typedef LW_PGD_TRANSENTRY  *(*PGDFUNCPTR)();
typedef LW_PMD_TRANSENTRY  *(*PMDFUNCPTR)();
#if LW_CFG_VMM_PAGE_4L_EN > 0
typedef LW_PTS_TRANSENTRY  *(*PTSFUNCPTR)();
#endif                                                                  /*  LW_CFG_VMM_PAGE_4L_EN > 0   */
typedef LW_PTE_TRANSENTRY  *(*PTEFUNCPTR)();
#endif                                                                  /*  __cplusplus                 */

typedef struct {
    ULONG                    MMUOP_ulOption;                            /*  MMU 选项                    */
#define LW_VMM_MMU_FLUSH_TLB_MP     0x01                                /*  每一个核是否都要清快表      */

    FUNCPTR                  MMUOP_pfuncMemInit;                        /*  初始化内存, (页表和目录项)  */
    FUNCPTR                  MMUOP_pfuncGlobalInit;                     /*  初始化全局映射关系          */
    
    PGDFUNCPTR               MMUOP_pfuncPGDAlloc;                       /*  创建 PGD 空间               */
    VOIDFUNCPTR              MMUOP_pfuncPGDFree;                        /*  释放 PGD 空间               */
    PMDFUNCPTR               MMUOP_pfuncPMDAlloc;                       /*  创建 PMD 空间               */
    VOIDFUNCPTR              MMUOP_pfuncPMDFree;                        /*  释放 PMD 空间               */
#if LW_CFG_VMM_PAGE_4L_EN > 0
    PTSFUNCPTR               MMUOP_pfuncPTSAlloc;                       /*  创建 PTS 空间               */
    VOIDFUNCPTR              MMUOP_pfuncPTSFree;                        /*  释放 PTS 空间               */
#endif                                                                  /*  LW_CFG_VMM_PAGE_4L_EN > 0   */
    PTEFUNCPTR               MMUOP_pfuncPTEAlloc;                       /*  创建 PTE 空间               */
    VOIDFUNCPTR              MMUOP_pfuncPTEFree;                        /*  释放 PTE 空间               */

    BOOLFUNCPTR              MMUOP_pfuncPGDIsOk;                        /*  PGD 入口项是否正确          */
    BOOLFUNCPTR              MMUOP_pfuncPMDIsOk;                        /*  PMD 入口项是否正确          */
#if LW_CFG_VMM_PAGE_4L_EN > 0
    BOOLFUNCPTR              MMUOP_pfuncPTSIsOk;                        /*  PTS 入口项是否正确          */
#endif                                                                  /*  LW_CFG_VMM_PAGE_4L_EN > 0   */
    BOOLFUNCPTR              MMUOP_pfuncPTEIsOk;                        /*  PTE 入口项是否正确          */

    PGDFUNCPTR               MMUOP_pfuncPGDOffset;                      /*  通过地址获得指定 PGD 表项   */
    PMDFUNCPTR               MMUOP_pfuncPMDOffset;                      /*  通过地址获得指定 PMD 表项   */
#if LW_CFG_VMM_PAGE_4L_EN > 0
    PTSFUNCPTR               MMUOP_pfuncPTSOffset;                      /*  通过地址获得指定 PTS 表项   */
#endif                                                                  /*  LW_CFG_VMM_PAGE_4L_EN > 0   */
    PTEFUNCPTR               MMUOP_pfuncPTEOffset;                      /*  通过地址获得指定 PTE 表项   */
    
    FUNCPTR                  MMUOP_pfuncPTEPhysGet;                     /*  通过 PTE 条目获取物理地址   */
    
    ULONGFUNCPTR             MMUOP_pfuncFlagGet;                        /*  获得页面标志                */
    FUNCPTR                  MMUOP_pfuncFlagSet;                        /*  设置页面标志 (当前未使用)   */
    
    VOIDFUNCPTR              MMUOP_pfuncMakeTrans;                      /*  设置页面转换关系描述符      */
    VOIDFUNCPTR              MMUOP_pfuncMakeCurCtx;                     /*  激活当前的页表转换关系      */
    VOIDFUNCPTR              MMUOP_pfuncInvalidateTLB;                  /*  无效 TLB 表                 */
    
    VOIDFUNCPTR              MMUOP_pfuncSetEnable;                      /*  启动 MMU                    */
    VOIDFUNCPTR              MMUOP_pfuncSetDisable;                     /*  关闭 MMU                    */
} LW_MMU_OP;

#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */

typedef LW_MMU_OP           *PLW_MMU_OP;
extern  LW_MMU_OP            _G_mmuOpLib;                               /*  MMU 操作函数集              */

/*********************************************************************************************************
  MMU 锁
*********************************************************************************************************/
#ifndef __VMM_MAIN_FILE
extern LW_OBJECT_HANDLE     _G_ulVmmLock;
#endif

#define __VMM_LOCK()        API_SemaphoreMPend(_G_ulVmmLock, LW_OPTION_WAIT_INFINITE)
#define __VMM_UNLOCK()      API_SemaphoreMPost(_G_ulVmmLock)

/*********************************************************************************************************
  MMU 获得选项信息
*********************************************************************************************************/
#if LW_CFG_VMM_L4_HYPERVISOR_EN == 0

#define __VMM_MMU_OPTION()                      _G_mmuOpLib.MMUOP_ulOption

#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */
/*********************************************************************************************************
  MMU 操作宏
*********************************************************************************************************/

#define __VMM_MMU_MEM_INIT(pmmuctx)             (_G_mmuOpLib.MMUOP_pfuncMemInit) ?  \
            _G_mmuOpLib.MMUOP_pfuncMemInit(pmmuctx) : (PX_ERROR)
#define __VMM_MMU_GLOBAL_INIT(pcmachine)        (_G_mmuOpLib.MMUOP_pfuncGlobalInit) ?   \
            _G_mmuOpLib.MMUOP_pfuncGlobalInit(pcmachine) : (PX_ERROR)
            
/*********************************************************************************************************
  MMU 页面开辟与释放
*********************************************************************************************************/
#if LW_CFG_VMM_L4_HYPERVISOR_EN > 0

#define __VMM_MMU_PAGE_MAP(pmmuctx, ulPhyAddr, ulVirAddr, ulFlag)   (_G_mmuOpLib.MMUOP_pfuncPageMap) ? \
            _G_mmuOpLib.MMUOP_pfuncPageMap(pmmuctx, ulPhyAddr, ulVirAddr, ulFlag) : (PX_ERROR)

#define __VMM_MMU_PAGE_UNMAP(pmmuctx, ulVirAddr)    (_G_mmuOpLib.MMUOP_pfuncPageUnmap) ? \
            _G_mmuOpLib.MMUOP_pfuncPageUnmap(pmmuctx, ulVirAddr) : (PX_ERROR)

#define __VMM_MMU_PHYS_GET(ulVirAddr, pulPhysicalAddr)  (_G_mmuOpLib.MMUOP_pfuncVirToPhy) ? \
            _G_mmuOpLib.MMUOP_pfuncVirToPhy(ulVirAddr, pulPhysicalAddr) : (PX_ERROR)

#else                                                                   /* !LW_CFG_VMM_L4_HYPERVISOR_EN */

#define __VMM_MMU_PGD_ALLOC(pmmuctx, ulAddr)                (_G_mmuOpLib.MMUOP_pfuncPGDAlloc) ? \
            _G_mmuOpLib.MMUOP_pfuncPGDAlloc(pmmuctx, ulAddr) : (LW_NULL)
#define __VMM_MMU_PGD_FREE(p_pgdentry)  \
        if (_G_mmuOpLib.MMUOP_pfuncPGDFree) {   \
            _G_mmuOpLib.MMUOP_pfuncPGDFree(p_pgdentry);                         \
        }
#define __VMM_MMU_PMD_ALLOC(pmmuctx, p_pgdentry, ulAddr)    (_G_mmuOpLib.MMUOP_pfuncPMDAlloc) ? \
            _G_mmuOpLib.MMUOP_pfuncPMDAlloc(pmmuctx, p_pgdentry, ulAddr) : (LW_NULL)
#define __VMM_MMU_PMD_FREE(p_pmdentry)  \
        if (_G_mmuOpLib.MMUOP_pfuncPMDFree) {   \
            _G_mmuOpLib.MMUOP_pfuncPMDFree(p_pmdentry); \
        }
        
#if LW_CFG_VMM_PAGE_4L_EN > 0
#define __VMM_MMU_PTS_ALLOC(pmmuctx, p_pmdentry, ulAddr)    (_G_mmuOpLib.MMUOP_pfuncPTSAlloc) ? \
            _G_mmuOpLib.MMUOP_pfuncPTSAlloc(pmmuctx, p_pmdentry, ulAddr) : (LW_NULL)
#define __VMM_MMU_PTS_FREE(p_ptsentry)  \
        if (_G_mmuOpLib.MMUOP_pfuncPTSFree) {   \
            _G_mmuOpLib.MMUOP_pfuncPTSFree(p_ptsentry); \
        }
#define __VMM_MMU_PTE_ALLOC(pmmuctx, p_ptsentry, ulAddr)    (_G_mmuOpLib.MMUOP_pfuncPTEAlloc) ? \
            _G_mmuOpLib.MMUOP_pfuncPTEAlloc(pmmuctx, p_ptsentry, ulAddr) : (LW_NULL)
#else                                                                   /*  LW_CFG_VMM_PAGE_4L_EN > 0   */
#define __VMM_MMU_PTE_ALLOC(pmmuctx, p_pmdentry, ulAddr)    (_G_mmuOpLib.MMUOP_pfuncPTEAlloc) ? \
            _G_mmuOpLib.MMUOP_pfuncPTEAlloc(pmmuctx, p_pmdentry, ulAddr) : (LW_NULL)
#endif                                                                  /*  !LW_CFG_VMM_PAGE_4L_EN > 0  */

#define __VMM_MMU_PTE_FREE(p_pteentry)  \
        if (_G_mmuOpLib.MMUOP_pfuncPTEFree) {   \
            _G_mmuOpLib.MMUOP_pfuncPTEFree(p_pteentry); \
        }
/*********************************************************************************************************
  MMU 页面描述符判断
*********************************************************************************************************/

#define __VMM_MMU_PGD_NONE(pgdentry)    (_G_mmuOpLib.MMUOP_pfuncPGDIsOk) ? \
            !(_G_mmuOpLib.MMUOP_pfuncPGDIsOk(pgdentry)) : (LW_TRUE)
#define __VMM_MMU_PMD_NONE(pmdentry)    (_G_mmuOpLib.MMUOP_pfuncPMDIsOk) ? \
            !(_G_mmuOpLib.MMUOP_pfuncPMDIsOk(pmdentry)) : (LW_TRUE)
#if LW_CFG_VMM_PAGE_4L_EN > 0
#define __VMM_MMU_PTS_NONE(ptsentry)    (_G_mmuOpLib.MMUOP_pfuncPTSIsOk) ? \
            !(_G_mmuOpLib.MMUOP_pfuncPTSIsOk(ptsentry)) : (LW_TRUE)
#endif                                                                  /*  LW_CFG_VMM_PAGE_4L_EN > 0   */
#define __VMM_MMU_PTE_NONE(pteentry)    (_G_mmuOpLib.MMUOP_pfuncPTEIsOk) ? \
            !(_G_mmuOpLib.MMUOP_pfuncPTEIsOk(pteentry)) : (LW_TRUE)
            
/*********************************************************************************************************
  MMU 页面描述符获取
*********************************************************************************************************/

#define __VMM_MMU_PGD_OFFSET(pmmuctx, ulAddr)       (_G_mmuOpLib.MMUOP_pfuncPGDOffset) ?    \
            _G_mmuOpLib.MMUOP_pfuncPGDOffset(pmmuctx, ulAddr) : (LW_NULL)
#define __VMM_MMU_PMD_OFFSET(p_pgdentry, ulAddr)    (_G_mmuOpLib.MMUOP_pfuncPMDOffset) ?    \
            _G_mmuOpLib.MMUOP_pfuncPMDOffset(p_pgdentry, ulAddr) : (LW_NULL)
#if LW_CFG_VMM_PAGE_4L_EN > 0
#define __VMM_MMU_PTS_OFFSET(p_pmdentry, ulAddr)    (_G_mmuOpLib.MMUOP_pfuncPTSOffset) ?    \
            _G_mmuOpLib.MMUOP_pfuncPTSOffset(p_pmdentry, ulAddr) : (LW_NULL)
#define __VMM_MMU_PTE_OFFSET(p_ptsentry, ulAddr)    (_G_mmuOpLib.MMUOP_pfuncPTEOffset) ?    \
            _G_mmuOpLib.MMUOP_pfuncPTEOffset(p_ptsentry, ulAddr) : (LW_NULL)
#else                                                                   /*  LW_CFG_VMM_PAGE_4L_EN > 0   */
#define __VMM_MMU_PTE_OFFSET(p_pmdentry, ulAddr)    (_G_mmuOpLib.MMUOP_pfuncPTEOffset) ?    \
            _G_mmuOpLib.MMUOP_pfuncPTEOffset(p_pmdentry, ulAddr) : (LW_NULL)
#endif                                                                  /*  !LW_CFG_VMM_PAGE_4L_EN > 0  */

/*********************************************************************************************************
  MMU 获取物理地址
*********************************************************************************************************/

#define __VMM_MMU_PHYS_GET(pteentry, pulPhysicalAddr)   (_G_mmuOpLib.MMUOP_pfuncPTEPhysGet) ?    \
            _G_mmuOpLib.MMUOP_pfuncPTEPhysGet(pteentry, pulPhysicalAddr) : (PX_ERROR)

#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */

/*********************************************************************************************************
  MMU 页面描述符标志
*********************************************************************************************************/

#define __VMM_MMU_FLAG_GET(pmmuctx, ulAddr)         (_G_mmuOpLib.MMUOP_pfuncFlagGet) ?  \
            _G_mmuOpLib.MMUOP_pfuncFlagGet(pmmuctx, ulAddr) : 0ul
#define __VMM_MMU_FLAG_SET(pmmuctx, ulAddr, ulFlag) (_G_mmuOpLib.MMUOP_pfuncFlagSet) ?  \
            _G_mmuOpLib.MMUOP_pfuncFlagSet(pmmuctx, ulAddr, (ulFlag)) : (PX_ERROR)
            
/*********************************************************************************************************
  MMU 内部操作
*********************************************************************************************************/
#if LW_CFG_VMM_L4_HYPERVISOR_EN > 0

#define __VMM_MMU_MAKE_CURCTX(pmmuctx)      (VOID)pmmuctx               /*  未来扩展                    */

#else

#define __VMM_MMU_MAKE_TRANS(pmmuctx, p_pteentry, ulVirtualAddr, ulPhysicalAddr, ulFlag)   \
        if (_G_mmuOpLib.MMUOP_pfuncMakeTrans) { \
            _G_mmuOpLib.MMUOP_pfuncMakeTrans(pmmuctx, p_pteentry,                       \
                                             ulVirtualAddr, ulPhysicalAddr, (ulFlag));  \
        }
#define __VMM_MMU_MAKE_CURCTX(pmmuctx)  \
        if (_G_mmuOpLib.MMUOP_pfuncMakeCurCtx) {    \
            _G_mmuOpLib.MMUOP_pfuncMakeCurCtx(pmmuctx); \
        }
#define __VMM_MMU_INV_TLB(pmmuctx, ulPageAddr, ulPageNum)  \
        if (_G_mmuOpLib.MMUOP_pfuncInvalidateTLB) { \
            _G_mmuOpLib.MMUOP_pfuncInvalidateTLB(pmmuctx, ulPageAddr, ulPageNum); \
        }
#define __VMM_MMU_ENABLE()  \
        if (_G_mmuOpLib.MMUOP_pfuncSetEnable) { \
            _G_mmuOpLib.MMUOP_pfuncSetEnable(); \
        }
#define __VMM_MMU_DISABLE() \
        if (_G_mmuOpLib.MMUOP_pfuncSetDisable) { \
            _G_mmuOpLib.MMUOP_pfuncSetDisable(); \
        }
        
#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */
/*********************************************************************************************************
  mmu 信息
*********************************************************************************************************/

typedef struct __lw_mmu_context {
    LW_VMM_AREA              MMUCTX_vmareaVirSpace;                     /*  虚拟地址空间反查表          */
#if LW_CFG_VMM_L4_HYPERVISOR_EN > 0
    INT                      MMUCTX_iProcId;
#else
    LW_PGD_TRANSENTRY       *MMUCTX_pgdEntry;                           /*  PGD 表入口地址              */
#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */
} LW_MMU_CONTEXT;
typedef LW_MMU_CONTEXT      *PLW_MMU_CONTEXT;

/*********************************************************************************************************
  VMM 内部匹配
*********************************************************************************************************/
#if LW_CFG_VMM_L4_HYPERVISOR_EN == 0

static LW_INLINE  LW_PGD_TRANSENTRY  *__vmm_pgd_alloc (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    if (pmmuctx->MMUCTX_pgdEntry == LW_NULL) {
        return  (__VMM_MMU_PGD_ALLOC(pmmuctx, ulAddr));
    } else {
        return  (__VMM_MMU_PGD_OFFSET(pmmuctx, ulAddr));
    }
}

static LW_INLINE  VOID  __vmm_pgd_free (LW_PGD_TRANSENTRY  *p_pgdentry)
{
    __VMM_MMU_PGD_FREE(p_pgdentry);
}

static LW_INLINE LW_PMD_TRANSENTRY   *__vmm_pmd_alloc (PLW_MMU_CONTEXT    pmmuctx, 
                                             LW_PGD_TRANSENTRY *p_pgdentry,
                                             addr_t             ulAddr)
{
    if (__VMM_MMU_PGD_NONE(*p_pgdentry)) {
        return  (__VMM_MMU_PMD_ALLOC(pmmuctx, p_pgdentry, ulAddr));
    } else {
        return  (__VMM_MMU_PMD_OFFSET(p_pgdentry, ulAddr));
    }
}

static LW_INLINE VOID  __vmm_pmd_free (LW_PMD_TRANSENTRY  *p_pmdentry)
{
    __VMM_MMU_PMD_FREE(p_pmdentry);
}

#if LW_CFG_VMM_PAGE_4L_EN > 0                                           /*  LW_CFG_VMM_PAGE_4L_EN > 0   */
static LW_INLINE LW_PTS_TRANSENTRY   *__vmm_pts_alloc (PLW_MMU_CONTEXT    pmmuctx, 
                                             LW_PMD_TRANSENTRY *p_pmdentry,
                                             addr_t             ulAddr)
{
    if (__VMM_MMU_PMD_NONE(*p_pmdentry)) {
        return  (__VMM_MMU_PTS_ALLOC(pmmuctx, p_pmdentry, ulAddr));
    } else {
        return  (__VMM_MMU_PTS_OFFSET(p_pmdentry, ulAddr));
    }
}

static LW_INLINE VOID  __vmm_pts_free (LW_PTS_TRANSENTRY  *p_ptsentry)
{
    __VMM_MMU_PTS_FREE(p_ptsentry);
}

static LW_INLINE LW_PTE_TRANSENTRY   *__vmm_pte_alloc (PLW_MMU_CONTEXT    pmmuctx, 
                                             LW_PTS_TRANSENTRY *p_ptsentry,
                                             addr_t             ulAddr)
{
    if (__VMM_MMU_PTS_NONE(*p_ptsentry)) {
        return  (__VMM_MMU_PTE_ALLOC(pmmuctx, p_ptsentry, ulAddr));
    } else {
        return  (__VMM_MMU_PTE_OFFSET(p_ptsentry, ulAddr));
    }
}

#else                                                                   /*  !LW_CFG_VMM_PAGE_4L_EN > 0  */
static LW_INLINE LW_PTE_TRANSENTRY   *__vmm_pte_alloc (PLW_MMU_CONTEXT    pmmuctx, 
                                             LW_PMD_TRANSENTRY *p_pmdentry,
                                             addr_t             ulAddr)
{
    if (__VMM_MMU_PMD_NONE(*p_pmdentry)) {
        return  (__VMM_MMU_PTE_ALLOC(pmmuctx, p_pmdentry, ulAddr));
    } else {
        return  (__VMM_MMU_PTE_OFFSET(p_pmdentry, ulAddr));
    }
}
#endif                                                                  /*  !LW_CFG_VMM_PAGE_4L_EN > 0  */

static LW_INLINE VOID  __vmm_pte_free (LW_PTE_TRANSENTRY  *p_pteentry)
{
    __VMM_MMU_PTE_FREE(p_pteentry);
}

#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */
/*********************************************************************************************************
  内部函数
*********************************************************************************************************/

PLW_MMU_CONTEXT         __vmmGetCurCtx(VOID);                           /*  get current mmu context     */
ULONG                   __vmmLibPrimaryInit(LW_MMU_PHYSICAL_DESC  pphydesc[],
                                            CPCHAR                pcMachineName);
                                                                        /*  init current mmu context    */
#if LW_CFG_SMP_EN > 0
ULONG                   __vmmLibSecondaryInit(CPCHAR  pcMachineName);
#endif                                                                  /*  LW_CFG_SMP_EN               */

ULONG                   __vmmLibPageMap(addr_t ulPhysicalAddr, 
                                        addr_t ulVirtualAddr, 
                                        ULONG  ulPageNum, 
                                        ULONG  ulFlag);                 /*  mmu map                     */
ULONG                   __vmmLibGetFlag(addr_t  ulVirtualAddr, ULONG  *pulFlag);
ULONG                   __vmmLibSetFlag(addr_t  ulVirtualAddr, ULONG   ulPageNum, ULONG  ulFlag);
ULONG                   __vmmLibVirtualToPhysical(addr_t  ulVirtualAddr, addr_t  *pulPhysicalAddr);

/*********************************************************************************************************
  bsp api
*********************************************************************************************************/

LW_API LW_MMU_OP       *API_VmmGetLibBlock(VOID);                       /*  BSP get mmu op lib          */

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
#endif                                                                  /*  __PAGETABLE_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
