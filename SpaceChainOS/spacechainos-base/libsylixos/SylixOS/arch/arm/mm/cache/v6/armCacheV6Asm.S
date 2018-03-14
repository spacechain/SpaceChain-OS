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
;** 文   件   名: armCacheV6Asm.S
;**
;** 创   建   人: Han.Hui (韩辉)
;**
;** 文件创建日期: 2013 年 12 月 09 日
;**
;** 描        述: ARMv6 体系构架 CACHE 驱动.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/kernel/cache_cfg.h>

#if !defined(__SYLIXOS_ARM_ARCH_M__)

#if LW_CFG_CACHE_EN > 0

    FILE_BEGIN()
    
    EXPORT_LABEL(armDCacheV6Disable)
    EXPORT_LABEL(armDCacheV6FlushAll)
    EXPORT_LABEL(armDCacheV6ClearAll)
    EXPORT_LABEL(armDCacheV6InvalidateAll)
    
;/*********************************************************************************************************
;  ARMv6 关闭 DCACHE
;*********************************************************************************************************/

FUNC_DEF(armDCacheV6Disable)
    MRC     p15, 0, R0, c1, c0, 0
    AND     R0, R0, #P15_R1_C
    CMP     R0, #0
    BEQ     dcache_not_en

    STMFD   SP!, {LR}
    BL      armDCacheV6ClearAll
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
    BL      armDCacheV6InvalidateAll
    LDMFD   SP!, {LR}
    BX      LR
    FUNC_END()
    
;/*********************************************************************************************************
;  ARMv6 回写 DCACHE
;*********************************************************************************************************/

FUNC_DEF(armDCacheV6FlushAll)
    MOV     R0, #0
    MCR     p15, 0, R0, c7, c10, 0
#if __SYLIXOS_ARM_ARCH__ < 8
    MCR     p15, 0, R0, c7, c10, 4                                      ;/* Drain write-buffer           */
#endif
    BX      LR
    FUNC_END()
    
;/*********************************************************************************************************
;  ARMv6 回写并无效 DCACHE
;*********************************************************************************************************/

FUNC_DEF(armDCacheV6ClearAll)
    MOV     R0, #0
    MCR     p15, 0, R0, c7, c14, 0
#if __SYLIXOS_ARM_ARCH__ < 8
    MCR     p15, 0, R0, c7, c10, 4                                      ;/* Drain write-buffer           */
#endif
    BX      LR
    FUNC_END()

;/*********************************************************************************************************
;  ARMv6 无效 DCACHE
;*********************************************************************************************************/

FUNC_DEF(armDCacheV6InvalidateAll)
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
