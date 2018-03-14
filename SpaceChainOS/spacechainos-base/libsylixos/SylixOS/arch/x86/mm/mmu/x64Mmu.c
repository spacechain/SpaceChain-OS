/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: x64Mmu.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2017 年 06 月 09 日
**
** 描        述: x86-64 体系构架 MMU 驱动.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "arch/x86/common/x86Cr.h"
#include "arch/x86/pentium/x86Pentium.h"
/*********************************************************************************************************
  PML4E PDPTE PDE PTE 中的位定义
*********************************************************************************************************/
#define X64_MMU_PRESENT             (1)                                 /*  映射存在                    */
#define X64_MMU_PRESENT_SHIFT       (0)

#define X64_MMU_RW                  (1)                                 /*  可写                        */
#define X64_MMU_RW_NO               (0)
#define X64_MMU_RW_SHIFT            (1)

#define X64_MMU_US                  (1)                                 /*  用户模式可访问              */
#define X64_MMU_US_NO               (0)
#define X64_MMU_US_SHIFT            (2)

#define X64_MMU_PWT                 (1)                                 /*  写穿透                      */
#define X64_MMU_PWT_NO              (0)
#define X64_MMU_PWT_SHIFT           (3)

#define X64_MMU_PCD                 (1)                                 /*  不可 CACHE                  */
#define X64_MMU_PCD_NO              (0)
#define X64_MMU_PCD_SHIFT           (4)

#define X64_MMU_A                   (1)                                 /*  访问过                      */
#define X64_MMU_A_NO                (0)
#define X64_MMU_A_SHIFT             (5)

#define X64_MMU_XD                  (1)                                 /*  禁止执行                    */
#define X64_MMU_XD_NO               (0)
#define X64_MMU_XD_SHIFT            (63)
/*********************************************************************************************************
  以下 PDPTE PDE 有效
  PDPTE 中 PS 标志位表示是否提供 1G 的物理页面地址，0 为 4K 和 2M 页面
  PDE 中 PS 标志位表示是否提供 2M 的物理页面地址，0 为 4K 页面
*********************************************************************************************************/
#define X64_MMU_PS                  (1)
#define X64_MMU_PS_NO               (0)
#define X64_MMU_PS_SHIFT            (7)
/*********************************************************************************************************
  以下 PTE 有效
*********************************************************************************************************/
#define X64_MMU_D                   (1)                                 /*  脏位                        */
#define X64_MMU_D_NO                (0)
#define X64_MMU_D_SHIFT             (6)

#define X64_MMU_PAT                 (1)                                 /*  If the PAT is supported,    */
#define X64_MMU_PAT_NO              (0)                                 /*  indirectly determines       */
#define X64_MMU_PAT_SHIFT           (7)                                 /*  the memory type             */

#define X64_MMU_G                   (1)                                 /*  全局                        */
#define X64_MMU_G_NO                (0)
#define X64_MMU_G_SHIFT             (8)
/*********************************************************************************************************
  PML4E PDPTE PDE PTE 中的掩码
*********************************************************************************************************/
#define X64_MMU_MASK                (0xffffffffff000ul)                 /*  [51:12]                     */
/*********************************************************************************************************
  页面数大于其 1/2 时, 全清 TLB
*********************************************************************************************************/
#define X64_MMU_TLB_NR              (128)                               /*  Intel x86 vol 3 Table 11-1  */
/*********************************************************************************************************
  虚拟地址的大小
*********************************************************************************************************/
#define X64_MMU_VIRT_ADDR_SIZE      (48)
/*********************************************************************************************************
  外部函数声明
*********************************************************************************************************/
extern VOID  x64MmuInvalidateTLB(VOID);
extern VOID  x64MmuInvalidateTLBMVA(PVOID  pvAddr);

extern VOID  x64MmuEnable(VOID);
extern VOID  x64MmuDisable(VOID);

extern VOID  x86DCacheFlush(PVOID  pvStart, size_t  stSize);
/*********************************************************************************************************
  全局变量定义
*********************************************************************************************************/
static LW_OBJECT_HANDLE     _G_hPGDPartition;                           /*  系统目前仅使用一个 PGD      */
static LW_OBJECT_HANDLE     _G_hPMDPartition;                           /*  PMD 缓冲区                  */
static LW_OBJECT_HANDLE     _G_hPTSPartition;                           /*  PTS 缓冲区                  */
static LW_OBJECT_HANDLE     _G_hPTEPartition;                           /*  PTE 缓冲区                  */
/*********************************************************************************************************
** 函数名称: x64MmuFlags2Attr
** 功能描述: 根据 SylixOS 权限标志, 生成 x86 MMU 权限标志
** 输　入  : ulFlag                  SylixOS 权限标志
**           pucRW                   是否可写
**           pucUS                   是否普通用户
**           pucPWT                  是否写穿透
**           pucPCD                  是否 CACHE 关闭
**           pucA                    是否能访问
**           pucXD                   是否禁止执行
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  x64MmuFlags2Attr (ULONG   ulFlag,
                              UINT8  *pucRW,
                              UINT8  *pucUS,
                              UINT8  *pucPWT,
                              UINT8  *pucPCD,
                              UINT8  *pucA,
                              UINT8  *pucXD)
{
    if (!(ulFlag & LW_VMM_FLAG_VALID)) {                                /*  无效的映射关系              */
        return  (PX_ERROR);
    }

    if (ulFlag & LW_VMM_FLAG_WRITABLE) {                                /*  是否可写                    */
        *pucRW = X64_MMU_RW;

    } else {
        *pucRW = X64_MMU_RW_NO;
    }

    *pucUS = X64_MMU_US_NO;                                             /*  始终 supervisor             */

    if ((ulFlag & LW_VMM_FLAG_CACHEABLE) &&
        (ulFlag & LW_VMM_FLAG_BUFFERABLE)) {                            /*  CACHE 与 BUFFER 控制        */
        *pucPCD = X64_MMU_PCD_NO;
        *pucPWT = X64_MMU_PWT_NO;

    } else if (ulFlag & LW_VMM_FLAG_CACHEABLE) {
        *pucPCD = X64_MMU_PCD_NO;
        *pucPWT = X64_MMU_PWT;

    } else if (ulFlag & LW_VMM_FLAG_BUFFERABLE) {
        *pucPCD = X64_MMU_PCD_NO;
        *pucPWT = X64_MMU_PWT;

    } else {
        *pucPCD = X64_MMU_PCD;
        *pucPWT = X64_MMU_PWT;
    }

    if (ulFlag & LW_VMM_FLAG_ACCESS) {                                  /*  是否可访问                  */
        *pucA = X64_MMU_A;

    } else {
        *pucA = X64_MMU_A_NO;
    }

    if (ulFlag & LW_VMM_FLAG_EXECABLE) {                                /*  是否可执行                  */
        *pucXD = X64_MMU_XD_NO;

    } else {
        *pucXD = X64_MMU_XD;
    }

    return (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: x64MmuAttr2Flags
** 功能描述: 根据 x86 MMU 权限标志, 生成 SylixOS 权限标志
** 输　入  : ucRW                    是否可写
**           ucUS                    是否普通用户
**           ucPWT                   是否写穿透
**           ucPCD                   是否 CACHE 关闭
**           ucA                     是否能访问
**           ucXD                    是否禁止执行
**           pulFlag                 SylixOS 权限标志
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  x64MmuAttr2Flags (UINT8   ucRW,
                              UINT8   ucUS,
                              UINT8   ucPWT,
                              UINT8   ucPCD,
                              UINT8   ucA,
                              UINT8   ucXD,
                              ULONG  *pulFlag)
{
    *pulFlag = LW_VMM_FLAG_VALID;

    if (ucRW == X64_MMU_RW) {
        *pulFlag |= LW_VMM_FLAG_WRITABLE;

    } else {
        *pulFlag |= LW_VMM_FLAG_UNWRITABLE;
    }

    if (ucPCD == X64_MMU_PCD) {
        *pulFlag |= LW_VMM_FLAG_UNCACHEABLE;
        *pulFlag |= LW_VMM_FLAG_UNBUFFERABLE;

    } else {
        *pulFlag |= LW_VMM_FLAG_CACHEABLE;

        if (ucPWT == X64_MMU_PWT) {
            *pulFlag |= LW_VMM_FLAG_UNBUFFERABLE;
        } else {
            *pulFlag |= LW_VMM_FLAG_BUFFERABLE;
        }
    }

    if (ucA == X64_MMU_A) {
        *pulFlag |= LW_VMM_FLAG_ACCESS;

    } else {
        *pulFlag |= LW_VMM_FLAG_UNACCESS;
    }

    if (ucXD == X64_MMU_XD) {
        *pulFlag |= LW_VMM_FLAG_UNEXECABLE;

    } else {
        *pulFlag |= LW_VMM_FLAG_EXECABLE;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: x64MmuBuildPgdEntry
** 功能描述: 生成一个一级描述符 (PGD 描述符)
** 输　入  : ulBaseAddr              基地址     (二级页表基地址)
**           ucRW                    是否可写
**           ucUS                    是否普通用户
**           ucPWT                   是否写穿透
**           ucPCD                   是否 CACHE 关闭
**           ucA                     是否能访问
**           ucXD                    是否禁止执行
** 输　出  : 一级描述符
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_INLINE LW_PGD_TRANSENTRY  x64MmuBuildPgdEntry (addr_t  ulBaseAddr,
                                                         UINT8   ucRW,
                                                         UINT8   ucUS,
                                                         UINT8   ucPWT,
                                                         UINT8   ucPCD,
                                                         UINT8   ucA,
                                                         UINT8   ucXD)
{
    LW_PGD_TRANSENTRY  ulDescriptor;

    ulDescriptor = (ulBaseAddr & X64_MMU_MASK)
                 | (ucA   << X64_MMU_PRESENT_SHIFT)
                 | (ucRW  << X64_MMU_RW_SHIFT)
                 | (ucUS  << X64_MMU_US_SHIFT)
                 | (ucPWT << X64_MMU_PWT_SHIFT)
                 | (ucPCD << X64_MMU_PCD_SHIFT)
                 | ((UINT64)ucXD << X64_MMU_XD_SHIFT);

    return  (ulDescriptor);
}
/*********************************************************************************************************
** 函数名称: x64MmuBuildPmdEntry
** 功能描述: 生成一个二级描述符 (PMD 描述符)
** 输　入  : ulBaseAddr              基地址     (三级页表基地址)
**           ucRW                    是否可写
**           ucUS                    是否普通用户
**           ucPWT                   是否写穿透
**           ucPCD                   是否 CACHE 关闭
**           ucA                     是否能访问
**           ucXD                    是否禁止执行
** 输　出  : 二级描述符
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_INLINE LW_PMD_TRANSENTRY  x64MmuBuildPmdEntry (addr_t  ulBaseAddr,
                                                         UINT8   ucRW,
                                                         UINT8   ucUS,
                                                         UINT8   ucPWT,
                                                         UINT8   ucPCD,
                                                         UINT8   ucA,
                                                         UINT8   ucXD)
{
    LW_PMD_TRANSENTRY  ulDescriptor;

    ulDescriptor = (ulBaseAddr & X64_MMU_MASK)
                 | (ucA   << X64_MMU_PRESENT_SHIFT)
                 | (ucRW  << X64_MMU_RW_SHIFT)
                 | (ucUS  << X64_MMU_US_SHIFT)
                 | (ucPWT << X64_MMU_PWT_SHIFT)
                 | (ucPCD << X64_MMU_PCD_SHIFT)
                 | ((UINT64)ucXD << X64_MMU_XD_SHIFT);

    return  (ulDescriptor);
}
/*********************************************************************************************************
** 函数名称: x64MmuBuildPtsEntry
** 功能描述: 生成一个三级描述符 (PTS 描述符)
** 输　入  : ulBaseAddr              基地址     (四级页表基地址)
**           ucRW                    是否可写
**           ucUS                    是否普通用户
**           ucPWT                   是否写穿透
**           ucPCD                   是否 CACHE 关闭
**           ucA                     是否能访问
**           ucXD                    是否禁止执行
** 输　出  : 三级描述符
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_INLINE LW_PTS_TRANSENTRY  x64MmuBuildPtsEntry (addr_t  ulBaseAddr,
                                                         UINT8   ucRW,
                                                         UINT8   ucUS,
                                                         UINT8   ucPWT,
                                                         UINT8   ucPCD,
                                                         UINT8   ucA,
                                                         UINT8   ucXD)
{
    LW_PTS_TRANSENTRY  ulDescriptor;

    ulDescriptor = (ulBaseAddr & X64_MMU_MASK)
                 | (ucA   << X64_MMU_PRESENT_SHIFT)
                 | (ucRW  << X64_MMU_RW_SHIFT)
                 | (ucUS  << X64_MMU_US_SHIFT)
                 | (ucPWT << X64_MMU_PWT_SHIFT)
                 | (ucPCD << X64_MMU_PCD_SHIFT)
                 | ((UINT64)ucXD << X64_MMU_XD_SHIFT);

    return  (ulDescriptor);
}
/*********************************************************************************************************
** 函数名称: x64MmuBuildPteEntry
** 功能描述: 生成一个四级描述符 (PTE 描述符)
** 输　入  : ulBaseAddr              基地址     (页基地址)
**           ucRW                    是否可写
**           ucUS                    是否普通用户
**           ucPWT                   是否写穿透
**           ucPCD                   是否 CACHE 关闭
**           ucA                     是否能访问
**           ucXD                    是否禁止执行
** 输　出  : 四级描述符
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_INLINE LW_PTE_TRANSENTRY  x64MmuBuildPteEntry (addr_t  ulBaseAddr,
                                                         UINT8   ucRW,
                                                         UINT8   ucUS,
                                                         UINT8   ucPWT,
                                                         UINT8   ucPCD,
                                                         UINT8   ucA,
                                                         UINT8   ucXD)
{
    LW_PTE_TRANSENTRY  ulDescriptor;

    ulDescriptor = (ulBaseAddr & X64_MMU_MASK)
                 | (ucA   << X64_MMU_PRESENT_SHIFT)
                 | (ucRW  << X64_MMU_RW_SHIFT)
                 | (ucUS  << X64_MMU_US_SHIFT)
                 | (ucPWT << X64_MMU_PWT_SHIFT)
                 | (ucPCD << X64_MMU_PCD_SHIFT)
                 | ((UINT64)ucXD << X64_MMU_XD_SHIFT);

    return  (ulDescriptor);
}
/*********************************************************************************************************
** 函数名称: x64MmuMemInit
** 功能描述: 初始化 MMU 页表内存区
** 输　入  : pmmuctx        mmu 上下文
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  x64MmuMemInit (PLW_MMU_CONTEXT  pmmuctx)
{
#define PGD_BLOCK_SIZE  (4 * LW_CFG_KB_SIZE)
#define PMD_BLOCK_SIZE  (4 * LW_CFG_KB_SIZE)
#define PTS_BLOCK_SIZE  (4 * LW_CFG_KB_SIZE)
#define PTE_BLOCK_SIZE  (4 * LW_CFG_KB_SIZE)

    PVOID  pvPgdTable;
    PVOID  pvPmdTable;
    PVOID  pvPtsTable;
    PVOID  pvPteTable;
    
    ULONG  ulPgdNum = bspMmuPgdMaxNum();
    ULONG  ulPmdNum = bspMmuPmdMaxNum();
    ULONG  ulPtsNum = bspMmuPtsMaxNum();
    ULONG  ulPteNum = bspMmuPteMaxNum();
    
    pvPgdTable = __KHEAP_ALLOC_ALIGN((size_t)ulPgdNum * PGD_BLOCK_SIZE, PGD_BLOCK_SIZE);
    pvPmdTable = __KHEAP_ALLOC_ALIGN((size_t)ulPmdNum * PMD_BLOCK_SIZE, PMD_BLOCK_SIZE);
    pvPtsTable = __KHEAP_ALLOC_ALIGN((size_t)ulPtsNum * PTS_BLOCK_SIZE, PTS_BLOCK_SIZE);
    pvPteTable = __KHEAP_ALLOC_ALIGN((size_t)ulPteNum * PTE_BLOCK_SIZE, PTE_BLOCK_SIZE);
    
    if (!pvPgdTable || !pvPmdTable || !pvPtsTable || !pvPteTable) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not allocate page table.\r\n");
        return  (PX_ERROR);
    }
    
    _G_hPGDPartition = API_PartitionCreate("pgd_pool", pvPgdTable, ulPgdNum, PGD_BLOCK_SIZE,
                                           LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    _G_hPMDPartition = API_PartitionCreate("pmd_pool", pvPmdTable, ulPmdNum, PMD_BLOCK_SIZE,
                                           LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    _G_hPTSPartition = API_PartitionCreate("pts_pool", pvPtsTable, ulPtsNum, PTS_BLOCK_SIZE,
                                           LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    _G_hPTEPartition = API_PartitionCreate("pte_pool", pvPteTable, ulPteNum, PTE_BLOCK_SIZE,
                                           LW_OPTION_OBJECT_GLOBAL, LW_NULL);
                                           
    if (!_G_hPGDPartition || !_G_hPMDPartition || !_G_hPTSPartition || !_G_hPTEPartition) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not allocate page pool.\r\n");
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: x64MmuGlobalInit
** 功能描述: 调用 BSP 对 MMU 初始化
** 输　入  : pcMachineName     使用的机器名称
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  x64MmuGlobalInit (CPCHAR  pcMachineName)
{
    UINT64  uiMsr;

    archCacheReset(pcMachineName);

    x64MmuInvalidateTLB();
    
    x86PentiumMsrGet(X86_MSR_IA32_EFER, &uiMsr);
    uiMsr |= X86_IA32_EFER_NXE;
    x86PentiumMsrSet(X86_MSR_IA32_EFER, &uiMsr);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: x64MmuPgdOffset
** 功能描述: 通过虚拟地址计算 PGD 项
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址
** 输　出  : 对应的 PGD 表项地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  *x64MmuPgdOffset (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry = pmmuctx->MMUCTX_pgdEntry;
    REGISTER ULONG               ulPgdNum;

    if (ulAddr & (~((1ULL << X64_MMU_VIRT_ADDR_SIZE) - 1))) {           /*  不在合法的虚拟地址空间内    */
        return  (LW_NULL);
    }

    ulAddr    &= LW_CFG_VMM_PGD_MASK;
    ulPgdNum   = ulAddr >> LW_CFG_VMM_PGD_SHIFT;                        /*  计算 PGD 号                 */

    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry |
                 (ulPgdNum * sizeof(LW_PGD_TRANSENTRY)));               /*  获得一级页表描述符地址      */
               
    return  (p_pgdentry);
}
/*********************************************************************************************************
** 函数名称: x64MmuPmdOffset
** 功能描述: 通过虚拟地址计算 PMD 项
** 输　入  : p_pgdentry     pgd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 对应的 PMD 表项地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PMD_TRANSENTRY  *x64MmuPmdOffset (LW_PGD_TRANSENTRY  *p_pgdentry, addr_t  ulAddr)
{
    REGISTER LW_PMD_TRANSENTRY  *p_pmdentry;
    REGISTER LW_PGD_TRANSENTRY   ulTemp;
    REGISTER ULONG               ulPmdNum;

    ulTemp = (LW_PGD_TRANSENTRY)(*p_pgdentry);                          /*  获得一级页表描述符          */

    p_pmdentry = (LW_PMD_TRANSENTRY *)(ulTemp & X64_MMU_MASK);          /*  获得二级页表基地址          */

    ulAddr    &= LW_CFG_VMM_PMD_MASK;
    ulPmdNum   = ulAddr >> LW_CFG_VMM_PMD_SHIFT;                        /*  计算 PMD 号                 */

    p_pmdentry = (LW_PMD_TRANSENTRY *)((addr_t)p_pmdentry |
                 (ulPmdNum * sizeof(LW_PMD_TRANSENTRY)));               /*  获得二级页表描述符地址      */

    return  (p_pmdentry);
}
/*********************************************************************************************************
** 函数名称: x64MmuPtsOffset
** 功能描述: 通过虚拟地址计算 PTS 项
** 输　入  : p_pmdentry     pmd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 对应的 PTS 表项地址
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_PTS_TRANSENTRY  *x64MmuPtsOffset (LW_PMD_TRANSENTRY  *p_pmdentry, addr_t  ulAddr)
{
    REGISTER LW_PTS_TRANSENTRY  *p_ptsentry;
    REGISTER LW_PMD_TRANSENTRY   ulTemp;
    REGISTER ULONG               ulPtsNum;

    ulTemp = (LW_PMD_TRANSENTRY)(*p_pmdentry);                          /*  获得二级页表描述符          */

    p_ptsentry = (LW_PTS_TRANSENTRY *)(ulTemp & X64_MMU_MASK);          /*  获得三级页表基地址          */

    ulAddr    &= LW_CFG_VMM_PTS_MASK;
    ulPtsNum   = ulAddr >> LW_CFG_VMM_PTS_SHIFT;                        /*  计算 PTS 号                 */

    p_ptsentry = (LW_PTS_TRANSENTRY *)((addr_t)p_ptsentry |
                 (ulPtsNum * sizeof(LW_PTS_TRANSENTRY)));               /*  获得三级页表描述符地址      */

    return  (p_ptsentry);
}
/*********************************************************************************************************
** 函数名称: x64MmuPteOffset
** 功能描述: 通过虚拟地址计算 PTE 项
** 输　入  : p_ptsentry     pts 入口地址
**           ulAddr         虚拟地址
** 输　出  : 对应的 PTE 表项地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static  LW_PTE_TRANSENTRY  *x64MmuPteOffset (LW_PTS_TRANSENTRY  *p_ptsentry, addr_t  ulAddr)
{
    REGISTER LW_PTE_TRANSENTRY  *p_pteentry;
    REGISTER LW_PTS_TRANSENTRY   ulTemp;
    REGISTER ULONG               ulPageNum;

    ulTemp = (LW_PTS_TRANSENTRY)(*p_ptsentry);                          /*  获得三级页表描述符          */
    
    p_pteentry = (LW_PTE_TRANSENTRY *)(ulTemp & X64_MMU_MASK);          /*  获得四级页表基地址          */

    ulAddr    &= 0x1fful << LW_CFG_VMM_PAGE_SHIFT;                      /*  不要使用LW_CFG_VMM_PAGE_MASK*/
    ulPageNum  = ulAddr >> LW_CFG_VMM_PAGE_SHIFT;                       /*  计算段内页号                */

    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry |
                 (ulPageNum * sizeof(LW_PTE_TRANSENTRY)));              /*  获得虚拟地址页表描述符地址  */

    return  (p_pteentry);
}
/*********************************************************************************************************
** 函数名称: x64MmuPgdIsOk
** 功能描述: 判断 PGD 项的描述符是否正确
** 输　入  : pgdentry       PGD 项描述符
** 输　出  : 是否正确
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static BOOL  x64MmuPgdIsOk (LW_PGD_TRANSENTRY  pgdentry)
{
    return  ((pgdentry & (X64_MMU_PRESENT << X64_MMU_PRESENT_SHIFT)) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: x64MmuPmdIsOk
** 功能描述: 判断 PMD 项的描述符是否正确
** 输　入  : pmdentry       PMD 项描述符
** 输　出  : 是否正确
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static BOOL  x64MmuPmdIsOk (LW_PMD_TRANSENTRY  pmdentry)
{
    return  ((pmdentry & (X64_MMU_PRESENT << X64_MMU_PRESENT_SHIFT)) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: x64MmuPtsIsOk
** 功能描述: 判断 PTS 项的描述符是否正确
** 输　入  : ptsentry       PTS 项描述符
** 输　出  : 是否正确
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static BOOL  x64MmuPtsIsOk (LW_PTS_TRANSENTRY  ptsentry)
{
    return  ((ptsentry & (X64_MMU_PRESENT << X64_MMU_PRESENT_SHIFT)) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: x64MmuPteIsOk
** 功能描述: 判断 PTE 项的描述符是否正确
** 输　入  : pteentry       PTE 项描述符
** 输　出  : 是否正确
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static BOOL  x64MmuPteIsOk (LW_PTE_TRANSENTRY  pteentry)
{
    return  ((pteentry & (X64_MMU_PRESENT << X64_MMU_PRESENT_SHIFT)) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: x64MmuPgdAlloc
** 功能描述: 分配 PGD 项
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址 (参数 0 即偏移量为 0 , 需要返回页表基地址)
** 输　出  : 分配 PGD 地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY *x64MmuPgdAlloc (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry;
    REGISTER ULONG               ulPgdNum;
    
    if (ulAddr & (~((1ULL << X64_MMU_VIRT_ADDR_SIZE) - 1))) {           /*  不在合法的虚拟地址空间内    */
        return  (LW_NULL);
    }

    p_pgdentry = (LW_PGD_TRANSENTRY *)API_PartitionGet(_G_hPGDPartition);
    if (!p_pgdentry) {
        return  (LW_NULL);
    }
    
    lib_bzero(p_pgdentry, PGD_BLOCK_SIZE);

    ulAddr    &= LW_CFG_VMM_PGD_MASK;
    ulPgdNum   = ulAddr >> LW_CFG_VMM_PGD_SHIFT;                        /*  计算 PGD 号                 */

    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry |
                 (ulPgdNum * sizeof(LW_PGD_TRANSENTRY)));               /*  获得一级页表描述符地址      */
               
    return  (p_pgdentry);
}
/*********************************************************************************************************
** 函数名称: x64MmuPgdFree
** 功能描述: 释放 PGD 项
** 输　入  : p_pgdentry     pgd 入口地址
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  x64MmuPgdFree (LW_PGD_TRANSENTRY  *p_pgdentry)
{
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry & (~(PGD_BLOCK_SIZE - 1)));
    
    API_PartitionPut(_G_hPGDPartition, (PVOID)p_pgdentry);
}
/*********************************************************************************************************
** 函数名称: x64MmuPmdAlloc
** 功能描述: 分配 PMD 项
** 输　入  : pmmuctx        mmu 上下文
**           p_pgdentry     pgd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 分配 PMD 地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PMD_TRANSENTRY *x64MmuPmdAlloc (PLW_MMU_CONTEXT     pmmuctx,
                                          LW_PGD_TRANSENTRY  *p_pgdentry,
                                          addr_t              ulAddr)
{
#if LW_CFG_CACHE_EN > 0
    INTREG  iregInterLevel;
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

    LW_PMD_TRANSENTRY  *p_pmdentry = (LW_PMD_TRANSENTRY *)API_PartitionGet(_G_hPMDPartition);

    if (!p_pmdentry) {
        return  (LW_NULL);
    }

    lib_bzero(p_pmdentry, PMD_BLOCK_SIZE);

    *p_pgdentry = x64MmuBuildPgdEntry((addr_t)p_pmdentry,
                                      X64_MMU_RW,
                                      X64_MMU_US_NO,
                                      X64_MMU_PWT_NO,
                                      X64_MMU_PCD_NO,
                                      X64_MMU_A,
                                      X64_MMU_XD_NO);                   /*  设置一级页表描述符          */
#if LW_CFG_CACHE_EN > 0
    iregInterLevel = KN_INT_DISABLE();
    x86DCacheFlush((PVOID)p_pgdentry, sizeof(LW_PGD_TRANSENTRY));
    KN_INT_ENABLE(iregInterLevel);
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

    return  (x64MmuPmdOffset(p_pgdentry, ulAddr));
}
/*********************************************************************************************************
** 函数名称: x64MmuPmdFree
** 功能描述: 释放 PMD 项
** 输　入  : p_pmdentry     pmd 入口地址
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  x64MmuPmdFree (LW_PMD_TRANSENTRY  *p_pmdentry)
{
    p_pmdentry = (LW_PMD_TRANSENTRY *)((addr_t)p_pmdentry & (~(PMD_BLOCK_SIZE - 1)));

    API_PartitionPut(_G_hPMDPartition, (PVOID)p_pmdentry);
}
/*********************************************************************************************************
** 函数名称: x64MmuPtsAlloc
** 功能描述: 分配 PTS 项
** 输　入  : pmmuctx        mmu 上下文
**           p_pmdentry     pmd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 分配 PTS 地址
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_PTS_TRANSENTRY *x64MmuPtsAlloc (PLW_MMU_CONTEXT     pmmuctx,
                                          LW_PMD_TRANSENTRY  *p_pmdentry,
                                          addr_t              ulAddr)
{
#if LW_CFG_CACHE_EN > 0
    INTREG  iregInterLevel;
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

    LW_PTS_TRANSENTRY  *p_ptsentry = (LW_PTS_TRANSENTRY *)API_PartitionGet(_G_hPTSPartition);

    if (!p_ptsentry) {
        return  (LW_NULL);
    }

    lib_bzero(p_ptsentry, PTS_BLOCK_SIZE);

    *p_pmdentry = x64MmuBuildPmdEntry((addr_t)p_ptsentry,
                                      X64_MMU_RW,
                                      X64_MMU_US_NO,
                                      X64_MMU_PWT_NO,
                                      X64_MMU_PCD_NO,
                                      X64_MMU_A,
                                      X64_MMU_XD_NO);                   /*  设置二级页表描述符          */
#if LW_CFG_CACHE_EN > 0
    iregInterLevel = KN_INT_DISABLE();
    x86DCacheFlush((PVOID)p_pmdentry, sizeof(LW_PMD_TRANSENTRY));
    KN_INT_ENABLE(iregInterLevel);
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

    return  (x64MmuPtsOffset(p_pmdentry, ulAddr));
}
/*********************************************************************************************************
** 函数名称: x64MmuPtsFree
** 功能描述: 释放 PTS 项
** 输　入  : p_ptsentry     pts 入口地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  x64MmuPtsFree (LW_PTS_TRANSENTRY  *p_ptsentry)
{
    p_ptsentry = (LW_PTS_TRANSENTRY *)((addr_t)p_ptsentry & (~(PTS_BLOCK_SIZE - 1)));

    API_PartitionPut(_G_hPTSPartition, (PVOID)p_ptsentry);
}
/*********************************************************************************************************
** 函数名称: x64MmuPteAlloc
** 功能描述: 分配 PTE 项
** 输　入  : pmmuctx        mmu 上下文
**           p_pmdentry     pmd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 分配 PTE 地址
** 全局变量: 
** 调用模块: VMM 这里没有关闭中断, 回写 CACHE 时, 需要手动关中断, SylixOS 映射完毕会自动清快表, 所以
             这里不用清除快表.
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  *x64MmuPteAlloc (PLW_MMU_CONTEXT     pmmuctx,
                                           LW_PTS_TRANSENTRY  *p_ptsentry,
                                           addr_t              ulAddr)
{
#if LW_CFG_CACHE_EN > 0
    INTREG  iregInterLevel;
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

    LW_PTE_TRANSENTRY  *p_pteentry = (LW_PTE_TRANSENTRY *)API_PartitionGet(_G_hPTEPartition);
    
    if (!p_pteentry) {
        return  (LW_NULL);
    }
    
    lib_bzero(p_pteentry, PTE_BLOCK_SIZE);
    
    *p_ptsentry = x64MmuBuildPtsEntry((addr_t)p_pteentry,
                                      X64_MMU_RW,
                                      X64_MMU_US_NO,
                                      X64_MMU_PWT_NO,
                                      X64_MMU_PCD_NO,
                                      X64_MMU_A,
                                      X64_MMU_XD_NO);                   /*  设置三级页表描述符          */
#if LW_CFG_CACHE_EN > 0
    iregInterLevel = KN_INT_DISABLE();
    x86DCacheFlush((PVOID)p_ptsentry, sizeof(LW_PTS_TRANSENTRY));
    KN_INT_ENABLE(iregInterLevel);
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

    return  (x64MmuPteOffset(p_ptsentry, ulAddr));
}
/*********************************************************************************************************
** 函数名称: x64MmuPteFree
** 功能描述: 释放 PTE 项
** 输　入  : p_pteentry     pte 入口地址
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  x64MmuPteFree (LW_PTE_TRANSENTRY  *p_pteentry)
{
    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry & (~(PTE_BLOCK_SIZE - 1)));
    
    API_PartitionPut(_G_hPTEPartition, (PVOID)p_pteentry);
}
/*********************************************************************************************************
** 函数名称: x64MmuPtePhysGet
** 功能描述: 通过 PTE 表项, 查询物理地址
** 输　入  : pteentry           pte 表项
**           pulPhysicalAddr    获得的物理地址
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  x64MmuPtePhysGet (LW_PTE_TRANSENTRY  pteentry, addr_t  *pulPhysicalAddr)
{
    *pulPhysicalAddr = (addr_t)(pteentry & X64_MMU_MASK);               /*  获得物理地址                */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: x64MmuFlagGet
** 功能描述: 获得指定虚拟地址的 SylixOS 权限标志
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址
** 输　出  : SylixOS 权限标志
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ULONG  x64MmuFlagGet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    LW_PGD_TRANSENTRY  *p_pgdentry = x64MmuPgdOffset(pmmuctx, ulAddr);  /*  获得一级描述符地址          */

    if (p_pgdentry && x64MmuPgdIsOk(*p_pgdentry)) {                     /*  一级描述符有效              */
        LW_PMD_TRANSENTRY  *p_pmdentry = x64MmuPmdOffset(p_pgdentry,
                                                         ulAddr);       /*  获得二级描述符地址          */

        if (x64MmuPmdIsOk(*p_pmdentry)) {                               /*  二级描述符有效              */
            LW_PTS_TRANSENTRY  *p_ptsentry = x64MmuPtsOffset(p_pmdentry,
                                                             ulAddr);   /*  获得三级描述符地址          */

            if (x64MmuPtsIsOk(*p_ptsentry)) {                           /*  三级描述符有效              */
                LW_PTE_TRANSENTRY  *p_pteentry = x64MmuPteOffset(p_ptsentry,
                                                                ulAddr);/*  获得四级描述符地址         */
                LW_PTE_TRANSENTRY   pteentry = *p_pteentry;             /*  获得四级描述符              */

                if (x64MmuPteIsOk(pteentry)) {                          /*  四级描述符有效              */
                    UINT8   ucRW, ucUS, ucPWT, ucPCD, ucA, ucXD;
                    ULONG   ulFlag;

                    ucRW  = (UINT8)((pteentry >> X64_MMU_RW_SHIFT)      & 0x01);
                    ucUS  = (UINT8)((pteentry >> X64_MMU_US_SHIFT)      & 0x01);
                    ucPWT = (UINT8)((pteentry >> X64_MMU_PWT_SHIFT)     & 0x01);
                    ucPCD = (UINT8)((pteentry >> X64_MMU_PCD_SHIFT)     & 0x01);
                    ucA   = (UINT8)((pteentry >> X64_MMU_PRESENT_SHIFT) & 0x01);
                    ucXD  = (UINT8)((pteentry >> X64_MMU_XD_SHIFT)      & 0x01);

                    x64MmuAttr2Flags(ucRW, ucUS, ucPWT, ucPCD, ucA, ucXD, &ulFlag);

                    return  (ulFlag);
                }
            }
        }
    }

    return  (LW_VMM_FLAG_UNVALID);
}
/*********************************************************************************************************
** 函数名称: x64MmuFlagSet
** 功能描述: 设置指定虚拟地址的 flag 标志
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址
**           ulFlag         flag 标志
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
** 注  意  : 这里不需要清除快表 TLB, 因为 VMM 自身会作此操作.
*********************************************************************************************************/
static INT  x64MmuFlagSet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr, ULONG  ulFlag)
{
    UINT8   ucRW, ucUS, ucPWT, ucPCD, ucA, ucXD;

    if (x64MmuFlags2Attr(ulFlag, &ucRW,  &ucUS,
                         &ucPWT, &ucPCD, &ucA, &ucXD) != ERROR_NONE) {  /*  无效的映射关系              */
        return  (PX_ERROR);
    }

    LW_PGD_TRANSENTRY  *p_pgdentry = x64MmuPgdOffset(pmmuctx, ulAddr);  /*  获得一级描述符地址          */

    if (p_pgdentry && x64MmuPgdIsOk(*p_pgdentry)) {                     /*  一级描述符有效              */
        LW_PMD_TRANSENTRY  *p_pmdentry = x64MmuPmdOffset(p_pgdentry,
                                                         ulAddr);       /*  获得二级描述符地址          */

        if (x64MmuPmdIsOk(*p_pmdentry)) {                               /*  二级描述符有效              */
            LW_PTS_TRANSENTRY  *p_ptsentry = x64MmuPtsOffset(p_pmdentry,
                                                             ulAddr);   /*  获得三级描述符地址          */

            if (x64MmuPtsIsOk(*p_ptsentry)) {                           /*  三级描述符有效              */
                LW_PTE_TRANSENTRY  *p_pteentry = x64MmuPteOffset(p_ptsentry,
                                                                ulAddr);/*  获得四级描述符地址         */
                LW_PTE_TRANSENTRY   pteentry = *p_pteentry;             /*  获得四级描述符              */

                if (x64MmuPteIsOk(pteentry)) {                          /*  四级描述符有效              */
                    addr_t  ulPhysicalAddr = (addr_t)(pteentry & X64_MMU_MASK);

                    *p_pteentry = x64MmuBuildPteEntry(ulPhysicalAddr,
                                                      ucRW,  ucUS, ucPWT,
                                                      ucPCD, ucA,  ucXD);
#if LW_CFG_CACHE_EN > 0
                    x86DCacheFlush((PVOID)p_pteentry, sizeof(LW_PTE_TRANSENTRY));
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
                    x64MmuInvalidateTLBMVA((PVOID)ulAddr);

                    return  (ERROR_NONE);
                }
            }
        }
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: x64MmuMakeTrans
** 功能描述: 设置页面映射关系
** 输　入  : pmmuctx        mmu 上下文
**           p_pteentry     对应的页表项
**           ulVirtualAddr  虚拟地址
**           ulPhysicalAddr 物理地址
**           ulFlag         对应的类型
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
** 注  意  : 这里不需要清除快表 TLB, 因为 VMM 自身会作此操作.
*********************************************************************************************************/
static VOID  x64MmuMakeTrans (PLW_MMU_CONTEXT     pmmuctx,
                              LW_PTE_TRANSENTRY  *p_pteentry,
                              addr_t              ulVirtualAddr,
                              addr_t              ulPhysicalAddr,
                              addr_t              ulFlag)
{
    UINT8   ucRW, ucUS, ucPWT, ucPCD, ucA, ucXD;
    
    if (x64MmuFlags2Attr(ulFlag, &ucRW,  &ucUS,
                         &ucPWT, &ucPCD, &ucA, &ucXD) != ERROR_NONE) {  /*  无效的映射关系              */
        return;
    }

    *p_pteentry = x64MmuBuildPteEntry(ulPhysicalAddr,
                                      ucRW,  ucUS, ucPWT,
                                      ucPCD, ucA,  ucXD);
                                                        
#if LW_CFG_CACHE_EN > 0
    x86DCacheFlush((PVOID)p_pteentry, sizeof(LW_PTE_TRANSENTRY));
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
}
/*********************************************************************************************************
** 函数名称: x64MmuMakeCurCtx
** 功能描述: 设置 MMU 当前上下文
** 输　入  : pmmuctx        mmu 上下文
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  x64MmuMakeCurCtx (PLW_MMU_CONTEXT  pmmuctx)
{
    X86_CR_REG  ulCr3Val;

    ulCr3Val  = ((X86_CR_REG)pmmuctx->MMUCTX_pgdEntry) & X64_MMU_MASK;

    ulCr3Val |= X64_MMU_PWT_NO << X64_MMU_PWT_SHIFT;
    ulCr3Val |= X64_MMU_PCD_NO << X64_MMU_PCD_SHIFT;

    x86Cr3Set(ulCr3Val);
}
/*********************************************************************************************************
** 函数名称: x64MmuInvTLB
** 功能描述: 无效当前 CPU TLB
** 输　入  : pmmuctx        mmu 上下文
**           ulPageAddr     页面虚拟地址
**           ulPageNum      页面个数
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  x64MmuInvTLB (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulPageAddr, ULONG  ulPageNum)
{
    if (ulPageNum > (X64_MMU_TLB_NR >> 1)) {
        x64MmuInvalidateTLB();                                          /*  全部清除 TLB                */

    } else {
        ULONG  i;

        for (i = 0; i < ulPageNum; i++) {
            x64MmuInvalidateTLBMVA((PVOID)ulPageAddr);                  /*  逐个页面清除 TLB            */
            ulPageAddr += LW_CFG_VMM_PAGE_SIZE;
        }
    }
}
/*********************************************************************************************************
** 函数名称: x86MmuInit
** 功能描述: MMU 系统初始化
** 输　入  : pmmuop            MMU 操作函数集
**           pcMachineName     使用的机器名称
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  x86MmuInit (LW_MMU_OP  *pmmuop, CPCHAR  pcMachineName)
{
    pmmuop->MMUOP_ulOption = LW_VMM_MMU_FLUSH_TLB_MP;

    pmmuop->MMUOP_pfuncMemInit    = x64MmuMemInit;
    pmmuop->MMUOP_pfuncGlobalInit = x64MmuGlobalInit;
    
    pmmuop->MMUOP_pfuncPGDAlloc = x64MmuPgdAlloc;
    pmmuop->MMUOP_pfuncPGDFree  = x64MmuPgdFree;
    pmmuop->MMUOP_pfuncPMDAlloc = x64MmuPmdAlloc;
    pmmuop->MMUOP_pfuncPMDFree  = x64MmuPmdFree;
    pmmuop->MMUOP_pfuncPTSAlloc = x64MmuPtsAlloc;
    pmmuop->MMUOP_pfuncPTSFree  = x64MmuPtsFree;
    pmmuop->MMUOP_pfuncPTEAlloc = x64MmuPteAlloc;
    pmmuop->MMUOP_pfuncPTEFree  = x64MmuPteFree;
    
    pmmuop->MMUOP_pfuncPGDIsOk = x64MmuPgdIsOk;
    pmmuop->MMUOP_pfuncPMDIsOk = x64MmuPmdIsOk;
    pmmuop->MMUOP_pfuncPTSIsOk = x64MmuPtsIsOk;
    pmmuop->MMUOP_pfuncPTEIsOk = x64MmuPteIsOk;
    
    pmmuop->MMUOP_pfuncPGDOffset = x64MmuPgdOffset;
    pmmuop->MMUOP_pfuncPMDOffset = x64MmuPmdOffset;
    pmmuop->MMUOP_pfuncPTSOffset = x64MmuPtsOffset;
    pmmuop->MMUOP_pfuncPTEOffset = x64MmuPteOffset;
    
    pmmuop->MMUOP_pfuncPTEPhysGet = x64MmuPtePhysGet;
    
    pmmuop->MMUOP_pfuncFlagGet = x64MmuFlagGet;
    pmmuop->MMUOP_pfuncFlagSet = x64MmuFlagSet;
    
    pmmuop->MMUOP_pfuncMakeTrans     = x64MmuMakeTrans;
    pmmuop->MMUOP_pfuncMakeCurCtx    = x64MmuMakeCurCtx;
    pmmuop->MMUOP_pfuncInvalidateTLB = x64MmuInvTLB;
    pmmuop->MMUOP_pfuncSetEnable     = x64MmuEnable;
    pmmuop->MMUOP_pfuncSetDisable    = x64MmuDisable;
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
