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
** 文   件   名: x86Mmu.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 07 月 09 日
**
** 描        述: x86 体系构架 MMU 驱动.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "arch/x86/common/x86Cr.h"
/*********************************************************************************************************
  PDPTE PDE PTE 中的位定义
*********************************************************************************************************/
#define X86_MMU_PRESENT             (1)                                 /*  映射存在                    */
#define X86_MMU_PRESENT_SHIFT       (0)

#define X86_MMU_RW                  (1)                                 /*  可写                        */
#define X86_MMU_RW_NO               (0)
#define X86_MMU_RW_SHIFT            (1)

#define X86_MMU_US                  (1)                                 /*  用户模式可访问              */
#define X86_MMU_US_NO               (0)
#define X86_MMU_US_SHIFT            (2)

#define X86_MMU_PWT                 (1)                                 /*  写穿透                      */
#define X86_MMU_PWT_NO              (0)
#define X86_MMU_PWT_SHIFT           (3)

#define X86_MMU_PCD                 (1)                                 /*  不可 CACHE                  */
#define X86_MMU_PCD_NO              (0)
#define X86_MMU_PCD_SHIFT           (4)

#define X86_MMU_A                   (1)                                 /*  访问过                      */
#define X86_MMU_A_NO                (0)
#define X86_MMU_A_SHIFT             (5)

/*
 * 以下 PDE 有效
 */
#define X86_MMU_PS                  (1)                                 /*  If CR4.PSE = 1, must be 0   */
#define X86_MMU_PS_NO               (0)                                 /*  otherwise, this entry maps a*/
#define X86_MMU_PS_SHIFT            (7)                                 /*  4-MByte page                */

/*
 * 以下 PTE 有效
 */
#define X86_MMU_D                   (1)                                 /*  脏位                        */
#define X86_MMU_D_NO                (0)
#define X86_MMU_D_SHIFT             (6)

#define X86_MMU_PAT                 (1)                                 /*  If the PAT is supported,    */
#define X86_MMU_PAT_NO              (0)                                 /*  indirectly determines       */
#define X86_MMU_PAT_SHIFT           (7)                                 /*  the memory type             */

#define X86_MMU_G                   (1)                                 /*  全局                        */
#define X86_MMU_G_NO                (0)
#define X86_MMU_G_SHIFT             (8)
/*********************************************************************************************************
  PDPTE PDE PTE 中的掩码
*********************************************************************************************************/
#define X86_MMU_MASK                (0xfffff000)
/*********************************************************************************************************
  页面数大于其 1/2 时, 全清 TLB
*********************************************************************************************************/
#define X86_MMU_TLB_NR              (64)                                /*  Intel x86 vol 3 Table 11-1  */
/*********************************************************************************************************
  外部函数声明
*********************************************************************************************************/
extern VOID  x86MmuInvalidateTLB(VOID);
extern VOID  x86MmuInvalidateTLBMVA(PVOID  pvAddr);

extern VOID  x86MmuEnable(VOID);
extern VOID  x86MmuDisable(VOID);

extern VOID  x86DCacheFlush(PVOID  pvStart, size_t  stSize);
/*********************************************************************************************************
  全局变量定义
*********************************************************************************************************/
static LW_OBJECT_HANDLE     _G_hPGDPartition;                           /*  系统目前仅使用一个 PGD      */
static LW_OBJECT_HANDLE     _G_hPTEPartition;                           /*  PTE 缓冲区                  */
/*********************************************************************************************************
** 函数名称: x86MmuFlags2Attr
** 功能描述: 根据 SylixOS 权限标志, 生成 x86 MMU 权限标志
** 输　入  : ulFlag                  SylixOS 权限标志
**           pucRW                   是否可写
**           pucUS                   是否普通用户
**           pucPWT                  是否写穿透
**           pucPCD                  是否 CACHE 关闭
**           pucA                    是否能访问
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  x86MmuFlags2Attr (ULONG   ulFlag,
                              UINT8  *pucRW,
                              UINT8  *pucUS,
                              UINT8  *pucPWT,
                              UINT8  *pucPCD,
                              UINT8  *pucA)
{
    if (!(ulFlag & LW_VMM_FLAG_VALID)) {                                /*  无效的映射关系              */
        return  (PX_ERROR);
    }

    if (ulFlag & LW_VMM_FLAG_WRITABLE) {                                /*  是否可写                    */
        *pucRW = X86_MMU_RW;

    } else {
        *pucRW = X86_MMU_RW_NO;
    }

    *pucUS = X86_MMU_US_NO;                                             /*  始终 supervisor             */

    if ((ulFlag & LW_VMM_FLAG_CACHEABLE) &&
        (ulFlag & LW_VMM_FLAG_BUFFERABLE)) {                            /*  CACHE 与 BUFFER 控制        */
        *pucPCD = X86_MMU_PCD_NO;
        *pucPWT = X86_MMU_PWT_NO;

    } else if (ulFlag & LW_VMM_FLAG_CACHEABLE) {
        *pucPCD = X86_MMU_PCD_NO;
        *pucPWT = X86_MMU_PWT;

    } else if (ulFlag & LW_VMM_FLAG_BUFFERABLE) {
        *pucPCD = X86_MMU_PCD_NO;
        *pucPWT = X86_MMU_PWT;

    } else {
        *pucPCD = X86_MMU_PCD;
        *pucPWT = X86_MMU_PWT;
    }

    if (ulFlag & LW_VMM_FLAG_ACCESS) {                                  /*  是否可访问                  */
        *pucA = X86_MMU_A;

    } else {
        *pucA = X86_MMU_A_NO;
    }

    return (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: x86MmuAttr2Flags
** 功能描述: 根据 x86 MMU 权限标志, 生成 SylixOS 权限标志
** 输　入  : ucRW                    是否可写
**           ucUS                    是否普通用户
**           ucPWT                   是否写穿透
**           ucPCD                   是否 CACHE 关闭
**           ucA                     是否能访问
**           pulFlag                 SylixOS 权限标志
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  x86MmuAttr2Flags (UINT8   ucRW,
                              UINT8   ucUS,
                              UINT8   ucPWT,
                              UINT8   ucPCD,
                              UINT8   ucA,
                              ULONG  *pulFlag)
{
    *pulFlag = LW_VMM_FLAG_VALID;

    if (ucRW == X86_MMU_RW) {
        *pulFlag |= LW_VMM_FLAG_WRITABLE;

    } else {
        *pulFlag |= LW_VMM_FLAG_UNWRITABLE;
    }

    if (ucPCD == X86_MMU_PCD) {
        *pulFlag |= LW_VMM_FLAG_UNCACHEABLE;
        *pulFlag |= LW_VMM_FLAG_UNBUFFERABLE;

    } else {
        *pulFlag |= LW_VMM_FLAG_CACHEABLE;

        if (ucPWT == X86_MMU_PWT) {
            *pulFlag |= LW_VMM_FLAG_UNBUFFERABLE;
        } else {
            *pulFlag |= LW_VMM_FLAG_BUFFERABLE;
        }
    }

    if (ucA == X86_MMU_A) {
        *pulFlag |= LW_VMM_FLAG_ACCESS;

    } else {
        *pulFlag |= LW_VMM_FLAG_UNACCESS;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: x86MmuBuildPgdesc
** 功能描述: 生成一个一级描述符 (PGD 描述符)
** 输　入  : uiBaseAddr              基地址     (二级页表基地址)
**           ucRW                    是否可写
**           ucUS                    是否普通用户
**           ucPWT                   是否写穿透
**           ucPCD                   是否 CACHE 关闭
**           ucA                     是否能访问
** 输　出  : 一级描述符
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  x86MmuBuildPgdesc (UINT32  uiBaseAddr,
                                             UINT8   ucRW,
                                             UINT8   ucUS,
                                             UINT8   ucPWT,
                                             UINT8   ucPCD,
                                             UINT8   ucA)
{
    LW_PGD_TRANSENTRY  uiDescriptor;

    uiDescriptor = (uiBaseAddr & X86_MMU_MASK)
                 | (ucA   << X86_MMU_PRESENT_SHIFT)
                 | (ucRW  << X86_MMU_RW_SHIFT)
                 | (ucUS  << X86_MMU_US_SHIFT)
                 | (ucPWT << X86_MMU_PWT_SHIFT)
                 | (ucPCD << X86_MMU_PCD_SHIFT);

    return  (uiDescriptor);
}
/*********************************************************************************************************
** 函数名称: x86MmuBuildPtentry
** 功能描述: 生成一个二级描述符 (PTE 描述符)
** 输　入  : uiBaseAddr              基地址     (页地址)
**           ucRW                    是否可写
**           ucUS                    是否普通用户
**           ucPWT                   是否写穿透
**           ucPCD                   是否 CACHE 关闭
**           ucA                     是否能访问
** 输　出  : 二级描述符
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  x86MmuBuildPtentry (UINT32  uiBaseAddr,
                                              UINT8   ucRW,
                                              UINT8   ucUS,
                                              UINT8   ucPWT,
                                              UINT8   ucPCD,
                                              UINT8   ucA)
{
    LW_PTE_TRANSENTRY  uiDescriptor;
    
    uiDescriptor = (uiBaseAddr & X86_MMU_MASK)
                 | (ucA   << X86_MMU_PRESENT_SHIFT)
                 | (ucRW  << X86_MMU_RW_SHIFT)
                 | (ucUS  << X86_MMU_US_SHIFT)
                 | (ucPWT << X86_MMU_PWT_SHIFT)
                 | (ucPCD << X86_MMU_PCD_SHIFT);

    return  (uiDescriptor);
}
/*********************************************************************************************************
** 函数名称: x86MmuMemInit
** 功能描述: 初始化 MMU 页表内存区
** 输　入  : pmmuctx        mmu 上下文
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
** 注  意  : x86 体系结构要求: 一级页表基地址需要保持 4 KByte 对齐, 单条目映射 4 MByte 空间.
                               二级页表基地址需要保持 4 KByte 对齐, 单条目映射 4 KByte 空间.
*********************************************************************************************************/
static INT  x86MmuMemInit (PLW_MMU_CONTEXT  pmmuctx)
{
#define PGD_BLOCK_SIZE  (4 * LW_CFG_KB_SIZE)
#define PTE_BLOCK_SIZE  (4 * LW_CFG_KB_SIZE)

    PVOID  pvPgdTable;
    PVOID  pvPteTable;
    
    ULONG  ulPgdNum = bspMmuPgdMaxNum();
    ULONG  ulPteNum = bspMmuPteMaxNum();
    
    pvPgdTable = __KHEAP_ALLOC_ALIGN((size_t)ulPgdNum * PGD_BLOCK_SIZE, PGD_BLOCK_SIZE);
    pvPteTable = __KHEAP_ALLOC_ALIGN((size_t)ulPteNum * PTE_BLOCK_SIZE, PTE_BLOCK_SIZE);
    
    if (!pvPgdTable || !pvPteTable) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not allocate page table.\r\n");
        return  (PX_ERROR);
    }
    
    _G_hPGDPartition = API_PartitionCreate("pgd_pool", pvPgdTable, ulPgdNum, PGD_BLOCK_SIZE,
                                           LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    _G_hPTEPartition = API_PartitionCreate("pte_pool", pvPteTable, ulPteNum, PTE_BLOCK_SIZE,
                                           LW_OPTION_OBJECT_GLOBAL, LW_NULL);
                                           
    if (!_G_hPGDPartition || !_G_hPTEPartition) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not allocate page pool.\r\n");
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: x86MmuGlobalInit
** 功能描述: 调用 BSP 对 MMU 初始化
** 输　入  : pcMachineName     使用的机器名称
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  x86MmuGlobalInit (CPCHAR  pcMachineName)
{
    archCacheReset(pcMachineName);
    
    x86MmuInvalidateTLB();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: x86MmuPgdOffset
** 功能描述: 通过虚拟地址计算 PGD 项
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址
** 输　出  : 对应的 PGD 表项地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  *x86MmuPgdOffset (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry = pmmuctx->MMUCTX_pgdEntry;
    REGISTER UINT32              uiPgdNum;
    
    uiPgdNum   = ulAddr >> LW_CFG_VMM_PGD_SHIFT;                        /*  计算 PGD 号                 */

    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry |
                 (uiPgdNum * sizeof(LW_PGD_TRANSENTRY)));               /*  获得一级页表描述符地址      */
               
    return  (p_pgdentry);
}
/*********************************************************************************************************
** 函数名称: x86MmuPmdOffset
** 功能描述: 通过虚拟地址计算 PMD 项
** 输　入  : p_pgdentry     pgd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 对应的 PMD 表项地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PMD_TRANSENTRY  *x86MmuPmdOffset (LW_PGD_TRANSENTRY  *p_pgdentry, addr_t  ulAddr)
{
    return  ((LW_PMD_TRANSENTRY *)p_pgdentry);                          /*  x86 无 PMD 项               */
}
/*********************************************************************************************************
** 函数名称: x86MmuPteOffset
** 功能描述: 通过虚拟地址计算 PMD 项
** 输　入  : p_pgdentry     pgd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 对应的 PMD 表项地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static  LW_PTE_TRANSENTRY  *x86MmuPteOffset (LW_PMD_TRANSENTRY  *p_pmdentry, addr_t  ulAddr)
{
    REGISTER LW_PTE_TRANSENTRY  *p_pteentry;
    REGISTER LW_PMD_TRANSENTRY   uiTemp;
    REGISTER UINT32              uiPageNum;

    uiTemp = (LW_PMD_TRANSENTRY)(*p_pmdentry);                          /*  获得一级页表描述符          */
    
    p_pteentry = (LW_PTE_TRANSENTRY *)(uiTemp & X86_MMU_MASK);          /*  获得二级页表基地址          */

    ulAddr    &= ~LW_CFG_VMM_PGD_MASK;
    uiPageNum  = ulAddr >> LW_CFG_VMM_PAGE_SHIFT;                       /*  计算段内页号                */

    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry |
                 (uiPageNum * sizeof(LW_PTE_TRANSENTRY)));              /*  获得虚拟地址页表描述符地址  */

    return  (p_pteentry);
}
/*********************************************************************************************************
** 函数名称: x86MmuPgdIsOk
** 功能描述: 判断 PGD 项的描述符是否正确
** 输　入  : pgdentry       PGD 项描述符
** 输　出  : 是否正确
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static BOOL  x86MmuPgdIsOk (LW_PGD_TRANSENTRY  pgdentry)
{
    return  ((pgdentry & (X86_MMU_PRESENT << X86_MMU_PRESENT_SHIFT)) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: x86MmuPteIsOk
** 功能描述: 判断 PTE 项的描述符是否正确
** 输　入  : pteentry       PTE 项描述符
** 输　出  : 是否正确
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static BOOL  x86MmuPteIsOk (LW_PTE_TRANSENTRY  pteentry)
{
    return  ((pteentry & (X86_MMU_PRESENT << X86_MMU_PRESENT_SHIFT)) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: x86MmuPgdAlloc
** 功能描述: 分配 PGD 项
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址 (参数 0 即偏移量为 0 , 需要返回页表基地址)
** 输　出  : 分配 PGD 地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY *x86MmuPgdAlloc (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry = (LW_PGD_TRANSENTRY *)API_PartitionGet(_G_hPGDPartition);
    REGISTER UINT32              uiPgdNum;
    
    if (!p_pgdentry) {
        return  (LW_NULL);
    }
    
    lib_bzero(p_pgdentry, PGD_BLOCK_SIZE);                              /*  无效一级页表项              */

    uiPgdNum   = ulAddr >> LW_CFG_VMM_PGD_SHIFT;                        /*  计算 PGD 号                 */

    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry |
                 (uiPgdNum * sizeof(LW_PGD_TRANSENTRY)));               /*  获得一级页表描述符地址      */
               
    return  (p_pgdentry);
}
/*********************************************************************************************************
** 函数名称: x86MmuPgdFree
** 功能描述: 释放 PGD 项
** 输　入  : p_pgdentry     pgd 入口地址
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  x86MmuPgdFree (LW_PGD_TRANSENTRY  *p_pgdentry)
{
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry & (~(PGD_BLOCK_SIZE - 1)));
    
    API_PartitionPut(_G_hPGDPartition, (PVOID)p_pgdentry);
}
/*********************************************************************************************************
** 函数名称: x86MmuPmdAlloc
** 功能描述: 分配 PMD 项
** 输　入  : pmmuctx        mmu 上下文
**           p_pgdentry     pgd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 分配 PMD 地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PMD_TRANSENTRY *x86MmuPmdAlloc (PLW_MMU_CONTEXT     pmmuctx,
                                          LW_PGD_TRANSENTRY  *p_pgdentry,
                                          addr_t              ulAddr)
{
    return  ((LW_PMD_TRANSENTRY *)p_pgdentry);
}
/*********************************************************************************************************
** 函数名称: x86MmuPmdFree
** 功能描述: 释放 PMD 项
** 输　入  : p_pmdentry     pmd 入口地址
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  x86MmuPmdFree (LW_PMD_TRANSENTRY  *p_pmdentry)
{
    (VOID)p_pmdentry;
}
/*********************************************************************************************************
** 函数名称: x86MmuPteAlloc
** 功能描述: 分配 PTE 项
** 输　入  : pmmuctx        mmu 上下文
**           p_pmdentry     pmd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 分配 PTE 地址
** 全局变量: 
** 调用模块: VMM 这里没有关闭中断, 回写 CACHE 时, 需要手动关中断, SylixOS 映射完毕会自动清快表, 所以
             这里不用清除快表.
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  *x86MmuPteAlloc (PLW_MMU_CONTEXT     pmmuctx,
                                           LW_PMD_TRANSENTRY  *p_pmdentry,
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
    
    *p_pmdentry = (LW_PMD_TRANSENTRY)x86MmuBuildPgdesc((UINT32)p_pteentry,
                                                       X86_MMU_RW,
                                                       X86_MMU_US_NO,
                                                       X86_MMU_PWT_NO,
                                                       X86_MMU_PCD_NO,
                                                       X86_MMU_A);      /*  设置一级页表描述符          */
#if LW_CFG_CACHE_EN > 0
    iregInterLevel = KN_INT_DISABLE();
    x86DCacheFlush((PVOID)p_pmdentry, sizeof(LW_PMD_TRANSENTRY));
    KN_INT_ENABLE(iregInterLevel);
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

    return  (x86MmuPteOffset(p_pmdentry, ulAddr));
}
/*********************************************************************************************************
** 函数名称: x86MmuPteFree
** 功能描述: 释放 PTE 项
** 输　入  : p_pteentry     pte 入口地址
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  x86MmuPteFree (LW_PTE_TRANSENTRY  *p_pteentry)
{
    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry & (~(PTE_BLOCK_SIZE - 1)));
    
    API_PartitionPut(_G_hPTEPartition, (PVOID)p_pteentry);
}
/*********************************************************************************************************
** 函数名称: x86MmuPtePhysGet
** 功能描述: 通过 PTE 表项, 查询物理地址
** 输　入  : pteentry           pte 表项
**           pulPhysicalAddr    获得的物理地址
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  x86MmuPtePhysGet (LW_PTE_TRANSENTRY  pteentry, addr_t  *pulPhysicalAddr)
{
    *pulPhysicalAddr = (addr_t)(pteentry & X86_MMU_MASK);               /*  获得物理地址                */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: x86MmuFlagGet
** 功能描述: 获得指定虚拟地址的 SylixOS 权限标志
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址
** 输　出  : SylixOS 权限标志
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ULONG  x86MmuFlagGet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    LW_PGD_TRANSENTRY  *p_pgdentry = x86MmuPgdOffset(pmmuctx, ulAddr);  /*  获得一级描述符地址          */
    UINT8               ucRW, ucUS, ucPWT, ucPCD, ucA;
    ULONG               ulFlag;

    if (x86MmuPgdIsOk(*p_pgdentry)) {                                   /*  一级描述符有效              */
        LW_PTE_TRANSENTRY  *p_pteentry = x86MmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                                         ulAddr);       /*  获得二级描述符地址          */

        LW_PTE_TRANSENTRY   uiDescriptor = *p_pteentry;                 /*  获得二级描述符              */

        if (x86MmuPteIsOk(uiDescriptor)) {                              /*  二级描述符有效              */
            ucRW  = (UINT8)((uiDescriptor >> X86_MMU_RW_SHIFT)      & 0x01);
            ucUS  = (UINT8)((uiDescriptor >> X86_MMU_US_SHIFT)      & 0x01);
            ucPWT = (UINT8)((uiDescriptor >> X86_MMU_PWT_SHIFT)     & 0x01);
            ucPCD = (UINT8)((uiDescriptor >> X86_MMU_PCD_SHIFT)     & 0x01);
            ucA   = (UINT8)((uiDescriptor >> X86_MMU_PRESENT_SHIFT) & 0x01);

            x86MmuAttr2Flags(ucRW, ucUS, ucPWT, ucPCD, ucA, &ulFlag);

            return  (ulFlag);
        }
    }

    return  (LW_VMM_FLAG_UNVALID);
}
/*********************************************************************************************************
** 函数名称: x86MmuFlagSet
** 功能描述: 设置指定虚拟地址的 flag 标志
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址
**           ulFlag         flag 标志
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
** 注  意  : 这里不需要清除快表 TLB, 因为 VMM 自身会作此操作.
*********************************************************************************************************/
static INT  x86MmuFlagSet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr, ULONG  ulFlag)
{
    LW_PGD_TRANSENTRY  *p_pgdentry = x86MmuPgdOffset(pmmuctx, ulAddr);  /*  获得一级描述符地址          */
    UINT8               ucRW, ucUS, ucPWT, ucPCD, ucA;

    if (x86MmuFlags2Attr(ulFlag, &ucRW,  &ucUS,
                         &ucPWT, &ucPCD, &ucA) != ERROR_NONE) {         /*  无效的映射关系              */
        return  (PX_ERROR);
    }

    if (x86MmuPgdIsOk(*p_pgdentry)) {                                   /*  一级描述符有效              */
        LW_PTE_TRANSENTRY  *p_pteentry = x86MmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                                         ulAddr);       /*  获得二级描述符地址          */

        LW_PTE_TRANSENTRY   uiDescriptor = *p_pteentry;                 /*  获得二级描述符              */

        if (x86MmuPteIsOk(uiDescriptor)) {                              /*  二级描述符有效              */
            addr_t  ulPhysicalAddr = (addr_t)(*p_pteentry & X86_MMU_MASK);

            *p_pteentry = (LW_PTE_TRANSENTRY)x86MmuBuildPtentry((UINT32)ulPhysicalAddr,
                                                                ucRW, ucUS, ucPWT, ucPCD, ucA);
#if LW_CFG_CACHE_EN > 0
            x86DCacheFlush((PVOID)p_pteentry, sizeof(LW_PTE_TRANSENTRY));
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
            x86MmuInvalidateTLBMVA((PVOID)ulAddr);

            return  (ERROR_NONE);
        }
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: x86MmuMakeTrans
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
static VOID  x86MmuMakeTrans (PLW_MMU_CONTEXT     pmmuctx,
                              LW_PTE_TRANSENTRY  *p_pteentry,
                              addr_t              ulVirtualAddr,
                              addr_t              ulPhysicalAddr,
                              addr_t              ulFlag)
{
    UINT8  ucRW, ucUS, ucPWT, ucPCD, ucA;
    
    if (x86MmuFlags2Attr(ulFlag, &ucRW,  &ucUS,
                         &ucPWT, &ucPCD, &ucA) != ERROR_NONE) {         /*  无效的映射关系              */
        return;
    }

    *p_pteentry = (LW_PTE_TRANSENTRY)x86MmuBuildPtentry((UINT32)ulPhysicalAddr,
                                                        ucRW, ucUS, ucPWT, ucPCD, ucA);
                                                        
#if LW_CFG_CACHE_EN > 0
    x86DCacheFlush((PVOID)p_pteentry, sizeof(LW_PTE_TRANSENTRY));
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
}
/*********************************************************************************************************
** 函数名称: x86MmuMakeCurCtx
** 功能描述: 设置 MMU 当前上下文
** 输　入  : pmmuctx        mmu 上下文
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  x86MmuMakeCurCtx (PLW_MMU_CONTEXT  pmmuctx)
{
    UINT32  uiCr3Val;

    uiCr3Val  = ((UINT32)pmmuctx->MMUCTX_pgdEntry) & X86_MMU_MASK;

    uiCr3Val |= X86_MMU_PWT_NO << X86_MMU_PWT_SHIFT;
    uiCr3Val |= X86_MMU_PCD_NO << X86_MMU_PCD_SHIFT;

    x86Cr3Set(uiCr3Val);
}
/*********************************************************************************************************
** 函数名称: x86MmuInvTLB
** 功能描述: 无效当前 CPU TLB
** 输　入  : pmmuctx        mmu 上下文
**           ulPageAddr     页面虚拟地址
**           ulPageNum      页面个数
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  x86MmuInvTLB (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulPageAddr, ULONG  ulPageNum)
{
    if (ulPageNum > (X86_MMU_TLB_NR >> 1)) {
        x86MmuInvalidateTLB();                                          /*  全部清除 TLB                */

    } else {
        ULONG  i;

        for (i = 0; i < ulPageNum; i++) {
            x86MmuInvalidateTLBMVA((PVOID)ulPageAddr);                  /*  逐个页面清除 TLB            */
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

    pmmuop->MMUOP_pfuncMemInit    = x86MmuMemInit;
    pmmuop->MMUOP_pfuncGlobalInit = x86MmuGlobalInit;
    
    pmmuop->MMUOP_pfuncPGDAlloc = x86MmuPgdAlloc;
    pmmuop->MMUOP_pfuncPGDFree  = x86MmuPgdFree;
    pmmuop->MMUOP_pfuncPMDAlloc = x86MmuPmdAlloc;
    pmmuop->MMUOP_pfuncPMDFree  = x86MmuPmdFree;
    pmmuop->MMUOP_pfuncPTEAlloc = x86MmuPteAlloc;
    pmmuop->MMUOP_pfuncPTEFree  = x86MmuPteFree;
    
    pmmuop->MMUOP_pfuncPGDIsOk = x86MmuPgdIsOk;
    pmmuop->MMUOP_pfuncPMDIsOk = x86MmuPgdIsOk;
    pmmuop->MMUOP_pfuncPTEIsOk = x86MmuPteIsOk;
    
    pmmuop->MMUOP_pfuncPGDOffset = x86MmuPgdOffset;
    pmmuop->MMUOP_pfuncPMDOffset = x86MmuPmdOffset;
    pmmuop->MMUOP_pfuncPTEOffset = x86MmuPteOffset;
    
    pmmuop->MMUOP_pfuncPTEPhysGet = x86MmuPtePhysGet;
    
    pmmuop->MMUOP_pfuncFlagGet = x86MmuFlagGet;
    pmmuop->MMUOP_pfuncFlagSet = x86MmuFlagSet;
    
    pmmuop->MMUOP_pfuncMakeTrans     = x86MmuMakeTrans;
    pmmuop->MMUOP_pfuncMakeCurCtx    = x86MmuMakeCurCtx;
    pmmuop->MMUOP_pfuncInvalidateTLB = x86MmuInvTLB;
    pmmuop->MMUOP_pfuncSetEnable     = x86MmuEnable;
    pmmuop->MMUOP_pfuncSetDisable    = x86MmuDisable;
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
