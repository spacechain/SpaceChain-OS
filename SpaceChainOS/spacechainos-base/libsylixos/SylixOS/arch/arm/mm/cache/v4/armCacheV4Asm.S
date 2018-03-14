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
;** 文   件   名: armCacheV4Asm.S
;**
;** 创   建   人: Han.Hui (韩辉)
;**
;** 文件创建日期: 2013 年 12 月 09 日
;**
;** 描        述: ARMv4 体系构架 CACHE 驱动.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/kernel/cache_cfg.h>

#if !defined(__SYLIXOS_ARM_ARCH_M__)

#if LW_CFG_CACHE_EN > 0

    FILE_BEGIN()
    
    EXPORT_LABEL(armDCacheV4Disable)
    EXPORT_LABEL(armDCacheV4FlushAll)
    EXPORT_LABEL(armDCacheV4ClearAll)
    EXPORT_LABEL(armDCacheV4InvalidateAll)
    
;/*********************************************************************************************************
;  ARMv4 关闭 DCACHE
;*********************************************************************************************************/

FUNC_DEF(armDCacheV4Disable)
    MRC     p15, 0, R0, c1, c0, 0
    AND     R0, R0, #P15_R1_C
    CMP     R0, #0
    BEQ     dcache_not_en

    ;/* 
    ; * 回写并无效 DCACHE
    ; */
    STMFD   SP!, {LR}
    BL      armDCacheV4ClearAll
    LDMFD   SP!, {LR}
    
    ;/* 
    ; * 保证 victim 指针不指向锁定区 
    ; */
    MRC     p15, 0, R0, c9, c0, 0                                       ;/* Read D-cache lockdown base   */
    MCR     p15, 0, R0, c9, c0, 0                                       ;/* Write D-cache lockdown base  */

#if __SYLIXOS_ARM_ARCH__ < 8
    MOV     R0, #0
    MCR     p15, 0, R0, c7, c10, 4                                      ;/* Drain write-buffer           */
#endif
    
    MRC     p15, 0, R0, c1, c0, 0
    BIC     R0, R0, #P15_R1_C
    MCR     p15, 0, R0, c1, c0, 0
    BX      LR
	
LINE_LABEL(dcache_not_en)
    ;/* 
    ; * 无效 DCACHE
    ; */
    STMFD   SP!, {LR}
    BL      armDCacheV4InvalidateAll
    LDMFD   SP!, {LR}
    BX      LR
    FUNC_END()
    
;/*********************************************************************************************************
;  ARMv4 回写 DCACHE
;*********************************************************************************************************/

#define CACHE_NWAYS         64
#define CACHE_NWAYS_SFT     26
#define CACHE_NSETS         8
#define CACHE_NSETS_SFT     5

FUNC_DEF(armDCacheV4FlushAll)
    STMFD   SP!, {R4-R6}
    
    MOV     R5, #0
1:
    MOV     R4, #0
2:

    MOV     R6, R5, LSL #CACHE_NWAYS_SFT
    ORR     R0, R6, R4, LSL #CACHE_NSETS_SFT
    MCR     p15, 0, R0, c7, c10, 2                                      ;/*  回写 D CACHE                */
    
    ADD     R4, R4, #1
    CMP     R4, #CACHE_NSETS
    BLT     2b
    
    ADD     R5, R5, #1
    CMP     R5, #CACHE_NWAYS
    BLT     1b

#if __SYLIXOS_ARM_ARCH__ < 8
    MOV     R0, #0
    MCR     p15, 0, R0, c7, c10, 4                                      ;/* Drain write-buffer           */
#endif
    
    ;/* 
    ; * 保证 victim 指针不指向锁定区 
    ; */
    MRC     p15, 0, R0, c9, c0, 0                                       ;/* Read D-cache lockdown base   */
    MCR     p15, 0, R0, c9, c0, 0                                       ;/* Write D-cache lockdown base  */
    
    LDMFD   SP!, {R4-R6}
    BX      LR
    FUNC_END()
    
;/*********************************************************************************************************
;  ARMv4 回写并无效 DCACHE
;*********************************************************************************************************/

FUNC_DEF(armDCacheV4ClearAll)
    STMFD   SP!, {R4-R6}
    
    MOV     R5, #0
1:
    MOV     R4, #0
2:

    MOV     R6, R5, LSL #CACHE_NWAYS_SFT
    ORR     R0, R6, R4, LSL #CACHE_NSETS_SFT
    MCR     p15, 0, R0, c7, c14, 2                                      ;/*  回写并无效 D CACHE          */
    
    ADD     R4, R4, #1
    CMP     R4, #CACHE_NSETS
    BLT     2b
    
    ADD     R5, R5, #1
    CMP     R5, #CACHE_NWAYS
    BLT     1b

#if __SYLIXOS_ARM_ARCH__ < 8
    MOV     R0, #0
    MCR     p15, 0, R0, c7, c10, 4                                      ;/* Drain write-buffer           */
#endif

    ;/* 
    ; * 保证 victim 指针不指向锁定区 
    ; */
    MRC     p15, 0, R0, c9, c0, 0                                       ;/* Read D-cache lockdown base   */
    MCR     p15, 0, R0, c9, c0, 0                                       ;/* Write D-cache lockdown base  */
    
    LDMFD   SP!, {R4-R6}
    BX      LR
    FUNC_END()

;/*********************************************************************************************************
;  ARMv4 无效 DCACHE
;*********************************************************************************************************/

FUNC_DEF(armDCacheV4InvalidateAll)
    STMFD   SP!, {R4-R6}

    MOV     R5, #0
1:
    MOV     R4, #0
2:

    MOV     R6, R5, LSL #CACHE_NWAYS_SFT
    ORR     R0, R6, R4, LSL #CACHE_NSETS_SFT
    MCR     p15, 0, R0, c7, c6, 2                                       ;/*  无效 D CACHE                */

    ADD     R4, R4, #1
    CMP     R4, #CACHE_NSETS
    BLT     2b

    ADD     R5, R5, #1
    CMP     R5, #CACHE_NWAYS
    BLT     1b

    ;/*
    ; * 保证 victim 指针不指向锁定区
    ; */
    MRC     p15, 0, R0, c9, c0, 0                                       ;/* Read D-cache lockdown base   */
    MCR     p15, 0, R0, c9, c0, 0                                       ;/* Write D-cache lockdown base  */

    LDMFD   SP!, {R4-R6}
    BX      LR
    FUNC_END()

    FILE_END()
    
#endif
#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
