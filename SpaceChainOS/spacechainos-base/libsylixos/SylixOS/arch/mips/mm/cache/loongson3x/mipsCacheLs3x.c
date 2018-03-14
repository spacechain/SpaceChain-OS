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
** 文   件   名: mipsCacheLs3x.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 11 月 02 日
**
** 描        述: Loongson3x 体系构架 CACHE 驱动.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
#include "arch/mips/common/cp0/mipsCp0.h"
#include "arch/mips/mm/cache/mipsCacheCommon.h"
/*********************************************************************************************************
  外部函数声明
*********************************************************************************************************/
extern VOID  ls3xCacheEnableHw(VOID);
extern VOID  ls3xCacheDisableHw(VOID);
/*********************************************************************************************************
  内部函数前置声明
*********************************************************************************************************/
static INT   ls3xCacheProbe(VOID);
/*********************************************************************************************************
  L1 CACHE 状态
*********************************************************************************************************/
static INT          _G_iLs3xCacheStatus = L1_CACHE_DIS;
/*********************************************************************************************************
  CACHE 信息
*********************************************************************************************************/
static MIPS_CACHE   _G_ICache, _G_DCache;                               /*  I-Cache 和 D-Cache 信息     */
static MIPS_CACHE   _G_VCache, _G_SCache;                               /*  V-Cache 和 S-Cache 信息     */
/*********************************************************************************************************
** 函数名称: ls3xBranchPredictionDisable
** 功能描述: 禁能分支预测
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  ls3xBranchPredictionDisable (VOID)
{
    UINT32  uiDiag = mipsCp0DiagRead();

    uiDiag |= 1 << 0;                                                   /*  置 1 时禁用分支预测         */
    mipsCp0DiagWrite(uiDiag);
}
/*********************************************************************************************************
** 函数名称: ls3xBranchPredictionEnable
** 功能描述: 使能分支预测
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  ls3xBranchPredictionEnable (VOID)
{
    UINT32  uiDiag = mipsCp0DiagRead();

    uiDiag &= ~(1 << 0);
    mipsCp0DiagWrite(uiDiag);
}
/*********************************************************************************************************
** 函数名称: ls3xBranchPredictorInvalidate
** 功能描述: 无效分支预测
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  ls3xBranchPredictorInvalidate (VOID)
{
    UINT32  uiDiag = mipsCp0DiagRead();

    uiDiag |= 1 << 1;                                                   /*  写入 1 清空 BRBTB 和 BTAC   */
    mipsCp0DiagWrite(uiDiag);
}
/*********************************************************************************************************
** 函数名称: ls3aR1CacheFlushAll
** 功能描述: Loongson3A R1 回写所有 CACHE
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  ls3aR1CacheFlushAll (VOID)
{
    REGISTER PVOID  pvAddr;

    __asm__ __volatile__(
        "   .set push                     \n"
        "   .set noreorder                \n"
        "   li %[addr], 0x80000000        \n"                           /*  KSEG0                       */
        "1: cache 0, 0(%[addr])           \n"                           /*  flush L1 ICache             */
        "   cache 0, 1(%[addr])           \n"
        "   cache 0, 2(%[addr])           \n"
        "   cache 0, 3(%[addr])           \n"
        "   cache 1, 0(%[addr])           \n"                           /*  flush L1 DCache             */
        "   cache 1, 1(%[addr])           \n"
        "   cache 1, 2(%[addr])           \n"
        "   cache 1, 3(%[addr])           \n"
        "   addiu %[sets], %[sets], -1    \n"
        "   bnez  %[sets], 1b             \n"
        "   addiu %[addr], %[addr], 0x20  \n"
        "   sync                          \n"
        "   .set pop                      \n"
        : [addr] "=&r" (pvAddr)
        : [sets] "r" (_G_DCache.CACHE_uiSetNr));
}
/*********************************************************************************************************
** 函数名称: ls3aR2CacheFlushAll
** 功能描述: Loongson3A R2 回写所有 CACHE
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  ls3aR2CacheFlushAll (VOID)
{
    REGISTER PVOID  pvAddr;

    __asm__ __volatile__(
        "   .set push                     \n"
        "   .set noreorder                \n"
        "   li %[addr], 0x80000000        \n"                           /*  KSEG0                       */
        "1: cache 0, 0(%[addr])           \n"                           /*  flush L1 ICache             */
        "   cache 0, 1(%[addr])           \n"
        "   cache 0, 2(%[addr])           \n"
        "   cache 0, 3(%[addr])           \n"
        "   cache 1, 0(%[addr])           \n"                           /*  flush L1 DCache             */
        "   cache 1, 1(%[addr])           \n"
        "   cache 1, 2(%[addr])           \n"
        "   cache 1, 3(%[addr])           \n"
        "   addiu %[sets], %[sets], -1    \n"
        "   bnez  %[sets], 1b             \n"
        "   addiu %[addr], %[addr], 0x40  \n"
        "   li %[addr], 0x80000000        \n"                           /*  KSEG0                       */
        "2: cache 2, 0(%[addr])           \n"                           /*  flush L1 VCache             */
        "   cache 2, 1(%[addr])           \n"
        "   cache 2, 2(%[addr])           \n"
        "   cache 2, 3(%[addr])           \n"
        "   cache 2, 4(%[addr])           \n"
        "   cache 2, 5(%[addr])           \n"
        "   cache 2, 6(%[addr])           \n"
        "   cache 2, 7(%[addr])           \n"
        "   cache 2, 8(%[addr])           \n"
        "   cache 2, 9(%[addr])           \n"
        "   cache 2, 10(%[addr])          \n"
        "   cache 2, 11(%[addr])          \n"
        "   cache 2, 12(%[addr])          \n"
        "   cache 2, 13(%[addr])          \n"
        "   cache 2, 14(%[addr])          \n"
        "   cache 2, 15(%[addr])          \n"
        "   addiu %[vsets], %[vsets], -1  \n"
        "   bnez  %[vsets], 2b            \n"
        "   addiu %[addr], %[addr], 0x40  \n"
        "   sync                          \n"
        "   .set pop                      \n"
        : [addr] "=&r" (pvAddr)
        : [sets] "r" (_G_DCache.CACHE_uiSetNr),
          [vsets] "r" (_G_VCache.CACHE_uiSetNr));
}
/*********************************************************************************************************
** 函数名称: ls3bCacheFlushAll
** 功能描述: Loongson3B 回写所有 CACHE
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  ls3bCacheFlushAll (VOID)
{
    REGISTER PVOID  pvAddr;

    __asm__ __volatile__(
        "   .set push                     \n"
        "   .set noreorder                \n"
        "   li %[addr], 0x80000000        \n"                           /*  KSEG0                       */
        "1: cache 0, 0(%[addr])           \n"                           /*  flush L1 ICache             */
        "   cache 0, 1(%[addr])           \n"
        "   cache 0, 2(%[addr])           \n"
        "   cache 0, 3(%[addr])           \n"
        "   cache 1, 0(%[addr])           \n"                           /*  flush L1 DCache             */
        "   cache 1, 1(%[addr])           \n"
        "   cache 1, 2(%[addr])           \n"
        "   cache 1, 3(%[addr])           \n"
        "   addiu %[sets], %[sets], -1    \n"
        "   bnez  %[sets], 1b             \n"
        "   addiu %[addr], %[addr], 0x20  \n"
        "   sync                          \n"
        "   .set pop                      \n"
        : [addr] "=&r" (pvAddr)
        : [sets] "r" (_G_DCache.CACHE_uiSetNr));
}
/*********************************************************************************************************
** 函数名称: ls3xCacheFlushAll
** 功能描述: 回写所有 CACHE
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  ls3xCacheFlushAll (VOID)
{
    INT     iError;

    iError = ls3xCacheProbe();
    if (iError != ERROR_NONE) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "No Cache!\r\n");
        return;
    }

    switch (mipsCp0PRIdRead() & 0xf) {

    case PRID_REV_LOONGSON3A_R2:
    case PRID_REV_LOONGSON3A_R3:
        ls3aR2CacheFlushAll();
        break;

    case PRID_REV_LOONGSON3B_R1:
    case PRID_REV_LOONGSON3B_R2:
        ls3bCacheFlushAll();
        break;

    case PRID_REV_LOONGSON2K:
        ls3aR1CacheFlushAll();
        break;

    case PRID_REV_LOONGSON3A_R1:
    default:
        ls3aR1CacheFlushAll();
        break;
    }
}
/*********************************************************************************************************
** 函数名称: ls3xCacheEnable
** 功能描述: 使能 CACHE
** 输　入  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ls3xCacheEnable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        if (LW_CPU_GET_CUR_ID() == 0) {
            _G_iLs3xCacheStatus |= L1_CACHE_I_EN;
        }
    } else {
        if (LW_CPU_GET_CUR_ID() == 0) {
            _G_iLs3xCacheStatus |= L1_CACHE_D_EN;
        }
    }

    if (_G_iLs3xCacheStatus == L1_CACHE_EN) {
        ls3xCacheEnableHw();
        ls3xBranchPredictionEnable();
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ls3xCacheDisable
** 功能描述: 禁能 CACHE
** 输　入  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ls3xCacheDisable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        if (LW_CPU_GET_CUR_ID() == 0) {
            _G_iLs3xCacheStatus &= ~L1_CACHE_I_EN;
        }
    } else {
        if (LW_CPU_GET_CUR_ID() == 0) {
            _G_iLs3xCacheStatus &= ~L1_CACHE_D_EN;
        }
    }

    if (_G_iLs3xCacheStatus == L1_CACHE_DIS) {
        ls3xCacheDisableHw();
        ls3xBranchPredictionDisable();
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ls3xCacheFlushNone
** 功能描述: CACHE 脏数据回写 (伪函数)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ls3xCacheFlushNone (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ls3xCacheFlushPageNone
** 功能描述: CACHE 脏数据回写 (伪函数)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           pvPdrs        物理地址
**           stBytes       长度
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ls3xCacheFlushPageNone (LW_CACHE_TYPE  cachetype,
                                    PVOID          pvAdrs,
                                    PVOID          pvPdrs,
                                    size_t         stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ls3xCacheInvalidateNone
** 功能描述: 指定类型的 CACHE 使部分无效(访问不命中) (伪函数)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ls3xCacheInvalidateNone (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ls3xCacheInvalidatePageNone
** 功能描述: 指定类型的 CACHE 使部分无效(访问不命中) (伪函数)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           pvPdrs        物理地址
**           stBytes       长度
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  ls3xCacheInvalidatePageNone (LW_CACHE_TYPE  cachetype,
                                         PVOID          pvAdrs,
                                         PVOID          pvPdrs,
                                         size_t         stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ls3xCacheClearNone
** 功能描述: 指定类型的 CACHE 使部分或全部清空(回写内存)并无效(访问不命中) (伪函数)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  ls3xCacheClearNone (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ls3xCacheClearPageNone
** 功能描述: 指定类型的 CACHE 使部分或全部清空(回写内存)并无效(访问不命中)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           pvPdrs        物理地址
**           stBytes       长度
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  ls3xCacheClearPageNone (LW_CACHE_TYPE  cachetype,
                                    PVOID          pvAdrs,
                                    PVOID          pvPdrs,
                                    size_t         stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ls3xCacheLock
** 功能描述: 锁定指定类型的 CACHE 
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  ls3xCacheLockNone (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: ls3xCacheUnlock
** 功能描述: 解锁指定类型的 CACHE 
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  ls3xCacheUnlockNone (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: ls3xCacheTextUpdate
** 功能描述: 清空(回写内存) D CACHE 无效(访问不命中) I CACHE
** 输　入  : pvAdrs                        虚拟地址
**           stBytes                       长度
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
** 注  意  : L2 cache 为统一 CACHE 所以 text update 不需要操作 L2 cache.
*********************************************************************************************************/
static INT  ls3xCacheTextUpdateNone (PVOID  pvAdrs, size_t  stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ls3xCacheDataUpdate
** 功能描述: 回写 D CACHE (仅回写 CPU 独享级 CACHE)
** 输　入  : pvAdrs                        虚拟地址
**           stBytes                       长度
**           bInv                          是否为回写无效
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
** 注  意  : L2 cache 为统一 CACHE 所以 data update 不需要操作 L2 cache.
*********************************************************************************************************/
static INT  ls3xCacheDataUpdateNone (PVOID  pvAdrs, size_t  stBytes, BOOL  bInv)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ls3xL3CacheProbe
** 功能描述: L3 S-CACHE 探测
** 输　入  : NONE
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ls3xL3SCacheProbe (VOID)
{
    UINT32  uiConfig;
    UINT32  uiLineSize;
    UINT32  uiPrid;

    uiConfig = mipsCp0Config2Read();                                    /*  读 Config2                  */
    if ((uiLineSize = ((uiConfig >> 4) & 15))) {
        _G_SCache.CACHE_bPresent   = LW_TRUE;
        _G_SCache.CACHE_uiLineSize = 2 << uiLineSize;
        _G_SCache.CACHE_uiSetNr    = 64 << ((uiConfig >> 8) & 15);
        _G_SCache.CACHE_uiWayNr    = 1 + (uiConfig & 15);
        _G_SCache.CACHE_uiSize     = _G_SCache.CACHE_uiSetNr * _G_SCache.CACHE_uiWayNr *
                                     _G_SCache.CACHE_uiLineSize;
        /*
         * Loongson-3 has 4 cores, 1MB scache for each. scaches are shared
         */
        uiPrid = mipsCp0PRIdRead() & 0xf;
        if (uiPrid != PRID_REV_LOONGSON2K) {                            /*  Loongson2K1000 为 1M        */
            _G_SCache.CACHE_uiSize *= 4;
        }

        _G_SCache.CACHE_uiWayStep  = _G_SCache.CACHE_uiSetNr * _G_SCache.CACHE_uiLineSize;

    } else {
        _G_SCache.CACHE_bPresent = LW_FALSE;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ls3xL2CacheProbe
** 功能描述: L2 V-CACHE 探测
** 输　入  : NONE
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ls3xL2VCacheProbe (VOID)
{
    UINT32  uiConfig;
    UINT32  uiLineSize;
    UINT32  uiPrid;

    uiPrid = mipsCp0PRIdRead() & 0xf;
    if (uiPrid == PRID_REV_LOONGSON2K) {                                /*  Loongson2K1000 无 V CACHE   */
        _G_VCache.CACHE_bPresent = LW_FALSE;
        return  (ERROR_NONE);
    }

    uiConfig = mipsCp0Config2Read();                                    /*  读 Config2                  */
    if ((uiLineSize = ((uiConfig >> 20) & 15))) {
        _G_VCache.CACHE_bPresent   = LW_TRUE;
        _G_VCache.CACHE_uiLineSize = 2 << uiLineSize;
        _G_VCache.CACHE_uiSetNr    = 64 << ((uiConfig >> 24) & 15);
        _G_VCache.CACHE_uiWayNr    = 1 + ((uiConfig >> 16) & 15);
        _G_VCache.CACHE_uiSize     = _G_VCache.CACHE_uiSetNr * _G_VCache.CACHE_uiWayNr *
                                     _G_VCache.CACHE_uiLineSize;
        _G_VCache.CACHE_uiWayStep  = _G_VCache.CACHE_uiSetNr * _G_VCache.CACHE_uiLineSize;

    } else {
        _G_VCache.CACHE_bPresent = LW_FALSE;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ls3xCacheProbe
** 功能描述: CACHE 探测
** 输　入  : NONE
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ls3xCacheProbe (VOID)
{
    static BOOL    bIsProbed = LW_FALSE;
           UINT32  uiConfig;
           UINT32  uiTemp;

    if (bIsProbed) {
        return  (ERROR_NONE);
    }

    uiConfig = mipsCp0ConfigRead();                                     /*  读 Config0                  */
    if (!(uiConfig & (M_ConfigMore))) {                                 /*  没有 Config1, 退出          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "No CP0 Config1 Register!\r\n");
        return  (PX_ERROR);
    }

    uiConfig = mipsCp0Config1Read();                                    /*  读 Config1                  */
    uiTemp   = (uiConfig & M_Config1IL) >> S_Config1IL;
    if (uiTemp) {
        _G_ICache.CACHE_bPresent   = LW_TRUE;

        _G_ICache.CACHE_uiLineSize = 2 << uiTemp;

        uiTemp                     = (uiConfig & M_Config1IS) >> S_Config1IS;
        _G_ICache.CACHE_uiSetNr    = 64 << uiTemp;

        uiTemp                     = (uiConfig & M_Config1IA) >> S_Config1IA;
        _G_ICache.CACHE_uiWayNr    = 1 + uiTemp;

        _G_ICache.CACHE_uiSize     = _G_ICache.CACHE_uiSetNr * _G_ICache.CACHE_uiWayNr *
                                     _G_ICache.CACHE_uiLineSize;

        _G_ICache.CACHE_uiWayStep  = _G_ICache.CACHE_uiSetNr * _G_ICache.CACHE_uiLineSize;

    } else {                                                            /*  没有 I-Cache                */
        _G_ICache.CACHE_bPresent   = LW_FALSE;
    }

    uiTemp = (uiConfig & M_Config1DL) >> S_Config1DL;
    if (uiTemp) {
        _G_DCache.CACHE_bPresent   = LW_TRUE;

        _G_DCache.CACHE_uiLineSize = 2 << uiTemp;

        uiTemp                     = (uiConfig & M_Config1DS) >> S_Config1DS;
        _G_DCache.CACHE_uiSetNr    = 64 << uiTemp;

        uiTemp                     = (uiConfig & M_Config1DA) >> S_Config1DA;
        _G_DCache.CACHE_uiWayNr    = 1 + uiTemp;

        _G_DCache.CACHE_uiSize     = _G_DCache.CACHE_uiSetNr * _G_DCache.CACHE_uiWayNr *
                                     _G_DCache.CACHE_uiLineSize;

        _G_DCache.CACHE_uiWayStep  = _G_DCache.CACHE_uiSetNr * _G_DCache.CACHE_uiLineSize;

    } else {                                                            /*  没有 D-Cache                */
        _G_DCache.CACHE_bPresent   = LW_FALSE;
    }

    ls3xL2VCacheProbe();
    ls3xL3SCacheProbe();

    bIsProbed = LW_TRUE;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ls3xCacheInfoShow
** 功能描述: CACHE 信息打印
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  ls3xCacheInfoShow (VOID)
{
    INT     iLevel = 2;

    if (_G_ICache.CACHE_bPresent) {
        _DebugFormat(__LOGMESSAGE_LEVEL, "L1 I-CACHE size %dKB (%d line size, %d way, %d set).\r\n",
                     _G_ICache.CACHE_uiSize / 1024,
                     _G_ICache.CACHE_uiLineSize,
                     _G_ICache.CACHE_uiWayNr,
                     _G_ICache.CACHE_uiSetNr);
    }

    if (_G_DCache.CACHE_bPresent) {
        _DebugFormat(__LOGMESSAGE_LEVEL, "L1 D-CACHE size %dKB (%d line size, %d way, %d set).\r\n",
                     _G_DCache.CACHE_uiSize / 1024,
                     _G_DCache.CACHE_uiLineSize,
                     _G_DCache.CACHE_uiWayNr,
                     _G_DCache.CACHE_uiSetNr);
    }

    if (_G_VCache.CACHE_bPresent) {
        _DebugFormat(__LOGMESSAGE_LEVEL, "L2 V-CACHE size %dKB (%d line size, %d way, %d set).\r\n",
                     _G_VCache.CACHE_uiSize / 1024,
                     _G_VCache.CACHE_uiLineSize,
                     _G_VCache.CACHE_uiWayNr,
                     _G_VCache.CACHE_uiSetNr);
        iLevel++;
    }

    if (_G_SCache.CACHE_bPresent) {
        _DebugFormat(__LOGMESSAGE_LEVEL, "L%d S-CACHE size %dKB (%d line size, %d way, %d set).\r\n",
                     iLevel,
                     _G_SCache.CACHE_uiSize / 1024,
                     _G_SCache.CACHE_uiLineSize,
                     _G_SCache.CACHE_uiWayNr,
                     _G_SCache.CACHE_uiSetNr);
    }
}
/*********************************************************************************************************
** 函数名称: mipsCacheLs3xInit
** 功能描述: 初始化 CACHE
** 输　入  : pcacheop       CACHE 操作函数集
**           uiInstruction  指令 CACHE 参数
**           uiData         数据 CACHE 参数
**           pcMachineName  机器名称
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  mipsCacheLs3xInit (LW_CACHE_OP  *pcacheop,
                         CACHE_MODE    uiInstruction,
                         CACHE_MODE    uiData,
                         CPCHAR        pcMachineName)
{
    INT     iError;

    iError = ls3xCacheProbe();
    if (iError != ERROR_NONE) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "No Cache!\r\n");
        return;
    }

    ls3xCacheInfoShow();                                                /*  打印 CACHE 信息             */

    pcacheop->CACHEOP_ulOption = 0ul;                                   /*  无须 TEXT_UPDATE_MP 选项    */

    pcacheop->CACHEOP_iILoc = CACHE_LOCATION_VIPT;                      /*  VIPT                        */
    pcacheop->CACHEOP_iDLoc = CACHE_LOCATION_VIPT;

    pcacheop->CACHEOP_iICacheLine = _G_ICache.CACHE_uiLineSize;
    pcacheop->CACHEOP_iDCacheLine = _G_DCache.CACHE_uiLineSize;

    pcacheop->CACHEOP_iICacheWaySize = _G_ICache.CACHE_uiWayStep;
    pcacheop->CACHEOP_iDCacheWaySize = _G_DCache.CACHE_uiWayStep;

    pcacheop->CACHEOP_pfuncEnable  = ls3xCacheEnable;
    pcacheop->CACHEOP_pfuncDisable = ls3xCacheDisable;
    /*
     * Loongson3x 实现了各级 CACHE 的硬件一致性，所有 CACHE 函数都是 NONE 函数
     */
    pcacheop->CACHEOP_pfuncFlush          = ls3xCacheFlushNone;
    pcacheop->CACHEOP_pfuncFlushPage      = ls3xCacheFlushPageNone;
    pcacheop->CACHEOP_pfuncInvalidate     = ls3xCacheInvalidateNone;
    pcacheop->CACHEOP_pfuncInvalidatePage = ls3xCacheInvalidatePageNone;
    pcacheop->CACHEOP_pfuncClear          = ls3xCacheClearNone;
    pcacheop->CACHEOP_pfuncClearPage      = ls3xCacheClearPageNone;
    pcacheop->CACHEOP_pfuncTextUpdate     = ls3xCacheTextUpdateNone;
    pcacheop->CACHEOP_pfuncDataUpdate     = ls3xCacheDataUpdateNone;
    
    pcacheop->CACHEOP_pfuncLock           = ls3xCacheLockNone;          /*  暂时不支持锁定操作          */
    pcacheop->CACHEOP_pfuncUnlock         = ls3xCacheUnlockNone;
    
#if LW_CFG_VMM_EN > 0
    pcacheop->CACHEOP_pfuncDmaMalloc      = API_VmmDmaAlloc;
    pcacheop->CACHEOP_pfuncDmaMallocAlign = API_VmmDmaAllocAlign;
    pcacheop->CACHEOP_pfuncDmaFree        = API_VmmDmaFree;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** 函数名称: mipsCacheLs3xReset
** 功能描述: 复位 CACHE 
** 输　入  : pcMachineName  机器名称
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  mipsCacheLs3xReset (CPCHAR  pcMachineName)
{
    INT     iError;

    iError = ls3xCacheProbe();
    if (iError != ERROR_NONE) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "No Cache!\r\n");
        return;
    }

    ls3xBranchPredictorInvalidate();
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
