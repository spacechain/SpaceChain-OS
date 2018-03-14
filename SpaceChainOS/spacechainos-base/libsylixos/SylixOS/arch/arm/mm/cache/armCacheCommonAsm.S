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
;** 文   件   名: armCacheCommon.S
;**
;** 创   建   人: Han.Hui (韩辉)
;**
;** 文件创建日期: 2013 年 12 月 09 日
;**
;** 描        述: ARM 体系架构 CACHE 通用函数支持.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/kernel/cache_cfg.h>

#if !defined(__SYLIXOS_ARM_ARCH_M__)

#if LW_CFG_CACHE_EN > 0

    FILE_BEGIN()

    EXPORT_LABEL(armCacheTypeReg)
    EXPORT_LABEL(armCacheRetireRR)
    EXPORT_LABEL(armCacheRetireDefault)
    EXPORT_LABEL(armICacheEnable)
    EXPORT_LABEL(armDCacheEnable)
    EXPORT_LABEL(armICacheDisable)
    EXPORT_LABEL(armICacheInvalidate)
    EXPORT_LABEL(armICacheInvalidateAll)
    EXPORT_LABEL(armDCacheInvalidate)
    EXPORT_LABEL(armDCacheFlush)
    EXPORT_LABEL(armDCacheClear)

;/*********************************************************************************************************
;  获得 CP15 基本信息
;*********************************************************************************************************/

FUNC_DEF(armCacheTypeReg)
    MRC     p15, 0, R0, c0, c0, 1
    BX      LR
    FUNC_END()
    
;/*********************************************************************************************************
;  设置 CACHE 淘汰算法 
;*********************************************************************************************************/

FUNC_DEF(armCacheRetireRR)
    MRC     p15, 0, R0, c1, c0, 0
    ORR     R0, R0, #P15_R1_RR
    MCR     p15, 0, R0, c1, c0, 0
    BX      LR
    FUNC_END()

FUNC_DEF(armCacheRetireDefault)
    MRC     p15, 0, R0, c1, c0, 0
    BIC     R0, R0, #P15_R1_RR
    MCR     p15, 0, R0, c1, c0, 0
    BX      LR
    FUNC_END()

;/*********************************************************************************************************
;  使能 CACHE
;*********************************************************************************************************/

FUNC_DEF(armICacheEnable)
    MRC     p15, 0, R0, c1, c0, 0
    ORR     R0, R0, #P15_R1_I
    MCR     p15, 0, R0, c1, c0, 0
    BX      LR
    FUNC_END()

FUNC_DEF(armDCacheEnable)
    MRC     p15, 0, R0, c1, c0, 0
    ORR     R0, R0, #P15_R1_C
    MCR     p15, 0, R0, c1, c0, 0
    BX      LR
    FUNC_END()

;/*********************************************************************************************************
;  禁能 ICACHE
;*********************************************************************************************************/

FUNC_DEF(armICacheDisable)
    MRC     p15, 0, R0, c1, c0, 0
    AND     R0, R0, #P15_R1_I
    CMP     R0, #0
    BEQ     icache_not_en
    
    MRC     p15, 0, R0, c1, c0, 0
    BIC     R0, R0, #P15_R1_I
    MCR     p15, 0, R0, c1, c0, 0

LINE_LABEL(icache_not_en)
    BX      LR
    FUNC_END()

;/*********************************************************************************************************
;  无效整个 ICACHE
;*********************************************************************************************************/

FUNC_DEF(armICacheInvalidateAll)
    ARM_DSB()
    ARM_ISB()
    MOV     R0, #0
#if LW_CFG_SMP_EN > 0
    SMP_UP_CHECK(R1)
    MCRHI   p15, 0, R0, c7, c1, 0                                       ;/* ICIALLUIS SMP                */
    MCRLS   p15, 0, R0, c7, c5, 0                                       ;/* ICIALLU UP                   */
#if __SYLIXOS_ARM_ARCH__ >= 6
    MCRHI   p15, 0, R0, c7, c1, 6                                       ;/* BPIALLIS SMP                 */
    MCRLS   p15, 0, R0, c7, c5, 6                                       ;/* BPIALL UP                    */
#endif
#else
    MCR     p15, 0, R0, c7, c5, 0                                       ;/* ICIALLU UP                   */
#if __SYLIXOS_ARM_ARCH__ >= 6
    MCR     p15, 0, R0, c7, c5, 6                                       ;/* BPIALL UP                    */
#endif
#endif
    ARM_NOP(R0)                                                         ;/* 3 * NOP                      */
    ARM_NOP(R0)
    ARM_NOP(R0)
    ARM_DSB()
    ARM_ISB()
    BX      LR
    FUNC_END()

;/*********************************************************************************************************
;  无效 ICACHE R0=void *start; R1=void *end; R2=cache line size
;*********************************************************************************************************/

FUNC_DEF(armICacheInvalidate)
    ARM_DSB()
    ARM_ISB()
1:
    MCR     p15, 0, R0, c7, c5, 1
    ADD     R0, R0, R2
    CMP     R0, R1
    BCC     1b

#if __SYLIXOS_ARM_ARCH__ >= 6
    MOV     R0, #0
#if LW_CFG_SMP_EN > 0
    SMP_UP_CHECK(R1)
    MCRHI   p15, 0, R0, c7, c1, 6                                       ;/* BPIALLIS SMP                 */
    MCRLS   p15, 0, R0, c7, c5, 6                                       ;/* BPIALL UP                    */
#else
    MCR     p15, 0, R0, c7, c5, 6                                       ;/* BPIALL UP                    */
#endif
#endif
    ARM_NOP(R0)                                                         ;/* 3 * NOP                      */
    ARM_NOP(R0)
    ARM_NOP(R0)
    ARM_DSB()
    ARM_ISB()
    BX      LR
    FUNC_END()

;/*********************************************************************************************************
;  将指定虚拟地址的 DCACHE 无效  R0=void *start; R1=void *end; R2=cache line size
;*********************************************************************************************************/

FUNC_DEF(armDCacheInvalidate)
    ARM_DSB()
    ARM_ISB()
1:
    MCR     p15, 0, R0, c7, c6, 1
    ADD     R0, R0, R2
    CMP     R0, R1
    BCC     1b
    ARM_DSB()
    ARM_ISB()
    BX      LR
    FUNC_END()

;/*********************************************************************************************************
;  将指定虚拟地址的 DCACHE 回写  R0=void *start; R1=void *end; R2=cache line size
;*********************************************************************************************************/

FUNC_DEF(armDCacheFlush)
    ARM_DSB()
    ARM_ISB()
1:
    MCR     p15, 0, R0, c7, c10, 1
    ADD     R0, R0, R2
    CMP     R0, R1
    BCC     1b
    ARM_DSB()
    ARM_ISB()
    BX      LR
    FUNC_END()

;/*********************************************************************************************************
;  将指定虚拟地址的 DCACHE 回写并无效  R0=void *start; R1=void *end; R2=cache line size
;*********************************************************************************************************/

FUNC_DEF(armDCacheClear)
    ARM_DSB()
    ARM_ISB()
1:
    MCR     p15, 0, R0, c7, c14, 1
    ADD     R0, R0, R2
    CMP     R0, R1
    BCC     1b
    ARM_DSB()
    ARM_ISB()
    BX      LR
    FUNC_END()

    FILE_END()
    
#endif
#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
