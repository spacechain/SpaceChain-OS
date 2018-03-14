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
;** 文   件   名: armVfpV7MAsm.S
;**
;** 创   建   人: Han.Hui (韩辉)
;**
;** 文件创建日期: 2017 年 11 月 08 日
;**
;** 描        述: ARM 体系架构 Cortex-Mx VFPv4/5 支持.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>

#if defined(__SYLIXOS_ARM_ARCH_M__)

#if LW_CFG_CPU_FPU_EN > 0

    FILE_BEGIN()

    IMPORT_LABEL(armVfpV7MEnable)

    EXPORT_LABEL(armVfpV7MSave)
    EXPORT_LABEL(armVfpV7MRestore)

;/*********************************************************************************************************
;  Cortex-Mx VFPv4/5 保存/恢复寄存器
;*********************************************************************************************************/

#define VFP_FPSCR   0x00
#define VFP_GPR     0x08

FUNC_DEF(armVfpV7MSave)
    STMFD   SP!, {R0, LR}
    LDR     R1 , =armVfpV7MEnable
    BLX     R1
    LDMFD   SP!, {R0, LR}

    VMRS    R1 , FPSCR
    STR     R1 , [R0, #VFP_FPSCR]
    ADD     R1 , R0, #VFP_GPR
    VSTM    R1!, {d0-d15}
    BX      LR
    FUNC_END()

FUNC_DEF(armVfpV7MRestore)
    STMFD   SP!, {R0, LR}
    LDR     R1 , =armVfpV7MEnable
    BLX     R1
    LDMFD   SP!, {R0, LR}

    LDR     R1 , [R0, #VFP_FPSCR]
    VMSR    FPSCR , R1
    ADD     R1 , R0, #VFP_GPR
    VLDM    R1!, {d0-d15}
    BX      LR
    FUNC_END()

    FILE_END()

#endif
#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
