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
** 文   件   名: ppcCacheEC603.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 03 月 30 日
**
** 描        述: PowerPC EC603 体系构架 CACHE 驱动.
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
extern VOID     ppcEC603DCacheDisable(VOID);
extern VOID     ppcEC603DCacheEnable(VOID);
extern VOID     ppcEC603ICacheDisable(VOID);
extern VOID     ppcEC603ICacheEnable(VOID);

extern VOID     ppcEC603DCacheClearAll(VOID);
extern VOID     ppcEC603DCacheFlushAll(VOID);
extern VOID     ppcEC603ICacheInvalidateAll(VOID);

extern VOID     ppcEC603DCacheClear(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppcEC603DCacheFlush(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppcEC603DCacheInvalidate(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppcEC603ICacheInvalidate(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);

extern VOID     ppcEC603BranchPredictionDisable(VOID);
extern VOID     ppcEC603BranchPredictionEnable(VOID);
extern VOID     ppcEC603BranchPredictorInvalidate(VOID);

extern VOID     ppcEC603TextUpdate(PVOID  pvStart, PVOID  pvEnd,
                                   UINT32  uiICacheLineSize, UINT32  uiDCacheLineSize);
/*********************************************************************************************************
** 函数名称: ppcEC603CacheProbe
** 功能描述: CACHE 探测
** 输　入  : pcMachineName         机器名
**           pICache               I-Cache 信息
**           pDCache               D-Cache 信息
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT   ppcEC603CacheProbe (CPCHAR  pcMachineName, PPC_CACHE  *pICache, PPC_CACHE  *pDCache)
{
    /*
     * TODO 暂时不支持
     */
    return  (PX_ERROR);
}
/*********************************************************************************************************
  EC603 CACHE 驱动
*********************************************************************************************************/
PPC_L1C_DRIVER  _G_ppcEC603CacheDriver = {
    "EC603",
    ppcEC603CacheProbe,

    ppcEC603DCacheDisable,
    ppcEC603DCacheEnable,
    ppcEC603ICacheDisable,
    ppcEC603ICacheEnable,

    ppcEC603DCacheClearAll,
    ppcEC603DCacheFlushAll,
    ppcEC603ICacheInvalidateAll,

    ppcEC603DCacheClear,
    ppcEC603DCacheFlush,
    ppcEC603DCacheInvalidate,
    ppcEC603ICacheInvalidate,

    ppcEC603BranchPredictionDisable,
    ppcEC603BranchPredictionEnable,
    ppcEC603BranchPredictorInvalidate,

    ppcEC603TextUpdate,
};

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
