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
** 文   件   名: loader_lib.h
**
** 创   建   人: Jiang.Taijin (蒋太金)
**
** 文件创建日期: 2010 年 04 月 17 日
**
** 描        述: 公用头文件

** BUG:
2011.02.20  为兼容 posix 动态链接库标准, 加入对符号全局性与局部性的处理.
2012.09.21  兼容 BSD 程序, 加入了对 issetugid 的功能.
2012.10.25  加入进程 I/O 环境.
2012.12.09  加入进程树功能.
2012.12.18  加入进程私有 FD 表.
2014.05.13  加入子线程链表.
2014.09.30  加入进程定时器.
2015.09.01  加入 MIPS 架构
*********************************************************************************************************/

#ifndef __LOADER_LIB_H
#define __LOADER_LIB_H

#include "../elf/elf_type.h"
#include "loader_vppatch.h"

/*********************************************************************************************************
  模块运行时占用的内存段，卸载时需释放这些段
*********************************************************************************************************/

typedef struct {
    addr_t                  ESEG_ulAddr;                                /*  内存段地址                  */
    size_t                  ESEG_stLen;                                 /*  内存段长度                  */
#if (LW_CFG_TRUSTED_COMPUTING_EN > 0) || defined(LW_CFG_CPU_ARCH_C6X)   /*  C6X 使用SEGMENT实现backtrace*/
    BOOL                    ESEG_bCanExec;                              /*  是否可执行                  */
#endif                                                                  /*  LW_CFG_TRUSTED_COMPUTING_EN */
} LW_LD_EXEC_SEGMENT;

/*********************************************************************************************************
  模块符号表缓冲
*********************************************************************************************************/

typedef struct {
    LW_LIST_LINE            ESYM_lineManage;                            /*  管理链表                    */
    size_t                  ESYM_stSize;                                /*  分段缓冲大小                */
    size_t                  ESYM_stUsed;                                /*  本分段已经使用的内存数量    */
} LW_LD_EXEC_SYMBOL;

#define LW_LD_EXEC_SYMBOL_HDR_SIZE  ROUND_UP(sizeof(LW_LD_EXEC_SYMBOL), sizeof(LW_STACK))

/*********************************************************************************************************
  模块类型，KO\SO
*********************************************************************************************************/

#define LW_LD_MOD_TYPE_KO           0                                   /*  内核模块                    */
#define LW_LD_MOD_TYPE_SO           1                                   /*  应用程序或动态链接库        */

/*********************************************************************************************************
  模块运行状态
*********************************************************************************************************/

#define LW_LD_STATUS_UNLOAD         0                                   /*  未加载                      */
#define LW_LD_STATUS_LOADED         1                                   /*  已加载未初始化              */
#define LW_LD_STATUS_INITED         2                                   /*  已初始化                    */
#define LW_LD_STATUS_FINIED         3                                   /*  已初始化                    */

/*********************************************************************************************************
  模块结构体用于组织模块的信息
*********************************************************************************************************/

#define __LW_LD_EXEC_MODULE_MAGIC   0x25ef68af

#ifdef LW_CFG_CPU_ARCH_MIPS
/*********************************************************************************************************
  MIPS_HI16_RELOC_INFO 提供一种方法把 mipsElfHI16RelocateRela() 信息传递给 mipsElfLO16RelocateRela()
*********************************************************************************************************/
struct __MIPS_HI16_RELOC_INFO;
typedef struct __MIPS_HI16_RELOC_INFO  MIPS_HI16_RELOC_INFO, *PMIPS_HI16_RELOC_INFO;

struct __MIPS_HI16_RELOC_INFO {
    Elf_Addr 			   *HI16_pAddr;
    Elf_Addr                HI16_valAddr;
    PMIPS_HI16_RELOC_INFO   HI16_pNext;
};
#endif                                                                  /*  LW_CFG_CPU_ARCH_MIPS        */

/*********************************************************************************************************
  内核模块 atexit 函数
*********************************************************************************************************/

typedef struct {
    LW_LIST_MONO            EMODAE_pmonoNext;
    VOIDFUNCPTR             EMODAE_pfunc;                               /*  atexit 调用函数             */
} LW_LD_EXEC_MODATEXIT;

/*********************************************************************************************************
  模块
*********************************************************************************************************/

typedef struct {
    ULONG                   EMOD_ulMagic;                               /*  用于识别本结构体            */
    ULONG                   EMOD_ulModType;                             /*  模块类型，KO还是SO文件      */
    ULONG                   EMOD_ulStatus;                              /*  模块当前运行状态            */
    ULONG                   EMOD_ulRefs;                                /*  模块引用计数                */
    PVOID                   EMOD_pvUsedArr;                             /*  依赖的模块数组              */
    ULONG                   EMOD_ulUsedCnt;                             /*  依赖模块数组大小            */

    PCHAR                   EMOD_pcSymSection;                          /*  仅加载指定 section 符号表   */

    VOIDFUNCPTR            *EMOD_ppfuncInitArray;                       /*  初始化函数数组              */
    ULONG                   EMOD_ulInitArrCnt;                          /*  初始化函数个数              */
    VOIDFUNCPTR            *EMOD_ppfuncFiniArray;                       /*  结束函数数组                */
    ULONG                   EMOD_ulFiniArrCnt;                          /*  结束函数个数                */

    PCHAR                   EMOD_pcInit;                                /*  初始化函数名称              */
    FUNCPTR                 EMOD_pfuncInit;                             /*  初始化函数指针              */

    PCHAR                   EMOD_pcExit;                                /*  结束函数名称                */
    FUNCPTR                 EMOD_pfuncExit;                             /*  结束函数指针                */

    PCHAR                   EMOD_pcEntry;                               /*  入口函数名称                */
    FUNCPTR                 EMOD_pfuncEntry;                            /*  main函数指针                */
    BOOL                    EMOD_bIsSymbolEntry;                        /*  是否为符号入口              */

    size_t                  EMOD_stLen;                                 /*  为模块分配的内存长度        */
    PVOID                   EMOD_pvBaseAddr;                            /*  为模块分配的内存地址        */

    BOOL                    EMOD_bIsGlobal;                             /*  是否为全局符号              */
    ULONG                   EMOD_ulSymCount;                            /*  导出符号数目                */
    ULONG                   EMOD_ulSymHashSize;                         /*  符号 hash 表大小            */
    LW_LIST_LINE_HEADER    *EMOD_psymbolHash;                           /*  导出符号列 hash 表          */
    LW_LIST_LINE_HEADER     EMOD_plineSymbolBuffer;                     /*  导出符号表缓冲              */

    ULONG                   EMOD_ulSegCount;                            /*  内存段数目                  */
    LW_LD_EXEC_SEGMENT     *EMOD_psegmentArry;                          /*  内存段列表                  */
    BOOL                    EMOD_bExportSym;                            /*  是否导出符号                */
    PCHAR                   EMOD_pcModulePath;                          /*  模块文件路径                */

    LW_LD_VPROC            *EMOD_pvproc;                                /*  所属进程                    */
    LW_LIST_RING            EMOD_ringModules;                           /*  进程中所有的库链表          */
    PVOID                   EMOD_pvFormatInfo;                          /*  重定位相关信息              */

    LW_LIST_MONO_HEADER     EMOD_pmonoAtexit;                           /*  内核模块 atexit             */

#ifdef LW_CFG_CPU_ARCH_ARM
    size_t                  EMOD_stARMExidxCount;                       /*  ARM.exidx 段长度            */
    PVOID                   EMOD_pvARMExidx;                            /*  ARM.exidx 段内存地址        */
#endif                                                                  /*  LW_CFG_CPU_ARCH_ARM         */

#ifdef LW_CFG_CPU_ARCH_MIPS
    MIPS_HI16_RELOC_INFO   *EMOD_pMIPSHi16List;
#endif                                                                  /*  LW_CFG_CPU_ARCH_MIPS        */

#ifdef LW_CFG_CPU_ARCH_C6X
    ULONG                  *EMOD_pulDsbtTable;                          /*  DSP DSBT表位置              */
    ULONG                   EMOD_ulDsbtSize;                            /*  DSP DSBT表大小              */
    ULONG                   EMOD_ulDsbtIndex;                           /*  DSP DSBT表索引              */
#endif                                                                  /*  LW_CFG_CPU_ARCH_C6X         */
} LW_LD_EXEC_MODULE;

/*********************************************************************************************************
  最大依赖库数目
*********************************************************************************************************/
#define __LW_MAX_NEEDED_LIB     64
/*********************************************************************************************************
   解析后的dynamic段数据结构
*********************************************************************************************************/
typedef struct {
    Elf_Sym     *psymTable;                                             /*  符号表指针                  */
    ULONG        ulSymCount;                                            /*  符号数目                    */
    PCHAR        pcStrTable;                                            /*  字符串表指针                */
    Elf_Rel     *prelTable;                                             /*  rel重定位表指针             */
    ULONG        ulRelSize;                                             /*  重定位表大小                */
    ULONG        ulRelCount;                                            /*  重定位表项数目              */
    Elf_Rela    *prelaTable;                                            /*  rel重定位表指针             */
    ULONG        ulRelaSize;                                            /*  重定位表大小                */
    ULONG        ulRelaCount;                                           /*  重定位表项数目              */
    Elf_Hash    *phash;                                                 /*  hash表指针                  */
    PCHAR        pvJmpRTable;                                           /*  plt重定位表指针             */
    ULONG        ulPltRel;                                              /*  plt重定位表项类型           */
    ULONG        ulJmpRSize;                                            /*  plt重定位表大小             */
    Elf_Addr    *paddrInitArray;                                        /*  初始化函数数组              */
    Elf_Addr    *paddrFiniArray;                                        /*  结束函数数组                */
    ULONG        ulInitArrSize;                                         /*  初始化函数数组大小          */
    ULONG        ulFiniArrSize;                                         /*  结束函数数组大小            */
    Elf_Addr     addrInit;                                              /*  初始化函数                  */
    Elf_Addr     addrFini;                                              /*  结束函数                    */
    Elf_Addr     addrMin;                                               /*  最小虚拟地址                */
    Elf_Addr     addrMax;                                               /*  最大虚拟地址                */
    Elf_Word     wdNeededArr[__LW_MAX_NEEDED_LIB];
    ULONG        ulNeededCnt;
    Elf_Addr    *ulPltGotAddr;                                          /*  全局GOT的地址               */
#ifdef  LW_CFG_CPU_ARCH_MIPS
    ULONG        ulMIPSGotSymIdx;                                       /*  Dynsym 第一个GOT入口        */
    ULONG        ulMIPSLocalGotNumIdx;                                  /*  MIPS Local GOT 入口数量     */
    ULONG        ulMIPSSymNumIdx;                                       /*  MIPS Dynsym 入口数量        */
    ULONG        ulMIPSPltGotIdx;                                       /*  MIPS .got.plt 地址          */
#endif                                                                  /*  LW_CFG_CPU_ARCH_MIPS        */
} ELF_DYN_DIR;

/*********************************************************************************************************
  module 操作锁
*********************************************************************************************************/

extern LW_OBJECT_HANDLE     _G_ulVProcMutex;

#define LW_LD_LOCK()        API_SemaphoreMPend(_G_ulVProcMutex, LW_OPTION_WAIT_INFINITE)
#define LW_LD_UNLOCK()      API_SemaphoreMPost(_G_ulVProcMutex)

/*********************************************************************************************************
  调试信息打印
*********************************************************************************************************/

#ifdef  __SYLIXOS_DEBUG
#define LD_DEBUG_MSG(msg)   printf msg
#else
#define LD_DEBUG_MSG(msg)
#endif                                                                  /*  __LOAD_DEBUG                */

/*********************************************************************************************************
  进程默认入口与应用入口
*********************************************************************************************************/

#define LW_LD_DEFAULT_ENTRY         "_start"                            /*  进程初始化入口符号          */
#define LW_LD_PROCESS_ENTRY         "main"                              /*  进程主入口符号              */

/*********************************************************************************************************
  小容量内存管理
*********************************************************************************************************/

#define LW_LD_SAFEMALLOC(size)      __SHEAP_ALLOC((size_t)size)
#define LW_LD_SAFEFREE(a)           { if (a) { __SHEAP_FREE((PVOID)a); a = 0; } }
                                                                        /*  安全释放                    */

/*********************************************************************************************************
  内核模块大容量内存管理
*********************************************************************************************************/

PVOID __ldMalloc(size_t  stLen);                                        /*  分配内存                    */
PVOID __ldMallocAlign(size_t  stLen, size_t  stAlign);
VOID  __ldFree(PVOID  pvAddr);                                          /*  释放内存                    */

#define LW_LD_VMSAFEMALLOC(size)    \
        __ldMalloc(size)
        
#define LW_LD_VMSAFEMALLOC_ALIGN(size, align)   \
        __ldMallocAlign(size, align)
        
#define LW_LD_VMSAFEFREE(a) \
        { if (a) { __ldFree((PVOID)a); a = 0; } }

/*********************************************************************************************************
  共享库大容量内存管理
*********************************************************************************************************/

extern LW_OBJECT_HANDLE             _G_ulExecShareLock;                 /*  共享库内存段锁              */

PVOID  __ldMallocArea(size_t  stLen);
PVOID  __ldMallocAreaAlign(size_t  stLen, size_t  stAlign);
VOID   __ldFreeArea(PVOID  pvAddr);

#define LW_LD_VMSAFEMALLOC_AREA(size)   \
        __ldMallocArea(size)
        
#define LW_LD_VMSAFEMALLOC_AREA_ALIGN(size, align)  \
        __ldMallocAreaAlign(size, align)
        
#define LW_LD_VMSAFEFREE_AREA(a)    \
        { if (a) { __ldFreeArea((PVOID)a); a = 0; } }
        
INT     __ldMmap(PVOID  pvBase, size_t  stAddrOft, INT  iFd, struct stat64 *pstat64,
                 off_t  oftOffset, size_t  stLen,  BOOL  bCanShare, BOOL  bCanExec);
VOID    __ldShare(PVOID  pvBase, size_t  stLen, dev_t  dev, ino64_t ino64);
VOID    __ldShareAbort(dev_t  dev, ino64_t  ino64);
INT     __ldShareConfig(BOOL  bShareEn, BOOL  *pbPrev);

#define LW_LD_VMSAFEMAP_AREA(base, addr_offset, fd, pstat64, file_offset, len, can_share, can_exec) \
        __ldMmap(base, addr_offset, fd, pstat64, file_offset, len, can_share, can_exec)
        
#define LW_LD_VMSAFE_SHARE(base, len, dev, ino64) \
        __ldShare(base, len, dev, ino64)
        
#define LW_LD_VMSAFE_SHARE_ABORT(dev, ino64)    \
        __ldShareAbort(dev, ino64)
        
#define LW_LD_VMSAFE_SHARE_CONFIG(can_share, prev)  \
        __ldShareConfig(can_share, prev)

/*********************************************************************************************************
  地址转换
*********************************************************************************************************/

#define LW_LD_V2PADDR(vBase, pBase, vAddr)      \
        ((size_t)pBase + (size_t)vAddr - (size_t)vBase)                 /*  计算物理地址                */ 

#define LW_LD_P2VADDR(vBase, pBase, pAddr)      \
        ((size_t)vBase + (size_t)pAddr - (size_t)pBase)                 /*  计算虚拟地址                */
        
/*********************************************************************************************************
  vp 补丁操作
*********************************************************************************************************/
#define LW_LD_VMEM_MAX      64

PCHAR __moduleVpPatchVersion(LW_LD_EXEC_MODULE *pmodule);               /*  获得补丁版本                */

#if LW_CFG_VMM_EN == 0
PVOID __moduleVpPatchHeap(LW_LD_EXEC_MODULE *pmodule);                  /*  获得补丁独立内存堆          */
#endif                                                                  /*  LW_CFG_VMM_EN == 0          */

INT   __moduleVpPatchVmem(LW_LD_EXEC_MODULE *pmodule, PVOID  ppvArea[], INT  iSize);
                                                                        /*  获得进程虚拟内存空间        */
VOID  __moduleVpPatchInit(LW_LD_EXEC_MODULE *pmodule);                  /*  进程补丁构造与析构          */
VOID  __moduleVpPatchFini(LW_LD_EXEC_MODULE *pmodule);
/*********************************************************************************************************
  体系结构特有函数
*********************************************************************************************************/

#ifdef LW_CFG_CPU_ARCH_ARM
typedef long unsigned int   *_Unwind_Ptr;
_Unwind_Ptr dl_unwind_find_exidx(_Unwind_Ptr pc, int *pcount, PVOID *pvVProc);
#endif                                                                  /*  LW_CFG_CPU_ARCH_ARM         */

#endif                                                                  /*  __LOADER_LIB_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
