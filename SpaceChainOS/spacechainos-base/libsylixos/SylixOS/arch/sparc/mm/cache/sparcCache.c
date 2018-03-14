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
** 文   件   名: sparcCache.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2017 年 10 月 10 日
**
** 描        述: SPARC 体系构架 CACHE 驱动.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
/*********************************************************************************************************
  LEON CACHE FLUSH 操作函数指针
*********************************************************************************************************/
static VOID  (*leonFlushICacheAll)(VOID) = LW_NULL;
static VOID  (*leonFlushDCacheAll)(VOID) = LW_NULL;
/*********************************************************************************************************
  LEON2 相关定义
*********************************************************************************************************/
#define LEON2_PREGS           0x80000000
#define LEON2_CCR             (LEON2_PREGS + 0x14)
#define LEON2_LCR             (LEON2_PREGS + 0x24)

#define LEON2_CCR_IEN         (3 << 0)                                  /*  使能 ICACHE                 */
#define LEON2_CCR_DEN         (3 << 2)                                  /*  使能 DCACHE                 */
#define LEON2_CCR_IFP         (1 << 15)                                 /*  ICACHE flush pend           */
#define LEON2_CCR_DFP         (1 << 14)                                 /*  DCACHE flush pend           */
#define LEON2_CCR_IBURST      (1 << 16)                                 /*  使能 Instruction burst fetch*/
#define LEON2_CCR_DSNOOP      (1 << 23)                                 /*  使能 DCACHE SNOOP           */
/*********************************************************************************************************
** 函数名称: leon2FlushICacheAll
** 功能描述: LEON2 FLUSH 所有 ICACHE
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  leon2FlushICacheAll (VOID)
{
    UINT32  uiCCR;

    __asm__ __volatile__ (" flush ");

    while ((uiCCR = read32(LEON2_CCR)) & LEON2_CCR_IFP) {
    }
}
/*********************************************************************************************************
** 函数名称: leon2FlushDCacheAll
** 功能描述: LEON2 FLUSH 所有 DCACHE
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  leon2FlushDCacheAll (VOID)
{
    UINT32  uiCCR;

    __asm__ __volatile__ ("sta %%g0, [%%g0] %0\n\t" : :
                          "i"(ASI_LEON_DFLUSH) : "memory");

    while ((uiCCR = read32(LEON2_CCR)) & LEON2_CCR_DFP) {
    }
}
/*********************************************************************************************************
** 函数名称: leon2CacheEnable
** 功能描述: LEON2 使能 CACHE
** 输　入  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  leon2CacheEnable (LW_CACHE_TYPE  cachetype)
{
    UINT32  uiCCR;

    if (cachetype == INSTRUCTION_CACHE) {
        leon2FlushICacheAll();

        uiCCR  = read32(LEON2_CCR);
        uiCCR |= LEON2_CCR_IEN;                                         /*  使能 ICACHE                 */
        uiCCR |= LEON2_CCR_IBURST;                                      /*  使能 Instruction burst fetch*/
        write32(uiCCR, LEON2_CCR);

    } else {
        leon2FlushDCacheAll();

        uiCCR  = read32(LEON2_CCR);
        uiCCR |= LEON2_CCR_DEN;                                         /*  使能 DCACHE                 */
        uiCCR |= LEON2_CCR_DSNOOP;                                      /*  使能 DCACHE SNOOP           */
        write32(uiCCR, LEON2_CCR);
    }

    return (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: leon2CacheDisable
** 功能描述: LEON2 禁能 CACHE
** 输　入  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
** 注  意  : DCACHE 为写穿透模式, 不用回写.
*********************************************************************************************************/
static INT  leon2CacheDisable (LW_CACHE_TYPE  cachetype)
{
    UINT32  uiCCR = read32(LEON2_CCR);

    if (cachetype == INSTRUCTION_CACHE) {
        uiCCR &= ~(LEON2_CCR_IEN);                                      /*  禁能 ICACHE                 */

    } else {
        uiCCR &= ~(LEON2_CCR_DEN);                                      /*  禁能 ICACHE                 */
    }

    write32(uiCCR, LEON2_CCR);
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: leon2CacheProbe
** 功能描述: LEON2 初始化 CACHE
** 输　入  : pcacheop       CACHE 操作函数集
**           uiInstruction  指令 CACHE 参数
**           uiData         数据 CACHE 参数
**           pcMachineName  机器名称
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  leon2CacheProbe (LW_CACHE_OP *pcacheop,
                              CACHE_MODE   uiInstruction,
                              CACHE_MODE   uiData,
                              CPCHAR       pcMachineName)
{
    pcacheop->CACHEOP_pfuncEnable  = leon2CacheEnable;
    pcacheop->CACHEOP_pfuncDisable = leon2CacheDisable;

    pcacheop->CACHEOP_ulOption = 0;                                     /*  No SMP support!             */

    pcacheop->CACHEOP_iILoc = CACHE_LOCATION_VIVT;
    pcacheop->CACHEOP_iDLoc = CACHE_LOCATION_VIVT;

    pcacheop->CACHEOP_iICacheLine = 32;
    pcacheop->CACHEOP_iDCacheLine = 32;

    pcacheop->CACHEOP_iICacheWaySize = 8 * LW_CFG_KB_SIZE;              /*  32KB 的四路组相联 ICACHE    */
    pcacheop->CACHEOP_iDCacheWaySize = 8 * LW_CFG_KB_SIZE;              /*  16KB 的两路组相联 DCACHE    */

    leonFlushICacheAll = leon2FlushICacheAll;
    leonFlushDCacheAll = leon2FlushDCacheAll;
}
/*********************************************************************************************************
  LEON3 相关定义
*********************************************************************************************************/
typedef struct {
    ULONG   LC_ulCCR;                               /*  0x00 - Cache Control Register                   */
    ULONG   LC_ulICCR;                              /*  0x08 - Instruction Cache Configuration Register */
    ULONG   LC_ulDCCR;                              /*  0x0c - Data Cache Configuration Register        */
} LEON3_CACHE_REGS;                                 /*  LEON3 CACHE 配置寄存器                          */

#define LEON3_CCR_IFP         (1 << 15)                                 /*  ICACHE flush pend           */
#define LEON3_CCR_DFP         (1 << 14)                                 /*  DCACHE flush pend           */
/*********************************************************************************************************
** 函数名称: leon3FlushICacheAll
** 功能描述: LEON3 FLUSH 所有 ICACHE
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  leon3FlushICacheAll (VOID)
{
    UINT32  uiCCR;

    __asm__ __volatile__ (" flush ");

    do {
        __asm__ __volatile__ ("lda [%%g0] 2, %0\n\t" : "=r"(uiCCR));
    } while (uiCCR & LEON3_CCR_IFP);
}
/*********************************************************************************************************
** 函数名称: leon3FlushDCacheAll
** 功能描述: LEON3 FLUSH 所有 DCACHE
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  leon3FlushDCacheAll (VOID)
{
    UINT32  uiCCR;

    __asm__ __volatile__ ("sta %%g0, [%%g0] %0\n\t" : :
                          "i"(ASI_LEON_DFLUSH) : "memory");

    do {
        __asm__ __volatile__ ("lda [%%g0] 2, %0\n\t" : "=r"(uiCCR));
    } while (uiCCR & LEON3_CCR_DFP);
}
/*********************************************************************************************************
** 函数名称: leon3CacheEnable
** 功能描述: LEON3 使能 CACHE
** 输　入  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  leon3CacheEnable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        leon3FlushICacheAll();

        __asm__ __volatile__ ("lda  [%%g0] 2, %%l1\n\t"
                              "set  0x010003, %%l2\n\t"                 /*  使能 ICACHE                 */
                              "or   %%l2, %%l1, %%l2\n\t"               /*  使能 Instruction burst fetch*/
                              "sta  %%l2, [%%g0] 2\n\t" : : : "l1", "l2");
    } else {
        leon3FlushDCacheAll();

        __asm__ __volatile__ ("lda  [%%g0] 2, %%l1\n\t"
                              "set  0x80000c, %%l2\n\t"                 /*  使能 DCACHE                 */
                              "or   %%l2, %%l1, %%l2\n\t"               /*  使能 DCACHE SNOOP           */
                              "sta  %%l2, [%%g0] 2\n\t" : : : "l1", "l2");
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: leon3CacheDisable
** 功能描述: LEON3 禁能 CACHE
** 输　入  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
** 注  意  : DCACHE 为写穿透模式, 不用回写.
*********************************************************************************************************/
static INT  leon3CacheDisable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        __asm__ __volatile__ ("lda  [%%g0] 2, %%l1\n\t"
                              "set  0x000003, %%l2\n\t"                 /*  禁能 ICACHE                 */
                              "andn %%l2, %%l1, %%l2\n\t"
                              "sta  %%l2, [%%g0] 2\n\t" : : : "l1", "l2");
    } else {
        __asm__ __volatile__ ("lda  [%%g0] 2, %%l1\n\t"
                              "set  0x00000c, %%l2\n\t"                 /*  禁能 DCACHE                 */
                              "andn %%l2, %%l1, %%l2\n\t"
                              "sta  %%l2, [%%g0] 2\n\t" : : : "l1", "l2");
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: leon3GetCacheRegs
** 功能描述: LEON3 获得所有 CACHE 配置寄存器
** 输　入  : pRegs         CACHE 配置寄存器
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  leon3GetCacheRegs (LEON3_CACHE_REGS  *pRegs)
{
    ULONG  ulCCR, ulICCR, ulDCCR;

    if (!pRegs) {
        return;
    }

    /*
     * Get Cache regs from "Cache ASI" address 0x0, 0x8 and 0xC
     */
    __asm__ __volatile__ ("lda [%%g0] %3, %0\n\t"
                          "mov 0x08, %%g1\n\t"
                          "lda [%%g1] %3, %1\n\t"
                          "mov 0x0c, %%g1\n\t"
                          "lda [%%g1] %3, %2\n\t"
                          : "=r"(ulCCR), "=r"(ulICCR), "=r"(ulDCCR)     /*  output                      */
                          : "i"(ASI_LEON_CACHEREGS)                     /*  input                       */
                          : "g1"                                        /*  clobber list                */
                          );

    pRegs->LC_ulCCR  = ulCCR;
    pRegs->LC_ulICCR = ulICCR;
    pRegs->LC_ulDCCR = ulDCCR;
}
/*********************************************************************************************************
** 函数名称: leon3CacheProbe
** 功能描述: LEON3 初始化 CACHE
** 输　入  : pcacheop       CACHE 操作函数集
**           uiInstruction  指令 CACHE 参数
**           uiData         数据 CACHE 参数
**           pcMachineName  机器名称
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  leon3CacheProbe (LW_CACHE_OP *pcacheop,
                              CACHE_MODE   uiInstruction,
                              CACHE_MODE   uiData,
                              CPCHAR       pcMachineName)
{
    LEON3_CACHE_REGS  cregs;
    UINT              uiSetSize, uiSetNr;
    CHAR             *pcSetStr[4] = { "direct mapped",
                                      "2-way associative",
                                      "3-way associative",
                                      "4-way associative" };

    leon3GetCacheRegs(&cregs);

    uiSetNr = (cregs.LC_ulDCCR & LEON3_XCCR_SETS_MASK) >> 24;

    /*
     * (ssize=>realsize) 0=>1k, 1=>2k, 2=>4k, 3=>8k ...
     */
    uiSetSize = 1 << ((cregs.LC_ulDCCR & LEON3_XCCR_SSIZE_MASK) >> 20);

    _DebugFormat(__LOGMESSAGE_LEVEL, "CACHE: %s cache, set size %dk\r\n",
                 uiSetNr > 3 ? "unknown" : pcSetStr[uiSetNr], uiSetSize);

    if ((uiSetSize <= (LW_CFG_VMM_PAGE_SIZE / 1024)) && (uiSetNr == 0)) {
        /*
         * Set Size <= Page size  ==> Invalidate on every context switch not needed.
         */
        _DebugFormat(__LOGMESSAGE_LEVEL, "CACHE: not invalidate on every context switch\r\n");
    }

    pcacheop->CACHEOP_pfuncEnable  = leon3CacheEnable;
    pcacheop->CACHEOP_pfuncDisable = leon3CacheDisable;

    pcacheop->CACHEOP_ulOption = 0ul;                                   /* No need CACHE_TEXT_UPDATE_MP */

    pcacheop->CACHEOP_iILoc = CACHE_LOCATION_VIVT;
    pcacheop->CACHEOP_iDLoc = CACHE_LOCATION_VIVT;

    pcacheop->CACHEOP_iICacheLine = 32;
    pcacheop->CACHEOP_iDCacheLine = 32;

    pcacheop->CACHEOP_iICacheWaySize = uiSetSize * 1024;
    pcacheop->CACHEOP_iDCacheWaySize = uiSetSize * 1024;

    leonFlushICacheAll = leon3FlushICacheAll;
    leonFlushDCacheAll = leon3FlushDCacheAll;
}
/*********************************************************************************************************
** 函数名称: sparcCacheFlush
** 功能描述: CACHE 脏数据回写
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
** 注  意  : DCACHE 为写穿透模式.
*********************************************************************************************************/
INT  sparcCacheFlush (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: sparcCacheFlushPage
** 功能描述: CACHE 脏数据回写
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           pvPdrs        物理地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
** 注  意  : DCACHE 为写穿透模式.
*********************************************************************************************************/
static INT  sparcCacheFlushPage (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, PVOID  pvPdrs, size_t  stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: sparcCacheInvalidate
** 功能描述: 指定类型的 CACHE 使部分无效(访问不命中)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  sparcCacheInvalidate (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    if (cachetype == INSTRUCTION_CACHE) {
        leonFlushICacheAll();

    } else {
        leonFlushDCacheAll();
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: sparcCacheInvalidatePage
** 功能描述: 指定类型的 CACHE 使部分无效(访问不命中)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           pvPdrs        物理地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  sparcCacheInvalidatePage (LW_CACHE_TYPE cachetype, PVOID pvAdrs, PVOID pvPdrs, size_t stBytes)
{
    if (cachetype == INSTRUCTION_CACHE) {
        leonFlushICacheAll();

    } else {
        leonFlushDCacheAll();
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: sparcCacheClear
** 功能描述: 指定类型的 CACHE 使部分或全部清空(回写内存)并无效(访问不命中)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  sparcCacheClear (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    if (cachetype == INSTRUCTION_CACHE) {
        leonFlushICacheAll();

    } else {
        leonFlushDCacheAll();
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: sparcCacheClearPage
** 功能描述: 指定类型的 CACHE 使部分或全部清空(回写内存)并无效(访问不命中)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           pvPdrs        物理地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  sparcCacheClearPage (LW_CACHE_TYPE cachetype, PVOID pvAdrs, PVOID pvPdrs, size_t stBytes)
{
    if (cachetype == INSTRUCTION_CACHE) {
        leonFlushICacheAll();

    } else {
        leonFlushDCacheAll();
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: sparcCacheLock
** 功能描述: 锁定指定类型的 CACHE
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  sparcCacheLock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: sparcCacheUnlock
** 功能描述: 解锁指定类型的 CACHE
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  sparcCacheUnlock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: sparcCacheTextUpdate
** 功能描述: 清空(回写内存) D CACHE 无效(访问不命中) I CACHE
** 输　入  : pvAdrs                        虚拟地址
**           stBytes                       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
** 注  意  : L2 cache 为统一 CACHE 所以 text update 不需要操作 L2 cache.
             DCACHE 为写穿透模式, 只需要无效 ICACHE.
*********************************************************************************************************/
static INT  sparcCacheTextUpdate (PVOID  pvAdrs, size_t  stBytes)
{
    leonFlushICacheAll();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: sparcCacheDataUpdate
** 功能描述: 回写 D CACHE (仅回写 CPU 独享级 CACHE)
** 输　入  : pvAdrs                        虚拟地址
**           stBytes                       长度
**           bInv                          是否为回写无效
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
** 注  意  : L2 cache 为统一 CACHE 所以 data update 不需要操作 L2 cache.
             DCACHE 为写穿透模式.
*********************************************************************************************************/
static INT  sparcCacheDataUpdate (PVOID  pvAdrs, size_t  stBytes, BOOL  bInv)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: archCacheV4Init
** 功能描述: 初始化 CACHE
** 输　入  : pcacheop       CACHE 操作函数集
**           uiInstruction  指令 CACHE 参数
**           uiData         数据 CACHE 参数
**           pcMachineName  机器名称
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  sparcCacheInit (LW_CACHE_OP *pcacheop,
                      CACHE_MODE   uiInstruction,
                      CACHE_MODE   uiData,
                      CPCHAR       pcMachineName)
{
    if ((lib_strcmp(pcMachineName, SPARC_MACHINE_LEON3) == 0) ||
        (lib_strcmp(pcMachineName, SPARC_MACHINE_LEON4) == 0)) {
        leon3CacheProbe(pcacheop, uiInstruction, uiData, pcMachineName);

    } else {
        leon2CacheProbe(pcacheop, uiInstruction, uiData, pcMachineName);
    }

    pcacheop->CACHEOP_pfuncLock   = sparcCacheLock;                     /*  暂时不支持锁定操作          */
    pcacheop->CACHEOP_pfuncUnlock = sparcCacheUnlock;

    pcacheop->CACHEOP_pfuncFlush          = sparcCacheFlush;
    pcacheop->CACHEOP_pfuncFlushPage      = sparcCacheFlushPage;
    pcacheop->CACHEOP_pfuncInvalidate     = sparcCacheInvalidate;
    pcacheop->CACHEOP_pfuncInvalidatePage = sparcCacheInvalidatePage;
    pcacheop->CACHEOP_pfuncClear          = sparcCacheClear;
    pcacheop->CACHEOP_pfuncClearPage      = sparcCacheClearPage;
    pcacheop->CACHEOP_pfuncTextUpdate     = sparcCacheTextUpdate;
    pcacheop->CACHEOP_pfuncDataUpdate     = sparcCacheDataUpdate;

#if LW_CFG_VMM_EN > 0
    pcacheop->CACHEOP_pfuncDmaMalloc      = API_VmmDmaAlloc;
    pcacheop->CACHEOP_pfuncDmaMallocAlign = API_VmmDmaAllocAlign;
    pcacheop->CACHEOP_pfuncDmaFree        = API_VmmDmaFree;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** 函数名称: archCacheV4Reset
** 功能描述: 复位 CACHE
** 输　入  : pcMachineName  机器名称
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  sparcCacheReset (CPCHAR  pcMachineName)
{
    if ((lib_strcmp(pcMachineName, SPARC_MACHINE_LEON3) == 0) ||
        (lib_strcmp(pcMachineName, SPARC_MACHINE_LEON4) == 0)) {
        leon3CacheDisable(DATA_CACHE);
        leon3CacheDisable(INSTRUCTION_CACHE);

    } else {
        leon2CacheDisable(DATA_CACHE);
        leon2CacheDisable(INSTRUCTION_CACHE);
    }
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
