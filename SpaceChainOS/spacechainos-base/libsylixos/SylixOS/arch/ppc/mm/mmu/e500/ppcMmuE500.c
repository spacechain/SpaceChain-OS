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
** 文   件   名: ppcE500Mmu.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 05 月 04 日
**
** 描        述: PowerPC E500 体系构架 MMU 驱动.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "arch/ppc/arch_e500.h"
#include "arch/ppc/common/e500/ppcSprE500.h"
#include "./ppcMmuE500Reg.h"
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static LW_OBJECT_HANDLE     _G_hPGDPartition = LW_HANDLE_INVALID;       /*  系统目前仅使用一个 PGD      */
static addr_t               _G_ulPTETable    = 0;                       /*  PTE 表                      */
static UINT                 _G_uiTlbSize     = 0;                       /*  TLB 数组大小                */
static BOOL                 _G_bMas2MBit     = LW_FALSE;                /*  多核一致性                  */
static BOOL                 _G_bHasMAS7      = LW_FALSE;                /*  是否有 MAS7 寄存器          */
static BOOL                 _G_bHasHID1      = LW_FALSE;                /*  是否有 HID1 寄存器          */
/*********************************************************************************************************
  定义
*********************************************************************************************************/
#define MMU_MAS2_M          _G_bMas2MBit                                /*  是否多核一致性              */
#define MMU_MAS4_X0D        0                                           /*  Implement depend page attr  */
#define MMU_MAS4_X1D        0                                           /*  Implement depend page attr  */
/*********************************************************************************************************
  外部接口声明
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
extern INT     ppcCacheDataUpdate(PVOID  pvAdrs, size_t  stBytes, BOOL  bInv);
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

extern VOID    ppcE500MmuInvalidateTLB(VOID);
extern VOID    ppcE500MmuInvalidateTLBEA(addr_t  ulAddr);

extern VOID    archE500InstructionStorageExceptionHandle(addr_t  ulRetAddr);
extern VOID    archE500DataStorageExceptionHandle(addr_t  ulRetAddr);
/*********************************************************************************************************
** 函数名称: archE500MmuDataTlbErrorHandle
** 功能描述: 数据访问 TLB 错误处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archE500MmuDataTlbErrorHandle (addr_t  ulRetAddr)
{
    addr_t              ulMissAddr = ppcE500GetDEAR();
    addr_t              uiEPN      = ulMissAddr >> LW_CFG_VMM_PAGE_SHIFT;
    LW_PTE_TRANSENTRY  *p_pteentry = (LW_PTE_TRANSENTRY *)(_G_ulPTETable + \
                                     uiEPN * sizeof(LW_PTE_TRANSENTRY));
    LW_PTE_TRANSENTRY   pteentry   = *p_pteentry;
    MAS2_REG            uiMAS2;

    if (pteentry.MAS3_bValid) {
        uiMAS2.MAS2_uiValue       = ppcE500MmuGetMAS2();
        uiMAS2.MAS2_bWT           = pteentry.MAS3_bWT;
        uiMAS2.MAS2_bUnCache      = pteentry.MAS3_bUnCache;
        uiMAS2.MAS2_bMemCoh       = pteentry.MAS3_bMemCoh;
        uiMAS2.MAS2_bGuarded      = LW_FALSE;
        uiMAS2.MAS2_bLittleEndian = LW_FALSE;

        ppcE500MmuSetMAS2(uiMAS2.MAS2_uiValue);
        ppcE500MmuSetMAS3(pteentry.MAS3_uiValue);

        if (_G_bHasMAS7) {
            ppcE500MmuSetMAS7(0);
        }

        PPC_EXEC_INS("ISYNC");
        PPC_EXEC_INS("SYNC");
        PPC_EXEC_INS("TLBWE");
        PPC_EXEC_INS("ISYNC");

    } else {
        archE500DataStorageExceptionHandle(ulRetAddr);
    }
}
/*********************************************************************************************************
** 函数名称: archE500MmuInstructionTlbErrorHandle
** 功能描述: 指令执行 TLB 错误处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archE500MmuInstructionTlbErrorHandle (addr_t  ulRetAddr)
{
    addr_t              ulMissAddr = ulRetAddr;
    addr_t              uiEPN      = ulMissAddr >> LW_CFG_VMM_PAGE_SHIFT;
    LW_PTE_TRANSENTRY  *p_pteentry = (LW_PTE_TRANSENTRY *)(_G_ulPTETable + \
                                     uiEPN * sizeof(LW_PTE_TRANSENTRY));
    LW_PTE_TRANSENTRY   pteentry   = *p_pteentry;
    MAS2_REG            uiMAS2;

    if (pteentry.MAS3_bValid) {
        uiMAS2.MAS2_uiValue       = ppcE500MmuGetMAS2();
        uiMAS2.MAS2_bWT           = pteentry.MAS3_bWT;
        uiMAS2.MAS2_bUnCache      = pteentry.MAS3_bUnCache;
        uiMAS2.MAS2_bMemCoh       = pteentry.MAS3_bMemCoh;
        uiMAS2.MAS2_bGuarded      = LW_FALSE;
        uiMAS2.MAS2_bLittleEndian = LW_FALSE;

        ppcE500MmuSetMAS2(uiMAS2.MAS2_uiValue);
        ppcE500MmuSetMAS3(pteentry.MAS3_uiValue);

        if (_G_bHasMAS7) {
            ppcE500MmuSetMAS7(0);
        }

        PPC_EXEC_INS("ISYNC");
        PPC_EXEC_INS("SYNC");
        PPC_EXEC_INS("TLBWE");
        PPC_EXEC_INS("ISYNC");

    } else {
        archE500InstructionStorageExceptionHandle(ulRetAddr);
    }
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuDataTlbPreLoad
** 功能描述: 数据访问 TLB 预加载
** 输　入  : ulAddr        数据访问地址
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  ppcE500MmuDataTlbPreLoad (addr_t  ulAddr)
{
    addr_t              uiEPN      = ulAddr >> LW_CFG_VMM_PAGE_SHIFT;
    LW_PTE_TRANSENTRY  *p_pteentry = (LW_PTE_TRANSENTRY *)(_G_ulPTETable + \
                                     uiEPN * sizeof(LW_PTE_TRANSENTRY));
    LW_PTE_TRANSENTRY   pteentry   = *p_pteentry;
    MAS2_REG            uiMAS2;

    if (pteentry.MAS3_bValid) {
        uiMAS2.MAS2_uiValue       = 0;
        uiMAS2.MAS2_uiEPN         = uiEPN;
        uiMAS2.MAS2_bX0           = MMU_MAS4_X0D;
        uiMAS2.MAS2_bX1           = MMU_MAS4_X1D;
        uiMAS2.MAS2_bWT           = pteentry.MAS3_bWT;
        uiMAS2.MAS2_bUnCache      = pteentry.MAS3_bUnCache;
        uiMAS2.MAS2_bMemCoh       = pteentry.MAS3_bMemCoh;
        uiMAS2.MAS2_bGuarded      = LW_FALSE;
        uiMAS2.MAS2_bLittleEndian = LW_FALSE;

        ppcE500MmuSetMAS2(uiMAS2.MAS2_uiValue);
        ppcE500MmuSetMAS3(pteentry.MAS3_uiValue);

        if (_G_bHasMAS7) {
            ppcE500MmuSetMAS7(0);
        }

        PPC_EXEC_INS("ISYNC");
        PPC_EXEC_INS("SYNC");
        PPC_EXEC_INS("TLBWE");
        PPC_EXEC_INS("ISYNC");

        return  (ERROR_NONE);

    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuDataStorageAbortType
** 功能描述: 数据访问终止类型
** 输　入  : ulAbortAddr   终止地址
**           bIsWrite      是否存储引发的异常
** 输　出  : 数据访问终止类型
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ULONG  ppcE500MmuDataStorageAbortType (addr_t  ulAbortAddr, BOOL  bIsWrite)
{
    addr_t              uiEPN      = ulAbortAddr >> LW_CFG_VMM_PAGE_SHIFT;
    LW_PTE_TRANSENTRY  *p_pteentry = (LW_PTE_TRANSENTRY *)(_G_ulPTETable + \
                                     uiEPN * sizeof(LW_PTE_TRANSENTRY));
    LW_PTE_TRANSENTRY   pteentry   = *p_pteentry;

    if (pteentry.MAS3_bValid && pteentry.MAS3_uiRPN) {
        if (bIsWrite) {
            if (!pteentry.MAS3_bSuperWrite) {
                return  (LW_VMM_ABORT_TYPE_PERM);
            }
        } else {
            if (!pteentry.MAS3_bSuperRead) {
                return  (LW_VMM_ABORT_TYPE_PERM);
            }
        }
        return  (LW_VMM_ABORT_TYPE_MAP);

    } else {
        return  (LW_VMM_ABORT_TYPE_MAP);
    }
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuInstStorageAbortType
** 功能描述: 指令访问终止类型
** 输　入  : ulAbortAddr   终止地址
** 输　出  : 指令访问终止类型
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ULONG  ppcE500MmuInstStorageAbortType (addr_t  ulAbortAddr)
{
    addr_t              uiEPN      = ulAbortAddr >> LW_CFG_VMM_PAGE_SHIFT;
    LW_PTE_TRANSENTRY  *p_pteentry = (LW_PTE_TRANSENTRY *)(_G_ulPTETable + \
                                     uiEPN * sizeof(LW_PTE_TRANSENTRY));
    LW_PTE_TRANSENTRY   pteentry   = *p_pteentry;

    if (pteentry.MAS3_bValid && pteentry.MAS3_uiRPN) {
        if (!pteentry.MAS3_bSuperExec) {
            return  (LW_VMM_ABORT_TYPE_PERM);
        }
        return  (LW_VMM_ABORT_TYPE_MAP);

    } else {
        return  (LW_VMM_ABORT_TYPE_MAP);
    }
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuEnable
** 功能描述: 使能 MMU
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  ppcE500MmuEnable (VOID)
{
    /*
     * E500 总是使能 MMU
     */
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuDisable
** 功能描述: 禁能 MMU
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  ppcE500MmuDisable (VOID)
{
    /*
     * E500 总是使能 MMU
     */
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuBuildPgdesc
** 功能描述: 生成一个一级描述符 (PGD 描述符)
** 输　入  : uiBaseAddr              二级页表基地址
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  ppcE500MmuBuildPgdesc (UINT32  uiBaseAddr)
{
    return  (uiBaseAddr);                                               /*  一级描述符就是二级页表基地址*/
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuBuildPtentry
** 功能描述: 生成一个二级描述符 (PTE 描述符)
** 输　入  : uiBaseAddr              物理页地址
**           uiFlag
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  ppcE500MmuBuildPtentry (UINT32  uiBaseAddr,
                                                  ULONG   ulFlag)
{
    LW_PTE_TRANSENTRY   uiDescriptor;
    UINT32              uiRPN;

    uiDescriptor.MAS3_uiValue = 0;

    if (ulFlag & LW_VMM_FLAG_ACCESS) {
        uiRPN = uiBaseAddr >> LW_CFG_VMM_PAGE_SHIFT;                    /*  计算 RPN                    */

        uiDescriptor.MAS3_uiRPN = uiRPN;                                /*  填充 RPN                    */

        if (ulFlag & LW_VMM_FLAG_VALID) {
            uiDescriptor.MAS3_bValid = LW_TRUE;                         /*  有效                        */
        }

        uiDescriptor.MAS3_bSuperRead = LW_TRUE;                         /*  可读                        */

        if (ulFlag & LW_VMM_FLAG_WRITABLE) {
            uiDescriptor.MAS3_bSuperWrite = LW_TRUE;                    /*  可写                        */
        }

        if (ulFlag & LW_VMM_FLAG_EXECABLE) {
            uiDescriptor.MAS3_bSuperExec = LW_TRUE;                     /*  可执行                      */
        }

        if (!(ulFlag & LW_VMM_FLAG_CACHEABLE)) {
            uiDescriptor.MAS3_bUnCache = LW_TRUE;                       /*  不可 Cache                  */
        }

        if ((ulFlag  & LW_VMM_FLAG_CACHEABLE) &&
            !(ulFlag & LW_VMM_FLAG_BUFFERABLE)) {
            uiDescriptor.MAS3_bWT = LW_TRUE;                            /*  写穿透                      */
        }

        if (MMU_MAS2_M) {
            uiDescriptor.MAS3_bMemCoh = LW_TRUE;                        /*  多核一致性                  */
        }
    }

    return  (uiDescriptor);
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuMemInit
** 功能描述: 初始化 MMU 页表内存区
** 输　入  : pmmuctx        mmu 上下文
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  ppcE500MmuMemInit (PLW_MMU_CONTEXT  pmmuctx)
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

    pvPteTable = __KHEAP_ALLOC(PTE_TABLE_SIZE);
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

    _G_ulPTETable = (addr_t)pvPteTable;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuGlobalInit
** 功能描述: 调用 BSP 对 MMU 初始化
** 输　入  : pcMachineName     使用的机器名称
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  ppcE500MmuGlobalInit (CPCHAR  pcMachineName)
{
    MMUCFG_REG  uiMMUCFG;
    MAS4_REG    uiMAS4;
    UINT32      uiHID1;

    /*
     * 设置 PID
     */
    uiMMUCFG.MMUCFG_uiValue = ppcE500MmuGetMMUCFG();
    ppcE500MmuSetPID0(0);
    if (uiMMUCFG.MMUCFG_ucNPIDS > 1) {
        ppcE500MmuSetPID1(0);
        if (uiMMUCFG.MMUCFG_ucNPIDS > 2) {
            ppcE500MmuSetPID2(0);
        }
    }

    /*
     * 设置 MAS4
     */
    uiMAS4.MAS4_uiValue   = 0;
    uiMAS4.MAS4_bTLBSELD  = 0;
    uiMAS4.MAS4_ucTIDSELD = 0;
    uiMAS4.MAS4_ucTSIZED  = MMU_TRANS_SZ_4K;
    uiMAS4.MAS4_bX0D      = MMU_MAS4_X0D;
    uiMAS4.MAS4_bX1D      = MMU_MAS4_X1D;
    uiMAS4.MAS4_bWD       = LW_FALSE;
    uiMAS4.MAS4_bID       = LW_TRUE;
    uiMAS4.MAS4_bMD       = LW_TRUE;
    uiMAS4.MAS4_bGD       = LW_FALSE;
    uiMAS4.MAS4_bED       = LW_FALSE;

    ppcE500MmuSetMAS4(uiMAS4.MAS4_uiValue);

    /*
     * 使能地址广播
     */
    if (_G_bHasHID1) {
        uiHID1 = ppcE500GetHID1();
        if (MMU_MAS2_M) {
            uiHID1 |=  ARCH_PPC_HID1_ABE;
        } else {
            uiHID1 &= ~ARCH_PPC_HID1_ABE;
        }
        ppcE500SetHID1(uiHID1);
    }

    /*
     * 有 MAS7 寄存器, 则使能 MAS7 寄存器的访问
     */
    if (_G_bHasMAS7) {
        UINT32  uiHID0;

        uiHID0  = ppcE500GetHID0();
        uiHID0 |= ARCH_PPC_HID0_MAS7EN;
        ppcE500SetHID0(uiHID0);
    }

    if (LW_CPU_GET_CUR_ID() == 0) {                                     /*  仅 Core0 复位 Cache         */
        archCacheReset(pcMachineName);                                  /*  复位 Cache                  */
    }

    ppcE500MmuInvalidateTLB();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuPgdOffset
** 功能描述: 通过虚拟地址计算 PGD 项
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址
** 输　出  : 对应的 PGD 表项地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  *ppcE500MmuPgdOffset (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry = pmmuctx->MMUCTX_pgdEntry;
    REGISTER UINT32              uiPgdNum;
    
    uiPgdNum   = ulAddr >> LW_CFG_VMM_PGD_SHIFT;
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry +
                 (uiPgdNum * sizeof(LW_PGD_TRANSENTRY)));               /*  获得一级页表描述符地址      */
               
    return  (p_pgdentry);
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuPmdOffset
** 功能描述: 通过虚拟地址计算 PMD 项
** 输　入  : p_pgdentry     pgd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 对应的 PMD 表项地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PMD_TRANSENTRY  *ppcE500MmuPmdOffset (LW_PGD_TRANSENTRY  *p_pgdentry, addr_t  ulAddr)
{
    return  ((LW_PMD_TRANSENTRY *)p_pgdentry);                          /*  PowerPC 无 PMD 项           */
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuPteOffset
** 功能描述: 通过虚拟地址计算 PTE 项
** 输　入  : p_pmdentry     pmd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 对应的 PTE 表项地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static  LW_PTE_TRANSENTRY  *ppcE500MmuPteOffset (LW_PMD_TRANSENTRY  *p_pmdentry, addr_t  ulAddr)
{
    REGISTER LW_PTE_TRANSENTRY  *p_pteentry;
    REGISTER UINT32              uiTemp;
    REGISTER UINT32              uiPageNum;

    uiTemp = (UINT32)(*p_pmdentry);                                     /*  获得一级页表描述符          */
    
    p_pteentry = (LW_PTE_TRANSENTRY *)(uiTemp);                         /*  获得二级页表基地址          */

    ulAddr &= ~LW_CFG_VMM_PGD_MASK;

    uiPageNum = ulAddr >> LW_CFG_VMM_PAGE_SHIFT;

    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry +
                  (uiPageNum * sizeof(LW_PTE_TRANSENTRY)));             /*  获得虚拟地址页表描述符地址  */
    
    return  (p_pteentry);
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuPgdIsOk
** 功能描述: 判断 PGD 项的描述符是否正确
** 输　入  : pgdentry       PGD 项描述符
** 输　出  : 是否正确
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static BOOL  ppcE500MmuPgdIsOk (LW_PGD_TRANSENTRY  pgdentry)
{
    return  (pgdentry ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuPteIsOk
** 功能描述: 判断 PTE 项的描述符是否正确
** 输　入  : pteentry       PTE 项描述符
** 输　出  : 是否正确
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static BOOL  ppcE500MmuPteIsOk (LW_PTE_TRANSENTRY  pteentry)
{
    return  ((pteentry.MAS3_bValid) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuPgdAlloc
** 功能描述: 分配 PGD 项
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址 (参数 0 即偏移量为 0 , 需要返回页表基地址)
** 输　出  : 分配 PGD 地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  *ppcE500MmuPgdAlloc (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry = (LW_PGD_TRANSENTRY *)API_PartitionGet(_G_hPGDPartition);
    REGISTER UINT32              uiPgdNum;
    
    if (!p_pgdentry) {
        return  (LW_NULL);
    }
    
    lib_bzero(p_pgdentry, PGD_BLOCK_SIZE);                              /*  无效一级页表项              */

    uiPgdNum   = ulAddr >> LW_CFG_VMM_PGD_SHIFT;
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry +
                 (uiPgdNum * sizeof(LW_PGD_TRANSENTRY)));               /*  获得一级页表描述符地址      */
               
    return  (p_pgdentry);
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuPgdFree
** 功能描述: 释放 PGD 项
** 输　入  : p_pgdentry     pgd 入口地址
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  ppcE500MmuPgdFree (LW_PGD_TRANSENTRY  *p_pgdentry)
{
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry & (~(PGD_BLOCK_SIZE - 1)));
    
    API_PartitionPut(_G_hPGDPartition, (PVOID)p_pgdentry);
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuPmdAlloc
** 功能描述: 分配 PMD 项
** 输　入  : pmmuctx        mmu 上下文
**           p_pgdentry     pgd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 分配 PMD 地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PMD_TRANSENTRY  *ppcE500MmuPmdAlloc (PLW_MMU_CONTEXT     pmmuctx,
                                               LW_PGD_TRANSENTRY  *p_pgdentry,
                                               addr_t              ulAddr)
{
    return  ((LW_PMD_TRANSENTRY *)p_pgdentry);
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuPmdFree
** 功能描述: 释放 PMD 项
** 输　入  : p_pmdentry     pmd 入口地址
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  ppcE500MmuPmdFree (LW_PMD_TRANSENTRY  *p_pmdentry)
{
    (VOID)p_pmdentry;
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuPteAlloc
** 功能描述: 分配 PTE 项
** 输　入  : pmmuctx        mmu 上下文
**           p_pmdentry     pmd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 分配 PTE 地址
** 全局变量: 
** 调用模块: VMM 这里没有关闭中断, 回写 CACHE 时, 需要手动关中断, SylixOS 映射完毕会自动清快表, 所以
             这里不用清除快表.
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  *ppcE500MmuPteAlloc (PLW_MMU_CONTEXT     pmmuctx,
                                               LW_PMD_TRANSENTRY  *p_pmdentry,
                                               addr_t              ulAddr)
{
    REGISTER LW_PTE_TRANSENTRY  *p_pteentry;
    REGISTER UINT32              uiPgdNum;

    uiPgdNum   = ulAddr >> LW_CFG_VMM_PGD_SHIFT;
    p_pteentry = (LW_PTE_TRANSENTRY *)(_G_ulPTETable + (uiPgdNum * PTE_BLOCK_SIZE));

    lib_bzero(p_pteentry, PTE_BLOCK_SIZE);

    *p_pmdentry = (LW_PMD_TRANSENTRY)ppcE500MmuBuildPgdesc((UINT32)p_pteentry); /*  设置二级页表基地址  */

    return  (ppcE500MmuPteOffset(p_pmdentry, ulAddr));
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuPteFree
** 功能描述: 释放 PTE 项
** 输　入  : p_pteentry     pte 入口地址
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  ppcE500MmuPteFree (LW_PTE_TRANSENTRY  *p_pteentry)
{
    (VOID)p_pteentry;
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuPtePhysGet
** 功能描述: 通过 PTE 表项, 查询物理地址
** 输　入  : pteentry           pte 表项
**           pulPhysicalAddr    获得的物理地址
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  ppcE500MmuPtePhysGet (LW_PTE_TRANSENTRY  pteentry, addr_t  *pulPhysicalAddr)
{
    UINT32   uiRPN   = pteentry.MAS3_uiRPN;                             /*  获得物理页面号              */

    *pulPhysicalAddr = uiRPN << LW_CFG_VMM_PAGE_SHIFT;                  /*  计算页面物理地址            */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuFlagGet
** 功能描述: 获得指定虚拟地址的 SylixOS 权限标志
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址
** 输　出  : SylixOS 权限标志
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ULONG  ppcE500MmuFlagGet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    LW_PGD_TRANSENTRY  *p_pgdentry = ppcE500MmuPgdOffset(pmmuctx, ulAddr);  /*  获得一级描述符地址      */

    if (ppcE500MmuPgdIsOk(*p_pgdentry)) {                               /*  一级描述符有效              */
        LW_PTE_TRANSENTRY  *p_pteentry = ppcE500MmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                                             ulAddr);   /*  获得二级描述符地址          */

        LW_PTE_TRANSENTRY   uiDescriptor = *p_pteentry;                 /*  获得二级描述符              */

        if (ppcE500MmuPteIsOk(uiDescriptor)) {                          /*  二级描述符有效              */
            ULONG    ulFlag = 0;

            if (uiDescriptor.MAS3_bValid) {                             /*  有效                        */
                ulFlag |= LW_VMM_FLAG_VALID;                            /*  映射有效                    */
            }

            ulFlag |= LW_VMM_FLAG_ACCESS;                               /*  可以访问                    */
            ulFlag |= LW_VMM_FLAG_GUARDED;                              /*  进行严格权限检查            */

            if (uiDescriptor.MAS3_bSuperExec) {
                ulFlag |= LW_VMM_FLAG_EXECABLE;                         /*  可以执行                    */
            }

            if (uiDescriptor.MAS3_bSuperWrite) {                        /*  可写                        */
                ulFlag |= LW_VMM_FLAG_WRITABLE;
            }

            if (!uiDescriptor.MAS3_bUnCache) {                          /*  可 Cache                    */
                ulFlag |= LW_VMM_FLAG_CACHEABLE;

                if (!uiDescriptor.MAS3_bWT) {                           /*  非写穿透                    */
                    ulFlag |= LW_VMM_FLAG_BUFFERABLE;
                }
            }

            return  (ulFlag);
        }
    }

    return  (LW_VMM_FLAG_UNVALID);
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuFlagSet
** 功能描述: 设置指定虚拟地址的 flag 标志
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址
**           ulFlag         flag 标志
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
** 注  意  : 这里不需要清除快表 TLB, 因为 VMM 自身会作此操作.
*********************************************************************************************************/
static INT  ppcE500MmuFlagSet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr, ULONG  ulFlag)
{
    LW_PGD_TRANSENTRY  *p_pgdentry = ppcE500MmuPgdOffset(pmmuctx, ulAddr);  /*  获得一级描述符地址      */

    if (!(ulFlag & LW_VMM_FLAG_VALID)) {                                /*  无效的映射关系              */
        return  (PX_ERROR);
    }

    if (ppcE500MmuPgdIsOk(*p_pgdentry)) {                               /*  一级描述符有效              */
        LW_PTE_TRANSENTRY  *p_pteentry = ppcE500MmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                                             ulAddr);   /*  获得二级描述符地址          */

        LW_PTE_TRANSENTRY   uiDescriptor = *p_pteentry;                 /*  获得二级描述符              */

        if (ppcE500MmuPteIsOk(uiDescriptor)) {                          /*  二级描述符有效              */
            UINT32   uiRPN = uiDescriptor.MAS3_uiRPN;                   /*  获得物理页号                */
            addr_t   ulPhysicalAddr = uiRPN << LW_CFG_VMM_PAGE_SHIFT;   /*  计算页面物理地址            */

            /*
             * 构建二级描述符并设置二级描述符
             */
            *p_pteentry = (LW_PTE_TRANSENTRY)ppcE500MmuBuildPtentry((UINT32)ulPhysicalAddr,
                                                                    ulFlag);
#if LW_CFG_CACHE_EN > 0
            ppcCacheDataUpdate((PVOID)p_pteentry,
                               sizeof(LW_PTE_TRANSENTRY), LW_FALSE);
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
            return  (ERROR_NONE);

        } else {
            return  (PX_ERROR);
        }

    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuMakeTrans
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
static VOID  ppcE500MmuMakeTrans (PLW_MMU_CONTEXT     pmmuctx,
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
    *p_pteentry = (LW_PTE_TRANSENTRY)ppcE500MmuBuildPtentry((UINT32)ulPhysicalAddr,
                                                            ulFlag);
#if LW_CFG_CACHE_EN > 0
    ppcCacheDataUpdate((PVOID)p_pteentry,
                       sizeof(LW_PTE_TRANSENTRY), LW_FALSE);
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuMakeCurCtx
** 功能描述: 设置 MMU 当前上下文
** 输　入  : pmmuctx        mmu 上下文
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  ppcE500MmuMakeCurCtx (PLW_MMU_CONTEXT  pmmuctx)
{
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuInvTLB
** 功能描述: 无效当前 CPU TLB
** 输　入  : pmmuctx        mmu 上下文
**           ulPageAddr     页面虚拟地址
**           ulPageNum      页面个数
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  ppcE500MmuInvTLB (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulPageAddr, ULONG  ulPageNum)
{
    ULONG   i;

    if (ulPageNum > (_G_uiTlbSize >> 1)) {
        ppcE500MmuInvalidateTLB();                                      /*  全部清除 TLB                */

    } else {
        for (i = 0; i < ulPageNum; i++) {
            ppcE500MmuInvalidateTLBEA((addr_t)ulPageAddr);              /*  逐个页面清除 TLB            */
            ulPageAddr += LW_CFG_VMM_PAGE_SIZE;
        }
    }
}
/*********************************************************************************************************
** 函数名称: ppcE500MmuInit
** 功能描述: MMU 系统初始化
** 输　入  : pmmuop            MMU 操作函数集
**           pcMachineName     使用的机器名称
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  ppcE500MmuInit (LW_MMU_OP  *pmmuop, CPCHAR  pcMachineName)
{
    TLBCFG_REG  uiTLB0CFG;

    /*
     * 使能了地址广播(HID1[ABE] 位)后, tlbsync 指令会自动多核同步
     */
    pmmuop->MMUOP_ulOption = 0ul;

    pmmuop->MMUOP_pfuncMemInit       = ppcE500MmuMemInit;
    pmmuop->MMUOP_pfuncGlobalInit    = ppcE500MmuGlobalInit;
    
    pmmuop->MMUOP_pfuncPGDAlloc      = ppcE500MmuPgdAlloc;
    pmmuop->MMUOP_pfuncPGDFree       = ppcE500MmuPgdFree;
    pmmuop->MMUOP_pfuncPMDAlloc      = ppcE500MmuPmdAlloc;
    pmmuop->MMUOP_pfuncPMDFree       = ppcE500MmuPmdFree;
    pmmuop->MMUOP_pfuncPTEAlloc      = ppcE500MmuPteAlloc;
    pmmuop->MMUOP_pfuncPTEFree       = ppcE500MmuPteFree;
    
    pmmuop->MMUOP_pfuncPGDIsOk       = ppcE500MmuPgdIsOk;
    pmmuop->MMUOP_pfuncPMDIsOk       = ppcE500MmuPgdIsOk;
    pmmuop->MMUOP_pfuncPTEIsOk       = ppcE500MmuPteIsOk;
    
    pmmuop->MMUOP_pfuncPGDOffset     = ppcE500MmuPgdOffset;
    pmmuop->MMUOP_pfuncPMDOffset     = ppcE500MmuPmdOffset;
    pmmuop->MMUOP_pfuncPTEOffset     = ppcE500MmuPteOffset;
    
    pmmuop->MMUOP_pfuncPTEPhysGet    = ppcE500MmuPtePhysGet;
    
    pmmuop->MMUOP_pfuncFlagGet       = ppcE500MmuFlagGet;
    pmmuop->MMUOP_pfuncFlagSet       = ppcE500MmuFlagSet;
    
    pmmuop->MMUOP_pfuncMakeTrans     = ppcE500MmuMakeTrans;
    pmmuop->MMUOP_pfuncMakeCurCtx    = ppcE500MmuMakeCurCtx;
    pmmuop->MMUOP_pfuncInvalidateTLB = ppcE500MmuInvTLB;
    pmmuop->MMUOP_pfuncSetEnable     = ppcE500MmuEnable;
    pmmuop->MMUOP_pfuncSetDisable    = ppcE500MmuDisable;

    /*
     * 多核一致性须使能 HID1[ABE] 位
     */
    MMU_MAS2_M = (LW_NCPUS > 1) ? 1 : 0;                                /*  多核一致性位设置            */

    if ((lib_strcmp(pcMachineName, PPC_MACHINE_E500V2) == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_E500MC) == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_E5500)  == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_E6500)  == 0)) {
        _G_bHasMAS7 = LW_TRUE;
    } else {
        _G_bHasMAS7 = LW_FALSE;
    }

    if ((lib_strcmp(pcMachineName, PPC_MACHINE_E500MC) == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_E5500)  == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_E6500)  == 0)) {
        _G_bHasHID1 = LW_FALSE;
    } else {
        _G_bHasHID1 = LW_TRUE;
    }

    /*
     * 获得 TLB0 条目数
     */
    uiTLB0CFG.TLBCFG_uiValue = ppcE500MmuGetTLB0CFG();
    _G_uiTlbSize = uiTLB0CFG.TLBCFG_usNENTRY;
    _DebugFormat(__LOGMESSAGE_LEVEL, "%s %s MMU TLB0 size = %d.\r\n",
                 LW_CFG_CPU_ARCH_FAMILY, pcMachineName, _G_uiTlbSize);
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
