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
;** 文   件   名: armMpuV7RAsm.S
;**
;** 创   建   人: Han.Hui (韩辉)
;**
;** 文件创建日期: 2017 年 11 月 08 日
;**
;** 描        述: ARM 体系构架 MPU 驱动.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>

#if defined(__SYLIXOS_ARM_ARCH_R__)

#if (LW_CFG_VMM_EN == 0) && (LW_CFG_ARM_MPU > 0)

    FILE_BEGIN()

    EXPORT_LABEL(armMpuV7REnable)
    EXPORT_LABEL(armMpuV7RDisable)
    EXPORT_LABEL(armMpuV7RBgEnable)
    EXPORT_LABEL(armMpuV7RBgDisable)
    EXPORT_LABEL(armMpuV7RGetRegionNum)
    EXPORT_LABEL(armMpuV7RGetRegionNumIns)
    EXPORT_LABEL(armMpuV7RAreRegionsSeparate)
    EXPORT_LABEL(armMpuV7RSelectRegion)
    EXPORT_LABEL(armMpuV7RSetRegionBase)
    EXPORT_LABEL(armMpuV7RSetRegionAP)
    EXPORT_LABEL(armMpuV7RSetRegionSize)
    EXPORT_LABEL(armMpuV7RSetRegionBaseIns)
    EXPORT_LABEL(armMpuV7RSetRegionAPIns)
    EXPORT_LABEL(armMpuV7RSetRegionSizeIns)

;/*********************************************************************************************************
;  MPU 操作函数函数
;*********************************************************************************************************/

FUNC_DEF(armMpuV7REnable)
    MRC     p15, #0, R0, c1, c0, #0
    ORR     R0,  R0, #1
    ARM_DSB()
    MCR     p15, #0, R0, c1, c0, #0
    ARM_ISB()
    BX      LR
    FUNC_END()

FUNC_DEF(armMpuV7RDisable)
    MRC     p15, #0, R0, c1, c0, #0
    BIC     R0,  R0, #1
    ARM_DSB()
    MCR     p15, #0, R0, c1, c0, #0
    ARM_ISB()
    BX      LR
    FUNC_END()

FUNC_DEF(armMpuV7RBgEnable)
    MRC     p15, #0, R0, c1, c0, #0
    ORR     R0,  R0, #0x20000
    MCR     p15, #0, R0, c1, c0, #0
    BX      LR
    FUNC_END()

FUNC_DEF(armMpuV7RBgDisable)
    MRC     p15, #0, R0, c1, c0, #0
    BIC     R0,  R0, #0x20000
    MCR     p15, #0, R0, c1, c0, #0
    BX      LR
    FUNC_END()

FUNC_DEF(armMpuV7RGetRegionNum)
    MRC     p15, #0, R0, c0, c0, #4
    UXTB    R0,  R0, ROR #8
    BX      LR
    FUNC_END()

FUNC_DEF(armMpuV7RGetRegionNumIns)
    MRC     p15, #0, R0, c0, c0, #4
    UXTB    R0,  R0, ROR #16
    BX      LR
    FUNC_END()

FUNC_DEF(armMpuV7RAreRegionsSeparate)
    MRC     p15, #0, R0, c0, c0, #4
    UXTB    R0,  R0
    BX      LR
    FUNC_END()

FUNC_DEF(armMpuV7RSelectRegion)
    MCR     p15, #0, R0, c6, c2, #0
    BX      LR
    FUNC_END()

FUNC_DEF(armMpuV7RSetRegionBase)
    MCR     p15, #0, R0, c6, c1, #0
    BX      LR
    FUNC_END()

FUNC_DEF(armMpuV7RSetRegionAP)
    MCR     p15, #0, R0, c6, c1, #4
    BX      LR
    FUNC_END()

FUNC_DEF(armMpuV7RSetRegionSize)
    MCR     p15, #0, R0, c6, c1, #2
    BX      LR
    FUNC_END()

FUNC_DEF(armMpuV7RSetRegionBaseIns)
    MCR     p15, #0, R0, c6, c1, #1
    BX      LR
    FUNC_END()

FUNC_DEF(armMpuV7RSetRegionAPIns)
    MCR     p15, #0, R0, c6, c1, #5
    BX      LR
    FUNC_END()

FUNC_DEF(armMpuV7RSetRegionSizeIns)
    MCR     p15, #0, R0, c6, c1, #3
    BX      LR
    FUNC_END()

    FILE_END()

#endif
#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
