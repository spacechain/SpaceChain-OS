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
** 文   件   名: armMmuV7.c
**
** 创   建   人: Jiao.Jinxing (焦进星)
**
** 文件创建日期: 2013 年 12 月 09 日
**
** 描        述: ARMv7 体系构架 MMU 驱动.
**
** BUG:
2014.05.24  ARMv7 带有 L2 CACHE 所以一级 CACHE 需要为写穿模式.
2014.09.04  重构 CACHE 权限, L1, L2 可分别控制回写与写穿模式.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "../armMmuCommon.h"
#include "../../cache/armCacheCommon.h"
#include "../../../common/cp15/armCp15.h"
#include "../../../param/armParam.h"
/*********************************************************************************************************
  一级描述符类型定义
*********************************************************************************************************/
#define COARSE_TBASE        (1)                                         /*  二级粗粒度页表基地址        */
#define SEGMENT_BASE        (2)                                         /*  段映射基地址                */
/*********************************************************************************************************
  二级描述符类型定义
*********************************************************************************************************/
#define FAIL_DESC           (0)                                         /*  变换失效                    */
#define SMALLPAGE_DESC      (2)                                         /*  小页基地址                  */
/*********************************************************************************************************
  域属性位
*********************************************************************************************************/
#define DOMAIN_FAIL         (0)                                         /*  产生访问失效                */
#define DOMAIN_CHECK        (1)                                         /*  进行权限检查                */
#define DOMAIN_NOCHK        (3)                                         /*  不进行权限检查              */
/*********************************************************************************************************
  域描述符
*********************************************************************************************************/
#define DOMAIN_ATTR         ((DOMAIN_CHECK)      |  \
                             (DOMAIN_NOCHK << 2) |  \
                             (DOMAIN_FAIL  << 4))
/*********************************************************************************************************
  3 个域定义
*********************************************************************************************************/
#define ACCESS_AND_CHK      0                                           /*  0 号域                      */
#define ACCESS_NOT_CHK      1                                           /*  1 号域                      */
#define ACCESS_FAIL         2                                           /*  2 号域                      */
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static LW_OBJECT_HANDLE     _G_hPGDPartition;                           /*  系统目前仅使用一个 PGD      */
static LW_OBJECT_HANDLE     _G_hPTEPartition;                           /*  PTE 缓冲区                  */
/*********************************************************************************************************
  内存映射属性配置
*********************************************************************************************************/
static BOOL                 _G_bVMSAForceShare = LW_FALSE;              /*  强制为共享模式              */
static UINT                 _G_uiVMSANonSec    = 0;                     /*  默认为安全模式              */
static UINT                 _G_uiVMSAShare;                             /*  共享位值                    */
static UINT                 _G_uiVMSADevType   = ARM_MMU_V7_DEV_SHAREABLE;
/*********************************************************************************************************
  全局定义
*********************************************************************************************************/
#define VMSA_FORCE_S        _G_bVMSAForceShare                          /*  强制为共享模式              */
#define VMSA_S              _G_uiVMSAShare                              /*  共享位值                    */
#define VMSA_NS             _G_uiVMSANonSec                             /*  非安全位值                  */
#define VMSA_nG             0ul                                         /*  非全局位值                  */
/*********************************************************************************************************
  汇编函数
*********************************************************************************************************/
extern UINT32 armMmuV7GetTTBCR(VOID);
extern VOID   armMmuV7SetTTBCR(UINT32 uiTTBCR);
/*********************************************************************************************************
** 函数名称: armMmuFlags2Attr
** 功能描述: 根据 SylixOS 权限标志, 生成 ARM MMU 权限标志
** 输　入  : ulState                 内存访问权限
**           pucAP                   访问权限
**           pucDomain               所属控制域
**           pucCB                   CACHE 控制参数
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  armMmuFlags2Attr (ULONG   ulFlag,
                              UINT8  *pucAP,
                              UINT8  *pucAP2,
                              UINT8  *pucDomain,
                              UINT8  *pucCB,
                              UINT8  *pucTEX,
                              UINT8  *pucXN)
{
    if (!(ulFlag & LW_VMM_FLAG_VALID)) {                                /*  无效的映射关系              */
        *pucDomain = ACCESS_FAIL;
        return  (PX_ERROR);
    }

    if (ulFlag & LW_VMM_FLAG_ACCESS) {                                  /*  是否拥有访问权限            */
        if (ulFlag & LW_VMM_FLAG_GUARDED) {
            *pucDomain = ACCESS_AND_CHK;                                /*  进行权限检查                */
        } else {
            *pucDomain = ACCESS_NOT_CHK;                                /*  不进行权限检查              */
        }
    } else {
        *pucDomain = ACCESS_FAIL;                                       /*  访问失效                    */
    }

    if (ulFlag & LW_VMM_FLAG_WRITABLE) {                                /*  是否可写                    */
        *pucAP2 = 0x0;                                                  /*  可写                        */
        *pucAP  = 0x3;
    } else {
        *pucAP2 = 0x1;                                                  /*  只读                        */
        *pucAP  = 0x3;
    }

    if ((ulFlag & LW_VMM_FLAG_CACHEABLE) &&
        (ulFlag & LW_VMM_FLAG_BUFFERABLE)) {                            /*  CACHE 与 BUFFER 控制        */
        *pucTEX = 0x1;                                                  /*  Outer: 回写, 写分配         */
        *pucCB  = 0x3;                                                  /*  Inner: 回写, 写分配         */

    } else if (ulFlag & LW_VMM_FLAG_CACHEABLE) {
        *pucTEX = 0x0;                                                  /*  Outer: 回写, 不具备写分配   */
        *pucCB  = 0x3;                                                  /*  Inner: 回写, 不具备写分配   */

    } else if (ulFlag & LW_VMM_FLAG_BUFFERABLE) {
        *pucTEX = 0x0;                                                  /*  Outer: 写穿透, 不具备写分配 */
        *pucCB  = 0x2;                                                  /*  Inner: 写穿透, 不具备写分配 */

    } else {
        switch (_G_uiVMSADevType) {
        
        case ARM_MMU_V7_DEV_SHAREABLE:
            *pucTEX = 0x0;
            *pucCB  = 0x1;
            break;
        
        case ARM_MMU_V7_DEV_NON_SHAREABLE:
            *pucTEX = 0x2;
            *pucCB  = 0x0;
            break;
            
        default:                                                        /*  强排序共享设备内存          */
            *pucTEX = 0x0;
            *pucCB  = 0x0;
            break;
        }
    }

    if (ulFlag & LW_VMM_FLAG_EXECABLE) {
        *pucXN = 0x0;
    } else {
        *pucXN = 0x1;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: armMmuAttr2Flags
** 功能描述: 根据 ARM MMU 权限标志, 生成 SylixOS 权限标志
** 输　入  : ulState                 内存访问权限
**           pucAP                   访问权限
**           pucDomain               所属控制域
**           pucCB                   CACHE 控制参数
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  armMmuAttr2Flags (UINT8  ucAP,
                              UINT8  ucAP2,
                              UINT8  ucDomain,
                              UINT8  ucCB,
                              UINT8  ucTEX,
                              UINT8  ucXN,
                              ULONG *pulFlag)
{
    (VOID)ucAP;
    (VOID)ucTEX;

    *pulFlag = LW_VMM_FLAG_VALID;
    
    if (ucDomain == ACCESS_AND_CHK) {
        *pulFlag |= LW_VMM_FLAG_GUARDED;
        *pulFlag |= LW_VMM_FLAG_ACCESS;
    
    } else if (ucDomain == ACCESS_NOT_CHK) {
        *pulFlag |= LW_VMM_FLAG_ACCESS;
    }
    
    if (ucAP2 == 0) {
        *pulFlag |= LW_VMM_FLAG_WRITABLE;
    }
    
    switch (ucCB) {
    
    case 0x3:
        if (ucTEX == 0x1) {
            *pulFlag |= LW_VMM_FLAG_CACHEABLE | LW_VMM_FLAG_BUFFERABLE;
        } else {
            *pulFlag |= LW_VMM_FLAG_CACHEABLE;
        }
        break;
        
    case 0x2:
        *pulFlag |= LW_VMM_FLAG_BUFFERABLE;
        break;

    default:
        break;
    }
    
    if (ucXN == 0x0) {
        *pulFlag |= LW_VMM_FLAG_EXECABLE;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: armMmuBuildPgdesc
** 功能描述: 生成一个一级描述符 (PGD 描述符)
** 输　入  : uiBaseAddr              基地址     (段基地址、二级页表基地址)
**           ucAP                    访问权限
**           ucDomain                域
**           ucCB                    CACHE 和 WRITEBUFFER 控制
**           ucType                  一级描述符类型
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  armMmuBuildPgdesc (UINT32  uiBaseAddr,
                                             UINT8   ucAP,
                                             UINT8   ucAP2,
                                             UINT8   ucDomain,
                                             UINT8   ucCB,
                                             UINT8   ucTEX,
                                             UINT8   ucXN,
                                             UINT8   ucType)
{
    LW_PGD_TRANSENTRY   uiDescriptor;

    switch (ucType) {
    
    case COARSE_TBASE:                                                  /*  粗粒度二级页表描述符        */
        uiDescriptor = (uiBaseAddr & 0xFFFFFC00)
                     | (ucDomain <<  5)
                     | (VMSA_NS  <<  3)
                     | ucType;
        break;
        
    case SEGMENT_BASE:                                                  /*  段描述符                    */
        uiDescriptor = (uiBaseAddr & 0xFFF00000)
                     | (VMSA_NS  << 19)
                     | (VMSA_nG  << 17)
                     | (VMSA_S   << 16)
                     | (ucAP2    << 15)
                     | (ucTEX    << 12)
                     | (ucAP     << 10)
                     | (ucDomain <<  5)
                     | (ucXN     <<  4)
                     | (ucCB     <<  2)
                     | ucType;
        break;
        
    default:
        uiDescriptor = 0;                                               /*  访问失效                    */
        break;
    }
    
    return  (uiDescriptor);
}
/*********************************************************************************************************
** 函数名称: armMmuBuildPtentry
** 功能描述: 生成一个二级描述符 (PTE 描述符)
** 输　入  : uiBaseAddr              基地址     (页地址)
**           ucAP                    访问权限
**           ucDomain                域
**           ucCB                    CACHE 和 WRITEBUFFER 控制
**           ucType                  二级描述符类型
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  armMmuBuildPtentry (UINT32  uiBaseAddr,
                                              UINT8   ucAP,
                                              UINT8   ucAP2,
                                              UINT8   ucDomain,
                                              UINT8   ucCB,
                                              UINT8   ucTEX,
                                              UINT8   ucXN,
                                              UINT8   ucType)
{
    LW_PTE_TRANSENTRY   uiDescriptor;

    switch (ucType) {
    
    case SMALLPAGE_DESC:                                                /*  小页描述符                  */
        uiDescriptor = (uiBaseAddr & 0xFFFFF000)
                     | (VMSA_nG  << 11)
                     | (VMSA_S   << 10)
                     | (ucAP2    <<  9)
                     | (ucTEX    <<  6)
                     | (ucAP     <<  4)
                     | (ucCB     <<  2)
                     | (ucXN     <<  0)
                     | ucType;
        break;

    default:
        uiDescriptor = 0;                                               /*  访问失效                    */
        break;
    }
    
    return  (uiDescriptor);
}
/*********************************************************************************************************
** 函数名称: armMmuMemInit
** 功能描述: 初始化 MMU 页表内存区
** 输　入  : pmmuctx        mmu 上下文
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
** 注  意  : ARM 体系结构要求: 一级页表基地址需要保持 16 KByte 对齐, 单条目映射 1 MByte 空间.
                               二级页表基地址需要保持  1 KByte 对齐, 单条目映射 4 KByte 空间.
*********************************************************************************************************/
static INT  armMmuMemInit (PLW_MMU_CONTEXT  pmmuctx)
{
#define PGD_BLOCK_SIZE  (16 * LW_CFG_KB_SIZE)
#define PTE_BLOCK_SIZE  ( 1 * LW_CFG_KB_SIZE)

    PVOID   pvPgdTable;
    PVOID   pvPteTable;
    
    ULONG   ulPgdNum = bspMmuPgdMaxNum();
    ULONG   ulPteNum = bspMmuPteMaxNum();
    
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
** 函数名称: armMmuGlobalInit
** 功能描述: 调用 BSP 对 MMU 初始化
** 输　入  : pcMachineName     使用的机器名称
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  armMmuGlobalInit (CPCHAR  pcMachineName)
{
    ARM_PARAM   *param = archKernelParamGet();
    
    archCacheReset(pcMachineName);

    armMmuInvalidateTLB();

    armMmuSetDomain(DOMAIN_ATTR);
    armMmuSetProcessId(0);

    /*
     *  地址检查选项 (Qt 里面用到了非对齐指令)
     *  注 意: 如果使能地址对齐检查, GCC 编译必须加入 -mno-unaligned-access 选项 (不生成非对齐访问指令)
     */
    if (param->AP_bUnalign) {
        armMmuDisableAlignFault();
        
    } else {
        armMmuEnableAlignFault();                                       /*  -mno-unaligned-access       */
    }
    
    armControlFeatureDisable(CP15_CONTROL_TEXREMAP);                    /*  Disable TEX remapping       */

    armMmuV7SetTTBCR(0);                                                /*  Use the 32-bit translation  */
                                                                        /*  system, Always use TTBR0    */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: armMmuPgdOffset
** 功能描述: 通过虚拟地址计算 PGD 项
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址
** 输　出  : 对应的 PGD 表项地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY *armMmuPgdOffset (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry = pmmuctx->MMUCTX_pgdEntry;
    
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry
               | ((ulAddr >> LW_CFG_VMM_PGD_SHIFT) << 2));              /*  获得一级页表描述符地址      */
               
    return  (p_pgdentry);
}
/*********************************************************************************************************
** 函数名称: armMmuPmdOffset
** 功能描述: 通过虚拟地址计算 PMD 项
** 输　入  : p_pgdentry     pgd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 对应的 PMD 表项地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PMD_TRANSENTRY *armMmuPmdOffset (LW_PGD_TRANSENTRY  *p_pgdentry, addr_t  ulAddr)
{
    return  ((LW_PMD_TRANSENTRY *)p_pgdentry);                          /*  ARM 无 PMD 项               */
}
/*********************************************************************************************************
** 函数名称: armMmuPteOffset
** 功能描述: 通过虚拟地址计算 PTE 项
** 输　入  : p_pgdentry     pgd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 对应的 PTE 表项地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static  LW_PTE_TRANSENTRY *armMmuPteOffset (LW_PMD_TRANSENTRY  *p_pmdentry, addr_t  ulAddr)
{
    REGISTER LW_PTE_TRANSENTRY  *p_pteentry;
    REGISTER UINT32              uiTemp;
    
    uiTemp = (UINT32)(*p_pmdentry);                                     /*  获得二级页表描述符          */
    
    p_pteentry = (LW_PTE_TRANSENTRY *)(uiTemp & (~(LW_CFG_KB_SIZE - 1)));
                                                                        /*  获得粗粒度二级页表基地址    */
    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry
               | ((ulAddr >> 10) & 0x3FC));                             /*  获得对应虚拟地址页表描述符  */
    
    return  (p_pteentry);
}
/*********************************************************************************************************
** 函数名称: armMmuPgdIsOk
** 功能描述: 判断 PGD 项的描述符是否正确
** 输　入  : pgdentry       PGD 项描述符
** 输　出  : 是否正确
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static BOOL  armMmuPgdIsOk (LW_PGD_TRANSENTRY  pgdentry)
{
    return  (((pgdentry & 0x03) == COARSE_TBASE) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: armMmuPteIsOk
** 功能描述: 判断 PTE 项的描述符是否正确
** 输　入  : pteentry       PTE 项描述符
** 输　出  : 是否正确
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static BOOL  armMmuPteIsOk (LW_PTE_TRANSENTRY  pteentry)
{
    /*
     * 注意, SMALLPAGE 描述符最低位是 XN 位, 所以与上 0x02, 而不是 0x03
     */
    return  (((pteentry & 0x02) == SMALLPAGE_DESC) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: armMmuPgdAlloc
** 功能描述: 分配 PGD 项
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址 (参数 0 即偏移量为 0 , 需要返回页表基地址)
** 输　出  : 分配 PGD 地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY *armMmuPgdAlloc (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
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
** 函数名称: armMmuPgdFree
** 功能描述: 释放 PGD 项
** 输　入  : p_pgdentry     pgd 入口地址
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  armMmuPgdFree (LW_PGD_TRANSENTRY  *p_pgdentry)
{
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry & (~((16 * LW_CFG_KB_SIZE) - 1)));
    
    API_PartitionPut(_G_hPGDPartition, (PVOID)p_pgdentry);
}
/*********************************************************************************************************
** 函数名称: armMmuPmdAlloc
** 功能描述: 分配 PMD 项
** 输　入  : pmmuctx        mmu 上下文
**           p_pgdentry     pgd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 分配 PMD 地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PMD_TRANSENTRY *armMmuPmdAlloc (PLW_MMU_CONTEXT     pmmuctx, 
                                          LW_PGD_TRANSENTRY  *p_pgdentry,
                                          addr_t              ulAddr)
{
    return  ((LW_PMD_TRANSENTRY *)p_pgdentry);
}
/*********************************************************************************************************
** 函数名称: armMmuPmdFree
** 功能描述: 释放 PMD 项
** 输　入  : p_pmdentry     pmd 入口地址
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  armMmuPmdFree (LW_PMD_TRANSENTRY  *p_pmdentry)
{
    (VOID)p_pmdentry;
}
/*********************************************************************************************************
** 函数名称: armMmuPteAlloc
** 功能描述: 分配 PTE 项
** 输　入  : pmmuctx        mmu 上下文
**           p_pmdentry     pmd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 分配 PTE 地址
** 全局变量: 
** 调用模块: VMM 这里没有关闭中断, 回写 CACHE 时, 需要手动关中断, SylixOS 映射完毕会自动清快表, 所以
             这里不用清除快表.
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  *armMmuPteAlloc (PLW_MMU_CONTEXT     pmmuctx, 
                                           LW_PMD_TRANSENTRY  *p_pmdentry,
                                           addr_t              ulAddr)
{
#if LW_CFG_CACHE_EN > 0
    INTREG              iregInterLevel;
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
    UINT8               ucAP;                                           /*  存储权限                    */
    UINT8               ucDomain;                                       /*  域                          */
    UINT8               ucCB;                                           /*  CACHE 与缓冲区控制          */
    UINT8               ucAP2;                                          /*  存储权限                    */
    UINT8               ucTEX;                                          /*  CACHE 与缓冲区控制          */
    UINT8               ucXN;                                           /*  永不执行位                  */

    LW_PTE_TRANSENTRY  *p_pteentry = (LW_PTE_TRANSENTRY *)API_PartitionGet(_G_hPTEPartition);
    
    if (!p_pteentry) {
        return  (LW_NULL);
    }
    
    lib_bzero(p_pteentry, PTE_BLOCK_SIZE);
    
    armMmuFlags2Attr(LW_VMM_FLAG_VALID   |
                     LW_VMM_FLAG_ACCESS  |
                     LW_VMM_FLAG_GUARDED |
                     LW_VMM_FLAG_WRITABLE|
                     LW_VMM_FLAG_EXECABLE,
                     &ucAP, &ucAP2,
                     &ucDomain,
                     &ucCB, &ucTEX,
                     &ucXN);

    *p_pmdentry = (LW_PMD_TRANSENTRY)armMmuBuildPgdesc((UINT32)p_pteentry,
                                                       ucAP, ucAP2,
                                                       ucDomain,
                                                       ucCB, ucTEX,
                                                       ucXN,
                                                       COARSE_TBASE);   /*  设置二级页面基地址          */
#if LW_CFG_CACHE_EN > 0
    iregInterLevel = KN_INT_DISABLE();
    armDCacheFlush((PVOID)p_pmdentry, (PVOID)p_pmdentry, 32);           /*  第三个参数无影响            */
    KN_INT_ENABLE(iregInterLevel);
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

    return  (armMmuPteOffset(p_pmdentry, ulAddr));
}
/*********************************************************************************************************
** 函数名称: armMmuPteFree
** 功能描述: 释放 PTE 项
** 输　入  : p_pteentry     pte 入口地址
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  armMmuPteFree (LW_PTE_TRANSENTRY  *p_pteentry)
{
    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry & (~(LW_CFG_KB_SIZE - 1)));
    
    API_PartitionPut(_G_hPTEPartition, (PVOID)p_pteentry);
}
/*********************************************************************************************************
** 函数名称: armMmuPtePhysGet
** 功能描述: 通过 PTE 表项, 查询物理地址
** 输　入  : pteentry           pte 表项
**           pulPhysicalAddr    获得的物理地址
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  armMmuPtePhysGet (LW_PTE_TRANSENTRY  pteentry, addr_t  *pulPhysicalAddr)
{
    *pulPhysicalAddr = (addr_t)(pteentry & (UINT32)0xFFFFF000);         /*  获得物理地址                */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: armMmuFlagGet
** 功能描述: 获得指定虚拟地址的 SylixOS 权限标志
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址
** 输　出  : SylixOS 权限标志
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ULONG  armMmuFlagGet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    LW_PGD_TRANSENTRY  *p_pgdentry = armMmuPgdOffset(pmmuctx, ulAddr);
    INT                 iDescType;
    UINT8               ucAP;                                           /*  存储权限                    */
    UINT8               ucDomain;                                       /*  域                          */
    UINT8               ucCB;                                           /*  CACHE 与缓冲区控制          */
    UINT8               ucAP2;                                          /*  存储权限                    */
    UINT8               ucTEX;                                          /*  CACHE 与缓冲区控制          */
    UINT8               ucXN;                                           /*  永不执行位                  */
    ULONG               ulFlag = 0;
    
    iDescType = (*p_pgdentry) & 0x03;                                   /*  获得一级页表类型            */
    if (iDescType == SEGMENT_BASE) {                                    /*  基于段的映射                */
        UINT32  uiDescriptor = (UINT32)(*p_pgdentry);
        
        ucCB     = (UINT8)((uiDescriptor >>  2) & 0x03);
        ucDomain = (UINT8)((uiDescriptor >>  5) & 0x0F);
        ucAP     = (UINT8)((uiDescriptor >> 10) & 0x02);

        ucAP2    = (UINT8)((uiDescriptor >> 15) & 0x01);
        ucTEX    = (UINT8)((uiDescriptor >> 12) & 0x07);

        ucXN     = (UINT8)((uiDescriptor >>  4) & 0x01);
    
    } else if (iDescType == COARSE_TBASE) {                             /*  基于粗粒度二级页表映射      */
        UINT32              uiDescriptor = (UINT32)(*p_pgdentry);
        LW_PTE_TRANSENTRY  *p_pteentry   = armMmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry, ulAddr);
        
        if (armMmuPteIsOk(*p_pteentry)) {
            ucDomain     = (UINT8)((uiDescriptor >>  5) & 0x0F);
            
            uiDescriptor = (UINT32)(*p_pteentry);
            ucCB         = (UINT8)((uiDescriptor >>  2) & 0x03);
            ucAP         = (UINT8)((uiDescriptor >>  4) & 0x03);

            ucAP2        = (UINT8)((uiDescriptor >>  9) & 0x01);
            ucTEX        = (UINT8)((uiDescriptor >>  6) & 0x07);

            ucXN         = (UINT8)((uiDescriptor >>  0) & 0x01);
        
        } else {
            return  (LW_VMM_FLAG_UNVALID);
        }
    } else {
        return  (LW_VMM_FLAG_UNVALID);
    }
    
    armMmuAttr2Flags(ucAP, ucAP2, ucDomain, ucCB, ucTEX, ucXN, &ulFlag);
    
    return  (ulFlag);
}
/*********************************************************************************************************
** 函数名称: armMmuFlagSet
** 功能描述: 设置指定虚拟地址的 flag 标志
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址
**           ulFlag         flag 标志
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
** 注  意  : 这里不需要清除快表 TLB, 因为 VMM 自身会作此操作.
*********************************************************************************************************/
static INT  armMmuFlagSet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr, ULONG  ulFlag)
{
    LW_PGD_TRANSENTRY  *p_pgdentry = armMmuPgdOffset(pmmuctx, ulAddr);
    INT                 iDescType;
    
    UINT8               ucAP;                                           /*  存储权限                    */
    UINT8               ucDomain;                                       /*  域                          */
    UINT8               ucCB;                                           /*  CACHE 与缓冲区控制          */
    UINT8               ucAP2;                                          /*  存储权限                    */
    UINT8               ucTEX;                                          /*  CACHE 与缓冲区控制          */
    UINT8               ucXN;                                           /*  永不执行位                  */
    UINT8               ucType;
    
    if (ulFlag & LW_VMM_FLAG_ACCESS) {
        ucType = SMALLPAGE_DESC;
    } else {
        ucType = FAIL_DESC;                                             /*  访问将失效                  */
    }
    
    if (armMmuFlags2Attr(ulFlag,
                         &ucAP, &ucAP2,
                         &ucDomain,
                         &ucCB, &ucTEX,
                         &ucXN) < 0) {                                  /*  无效的映射关系              */
        return  (PX_ERROR);
    }
    
    iDescType = (*p_pgdentry) & 0x03;                                   /*  获得一级页表类型            */
    if (iDescType == SEGMENT_BASE) {                                    /*  基于段的映射                */
        return  (ERROR_NONE);
    
    } else if (iDescType == COARSE_TBASE) {                             /*  基于粗粒度二级页表映射      */
        REGISTER LW_PTE_TRANSENTRY  *p_pteentry = armMmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                                                  ulAddr);
        if (armMmuPteIsOk(*p_pteentry)) {
            addr_t   ulPhysicalAddr = (addr_t)(*p_pteentry & 0xFFFFF000);
            *p_pteentry = (LW_PTE_TRANSENTRY)armMmuBuildPtentry((UINT32)ulPhysicalAddr,
                                                                ucAP, ucAP2,
                                                                ucDomain,
                                                                ucCB, ucTEX,
                                                                ucXN,
                                                                ucType);
#if LW_CFG_CACHE_EN > 0
            armDCacheFlush((PVOID)p_pteentry, (PVOID)p_pteentry, 32);   /*  第三个参数无影响            */
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
            return  (ERROR_NONE);
        }
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: armMmuMakeTrans
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
static VOID  armMmuMakeTrans (PLW_MMU_CONTEXT     pmmuctx,
                              LW_PTE_TRANSENTRY  *p_pteentry,
                              addr_t              ulVirtualAddr,
                              addr_t              ulPhysicalAddr,
                              addr_t              ulFlag)
{
    UINT8               ucAP;                                           /*  存储权限                    */
    UINT8               ucDomain;                                       /*  域                          */
    UINT8               ucCB;                                           /*  CACHE 与缓冲区控制          */
    UINT8               ucAP2;                                          /*  存储权限                    */
    UINT8               ucTEX;                                          /*  CACHE 与缓冲区控制          */
    UINT8               ucXN;                                           /*  永不执行位                  */
    UINT8               ucType;
    
    if (ulFlag & LW_VMM_FLAG_ACCESS) {
        ucType = SMALLPAGE_DESC;
    } else {
        ucType = FAIL_DESC;                                             /*  访问将失效                  */
    }
    
    if (armMmuFlags2Attr(ulFlag,
                         &ucAP, &ucAP2,
                         &ucDomain,
                         &ucCB, &ucTEX,
                         &ucXN) < 0) {                                  /*  无效的映射关系              */
        return;
    }
    
    *p_pteentry = (LW_PTE_TRANSENTRY)armMmuBuildPtentry((UINT32)ulPhysicalAddr,
                                                        ucAP, ucAP2,
                                                        ucDomain,
                                                        ucCB, ucTEX,
                                                        ucXN,
                                                        ucType);
                                                        
#if LW_CFG_CACHE_EN > 0
    armDCacheFlush((PVOID)p_pteentry, (PVOID)p_pteentry, 32);           /*  第三个参数无影响            */
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
}
/*********************************************************************************************************
** 函数名称: armMmuMakeCurCtx
** 功能描述: 设置 MMU 当前上下文, 这里使用 TTBR1 页表基址寄存器.
** 输　入  : pmmuctx        mmu 上下文
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  armMmuMakeCurCtx (PLW_MMU_CONTEXT  pmmuctx)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry;

    if (LW_NCPUS > 1) {
        /*
         *  Set location of level 1 page table
         * ------------------------------------
         *  31:14 - Base addr
         *  13:7  - 0x0
         *  6     - IRGN[0]     0x1 (Normal memory, Inner Write-Back Write-Allocate Cacheable)
         *  5     - NOS         0x0 (Outer Shareable)
         *  4:3   - RGN         0x1 (Normal memory, Outer Write-Back Write-Allocate Cacheable)
         *  2     - IMP         0x0
         *  1     - S           0x1 (Shareable)
         *  0     - IRGN[1]     0x0 (Normal memory, Inner Write-Back Write-Allocate Cacheable)
         */
        p_pgdentry = (LW_PGD_TRANSENTRY *)((ULONG)pmmuctx->MMUCTX_pgdEntry
                   | (1 << 6)
                   | ((!VMSA_S) << 5)
                   | (1 << 3)
                   | (0 << 2)
                   | (VMSA_S << 1)
                   | (0 << 0));
    } else {
        /*
         *  Set location of level 1 page table
         * ------------------------------------
         *  31:14 - Base addr
         *  13:7  - 0x0
         *  5     - NOS         0x1 (Outer NonShareable)
         *  4:3   - RGN         0x1 (Normal memory, Outer Write-Back Write-Allocate Cacheable)
         *  2     - IMP         0x0
         *  1     - S           0x0 (NonShareable)
         *  0     - C           0x1 (Inner cacheable)
         */
        p_pgdentry = (LW_PGD_TRANSENTRY *)((ULONG)pmmuctx->MMUCTX_pgdEntry
                   | ((!VMSA_S) << 5)
                   | (1 << 3)
                   | (0 << 2)
                   | (VMSA_S << 1)
                   | (1 << 0));
    }
    
    armMmuSetTTBase(p_pgdentry);
    armMmuSetTTBase1(p_pgdentry);
}
/*********************************************************************************************************
** 函数名称: armMmuInvTLB
** 功能描述: 无效当前 CPU TLB
** 输　入  : pmmuctx        mmu 上下文
**           ulPageAddr     页面虚拟地址
**           ulPageNum      页面个数
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  armMmuInvTLB (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulPageAddr, ULONG  ulPageNum)
{
    ULONG   i;

    if (ulPageNum > 16) {
        armMmuInvalidateTLB();                                          /*  全部清除 TLB                */

    } else {
        for (i = 0; i < ulPageNum; i++) {
            armMmuInvalidateTLBMVA((PVOID)ulPageAddr);                  /*  逐个页面清除 TLB            */
            ulPageAddr += LW_CFG_VMM_PAGE_SIZE;
        }
    }
}
/*********************************************************************************************************
** 函数名称: armMmuV7Init
** 功能描述: MMU 系统初始化
** 输　入  : pmmuop            MMU 操作函数集
**           pcMachineName     使用的机器名称
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  armMmuV7Init (LW_MMU_OP *pmmuop, CPCHAR  pcMachineName)
{
    pmmuop->MMUOP_ulOption = 0ul;
    pmmuop->MMUOP_pfuncMemInit    = armMmuMemInit;
    pmmuop->MMUOP_pfuncGlobalInit = armMmuGlobalInit;
    
    pmmuop->MMUOP_pfuncPGDAlloc = armMmuPgdAlloc;
    pmmuop->MMUOP_pfuncPGDFree  = armMmuPgdFree;
    pmmuop->MMUOP_pfuncPMDAlloc = armMmuPmdAlloc;
    pmmuop->MMUOP_pfuncPMDFree  = armMmuPmdFree;
    pmmuop->MMUOP_pfuncPTEAlloc = armMmuPteAlloc;
    pmmuop->MMUOP_pfuncPTEFree  = armMmuPteFree;
    
    pmmuop->MMUOP_pfuncPGDIsOk = armMmuPgdIsOk;
    pmmuop->MMUOP_pfuncPMDIsOk = armMmuPgdIsOk;
    pmmuop->MMUOP_pfuncPTEIsOk = armMmuPteIsOk;
    
    pmmuop->MMUOP_pfuncPGDOffset = armMmuPgdOffset;
    pmmuop->MMUOP_pfuncPMDOffset = armMmuPmdOffset;
    pmmuop->MMUOP_pfuncPTEOffset = armMmuPteOffset;
    
    pmmuop->MMUOP_pfuncPTEPhysGet = armMmuPtePhysGet;
    
    pmmuop->MMUOP_pfuncFlagGet = armMmuFlagGet;
    pmmuop->MMUOP_pfuncFlagSet = armMmuFlagSet;
    
    pmmuop->MMUOP_pfuncMakeTrans     = armMmuMakeTrans;
    pmmuop->MMUOP_pfuncMakeCurCtx    = armMmuMakeCurCtx;
    pmmuop->MMUOP_pfuncInvalidateTLB = armMmuInvTLB;
    pmmuop->MMUOP_pfuncSetEnable     = armMmuEnable;
    pmmuop->MMUOP_pfuncSetDisable    = armMmuDisable;
    
    VMSA_S = ((LW_NCPUS > 1) || VMSA_FORCE_S) ? 1 : 0;                  /*  共享位设置                  */
}
/*********************************************************************************************************
** 函数名称: armMmuV7ForceShare
** 功能描述: MMU 系统强制为 share 模式
** 输　入  : bEnOrDis      是否使能强制 share 模式, 此函数仅可在 bspKernelInitHook() 中被调用.
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  armMmuV7ForceShare (BOOL  bEnOrDis)
{
    VMSA_FORCE_S = bEnOrDis;
}
/*********************************************************************************************************
** 函数名称: armMmuV7ForceNonSecure
** 功能描述: MMU 系统强制为 Non-Secure 模式
** 输　入  : bEnOrDis      是否使能强制 Non-Secure 模式, 此函数仅可在 bspKernelInitHook() 中被调用.
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  armMmuV7ForceNonSecure (BOOL  bEnOrDis)
{
    VMSA_NS = (bEnOrDis) ? 1 : 0;
}
/*********************************************************************************************************
** 函数名称: armMmuV7ForceDevType
** 功能描述: MMU 系统强制设置 Dev 内存类型
** 输　入  : uiType        设备内存访问类型
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  armMmuV7ForceDevType (UINT  uiType)
{
    _G_uiVMSADevType = uiType;
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
