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
** 文   件   名: x86Cache.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 07 月 18 日
**
** 描        述: x86 体系构架 CACHE 驱动.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
#include "arch/x86/common/x86Topology.h"
/*********************************************************************************************************
  宏定义
*********************************************************************************************************/
#define L1_CACHE_I_EN   0x01
#define L1_CACHE_D_EN   0x02
#define L1_CACHE_EN     (L1_CACHE_I_EN | L1_CACHE_D_EN)
#define L1_CACHE_DIS    0x00
/*********************************************************************************************************
  全局变量定义
*********************************************************************************************************/
static INT      _G_iX86CacheStatus    = L1_CACHE_DIS;                   /*  L1 CACHE 状态               */
static FUNCPTR  _G_pfuncX86CacheFlush = LW_NULL;                        /*  CACHE FLUSH 函数指针        */
/*********************************************************************************************************
  外部函数声明
*********************************************************************************************************/
extern VOID  x86CacheEnableHw(VOID);
extern VOID  x86CacheDisableHw(VOID);
extern VOID  x86CacheResetHw(VOID);
extern VOID  x86CacheFlushX86Hw(VOID);
extern VOID  x86CacheFlushPen4Hw(PVOID  pvAdrs, size_t  stBytes);
extern VOID  x86CacheClearPen4Hw(PVOID  pvAdrs, size_t  stBytes);
/*********************************************************************************************************
** 函数名称: x86CacheEnableNone
** 功能描述: 使能 CACHE (伪函数)
** 输　入  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  x86CacheEnableNone (LW_CACHE_TYPE  cachetype)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: x86CacheEnable
** 功能描述: 使能 CACHE
** 输　入  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  x86CacheEnable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        if (LW_CPU_GET_CUR_ID() == 0) {
            _G_iX86CacheStatus |= L1_CACHE_I_EN;
        }
    } else {
        if (LW_CPU_GET_CUR_ID() == 0) {
            _G_iX86CacheStatus |= L1_CACHE_D_EN;
        }
    }

    if (_G_iX86CacheStatus == L1_CACHE_EN) {
        x86CacheEnableHw();
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: x86CacheDisableNone
** 功能描述: 禁能 CACHE (伪函数)
** 输　入  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  x86CacheDisableNone (LW_CACHE_TYPE  cachetype)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: x86CacheDisable
** 功能描述: 禁能 CACHE
** 输　入  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  x86CacheDisable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        if (LW_CPU_GET_CUR_ID() == 0) {
            _G_iX86CacheStatus &= ~L1_CACHE_I_EN;
        }
    } else {
        if (LW_CPU_GET_CUR_ID() == 0) {
            _G_iX86CacheStatus &= ~L1_CACHE_D_EN;
        }
    }

    if (_G_iX86CacheStatus == L1_CACHE_DIS) {
        x86CacheDisableHw();
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: x86CacheFlushNone
** 功能描述: CACHE 脏数据回写 (伪函数)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  x86CacheFlushNone (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: x86CacheFlush
** 功能描述: CACHE 脏数据回写
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT	x86CacheFlushX86 (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    if (cachetype == DATA_CACHE) {
        X86_WBINVD();
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: x86CacheFlushPen4
** 功能描述: CACHE 脏数据回写 奔腾4以上专用
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  x86CacheFlushPen4 (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    if (cachetype == DATA_CACHE) {
        x86CacheFlushPen4Hw(pvAdrs, stBytes);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: x86CacheFlushPageNone
** 功能描述: CACHE 脏数据回写 (伪函数)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           pvPdrs        物理地址
**           stBytes       长度
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  x86CacheFlushPageNone (LW_CACHE_TYPE  cachetype,
                                   PVOID          pvAdrs,
                                   PVOID          pvPdrs,
                                   size_t         stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: x86CacheFlushPage
** 功能描述: CACHE 脏数据回写
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           pvPdrs        物理地址
**           stBytes       长度
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  x86CacheFlushPageX86 (LW_CACHE_TYPE  cachetype,
                                  PVOID          pvAdrs,
                                  PVOID          pvPdrs,
                                  size_t         stBytes)
{
    if (cachetype == DATA_CACHE) {
        X86_WBINVD();
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: x86CacheFlushPagePen4
** 功能描述: CACHE 脏数据回写 奔腾4以上专用
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           pvPdrs        物理地址
**           stBytes       长度
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT	x86CacheFlushPagePen4 (LW_CACHE_TYPE  cachetype,
                                   PVOID          pvAdrs,
                                   PVOID          pvPdrs,
                                   size_t         stBytes)
{
    if (cachetype == DATA_CACHE) {
        x86CacheFlushPen4Hw(pvAdrs, stBytes);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: x86CacheInvalidateNone
** 功能描述: 指定类型的 CACHE 使部分无效(访问不命中) (伪函数)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  x86CacheInvalidateNone (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: x86CacheInvalidate
** 功能描述: 指定类型的 CACHE 使部分无效(访问不命中)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT	x86CacheInvalidateX86 (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    X86_WBINVD();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: x86CacheInvalidatePen4
** 功能描述: 指定类型的 CACHE 使部分无效(访问不命中) 奔腾4以上专用
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  x86CacheInvalidatePen4 (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    x86CacheClearPen4Hw(pvAdrs, stBytes);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: x86CacheInvalidatePageNone
** 功能描述: 指定类型的 CACHE 使部分无效(访问不命中) (伪函数)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           pvPdrs        物理地址
**           stBytes       长度
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  x86CacheInvalidatePageNone (LW_CACHE_TYPE  cachetype,
                                        PVOID          pvAdrs,
                                        PVOID          pvPdrs,
                                        size_t         stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: x86CacheInvalidatePage
** 功能描述: 指定类型的 CACHE 使部分无效(访问不命中)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           pvPdrs        物理地址
**           stBytes       长度
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT	x86CacheInvalidatePageX86 (LW_CACHE_TYPE  cachetype,
                                       PVOID          pvAdrs,
                                       PVOID          pvPdrs,
                                       size_t         stBytes)
{
    X86_WBINVD();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: x86CacheInvalidatePage
** 功能描述: 指定类型的 CACHE 使部分无效(访问不命中) 奔腾4以上专用
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           pvPdrs        物理地址
**           stBytes       长度
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  x86CacheInvalidatePagePen4 (LW_CACHE_TYPE  cachetype,
                                        PVOID          pvAdrs,
                                        PVOID          pvPdrs,
                                        size_t         stBytes)
{
    x86CacheClearPen4Hw(pvAdrs, stBytes);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: x86CacheClearNone
** 功能描述: 指定类型的 CACHE 使部分或全部清空(回写内存)并无效(访问不命中) (伪函数)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  x86CacheClearNone (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: x86CacheClear
** 功能描述: 指定类型的 CACHE 使部分或全部清空(回写内存)并无效(访问不命中)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT	x86CacheClearX86 (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    X86_WBINVD();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: x86CacheClearPen4
** 功能描述: 指定类型的 CACHE 使部分或全部清空(回写内存)并无效(访问不命中) 奔腾4以上专用
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  x86CacheClearPen4 (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    x86CacheClearPen4Hw(pvAdrs, stBytes);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: x86CacheClearPageNone
** 功能描述: 指定类型的 CACHE 使部分或全部清空(回写内存)并无效(访问不命中)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           pvPdrs        物理地址
**           stBytes       长度
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  x86CacheClearPageNone (LW_CACHE_TYPE  cachetype,
                                   PVOID          pvAdrs,
                                   PVOID          pvPdrs,
                                   size_t         stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: x86CacheClearPage
** 功能描述: 指定类型的 CACHE 使部分或全部清空(回写内存)并无效(访问不命中)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           pvPdrs        物理地址
**           stBytes       长度
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT	x86CacheClearPageX86 (LW_CACHE_TYPE  cachetype,
                                  PVOID          pvAdrs,
                                  PVOID          pvPdrs,
                                  size_t         stBytes)
{
    X86_WBINVD();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: x86CacheClearPagePen4
** 功能描述: 指定类型的 CACHE 使部分或全部清空(回写内存)并无效(访问不命中)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           pvPdrs        物理地址
**           stBytes       长度
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  x86CacheClearPagePen4 (LW_CACHE_TYPE  cachetype,
                                   PVOID          pvAdrs,
                                   PVOID          pvPdrs,
                                   size_t         stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: x86CacheLock
** 功能描述: 锁定指定类型的 CACHE 
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT	x86CacheLockNone (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: x86CacheUnlock
** 功能描述: 解锁指定类型的 CACHE 
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT	x86CacheUnlockNone (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: x86CacheTextUpdate
** 功能描述: 清空(回写内存) D CACHE 无效(访问不命中) I CACHE
** 输　入  : pvAdrs                        虚拟地址
**           stBytes                       长度
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
** 注  意  : L2 cache 为统一 CACHE 所以 text update 不需要操作 L2 cache.
*********************************************************************************************************/
static INT	x86CacheTextUpdateNone (PVOID  pvAdrs, size_t  stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: x86CacheDataUpdate
** 功能描述: 回写 D CACHE (仅回写 CPU 独享级 CACHE)
** 输　入  : pvAdrs                        虚拟地址
**           stBytes                       长度
**           bInv                          是否为回写无效
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
** 注  意  : L2 cache 为统一 CACHE 所以 data update 不需要操作 L2 cache.
*********************************************************************************************************/
static INT  x86CacheDataUpdateNone (PVOID  pvAdrs, size_t  stBytes, BOOL  bInv)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: x86CacheDataUpdate
** 功能描述: 回写 D CACHE (仅回写 CPU 独享级 CACHE)
** 输　入  : pvAdrs                        虚拟地址
**           stBytes                       长度
**           bInv                          是否为回写无效
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
** 注  意  : L2 cache 为统一 CACHE 所以 data update 不需要操作 L2 cache.
*********************************************************************************************************/
static INT  x86CacheDataUpdateX86 (PVOID  pvAdrs, size_t  stBytes, BOOL  bInv)
{
    X86_WBINVD();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: x86CacheDataUpdate
** 功能描述: 回写 D CACHE (仅回写 CPU 独享级 CACHE) 奔腾4以上专用
** 输　入  : pvAdrs                        虚拟地址
**           stBytes                       长度
**           bInv                          是否为回写无效
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
** 注  意  : L2 cache 为统一 CACHE 所以 data update 不需要操作 L2 cache.
*********************************************************************************************************/
static INT	x86CacheDataUpdatePen4 (PVOID  pvAdrs, size_t  stBytes, BOOL  bInv)
{
    if (bInv) {
        x86CacheClearPen4Hw(pvAdrs, stBytes);
    } else {
        x86CacheFlushPen4Hw(pvAdrs, stBytes);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: x86CacheInit
** 功能描述: 初始化 CACHE 
** 输　入  : pcacheop       CACHE 操作函数集
**           uiInstruction  指令 CACHE 参数
**           uiData         数据 CACHE 参数
**           pcMachineName  机器名称
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  x86CacheInit (LW_CACHE_OP  *pcacheop,
                    CACHE_MODE    uiInstruction,
                    CACHE_MODE    uiData,
                    CPCHAR        pcMachineName)
{
    pcacheop->CACHEOP_ulOption = 0ul;                                   /*  无须 TEXT_UPDATE_MP 选项    */

    pcacheop->CACHEOP_iILoc = CACHE_LOCATION_PIPT;                      /*  PIPT                        */
    pcacheop->CACHEOP_iDLoc = CACHE_LOCATION_PIPT;
    
    pcacheop->CACHEOP_iICacheLine     = X86_FEATURE_CACHE_FLUSH_BYTES;
    pcacheop->CACHEOP_iDCacheLine     = X86_FEATURE_CACHE_FLUSH_BYTES;

    pcacheop->CACHEOP_iICacheWaySize  = X86_FEATURE_ICACHE_WAY_SIZE;
    pcacheop->CACHEOP_iDCacheWaySize  = X86_FEATURE_DCACHE_WAY_SIZE;

    /*
     * 默认所有 CACHE 函数都是 NONE 函数
     */
    pcacheop->CACHEOP_pfuncEnable         = x86CacheEnableNone;
    pcacheop->CACHEOP_pfuncDisable        = x86CacheDisableNone;
    
    pcacheop->CACHEOP_pfuncFlush          = x86CacheFlushNone;
    pcacheop->CACHEOP_pfuncFlushPage      = x86CacheFlushPageNone;
    pcacheop->CACHEOP_pfuncInvalidate     = x86CacheInvalidateNone;
    pcacheop->CACHEOP_pfuncInvalidatePage = x86CacheInvalidatePageNone;
    pcacheop->CACHEOP_pfuncClear          = x86CacheClearNone;
    pcacheop->CACHEOP_pfuncClearPage      = x86CacheClearPageNone;
    pcacheop->CACHEOP_pfuncTextUpdate     = x86CacheTextUpdateNone;
    pcacheop->CACHEOP_pfuncDataUpdate     = x86CacheDataUpdateNone;
    
    pcacheop->CACHEOP_pfuncLock           = x86CacheLockNone;           /*  暂时不支持锁定操作          */
    pcacheop->CACHEOP_pfuncUnlock         = x86CacheUnlockNone;
    
#if LW_CFG_VMM_EN > 0
    pcacheop->CACHEOP_pfuncDmaMalloc      = API_VmmDmaAlloc;
    pcacheop->CACHEOP_pfuncDmaMallocAlign = API_VmmDmaAllocAlign;
    pcacheop->CACHEOP_pfuncDmaFree        = API_VmmDmaFree;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

    /*
     * 下面根据特性填充相应的 CACHE 函数
     * i386 没有 CACHE, 但它太古老了, 不对它作支持, 认为都有 CACHE
     */
    pcacheop->CACHEOP_pfuncEnable = x86CacheEnable;

    if (LW_NCPUS == 1) {                                                /*  单核的 CPU                  */
        pcacheop->CACHEOP_pfuncDisable = x86CacheDisable;               /*  才有 CACHE 关闭函数         */
                                                                        /*  和以下的函数                */
        if ((uiData & CACHE_SNOOP_ENABLE) == 0) {                       /*  SNOOP 不使能才需要          */
            if (X86_FEATURE_HAS_CLFLUSH) {                              /*  有 CLFLUSH 指令             */
                pcacheop->CACHEOP_pfuncFlush          = x86CacheFlushPen4;
                pcacheop->CACHEOP_pfuncFlushPage      = x86CacheFlushPagePen4;
                pcacheop->CACHEOP_pfuncInvalidate     = x86CacheInvalidatePen4;
                pcacheop->CACHEOP_pfuncInvalidatePage = x86CacheInvalidatePagePen4;
                pcacheop->CACHEOP_pfuncClear          = x86CacheClearPen4;
                pcacheop->CACHEOP_pfuncClearPage      = x86CacheClearPagePen4;
                pcacheop->CACHEOP_pfuncDataUpdate     = x86CacheDataUpdatePen4;

            } else {                                                    /*  其它型号使用 WBINVD 指令    */
                pcacheop->CACHEOP_pfuncFlush          = x86CacheFlushX86;
                pcacheop->CACHEOP_pfuncFlushPage      = x86CacheFlushPageX86;
                pcacheop->CACHEOP_pfuncInvalidate     = x86CacheInvalidateX86;
                pcacheop->CACHEOP_pfuncInvalidatePage = x86CacheInvalidatePageX86;
                pcacheop->CACHEOP_pfuncClear          = x86CacheClearX86;
                pcacheop->CACHEOP_pfuncClearPage      = x86CacheClearPageX86;
                pcacheop->CACHEOP_pfuncDataUpdate     = x86CacheDataUpdateX86;
            }
        }
    }

    _G_pfuncX86CacheFlush = pcacheop->CACHEOP_pfuncFlush;               /*  记录 FLUSH 函数             */
}
/*********************************************************************************************************
** 函数名称: x86CacheReset
** 功能描述: 复位 CACHE 
** 输　入  : pcMachineName  机器名称
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
** 注  意  : i386 没有 CACHE, 但它太古老了, 不对它作支持, 认为都有 CACHE.
*********************************************************************************************************/
VOID  x86CacheReset (CPCHAR  pcMachineName)
{
#if LW_CFG_SMP_EN == 0
    x86CacheResetHw();
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
}
/*********************************************************************************************************
** 函数名称: x86DCacheFlush
** 功能描述: D-CACHE 回写
** 输　入  : pvStart         开始地址
**           stSize          长度
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  x86DCacheFlush (PVOID  pvStart, size_t  stSize)
{
    if (_G_pfuncX86CacheFlush) {
        _G_pfuncX86CacheFlush(DATA_CACHE, pvStart, stSize);
    }
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
