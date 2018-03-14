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
** 文   件   名: vmm.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 12 月 09 日
**
** 描        述: 平台无关虚拟内存管理.

** BUG:
2009.11.10  LW_VMM_ZONEDESC 中加入 DMA 区域判断符, 表明该区域是否可供 DMA 使用.
            DMA 内存分配, 返回值为物理地址.
2011.03.18  将 LW_VMM_ZONEDESC 更名为 LW_VMM_ZONE_DESC.
*********************************************************************************************************/

#ifndef __VMM_H
#define __VMM_H

/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0

/*********************************************************************************************************
  ZONE 类型
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL

#define LW_ZONE_ATTR_NONE       0                                       /*  无特殊属性                  */
#define LW_ZONE_ATTR_DMA        1                                       /*  映射为 DMA 平板模式使用     */

/*********************************************************************************************************
  物理内存信息描述
  注意:
  TEXT, DATA, DMA 物理段 PHYD_ulPhyAddr 必须等于 PHYD_ulVirMap,
  TEXT, DATA, VECTOR, BOOTSFR, DMA 物理段 PHYD_ulVirMap 区间不能与虚拟内存空间冲突.
*********************************************************************************************************/

typedef struct __lw_vmm_physical_desc {
    addr_t                   PHYD_ulPhyAddr;                            /*  物理地址 (页对齐地址)       */
    addr_t                   PHYD_ulVirMap;                             /*  需要初始化的映射关系        */
    size_t                   PHYD_stSize;                               /*  物理内存区长度 (页对齐长度) */
    
#define LW_PHYSICAL_MEM_TEXT        0                                   /*  内核代码段                  */
#define LW_PHYSICAL_MEM_DATA        1                                   /*  内核数据段 (包括 HEAP)      */
#define LW_PHYSICAL_MEM_VECTOR      2                                   /*  硬件向量表                  */
#define LW_PHYSICAL_MEM_BOOTSFR     3                                   /*  启动时需要的特殊功能寄存器  */
#define LW_PHYSICAL_MEM_BUSPOOL     4                                   /*  总线地址池, 不进行提前映射  */
#define LW_PHYSICAL_MEM_DMA         5                                   /*  DMA 物理内存, 不进行提前映射*/
#define LW_PHYSICAL_MEM_APP         6                                   /*  装载程序内存, 不进行提前映射*/
    UINT32                   PHYD_uiType;                               /*  物理内存区间类型            */
    UINT32                   PHYD_uiReserve[8];
} LW_MMU_PHYSICAL_DESC;
typedef LW_MMU_PHYSICAL_DESC *PLW_MMU_PHYSICAL_DESC;

/*********************************************************************************************************
  虚拟地址空间描述
  注意:
  虚拟内存空间编址不能与物理内存空间 TEXT, DATA, VECTOR, BOOTSFR, DMA 区间冲突,
  DEV 属性内存分区最多一个.
  一般情况下 APP 虚拟内存区间要远远大于物理 APP 内存大小, 同时大于 DEV 虚拟内存区间大小.
*********************************************************************************************************/

typedef struct __lw_mmu_virtual_desc {
    addr_t                   VIRD_ulVirAddr;                            /*  虚拟空间地址 (页对齐地址)   */
    size_t                   VIRD_stSize;                               /*  虚拟空间长度 (页对齐长度)   */
    
#define LW_VIRTUAL_MEM_APP      0                                       /*  装载程序虚拟内存区间        */
#define LW_VIRTUAL_MEM_DEV      1                                       /*  设备映射虚拟内存空间        */
    UINT32                   VIRD_uiType;                               /*  虚拟内存区间类型            */
    UINT32                   VIRD_uiReserve[8];
} LW_MMU_VIRTUAL_DESC;
typedef LW_MMU_VIRTUAL_DESC *PLW_MMU_VIRTUAL_DESC;

#endif                                                                  /*  __SYLIXOS_KERNEL            */
/*********************************************************************************************************
  vmm 当前状态
*********************************************************************************************************/

typedef struct __lw_vmm_status {
    INT64                    VMMS_i64AbortCounter;                      /*  异常中止次数                */
    INT64                    VMMS_i64PageFailCounter;                   /*  缺页中断正常处理次数        */
    INT64                    VMMS_i64PageLackCounter;                   /*  系统缺少物理页面次数        */
    INT64                    VMMS_i64MapErrCounter;                     /*  映射错误次数                */
    INT64                    VMMS_i64SwpCounter;                        /*  交换次数                    */
    INT64                    VMMS_i64Reseve[8];
} LW_VMM_STATUS;
typedef LW_VMM_STATUS       *PLW_VMM_STATUS;

/*********************************************************************************************************
  VMM 初始化, 只能放在 API_KernelStart 回调中
  
  当为 SMP 系统时, API_KernelPrimaryStart    对应启动回调调用 API_VmmLibPrimaryInit
                   API_KernelSecondaryStart  对应启动回调调用 API_VmmLibSecondaryInit
*********************************************************************************************************/

#ifdef __SYLIXOS_KERNEL
LW_API ULONG        API_VmmLibPrimaryInit(LW_MMU_PHYSICAL_DESC  pphydesc[], 
                                          LW_MMU_VIRTUAL_DESC   pvirdes[],
                                          CPCHAR                pcMachineName);
                                                                        /*  初始化 VMM 组件及物理区域   */
#define API_VmmLibInit      API_VmmLibPrimaryInit

#if LW_CFG_SMP_EN > 0
LW_API ULONG        API_VmmLibSecondaryInit(CPCHAR  pcMachineName);
#endif                                                                  /*  LW_CFG_SMP_EN               */

LW_API ULONG        API_VmmLibAddPhyRam(addr_t  ulPhyRam, size_t  stSize);
                                                                        /*  添加物理内存用于 APP        */
/*********************************************************************************************************
  MMU 启动与停止
*********************************************************************************************************/

LW_API VOID         API_VmmMmuEnable(VOID);                             /*  MMU 启动                    */

LW_API VOID         API_VmmMmuDisable(VOID);                            /*  MMU 停止                    */

/*********************************************************************************************************
  VMM API (以下分配函数可以分配出确定的, 可供直接访问的内存空间)
*********************************************************************************************************/

LW_API PVOID        API_VmmMalloc(size_t stSize);                       /*  分配逻辑连续内存, 虚拟地址  */
LW_API PVOID        API_VmmMallocEx(size_t stSize, ULONG ulFlag);       /*  分配逻辑连续内存, 虚拟地址  */
LW_API PVOID        API_VmmMallocAlign(size_t stSize, 
                                       size_t stAlign, 
                                       ULONG  ulFlag);                  /*  分配逻辑连续内存, 虚拟地址  */
LW_API VOID         API_VmmFree(PVOID  pvVirtualMem);                   /*  回收虚拟连续内存            */

LW_API ULONG        API_VmmVirtualToPhysical(addr_t  ulVirtualAddr, 
                                             addr_t *pulPhysicalAddr);  /*  通过虚拟地址获取物理地址    */

LW_API BOOL         API_VmmVirtualIsInside(addr_t  ulAddr);             /*  指定地址是否在管理的虚拟空间*/

LW_API ULONG        API_VmmZoneStatus(ULONG     ulZoneIndex,
                                      addr_t   *pulPhysicalAddr,        /*  0 ~ LW_CFG_VMM_ZONE_NUM - 1 */
                                      size_t   *pstSize,
                                      addr_t   *pulPgd,
                                      ULONG    *pulFreePage,
                                      UINT     *puiAttr);               /*  获得物理区域的信息          */

LW_API ULONG        API_VmmVirtualStatus(UINT32   uiType,               /*  LW_VIRTUAL_MEM_APP / DEV    */
                                         ULONG    ulZoneIndex,          /*  0 ~ LW_CFG_VMM_VIR_NUM - 1  */
                                         addr_t  *pulVirtualAddr,
                                         size_t  *pulSize,
                                         ULONG   *pulFreePage);         /*  获得虚拟空间信息            */

LW_API VOID         API_VmmPhysicalKernelDesc(PLW_MMU_PHYSICAL_DESC  pphydescText, 
                                              PLW_MMU_PHYSICAL_DESC  pphydescData);
                                                                        /*  获得内核 TEXT DATA 段描述   */
/*********************************************************************************************************
  VMM 扩展操作
  
  仅开辟虚拟空间, 当出现第一次访问时, 将通过缺页中断分配物理内存, 当缺页中断中无法分配物理页面时, 将收到
  SIGSEGV 信号并结束线程. 
  
  API_VmmRemapArea() 仅供驱动程序使用, 方法类似于 linux remap_pfn_range() 函数.
*********************************************************************************************************/

LW_API PVOID        API_VmmMallocArea(size_t stSize, FUNCPTR  pfuncFiller, PVOID  pvArg);
                                                                        /*  分配逻辑连续内存, 虚拟地址  */
                                                                        
LW_API PVOID        API_VmmMallocAreaEx(size_t stSize, FUNCPTR  pfuncFiller, PVOID  pvArg, 
                                        INT  iFlags, ULONG ulFlag);     /*  分配逻辑连续内存, 虚拟地址  */
                                                                        
LW_API PVOID        API_VmmMallocAreaAlign(size_t stSize, size_t stAlign, 
                                           FUNCPTR  pfuncFiller, PVOID  pvArg, 
                                           INT  iFlags, ULONG  ulFlag); /*  分配逻辑连续内存, 虚拟地址  */
                                                                        
LW_API VOID         API_VmmFreeArea(PVOID  pvVirtualMem);               /*  回收虚拟连续内存            */

LW_API ULONG        API_VmmExpandArea(PVOID  pvVirtualMem, size_t  stExpSize);
                                                                        /*  扩展虚拟内存分区            */
LW_API PVOID        API_VmmSplitArea(PVOID  pvVirtualMem, size_t  stSize);
                                                                        /*  拆分虚拟内存分区            */
LW_API ULONG        API_VmmMergeArea(PVOID  pvVirtualMem1, PVOID  pvVirtualMem2);
                                                                        /*  合并虚拟内存分区            */
LW_API ULONG        API_VmmMoveArea(PVOID  pvVirtualTo, PVOID  pvVirtualFrom);
                                                                        /*  移动虚拟内存分区            */
LW_API ULONG        API_VmmPCountInArea(PVOID  pvVirtualMem, ULONG  *pulPageNum);
                                                                        /*  统计缺页中断分配的内存页面  */

LW_API ULONG        API_VmmRemapArea(PVOID  pvVirtualAddr, PVOID  pvPhysicalAddr, 
                                     size_t stSize, ULONG  ulFlag,
                                     FUNCPTR pfuncFiller, PVOID  pvArg);/*  建立新的映射关系            */
                                                                        /*  仅供驱动程序 mmap 使用此函数*/
LW_API ULONG        API_VmmInvalidateArea(PVOID  pvVirtualMem, 
                                          PVOID  pvSubMem, 
                                          size_t stSize);               /*  释放物理内存, 保留虚拟空间  */
                                          
LW_API VOID         API_VmmAbortStatus(PLW_VMM_STATUS  pvmms);          /*  获得访问中止统计信息        */
       
/*********************************************************************************************************
  VMM 对于 loader 或者其他内核模块提供的共享段支持 (仅供 loader 或其他 SylixOS 内核服务自己使用)
*********************************************************************************************************/

LW_API ULONG        API_VmmSetFiller(PVOID  pvVirtualMem, FUNCPTR  pfuncFiller, PVOID  pvArg);
                                                                        /*  设置填充函数                */
LW_API ULONG        API_VmmSetFindShare(PVOID  pvVirtualMem, PVOIDFUNCPTR  pfuncFindShare, PVOID  pvArg);
                                                                        /*  设置查询共享函数            */
LW_API ULONG        API_VmmPreallocArea(PVOID       pvVirtualMem, 
                                        PVOID       pvSubMem, 
                                        size_t      stSize, 
                                        FUNCPTR     pfuncTempFiller, 
                                        PVOID       pvTempArg,
                                        ULONG       ulFlag);            /*  预分配物理内存页面          */
                                        
LW_API ULONG        API_VmmShareArea(PVOID      pvVirtualMem1, 
                                     PVOID      pvVirtualMem2,
                                     size_t     stStartOft1, 
                                     size_t     stStartOft2, 
                                     size_t     stSize,
                                     BOOL       bExecEn,
                                     FUNCPTR    pfuncTempFiller, 
                                     PVOID      pvTempArg);             /*  设置共享区域                */

/*********************************************************************************************************
  内核专用 API 用户慎用 
  
  仅可以修改虚拟内存区分配出的页面, 修改大小已分配时的大小为基准
*********************************************************************************************************/

LW_API ULONG        API_VmmSetFlag(PVOID  pvVirtualAddr, 
                                   ULONG  ulFlag);                      /*  设置虚拟地址权限            */
                                   
LW_API ULONG        API_VmmGetFlag(PVOID  pvVirtualAddr, 
                                   ULONG *pulFlag);                     /*  获取虚拟地址权限            */

/*********************************************************************************************************
  应用程序堆栈分配专用 API (会多分配一页作为堆栈保护页面)
*********************************************************************************************************/

LW_API PVOID        API_VmmStackAlloc(size_t  stSize);                  /*  分配堆栈                    */

LW_API VOID         API_VmmStackFree(PVOID  pvVirtualMem);              /*  释放堆栈                    */

/*********************************************************************************************************
  以下 API 只负责分配物理内存, 并没有产生映射关系. 不能直接使用, 必须通过虚拟内存映射才能使用.
*********************************************************************************************************/

LW_API PVOID        API_VmmPhyAlloc(size_t stSize);                     /*  分配物理内存                */
LW_API PVOID        API_VmmPhyAllocEx(size_t  stSize, UINT  uiAttr);    /*  与上相同, 但可以指定内存属性*/
LW_API PVOID        API_VmmPhyAllocAlign(size_t stSize, 
                                         size_t stAlign,
                                         UINT   uiAttr);                /*  分配物理内存, 指定对齐关系  */
LW_API VOID         API_VmmPhyFree(PVOID  pvPhyMem);                    /*  释放物理内存                */

/*********************************************************************************************************
  以下 API 只允许驱动程序使用
  
  no cache 区域操作 (dma 操作返回的是直接的物理地址, 即平板映射, 并且在 LW_ZONE_ATTR_DMA 域中)
*********************************************************************************************************/

LW_API PVOID        API_VmmDmaAlloc(size_t  stSize);                    /*  分配物理连续内存, 物理地址  */
LW_API PVOID        API_VmmDmaAllocAlign(size_t stSize, size_t stAlign);/*  与上相同, 但可以指定对齐关系*/
LW_API PVOID        API_VmmDmaAllocAlignWithFlags(size_t  stSize, size_t  stAlign, ULONG  ulFlags);
                                                                        /*  与上相同, 但可以指定内存类型*/
LW_API VOID         API_VmmDmaFree(PVOID  pvDmaMem);                    /*  回收 DMA 内存缓冲区         */
#endif                                                                  /*  __SYLIXOS_KERNEL            */

/*********************************************************************************************************
  无 VMM 支持
*********************************************************************************************************/
#else

#ifdef __SYLIXOS_KERNEL
static LW_INLINE ULONG  API_VmmVirtualToPhysical (addr_t  ulVirtualAddr, addr_t *pulPhysicalAddr)
{
    if (pulPhysicalAddr) {
        *pulPhysicalAddr = ulVirtualAddr;
    }
    
    return  (ERROR_NONE);
}
#endif                                                                  /*  __SYLIXOS_KERNEL            */

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  API_VmmAbortIsr() 为异常处理函数, 只允许 ARCH 代码使用. 
  没有使能 VMM SylixOS 依然可以处理异常情况
  
  当异常类型为 LW_VMM_ABORT_TYPE_FPE 浮点异常时: 
  VMABT_uiMethod 取值为:
  
  FPE_INTDIV            1  Integer divide by zero
  FPE_INTOVF            2  Integer overflow
  FPE_FLTDIV            3  Floating-point divide by zero
  FPE_FLTOVF            4  Floating-point overflow
  FPE_FLTUND            5  Floating-point underflow
  FPE_FLTRES            6  Floating-point inexact result
  FPE_FLTINV            7  inval floating point operate
  FPE_FLTSUB            8  Subscript out of range
  
  当异常类型为 LW_VMM_ABORT_TYPE_BUS 浮点异常时: 
  
  BUS_ADRALN            1  Invalid address alignment
  BUS_ADRERR            2  Nonexistent physical memory
  BUS_OBJERR            3  Object-specific hardware err
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL

typedef struct __lw_vmm_abort {

#define LW_VMM_ABORT_TYPE_NOINFO            0                           /*  内部使用                    */
#define LW_VMM_ABORT_TYPE_TERMINAL          1                           /*  体系结构相关错误            */
#define LW_VMM_ABORT_TYPE_MAP               2                           /*  页表映射错误 (MMU 报告)     */
                                                                        /*  需要 uiMethod 标记访问类型  */
#define LW_VMM_ABORT_TYPE_PERM              3                           /*  访问权限错误 (MMU 报告)     */
                                                                        /*  需要 uiMethod 标记访问类型  */
#define LW_VMM_ABORT_TYPE_FPE               4                           /*  浮点运算器异常              */
#define LW_VMM_ABORT_TYPE_BUS               5                           /*  总线访问异常                */
#define LW_VMM_ABORT_TYPE_BREAK             6                           /*  断点异常                    */
#define LW_VMM_ABORT_TYPE_SYS               7                           /*  系统调用异常                */
#define LW_VMM_ABORT_TYPE_UNDEF             8                           /*  未定义指令, 将产生 SIGILL   */

#define LW_VMM_ABORT_TYPE_FATAL_ERROR       0xffffffff                  /*  致命错误, 需要立即重启      */

    UINT32               VMABT_uiType;
    
#define LW_VMM_ABORT_METHOD_READ            1                           /*  读访问                      */
#define LW_VMM_ABORT_METHOD_WRITE           2                           /*  写访问                      */
#define LW_VMM_ABORT_METHOD_EXEC            3                           /*  执行访问                    */
    
    UINT32               VMABT_uiMethod;
} LW_VMM_ABORT;
typedef LW_VMM_ABORT    *PLW_VMM_ABORT;

LW_API VOID         API_VmmAbortIsr(addr_t          ulRetAddr,
                                    addr_t          ulAbortAddr, 
                                    PLW_VMM_ABORT   pabtInfo,
                                    PLW_CLASS_TCB   ptcb);              /*  异常中断服务函数            */

#endif                                                                  /*  __SYLIXOS_KERNEL            */
/*********************************************************************************************************
  vmm api macro
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0

#ifdef __SYLIXOS_KERNEL
#define vmmMalloc                   API_VmmMalloc
#define vmmMallocEx                 API_VmmMallocEx
#define vmmMallocAlign              API_VmmMallocAlign
#define vmmFree                     API_VmmFree

#define vmmMallocArea               API_VmmMallocArea
#define vmmMallocAreaEx             API_VmmMallocAreaEx
#define vmmMallocAreaAlign          API_VmmMallocAreaAlign
#define vmmFreeArea                 API_VmmFreeArea
#define vmmPCountInArea             API_VmmPCountInArea
#define vmmRemapArea                API_VmmRemapArea
#define vmmInvalidateArea           API_VmmInvalidateArea
#define vmmAbortStatus              API_VmmAbortStatus

#define vmmStackAlloc               API_VmmStackAlloc
#define vmmStackFree                API_VmmStackFree

#define vmmPhyAlloc                 API_VmmPhyAlloc
#define vmmPhyAllocEx               API_VmmPhyAllocEx
#define vmmPhyAllocAlign            API_VmmPhyAllocAlign
#define vmmPhyFree                  API_VmmPhyFree

#define vmmDmaAlloc                 API_VmmDmaAlloc                     /*  返回值为 物理地址           */
#define vmmDmaAllocAlign            API_VmmDmaAllocAlign                /*  返回值为 物理地址           */
#define vmmDmaAllocAlignWithFlags   API_VmmDmaAllocAlignWithFlags
#define vmmDmaFree                  API_VmmDmaFree

#define vmmMap                      API_VmmMap
#define vmmVirtualToPhysical        API_VmmVirtualToPhysical
#define vmmPhysicalToVirtual        API_VmmPhysicalToVirtual            /*  仅支持VMM管理的物理内存查询 */
#define vmmVirtualIsInside          API_VmmVirtualIsInside

#define vmmSetFlag                  API_VmmSetFlag
#define vmmGetFlag                  API_VmmGetFlag

#define vmmZoneStatus               API_VmmZoneStatus
#define vmmVirtualStatus            API_VmmVirtualStatus
#define vmmPhysicalKernelDesc       API_VmmPhysicalKernelDesc
#endif                                                                  /*  __SYLIXOS_KERNEL            */

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
#endif                                                                  /*  __VMM_H                     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
