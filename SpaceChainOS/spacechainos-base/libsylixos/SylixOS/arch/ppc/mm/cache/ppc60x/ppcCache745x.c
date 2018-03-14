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
** 文   件   名: ppcCache745x.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 03 月 30 日
**
** 描        述: PowerPC MPC745X 体系构架 CACHE 驱动.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
#include "../common/ppcCache.h"
/*********************************************************************************************************
  外部接口声明
*********************************************************************************************************/
extern VOID     ppc745xDCacheDisable(VOID);
extern VOID     ppc745xDCacheEnable(VOID);
extern VOID     ppc745xICacheDisable(VOID);
extern VOID     ppc745xICacheEnable(VOID);

extern VOID     ppc745xDCacheClearAll(VOID);
extern VOID     ppc745xDCacheFlushAll(VOID);
extern VOID     ppc745xICacheInvalidateAll(VOID);

extern VOID     ppc745xDCacheClear(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppc745xDCacheFlush(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppc745xDCacheInvalidate(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppc745xICacheInvalidate(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);

extern VOID     ppc745xBranchPredictionDisable(VOID);
extern VOID     ppc745xBranchPredictionEnable(VOID);
extern VOID     ppc745xBranchPredictorInvalidate(VOID);

extern VOID     ppc745xTextUpdate(PVOID  pvStart, PVOID  pvEnd,
                                  UINT32  uiICacheLineSize, UINT32  uiDCacheLineSize);
/*********************************************************************************************************
** 函数名称: ppc745xCacheProbe
** 功能描述: CACHE 探测
** 输　入  : pcMachineName         机器名
**           pICache               I-Cache 信息
**           pDCache               D-Cache 信息
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT   ppc745xCacheProbe (CPCHAR  pcMachineName, PPC_CACHE  *pICache, PPC_CACHE  *pDCache)
{
    /*
     * TODO 暂时不支持
     */
    return  (PX_ERROR);
}
/*********************************************************************************************************
  MPC745X CACHE 驱动
*********************************************************************************************************/
PPC_L1C_DRIVER  _G_ppc745xCacheDriver = {
    "745X",
    ppc745xCacheProbe,

    ppc745xDCacheDisable,
    ppc745xDCacheEnable,
    ppc745xICacheDisable,
    ppc745xICacheEnable,

    ppc745xDCacheClearAll,
    ppc745xDCacheFlushAll,
    ppc745xICacheInvalidateAll,

    ppc745xDCacheClear,
    ppc745xDCacheFlush,
    ppc745xDCacheInvalidate,
    ppc745xICacheInvalidate,

    ppc745xBranchPredictionDisable,
    ppc745xBranchPredictionEnable,
    ppc745xBranchPredictorInvalidate,

    ppc745xTextUpdate,
};

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
