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
** 文   件   名: mipsL2R4k.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2017 年 07 月 18 日
**
** 描        述: MIPS R4K 体系构架 L2-CACHE 驱动.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0 && LW_CFG_MIPS_CACHE_L2 > 0
#include "arch/mips/common/cp0/mipsCp0.h"
#include "arch/mips/common/mipsCpuProbe.h"
#include "arch/mips/mm/cache/mipsCacheCommon.h"
#include "arch/mips/mm/cache/l2/mipsL2R4k.h"
/*********************************************************************************************************
  外部函数声明
*********************************************************************************************************/
extern VOID     mipsL2R4kLineFlush(PCHAR  pcAddr);
extern VOID     mipsL2R4kLineClear(PCHAR  pcAddr);
extern VOID     mipsL2R4kLineInvalidate(PCHAR  pcAddr);
extern VOID     mipsL2R4kIndexClear(PCHAR  pcAddr);
extern VOID     mipsL2R4kIndexStoreTag(PCHAR  pcAddr);
/*********************************************************************************************************
  CACHE 信息
*********************************************************************************************************/
static MIPS_CACHE   _G_SCache;                                          /*  SCACHE 信息                 */
/*********************************************************************************************************
  CACHE 特性
*********************************************************************************************************/
static BOOL         _G_bHaveHitWritebackS = LW_FALSE;                   /*  是否有 HitWritebackS 操作   */
/*********************************************************************************************************
  CACHE 循环操作时允许的最大大小, 大于该大小时将使用 All 操作
*********************************************************************************************************/
#define MIPS_L2_CACHE_LOOP_OP_MAX_SIZE  (64 * LW_CFG_KB_SIZE)
/*********************************************************************************************************
** 函数名称: __mipsL2R4kClear
** 功能描述: SCACHE 脏数据无效
** 输　入  : pvStart        开始地址
**           pvEnd          结束地址
**           uiStep         步进
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __mipsL2R4kClear (PVOID  pvStart, PVOID  pvEnd, size_t  uiStep)
{
    REGISTER PCHAR   pcAddr;

    for (pcAddr = (PCHAR)pvStart; pcAddr < (PCHAR)pvEnd; pcAddr += uiStep) {
        mipsL2R4kLineClear(pcAddr);
    }
    MIPS_PIPE_FLUSH();
}
/*********************************************************************************************************
** 函数名称: __mipsL2R4kFlush
** 功能描述: SCACHE 脏数据回写
** 输　入  : pvStart        开始地址
**           pvEnd          结束地址
**           uiStep         步进
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __mipsL2R4kFlush (PVOID  pvStart, PVOID  pvEnd, size_t  uiStep)
{
    REGISTER PCHAR   pcAddr;

    for (pcAddr = (PCHAR)pvStart; pcAddr < (PCHAR)pvEnd; pcAddr += uiStep) {
        mipsL2R4kLineFlush(pcAddr);
    }
    MIPS_PIPE_FLUSH();
}
/*********************************************************************************************************
** 函数名称: mipsL2R4kClearAll
** 功能描述: SCACHE 所有数据回写并无效
** 输　入  : NONE
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  mipsL2R4kClearAll (VOID)
{
    if (_G_SCache.CACHE_bPresent) {
        REGISTER INT     iWay;
        REGISTER PCHAR   pcLineAddr;
        REGISTER PCHAR   pcBaseAddr = (PCHAR)A_K0BASE;
        REGISTER PCHAR   pcEndAddr  = (PCHAR)(A_K0BASE + _G_SCache.CACHE_uiWayStep);

        for (pcLineAddr = pcBaseAddr; pcLineAddr < pcEndAddr; pcLineAddr += _G_SCache.CACHE_uiLineSize) {
            for (iWay = 0; iWay < _G_SCache.CACHE_uiWayNr; iWay++) {
                mipsL2R4kIndexClear(pcLineAddr + (iWay << _G_SCache.CACHE_uiWayBit));
            }
        }
        MIPS_PIPE_FLUSH();
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: mipsL2R4kFlushAll
** 功能描述: SCACHE 所有数据回写
** 输　入  : NONE
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  mipsL2R4kFlushAll (VOID)
{
    return  (mipsL2R4kClearAll());
}
/*********************************************************************************************************
** 函数名称: mipsL2R4kClear
** 功能描述: SCACHE 脏数据无效
** 输　入  : pvStart        开始虚拟地址
**           stBytes        字节数
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  mipsL2R4kClear (PVOID  pvAdrs, size_t stBytes)
{
    addr_t  ulEnd;

    if (stBytes >= MIPS_L2_CACHE_LOOP_OP_MAX_SIZE) {
        mipsL2R4kClearAll();                                            /*  全部回写并无效              */

    } else {
        MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_SCache.CACHE_uiLineSize);
        __mipsL2R4kClear(pvAdrs, (PVOID)ulEnd,
                         _G_SCache.CACHE_uiLineSize);                   /*  部分回写并无效              */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: mipsL2R4kFlush
** 功能描述: SCACHE 脏数据回写
** 输　入  : pvStart        开始虚拟地址
**           stBytes        字节数
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  mipsL2R4kFlush (PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (stBytes >= MIPS_L2_CACHE_LOOP_OP_MAX_SIZE) {
        mipsL2R4kFlushAll();                                            /*  全部回写                    */

    } else {
        MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_SCache.CACHE_uiLineSize);
        if (_G_bHaveHitWritebackS) {
            __mipsL2R4kFlush(pvAdrs, (PVOID)ulEnd,                      /*  部分回写                    */
                             _G_SCache.CACHE_uiLineSize);
        } else {
            __mipsL2R4kClear(pvAdrs, (PVOID)ulEnd,
                             _G_SCache.CACHE_uiLineSize);               /*  部分回写并无效              */
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: mipsL2R4kInvalidate
** 功能描述: SCACHE 脏数据无效
** 输　入  : pvStart        开始虚拟地址
**           stBytes        字节数
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  mipsL2R4kInvalidate (PVOID  pvStart, size_t stBytes)
{
    REGISTER PCHAR   pcAddr;
             addr_t  ulStart = (addr_t)pvStart;
             addr_t  ulEnd   = ulStart + stBytes;

    if (ulStart & ((addr_t)_G_SCache.CACHE_uiLineSize - 1)) {           /*  起始地址非 cache line 对齐  */
        ulStart &= ~((addr_t)_G_SCache.CACHE_uiLineSize - 1);
        __mipsL2R4kClear((PVOID)ulStart, (PVOID)ulStart, _G_SCache.CACHE_uiLineSize);
        ulStart += _G_SCache.CACHE_uiLineSize;
    }

    if (ulEnd & ((addr_t)_G_SCache.CACHE_uiLineSize - 1)) {             /*  结束地址非 cache line 对齐  */
        ulEnd &= ~((addr_t)_G_SCache.CACHE_uiLineSize - 1);
        __mipsL2R4kClear((PVOID)ulEnd, (PVOID)ulEnd, _G_SCache.CACHE_uiLineSize);
    }

    for (pcAddr = (PCHAR)ulStart; pcAddr < (PCHAR)ulEnd; pcAddr += _G_SCache.CACHE_uiLineSize) {
        mipsL2R4kLineInvalidate(pcAddr);                                /*  仅无效对齐部分              */
    }
    MIPS_PIPE_FLUSH();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: mipsL2R4kInitHw
** 功能描述: SCACHE 硬件初始化
** 输　入  : NONE
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  mipsL2R4kInitHw (VOID)
{
    REGISTER PCHAR   pcLineAddr;
    REGISTER PCHAR   pcBaseAddr = (PCHAR)A_K0BASE;
    REGISTER PCHAR   pcEndAddr;
    REGISTER INT32   iWay;
    REGISTER CHAR    cTemp;

    (VOID)cTemp;

    /*
     * init L2
     */
    if (_G_SCache.CACHE_bPresent) {
        pcEndAddr = (PCHAR)(A_K0BASE + _G_SCache.CACHE_uiSize);

        /*
         * clear all tags
         */
        for (pcLineAddr = pcBaseAddr; pcLineAddr < pcEndAddr; pcLineAddr += _G_SCache.CACHE_uiLineSize) {
            for (iWay = 0; iWay < _G_SCache.CACHE_uiWayNr; iWay ++) {
                mipsL2R4kIndexStoreTag(pcLineAddr + (iWay << _G_SCache.CACHE_uiWayBit));
            }
        }

        /*
         * load from each line (in cached space)
         */
        for (pcLineAddr = pcBaseAddr; pcLineAddr < pcEndAddr; pcLineAddr += _G_SCache.CACHE_uiLineSize) {
            cTemp = *pcLineAddr;
        }

        /*
         * clear all tags again
         */
        for (pcLineAddr = pcBaseAddr; pcLineAddr < pcEndAddr; pcLineAddr += _G_SCache.CACHE_uiLineSize) {
            for (iWay = 0; iWay < _G_SCache.CACHE_uiWayNr; iWay ++) {
                mipsL2R4kIndexStoreTag(pcLineAddr + (iWay << _G_SCache.CACHE_uiWayBit));
            }
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: loongson2SCacheProbe
** 功能描述: loongson2 SCACHE 探测
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  loongson2SCacheProbe (VOID)
{
    _G_SCache.CACHE_bPresent   = LW_TRUE;
    _G_SCache.CACHE_uiSize     = 512 * 1024;
    _G_SCache.CACHE_uiLineSize = 32;
    _G_SCache.CACHE_uiWayNr    = 4;
    _G_SCache.CACHE_uiWayBit   = 0;
    _G_SCache.CACHE_uiWayStep  = 128 * 1024;
    _G_SCache.CACHE_uiSetNr    =   4 * 1024;
}
/*********************************************************************************************************
** 函数名称: loongson2HSCacheProbe
** 功能描述: loongson2H SCACHE 探测
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  loongson2HSCacheProbe (VOID)
{
    UINT32  uiConfig;
    UINT32  uiTemp;

    uiConfig = mipsCp0ConfigRead();                                     /*  读 Config0                  */
    if (!(uiConfig & (M_ConfigMore))) {                                 /*  没有 Config1, 退出          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "No CP0 Config1 Register!\r\n");
        return;
    }

    uiConfig = mipsCp0Config1Read();                                    /*  读 Config1                  */
    if (!(uiConfig & (M_ConfigMore))) {                                 /*  没有 Config2, 退出          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "No CP0 Config2 Register!\r\n");
        return;
    }

    uiConfig = mipsCp0Config2Read();                                    /*  读 Config2                  */
    uiTemp   = (uiConfig & M_Config2SL) >> S_Config2SL;
    if (uiTemp) {
        _G_SCache.CACHE_bPresent   = LW_TRUE;
        _G_SCache.CACHE_uiLineSize = 2 << uiTemp;
        uiTemp                     = (uiConfig & M_Config2SS) >> S_Config2SS;
        _G_SCache.CACHE_uiSetNr    = 64 << uiTemp;
        uiTemp                     = (uiConfig & M_Config2SA) >> S_Config2SA;
        _G_SCache.CACHE_uiWayNr    = 1 + uiTemp;
        _G_SCache.CACHE_uiSize     = _G_SCache.CACHE_uiSetNr * _G_SCache.CACHE_uiWayNr *
                                     _G_SCache.CACHE_uiLineSize;
        _G_SCache.CACHE_uiWayStep  = _G_SCache.CACHE_uiSetNr * _G_SCache.CACHE_uiLineSize;
        _G_SCache.CACHE_uiWayBit   = 0;
    }
}
/*********************************************************************************************************
** 函数名称: mipsL2R4kSCacheProbe
** 功能描述: mips 标准 SCACHE 探测
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  mipsL2R4kSCacheProbe (VOID)
{
    UINT32  uiConfig;
    UINT32  uiTemp;

    uiConfig = mipsCp0ConfigRead();                                     /*  读 Config0                  */
    if (!(uiConfig & (M_ConfigMore))) {                                 /*  无 Config1, 退出            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "No CP0 Config1 Register!\r\n");
        return;
    }
    uiConfig = mipsCp0Config1Read();                                    /*  读 Config1                  */
    if (!(uiConfig & (M_ConfigMore))) {                                 /*  无 Config2, 退出            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "No CP0 Config2 Register!\r\n");
        return;
    }

    uiConfig = mipsCp0Config2Read();                                    /*  读 Config2                  */
    uiTemp   = (uiConfig & M_Config2SL) >> S_Config2SL;
    if (uiTemp) {
        _G_SCache.CACHE_bPresent   = LW_TRUE;
        _G_SCache.CACHE_uiLineSize = 2 << uiTemp;
        uiTemp                     = (uiConfig & M_Config2SS) >> S_Config2SS;
        _G_SCache.CACHE_uiSetNr    = 64 << uiTemp;
        uiTemp                     = (uiConfig & M_Config2SA) >> S_Config2SA;
        _G_SCache.CACHE_uiWayNr    = 1 + uiTemp;
        _G_SCache.CACHE_uiSize     = _G_SCache.CACHE_uiSetNr * _G_SCache.CACHE_uiWayNr *
                                     _G_SCache.CACHE_uiLineSize;
        _G_SCache.CACHE_uiWayStep  = _G_SCache.CACHE_uiSetNr * _G_SCache.CACHE_uiLineSize;
        _G_SCache.CACHE_uiWayBit   = lib_ffs(_G_SCache.CACHE_uiWayStep) - 1;
    }
}
/*********************************************************************************************************
** 函数名称: mipsL2R4kProbe
** 功能描述: L2 CACHE 探测
** 输　入  : NONE
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  mipsL2R4kProbe (VOID)
{
    _G_SCache.CACHE_bPresent = LW_FALSE;

    switch (_G_uiMipsCpuType) {

    case CPU_LOONGSON2:
        loongson2SCacheProbe();
        break;

    case CPU_LOONGSON3:
        loongson2HSCacheProbe();
        break;

    default:
        mipsL2R4kSCacheProbe();
        break;
    }

    if (_G_SCache.CACHE_bPresent) {
        _DebugFormat(__LOGMESSAGE_LEVEL,"L2 S-CACHE size %dKB (%d line size, %d way, %d set, %d waybit).\r\n",
                     _G_SCache.CACHE_uiSize / 1024,
                     _G_SCache.CACHE_uiLineSize,
                     _G_SCache.CACHE_uiWayNr,
                     _G_SCache.CACHE_uiSetNr,
                     _G_SCache.CACHE_uiWayBit);
    }

    return  (_G_SCache.CACHE_bPresent ? (ERROR_NONE) : (PX_ERROR));
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
