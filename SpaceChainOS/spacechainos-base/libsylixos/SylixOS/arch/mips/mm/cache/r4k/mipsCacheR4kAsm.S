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
;** 文   件   名: mipsCacheR4kAsm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2015 年 12 月 01 日
;**
;** 描        述: MIPS R4K 体系构架 CACHE 驱动.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/kernel/cache_cfg.h>
#include <config/cpu/cpu_cfg_mips.h>

#if LW_CFG_CACHE_EN > 0

    EXPORT_LABEL(mipsCacheR4kDisableHw)
    EXPORT_LABEL(mipsCacheR4kEnableHw)

    EXPORT_LABEL(mipsDCacheR4kLineFlush)
    EXPORT_LABEL(mipsDCacheR4kLineClear)
    EXPORT_LABEL(mipsDCacheR4kLineInvalidate)
    EXPORT_LABEL(mipsDCacheR4kIndexClear)
    EXPORT_LABEL(mipsDCacheR4kIndexStoreTag)

    EXPORT_LABEL(mipsICacheR4kLineInvalidate)
    EXPORT_LABEL(mipsICacheR4kIndexInvalidate)
    EXPORT_LABEL(mipsICacheR4kFill)
    EXPORT_LABEL(mipsICacheR4kIndexStoreTag)

    EXPORT_LABEL(mipsBranchPredictionDisable)
    EXPORT_LABEL(mipsBranchPredictionEnable)
    EXPORT_LABEL(mipsBranchPredictorInvalidate)

    IMPORT_LABEL(mipsICacheR4kInvalidateAll)
    IMPORT_LABEL(mipsDCacheR4kClearAll)
#if LW_CFG_MIPS_CACHE_L2 > 0
    IMPORT_LABEL(mipsL2R4kClearAll)
#endif

    WEAK(mipsBranchPredictionDisable)
    WEAK(mipsBranchPredictionEnable)
    WEAK(mipsBranchPredictorInvalidate)

    FILE_BEGIN()

;/*********************************************************************************************************
;  禁能 CACHE
;*********************************************************************************************************/

FUNC_DEF(mipsCacheR4kDisableHw)
    .set    push
    .set    noreorder
    .set    volatile

    MFC0(T0, CP0_CONFIG)
    ANDI    T0 , T0 , M_ConfigK0
    LI      T1 , MIPS_UNCACHED
    BEQ     T0 , T1 , 1f
    NOP

    SUBU    SP , 32
    SW      RA , 28(SP)
    SW      FP , 24(SP)
    MOV     FP , SP

    JAL     mipsDCacheR4kClearAll
    NOP

    JAL     mipsICacheR4kInvalidateAll
    NOP

#if LW_CFG_MIPS_CACHE_L2 > 0
    JAL     mipsL2R4kClearAll
    NOP
#endif

    MOV     SP , FP
    LW      RA , 28(SP)
    LW      FP , 24(SP)
    ADDU    SP , 32

    MFC0(T0, CP0_CONFIG)
    AND     T0 , T0 , ~M_ConfigK0
    ORI     T0 , T0 , MIPS_UNCACHED
    MTC0(T0, CP0_CONFIG)

1:
    JR      RA
    NOP

    .set    pop
    FUNC_END(mipsCacheR4kDisableHw)

;/*********************************************************************************************************
;  使能 CACHE
;*********************************************************************************************************/

FUNC_DEF(mipsCacheR4kEnableHw)
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
    FUNC_END(mipsCacheR4kEnableHw)

;/*********************************************************************************************************
;  CACHE 操作宏
;*********************************************************************************************************/

MACRO_DEF(CACHE_OP op addr)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile
    .set    mips32

    cache   \op , 0(\addr)

    JR      RA
    NOP

    .set    pop
    MACRO_END()

;/*********************************************************************************************************
;  回写一个 DCACHE 行
;*********************************************************************************************************/

FUNC_DEF(mipsDCacheR4kLineFlush)
    CACHE_OP    Hit_Writeback_D         A0
    FUNC_END(mipsDCacheR4kLineFlush)

;/*********************************************************************************************************
;  回写并无效一个 DCACHE 行
;*********************************************************************************************************/

FUNC_DEF(mipsDCacheR4kLineClear)
    CACHE_OP    Hit_Writeback_Inv_D     A0
    FUNC_END(mipsDCacheR4kLineClear)

;/*********************************************************************************************************
;  无效一个 DCACHE 行
;*********************************************************************************************************/

FUNC_DEF(mipsDCacheR4kLineInvalidate)
    CACHE_OP    Hit_Invalidate_D        A0
    FUNC_END(mipsDCacheR4kLineInvalidate)

;/*********************************************************************************************************
;  通过 Cache 存储器的地址索引回写并无效一个 DCACHE 行
;*********************************************************************************************************/

FUNC_DEF(mipsDCacheR4kIndexClear)
    CACHE_OP    Index_Writeback_Inv_D   A0
    FUNC_END(mipsDCacheR4kIndexClear)

;/*********************************************************************************************************
;  无效一个 ICACHE 行
;*********************************************************************************************************/

FUNC_DEF(mipsICacheR4kLineInvalidate)
    CACHE_OP    Hit_Invalidate_I        A0
    FUNC_END(mipsICacheR4kLineInvalidate)

;/*********************************************************************************************************
;  通过 CACHE 存储器的地址索引回写并无效一个 ICACHE 行
;*********************************************************************************************************/

FUNC_DEF(mipsICacheR4kIndexInvalidate)
    CACHE_OP    Index_Invalidate_I      A0
    FUNC_END(mipsICacheR4kIndexInvalidate)

;/*********************************************************************************************************
;  FILL ICACHE
;*********************************************************************************************************/

FUNC_DEF(mipsICacheR4kFill)
    CACHE_OP    Fill_I                  A0
    FUNC_END(mipsICacheR4kFill)

;/*********************************************************************************************************
;  设置 DCACHE 的 TAG
;*********************************************************************************************************/

FUNC_DEF(mipsDCacheR4kIndexStoreTag)
    CACHE_OP    Index_Store_Tag_D       A0
    FUNC_END(mipsDCacheR4kIndexStoreTag)

;/*********************************************************************************************************
;  设置 ICACHE 的 TAG
;*********************************************************************************************************/

FUNC_DEF(mipsICacheR4kIndexStoreTag)
    CACHE_OP    Index_Store_Tag_I       A0
    FUNC_END(mipsICacheR4kIndexStoreTag)

;/*********************************************************************************************************
;  禁能分支预测
;*********************************************************************************************************/

FUNC_DEF(mipsBranchPredictionDisable)
    JR      RA
    NOP
    FUNC_END(mipsBranchPredictionDisable)

;/*********************************************************************************************************
;  使能分支预测
;*********************************************************************************************************/

FUNC_DEF(mipsBranchPredictionEnable)
    JR      RA
    NOP
    FUNC_END(mipsBranchPredictionEnable)

;/*********************************************************************************************************
;  无效分支预测
;*********************************************************************************************************/

FUNC_DEF(mipsBranchPredictorInvalidate)
    JR      RA
    NOP
    FUNC_END(mipsBranchPredictorInvalidate)

    FILE_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
