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
;** 文   件   名: armMpCoreAsm.S
;**
;** 创   建   人: Han.Hui (韩辉)
;**
;** 文件创建日期: 2013 年 12 月 09 日
;**
;** 描        述: ARM 体系构架多核接口驱动.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>

#if LW_CFG_SMP_EN > 0

    FILE_BEGIN()
    
    IMPORT_LABEL(bspMpInt)
    
    EXPORT_LABEL(armSpinLock)
    EXPORT_LABEL(armSpinTryLock)
    EXPORT_LABEL(armSpinUnlock)
    EXPORT_LABEL(archMpCur)
    EXPORT_LABEL(archMpInt)
    
    WEAK(armSpinLock)
    WEAK(armSpinTryLock)
    WEAK(armSpinUnlock)
    WEAK(archMpCur)
    WEAK(archMpInt)

;/*********************************************************************************************************
;  自旋锁 ARMv6/v7
;*********************************************************************************************************/

#if __SYLIXOS_ARM_ARCH__ >= 6
FUNC_DEF(armSpinLock)
    MOV     R1, #1
1:
    LDREX   R2, [R0]
    CMP     R2, #0

#if (__SYLIXOS_ARM_ARCH__ >= 7) && (ARM_SPINLOCK_EVENT > 0)
    WFENE
#endif

    STREXEQ R2, R1, [R0]
    CMPEQ   R2, #0
    BNE     1b
    ARM_DMB()

    BX      LR
    FUNC_END()

FUNC_DEF(armSpinTryLock)
    MOV     R1, #1
    LDREX   R2, [R0]
    CMP     R2, #0
    STREXEQ R2, R1, [R0]
    MOV     R0, R2
    ARM_DMB()

    BX      LR
    FUNC_END()

FUNC_DEF(armSpinUnlock)
    MOV     R1, #0
    STR     R1, [R0]
    ARM_DSB()

#if (__SYLIXOS_ARM_ARCH__ >= 7) && (ARM_SPINLOCK_EVENT > 0)
    SEV
#endif

    BX      LR
    FUNC_END()

;/*********************************************************************************************************
;  自旋锁 ARMv4/v5
;*********************************************************************************************************/

#else
FUNC_DEF(armSpinLock)
    MOV     R1, #1
1:
    LDR     R2, [R0]
    CMP     R2, #0
    SWPEQ   R2, R1, [R0]
    CMPEQ   R2, #0
    BNE     1b
    BX      LR
    FUNC_END()

FUNC_DEF(armSpinTryLock)
    MOV     R1, #1
    LDR     R2, [R0]
    CMP     R2, #0
    SWPEQ   R2, R1, [R0]
    MOV     R0, R2
    BX      LR
    FUNC_END()
    
FUNC_DEF(armSpinUnlock)
    MOV     R1, #0
    STR     R1, [R0]
    ARM_DSB()
    BX      LR
    FUNC_END()
#endif

;/*********************************************************************************************************
;  获得当前核 ID ARMv6/v7
;*********************************************************************************************************/

#if __SYLIXOS_ARM_ARCH__ >= 6
FUNC_DEF(archMpCur)
    MRC     p15, 0, R0, c0, c0, 5
    AND     R0, R0, #0x03
    BX      LR
    FUNC_END()

;/*********************************************************************************************************
;  获得当前核 ID ARMv4/v5
;*********************************************************************************************************/

#else
FUNC_DEF(archMpCur)
    MOV     R0, #0
    BX      LR
    FUNC_END()
#endif
    
;/*********************************************************************************************************
;  产生一次核间中断
;*********************************************************************************************************/

FUNC_DEF(archMpInt)
    B       bspMpInt
    FUNC_END()
    
    FILE_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
