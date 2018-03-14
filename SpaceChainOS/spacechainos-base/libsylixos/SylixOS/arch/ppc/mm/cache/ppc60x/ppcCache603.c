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
** 文   件   名: ppcCache603.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 03 月 30 日
**
** 描        述: PowerPC 603 体系构架 CACHE 驱动.
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
extern VOID     ppc603DCacheDisable(VOID);
extern VOID     ppc603DCacheEnable(VOID);
extern VOID     ppc603ICacheDisable(VOID);
extern VOID     ppc603ICacheEnable(VOID);

extern VOID     ppc603DCacheClearAll(VOID);
extern VOID     ppc603DCacheFlushAll(VOID);
extern VOID     ppc603ICacheInvalidateAll(VOID);

extern VOID     ppc603DCacheClear(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppc603DCacheFlush(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppc603DCacheInvalidate(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppc603ICacheInvalidate(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);

extern VOID     ppc603BranchPredictionDisable(VOID);
extern VOID     ppc603BranchPredictionEnable(VOID);
extern VOID     ppc603BranchPredictorInvalidate(VOID);

extern VOID     ppc603TextUpdate(PVOID  pvStart, PVOID  pvEnd,
                                 UINT32  uiICacheLineSize, UINT32  uiDCacheLineSize);
/*********************************************************************************************************
** 函数名称: ppc603CacheProbe
** 功能描述: CACHE 探测
** 输　入  : pcMachineName         机器名
**           pICache               I-Cache 信息
**           pDCache               D-Cache 信息
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT   ppc603CacheProbe (CPCHAR  pcMachineName, PPC_CACHE  *pICache, PPC_CACHE  *pDCache)
{
    /*
     * TODO 暂时不支持
     */
    return  (PX_ERROR);
}
/*********************************************************************************************************
  603 CACHE 驱动
*********************************************************************************************************/
PPC_L1C_DRIVER  _G_ppc603CacheDriver = {
    "603",
    ppc603CacheProbe,

    ppc603DCacheDisable,
    ppc603DCacheEnable,
    ppc603ICacheDisable,
    ppc603ICacheEnable,

    ppc603DCacheClearAll,
    ppc603DCacheFlushAll,
    ppc603ICacheInvalidateAll,

    ppc603DCacheClear,
    ppc603DCacheFlush,
    ppc603DCacheInvalidate,
    ppc603ICacheInvalidate,

    ppc603BranchPredictionDisable,
    ppc603BranchPredictionEnable,
    ppc603BranchPredictorInvalidate,

    ppc603TextUpdate,
};

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
