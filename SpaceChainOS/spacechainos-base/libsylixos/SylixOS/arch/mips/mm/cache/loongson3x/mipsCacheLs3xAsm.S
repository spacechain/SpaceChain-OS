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
;** 文   件   名: mipsCacheLs3xAsm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2016 年 11 月 02 日
;**
;** 描        述: Loongson3x 体系构架 CACHE 驱动.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/kernel/cache_cfg.h>

#if LW_CFG_CACHE_EN > 0

    EXPORT_LABEL(ls3xCacheDisableHw)
    EXPORT_LABEL(ls3xCacheEnableHw)

    FILE_BEGIN()

;/*********************************************************************************************************
;  禁能 CACHE
;*********************************************************************************************************/

FUNC_DEF(ls3xCacheDisableHw)
    .set    push
    .set    noreorder
    .set    volatile

    JR      RA
    NOP

    .set    pop
    FUNC_END(ls3xCacheDisableHw)

;/*********************************************************************************************************
;  使能 CACHE
;*********************************************************************************************************/

FUNC_DEF(ls3xCacheEnableHw)
    .set    push
    .set    noreorder
    .set    volatile

    MFC0(T0, CP0_CONFIG)
    ANDI    T0 , T0 , M_ConfigK0
    LI      T1 , MIPS_CACHABLE_NONCOHERENT
    BEQ     T0 , T1 , 1f
    NOP

    MFC0(T0, CP0_CONFIG)
    AND     T0 , T0 , ~M_ConfigK0
    ORI     T0 , MIPS_CACHABLE_NONCOHERENT
    MTC0(T0, CP0_CONFIG)

1:
    JR      RA
    NOP

    .set    pop
    FUNC_END(ls3xCacheEnableHw)

    FILE_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
