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
;** 文   件   名: armCacheV7Asm.S
;**
;** 创   建   人: Han.Hui (韩辉)
;**
;** 文件创建日期: 2013 年 12 月 09 日
;**
;** 描        述: ARMv7 体系构架 CACHE 驱动.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/kernel/cache_cfg.h>

#if !defined(__SYLIXOS_ARM_ARCH_M__)

#if LW_CFG_CACHE_EN > 0

    FILE_BEGIN()
    
    EXPORT_LABEL(armCacheV7CTR)
    EXPORT_LABEL(armCacheV7CCSIDR)
    EXPORT_LABEL(armCacheV7CLIDR)
    EXPORT_LABEL(armCacheV7CSSELR)

    EXPORT_LABEL(armDCacheV7Disable)
    EXPORT_LABEL(armDCacheV7FlushPoU)
    EXPORT_LABEL(armDCacheV7FlushAll)
    EXPORT_LABEL(armDCacheV7FlushAllPoU)
    EXPORT_LABEL(armDCacheV7ClearAll)
    EXPORT_LABEL(armDCacheV7ClearAllPoU)
    EXPORT_LABEL(armDCacheV7InvalidateAll)
    EXPORT_LABEL(armDCacheV7InvalidateAllPoU)
    
;/*********************************************************************************************************
;  ARMv7 cache levels 最多 7 级 CACHE
;*********************************************************************************************************/

#define ARMv7_CACHE_LEVEL1          0
#define ARMv7_CACHE_LEVEL2          1
#define ARMv7_CACHE_LEVEL3          2
#define ARMv7_CACHE_LEVEL4          3
#define ARMv7_CACHE_LEVEL5          4
#define ARMv7_CACHE_LEVEL6          5
#define ARMv7_CACHE_LEVEL7          6

;/*********************************************************************************************************
;  ARMv7 CLIDR 寄存器 CACHE 类型
;*********************************************************************************************************/

#define ARMv7_CLDR_ICACHE           0x1
#define ARMv7_CLID_DCACHE           0x2
#define ARMv7_CLID_UCACHE           0x4

;/*********************************************************************************************************
;  ARMv7 CSSEL 寄存器 CACHE 类型
;*********************************************************************************************************/

#define ARMv7_CSSEL_ICACHE          0x1                                 ;/* select instruction cache     */
#define ARMv7_CSSEL_DCACHE          0x0                                 ;/* select data or unified cache */

;/*********************************************************************************************************
;  ARMv7 CSSELR 寄存信息
;*********************************************************************************************************/

#define ARMv7_CCSID_LINESIZE_MSK    0x7                                 ;/* line size bit mask           */
#define ARMv7_CCSID_WAYNUM_MSK      0x1ff8                              ;/* associativity bit mask       */
#define ARMv7_CCSID_SETNUM_MSK      0xfffe000                           ;/* set number bit mask          */
#define ARMv7_CCSID_MODE_MSK        0xf0000000                          ;/* mode bit mask                */

#define ARMv7_CCSID_WAYNUM_SHFT     3                                   ;/* associativity field shift    */
#define ARMv7_CCSID_SETNUM_SHFT     13                                  ;/* set number field shift       */

#define ARMv7_CCSID_MODE_WT         0x80000000                          ;/* write-through mode           */
#define ARMv7_CCSID_MODE_WB         0x40000000                          ;/* write-back mode              */
#define ARMv7_CCSID_MODE_RA         0x20000000                          ;/* read-allocate mode           */
#define ARMv7_CCSID_MODE_WA         0x10000000                          ;/* write-allocate mode          */

;/*********************************************************************************************************
;  ARMv7 获得 CACHE 相关寄存器
;*********************************************************************************************************/

FUNC_DEF(armCacheV7CTR)
    MRC     p15, 0, R0, c0, c0, 1
    BX      LR
    FUNC_END()

FUNC_DEF(armCacheV7CCSIDR)
    MRC     p15, 1, R0, c0, c0, 0
    BX      LR
    FUNC_END()

FUNC_DEF(armCacheV7CLIDR)
    MRC     p15, 1, R0, c0, c0, 1
    BX      LR
    FUNC_END()

FUNC_DEF(armCacheV7CSSELR)
    MRC     p15, 2, R0, c0, c0, 0
    BX      LR
    FUNC_END()

;/*********************************************************************************************************
;  ARMv7 关闭 DCACHE
;*********************************************************************************************************/

FUNC_DEF(armDCacheV7Disable)
    MRC     p15, 0, R0, c1, c0, 0
    AND     R0, R0, #P15_R1_C
    CMP     R0, #0
    BEQ     dcache_not_en

    STMFD   SP!, {LR}
    BL      armDCacheV7ClearAll
    LDMFD   SP!, {LR}
    
    ARM_DSB()
    ARM_ISB()
    
    MRC     p15, 0, R0, c1, c0, 0
    BIC     R0, R0, #P15_R1_C
    MCR     p15, 0, R0, c1, c0, 0
    BX      LR
	
LINE_LABEL(dcache_not_en)
    STMFD   SP!, {LR}
    BL      armDCacheV7InvalidateAll
    LDMFD   SP!, {LR}
    BX      LR
    FUNC_END()

;/*********************************************************************************************************
;  将指定虚拟地址的 DCACHE 回写 (point of unification (PoU))
;  R0=void *start; R1=void *end; R2=cache line size
;*********************************************************************************************************/

FUNC_DEF(armDCacheV7FlushPoU)
    ARM_DSB()
    ARM_ISB()
1:
    MCR     p15, 0, R0, c7, c11, 1
    ADD     R0, R0, R2
    CMP     R0, R1
    BCC     1b
    ARM_DSB()
    ARM_ISB()
    BX      LR
    FUNC_END()

;/*********************************************************************************************************
;  ARMv7 CLIDR 寄存器 level shift & mask
;*********************************************************************************************************/

#define LoUIS_MASK      0x00e00000
#define LoUIS_SHIFT     20
#define LoC_MASK        0x07000000
#define LoC_SHIFT       23
#define LoUU_MASK       0x38000000
#define LoUU_SHIFT      26

;/*********************************************************************************************************
;  ARMv7 DCACHE 操作宏
;*********************************************************************************************************/

#define _FLD_MAX_WAY    0x3ff
#define _FLD_MAX_IDX    0x7fff

MACRO_DEF(armDCacheV7Op Op CRm LoXM LoXS)
    STMFD   SP!, {R4-R11}
    ARM_DMB()
    MRC     p15, 1, R0, c0, c0, 1                                       ;/* Read CLID register           */
    ANDS    R3, R0, #\LoXM                                              ;/* Get Level of Coherency       */
    MOV     R3, R3, LSR #\LoXS
    BEQ     finished\Op
    MOV     R10, #0

LINE_LABEL(loop1\Op)
    ADD     R2, R10, R10, LSR #1
    MOV     R1, R0, LSR R2
    AND     R1, R1, #7
    CMP     R1, #2
    BLT     skip\Op
    MCR     p15, 2, R10, c0, c0, 0
    ARM_ISB()
    MRC     p15, 1, R1, c0, c0, 0
    AND     R2, R1, #7
    ADD     R2, R2, #4
    LDR     R4, =_FLD_MAX_WAY
    ANDS    R4, R4, R1, LSR #3
#if __SYLIXOS_ARM_ARCH__ >= 5
    CLZ     R5, R4
#else
    .long   0xe16f5f14
#endif
    LDR     R7, =_FLD_MAX_IDX
    ANDS    R7, R7, R1, LSR #13

LINE_LABEL(loop2\Op)
    MOV     R9, R4

LINE_LABEL(loop3\Op)
    ORR     R11, R10, R9, LSL R5
    ORR     R11, R11, R7, LSL R2
    MCR     p15, 0, R11, c7, \CRm, 2
    SUBS    R9, R9, #1
    BGE     loop3\Op
    SUBS    R7, R7, #1
    BGE     loop2\Op

LINE_LABEL(skip\Op)
    ADD     R10, R10, #2
    CMP     R3, R10
    BGT     loop1\Op

LINE_LABEL(finished\Op)
    MOV     R10, #0                                                     ;/*  swith back to cache lvl 0   */
    MCR     p15, 2, R10, c0, c0, 0                                      ;/*  select cache lvl in csselr  */

    ARM_DSB()
    ARM_ISB()
    LDMFD   SP!, {R4-R11}
    BX      LR
    MACRO_END()

;/*********************************************************************************************************
;  ARMv7 回写 DCACHE
;*********************************************************************************************************/

FUNC_DEF(armDCacheV7FlushAll)
    armDCacheV7Op   FlushAll            c10     LoC_MASK    LoC_SHIFT
    FUNC_END()

;/*********************************************************************************************************
;  ARMv7 回写 DCACHE PoU
;*********************************************************************************************************/

FUNC_DEF(armDCacheV7FlushAllPoU)
    armDCacheV7Op   FlushAllPoU         c10     LoUU_MASK   LoUU_SHIFT
    FUNC_END()
    
;/*********************************************************************************************************
;  ARMv7 回写并无效 DCACHE
;*********************************************************************************************************/

FUNC_DEF(armDCacheV7ClearAll)
    armDCacheV7Op   ClearAll            c14     LoC_MASK    LoC_SHIFT
    FUNC_END()

;/*********************************************************************************************************
;  ARMv7 回写并无效 DCACHE PoU
;*********************************************************************************************************/

FUNC_DEF(armDCacheV7ClearAllPoU)
    armDCacheV7Op   ClearAllPoU         c14     LoUU_MASK   LoUU_SHIFT
    FUNC_END()

;/*********************************************************************************************************
;  ARMv7 无效 DCACHE
;*********************************************************************************************************/

FUNC_DEF(armDCacheV7InvalidateAll)
    armDCacheV7Op   InvalidateAll       c6      LoC_MASK    LoC_SHIFT
    FUNC_END()

;/*********************************************************************************************************
;  ARMv7 无效 DCACHE
;*********************************************************************************************************/

FUNC_DEF(armDCacheV7InvalidateAllPoU)
    armDCacheV7Op   InvalidateAllPoU    c6      LoUU_MASK   LoUU_SHIFT
    FUNC_END()

    FILE_END()

#endif
#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
