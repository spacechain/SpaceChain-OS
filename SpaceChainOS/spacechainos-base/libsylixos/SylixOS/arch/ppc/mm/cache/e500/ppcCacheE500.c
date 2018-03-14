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
** 文   件   名: ppcCacheE500.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 05 月 03 日
**
** 描        述: PowerPC E500 体系构架 CACHE 驱动.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
#include "../common/ppcCache.h"
#define  __SYLIXOS_PPC_E500__
#include "arch/ppc/arch_e500.h"
/*********************************************************************************************************
  Probe 函数需要修改的变量(汇编需要用到)
*********************************************************************************************************/
UINT32  PPC_E500_ICACHE_ALIGN_SIZE = 32;
UINT32  PPC_E500_ICACHE_CNWAY      = 8;
UINT32  PPC_E500_ICACHE_SETS       = 128;
UINT32  PPC_E500_ICACHE_LINE_NUM   = 128 * 8;

UINT32  PPC_E500_DCACHE_ALIGN_SIZE = 32;
UINT32  PPC_E500_DCACHE_CNWAY      = 8;
UINT32  PPC_E500_DCACHE_SETS       = 128;
UINT32  PPC_E500_DCACHE_LINE_NUM   = 128 * 8;
/*********************************************************************************************************
  外部接口声明
*********************************************************************************************************/
extern VOID     ppcE500DCacheDisable(VOID);
extern VOID     ppcE500DCacheEnable(VOID);
extern VOID     ppcE500ICacheDisable(VOID);
extern VOID     ppcE500ICacheEnable(VOID);

extern VOID     ppcE500DCacheFlushAll(VOID);
extern VOID     ppcE500DCacheInvalidateAll(VOID);
extern VOID     ppcE500ICacheInvalidateAll(VOID);

extern VOID     ppcE500DCacheClear(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppcE500DCacheFlush(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppcE500DCacheInvalidate(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppcE500ICacheInvalidate(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);

extern VOID     ppcE500BranchPredictionDisable(VOID);
extern VOID     ppcE500BranchPredictionEnable(VOID);
extern VOID     ppcE500BranchPredictorInvalidate(VOID);

extern UINT32   ppcE500CacheGetL1CFG0(VOID);
extern UINT32   ppcE500CacheGetL1CFG1(VOID);

extern VOID     ppcE500TextUpdate(PVOID  pvStart, PVOID  pvEnd,
                                  UINT32  uiICacheLineSize, UINT32  uiDCacheLineSize);
/*********************************************************************************************************
** 函数名称: ppcE500CacheNoop
** 功能描述: CACHE 空操作
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  ppcE500CacheNoop (VOID)
{
}
/*********************************************************************************************************
** 函数名称: ppcE500CacheProbe
** 功能描述: CACHE 探测
** 输　入  : pcMachineName         机器名
**           pICache               I-Cache 信息
**           pDCache               D-Cache 信息
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT   ppcE500CacheProbe (CPCHAR  pcMachineName, PPC_CACHE  *pICache, PPC_CACHE  *pDCache)
{
    if ((lib_strcmp(pcMachineName, PPC_MACHINE_E500)   == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_E500V1) == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_E500V2) == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_E500MC) == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_E5500)  == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_E6500)  == 0)) {
        /*
         * D-Cache 探测
         */
        {
            E500_L1CFG0     l1cfg0;
            UINT32          uiCacheSize;

            l1cfg0.L1CFG0_uiValue = ppcE500CacheGetL1CFG0();

            uiCacheSize = l1cfg0.L1CFG0_usCSIZE * 1024;

            switch (l1cfg0.L1CFG0_ucCBSIZE) {

            case 0:
                PPC_E500_DCACHE_ALIGN_SIZE = 32;
                break;

            case 1:
                PPC_E500_DCACHE_ALIGN_SIZE = 64;
                break;

            case 2:
                PPC_E500_DCACHE_ALIGN_SIZE = 128;
                break;

            default:
                return  (PX_ERROR);
            }

            PPC_E500_DCACHE_CNWAY     = l1cfg0.L1CFG0_ucCNWAY + 1;
            PPC_E500_DCACHE_LINE_NUM  = (uiCacheSize / PPC_E500_DCACHE_ALIGN_SIZE);
            PPC_E500_DCACHE_SETS      = PPC_E500_DCACHE_LINE_NUM / PPC_E500_DCACHE_CNWAY;

            pDCache->CACHE_uiSize     = uiCacheSize;
            pDCache->CACHE_uiLineSize = PPC_E500_DCACHE_ALIGN_SIZE;
            pDCache->CACHE_uiSetNr    = PPC_E500_DCACHE_SETS;
            pDCache->CACHE_uiWayNr    = PPC_E500_DCACHE_CNWAY;
            pDCache->CACHE_uiWaySize  = pDCache->CACHE_uiSetNr * pDCache->CACHE_uiLineSize;
        }

        /*
         * I-Cache 探测
         */
        {
            E500_L1CFG1     l1cfg1;
            UINT32          uiCacheSize;

            l1cfg1.L1CFG1_uiValue = ppcE500CacheGetL1CFG1();

            uiCacheSize = l1cfg1.L1CFG1_ucCSIZE * 1024;

            switch (l1cfg1.L1CFG1_ucCBSIZE) {

            case 0:
                PPC_E500_ICACHE_ALIGN_SIZE = 32;
                break;

            case 1:
                PPC_E500_ICACHE_ALIGN_SIZE = 64;
                break;

            case 2:
                PPC_E500_ICACHE_ALIGN_SIZE = 128;
                break;

            default:
                return  (PX_ERROR);
            }

            PPC_E500_ICACHE_CNWAY     = l1cfg1.L1CFG1_ucCNWAY + 1;
            PPC_E500_ICACHE_LINE_NUM  = (uiCacheSize / PPC_E500_ICACHE_ALIGN_SIZE);
            PPC_E500_ICACHE_SETS      = PPC_E500_ICACHE_LINE_NUM / PPC_E500_ICACHE_CNWAY;

            pICache->CACHE_uiSize     = uiCacheSize;
            pICache->CACHE_uiLineSize = PPC_E500_ICACHE_ALIGN_SIZE;
            pICache->CACHE_uiSetNr    = PPC_E500_ICACHE_SETS;
            pICache->CACHE_uiWayNr    = PPC_E500_ICACHE_CNWAY;
            pICache->CACHE_uiWaySize  = pICache->CACHE_uiSetNr * pICache->CACHE_uiLineSize;
        }

        if (LW_NCPUS > 1) {                                             /*  SMP                         */
            extern PPC_L1C_DRIVER  _G_ppcE500CacheDriver;

            /*
             * SMP 机器, 以下 CACHE 操作均为空操作
             */
            _G_ppcE500CacheDriver.L1CD_pfuncDCacheDisable       = ppcE500CacheNoop;
            _G_ppcE500CacheDriver.L1CD_pfuncICacheDisable       = ppcE500CacheNoop;
            _G_ppcE500CacheDriver.L1CD_pfuncDCacheClearAll      = LW_NULL;
            _G_ppcE500CacheDriver.L1CD_pfuncDCacheFlushAll      = LW_NULL;
            _G_ppcE500CacheDriver.L1CD_pfuncICacheInvalidateAll = LW_NULL;
            _G_ppcE500CacheDriver.L1CD_pfuncDCacheClear         = (VOID (*)(PVOID, PVOID, UINT32))ppcE500CacheNoop;
            _G_ppcE500CacheDriver.L1CD_pfuncDCacheInvalidate    = (VOID (*)(PVOID, PVOID, UINT32))ppcE500CacheNoop;
            _G_ppcE500CacheDriver.L1CD_pfuncICacheInvalidate    = (VOID (*)(PVOID, PVOID, UINT32))ppcE500CacheNoop;
        }

        return  (ERROR_NONE);

    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
  E500 CACHE 驱动
*********************************************************************************************************/
PPC_L1C_DRIVER  _G_ppcE500CacheDriver = {
    "E500",
    ppcE500CacheProbe,

    ppcE500DCacheDisable,
    ppcE500DCacheEnable,
    ppcE500ICacheDisable,
    ppcE500ICacheEnable,

    LW_NULL,
    ppcE500DCacheFlushAll,
    ppcE500ICacheInvalidateAll,

    ppcE500DCacheClear,
    ppcE500DCacheFlush,
    ppcE500DCacheInvalidate,
    ppcE500ICacheInvalidate,

    ppcE500BranchPredictionDisable,
    ppcE500BranchPredictionEnable,
    ppcE500BranchPredictorInvalidate,

    ppcE500TextUpdate,
};

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
