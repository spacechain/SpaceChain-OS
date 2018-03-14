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
** 文   件   名: ppcCache.h
**
** 创   建   人: Yang.HaiFeng (杨海峰)
**
** 文件创建日期: 2016 年 01 月 18 日
**
** 描        述: PowerPC 体系构架 CACHE 驱动.
*********************************************************************************************************/

#ifndef __ARCH_PPCCACHE_H
#define __ARCH_PPCCACHE_H

/*********************************************************************************************************
  L1-CACHE 状态
*********************************************************************************************************/

#define L1_CACHE_I_EN   0x01
#define L1_CACHE_D_EN   0x02
#define L1_CACHE_EN     (L1_CACHE_I_EN | L1_CACHE_D_EN)
#define L1_CACHE_DIS    0x00

/*********************************************************************************************************
  CACHE 信息
*********************************************************************************************************/
typedef struct {
    UINT32              CACHE_uiSize;                                   /*  Cache 大小                  */
    UINT32              CACHE_uiLineSize;                               /*  Cache 行大小                */
    UINT32              CACHE_uiSetNr;                                  /*  组数                        */
    UINT32              CACHE_uiWayNr;                                  /*  路数                        */
    UINT32              CACHE_uiWaySize;                                /*  路大小                      */
} PPC_CACHE;

/*********************************************************************************************************
  CACHE 驱动
*********************************************************************************************************/

typedef struct {
    CPCHAR     L1CD_pcName;
    INT      (*L1CD_pfuncProbe)(CPCHAR  pcMachineName, PPC_CACHE  *pICache, PPC_CACHE  *pDCache);

    VOID     (*L1CD_pfuncDCacheDisable)(VOID);
    VOID     (*L1CD_pfuncDCacheEnable)(VOID);
    VOID     (*L1CD_pfuncICacheDisable)(VOID);
    VOID     (*L1CD_pfuncICacheEnable)(VOID);

    /*
     * 除以下三个 All 函数外, 所有函数必须实现
     */
    VOID     (*L1CD_pfuncDCacheClearAll)(VOID);
    VOID     (*L1CD_pfuncDCacheFlushAll)(VOID);
    VOID     (*L1CD_pfuncICacheInvalidateAll)(VOID);

    VOID     (*L1CD_pfuncDCacheClear)(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
    VOID     (*L1CD_pfuncDCacheFlush)(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
    VOID     (*L1CD_pfuncDCacheInvalidate)(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
    VOID     (*L1CD_pfuncICacheInvalidate)(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);

    VOID     (*L1CD_pfuncBranchPredictionDisable)(VOID);
    VOID     (*L1CD_pfuncBranchPredictionEnable)(VOID);
    VOID     (*L1CD_pfuncBranchPredictorInvalidate)(VOID);

    VOID     (*L1CD_pfuncTextUpdate)(PVOID  pvStart, PVOID  pvEnd,
                                     UINT32  uiICacheLineSize, UINT32  uiDCacheLineSize);
} PPC_L1C_DRIVER;

/*********************************************************************************************************
  CACHE 函数声明
*********************************************************************************************************/

INT  ppcCacheInit(LW_CACHE_OP *pcacheop,
                  CACHE_MODE   uiInstruction,
                  CACHE_MODE   uiData,
                  CPCHAR       pcMachineName);

INT  ppcCacheReset(CPCHAR  pcMachineName);

INT  ppcCacheStatus(VOID);

#endif                                                                  /*  __ARCH_PPCCACHE_H           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
