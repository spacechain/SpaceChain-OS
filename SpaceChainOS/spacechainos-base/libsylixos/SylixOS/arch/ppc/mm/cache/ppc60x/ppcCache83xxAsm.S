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
;** 文   件   名: ppcCache83xxAsm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2016 年 03 月 30 日
;**
;** 描        述: PowerPC MPC83XX 体系构架 CACHE 驱动.
;*********************************************************************************************************/

#define __SYLIXOS_PPC603__                  0x603
#define __SYLIXOS_PPCEC603__                0xEC603
#define __SYLIXOS_PPC604__                  0x604

#define __SYLIXOS_PPC_CPU__                 __SYLIXOS_PPC603__           /*  MPC83XX 是 603 的变种       */

#define __SYLIXOS_PPC83XX__

/*********************************************************************************************************
  重新定义接口名
*********************************************************************************************************/

#define ppc60xICacheEnable                  ppc83xxICacheEnable
#define ppc60xDCacheEnable                  ppc83xxDCacheEnable
#define ppc60xICacheDisable                 ppc83xxICacheDisable
#define ppc60xDCacheDisable                 ppc83xxDCacheDisable
#define ppc60xICacheInvalidate              ppc83xxICacheInvalidate
#define ppc60xDCacheInvalidate              ppc83xxDCacheInvalidate
#define ppc60xDCacheClear                   ppc83xxDCacheClear
#define ppc60xDCacheFlush                   ppc83xxDCacheFlush
#define ppc60xICacheInvalidateAll           ppc83xxICacheInvalidateAll
#define ppc60xDCacheInvalidateAll           ppc83xxDCacheInvalidateAll
#define ppc60xDCacheFlushAll                ppc83xxDCacheFlushAll
#define ppc60xDCacheClearAll                ppc83xxDCacheClearAll
#define ppc60xBranchPredictionDisable       ppc83xxBranchPredictionDisable
#define ppc60xBranchPredictionEnable        ppc83xxBranchPredictionEnable
#define ppc60xBranchPredictorInvalidate     ppc83xxBranchPredictorInvalidate
#define ppc60xTextUpdate                    ppc83xxTextUpdate

/*********************************************************************************************************
  包含 60X CACHE 驱动
*********************************************************************************************************/

#include "ppcCache60xAsm.S"

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
