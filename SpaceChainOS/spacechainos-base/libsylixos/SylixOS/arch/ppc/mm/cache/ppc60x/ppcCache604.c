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
** 文   件   名: ppcCache604.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 03 月 30 日
**
** 描        述: PowerPC 604 体系构架 CACHE 驱动.
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
extern VOID     ppc604DCacheDisable(VOID);
extern VOID     ppc604DCacheEnable(VOID);
extern VOID     ppc604ICacheDisable(VOID);
extern VOID     ppc604ICacheEnable(VOID);

extern VOID     ppc604DCacheClearAll(VOID);
extern VOID     ppc604DCacheFlushAll(VOID);
extern VOID     ppc604ICacheInvalidateAll(VOID);

extern VOID     ppc604DCacheClear(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppc604DCacheFlush(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppc604DCacheInvalidate(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppc604ICacheInvalidate(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);

extern VOID     ppc604BranchPredictionDisable(VOID);
extern VOID     ppc604BranchPredictionEnable(VOID);
extern VOID     ppc604BranchPredictorInvalidate(VOID);

extern VOID     ppc604TextUpdate(PVOID  pvStart, PVOID  pvEnd,
                                 UINT32  uiICacheLineSize, UINT32  uiDCacheLineSize);
/*********************************************************************************************************
** 函数名称: ppc604CacheProbe
** 功能描述: CACHE 探测
** 输　入  : pcMachineName         机器名
**           pICache               I-Cache 信息
**           pDCache               D-Cache 信息
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT   ppc604CacheProbe (CPCHAR  pcMachineName, PPC_CACHE  *pICache, PPC_CACHE  *pDCache)
{
    if (lib_strcmp(pcMachineName, PPC_MACHINE_750) == 0) {
        pICache->CACHE_uiLineSize  = 32;
        pICache->CACHE_uiWayNr     = 8;
        pICache->CACHE_uiSetNr     = 128;
        pICache->CACHE_uiSize      = pICache->CACHE_uiSetNr * pICache->CACHE_uiWayNr * \
                                     pICache->CACHE_uiLineSize;
        pICache->CACHE_uiWaySize   = pICache->CACHE_uiSetNr * pICache->CACHE_uiLineSize;

        pDCache->CACHE_uiLineSize  = 32;
        pDCache->CACHE_uiWayNr     = 8;
        pDCache->CACHE_uiSetNr     = 128;
        pDCache->CACHE_uiSize      = pDCache->CACHE_uiSetNr * pDCache->CACHE_uiWayNr * \
                                     pDCache->CACHE_uiLineSize;
        pDCache->CACHE_uiWaySize   = pDCache->CACHE_uiSetNr * pDCache->CACHE_uiLineSize;
        return  (ERROR_NONE);

    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
  604 CACHE 驱动
*********************************************************************************************************/
PPC_L1C_DRIVER  _G_ppc604CacheDriver = {
    "604",
    ppc604CacheProbe,

    ppc604DCacheDisable,
    ppc604DCacheEnable,
    ppc604ICacheDisable,
    ppc604ICacheEnable,

    ppc604DCacheClearAll,
    ppc604DCacheFlushAll,
    ppc604ICacheInvalidateAll,

    ppc604DCacheClear,
    ppc604DCacheFlush,
    ppc604DCacheInvalidate,
    ppc604ICacheInvalidate,

    ppc604BranchPredictionDisable,
    ppc604BranchPredictionEnable,
    ppc604BranchPredictorInvalidate,

    ppc604TextUpdate,
};

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
