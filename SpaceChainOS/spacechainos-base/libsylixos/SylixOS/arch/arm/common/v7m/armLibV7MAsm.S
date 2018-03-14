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
;** 文   件   名: armLibV7MAsm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2017 年 11 月 14 日
;**
;** 描        述: ARMv7M 体系构架内部库.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>

#if defined(__SYLIXOS_ARM_ARCH_M__)

    FILE_BEGIN()

    IMPORT_LABEL(__setjmpSetup)
    IMPORT_LABEL(__sigsetjmpSetup)
    IMPORT_LABEL(__longjmpSetup)
    IMPORT_LABEL(__siglongjmpSetup)
    IMPORT_LABEL(bspReboot)

    EXPORT_LABEL(setjmp)
    EXPORT_LABEL(sigsetjmp)
    EXPORT_LABEL(longjmp)
    EXPORT_LABEL(siglongjmp)

    EXPORT_LABEL(archFindLsb)
    EXPORT_LABEL(archFindMsb)

    EXPORT_LABEL(archIntDisable)
    EXPORT_LABEL(archIntEnable)
    EXPORT_LABEL(archIntEnableForce)
    EXPORT_LABEL(archPageCopy)
    EXPORT_LABEL(archReboot)
    EXPORT_LABEL(archBogoMipsLoop)

    WEAK(archIntDisable)
    WEAK(archIntEnable)
    WEAK(archIntEnableForce)

;/*********************************************************************************************************
;  ARM 关闭总中断
;*********************************************************************************************************/

FUNC_DEF(archIntDisable)
    MRS     R0 , BASEPRI                ;/*  Set prio int mask to mask all (except faults)               */
    MOV     R1 , #LW_CFG_CORTEX_M_INT_MASK
    MSR     BASEPRI,  R1
    BX      LR
    FUNC_END()

FUNC_DEF(archIntEnable)
    MSR     BASEPRI, R0
    BX      LR
    FUNC_END()

FUNC_DEF(archIntEnableForce)
    CPSIE   I                            /*  Enable interrupts at processor level                        */
    MOV     R1 , #0
    MSR     BASEPRI, R1
    BX      LR
    FUNC_END()

;/*********************************************************************************************************
;  ARM 前导零运算
;*********************************************************************************************************/

FUNC_DEF(archFindLsb)
    RSB     R1 , R0, #0
    ANDS    R0 , R1, R0
    ITT     NE
    CLZNE   R0 , R0
    RSBNE   R0 , R0, #32
    BX      LR
    FUNC_END()

FUNC_DEF(archFindMsb)
    CMP     R0 , #0
    ITT     NE
    CLZNE   R0 , R0
    RSBNE   R0 , R0, #32
    BX      LR
    FUNC_END()

;/*********************************************************************************************************
;  注意: setjmp 与 longjmp 上下文结构与线程上下文结构不同
;
;  +---------------+
;  |    R4 - R7    | (4 regs)
;  +---------------+
;  |    R8 - R10   | (3 regs)
;  +---------------+
;  |    FP(R11)    | (1 regs)
;  +---------------+
;  |    SP(R13)    | (1 regs)
;  +---------------+
;  |    LR(R14)    | (1 regs)
;  +---------------+
;*********************************************************************************************************/

;/*********************************************************************************************************
;  sigsetjmp (参数为 jmp_buf, mask_saved)
;*********************************************************************************************************/

FUNC_DEF(sigsetjmp)
    STMFD   SP!, {R0, R1, LR}
    BL      __sigsetjmpSetup
    LDMFD   SP!, {R0, R1, LR}

    STMIA   R0!, {R4, R5, R6, R7}                                       ;/* Save registers in jump buffer*/
    MOV     R1 , R8
    MOV     R2 , R9
    MOV     R3 , R10
    MOV     R4 , FP
    MOV     R5 , SP
    MOV     R6 , LR
    STMIA   R0!, {R1, R2, R3, R4, R5, R6}

    SUB     R0 , R0 , #40
    LDMIA   R0!, {R4, R5, R6, R7}                                       ;/* Restore callee-saved low regs*/

    MOV     R0 , #0                                                     ;/* Return zero.                 */
    BX      LR
    FUNC_END()

;/*********************************************************************************************************
;  setjmp (参数为 jmp_buf)
;*********************************************************************************************************/

FUNC_DEF(setjmp)
    STMFD   SP!, {R0, LR}
    BL      __setjmpSetup
    LDMFD   SP!, {R0, LR}

    STMIA   R0!, {R4, R5, R6, R7}                                       ;/* Save registers in jump buffer*/
    MOV     R1 , R8
    MOV     R2 , R9
    MOV     R3 , R10
    MOV     R4 , FP
    MOV     R5 , SP
    MOV     R6 , LR
    STMIA   R0!, {R1, R2, R3, R4, R5, R6}

    SUB     R0 , R0 , #40
    LDMIA   R0!, {R4, R5, R6, R7}                                       ;/* Restore callee-saved low regs*/

    MOV     R0 , #0                                                     ;/* Return zero.                 */
    BX      LR
    FUNC_END()

;/*********************************************************************************************************
;  siglongjmp (参数为 jmp_buf, retval)
;*********************************************************************************************************/

FUNC_DEF(siglongjmp)
    STMFD   SP!, {R0, R1, LR}
    BL      __siglongjmpSetup
    LDMFD   SP!, {R0, R1, LR}

    ADD     R0 , R0 , #16
    LDMIA   R0!, {R2, R3, R4, R5, R6}                                   ;/* Restore High regs.           */
    MOV     R8 , R2
    MOV     R9 , R3
    MOV     R10, R4
    MOV     FP , R5
    MOV     SP , R6

    LDMIA   R0!, {R3}                                                   ;/* lr                           */

    SUB     R0 , R0 , #40                                               ;/* Restore low regs.            */
    LDMIA   R0!, {R4, R5, R6, R7}
                                                                        ;/* Return the result argument,  */
                                                                        ;/* or 1 if it is zero.          */
    MOV     R0 , R1
    BNE     siglongjmpnotzero
    MOV     R0 , #1

LINE_LABEL(siglongjmpnotzero)
    BX      R3
    FUNC_END()

;/*********************************************************************************************************
;  longjmp (参数为 jmp_buf, retval)
;*********************************************************************************************************/

FUNC_DEF(longjmp)
    STMFD   SP!, {R0, R1, LR}
    BL      __longjmpSetup
    LDMFD   SP!, {R0, R1, LR}

    ADD     R0 , R0 , #16
    LDMIA   R0!, {R2, R3, R4, R5, R6}                                   ;/* Restore High regs.           */
    MOV     R8 , R2
    MOV     R9 , R3
    MOV     R10, R4
    MOV     FP , R5
    MOV     SP , R6

    LDMIA   R0!, {R3}                                                   ;/* lr                           */

    SUB     R0 , R0 , #40                                               ;/* Restore low regs.            */
    LDMIA   R0!, {R4, R5, R6, R7}
                                                                        ;/* Return the result argument,  */
                                                                        ;/* or 1 if it is zero.          */
    MOV     R0 , R1
    BNE     longjmpnotzero
    MOV     R0 , #1

LINE_LABEL(longjmpnotzero)
    BX      R3
    FUNC_END()

;/*********************************************************************************************************
;  系统重启
;*********************************************************************************************************/

FUNC_DEF(archReboot)
    B       bspReboot                                                   ;/* 不再返回, 由 BSP 进行复位操作*/
    FUNC_END()

;/*********************************************************************************************************
;  Bogo 循环
;*********************************************************************************************************/
FUNC_DEF(archBogoMipsLoop)
1:
    SUBS    R0 , R0 , #1
    BNE     1b
    BX      LR
    FUNC_END()

    FILE_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
