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
;** 文   件   名: armCacheV5Asm.S
;**
;** 创   建   人: Han.Hui (韩辉)
;**
;** 文件创建日期: 2013 年 12 月 09 日
;**
;** 描        述: ARMv5 体系构架 CACHE 驱动.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/kernel/cache_cfg.h>

#if !defined(__SYLIXOS_ARM_ARCH_M__)

#if LW_CFG_CACHE_EN > 0

    FILE_BEGIN()
    
    EXPORT_LABEL(armDCacheV5Disable)
    EXPORT_LABEL(armDCacheV5FlushAll)
    EXPORT_LABEL(armDCacheV5ClearAll)
    EXPORT_LABEL(armDCacheV5InvalidateAll)
    
;/*********************************************************************************************************
;  ARMv5 关闭 DCACHE
;*********************************************************************************************************/

FUNC_DEF(armDCacheV5Disable)
    MRC     p15, 0, R0, c1, c0, 0
    AND     R0, R0, #P15_R1_C
    CMP     R0, #0
    BEQ     dcache_not_en

    STMFD   SP!, {LR}
    BL      armDCacheV5ClearAll
    LDMFD   SP!, {LR}
    
#if __SYLIXOS_ARM_ARCH__ < 8
    MOV     R0, #0
    MCR     p15, 0, R0, c7, c10, 4                                      ;/* Drain write-buffer           */
#endif
    
    MRC     p15, 0, R0, c1, c0, 0
    BIC     R0, R0, #P15_R1_C
    MCR     p15, 0, R0, c1, c0, 0
    BX      LR
	
LINE_LABEL(dcache_not_en)
    STMFD   SP!, {LR}
    BL      armDCacheV5InvalidateAll
    LDMFD   SP!, {LR}
    BX      LR
    FUNC_END()
    
;/*********************************************************************************************************
;  ARMv5 回写 DCACHE
;*********************************************************************************************************/

FUNC_DEF(armDCacheV5FlushAll)
    MRC     p15, 0, R15, c7, c10, 3                                     ;/*  测试回写指令                */
    BNE     armDCacheV5FlushAll
#if __SYLIXOS_ARM_ARCH__ < 8
    MOV     R0, #0
    MCR     p15, 0, R0, c7, c10, 4                                      ;/* Drain write-buffer           */
#endif
    BX      LR
    FUNC_END()
    
;/*********************************************************************************************************
;  ARMv5 回写并无效 DCACHE
;*********************************************************************************************************/

FUNC_DEF(armDCacheV5ClearAll)
    MRC     p15, 0, R15, c7, c14, 3                                     ;/*  测试回写并无效指令          */
    BNE     armDCacheV5ClearAll
#if __SYLIXOS_ARM_ARCH__ < 8
    MOV     R0, #0
    MCR     p15, 0, R0, c7, c10, 4                                      ;/* Drain write-buffer           */
#endif
    BX      LR
    FUNC_END()

;/*********************************************************************************************************
;  ARMv5 无效 DCACHE
;*********************************************************************************************************/

FUNC_DEF(armDCacheV5InvalidateAll)
    MOV     R0, #0
    MCR     p15, 0, R0, c7, c6, 0
    BX      LR
    FUNC_END()

    FILE_END()
    
#endif
#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
