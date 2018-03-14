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
** 文   件   名: sparcMmu.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2017 年 10 月 09 日
**
** 描        述: SPARC 体系构架 LEON 变种处理器 MMU 驱动.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "SylixOS.h"
#if LW_CFG_CACHE_EN > 0
#include "../cache/sparcCache.h"
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  PTD PTE 中的位定义
*********************************************************************************************************/
#define SPARC_MMU_ET_INVALID            (0)                             /*  映射无效                    */
#define SPARC_MMU_ET_PTD                (1)
#define SPARC_MMU_ET_PTE                (2)
#define SPARC_MMU_ET_SHIFT              (0)
#define SPARC_MMU_ET_MASK               (0x3 << SPARC_MMU_ET_SHIFT)

#define SPARC_MMU_ACC_S_RO              (0)                             /*  只读                        */
#define SPARC_MMU_ACC_S_RW              (1)                             /*  可读写                      */
#define SPARC_MMU_ACC_S_RX              (2)                             /*  可读执行                    */
#define SPARC_MMU_ACC_S_RWX             (3)                             /*  可读写执行                  */
#define SPARC_MMU_ACC_SHIFT             (2)
#define SPARC_MMU_ACC_MASK              (0x7 << SPARC_MMU_ACC_SHIFT)

#define SPARC_MMU_R                     (1)                             /*  访问过                      */
#define SPARC_MMU_R_NO                  (0)
#define SPARC_MMU_R_SHIFT               (5)
#define SPARC_MMU_R_MASK                (1 << SPARC_MMU_R_SHIFT)

#define SPARC_MMU_M                     (1)                             /*  修改过                      */
#define SPARC_MMU_M_NO                  (0)
#define SPARC_MMU_M_SHIFT               (6)
#define SPARC_MMU_M_MASK                (1 << SPARC_MMU_M_SHIFT)

#define SPARC_MMU_C                     (1)                             /*  可 CACHE                    */
#define SPARC_MMU_C_NO                  (0)
#define SPARC_MMU_C_SHIFT               (7)
#define SPARC_MMU_C_MASK                (1 << SPARC_MMU_C_SHIFT)

#define SPARC_MMU_PTP_SHIFT             (2)                             /*  Page Table Pointer          */
#define SPARC_MMU_PPN_SHIFT             (8)                             /*  Physical Page Number        */

#define SPARC_MMU_PTP_PA_SHIFT          (6)
#define SPARC_MMU_PPN_PA_SHIFT          (LW_CFG_VMM_PAGE_SHIFT)
/*********************************************************************************************************
  MMU 控制寄存器相关宏定义
*********************************************************************************************************/
#define SPARC_MMU_CTRL_EN               (1 << 0)                        /*  Enable bit                  */
#define SPARC_MMU_CTRL_NF               (1 << 1)                        /*  No Fault                    */
#define SPARC_MMU_CTRL_PSO              (1 << 7)                        /*  Partial Store Ordering      */
/*********************************************************************************************************
  外部函数声明
*********************************************************************************************************/
extern UINT32  leonMmuGetCtrlReg(VOID);
extern VOID    leonMmuSetCtrlReg(UINT32  uiValue);
extern VOID    leonMmuSetCtxTblPtr(UINT32  uiValue);
extern VOID    leonMmuSetCtx(UINT32  uiValue);
extern UINT32  leonMmuGetCtx(VOID);
extern UINT32  leonMmuGetFaultStatus(VOID);
extern UINT32  leonMmuGetFaultAddr(VOID);
/*********************************************************************************************************
  全局变量定义
*********************************************************************************************************/
static LW_OBJECT_HANDLE     _G_hPGDPartition;                           /*  系统目前仅使用一个 PGD      */
static LW_OBJECT_HANDLE     _G_hPMDPartition;                           /*  PMD 缓冲区                  */
static LW_OBJECT_HANDLE     _G_hPTEPartition;                           /*  PTE 缓冲区                  */
static UINT32              *_G_puiCtxTbl;                               /*  上下文表                    */
/*********************************************************************************************************
  与处理器实现(LEON)有关的函数
*********************************************************************************************************/
/*********************************************************************************************************
** 函数名称: leonMmuFlushTlbAll
** 功能描述: LEON 无效所有 TLB
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_INLINE VOID  leonMmuFlushTlbAll (VOID)
{
    __asm__ __volatile__ ("sta %%g0, [%0] %1\n\t" : : "r"(0x400),
                          "i"(ASI_LEON_MMUFLUSH)  : "memory");
}
/*********************************************************************************************************
** 函数名称: sparcMmuGetFaultAddr
** 功能描述: 获得错误地址
** 输　入  : NONE
** 输　出  : 错误地址
** 全局变量:
** 调用模块:
*********************************************************************************************************/
addr_t  sparcMmuGetFaultAddr (VOID)
{
    return  (leonMmuGetFaultAddr());
}
/*********************************************************************************************************
** 函数名称: sparcMmuGetFaultStatus
** 功能描述: 获得错误状态
** 输　入  : NONE
** 输　出  : 错误状态
** 全局变量:
** 调用模块:
*********************************************************************************************************/
UINT32  sparcMmuGetFaultStatus (VOID)
{
    return  (leonMmuGetFaultStatus());
}
/*********************************************************************************************************
** 函数名称: sparcMmuEnable
** 功能描述: 使能 MMU
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  sparcMmuEnable (VOID)
{
    UINT32  uiCtrl;

    uiCtrl  = leonMmuGetCtrlReg();
    uiCtrl |= SPARC_MMU_CTRL_EN;
    uiCtrl |= SPARC_MMU_CTRL_PSO;
    uiCtrl &= ~(SPARC_MMU_CTRL_NF);
    leonMmuSetCtrlReg(uiCtrl);
}
/*********************************************************************************************************
** 函数名称: sparcMmuDisable
** 功能描述: 禁能 MMU
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  sparcMmuDisable (VOID)
{
    UINT32  uiCtrl;

    uiCtrl  = leonMmuGetCtrlReg();
    uiCtrl &= ~(SPARC_MMU_CTRL_EN);
    leonMmuSetCtrlReg(uiCtrl);
}
/*********************************************************************************************************
** 函数名称: sparcMmuMakeCurCtx
** 功能描述: 设置 MMU 当前上下文
** 输　入  : pmmuctx        mmu 上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  sparcMmuMakeCurCtx (PLW_MMU_CONTEXT  pmmuctx)
{
    UINT32  uiCtxPde;
    UINT32  uiCtxTblPtr;

    /*
     * 设置上下文表的第 0 个条目
     */
    uiCtxPde   = ((UINT32)pmmuctx->MMUCTX_pgdEntry) >> SPARC_MMU_PTP_PA_SHIFT;
    uiCtxPde <<= SPARC_MMU_PTP_SHIFT;
    uiCtxPde  |= SPARC_MMU_ET_PTD;
    _G_puiCtxTbl[0] = uiCtxPde;

    /*
     * 上下文表指针寄存器值
     */
    uiCtxTblPtr   = (addr_t)_G_puiCtxTbl >> SPARC_MMU_PTP_PA_SHIFT;
    uiCtxTblPtr <<= SPARC_MMU_PTP_SHIFT;

    /*
     * 设置上下文和上下文表指针寄存器
     */
    leonMmuSetCtx(0);
    leonMmuSetCtxTblPtr(uiCtxTblPtr);
}
/*********************************************************************************************************
** 函数名称: sparcMmuInvalidateTLB
** 功能描述: 无效所有 TLB
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  sparcMmuInvalidateTLB (VOID)
{
    leonMmuFlushTlbAll();
}
/*********************************************************************************************************
** 函数名称: sparcMmuInvalidateTLBMVA
** 功能描述: 无效指定 MVA 的 TLB
** 输　入  : pvAddr            虚拟地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  sparcMmuInvalidateTLBMVA (PVOID  pvAddr)
{
    leonMmuFlushTlbAll();                                               /*  LEON 无单个 TLB flush 操作  */
}
/*********************************************************************************************************
  与处理器实现无关, 与 SPARCv8 相关的函数
*********************************************************************************************************/
/*********************************************************************************************************
** 函数名称: sparcMmuFlags2Attr
** 功能描述: 根据 SylixOS 权限标志, 生成 sparc MMU 权限标志
** 输　入  : ulFlag                  SylixOS 权限标志
**           pucACC                  访问权限
**           pucC                    CACHE 属性
**           pucET                   Entry Type
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  sparcMmuFlags2Attr (ULONG   ulFlag,
                                UINT8  *pucACC,
                                UINT8  *pucC,
                                UINT8  *pucET)
{
    if (!(ulFlag & LW_VMM_FLAG_VALID)) {                                /*  无效的映射关系              */
        return  (PX_ERROR);
    }

    if (ulFlag & LW_VMM_FLAG_ACCESS) {                                  /*  是否可访问                  */
        *pucET = SPARC_MMU_ET_PTE;

    } else {
        *pucET = SPARC_MMU_ET_INVALID;
    }

    if (ulFlag & LW_VMM_FLAG_WRITABLE) {                                /*  是否可写                    */
        if (ulFlag & LW_VMM_FLAG_EXECABLE) {                            /*  是否可执行                  */
            *pucACC = SPARC_MMU_ACC_S_RWX;
        } else {
            *pucACC = SPARC_MMU_ACC_S_RW;
        }

    } else {
        if (ulFlag & LW_VMM_FLAG_EXECABLE) {                            /*  是否可执行                  */
            *pucACC = SPARC_MMU_ACC_S_RX;
        } else {
            *pucACC = SPARC_MMU_ACC_S_RO;
        }
    }

    if ((ulFlag & LW_VMM_FLAG_CACHEABLE) ||
        (ulFlag & LW_VMM_FLAG_BUFFERABLE)) {                            /*  CACHE 与 BUFFER 控制        */
        *pucC = SPARC_MMU_C;

    } else {
        *pucC = SPARC_MMU_C_NO;
    }

    return (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: sparcMmuAttr2Flags
** 功能描述: 根据 sparc MMU 权限标志, 生成 SylixOS 权限标志
** 输　入  : ucACC                   访问权限
**           ucC                     CACHE 属性
**           ucET                    Entry Type
**           pulFlag                 SylixOS 权限标志
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  sparcMmuAttr2Flags (UINT8   ucACC,
                                UINT8   ucC,
                                UINT8   ucET,
                                ULONG  *pulFlag)
{
    *pulFlag = LW_VMM_FLAG_VALID;

    if (ucET == SPARC_MMU_ET_PTE) {
        *pulFlag |= LW_VMM_FLAG_ACCESS;

    } else {
        *pulFlag |= LW_VMM_FLAG_UNACCESS;
    }

    if (ucC == SPARC_MMU_C) {
        *pulFlag |= LW_VMM_FLAG_CACHEABLE;

    } else {
        *pulFlag |= LW_VMM_FLAG_UNCACHEABLE;
    }

    switch (ucACC) {

    case SPARC_MMU_ACC_S_RO:
        break;

    case SPARC_MMU_ACC_S_RW:
        *pulFlag |= LW_VMM_FLAG_WRITABLE;
        break;

    case SPARC_MMU_ACC_S_RX:
        *pulFlag |= LW_VMM_FLAG_EXECABLE;
        break;

    case SPARC_MMU_ACC_S_RWX:
        *pulFlag |= LW_VMM_FLAG_WRITABLE | LW_VMM_FLAG_EXECABLE;
        break;

    default:
        break;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: sparcMmuBuildPgdEntry
** 功能描述: 生成一个一级描述符 (PGD 描述符)
** 输　入  : uiBaseAddr              基地址     (二级页表基地址)
**           ucACC                   访问权限
**           ucC                     CACHE 属性
**           ucET                    Entry Type
** 输　出  : 一级描述符
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  sparcMmuBuildPgdEntry (UINT32  uiBaseAddr,
                                                 UINT8   ucACC,
                                                 UINT8   ucC,
                                                 UINT8   ucET)
{
    LW_PGD_TRANSENTRY  uiDescriptor;
    UINT32             uiPTP = uiBaseAddr >> SPARC_MMU_PTP_PA_SHIFT;

    uiDescriptor = (uiPTP << SPARC_MMU_PTP_SHIFT)
                 | ((ucET ? SPARC_MMU_ET_PTD : SPARC_MMU_ET_INVALID) << SPARC_MMU_ET_SHIFT);

    return  (uiDescriptor);
}
/*********************************************************************************************************
** 函数名称: sparcMmuBuildPmdEntry
** 功能描述: 生成一个二级描述符 (PMD 描述符)
** 输　入  : uiBaseAddr              基地址     (三级页表基地址)
**           ucACC                   访问权限
**           ucC                     CACHE 属性
**           ucET                    Entry Type
** 输　出  : 二级描述符
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_PMD_TRANSENTRY  sparcMmuBuildPmdEntry (UINT32  uiBaseAddr,
                                                 UINT8   ucACC,
                                                 UINT8   ucC,
                                                 UINT8   ucET)
{
    LW_PMD_TRANSENTRY  uiDescriptor;
    UINT32             uiPTP = uiBaseAddr >> SPARC_MMU_PTP_PA_SHIFT;

    uiDescriptor = (uiPTP << SPARC_MMU_PTP_SHIFT)
                 | ((ucET ? SPARC_MMU_ET_PTD : SPARC_MMU_ET_INVALID) << SPARC_MMU_ET_SHIFT);

    return  (uiDescriptor);
}
/*********************************************************************************************************
** 函数名称: sparcMmuBuildPteEntry
** 功能描述: 生成一个三级描述符 (PTE 描述符)
** 输　入  : uiBaseAddr              基地址     (页地址)
**           ucACC                   访问权限
**           ucC                     CACHE 属性
**           ucET                    Entry Type
** 输　出  : 三级描述符
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  sparcMmuBuildPteEntry (UINT32  uiBaseAddr,
                                                 UINT8   ucACC,
                                                 UINT8   ucC,
                                                 UINT8   ucET)
{
    LW_PTE_TRANSENTRY  uiDescriptor;
    UINT32             uiPPN = uiBaseAddr >> LW_CFG_VMM_PAGE_SHIFT;
    
    uiDescriptor = (uiPPN << SPARC_MMU_PPN_SHIFT)
                 | (ucACC << SPARC_MMU_ACC_SHIFT)
                 | (ucC   << SPARC_MMU_C_SHIFT)
                 | (ucET  << SPARC_MMU_ET_SHIFT);

    return  (uiDescriptor);
}
/*********************************************************************************************************
** 函数名称: sparcMmuMemInit
** 功能描述: 初始化 MMU 页表内存区
** 输　入  : pmmuctx        mmu 上下文
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
** 注  意  : sparc 体系结构要求: 一级页表基地址需要保持 1  KByte 对齐, 单条目映射 16  MByte 空间.
                                 二级页表基地址需要保持 256 Byte 对齐, 单条目映射 256 KByte 空间.
                                 三级页表基地址需要保持 256 Byte 对齐, 单条目映射 4   KByte 空间.
                                 上下文表需要保持 1 KByte 对齐.
*********************************************************************************************************/
static INT  sparcMmuMemInit (PLW_MMU_CONTEXT  pmmuctx)
{
#define CTX_TBL_SIZE    (1 * LW_CFG_KB_SIZE)
#define PGD_BLOCK_SIZE  (1 * LW_CFG_KB_SIZE)
#define PMD_BLOCK_SIZE  (256)
#define PTE_BLOCK_SIZE  (256)

    PVOID  pvPgdTable;
    PVOID  pvPmdTable;
    PVOID  pvPteTable;
    
    ULONG  ulPgdNum = bspMmuPgdMaxNum();
    ULONG  ulPmdNum = bspMmuPmdMaxNum();
    ULONG  ulPteNum = bspMmuPteMaxNum();
    
    _G_puiCtxTbl = __KHEAP_ALLOC_ALIGN(CTX_TBL_SIZE, CTX_TBL_SIZE);

    pvPgdTable = __KHEAP_ALLOC_ALIGN((size_t)ulPgdNum * PGD_BLOCK_SIZE, PGD_BLOCK_SIZE);
    pvPmdTable = __KHEAP_ALLOC_ALIGN((size_t)ulPmdNum * PMD_BLOCK_SIZE, PMD_BLOCK_SIZE);
    pvPteTable = __KHEAP_ALLOC_ALIGN((size_t)ulPteNum * PTE_BLOCK_SIZE, PTE_BLOCK_SIZE);
    
    if (!_G_puiCtxTbl || !pvPgdTable || !pvPmdTable || !pvPteTable) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not allocate page table.\r\n");
        return  (PX_ERROR);
    }
    
    lib_bzero(_G_puiCtxTbl, CTX_TBL_SIZE);

    _G_hPGDPartition = API_PartitionCreate("pgd_pool", pvPgdTable, ulPgdNum, PGD_BLOCK_SIZE,
                                           LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    _G_hPMDPartition = API_PartitionCreate("pmd_pool", pvPmdTable, ulPmdNum, PMD_BLOCK_SIZE,
                                           LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    _G_hPTEPartition = API_PartitionCreate("pte_pool", pvPteTable, ulPteNum, PTE_BLOCK_SIZE,
                                           LW_OPTION_OBJECT_GLOBAL, LW_NULL);
                                           
    if (!_G_hPGDPartition || !_G_hPMDPartition || !_G_hPTEPartition) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not allocate page pool.\r\n");
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: sparcMmuGlobalInit
** 功能描述: 调用 BSP 对 MMU 初始化
** 输　入  : pcMachineName     使用的机器名称
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  sparcMmuGlobalInit (CPCHAR  pcMachineName)
{
    archCacheReset(pcMachineName);
    
    sparcMmuInvalidateTLB();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: sparcMmuPgdOffset
** 功能描述: 通过虚拟地址计算 PGD 项
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址
** 输　出  : 对应的 PGD 表项地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  *sparcMmuPgdOffset (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry = pmmuctx->MMUCTX_pgdEntry;
    REGISTER UINT32              uiPgdNum;
    
    uiPgdNum   = ulAddr >> LW_CFG_VMM_PGD_SHIFT;                        /*  计算 PGD 号                 */

    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry |
                 (uiPgdNum * sizeof(LW_PGD_TRANSENTRY)));               /*  获得一级页表描述符地址      */
               
    return  (p_pgdentry);
}
/*********************************************************************************************************
** 函数名称: sparcMmuPmdOffset
** 功能描述: 通过虚拟地址计算 PMD 项
** 输　入  : p_pgdentry     pgd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 对应的 PMD 表项地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PMD_TRANSENTRY  *sparcMmuPmdOffset (LW_PGD_TRANSENTRY  *p_pgdentry, addr_t  ulAddr)
{
    REGISTER LW_PMD_TRANSENTRY  *p_pmdentry;
    REGISTER LW_PGD_TRANSENTRY   uiTemp;
    REGISTER UINT32              uiPmdNum;
    REGISTER UINT32              uiPTP;
    REGISTER UINT32              uiPhyAddr;

    uiTemp = (LW_PGD_TRANSENTRY)(*p_pgdentry);                          /*  获得一级页表描述符          */

    uiPTP     = uiTemp >> SPARC_MMU_PTP_SHIFT;
    uiPhyAddr = uiPTP  << SPARC_MMU_PTP_PA_SHIFT;

    p_pmdentry = (LW_PMD_TRANSENTRY *)(uiPhyAddr);                      /*  获得二级页表基地址          */

    ulAddr    &= LW_CFG_VMM_PMD_MASK;
    uiPmdNum   = ulAddr >> LW_CFG_VMM_PMD_SHIFT;                        /*  计算 PMD 号                 */

    p_pmdentry = (LW_PMD_TRANSENTRY *)((addr_t)p_pmdentry |
                 (uiPmdNum * sizeof(LW_PMD_TRANSENTRY)));               /*  获得二级页表描述符地址      */

    return  (p_pmdentry);
}
/*********************************************************************************************************
** 函数名称: sparcMmuPteOffset
** 功能描述: 通过虚拟地址计算 PMD 项
** 输　入  : p_pgdentry     pgd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 对应的 PMD 表项地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static  LW_PTE_TRANSENTRY  *sparcMmuPteOffset (LW_PMD_TRANSENTRY  *p_pmdentry, addr_t  ulAddr)
{
    REGISTER LW_PTE_TRANSENTRY  *p_pteentry;
    REGISTER LW_PMD_TRANSENTRY   uiTemp;
    REGISTER UINT32              uiPageNum;
    REGISTER UINT32              uiPTP;
    REGISTER UINT32              uiPhyAddr;

    uiTemp = (LW_PMD_TRANSENTRY)(*p_pmdentry);                          /*  获得二级页表描述符          */

    uiPTP     = uiTemp >> SPARC_MMU_PTP_SHIFT;
    uiPhyAddr = uiPTP  << SPARC_MMU_PTP_PA_SHIFT;

    p_pteentry = (LW_PTE_TRANSENTRY *)(uiPhyAddr);                      /*  获得三级页表基地址          */

    ulAddr    &= LW_CFG_VMM_PTE_MASK;
    uiPageNum  = ulAddr  >> LW_CFG_VMM_PAGE_SHIFT;                      /*  计算段内页号                */

    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry |
                 (uiPageNum * sizeof(LW_PTE_TRANSENTRY)));              /*  获得虚拟地址页表描述符地址  */

    return  (p_pteentry);
}
/*********************************************************************************************************
** 函数名称: sparcMmuPgdIsOk
** 功能描述: 判断 PGD 项的描述符是否正确
** 输　入  : pgdentry       PGD 项描述符
** 输　出  : 是否正确
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static BOOL  sparcMmuPgdIsOk (LW_PGD_TRANSENTRY  pgdentry)
{
    return  ((pgdentry & (SPARC_MMU_ET_PTD << SPARC_MMU_ET_SHIFT)) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: sparcMmuPmdIsOk
** 功能描述: 判断 PMD 项的描述符是否正确
** 输　入  : pmdentry       PGD 项描述符
** 输　出  : 是否正确
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static BOOL  sparcMmuPmdIsOk (LW_PMD_TRANSENTRY  pmdentry)
{
    return  ((pmdentry & (SPARC_MMU_ET_PTD << SPARC_MMU_ET_SHIFT)) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: sparcMmuPteIsOk
** 功能描述: 判断 PTE 项的描述符是否正确
** 输　入  : pteentry       PTE 项描述符
** 输　出  : 是否正确
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static BOOL  sparcMmuPteIsOk (LW_PTE_TRANSENTRY  pteentry)
{
    return  ((pteentry & (SPARC_MMU_ET_PTE << SPARC_MMU_ET_SHIFT)) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: sparcMmuPgdAlloc
** 功能描述: 分配 PGD 项
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址 (参数 0 即偏移量为 0 , 需要返回页表基地址)
** 输　出  : 分配 PGD 地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY *sparcMmuPgdAlloc (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
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
** 函数名称: sparcMmuPgdFree
** 功能描述: 释放 PGD 项
** 输　入  : p_pgdentry     pgd 入口地址
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  sparcMmuPgdFree (LW_PGD_TRANSENTRY  *p_pgdentry)
{
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry & (~(PGD_BLOCK_SIZE - 1)));
    
    API_PartitionPut(_G_hPGDPartition, (PVOID)p_pgdentry);
}
/*********************************************************************************************************
** 函数名称: sparcMmuPmdAlloc
** 功能描述: 分配 PMD 项
** 输　入  : pmmuctx        mmu 上下文
**           p_pgdentry     pgd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 分配 PMD 地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PMD_TRANSENTRY *sparcMmuPmdAlloc (PLW_MMU_CONTEXT     pmmuctx,
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

    *p_pgdentry = sparcMmuBuildPgdEntry((UINT32)p_pmdentry,
                                        SPARC_MMU_ACC_S_RWX,
                                        SPARC_MMU_C,
                                        SPARC_MMU_ET_PTD);              /*  设置一级页表描述符          */
#if LW_CFG_CACHE_EN > 0
    iregInterLevel = KN_INT_DISABLE();
    sparcCacheFlush(DATA_CACHE, (PVOID)p_pgdentry, sizeof(LW_PGD_TRANSENTRY));
    KN_INT_ENABLE(iregInterLevel);
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

    return  (sparcMmuPmdOffset(p_pgdentry, ulAddr));
}
/*********************************************************************************************************
** 函数名称: sparcMmuPmdFree
** 功能描述: 释放 PMD 项
** 输　入  : p_pmdentry     pmd 入口地址
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  sparcMmuPmdFree (LW_PMD_TRANSENTRY  *p_pmdentry)
{
    p_pmdentry = (LW_PMD_TRANSENTRY *)((addr_t)p_pmdentry & (~(PMD_BLOCK_SIZE - 1)));

    API_PartitionPut(_G_hPMDPartition, (PVOID)p_pmdentry);
}
/*********************************************************************************************************
** 函数名称: sparcMmuPteAlloc
** 功能描述: 分配 PTE 项
** 输　入  : pmmuctx        mmu 上下文
**           p_pmdentry     pmd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 分配 PTE 地址
** 全局变量: 
** 调用模块: VMM 这里没有关闭中断, 回写 CACHE 时, 需要手动关中断, SylixOS 映射完毕会自动清快表, 所以
             这里不用清除快表.
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  *sparcMmuPteAlloc (PLW_MMU_CONTEXT     pmmuctx,
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
    
    *p_pmdentry = sparcMmuBuildPmdEntry((UINT32)p_pteentry,
                                        SPARC_MMU_ACC_S_RWX,
                                        SPARC_MMU_C,
                                        SPARC_MMU_ET_PTD);              /*  设置二级页表描述符          */
#if LW_CFG_CACHE_EN > 0
    iregInterLevel = KN_INT_DISABLE();
    sparcCacheFlush(DATA_CACHE, (PVOID)p_pmdentry, sizeof(LW_PMD_TRANSENTRY));
    KN_INT_ENABLE(iregInterLevel);
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

    return  (sparcMmuPteOffset(p_pmdentry, ulAddr));
}
/*********************************************************************************************************
** 函数名称: sparcMmuPteFree
** 功能描述: 释放 PTE 项
** 输　入  : p_pteentry     pte 入口地址
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  sparcMmuPteFree (LW_PTE_TRANSENTRY  *p_pteentry)
{
    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry & (~(PTE_BLOCK_SIZE - 1)));
    
    API_PartitionPut(_G_hPTEPartition, (PVOID)p_pteentry);
}
/*********************************************************************************************************
** 函数名称: sparcMmuPtePhysGet
** 功能描述: 通过 PTE 表项, 查询物理地址
** 输　入  : pteentry           pte 表项
**           pulPhysicalAddr    获得的物理地址
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  sparcMmuPtePhysGet (LW_PTE_TRANSENTRY  pteentry, addr_t  *pulPhysicalAddr)
{
    UINT32      uiPPN;
    UINT32      uiPhyAddr;

    uiPPN     = pteentry >> SPARC_MMU_PPN_SHIFT;
    uiPhyAddr = uiPPN    << SPARC_MMU_PPN_PA_SHIFT;

    *pulPhysicalAddr = (addr_t)(uiPhyAddr);                             /*  获得物理地址                */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: sparcMmuFlagGet
** 功能描述: 获得指定虚拟地址的 SylixOS 权限标志
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址
** 输　出  : SylixOS 权限标志
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ULONG  sparcMmuFlagGet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    LW_PGD_TRANSENTRY  *p_pgdentry = sparcMmuPgdOffset(pmmuctx, ulAddr);/*  获得一级描述符地址          */

    if (p_pgdentry && sparcMmuPgdIsOk(*p_pgdentry)) {                   /*  一级描述符有效              */
        LW_PMD_TRANSENTRY  *p_pmdentry = sparcMmuPmdOffset(p_pgdentry,
                                                           ulAddr);     /*  获得二级描述符地址          */

        if (sparcMmuPmdIsOk(*p_pmdentry)) {                             /*  二级描述符有效              */
            LW_PTE_TRANSENTRY  *p_pteentry = sparcMmuPteOffset(p_pmdentry,
                                                               ulAddr); /*  获得三级描述符地址          */
            LW_PTE_TRANSENTRY   pteentry   = *p_pteentry;

            if (sparcMmuPteIsOk(pteentry)) {                            /*  三级描述符有效              */
                UINT8   ucACC, ucC, ucET;
                ULONG   ulFlag;

                ucACC = (UINT8)((pteentry & SPARC_MMU_ACC_MASK) >> SPARC_MMU_ACC_SHIFT);
                ucC   = (UINT8)((pteentry & SPARC_MMU_C_MASK)   >> SPARC_MMU_C_SHIFT);
                ucET  = (UINT8)((pteentry & SPARC_MMU_ET_MASK)  >> SPARC_MMU_ET_SHIFT);

                sparcMmuAttr2Flags(ucACC, ucC, ucET, &ulFlag);

                return  (ulFlag);
            }
        }
    }

    return  (LW_VMM_FLAG_UNVALID);
}
/*********************************************************************************************************
** 函数名称: sparcMmuFlagSet
** 功能描述: 设置指定虚拟地址的 flag 标志
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址
**           ulFlag         flag 标志
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
** 注  意  : 这里不需要清除快表 TLB, 因为 VMM 自身会作此操作.
*********************************************************************************************************/
static INT  sparcMmuFlagSet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr, ULONG  ulFlag)
{
    UINT8   ucACC, ucC, ucET;

    if (sparcMmuFlags2Attr(ulFlag, &ucACC,
                           &ucC,   &ucET) != ERROR_NONE) {              /*  无效的映射关系              */
        return  (PX_ERROR);
    }

    LW_PGD_TRANSENTRY  *p_pgdentry = sparcMmuPgdOffset(pmmuctx, ulAddr);/*  获得一级描述符地址          */

    if (p_pgdentry && sparcMmuPgdIsOk(*p_pgdentry)) {                   /*  一级描述符有效              */
        LW_PMD_TRANSENTRY  *p_pmdentry = sparcMmuPmdOffset(p_pgdentry,
                                                           ulAddr);     /*  获得二级描述符地址          */

        if (sparcMmuPmdIsOk(*p_pmdentry)) {                             /*  二级描述符有效              */
            LW_PTE_TRANSENTRY  *p_pteentry = sparcMmuPteOffset(p_pmdentry,
                                                               ulAddr); /*  获得三级描述符地址         */
            LW_PTE_TRANSENTRY   pteentry   = *p_pteentry;               /*  获得三级描述符              */

            if (sparcMmuPteIsOk(pteentry)) {                            /*  三级描述符有效              */
                addr_t  ulPhysicalAddr;
                UINT32  uiPPN;

                uiPPN          = pteentry >> SPARC_MMU_PPN_SHIFT;
                ulPhysicalAddr = uiPPN    << SPARC_MMU_PPN_PA_SHIFT;

                *p_pteentry = sparcMmuBuildPteEntry(ulPhysicalAddr,
                                                    ucACC, ucC, ucET);
#if LW_CFG_CACHE_EN > 0
                sparcCacheFlush(DATA_CACHE, (PVOID)p_pteentry, sizeof(LW_PTE_TRANSENTRY));
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
                sparcMmuInvalidateTLBMVA((PVOID)ulAddr);

                return  (ERROR_NONE);
            }
        }
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: sparcMmuMakeTrans
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
static VOID  sparcMmuMakeTrans (PLW_MMU_CONTEXT     pmmuctx,
                                LW_PTE_TRANSENTRY  *p_pteentry,
                                addr_t              ulVirtualAddr,
                                addr_t              ulPhysicalAddr,
                                addr_t              ulFlag)
{
    UINT8   ucACC, ucC, ucET;
    
    if (sparcMmuFlags2Attr(ulFlag, &ucACC,
                           &ucC,   &ucET) != ERROR_NONE) {              /*  无效的映射关系              */
        return;
    }

    *p_pteentry = (LW_PTE_TRANSENTRY)sparcMmuBuildPteEntry((UINT32)ulPhysicalAddr,
                                                           ucACC, ucC, ucET);
                                                        
#if LW_CFG_CACHE_EN > 0
    sparcCacheFlush(DATA_CACHE, (PVOID)p_pteentry, sizeof(LW_PTE_TRANSENTRY));
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
}
/*********************************************************************************************************
** 函数名称: sparcMmuInvTLB
** 功能描述: 无效当前 CPU TLB
** 输　入  : pmmuctx        mmu 上下文
**           ulPageAddr     页面虚拟地址
**           ulPageNum      页面个数
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  sparcMmuInvTLB (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulPageAddr, ULONG  ulPageNum)
{
    if (ulPageNum > 0) {
        sparcMmuInvalidateTLB();                                        /*  全部清除 TLB                */
    }
}
/*********************************************************************************************************
** 函数名称: sparcMmuInit
** 功能描述: MMU 系统初始化
** 输　入  : pmmuop            MMU 操作函数集
**           pcMachineName     使用的机器名称
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
** 注  意  : LEON 多核 SPARC 处理器不需要 LW_VMM_MMU_FLUSH_TLB_MP 操作.
*********************************************************************************************************/
VOID  sparcMmuInit (LW_MMU_OP  *pmmuop, CPCHAR  pcMachineName)
{
    pmmuop->MMUOP_ulOption = 0ul;                                       /* No LW_VMM_MMU_FLUSH_TLB_MP   */

    pmmuop->MMUOP_pfuncMemInit    = sparcMmuMemInit;
    pmmuop->MMUOP_pfuncGlobalInit = sparcMmuGlobalInit;
    
    pmmuop->MMUOP_pfuncPGDAlloc = sparcMmuPgdAlloc;
    pmmuop->MMUOP_pfuncPGDFree  = sparcMmuPgdFree;
    pmmuop->MMUOP_pfuncPMDAlloc = sparcMmuPmdAlloc;
    pmmuop->MMUOP_pfuncPMDFree  = sparcMmuPmdFree;
    pmmuop->MMUOP_pfuncPTEAlloc = sparcMmuPteAlloc;
    pmmuop->MMUOP_pfuncPTEFree  = sparcMmuPteFree;
    
    pmmuop->MMUOP_pfuncPGDIsOk = sparcMmuPgdIsOk;
    pmmuop->MMUOP_pfuncPMDIsOk = sparcMmuPmdIsOk;
    pmmuop->MMUOP_pfuncPTEIsOk = sparcMmuPteIsOk;
    
    pmmuop->MMUOP_pfuncPGDOffset = sparcMmuPgdOffset;
    pmmuop->MMUOP_pfuncPMDOffset = sparcMmuPmdOffset;
    pmmuop->MMUOP_pfuncPTEOffset = sparcMmuPteOffset;
    
    pmmuop->MMUOP_pfuncPTEPhysGet = sparcMmuPtePhysGet;
    
    pmmuop->MMUOP_pfuncFlagGet = sparcMmuFlagGet;
    pmmuop->MMUOP_pfuncFlagSet = sparcMmuFlagSet;
    
    pmmuop->MMUOP_pfuncMakeTrans     = sparcMmuMakeTrans;
    pmmuop->MMUOP_pfuncMakeCurCtx    = sparcMmuMakeCurCtx;
    pmmuop->MMUOP_pfuncInvalidateTLB = sparcMmuInvTLB;
    pmmuop->MMUOP_pfuncSetEnable     = sparcMmuEnable;
    pmmuop->MMUOP_pfuncSetDisable    = sparcMmuDisable;
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
