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
** 文   件   名: mips32Mmu.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2015 年 10 月 12 日
**
** 描        述: MIPS32 体系构架 MMU 驱动.
**
** BUG:
2016.04.06  修改 TLB 无效对 EntryHi Register 操作(JZ4780 支持)
2016.06.14  为支持非 4K 大小页面重构代码
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "arch/mips/common/cp0/mipsCp0.h"
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static LW_OBJECT_HANDLE     _G_hPGDPartition = LW_HANDLE_INVALID;       /*  系统目前仅使用一个 PGD      */
static PVOID                _G_pvPTETable    = LW_NULL;                 /*  PTE 表                      */
static UINT32               _G_uiTlbSize     = 0;                       /*  TLB 数组大小                */
static INT                  _G_iMachineType  = MIPS_MACHINE_TYPE_24KF;  /*  机器类型                    */
/*********************************************************************************************************
  龙芯 Loongson2x 3x 系列处理器特有的 TLB(VTLB 或 JTLB 一定会有, 就不作判断了)
*********************************************************************************************************/
static BOOL                 _G_bLsHasITLB = LW_FALSE;                   /*  是否有 ITLB                 */
static BOOL                 _G_bLsHasDTLB = LW_FALSE;                   /*  是否有 DTLB                 */
static BOOL                 _G_bLsHasFTLB = LW_FALSE;                   /*  是否有 FTLB                 */
static BOOL                 _G_bLsHasNE   = LW_FALSE;                   /*  是否有 NE 位                */
/*********************************************************************************************************
  宏定义
*********************************************************************************************************/
#define MIPS32_TLB_SIZE     _G_uiTlbSize                                /*  TLB 数组大小                */
/*********************************************************************************************************
  ENTRYLO
*********************************************************************************************************/
#define MIPS32_ENTRYLO_G_BIT            (1 << 0)
#define MIPS32_ENTRYLO_V_BIT            (1 << 1)
#define MIPS32_ENTRYLO_D_BIT            (1 << 2)

#define MIPS32_ENTRYLO_C_SHIFT          (3)
#define MIPS32_ENTRYLO_C_MASK           (0x07 << MIPS32_ENTRYLO_C_SHIFT)

#define MIPS32_ENTRYLO_PFN_SHIFT        (6)
#define MIPS32_ENTRYLO_PFN_MASK         (0xffffff << MIPS32_ENTRYLO_PFN_SHIFT)

#define MIPS32_ENTRYLO_NE_BIT           (1 << 30)
/*********************************************************************************************************
  ENTRYHI
*********************************************************************************************************/
#define MIPS32_UNIQUE_ENTRYHI(idx)      (A_K1BASE + ((idx) << (LW_CFG_VMM_PAGE_SHIFT + 1)))
/*********************************************************************************************************
  PAGE_MASK
*********************************************************************************************************/
#define MIPS32_PAGE_MASK_4K             0x00000000
#define MIPS32_PAGE_MASK_16K            0x00006000
#define MIPS32_PAGE_MASK_64K            0x0001e000

#if   LW_CFG_VMM_PAGE_SIZE == (4  * LW_CFG_KB_SIZE)
#define MIPS32_PAGE_MASK                MIPS32_PAGE_MASK_4K
#elif LW_CFG_VMM_PAGE_SIZE == (16 * LW_CFG_KB_SIZE)
#define MIPS32_PAGE_MASK                MIPS32_PAGE_MASK_16K
#elif LW_CFG_VMM_PAGE_SIZE == (64 * LW_CFG_KB_SIZE)
#define MIPS32_PAGE_MASK                MIPS32_PAGE_MASK_64K
#else
#error  LW_CFG_VMM_PAGE_SIZE must be (4K, 16K, 64K)!
#endif                                                                  /*  LW_CFG_VMM_PAGE_SIZE        */
/*********************************************************************************************************
  TLB 操作
*********************************************************************************************************/
#define MIPS32_MMU_TLB_WRITE()          do { MIPS_EXEC_INS("TLBWI"); MIPS_EXEC_INS("EHB"); } while(0)
#define MIPS32_MMU_TLB_READ()           do { MIPS_EXEC_INS("TLBR");  MIPS_EXEC_INS("EHB"); } while(0)
#define MIPS32_MMU_TLB_PROBE()          do { MIPS_EXEC_INS("TLBP");  MIPS_EXEC_INS("EHB"); } while(0)
/*********************************************************************************************************
** 函数名称: mips32MmuInvalidateMicroTLB
** 功能描述: 无效 Micro TLB
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  mips32MmuInvalidateMicroTLB (VOID)
{
    if (_G_bLsHasITLB && _G_bLsHasDTLB) {
        mipsCp0DiagWrite((1 << 3) |
                         (1 << 2));                                     /*  清除 ITLB DTLB              */

    } else if (_G_bLsHasITLB) {
        mipsCp0DiagWrite(1 << 2);                                       /*  清除 ITLB                   */
    }
}
/*********************************************************************************************************
** 函数名称: mips32MmuEnable
** 功能描述: 使能 MMU
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  mips32MmuEnable (VOID)
{
}
/*********************************************************************************************************
** 函数名称: mips32MmuDisable
** 功能描述: 禁能 MMU
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  mips32MmuDisable (VOID)
{
}
/*********************************************************************************************************
** 函数名称: mips32MmuInvalidateTLB
** 功能描述: 无效 TLB
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 内部会无效 Micro TLB
*********************************************************************************************************/
static VOID  mips32MmuInvalidateTLB (VOID)
{
    if (_G_bLsHasITLB && _G_bLsHasDTLB && _G_bLsHasFTLB) {
        mipsCp0DiagWrite((1 << 13) |
                         (1 << 12) |
                         (1 <<  3) |
                         (1 <<  2));                                    /*  清除 ITLB DTLB VTLB FTLB    */

    } else {
        UINT32  uiEntryHiBak = mipsCp0EntryHiRead();
        INT     i;

        for (i = 0; i < MIPS32_TLB_SIZE; i++) {
            mipsCp0IndexWrite(i);

            mipsCp0EntryLo0Write(0);
            mipsCp0EntryLo1Write(0);

            mipsCp0EntryHiWrite(MIPS32_UNIQUE_ENTRYHI(i));

            MIPS32_MMU_TLB_WRITE();
        }

        mipsCp0EntryHiWrite(uiEntryHiBak);

        mips32MmuInvalidateMicroTLB();                                  /*  无效 Micro TLB              */
    }
}
/*********************************************************************************************************
** 函数名称: mips32MmuDumpTLB
** 功能描述: Dump TLB
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  mips32MmuDumpTLB (VOID)
{
             UINT32  uiEntryHiBak = mipsCp0EntryHiRead();
             UINT32  uiEntryLo0;
             UINT32  uiEntryLo1;
             UINT32  uiEntryHi;
             UINT32  uiPageMask;
    REGISTER INT     i;

    for (i = 0; i < MIPS32_TLB_SIZE; i++) {
        mipsCp0IndexWrite(i);

        MIPS32_MMU_TLB_READ();

        uiEntryLo0 = mipsCp0EntryLo0Read();
        uiEntryLo1 = mipsCp0EntryLo1Read();
        uiEntryHi  = mipsCp0EntryHiRead();
        uiPageMask = mipsCp0PageMaskRead();

        _PrintFormat("TLB[%02d]: EntryLo0=0x%08x, EntryLo1=0x%08x, "
                     "EntryHi=0x%08x, PageMask=0x%08x\r\n",
                     i, uiEntryLo0, uiEntryLo1, uiEntryHi, uiPageMask);
    }

    mipsCp0EntryHiWrite(uiEntryHiBak);
}
/*********************************************************************************************************
** 函数名称: mips32MmuInvalidateTLBMVA
** 功能描述: 无效指定 MVA 的 TLB
** 输　入  : ulAddr            指定 MVA
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 内部不会无效 Micro TLB, 外部操作完成后必须无效 Micro TLB
*********************************************************************************************************/
static VOID  mips32MmuInvalidateTLBMVA (addr_t  ulAddr)
{
             UINT32  uiEntryHiBak = mipsCp0EntryHiRead();
    REGISTER UINT32  uiEntryHi    = ulAddr & (LW_CFG_VMM_PAGE_MASK << 1);
    REGISTER INT32   iIndex;
    REGISTER INT     iReTry;

    for (iReTry = 0; iReTry < 2; iReTry++) {                            /*  不会出现两条一样的 TLB 条目 */
        mipsCp0EntryHiWrite(uiEntryHi);

        MIPS32_MMU_TLB_PROBE();

        iIndex = mipsCp0IndexRead();
        if (iIndex >= 0) {
            mipsCp0IndexWrite(iIndex);

            mipsCp0EntryLo0Write(0);
            mipsCp0EntryLo1Write(0);

            mipsCp0EntryHiWrite(MIPS32_UNIQUE_ENTRYHI(iIndex));

            MIPS32_MMU_TLB_WRITE();
        } else {
            break;
        }
    }

    mipsCp0EntryHiWrite(uiEntryHiBak);
}
/*********************************************************************************************************
** 函数名称: mips32MmuBuildPgdesc
** 功能描述: 生成一个一级描述符 (PGD 描述符)
** 输　入  : uiBaseAddr              二级页表基地址
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  mips32MmuBuildPgdesc (UINT32  uiBaseAddr)
{
    return  (uiBaseAddr);                                               /*  一级描述符就是二级页表基地址*/
}
/*********************************************************************************************************
** 函数名称: mips32MmuBuildPtentry
** 功能描述: 生成一个二级描述符 (PTE 描述符)
** 输　入  : uiBaseAddr              物理页地址
**           uiFlag
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  mips32MmuBuildPtentry (UINT32  uiBaseAddr,
                                                 ULONG   ulFlag)
{
    LW_PTE_TRANSENTRY   uiDescriptor;
    UINT32              uiPFN;

    if (ulFlag & LW_VMM_FLAG_ACCESS) {
        uiPFN = uiBaseAddr >> 12;                                       /*  计算 PFN                    */

        uiDescriptor = uiPFN << MIPS32_ENTRYLO_PFN_SHIFT;               /*  填充 PFN                    */

        if (ulFlag & LW_VMM_FLAG_VALID) {
            uiDescriptor |= MIPS32_ENTRYLO_V_BIT;                       /*  填充 V 位                   */
        }

        if (ulFlag & LW_VMM_FLAG_WRITABLE) {
            uiDescriptor |= MIPS32_ENTRYLO_D_BIT;                       /*  填充 D 位                   */
        }

        uiDescriptor |= MIPS32_ENTRYLO_G_BIT;                           /*  填充 G 位                   */

        if (ulFlag & LW_VMM_FLAG_CACHEABLE) {                           /*  填充 C 位                   */
                                                                        /*  龙芯 3A 上为一致性 CACHE    */
            uiDescriptor |= MIPS_CACHABLE_NONCOHERENT << MIPS32_ENTRYLO_C_SHIFT;
        } else {
            uiDescriptor |= MIPS_UNCACHED << MIPS32_ENTRYLO_C_SHIFT;
        }

        if (_G_bLsHasNE) {
            if (!(ulFlag & LW_VMM_FLAG_EXECABLE)) {                     /*  不可执行                    */
                uiDescriptor |= MIPS32_ENTRYLO_NE_BIT;                  /*  填充 NE 位                  */
            }
        }
    } else {
        uiDescriptor = 0;
    }

    return  (uiDescriptor);
}
/*********************************************************************************************************
** 函数名称: mips32MmuMemInit
** 功能描述: 初始化 MMU 页表内存区
** 输　入  : pmmuctx        mmu 上下文
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  mips32MmuMemInit (PLW_MMU_CONTEXT  pmmuctx)
{
#define PGD_BLOCK_SIZE  (4096 * sizeof(LW_PGD_TRANSENTRY))
#define PTE_BLOCK_SIZE  ((LW_CFG_MB_SIZE / LW_CFG_VMM_PAGE_SIZE) * sizeof(LW_PTE_TRANSENTRY))
#define PTE_TABLE_SIZE  ((LW_CFG_GB_SIZE / LW_CFG_VMM_PAGE_SIZE) * 4 * sizeof(LW_PTE_TRANSENTRY))

    PVOID   pvPgdTable;
    PVOID   pvPteTable;
    
    pvPgdTable = __KHEAP_ALLOC_ALIGN(PGD_BLOCK_SIZE, PGD_BLOCK_SIZE);
    if (!pvPgdTable) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not allocate page table.\r\n");
        return  (PX_ERROR);
    }

    /*
     * PTE 表需要 8MByte 对齐才能写入 Context 寄存器
     */
    pvPteTable = __KHEAP_ALLOC_ALIGN(PTE_TABLE_SIZE, 8 * LW_CFG_MB_SIZE);
    if (!pvPteTable) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not allocate page table.\r\n");
        __KHEAP_FREE(pvPgdTable);
        return  (PX_ERROR);
    }

    _G_hPGDPartition = API_PartitionCreate("pgd_pool", pvPgdTable, 1, PGD_BLOCK_SIZE,
                                           LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    if (!_G_hPGDPartition) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not allocate page pool.\r\n");
        __KHEAP_FREE(pvPgdTable);
        __KHEAP_FREE(pvPteTable);
        return  (PX_ERROR);
    }
    
    lib_bzero(pvPteTable, PTE_TABLE_SIZE);

    _G_pvPTETable = pvPteTable;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: mips32MmuGlobalInit
** 功能描述: 调用 BSP 对 MMU 初始化
** 输　入  : pcMachineName     使用的机器名称
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  mips32MmuGlobalInit (CPCHAR  pcMachineName)
{
    archCacheReset(pcMachineName);                                      /*  复位 CACHE                  */
    
    mipsCp0PageMaskWrite(MIPS32_PAGE_MASK);                             /*  PAGE MASK                   */

    mipsCp0EntryHiWrite(0);                                             /*  ASID = 0                    */

    mipsCp0WiredWrite(0);                                               /*  全部允许随机替换            */

    mips32MmuInvalidateTLB();                                           /*  无效 TLB                    */

    if (_G_iMachineType == MIPS_MACHINE_TYPE_LS3X) {
        mipsCp0PageGrainWrite(0);                                       /*  禁用龙芯 3A 的一些扩展功能  */
        if (_G_bLsHasFTLB) {
            mipsCp0GSConfigWrite(mipsCp0GSConfigRead() | (1 << 22));    /*  只用 VTLB, 不用 FTLB        */
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: mips32MmuPgdOffset
** 功能描述: 通过虚拟地址计算 PGD 项
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址
** 输　出  : 对应的 PGD 表项地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY *mips32MmuPgdOffset (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry = pmmuctx->MMUCTX_pgdEntry;
    REGISTER UINT32              uiPgdNum;
    
    uiPgdNum   = ulAddr >> LW_CFG_VMM_PGD_SHIFT;
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry |
                 (uiPgdNum * sizeof(LW_PGD_TRANSENTRY)));               /*  获得一级页表描述符地址      */
               
    return  (p_pgdentry);
}
/*********************************************************************************************************
** 函数名称: mips32MmuPmdOffset
** 功能描述: 通过虚拟地址计算 PMD 项
** 输　入  : p_pgdentry     pgd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 对应的 PMD 表项地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PMD_TRANSENTRY *mips32MmuPmdOffset (LW_PGD_TRANSENTRY  *p_pgdentry, addr_t  ulAddr)
{
    return  ((LW_PMD_TRANSENTRY *)p_pgdentry);                          /*  MIPS 无 PMD 项              */
}
/*********************************************************************************************************
** 函数名称: mips32MmuPteOffset
** 功能描述: 通过虚拟地址计算 PTE 项
** 输　入  : p_pmdentry     pmd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 对应的 PTE 表项地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static  LW_PTE_TRANSENTRY *mips32MmuPteOffset (LW_PMD_TRANSENTRY  *p_pmdentry, addr_t  ulAddr)
{
    REGISTER LW_PTE_TRANSENTRY  *p_pteentry;
    REGISTER UINT32              uiTemp;
    REGISTER UINT32              uiPageNum;

    uiTemp = (UINT32)(*p_pmdentry);                                     /*  获得一级页表描述符          */
    
    p_pteentry = (LW_PTE_TRANSENTRY *)(uiTemp);                         /*  获得二级页表基地址          */

    ulAddr &= ~LW_CFG_VMM_PGD_MASK;

    uiPageNum = ulAddr >> LW_CFG_VMM_PAGE_SHIFT;

    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry |
                  (uiPageNum * sizeof(LW_PTE_TRANSENTRY)));             /*  获得虚拟地址页表描述符地址  */
    
    return  (p_pteentry);
}
/*********************************************************************************************************
** 函数名称: mips32MmuPgdIsOk
** 功能描述: 判断 PGD 项的描述符是否正确
** 输　入  : pgdentry       PGD 项描述符
** 输　出  : 是否正确
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static BOOL  mips32MmuPgdIsOk (LW_PGD_TRANSENTRY  pgdentry)
{
    return  (pgdentry ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: mips32MmuPteIsOk
** 功能描述: 判断 PTE 项的描述符是否正确
** 输　入  : pteentry       PTE 项描述符
** 输　出  : 是否正确
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static BOOL  mips32MmuPteIsOk (LW_PTE_TRANSENTRY  pteentry)
{
    return  ((pteentry & MIPS32_ENTRYLO_V_BIT) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: mips32MmuPgdAlloc
** 功能描述: 分配 PGD 项
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址 (参数 0 即偏移量为 0 , 需要返回页表基地址)
** 输　出  : 分配 PGD 地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY *mips32MmuPgdAlloc (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry = (LW_PGD_TRANSENTRY *)API_PartitionGet(_G_hPGDPartition);
    REGISTER UINT32              uiPgdNum;
    
    if (!p_pgdentry) {
        return  (LW_NULL);
    }
    
    lib_bzero(p_pgdentry, PGD_BLOCK_SIZE);                              /*  无效一级页表项              */

    uiPgdNum   = ulAddr >> LW_CFG_VMM_PGD_SHIFT;
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry |
                 (uiPgdNum * sizeof(LW_PGD_TRANSENTRY)));               /*  获得一级页表描述符地址      */
               
    return  (p_pgdentry);
}
/*********************************************************************************************************
** 函数名称: mips32MmuPgdFree
** 功能描述: 释放 PGD 项
** 输　入  : p_pgdentry     pgd 入口地址
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  mips32MmuPgdFree (LW_PGD_TRANSENTRY  *p_pgdentry)
{
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry & (~(PGD_BLOCK_SIZE - 1)));
    
    API_PartitionPut(_G_hPGDPartition, (PVOID)p_pgdentry);
}
/*********************************************************************************************************
** 函数名称: mips32MmuPmdAlloc
** 功能描述: 分配 PMD 项
** 输　入  : pmmuctx        mmu 上下文
**           p_pgdentry     pgd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 分配 PMD 地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PMD_TRANSENTRY *mips32MmuPmdAlloc (PLW_MMU_CONTEXT     pmmuctx,
                                             LW_PGD_TRANSENTRY  *p_pgdentry,
                                             addr_t              ulAddr)
{
    return  ((LW_PMD_TRANSENTRY *)p_pgdentry);
}
/*********************************************************************************************************
** 函数名称: mips32MmuPmdFree
** 功能描述: 释放 PMD 项
** 输　入  : p_pmdentry     pmd 入口地址
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  mips32MmuPmdFree (LW_PMD_TRANSENTRY  *p_pmdentry)
{
    (VOID)p_pmdentry;
}
/*********************************************************************************************************
** 函数名称: mips32MmuPteAlloc
** 功能描述: 分配 PTE 项
** 输　入  : pmmuctx        mmu 上下文
**           p_pmdentry     pmd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 分配 PTE 地址
** 全局变量: 
** 调用模块: VMM 这里没有关闭中断, 回写 CACHE 时, 需要手动关中断, SylixOS 映射完毕会自动清快表, 所以
             这里不用清除快表.
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  *mips32MmuPteAlloc (PLW_MMU_CONTEXT     pmmuctx,
                                              LW_PMD_TRANSENTRY  *p_pmdentry,
                                              addr_t              ulAddr)
{
    REGISTER LW_PTE_TRANSENTRY  *p_pteentry;
    REGISTER UINT32              uiPgdNum;

    uiPgdNum   = ulAddr >> LW_CFG_VMM_PGD_SHIFT;
    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)_G_pvPTETable | (uiPgdNum * PTE_BLOCK_SIZE));

    lib_bzero(p_pteentry, PTE_BLOCK_SIZE);

    *p_pmdentry = (LW_PMD_TRANSENTRY)mips32MmuBuildPgdesc((UINT32)p_pteentry);  /*  设置二级页表基地址  */

    return  (mips32MmuPteOffset(p_pmdentry, ulAddr));
}
/*********************************************************************************************************
** 函数名称: mips32MmuPteFree
** 功能描述: 释放 PTE 项
** 输　入  : p_pteentry     pte 入口地址
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  mips32MmuPteFree (LW_PTE_TRANSENTRY  *p_pteentry)
{
    (VOID)p_pteentry;
}
/*********************************************************************************************************
** 函数名称: mips32MmuPtePhysGet
** 功能描述: 通过 PTE 表项, 查询物理地址
** 输　入  : pteentry           pte 表项
**           pulPhysicalAddr    获得的物理地址
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  mips32MmuPtePhysGet (LW_PTE_TRANSENTRY  pteentry, addr_t  *pulPhysicalAddr)
{
    UINT32   uiPFN   = (pteentry & MIPS32_ENTRYLO_PFN_MASK) >>
                        MIPS32_ENTRYLO_PFN_SHIFT;                       /*  获得物理页面号              */

    *pulPhysicalAddr = uiPFN << 12;                                     /*  计算页面物理地址            */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: mips32MmuFlagGet
** 功能描述: 获得指定虚拟地址的 SylixOS 权限标志
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址
** 输　出  : SylixOS 权限标志
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ULONG  mips32MmuFlagGet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    LW_PGD_TRANSENTRY  *p_pgdentry = mips32MmuPgdOffset(pmmuctx, ulAddr);/*  获得一级描述符地址         */

    if (mips32MmuPgdIsOk(*p_pgdentry)) {                                /*  一级描述符有效              */
        LW_PTE_TRANSENTRY  *p_pteentry = mips32MmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                                            ulAddr);    /*  获得二级描述符地址          */

        LW_PTE_TRANSENTRY   uiDescriptor = *p_pteentry;                 /*  获得二级描述符              */

        if (mips32MmuPteIsOk(uiDescriptor)) {                           /*  二级描述符有效              */
            ULONG    ulFlag = 0;
            UINT32   uiCache;

            if (uiDescriptor & MIPS32_ENTRYLO_V_BIT) {                  /*  有效                        */
            	ulFlag |= LW_VMM_FLAG_VALID;                         	/*  映射有效                    */
            }

            ulFlag |= LW_VMM_FLAG_ACCESS;                               /*  可以访问                    */

            if (_G_bLsHasNE) {
                if (!(uiDescriptor & MIPS32_ENTRYLO_NE_BIT)) {
                    ulFlag |= LW_VMM_FLAG_EXECABLE;                     /*  可以执行                    */
                }
            } else {
                ulFlag |= LW_VMM_FLAG_EXECABLE;                         /*  可以执行                    */
            }

            if (uiDescriptor & MIPS32_ENTRYLO_D_BIT) {                  /*  可写                        */
                ulFlag |= LW_VMM_FLAG_WRITABLE;
            }

            uiCache = (uiDescriptor & MIPS32_ENTRYLO_C_MASK)
                       >> MIPS32_ENTRYLO_C_SHIFT;                       /*  获得 CACHE 属性             */
            switch (uiCache) {

            case MIPS_UNCACHED:                                         /*  不可以 CACHE                */
                break;

            case MIPS_CACHABLE_NONCOHERENT:                             /*  可以 CACHE, 但不参与一致性  */
                ulFlag |= LW_VMM_FLAG_CACHEABLE;                        /*  龙芯 3A 上为一致性 CACHE    */
                break;

            default:
                break;
            }

            return  (ulFlag);
        }
    }

    return  (LW_VMM_FLAG_UNVALID);
}
/*********************************************************************************************************
** 函数名称: mips32MmuFlagSet
** 功能描述: 设置指定虚拟地址的 flag 标志
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址
**           ulFlag         flag 标志
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
** 注  意  : 这里不需要清除快表 TLB, 因为 VMM 自身会作此操作.
*********************************************************************************************************/
static INT  mips32MmuFlagSet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr, ULONG  ulFlag)
{
    LW_PGD_TRANSENTRY  *p_pgdentry = mips32MmuPgdOffset(pmmuctx, ulAddr);/*  获得一级描述符地址         */

    if (!(ulFlag & LW_VMM_FLAG_VALID)) {                                /*  无效的映射关系              */
        return  (PX_ERROR);
    }

    if (mips32MmuPgdIsOk(*p_pgdentry)) {                                /*  一级描述符有效              */
        LW_PTE_TRANSENTRY  *p_pteentry = mips32MmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                                            ulAddr);    /*  获得二级描述符地址          */

        LW_PTE_TRANSENTRY   uiDescriptor = *p_pteentry;                 /*  获得二级描述符              */

        if (mips32MmuPteIsOk(uiDescriptor)) {                           /*  二级描述符有效              */
            UINT32   uiPFN = (uiDescriptor & MIPS32_ENTRYLO_PFN_MASK) >>
                              MIPS32_ENTRYLO_PFN_SHIFT;                 /*  获得物理页面号              */
            addr_t   ulPhysicalAddr = uiPFN << 12;                      /*  计算页面物理地址            */

            /*
             * 构建二级描述符并设置二级描述符
             */
            *p_pteentry = (LW_PTE_TRANSENTRY)mips32MmuBuildPtentry((UINT32)ulPhysicalAddr,
                                                                   ulFlag);
            return  (ERROR_NONE);

        } else {
            return  (PX_ERROR);
        }

    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: mips32MmuMakeTrans
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
static VOID  mips32MmuMakeTrans (PLW_MMU_CONTEXT     pmmuctx,
                                 LW_PTE_TRANSENTRY  *p_pteentry,
                                 addr_t              ulVirtualAddr,
                                 addr_t              ulPhysicalAddr,
                                 addr_t              ulFlag)
{
    if (!(ulFlag & LW_VMM_FLAG_VALID)) {                                /*  无效的映射关系              */
        return;
    }

    /*
     * 构建二级描述符并设置二级描述符
     */
    *p_pteentry = (LW_PTE_TRANSENTRY)mips32MmuBuildPtentry((UINT32)ulPhysicalAddr,
                                                           ulFlag);
}
/*********************************************************************************************************
** 函数名称: mipsMmuTlbLoadStoreExcHandle
** 功能描述: MMU TLB 加载/存储异常处理
** 输　入  : ulAbortAddr       终止地址
** 输　出  : 终止类型
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ULONG  mipsMmuTlbLoadStoreExcHandle (addr_t  ulAbortAddr)
{
    ULONG       ulAbortType  = LW_VMM_ABORT_TYPE_MAP;
    UINT32      uiEntryHiBak = mipsCp0EntryHiRead();
    UINT32      uiMask;
    UINT32      uiEntryLo;
    INT32       iIndex;
    BOOL        bIsEntryLo1;

    mipsCp0EntryHiWrite(ulAbortAddr & (LW_CFG_VMM_PAGE_MASK << 1));

    MIPS32_MMU_TLB_PROBE();

    iIndex = mipsCp0IndexRead();
    if (iIndex >= 0) {
        MIPS32_MMU_TLB_READ();

        uiMask = mipsCp0PageMaskRead() | ~(LW_CFG_VMM_PAGE_MASK << 1);

        bIsEntryLo1 = !!(ulAbortAddr & uiMask & ~(uiMask >> 1));
        if (bIsEntryLo1) {
            uiEntryLo = mipsCp0EntryLo1Read();
        } else {
            uiEntryLo = mipsCp0EntryLo0Read();
        }

        if (uiEntryLo & MIPS32_ENTRYLO_V_BIT) {
            /*
             * TLB 中有映射条目, 并且映射有效, 理论上不应该出现这种情况,
             * 但在龙芯 1B 处理器上偶尔会出现(QEMU 并不出现),
             * 可能这是龙芯 1B 处理器的 BUG, 但不响应程序继续运行
             */
            ulAbortType = 0;
        } else {
            /*
             * TLB 无效异常(正确情况)
             */
            ulAbortType = LW_VMM_ABORT_TYPE_MAP;
        }
    } else {
        /*
         * 需要 TLB 重填, 但设计的 TLB 重填机制并不会由通用异常入口来处理
         */
        ULONG  ulNesting = LW_CPU_GET_CUR_NESTING();
        if (ulNesting > 1) {
        	/*
        	 * 如果 TLB 重填异常发生在异常里, 直接终止
        	 */
            _DebugHandle(__ERRORMESSAGE_LEVEL, "TLB refill error.\r\n");
            ulAbortType = LW_VMM_ABORT_TYPE_FATAL_ERROR;
        } else {
        	/*
        	 * 如果 TLB 重填异常不是发生异常里, 忽略之, 也不进行 TLB 重填
             * QEMU 运行 Qt 时会出现, 龙芯 1B 处理器并不出现
        	 */
            ulAbortType = 0;
        }
    }

    mipsCp0EntryHiWrite(uiEntryHiBak);

    return  (ulAbortType);
}
/*********************************************************************************************************
** 函数名称: mips32MmuMakeCurCtx
** 功能描述: 设置 MMU 当前上下文
** 输　入  : pmmuctx        mmu 上下文
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  mips32MmuMakeCurCtx (PLW_MMU_CONTEXT  pmmuctx)
{
    mipsCp0ContextWrite((UINT32)_G_pvPTETable);                         /*  将 PTE 表写入 Context 寄存器*/
}
/*********************************************************************************************************
** 函数名称: mips32MmuInvTLB
** 功能描述: 无效当前 CPU TLB
** 输　入  : pmmuctx        mmu 上下文
**           ulPageAddr     页面虚拟地址
**           ulPageNum      页面个数
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  mips32MmuInvTLB (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulPageAddr, ULONG  ulPageNum)
{
    REGISTER ULONG   i;

    if (ulPageNum > (MIPS32_TLB_SIZE >> 1)) {
        mips32MmuInvalidateTLB();                                       /*  全部清除 TLB                */

    } else {
        for (i = 0; i < ulPageNum; i++) {
            mips32MmuInvalidateTLBMVA((addr_t)ulPageAddr);              /*  逐个页面清除 TLB            */
            ulPageAddr += LW_CFG_VMM_PAGE_SIZE;
        }

        mips32MmuInvalidateMicroTLB();                                  /*  无效 Micro TLB              */
    }
}
/*********************************************************************************************************
** 函数名称: mips32MmuInit
** 功能描述: MMU 系统初始化
** 输　入  : pmmuop            MMU 操作函数集
**           pcMachineName     使用的机器名称
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  mips32MmuInit (LW_MMU_OP  *pmmuop, CPCHAR  pcMachineName)
{
    UINT32  uiMMUSize;
    UINT32  uiConfig = mipsCp0ConfigRead();                             /*  读 Config0                  */
    UINT8   ucMT;

    ucMT = ((uiConfig & M_ConfigMT) >> S_ConfigMT);
    if (ucMT != 1) {                                                    /*  Config0 MT 域 != 1，没有 MMU*/
        _DebugFormat(__PRINTMESSAGE_LEVEL, "Warning: Config register MMU type is not standard: %d!\r\n", ucMT);
    }

    if (uiConfig & (M_ConfigMore)) {                                    /*  有 Config1                  */
        uiConfig     = mipsCp0Config1Read();                            /*  读 Config1                  */
        uiMMUSize    = (uiConfig >> 25) & 0x3F;                         /*  获得 MMUSize 域             */
        _G_uiTlbSize = uiMMUSize + 1;

    } else {
        _G_uiTlbSize = 64;                                              /*  按最大算                    */
    }

    _DebugFormat(__LOGMESSAGE_LEVEL, "%s MMU TLB size = %d.\r\n", pcMachineName, MIPS32_TLB_SIZE);

    if ((lib_strcmp(pcMachineName, MIPS_MACHINE_LS1X) == 0)) {
        _G_iMachineType = MIPS_MACHINE_TYPE_LS1X;
        _G_bLsHasNE     = LW_TRUE;

    } else if ((lib_strcmp(pcMachineName, MIPS_MACHINE_LS2X) == 0)) {
        _G_iMachineType = MIPS_MACHINE_TYPE_LS2X;
        _G_bLsHasITLB   = LW_TRUE;                                      /*  Loongson2x 都有 ITLB        */
        _G_bLsHasNE     = LW_TRUE;

    } else if ((lib_strcmp(pcMachineName, MIPS_MACHINE_LS3X) == 0)) {
        _G_iMachineType = MIPS_MACHINE_TYPE_LS3X;
        _G_bLsHasITLB   = LW_TRUE;                                      /*  Loongson3x 都有 ITLB        */
        _G_bLsHasNE     = LW_TRUE;

        switch (mipsCp0PRIdRead() & 0xf) {

        case PRID_REV_LOONGSON3A_R2:                                    /*  Loongson3A2000              */
        case PRID_REV_LOONGSON3A_R3:                                    /*  Loongson3A3000              */
            _G_bLsHasDTLB = LW_TRUE;                                    /*  有 ITLB, DTLB, VTLB, FTLB   */
            _G_bLsHasFTLB = LW_TRUE;
            break;

        case PRID_REV_LOONGSON2K:                                       /*  Loongson2K1000              */
            _G_bLsHasDTLB = LW_FALSE;
            _G_bLsHasFTLB = LW_FALSE;
            break;

        case PRID_REV_LOONGSON3A_R1:                                    /*  Loongson3A1000              */
        case PRID_REV_LOONGSON3B_R1:                                    /*  Loongson3B1000              */
        case PRID_REV_LOONGSON3B_R2:                                    /*  Loongson3B2000              */
        default:                                                        /*  只有 JTLB 和 ITLB           */
            break;
        }

    } else if ((lib_strcmp(pcMachineName, MIPS_MACHINE_JZ47XX) == 0)) {
        _G_iMachineType = MIPS_MACHINE_TYPE_JZ47XX;
    }

#if LW_CFG_SMP_EN > 0
    pmmuop->MMUOP_ulOption = LW_VMM_MMU_FLUSH_TLB_MP;
#else
    pmmuop->MMUOP_ulOption = 0ul;
#endif                                                                  /*  LW_CFG_SMP_EN               */

    pmmuop->MMUOP_pfuncMemInit       = mips32MmuMemInit;
    pmmuop->MMUOP_pfuncGlobalInit    = mips32MmuGlobalInit;
    
    pmmuop->MMUOP_pfuncPGDAlloc      = mips32MmuPgdAlloc;
    pmmuop->MMUOP_pfuncPGDFree       = mips32MmuPgdFree;
    pmmuop->MMUOP_pfuncPMDAlloc      = mips32MmuPmdAlloc;
    pmmuop->MMUOP_pfuncPMDFree       = mips32MmuPmdFree;
    pmmuop->MMUOP_pfuncPTEAlloc      = mips32MmuPteAlloc;
    pmmuop->MMUOP_pfuncPTEFree       = mips32MmuPteFree;
    
    pmmuop->MMUOP_pfuncPGDIsOk       = mips32MmuPgdIsOk;
    pmmuop->MMUOP_pfuncPMDIsOk       = mips32MmuPgdIsOk;
    pmmuop->MMUOP_pfuncPTEIsOk       = mips32MmuPteIsOk;
    
    pmmuop->MMUOP_pfuncPGDOffset     = mips32MmuPgdOffset;
    pmmuop->MMUOP_pfuncPMDOffset     = mips32MmuPmdOffset;
    pmmuop->MMUOP_pfuncPTEOffset     = mips32MmuPteOffset;
    
    pmmuop->MMUOP_pfuncPTEPhysGet    = mips32MmuPtePhysGet;
    
    pmmuop->MMUOP_pfuncFlagGet       = mips32MmuFlagGet;
    pmmuop->MMUOP_pfuncFlagSet       = mips32MmuFlagSet;
    
    pmmuop->MMUOP_pfuncMakeTrans     = mips32MmuMakeTrans;
    pmmuop->MMUOP_pfuncMakeCurCtx    = mips32MmuMakeCurCtx;
    pmmuop->MMUOP_pfuncInvalidateTLB = mips32MmuInvTLB;
    pmmuop->MMUOP_pfuncSetEnable     = mips32MmuEnable;
    pmmuop->MMUOP_pfuncSetDisable    = mips32MmuDisable;
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
