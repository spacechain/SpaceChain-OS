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
** 文   件   名: ppcMmu.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 01 月 14 日
**
** 描        述: PowerPC 体系构架 MMU 驱动.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "./ppcMmuHashPageTbl.h"
/*********************************************************************************************************
  一级描述符类型定义
*********************************************************************************************************/
#define COARSE_TBASE        (1)                                         /*  二级粗粒度页表基地址        */
/*********************************************************************************************************
  二级描述符类型定义
*********************************************************************************************************/
#define FAIL_DESC           (0)                                         /*  变换失效                    */
#define SMALLPAGE_DESC      (2)                                         /*  小页基地址                  */
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static LW_OBJECT_HANDLE     _G_hPGDPartition;                           /*  系统目前仅使用一个 PGD      */
static LW_OBJECT_HANDLE     _G_hPTEPartition;                           /*  PTE 缓冲区                  */
static PLW_MMU_CONTEXT      _G_pMmuContext[LW_CFG_MAX_PROCESSORS];      /*  上下文                      */
static UINT                 _G_uiTlbNr;                                 /*  TLB 数目                    */
static UINT8                _G_ucWIM4CacheBuffer;
static UINT8                _G_ucWIM4Cache;
/*********************************************************************************************************
  访问权限 PP
*********************************************************************************************************/
#define PP_RW               (2)
#define PP_RO               (1)
#define PP_NORW             (0)
/*********************************************************************************************************
  WIMG
*********************************************************************************************************/
#define G_BIT               (1 << 0)
#define M_BIT               (1 << 1)
#define I_BIT               (1 << 2)
#define W_BIT               (1 << 3)

#define WIM_MASK            (W_BIT | I_BIT | M_BIT)
/*********************************************************************************************************
    OS Two-Level page table:

        L1 page table
                             L2 page table
        +-----------+                                Phy page
        | PGD ENTRY | -----> +-----------+
        +-----------+        | PTE ENTRY | -----> +-----------+
        |           |        +-----------+        |           |
        |           |        |           |        |           |
        |   4096    |        |    256    |        |           |
        |           |        |           |        |           |
        |           |        +-----------+        +-----------+
        +-----------+

    L1 page table size: 4GB / 1MB * sizeof(PGD ENTRY) = 4096 * 4 = 16 KB
    L2 page table size: 1MB / sizeof(PAGE) * size(PTE ENTRY) = 256 * 4 = 1 KB
*********************************************************************************************************/
/*********************************************************************************************************
  外部接口声明
*********************************************************************************************************/
extern VOID  ppcMmuInvalidateTLBNr(UINT  uiTlbNr);
extern VOID  ppcMmuInvalidateTLBEA(UINT32  uiEffectiveAddr);
extern VOID  ppcMmuEnable(VOID);
extern VOID  ppcMmuDisable(VOID);
/*********************************************************************************************************
** 函数名称: ppcMmuInvalidateTLBNr
** 功能描述: 无效所有的 TBL
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static  VOID  ppcMmuInvalidateTLB (VOID)
{
    ppcMmuInvalidateTLBNr(_G_uiTlbNr);
}
/*********************************************************************************************************
** 函数名称: ppcMmuFlags2Attr
** 功能描述: 根据 SylixOS 权限标志, 生成 MPC32 MMU 权限标志
** 输　入  : ulFlag                  内存访问权限
**           pucPP                   访问权限
**           pucWIMG                 CACHE 控制参数
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ppcMmuFlags2Attr (ULONG  ulFlag, UINT8  *pucPP, UINT8  *pucWIMG,  UINT8  *pucExec)
{
    UINT8  ucWIMG;

    if (!(ulFlag & LW_VMM_FLAG_VALID)) {                                /*  无效的映射关系              */
        return  (PX_ERROR);
    }

    if (ulFlag & LW_VMM_FLAG_ACCESS) {                                  /*  是否拥有访问权限            */
        if (ulFlag & LW_VMM_FLAG_WRITABLE) {                            /*  是否可写                    */
            *pucPP = PP_RW;

        } else {
            *pucPP = PP_RO;
        }
    } else {
        *pucPP = PP_NORW;
    }

    if ((ulFlag & LW_VMM_FLAG_CACHEABLE) &&
        (ulFlag & LW_VMM_FLAG_BUFFERABLE)) {                            /*  CACHE 与 BUFFER 控制        */
        ucWIMG = _G_ucWIM4CacheBuffer;

    } else if (ulFlag & LW_VMM_FLAG_CACHEABLE) {
        ucWIMG = _G_ucWIM4Cache;

    } else {
        ucWIMG = I_BIT | M_BIT;
    }

    *pucWIMG = ucWIMG;

    if (ulFlag & LW_VMM_FLAG_EXECABLE) {
        *pucExec = 1;
    } else {
        *pucExec = 0;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ppcMmuAttr2Flags
** 功能描述: 根据 MPC32 MMU 权限标志, 生成 SylixOS 权限标志
** 输　入  : ucPP                    访问权限
**           ucWIMG                  CACHE 控制参数
**           pulFlag                 内存访问权限
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ppcMmuAttr2Flags (UINT8  ucPP, UINT8  ucWIMG, UINT8  ucExec, ULONG *pulFlag)
{
    *pulFlag = LW_VMM_FLAG_VALID;

    switch (ucPP) {

    case PP_RW:
        *pulFlag |= LW_VMM_FLAG_ACCESS;
        *pulFlag |= LW_VMM_FLAG_WRITABLE;
        break;

    case PP_RO:
        *pulFlag |= LW_VMM_FLAG_ACCESS;
        break;

    case PP_NORW:
    default:
        break;
    }

    if ((ucWIMG & WIM_MASK) == _G_ucWIM4CacheBuffer) {
        *pulFlag |= LW_VMM_FLAG_CACHEABLE | LW_VMM_FLAG_BUFFERABLE;

    } else if ((ucWIMG & WIM_MASK) == _G_ucWIM4CacheBuffer) {
        *pulFlag |= LW_VMM_FLAG_CACHEABLE;
    }

    *pulFlag |= LW_VMM_FLAG_GUARDED;

    if (ucExec) {
        *pulFlag |= LW_VMM_FLAG_EXECABLE;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ppcMmuBuildPgdesc
** 功能描述: 生成一个一级描述符 (PGD 描述符)
** 输　入  : uiBaseAddr              基地址     (段基地址、二级页表基地址)
**           ucPP                    访问权限
**           ucWIMG                  CACHE 控制参数
**           ucType                  一级描述符类型
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  ppcMmuBuildPgdesc (UINT32  uiBaseAddr,
                                             UINT8   ucPP,
                                             UINT8   ucWIMG,
                                             UINT8   ucExec,
                                             UINT8   ucType)
{
    LW_PGD_TRANSENTRY   uiPgdDescriptor;

    switch (ucType) {

    case COARSE_TBASE:                                                  /*  粗粒度二级页表描述符        */
        uiPgdDescriptor = (uiBaseAddr & 0xFFFFFC00)
                         | ucType;
        break;

    default:
        uiPgdDescriptor = 0;                                            /*  访问失效                    */
        break;
    }

    return  (uiPgdDescriptor);
}
/*********************************************************************************************************
** 函数名称: ppcMmuBuildPtentry
** 功能描述: 生成一个二级描述符 (PTE 描述符)
** 输　入  : uiBaseAddr              基地址     (页地址)
**           ucPP                    访问权限
**           ucWIMG                  CACHE 控制参数
**           ucType                  二级描述符类型
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  ppcMmuBuildPtentry (UINT32  uiBaseAddr,
                                              UINT8   ucPP,
                                              UINT8   ucWIMG,
                                              UINT8   ucExec,
                                              UINT8   ucType)
{
    LW_PTE_TRANSENTRY   uiPteDescriptor;

    switch (ucType) {

    case SMALLPAGE_DESC:                                                /*  小页描述符                  */
        /*
         * 这里把 R 和 C 位设置为 0，因为设置完 PTE word1 后，会无效 TLB，
         * 所以不用担心 PTE 与对应的 TLB 不一致的问题
         */
        uiPteDescriptor.PTE_bRef        = 0;
        uiPteDescriptor.PTE_bChange     = 0;
        uiPteDescriptor.PTE_ucPP        = ucPP;
        uiPteDescriptor.PTE_ucWIMG      = ucWIMG;
        uiPteDescriptor.PTE_uiRPN       = uiBaseAddr >> LW_CFG_VMM_PAGE_SHIFT;
        uiPteDescriptor.PTE_ucReserved1 = ucType;
        uiPteDescriptor.PTE_bReserved2  = ucExec;
        break;

    default:
        uiPteDescriptor.PTE_uiValue = 0;                                /*  访问失效                    */
        break;
    }

    return  (uiPteDescriptor);
}
/*********************************************************************************************************
** 函数名称: ppcMmuMemInit
** 功能描述: 初始化 MMU 页表内存区
** 输　入  : pmmuctx        mmu 上下文
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
** 注  意  : 一级页表基地址需要保持 16 KByte 对齐, 单条目映射 1 MByte 空间.
             二级页表基地址需要保持  1 KByte 对齐, 单条目映射 4 KByte 空间.
*********************************************************************************************************/
static INT  ppcMmuMemInit (PLW_MMU_CONTEXT  pmmuctx)
{
#define PGD_BLOCK_SIZE  (16 * LW_CFG_KB_SIZE)
#define PTE_BLOCK_SIZE  ( 1 * LW_CFG_KB_SIZE)

    PVOID   pvPgdTable;
    PVOID   pvPteTable;

    ULONG   ulPgdNum = bspMmuPgdMaxNum();
    ULONG   ulPteNum = bspMmuPteMaxNum();

    INT     iError;

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

    iError = ppcMmuHashPageTblInit(MMU_PTE_MIN_SIZE_128M);
    if (iError != ERROR_NONE) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not init hashed page table.\r\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ppcMmuGlobalInit
** 功能描述: 调用 BSP 对 MMU 初始化
** 输　入  : pcMachineName     使用的机器名称
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ppcMmuGlobalInit (CPCHAR  pcMachineName)
{
    if (LW_CPU_GET_CUR_ID() == 0) {                                     /*  仅 Core0 复位 Cache         */
        archCacheReset(pcMachineName);                                  /*  复位 Cache                  */
    }

    ppcMmuInvalidateTLB();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ppcMmuPgdOffset
** 功能描述: 通过虚拟地址计算 PGD 项
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址
** 输　出  : 对应的 PGD 表项地址
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_PGD_TRANSENTRY *ppcMmuPgdOffset (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry = pmmuctx->MMUCTX_pgdEntry;

    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry
               | ((ulAddr >> LW_CFG_VMM_PGD_SHIFT) << 2));              /*  获得一级页表描述符地址      */

    return  (p_pgdentry);
}
/*********************************************************************************************************
** 函数名称: ppcMmuPmdOffset
** 功能描述: 通过虚拟地址计算 PMD 项
** 输　入  : p_pgdentry     pgd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 对应的 PMD 表项地址
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_PMD_TRANSENTRY *ppcMmuPmdOffset (LW_PGD_TRANSENTRY  *p_pgdentry, addr_t  ulAddr)
{
    return  ((LW_PMD_TRANSENTRY *)p_pgdentry);                          /*  无 PMD 项                   */
}
/*********************************************************************************************************
** 函数名称: ppcMmuPteOffset
** 功能描述: 通过虚拟地址计算 PMD 项
** 输　入  : p_pgdentry     pgd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 对应的 PMD 表项地址
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static  LW_PTE_TRANSENTRY *ppcMmuPteOffset (LW_PMD_TRANSENTRY  *p_pmdentry, addr_t  ulAddr)
{
    REGISTER LW_PTE_TRANSENTRY  *p_pteentry;
    REGISTER UINT32              uiTemp;

    uiTemp = (UINT32)(*p_pmdentry);                                     /*  获得二级页表描述符          */

    p_pteentry = (LW_PTE_TRANSENTRY *)(uiTemp & (~(LW_CFG_KB_SIZE - 1)));
                                                                        /*  获得粗粒度二级页表基地址    */
    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry
               | ((ulAddr >> 10) & 0x3FC));                             /*  获得对应虚拟地址页表描述符  */
                                                                        /*  地址                        */
    return  (p_pteentry);
}
/*********************************************************************************************************
** 函数名称: ppcMmuPgdIsOk
** 功能描述: 判断 PGD 项的描述符是否正确
** 输　入  : pgdentry       PGD 项描述符
** 输　出  : 是否正确
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static BOOL  ppcMmuPgdIsOk (LW_PGD_TRANSENTRY  pgdentry)
{
    return  (((pgdentry & 0x03) == COARSE_TBASE) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: ppcMmuPteIsOk
** 功能描述: 判断 PTE 项的描述符是否正确
** 输　入  : pteentry       PTE 项描述符
** 输　出  : 是否正确
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static BOOL  ppcMmuPteIsOk (LW_PTE_TRANSENTRY  pteentry)
{
    return  (((pteentry.PTE_ucReserved1 & 0x03) == SMALLPAGE_DESC) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: ppcMmuPgdAlloc
** 功能描述: 分配 PGD 项
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址 (参数 0 即偏移量为 0 , 需要返回页表基地址)
** 输　出  : 分配 PGD 地址
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_PGD_TRANSENTRY *ppcMmuPgdAlloc (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry = (LW_PGD_TRANSENTRY *)API_PartitionGet(_G_hPGDPartition);

    if (!p_pgdentry) {
        return  (LW_NULL);
    }

    lib_bzero(p_pgdentry, PGD_BLOCK_SIZE);                              /*  新的 PGD 无有效的页表项     */

    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry
               | ((ulAddr >> LW_CFG_VMM_PGD_SHIFT) << 2));              /*  pgd offset                  */

    return  (p_pgdentry);
}
/*********************************************************************************************************
** 函数名称: ppcMmuPgdFree
** 功能描述: 释放 PGD 项
** 输　入  : p_pgdentry     pgd 入口地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  ppcMmuPgdFree (LW_PGD_TRANSENTRY  *p_pgdentry)
{
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry & (~((16 * LW_CFG_KB_SIZE) - 1)));

    API_PartitionPut(_G_hPGDPartition, (PVOID)p_pgdentry);
}
/*********************************************************************************************************
** 函数名称: ppcMmuPmdAlloc
** 功能描述: 分配 PMD 项
** 输　入  : pmmuctx        mmu 上下文
**           p_pgdentry     pgd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 分配 PMD 地址
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_PMD_TRANSENTRY *ppcMmuPmdAlloc (PLW_MMU_CONTEXT     pmmuctx,
                                          LW_PGD_TRANSENTRY  *p_pgdentry,
                                          addr_t              ulAddr)
{
    return  ((LW_PMD_TRANSENTRY *)p_pgdentry);
}
/*********************************************************************************************************
** 函数名称: ppcMmuPmdFree
** 功能描述: 释放 PMD 项
** 输　入  : p_pmdentry     pmd 入口地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  ppcMmuPmdFree (LW_PMD_TRANSENTRY  *p_pmdentry)
{
    (VOID)p_pmdentry;
}
/*********************************************************************************************************
** 函数名称: ppcMmuPteAlloc
** 功能描述: 分配 PTE 项
** 输　入  : pmmuctx        mmu 上下文
**           p_pmdentry     pmd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 分配 PTE 地址
** 全局变量:
** 调用模块: SylixOS 映射完毕会自动清快表, 所以这里不用清除快表.
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  *ppcMmuPteAlloc (PLW_MMU_CONTEXT     pmmuctx,
                                           LW_PMD_TRANSENTRY  *p_pmdentry,
                                           addr_t              ulAddr)
{
    LW_PTE_TRANSENTRY  *p_pteentry = (LW_PTE_TRANSENTRY *)API_PartitionGet(_G_hPTEPartition);

    if (!p_pteentry) {
        return  (LW_NULL);
    }

    lib_bzero(p_pteentry, PTE_BLOCK_SIZE);

    *p_pmdentry = (LW_PMD_TRANSENTRY)ppcMmuBuildPgdesc((UINT32)p_pteentry,
                                                       PP_RW,
                                                       0,
                                                       1,
                                                       COARSE_TBASE);   /*  设置二级页表基地址          */

    return  (ppcMmuPteOffset(p_pmdentry, ulAddr));
}
/*********************************************************************************************************
** 函数名称: ppcMmuPteFree
** 功能描述: 释放 PTE 项
** 输　入  : p_pteentry     pte 入口地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  ppcMmuPteFree (LW_PTE_TRANSENTRY  *p_pteentry)
{
    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry & (~(LW_CFG_KB_SIZE - 1)));

    API_PartitionPut(_G_hPTEPartition, (PVOID)p_pteentry);
}
/*********************************************************************************************************
** 函数名称: ppcMmuPtePhysGet
** 功能描述: 通过 PTE 表项, 查询物理地址
** 输　入  : pteentry           pte 表项
**           pulPhysicalAddr    获得的物理地址
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ppcMmuPtePhysGet (LW_PTE_TRANSENTRY  pteentry, addr_t  *pulPhysicalAddr)
{
    *pulPhysicalAddr = (addr_t)(pteentry.PTE_uiRPN << LW_CFG_VMM_PAGE_SHIFT);   /*  获得物理地址        */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ppcMmuFlagGet
** 功能描述: 获得指定虚拟地址的 SylixOS 权限标志
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址
** 输　出  : SylixOS 权限标志
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ULONG  ppcMmuFlagGet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    LW_PGD_TRANSENTRY  *p_pgdentry = ppcMmuPgdOffset(pmmuctx, ulAddr);
    INT                 iDescType;
    UINT8               ucPP;                                           /*  存储权限                    */
    UINT8               ucWIMG;                                         /*  CACHE 与缓冲区控制          */
    UINT8               ucExec;
    ULONG               ulFlag;

    iDescType = (*p_pgdentry) & 0x03;                                   /*  获得一级页表类型            */
    if (iDescType == COARSE_TBASE) {                                    /*  基于粗粒度二级页表映射      */
        LW_PTE_TRANSENTRY  *p_pteentry = ppcMmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry, ulAddr);

        if (ppcMmuPteIsOk(*p_pteentry)) {
            ucPP    = p_pteentry->PTE_ucPP;
            ucWIMG  = p_pteentry->PTE_ucWIMG;
            ucExec  = p_pteentry->PTE_bReserved2;

            ppcMmuAttr2Flags(ucPP, ucWIMG, ucExec, &ulFlag);
            return  (ulFlag);

        } else {
            return  (LW_VMM_FLAG_UNVALID);
        }
    } else {
        return  (LW_VMM_FLAG_UNVALID);
    }
}
/*********************************************************************************************************
** 函数名称: ppcMmuFlagSet
** 功能描述: 设置指定虚拟地址的 flag 标志
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址
**           ulFlag         flag 标志
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
** 注  意  : 由于改变了单条目的页表, VMM 本身在这个函数并不无效快表, 所以这里需要无效指定条目的快表.
*********************************************************************************************************/
static INT  ppcMmuFlagSet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr, ULONG  ulFlag)
{
    LW_PGD_TRANSENTRY  *p_pgdentry = ppcMmuPgdOffset(pmmuctx, ulAddr);
    INT                 iDescType;
    UINT8               ucPP;                                           /*  存储权限                    */
    UINT8               ucWIMG;                                         /*  CACHE 与缓冲区控制          */
    UINT8               ucExec;
    UINT8               ucType;

    if (ulFlag & LW_VMM_FLAG_ACCESS) {
        ucType = SMALLPAGE_DESC;

    } else {
        ucType = FAIL_DESC;                                             /*  访问将失效                  */
    }

    if (ppcMmuFlags2Attr(ulFlag, &ucPP, &ucWIMG, &ucExec) < 0) {        /*  无效的映射关系              */
        return  (PX_ERROR);
    }

    iDescType = (*p_pgdentry) & 0x03;                                   /*  获得一级页表类型            */
    if (iDescType == COARSE_TBASE) {                                    /*  基于粗粒度二级页表映射      */
        REGISTER LW_PTE_TRANSENTRY  *p_pteentry = ppcMmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                                                     ulAddr);
        if (ppcMmuPteIsOk(*p_pteentry)) {
            addr_t   ulPhysicalAddr = (addr_t)(p_pteentry->PTE_uiRPN << LW_CFG_VMM_PAGE_SHIFT);

            *p_pteentry = (LW_PTE_TRANSENTRY)ppcMmuBuildPtentry((UINT32)ulPhysicalAddr,
                                                                ucPP,
                                                                ucWIMG,
                                                                ucExec,
                                                                ucType);
            ppcMmuHashPageTblFlagSet(ulAddr,
                                     p_pteentry->PTE_uiValue);

            return  (ERROR_NONE);
        }
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: ppcMmuMakeTrans
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
static VOID  ppcMmuMakeTrans (PLW_MMU_CONTEXT     pmmuctx,
                              LW_PTE_TRANSENTRY  *p_pteentry,
                              addr_t              ulVirtualAddr,
                              addr_t              ulPhysicalAddr,
                              addr_t              ulFlag)
{
    UINT8   ucPP;                                                       /*  存储权限                    */
    UINT8   ucWIMG;                                                     /*  CACHE 与缓冲区控制          */
    UINT8   ucExec;
    UINT8   ucType;

    if (ulFlag & LW_VMM_FLAG_ACCESS) {
        ucType = SMALLPAGE_DESC;

    } else {
        ucType = FAIL_DESC;                                             /*  访问将失效                  */
    }

    if (ppcMmuFlags2Attr(ulFlag, &ucPP, &ucWIMG, &ucExec) < 0) {        /*  无效的映射关系              */
        return;
    }

    *p_pteentry = (LW_PTE_TRANSENTRY)ppcMmuBuildPtentry((UINT32)ulPhysicalAddr,
                                                        ucPP,
                                                        ucWIMG,
                                                        ucExec,
                                                        ucType);

    ppcMmuHashPageTblMakeTrans(ulVirtualAddr,
                               p_pteentry->PTE_uiValue);
}
/*********************************************************************************************************
** 函数名称: ppcMmuMakeCurCtx
** 功能描述: 设置 MMU 当前上下文
** 输　入  : pmmuctx        mmu 上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  ppcMmuMakeCurCtx (PLW_MMU_CONTEXT  pmmuctx)
{
    ULONG   ulCPUId = LW_CPU_GET_CUR_ID();

    _G_pMmuContext[ulCPUId] = pmmuctx;
}
/*********************************************************************************************************
** 函数名称: ppcMmuInvTLB
** 功能描述: 无效当前 CPU TLB
** 输　入  : pmmuctx        mmu 上下文
**           ulPageAddr     页面虚拟地址
**           ulPageNum      页面个数
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  ppcMmuInvTLB (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulPageAddr, ULONG  ulPageNum)
{
    ULONG   i;

    if (ulPageNum > (_G_uiTlbNr >> 1)) {
        ppcMmuInvalidateTLB();                                          /*  全部清除 TLB                */

    } else {
        for (i = 0; i < ulPageNum; i++) {
            ppcMmuInvalidateTLBEA((UINT32)ulPageAddr);                  /*  逐个页面清除 TLB            */
            ulPageAddr += LW_CFG_VMM_PAGE_SIZE;
        }
    }
}
/*********************************************************************************************************
** 函数名称: ppcMmuPteMissHandle
** 功能描述: 处理 PTE 匹配失败异常
** 输　入  : ulAddr        异常处理
** 输　出  : 终止类型
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ULONG  ppcMmuPteMissHandle (addr_t  ulAddr)
{
    ULONG               ulCPUId = LW_CPU_GET_CUR_ID();
    LW_PGD_TRANSENTRY  *p_pgdentry;
    INT                 iDescType;

    if (!_G_pMmuContext[ulCPUId]) {
        return  (LW_VMM_ABORT_TYPE_MAP);
    }

    p_pgdentry = ppcMmuPgdOffset(_G_pMmuContext[ulCPUId], ulAddr);
    iDescType  = (*p_pgdentry) & 0x03;                                  /*  获得一级页表类型            */
    if (iDescType == COARSE_TBASE) {                                    /*  基于粗粒度二级页表映射      */
        LW_PTE_TRANSENTRY  *p_pteentry = ppcMmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry, ulAddr);

        if (ppcMmuPteIsOk(*p_pteentry)) {
            ppcMmuHashPageTblPteMiss(ulAddr, p_pteentry->PTE_uiValue);
            return  (0);

        } else {
            return  (LW_VMM_ABORT_TYPE_MAP);
        }
    } else {
        return  (LW_VMM_ABORT_TYPE_MAP);
    }
}
/*********************************************************************************************************
** 函数名称: ppcMmuPtePreLoad
** 功能描述: PTE 预加载
** 输　入  : ulAddr        数据访问地址
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  ppcMmuPtePreLoad (addr_t  ulAddr)
{
    ULONG               ulCPUId = LW_CPU_GET_CUR_ID();
    LW_PGD_TRANSENTRY  *p_pgdentry;
    INT                 iDescType;

    if (!_G_pMmuContext[ulCPUId]) {
        return  (PX_ERROR);
    }

    p_pgdentry = ppcMmuPgdOffset(_G_pMmuContext[ulCPUId], ulAddr);
    iDescType  = (*p_pgdentry) & 0x03;                                  /*  获得一级页表类型            */
    if (iDescType == COARSE_TBASE) {                                    /*  基于粗粒度二级页表映射      */
        LW_PTE_TRANSENTRY  *p_pteentry = ppcMmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry, ulAddr);

        if (ppcMmuPteIsOk(*p_pteentry)) {
            ppcMmuHashPageTblPtePreLoad(ulAddr, p_pteentry->PTE_uiValue);
            return  (ERROR_NONE);

        } else {
            return  (PX_ERROR);
        }
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: bspMmuTlbSize
** 功能描述: 获得 TLB 的数目
** 输　入  : NONE
** 输　出  : TLB 的数目
** 全局变量:
** 调用模块:
**
*********************************************************************************************************/
LW_WEAK ULONG  bspMmuTlbSize (VOID)
{
    return  (128);                                                      /*  128 适合 750 MPC83XX 机器   */
}
/*********************************************************************************************************
** 函数名称: ppcMmuInit
** 功能描述: MMU 系统初始化
** 输　入  : pmmuop            MMU 操作函数集
**           pcMachineName     使用的机器名称
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  ppcMmuInit (LW_MMU_OP *pmmuop, CPCHAR  pcMachineName)
{
    _G_uiTlbNr           = bspMmuTlbSize();                             /*  获得 TLB 的数目             */
    _G_ucWIM4CacheBuffer = M_BIT;
    _G_ucWIM4Cache       = W_BIT | M_BIT;

    pmmuop->MMUOP_ulOption           = 0ul;                             /*  tlbsync 指令会自动多核同步  */
    pmmuop->MMUOP_pfuncMemInit       = ppcMmuMemInit;
    pmmuop->MMUOP_pfuncGlobalInit    = ppcMmuGlobalInit;

    pmmuop->MMUOP_pfuncPGDAlloc      = ppcMmuPgdAlloc;
    pmmuop->MMUOP_pfuncPGDFree       = ppcMmuPgdFree;
    pmmuop->MMUOP_pfuncPMDAlloc      = ppcMmuPmdAlloc;
    pmmuop->MMUOP_pfuncPMDFree       = ppcMmuPmdFree;
    pmmuop->MMUOP_pfuncPTEAlloc      = ppcMmuPteAlloc;
    pmmuop->MMUOP_pfuncPTEFree       = ppcMmuPteFree;

    pmmuop->MMUOP_pfuncPGDIsOk       = ppcMmuPgdIsOk;
    pmmuop->MMUOP_pfuncPMDIsOk       = ppcMmuPgdIsOk;
    pmmuop->MMUOP_pfuncPTEIsOk       = ppcMmuPteIsOk;

    pmmuop->MMUOP_pfuncPGDOffset     = ppcMmuPgdOffset;
    pmmuop->MMUOP_pfuncPMDOffset     = ppcMmuPmdOffset;
    pmmuop->MMUOP_pfuncPTEOffset     = ppcMmuPteOffset;

    pmmuop->MMUOP_pfuncPTEPhysGet    = ppcMmuPtePhysGet;

    pmmuop->MMUOP_pfuncFlagGet       = ppcMmuFlagGet;
    pmmuop->MMUOP_pfuncFlagSet       = ppcMmuFlagSet;

    pmmuop->MMUOP_pfuncMakeTrans     = ppcMmuMakeTrans;
    pmmuop->MMUOP_pfuncMakeCurCtx    = ppcMmuMakeCurCtx;
    pmmuop->MMUOP_pfuncInvalidateTLB = ppcMmuInvTLB;
    pmmuop->MMUOP_pfuncSetEnable     = ppcMmuEnable;
    pmmuop->MMUOP_pfuncSetDisable    = ppcMmuDisable;
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
