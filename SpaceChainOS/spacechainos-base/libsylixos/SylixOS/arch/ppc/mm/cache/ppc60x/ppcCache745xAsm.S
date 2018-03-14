;/*********************************************************************************************************
;**
;**                                    中国软件开源组织
;**
;**                                   嵌入式实时操作系统
;**
;**                                       SylixOS(TM)
;**
;**                               Copyright  All Rights Reserved
;**
;**--------------文件信息--------------------------------------------------------------------------------
;**
;** 文   件   名: ppcCache745xAsm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2016 年 03 月 30 日
;**
;** 描        述: PowerPC MPC745X 体系构架 CACHE 驱动.
;*********************************************************************************************************/

#define __SYLIXOS_PPC603__                  0x603
#define __SYLIXOS_PPCEC603__                0xEC603
#define __SYLIXOS_PPC604__                  0x604

#define __SYLIXOS_PPC_CPU__                 __SYLIXOS_PPC604__           /*  MPC745X 是 604 的变种       */

#define __SYLIXOS_PPC745X__
#define __SYLIXOS_PPC_HAVE_ALTIVEC          1

/*********************************************************************************************************
  重新定义接口名
*********************************************************************************************************/

#define ppc60xICacheEnable                  ppc745xICacheEnable
#define ppc60xDCacheEnable                  ppc745xDCacheEnable
#define ppc60xICacheDisable                 ppc745xICacheDisable
#define ppc60xDCacheDisable                 ppc745xDCacheDisable
#define ppc60xICacheInvalidate              ppc745xICacheInvalidate
#define ppc60xDCacheInvalidate              ppc745xDCacheInvalidate
#define ppc60xDCacheClear                   ppc745xDCacheClear
#define ppc60xDCacheFlush                   ppc745xDCacheFlush
#define ppc60xICacheInvalidateAll           ppc745xICacheInvalidateAll
#define ppc60xDCacheInvalidateAll           ppc745xDCacheInvalidateAll
#define ppc60xDCacheFlushAll                ppc745xDCacheFlushAll
#define ppc60xDCacheClearAll                ppc745xDCacheClearAll
#define ppc60xBranchPredictionDisable       ppc745xBranchPredictionDisable
#define ppc60xBranchPredictionEnable        ppc745xBranchPredictionEnable
#define ppc60xBranchPredictorInvalidate     ppc745xBranchPredictorInvalidate
#define ppc60xTextUpdate                    ppc745xTextUpdate

/*********************************************************************************************************
  包含 60X CACHE 驱动
*********************************************************************************************************/

#include "ppcCache60xAsm.S"

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
