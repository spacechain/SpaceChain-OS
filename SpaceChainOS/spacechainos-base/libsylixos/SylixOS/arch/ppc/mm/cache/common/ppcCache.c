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
** 文   件   名: ppcCache.c
**
** 创   建   人: Yang.HaiFeng (杨海峰)
**
** 文件创建日期: 2016 年 01 月 18 日
**
** 描        述: PowerPC 体系构架 CACHE 驱动.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
#include "ppcCache.h"
/*********************************************************************************************************
  L2 CACHE 支持
*********************************************************************************************************/
#if LW_CFG_PPC_CACHE_L2 > 0
#include "../l2/ppcL2.h"
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */
/*********************************************************************************************************
  CACHE 循环操作时允许的最大大小, 大于该大小时将使用 All 操作
*********************************************************************************************************/
#define PPC_CACHE_LOOP_OP_MAX_SIZE     (8 * LW_CFG_KB_SIZE)
/*********************************************************************************************************
  CACHE 获得 pvAdrs 与 pvEnd 位置
*********************************************************************************************************/
#define PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiLineSize)               \
        do {                                                                \
            ulEnd  = (addr_t)((size_t)pvAdrs + stBytes);                    \
            pvAdrs = (PVOID)((addr_t)pvAdrs & ~((addr_t)uiLineSize - 1));   \
        } while (0)
/*********************************************************************************************************
  L1 CACHE 状态
*********************************************************************************************************/
static INT              _G_iCacheStatus = L1_CACHE_DIS;
/*********************************************************************************************************
  CACHE 信息
*********************************************************************************************************/
static PPC_CACHE        _G_ICache, _G_DCache;                           /*  I-Cache 和 D-Cache 信息     */
/*********************************************************************************************************
  Pointer of a page-aligned cacheable region to use as a flush buffer.
*********************************************************************************************************/
UINT8                  *_G_pucPpcCacheReadBuffer;
/*********************************************************************************************************
  CACHE 驱动
*********************************************************************************************************/
extern PPC_L1C_DRIVER   _G_ppc603CacheDriver;
extern PPC_L1C_DRIVER   _G_ppc604CacheDriver;
extern PPC_L1C_DRIVER   _G_ppc745xCacheDriver;
extern PPC_L1C_DRIVER   _G_ppc83xxCacheDriver;
extern PPC_L1C_DRIVER   _G_ppcEC603CacheDriver;
extern PPC_L1C_DRIVER   _G_ppcE200CacheDriver;
extern PPC_L1C_DRIVER   _G_ppcE500CacheDriver;

static PPC_L1C_DRIVER  *_G_ppcCacheDrivers[] = {
    &_G_ppc603CacheDriver,
    &_G_ppc604CacheDriver,
    &_G_ppc745xCacheDriver,
    &_G_ppc83xxCacheDriver,
    &_G_ppcEC603CacheDriver,
    &_G_ppcE200CacheDriver,
    &_G_ppcE500CacheDriver,
    LW_NULL,
};

static PPC_L1C_DRIVER  *_G_pcachedriver = LW_NULL;
/*********************************************************************************************************
  定义 CACHE 操作宏
*********************************************************************************************************/
#define ppcDCacheDisable                 _G_pcachedriver->L1CD_pfuncDCacheDisable
#define ppcDCacheEnable                  _G_pcachedriver->L1CD_pfuncDCacheEnable

#define ppcICacheDisable                 _G_pcachedriver->L1CD_pfuncICacheDisable
#define ppcICacheEnable                  _G_pcachedriver->L1CD_pfuncICacheEnable

/*
 * 调用以下三个 All 函数时, 须检查其是否有效
 */
#define ppcDCacheClearAll                _G_pcachedriver->L1CD_pfuncDCacheClearAll
#define ppcDCacheFlushAll                _G_pcachedriver->L1CD_pfuncDCacheFlushAll
#define ppcICacheInvalidateAll           _G_pcachedriver->L1CD_pfuncICacheInvalidateAll

#define ppcDCacheClear                   _G_pcachedriver->L1CD_pfuncDCacheClear
#define ppcDCacheFlush                   _G_pcachedriver->L1CD_pfuncDCacheFlush
#define ppcDCacheInvalidate              _G_pcachedriver->L1CD_pfuncDCacheInvalidate
#define ppcICacheInvalidate              _G_pcachedriver->L1CD_pfuncICacheInvalidate

#define ppcBranchPredictionDisable       _G_pcachedriver->L1CD_pfuncBranchPredictionDisable
#define ppcBranchPredictionEnable        _G_pcachedriver->L1CD_pfuncBranchPredictionEnable
#define ppcBranchPredictorInvalidate     _G_pcachedriver->L1CD_pfuncBranchPredictorInvalidate

#define ppcTextUpdate                    _G_pcachedriver->L1CD_pfuncTextUpdate
/*********************************************************************************************************
  内部函数
*********************************************************************************************************/
/*********************************************************************************************************
** 函数名称: ppcCacheEnable
** 功能描述: 使能 CACHE
** 输　入  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ppcCacheEnable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        if (LW_CPU_GET_CUR_ID() == 0) {
            _G_iCacheStatus |= L1_CACHE_I_EN;
        }
        ppcICacheEnable();

        ppcBranchPredictionEnable();

    } else {
        if (LW_CPU_GET_CUR_ID() == 0) {
            _G_iCacheStatus |= L1_CACHE_D_EN;
        }
        ppcDCacheEnable();
    }

#if LW_CFG_PPC_CACHE_L2 > 0
    if ((LW_CPU_GET_CUR_ID() == 0) &&
        (_G_iCacheStatus == L1_CACHE_EN)) {
        ppcL2Enable();
    }
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ppcCacheDisable
** 功能描述: 禁能 CACHE
** 输　入  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ppcCacheDisable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        if (LW_CPU_GET_CUR_ID() == 0) {
            _G_iCacheStatus &= ~L1_CACHE_I_EN;
        }
        ppcICacheDisable();
        ppcBranchPredictionDisable();

    } else {
        if (LW_CPU_GET_CUR_ID() == 0) {
            _G_iCacheStatus &= ~L1_CACHE_D_EN;
        }
        ppcDCacheDisable();
    }

#if LW_CFG_PPC_CACHE_L2 > 0
    if ((LW_CPU_GET_CUR_ID() == 0) &&
        (_G_iCacheStatus == L1_CACHE_DIS)) {
        ppcL2Disable();
    }
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ppcCacheFlush
** 功能描述: CACHE 脏数据回写
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
** 注  意  : 由于 L2 为物理地址 tag 所以这里暂时使用 L2 全部回写指令.
*********************************************************************************************************/
static INT  ppcCacheFlush (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == DATA_CACHE) {
        if ((stBytes >= PPC_CACHE_LOOP_OP_MAX_SIZE) && ppcDCacheFlushAll) {
            ppcDCacheFlushAll();                                        /*  全部回写                    */

        } else {
            PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
            ppcDCacheFlush(pvAdrs, (PVOID)ulEnd,                        /*  部分回写                    */
                           _G_DCache.CACHE_uiLineSize);
        }

#if LW_CFG_PPC_CACHE_L2 > 0
        ppcL2FlushAll();
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ppcCacheFlushPage
** 功能描述: CACHE 脏数据回写
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           pvPdrs        物理地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ppcCacheFlushPage (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, PVOID  pvPdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == DATA_CACHE) {
        if ((stBytes >= PPC_CACHE_LOOP_OP_MAX_SIZE) && ppcDCacheFlushAll) {
            ppcDCacheFlushAll();                                        /*  全部回写                    */

        } else {
            PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
            ppcDCacheFlush(pvAdrs, (PVOID)ulEnd,                        /*  部分回写                    */
                           _G_DCache.CACHE_uiLineSize);
        }

#if LW_CFG_PPC_CACHE_L2 > 0
        ppcL2Flush(pvPdrs, stBytes);
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ppcCacheInvalidate
** 功能描述: 指定类型的 CACHE 使部分无效(访问不命中)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址 (pvAdrs 必须等于物理地址)
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
** 注  意  : 此函数如果操作 DCACHE pvAdrs 虚拟地址与物理地址必须相同.
*********************************************************************************************************/
static INT  ppcCacheInvalidate (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if ((stBytes >= PPC_CACHE_LOOP_OP_MAX_SIZE) && ppcICacheInvalidateAll) {
            ppcICacheInvalidateAll();                                   /*  ICACHE 全部无效             */

        } else {
            PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_ICache.CACHE_uiLineSize);
            ppcICacheInvalidate(pvAdrs, (PVOID)ulEnd, _G_ICache.CACHE_uiLineSize);
        }
    } else {
        if (stBytes > 0) {                                              /*  必须 > 0                    */
            addr_t  ulStart = (addr_t)pvAdrs;
                    ulEnd   = ulStart + stBytes;

            if (ulStart & ((addr_t)_G_DCache.CACHE_uiLineSize - 1)) {   /*  起始地址非 cache line 对齐  */
                ulStart &= ~((addr_t)_G_DCache.CACHE_uiLineSize - 1);
                ppcDCacheClear((PVOID)ulStart, (PVOID)ulStart, _G_DCache.CACHE_uiLineSize);
                ulStart += _G_DCache.CACHE_uiLineSize;
            }

            if (ulEnd & ((addr_t)_G_DCache.CACHE_uiLineSize - 1)) {     /*  结束地址非 cache line 对齐  */
                ulEnd &= ~((addr_t)_G_DCache.CACHE_uiLineSize - 1);
                ppcDCacheClear((PVOID)ulEnd, (PVOID)ulEnd, _G_DCache.CACHE_uiLineSize);
            }
                                                                        /*  仅无效对齐部分              */
            ppcDCacheInvalidate((PVOID)ulStart, (PVOID)ulEnd, _G_DCache.CACHE_uiLineSize);

#if LW_CFG_PPC_CACHE_L2 > 0
            ppcL2Invalidate(pvAdrs, stBytes);                           /*  虚拟与物理地址必须相同      */
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */
        } else {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "stBytes == 0.\r\n");
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ppcCacheInvalidatePage
** 功能描述: 指定类型的 CACHE 使部分无效(访问不命中)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           pvPdrs        物理地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ppcCacheInvalidatePage (LW_CACHE_TYPE    cachetype,
                                    PVOID            pvAdrs,
                                    PVOID            pvPdrs,
                                    size_t           stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if ((stBytes >= PPC_CACHE_LOOP_OP_MAX_SIZE) && ppcICacheInvalidateAll) {
            ppcICacheInvalidateAll();                                   /*  ICACHE 全部无效             */

        } else {
            PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_ICache.CACHE_uiLineSize);
            ppcICacheInvalidate(pvAdrs, (PVOID)ulEnd, _G_ICache.CACHE_uiLineSize);
        }
    } else {
        if (stBytes > 0) {                                              /*  必须 > 0                    */
            addr_t  ulStart = (addr_t)pvAdrs;
                    ulEnd   = ulStart + stBytes;

            if (ulStart & ((addr_t)_G_DCache.CACHE_uiLineSize - 1)) {   /*  起始地址非 cache line 对齐  */
                ulStart &= ~((addr_t)_G_DCache.CACHE_uiLineSize - 1);
                ppcDCacheClear((PVOID)ulStart, (PVOID)ulStart, _G_DCache.CACHE_uiLineSize);
                ulStart += _G_DCache.CACHE_uiLineSize;
            }

            if (ulEnd & ((addr_t)_G_DCache.CACHE_uiLineSize - 1)) {     /*  结束地址非 cache line 对齐  */
                ulEnd &= ~((addr_t)_G_DCache.CACHE_uiLineSize - 1);
                ppcDCacheClear((PVOID)ulEnd, (PVOID)ulEnd, _G_DCache.CACHE_uiLineSize);
            }
                                                                        /*  仅无效对齐部分              */
            ppcDCacheInvalidate((PVOID)ulStart, (PVOID)ulEnd, _G_DCache.CACHE_uiLineSize);

#if LW_CFG_PPC_CACHE_L2 > 0
            ppcL2Invalidate(pvPdrs, stBytes);                           /*  虚拟与物理地址必须相同      */
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */
        } else {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "stBytes == 0.\r\n");
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ppcCacheClear
** 功能描述: 指定类型的 CACHE 使部分或全部清空(回写内存)并无效(访问不命中)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
** 注  意  : 由于 L2 为物理地址 tag 所以这里暂时使用 L2 全部回写并无效指令.
*********************************************************************************************************/
static INT  ppcCacheClear (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if ((stBytes >= PPC_CACHE_LOOP_OP_MAX_SIZE) && ppcICacheInvalidateAll) {
            ppcICacheInvalidateAll();                                   /*  ICACHE 全部无效             */

        } else {
            PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_ICache.CACHE_uiLineSize);
            ppcICacheInvalidate(pvAdrs, (PVOID)ulEnd, _G_ICache.CACHE_uiLineSize);
        }
    } else {
        if ((stBytes >= PPC_CACHE_LOOP_OP_MAX_SIZE) && ppcDCacheClearAll) {
            ppcDCacheClearAll();                                        /*  全部回写并无效              */

        } else {
            PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
            ppcDCacheClear(pvAdrs, (PVOID)ulEnd,
                           _G_DCache.CACHE_uiLineSize);                 /*  部分回写并无效              */
        }

#if LW_CFG_PPC_CACHE_L2 > 0
        ppcL2ClearAll();
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ppcCacheClearPage
** 功能描述: 指定类型的 CACHE 使部分或全部清空(回写内存)并无效(访问不命中)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           pvPdrs        物理地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ppcCacheClearPage (LW_CACHE_TYPE cachetype, PVOID pvAdrs, PVOID pvPdrs, size_t stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if ((stBytes >= PPC_CACHE_LOOP_OP_MAX_SIZE) && ppcICacheInvalidateAll) {
            ppcICacheInvalidateAll();                                   /*  ICACHE 全部无效             */

        } else {
            PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_ICache.CACHE_uiLineSize);
            ppcICacheInvalidate(pvAdrs, (PVOID)ulEnd, _G_ICache.CACHE_uiLineSize);
        }
    } else {
        if ((stBytes >= PPC_CACHE_LOOP_OP_MAX_SIZE) && ppcDCacheClearAll) {
            ppcDCacheClearAll();                                        /*  全部回写并无效              */

        } else {
            PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
            ppcDCacheClear(pvAdrs, (PVOID)ulEnd,
                           _G_DCache.CACHE_uiLineSize);                 /*  部分回写并无效              */
        }

#if LW_CFG_PPC_CACHE_L2 > 0
        ppcL2Clear(pvPdrs, stBytes);
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ppcCacheLock
** 功能描述: 锁定指定类型的 CACHE
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ppcCacheLock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: ppcCacheUnlock
** 功能描述: 解锁指定类型的 CACHE
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ppcCacheUnlock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: ppcCacheTextUpdate
** 功能描述: 清空(回写内存) D CACHE 无效(访问不命中) I CACHE
** 输　入  : pvAdrs                        虚拟地址
**           stBytes                       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
** 注  意  : L2 cache 为统一 CACHE 所以 text update 不需要操作 L2 cache.
*********************************************************************************************************/
static INT  ppcCacheTextUpdate (PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (ppcTextUpdate) {
        PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
        ppcTextUpdate(pvAdrs, (PVOID)ulEnd, _G_ICache.CACHE_uiLineSize, _G_DCache.CACHE_uiLineSize);
        return  (ERROR_NONE);
    }

    if ((stBytes >= PPC_CACHE_LOOP_OP_MAX_SIZE) &&
        ppcDCacheFlushAll && ppcICacheInvalidateAll) {
        ppcDCacheFlushAll();                                            /*  DCACHE 全部回写             */
        ppcICacheInvalidateAll();                                       /*  ICACHE 全部无效             */

    } else {
        PVOID   pvAdrsBak = pvAdrs;

        PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
        ppcDCacheFlush(pvAdrs, (PVOID)ulEnd,
                       _G_DCache.CACHE_uiLineSize);                     /*  部分回写                    */

        PPC_CACHE_GET_END(pvAdrsBak, stBytes, ulEnd, _G_ICache.CACHE_uiLineSize);
        ppcICacheInvalidate(pvAdrsBak, (PVOID)ulEnd, _G_ICache.CACHE_uiLineSize);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ppcCacheDataUpdate
** 功能描述: 回写 D CACHE (仅回写 CPU 独享级 CACHE)
** 输　入  : pvAdrs                        虚拟地址
**           stBytes                       长度
**           bInv                          是否为回写无效
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
** 注  意  : L2 cache 为统一 CACHE 所以 data update 不需要操作 L2 cache.
*********************************************************************************************************/
INT  ppcCacheDataUpdate (PVOID  pvAdrs, size_t  stBytes, BOOL  bInv)
{
    addr_t  ulEnd;

    if (bInv == LW_FALSE) {
        if ((stBytes >= PPC_CACHE_LOOP_OP_MAX_SIZE) && ppcDCacheFlushAll) {
            ppcDCacheFlushAll();                                        /*  全部回写                    */

        } else {
            PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
            ppcDCacheFlush(pvAdrs, (PVOID)ulEnd,
                           _G_DCache.CACHE_uiLineSize);                 /*  部分回写                    */
        }

    } else {
        if ((stBytes >= PPC_CACHE_LOOP_OP_MAX_SIZE) && ppcDCacheClearAll) {
            ppcDCacheClearAll();                                        /*  全部回写                    */

        } else {
            PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
            ppcDCacheClear(pvAdrs, (PVOID)ulEnd,
                           _G_DCache.CACHE_uiLineSize);                 /*  部分回写                    */
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ppcCacheProbe
** 功能描述: CACHE 探测
** 输　入  : NONE
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ppcCacheProbe (CPCHAR       pcMachineName)
{
    static BOOL  bProbed = LW_FALSE;
    INT          i;

    if (bProbed) {
        return  (ERROR_NONE);
    }

    i = 0;
    while (1) {
        _G_pcachedriver = _G_ppcCacheDrivers[i];

        if (_G_pcachedriver) {
            if (_G_pcachedriver->L1CD_pfuncProbe(pcMachineName, &_G_ICache, &_G_DCache) == ERROR_NONE) {
                break;
            }
        } else {
            return  (PX_ERROR);
        }

        i++;
    }

    /*
     * Alloc a page-aligned cacheable region to use as a flush buffer.
     * Worst case PLRU flush needs 1.5 * cache size.
     */
    _G_pucPpcCacheReadBuffer = __KHEAP_ALLOC_ALIGN(ROUND_UP((_G_DCache.CACHE_uiSize * 3) >> 1,
                                                   LW_CFG_VMM_PAGE_SIZE),
                                                   LW_CFG_VMM_PAGE_SIZE);
    if (!_G_pucPpcCacheReadBuffer) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        return  (PX_ERROR);
    }

    bProbed = LW_TRUE;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ppcCacheInit
** 功能描述: 初始化 CACHE
** 输　入  : pcacheop       CACHE 操作函数集
**           uiInstruction  指令 CACHE 参数
**           uiData         数据 CACHE 参数
**           pcMachineName  机器名称
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  ppcCacheInit (LW_CACHE_OP *pcacheop,
                   CACHE_MODE   uiInstruction,
                   CACHE_MODE   uiData,
                   CPCHAR       pcMachineName)
{
    INT     iError;

    iError = ppcCacheProbe(pcMachineName);
    if (iError != ERROR_NONE) {
        return  (iError);
    }

#if LW_CFG_PPC_CACHE_L2 > 0
    ppcL2Init(uiInstruction, uiData, pcMachineName);
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */

    pcacheop->CACHEOP_ulOption = 0ul;                                   /*  无须 TEXT_UPDATE_MP 选项    */

    pcacheop->CACHEOP_iILoc = CACHE_LOCATION_VIPT;
    pcacheop->CACHEOP_iDLoc = CACHE_LOCATION_VIPT;

    pcacheop->CACHEOP_iICacheLine = _G_ICache.CACHE_uiLineSize;
    pcacheop->CACHEOP_iDCacheLine = _G_DCache.CACHE_uiLineSize;

    pcacheop->CACHEOP_iICacheWaySize = _G_ICache.CACHE_uiWaySize;
    pcacheop->CACHEOP_iDCacheWaySize = _G_DCache.CACHE_uiWaySize;

    _DebugFormat(__LOGMESSAGE_LEVEL, "PowerPC I-Cache line size = %d byte Way size = %d byte.\r\n",
                 pcacheop->CACHEOP_iICacheLine, pcacheop->CACHEOP_iICacheWaySize);
    _DebugFormat(__LOGMESSAGE_LEVEL, "PowerPC D-Cache line size = %d byte Way size = %d byte.\r\n",
                 pcacheop->CACHEOP_iDCacheLine, pcacheop->CACHEOP_iDCacheWaySize);

    pcacheop->CACHEOP_pfuncEnable  = ppcCacheEnable;
    pcacheop->CACHEOP_pfuncDisable = ppcCacheDisable;

    pcacheop->CACHEOP_pfuncLock   = ppcCacheLock;                       /*  暂时不支持锁定操作          */
    pcacheop->CACHEOP_pfuncUnlock = ppcCacheUnlock;

    pcacheop->CACHEOP_pfuncFlush          = ppcCacheFlush;
    pcacheop->CACHEOP_pfuncFlushPage      = ppcCacheFlushPage;
    pcacheop->CACHEOP_pfuncInvalidate     = ppcCacheInvalidate;
    pcacheop->CACHEOP_pfuncInvalidatePage = ppcCacheInvalidatePage;
    pcacheop->CACHEOP_pfuncClear          = ppcCacheClear;
    pcacheop->CACHEOP_pfuncClearPage      = ppcCacheClearPage;
    pcacheop->CACHEOP_pfuncTextUpdate     = ppcCacheTextUpdate;
    pcacheop->CACHEOP_pfuncDataUpdate     = ppcCacheDataUpdate;

#if LW_CFG_VMM_EN > 0
    pcacheop->CACHEOP_pfuncDmaMalloc      = API_VmmDmaAlloc;
    pcacheop->CACHEOP_pfuncDmaMallocAlign = API_VmmDmaAllocAlign;
    pcacheop->CACHEOP_pfuncDmaFree        = API_VmmDmaFree;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ppcCacheReset
** 功能描述: 复位 CACHE
** 输　入  : pcMachineName  机器名称
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
** 注  意  : 如果有 lockdown 必须首先 unlock & invalidate 才能启动
*********************************************************************************************************/
INT  ppcCacheReset (CPCHAR  pcMachineName)
{
    INT     iError;

    iError = ppcCacheProbe(pcMachineName);
    if (iError != ERROR_NONE) {
        return  (iError);
    }

    if (ppcICacheInvalidateAll) {
        ppcICacheInvalidateAll();
    }
    ppcDCacheDisable();
    ppcICacheDisable();
    ppcBranchPredictorInvalidate();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ppcCacheStatus
** 功能描述: 获得 CACHE 状态
** 输　入  : NONE
** 输　出  : CACHE 状态
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  ppcCacheStatus (VOID)
{
    return  (_G_iCacheStatus);
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
