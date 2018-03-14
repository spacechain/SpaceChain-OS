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
;** 文   件   名: ppcCacheEC603Asm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2016 年 03 月 30 日
;**
;** 描        述: PowerPC EC603 体系构架 CACHE 驱动.
;*********************************************************************************************************/

#define __SYLIXOS_PPC603__                  0x603
#define __SYLIXOS_PPCEC603__                0xEC603
#define __SYLIXOS_PPC604__                  0x604

#define __SYLIXOS_PPC_CPU__                 __SYLIXOS_PPCEC603__

/*********************************************************************************************************
  重新定义接口名
*********************************************************************************************************/

#define ppc60xICacheEnable                  ppcEC603ICacheEnable
#define ppc60xDCacheEnable                  ppcEC603DCacheEnable
#define ppc60xICacheDisable                 ppcEC603ICacheDisable
#define ppc60xDCacheDisable                 ppcEC603DCacheDisable
#define ppc60xICacheInvalidate              ppcEC603ICacheInvalidate
#define ppc60xDCacheInvalidate              ppcEC603DCacheInvalidate
#define ppc60xDCacheClear                   ppcEC603DCacheClear
#define ppc60xDCacheFlush                   ppcEC603DCacheFlush
#define ppc60xICacheInvalidateAll           ppcEC603ICacheInvalidateAll
#define ppc60xDCacheInvalidateAll           ppcEC603DCacheInvalidateAll
#define ppc60xDCacheFlushAll                ppcEC603DCacheFlushAll
#define ppc60xDCacheClearAll                ppcEC603DCacheClearAll
#define ppc60xBranchPredictionDisable       ppcEC603BranchPredictionDisable
#define ppc60xBranchPredictionEnable        ppcEC603BranchPredictionEnable
#define ppc60xBranchPredictorInvalidate     ppcEC603BranchPredictorInvalidate
#define ppc60xTextUpdate                    ppcEC603TextUpdate

/*********************************************************************************************************
  包含 60X CACHE 驱动
*********************************************************************************************************/

#include "ppcCache60xAsm.S"

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
