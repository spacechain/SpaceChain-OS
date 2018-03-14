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
;** 文   件   名: ppcL2Cache745xAsm.s
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2017 年 09 月 01 日
;**
;** 描        述: MPC745x 体系构架 L2 CACHE 驱动.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/kernel/cache_cfg.h>

#if LW_CFG_CACHE_EN > 0 && LW_CFG_PPC_CACHE_L2 > 0

#define __SYLIXOS_PPC745X__

/*********************************************************************************************************
  重新定义接口名
*********************************************************************************************************/

#define ppc750L2CacheSetCR              ppc745xL2CacheSetCR
#define ppc750L2CacheGetCR              ppc745xL2CacheGetCR

#define ppc750L2CacheInit               ppc745xL2CacheInit

#define ppc750L2CacheEnable             ppc745xL2CacheEnable
#define ppc750L2CacheDisable            ppc745xL2CacheDisable
#define ppc750L2CacheIsEnable           ppc745xL2CacheIsEnable

#define ppc750L2CacheInvalidateAll      ppc745xL2CacheInvalidateAll

#define ppc750L2CacheFlushAllSW         ppc745xL2CacheFlushAllSW
#define ppc750L2CacheFlushAllHW         ppc745xL2CacheFlushAllHW
#define ppc750L2CacheSetWriteThrough    ppc745xL2CacheSetWriteThrough

/*********************************************************************************************************
  包含 750 L2 CACHE 驱动
*********************************************************************************************************/

#include "ppcL2Cache750Asm.S"

    FILE_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
