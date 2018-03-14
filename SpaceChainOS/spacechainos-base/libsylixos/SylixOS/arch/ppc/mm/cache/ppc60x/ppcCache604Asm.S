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
;** 文   件   名: ppcCache604Asm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2016 年 03 月 30 日
;**
;** 描        述: PowerPC 604 体系构架 CACHE 驱动.
;*********************************************************************************************************/

#define __SYLIXOS_PPC604__                  0x604
#define __SYLIXOS_PPCEC604__                0xEC604
#define __SYLIXOS_PPC604__                  0x604

#define __SYLIXOS_PPC_CPU__                 __SYLIXOS_PPC604__
#define __SYLIXOS_PPC_HAVE_ALTIVEC          1
#define ARCH_PPC604_USE_DCFA

/*********************************************************************************************************
  重新定义接口名
*********************************************************************************************************/

#define ppc60xICacheEnable                  ppc604ICacheEnable
#define ppc60xDCacheEnable                  ppc604DCacheEnable
#define ppc60xICacheDisable                 ppc604ICacheDisable
#define ppc60xDCacheDisable                 ppc604DCacheDisable
#define ppc60xICacheInvalidate              ppc604ICacheInvalidate
#define ppc60xDCacheInvalidate              ppc604DCacheInvalidate
#define ppc60xDCacheClear                   ppc604DCacheClear
#define ppc60xDCacheFlush                   ppc604DCacheFlush
#define ppc60xICacheInvalidateAll           ppc604ICacheInvalidateAll
#define ppc60xDCacheInvalidateAll           ppc604DCacheInvalidateAll
#define ppc60xDCacheFlushAll                ppc604DCacheFlushAll
#define ppc60xDCacheClearAll                ppc604DCacheClearAll
#define ppc60xBranchPredictionDisable       ppc604BranchPredictionDisable
#define ppc60xBranchPredictionEnable        ppc604BranchPredictionEnable
#define ppc60xBranchPredictorInvalidate     ppc604BranchPredictorInvalidate
#define ppc60xTextUpdate                    ppc604TextUpdate

/*********************************************************************************************************
  包含 60X CACHE 驱动
*********************************************************************************************************/

#include "ppcCache60xAsm.S"

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
