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
** 文   件   名: ppcCacheE200.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 05 月 03 日
**
** 描        述: PowerPC E200 体系构架 CACHE 驱动.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
#include "../common/ppcCache.h"
#define  __SYLIXOS_PPC_E200__
#include "arch/ppc/arch_e500.h"
/*********************************************************************************************************
  Probe 函数需要修改的变量(汇编需要用到)
*********************************************************************************************************/
UINT32  PPC_E200_CACHE_CNWAY      = 8;
UINT32  PPC_E200_CACHE_SETS       = 128;
UINT32  PPC_E200_CACHE_ALIGN_SIZE = 32;
UINT32  PPC_E200_CACHE_LINE_NUM   = 128 * 8;
/*********************************************************************************************************
  外部接口声明
*********************************************************************************************************/
extern VOID     ppcE200CacheDisable(VOID);
extern VOID     ppcE200CacheEnable(VOID);

extern VOID     ppcE200DCacheClear(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppcE200DCacheFlush(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppcE200DCacheInvalidate(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppcE200ICacheInvalidate(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);

extern VOID     ppcE200BranchPredictionDisable(VOID);
extern VOID     ppcE200BranchPredictionEnable(VOID);
extern VOID     ppcE200BranchPredictorInvalidate(VOID);

extern UINT32   ppcE200CacheGetL1CFG0(VOID);

extern VOID     ppcE200TextUpdate(PVOID  pvStart, PVOID  pvEnd,
                                  UINT32  uiICacheLineSize, UINT32  uiDCacheLineSize);
/*********************************************************************************************************
** 函数名称: __ppcE200CacheDisable
** 功能描述: 关闭 CACHE
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __ppcE200CacheDisable (VOID)
{
    if (ppcCacheStatus() == L1_CACHE_DIS) {
        ppcE200CacheDisable();
    }
}
/*********************************************************************************************************
** 函数名称: __ppcE200CacheEnable
** 功能描述: 使能 CACHE
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __ppcE200CacheEnable (VOID)
{
    if (ppcCacheStatus() == L1_CACHE_EN) {
        ppcE200CacheEnable();
    }
}
/*********************************************************************************************************
** 函数名称: ppcE200CacheProbe
** 功能描述: CACHE 探测
** 输　入  : pcMachineName         机器名
**           pICache               I-Cache 信息
**           pDCache               D-Cache 信息
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ppcE200CacheProbe (CPCHAR  pcMachineName, PPC_CACHE  *pICache, PPC_CACHE  *pDCache)
{
    if (lib_strcmp(pcMachineName, PPC_MACHINE_E200) == 0) {
        E200_L1CFG0     l1cfg0;
        UINT32          uiCacheSize;

        l1cfg0.L1CFG0_uiValue = ppcE200CacheGetL1CFG0();

        uiCacheSize = l1cfg0.L1CFG0_usCSIZE * 1024;

        switch (l1cfg0.L1CFG0_ucCBSIZE) {

        case 0:
            PPC_E200_CACHE_ALIGN_SIZE = 32;
            break;

        case 1:
            PPC_E200_CACHE_ALIGN_SIZE = 64;
            break;

        case 2:
            PPC_E200_CACHE_ALIGN_SIZE = 128;
            break;

        default:
            return  (PX_ERROR);
        }

        PPC_E200_CACHE_CNWAY      = l1cfg0.L1CFG0_ucCNWAY + 1;
        PPC_E200_CACHE_LINE_NUM   = (uiCacheSize / PPC_E200_CACHE_ALIGN_SIZE);
        PPC_E200_CACHE_SETS       = PPC_E200_CACHE_LINE_NUM / PPC_E200_CACHE_CNWAY;

        pICache->CACHE_uiSize     = uiCacheSize;
        pICache->CACHE_uiLineSize = PPC_E200_CACHE_ALIGN_SIZE;
        pICache->CACHE_uiSetNr    = PPC_E200_CACHE_SETS;
        pICache->CACHE_uiWayNr    = PPC_E200_CACHE_CNWAY;
        pICache->CACHE_uiWaySize  = pDCache->CACHE_uiSetNr * pDCache->CACHE_uiLineSize;

        *pDCache = *pICache;
        return  (ERROR_NONE);

    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
  E200 CACHE 驱动
*********************************************************************************************************/
PPC_L1C_DRIVER  _G_ppcE200CacheDriver = {
    "E200",
    ppcE200CacheProbe,

    __ppcE200CacheDisable,
    __ppcE200CacheEnable,
    __ppcE200CacheDisable,
    __ppcE200CacheEnable,

    /*
     * E200 为统一 CACHE, 不支持 ALL 操作
     */
    LW_NULL,
    LW_NULL,
    LW_NULL,

    ppcE200DCacheClear,
    ppcE200DCacheFlush,
    ppcE200DCacheInvalidate,
    ppcE200ICacheInvalidate,

    ppcE200BranchPredictionDisable,
    ppcE200BranchPredictionEnable,
    ppcE200BranchPredictorInvalidate,

    ppcE200TextUpdate,
};

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
