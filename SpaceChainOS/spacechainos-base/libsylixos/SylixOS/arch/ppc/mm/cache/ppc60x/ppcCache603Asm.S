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
;** 文   件   名: ppcCache603Asm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2016 年 03 月 30 日
;**
;** 描        述: PowerPC 603 体系构架 CACHE 驱动.
;*********************************************************************************************************/

#define __SYLIXOS_PPC603__                  0x603
#define __SYLIXOS_PPCEC603__                0xEC603
#define __SYLIXOS_PPC604__                  0x604

#define __SYLIXOS_PPC_CPU__                 __SYLIXOS_PPC603__

/*********************************************************************************************************
  重新定义接口名
*********************************************************************************************************/

#define ppc60xICacheEnable                  ppc603ICacheEnable
#define ppc60xDCacheEnable                  ppc603DCacheEnable
#define ppc60xICacheDisable                 ppc603ICacheDisable
#define ppc60xDCacheDisable                 ppc603DCacheDisable
#define ppc60xICacheInvalidate              ppc603ICacheInvalidate
#define ppc60xDCacheInvalidate              ppc603DCacheInvalidate
#define ppc60xDCacheClear                   ppc603DCacheClear
#define ppc60xDCacheFlush                   ppc603DCacheFlush
#define ppc60xICacheInvalidateAll           ppc603ICacheInvalidateAll
#define ppc60xDCacheInvalidateAll           ppc603DCacheInvalidateAll
#define ppc60xDCacheFlushAll                ppc603DCacheFlushAll
#define ppc60xDCacheClearAll                ppc603DCacheClearAll
#define ppc60xBranchPredictionDisable       ppc603BranchPredictionDisable
#define ppc60xBranchPredictionEnable        ppc603BranchPredictionEnable
#define ppc60xBranchPredictorInvalidate     ppc603BranchPredictorInvalidate
#define ppc60xTextUpdate                    ppc603TextUpdate

/*********************************************************************************************************
  包含 60X CACHE 驱动
*********************************************************************************************************/

#include "ppcCache60xAsm.S"

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
