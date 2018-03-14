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
;** 文   件   名: mipsLibAsm.S
;**
;** 创   建   人: Ryan.Xin (信金龙)
;**
;** 文件创建日期: 2015 年 09 月 01 日
;**
;** 描        述: MIPS 体系构架内部库.
;**
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/cpu/cpu_cfg.h>
#include "mipsContextAsm.h"

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

#if LW_CFG_MIPS_HAS_CLZ_INSTR > 0
    EXPORT_LABEL(archFindLsb)
    EXPORT_LABEL(archFindMsb)
#endif

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
;  MIPS 关闭总中断
;*********************************************************************************************************/

FUNC_DEF(archIntDisable)
    .set    push
    .set    noreorder
    .set    volatile

    MFC0(V0, CP0_STATUS)
    AND     T0 , V0 , ~M_StatusIE
    MTC0(T0, CP0_STATUS)
    JR      RA
    NOP

    .set    pop
    FUNC_END(archIntDisable)

;/*********************************************************************************************************
;  MIPS 打开总中断
;*********************************************************************************************************/

FUNC_DEF(archIntEnable)
    .set    push
    .set    noreorder
    .set    volatile

    ANDI    A0 , A0 , M_StatusIE
    BEQ     A0 , ZERO , 1f
	NOP
	MFC0(T0, CP0_STATUS)
    ORI     T1 , T0 , M_StatusIE
    MTC0(T1, CP0_STATUS)
1:
    JR      RA
    NOP

    .set    pop
    FUNC_END(archIntEnable)

;/*********************************************************************************************************
;  MIPS 强制打开总中断
;*********************************************************************************************************/

FUNC_DEF(archIntEnableForce)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile

    MFC0(T0, CP0_STATUS)
    ORI     T1 , T0 , M_StatusIE
    MTC0(T1, CP0_STATUS)
    JR      RA
    NOP

    .set    pop
    FUNC_END(archIntEnableForce)

;/*********************************************************************************************************
;  MIPS 前导零运算
;*********************************************************************************************************/

#if LW_CFG_MIPS_HAS_CLZ_INSTR > 0
FUNC_DEF(archFindLsb)
    NEGU    V0 , A0
    AND     A0 , V0 , A0
    CLZ     A0 , A0
    LI      V0 , 32
    JR      RA
    SUBU    V0 , V0 , A0
    FUNC_END(archFindLsb)

FUNC_DEF(archFindMsb)
    CLZ     A0 , A0
    LI      V0 , 32
    JR      RA
    SUBU    V0 , V0 , A0
    FUNC_END(archFindMsb)
#endif

;/*********************************************************************************************************
;  注意: setjmp 与 longjmp 上下文结构与线程上下文结构相同
;*********************************************************************************************************/

;/*********************************************************************************************************
;  调用设置函数宏
;*********************************************************************************************************/

MACRO_DEF(CALL_SETUP, setup)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile

    SUBU    SP , 32
    SW      RA , 28(SP)
    SW      FP , 24(SP)
    MOV     FP , SP
    SW      A0 , 20(SP)
    SW      A1 , 16(SP)

    JAL     \setup
    NOP

    MOV     SP , FP
    LW      RA , 28(SP)
    LW      FP , 24(SP)
    LW      A0 , 20(SP)
    LW      A1 , 16(SP)
    ADDU    SP , 32

    .set    pop
    MACRO_END()

;/*********************************************************************************************************
;  保存寄存器宏
;*********************************************************************************************************/

MACRO_DEF(SAVE_NON_VOLATILE_REG)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile

    SW      ZERO , 0(A0)
    SW      ZERO , STK_OFFSET_LAST(A0)
    SYNC
    ;/*
    ; * 后面不会再发生 TLB 重填异常
    ; */

    SW      $0  , 0  * 4(A0)
    SW      $1  , 1  * 4(A0)
    SW      $2  , 2  * 4(A0)
    SW      $3  , 3  * 4(A0)
    SW      $4  , 4  * 4(A0)
    SW      $5  , 5  * 4(A0)
    SW      $6  , 6  * 4(A0)
    SW      $7  , 7  * 4(A0)
    SW      $8  , 8  * 4(A0)
    SW      $9  , 9  * 4(A0)
    SW      $10 , 10 * 4(A0)
    SW      $11 , 11 * 4(A0)
    SW      $12 , 12 * 4(A0)
    SW      $13 , 13 * 4(A0)
    SW      $14 , 14 * 4(A0)
    SW      $15 , 15 * 4(A0)
    SW      $16 , 16 * 4(A0)
    SW      $17 , 17 * 4(A0)
    SW      $18 , 18 * 4(A0)
    SW      $19 , 19 * 4(A0)
    SW      $20 , 20 * 4(A0)
    SW      $21 , 21 * 4(A0)
    SW      $22 , 22 * 4(A0)
    SW      $23 , 23 * 4(A0)
    SW      $24 , 24 * 4(A0)
    SW      $25 , 25 * 4(A0)
    ;/*
    ; * $26 $27 是 K0 K1
    ; */
    SW      $28 , 28 * 4(A0)
    SW      $29 , 29 * 4(A0)
    SW      $30 , 30 * 4(A0)
    SW      $31 , 31 * 4(A0)

    MFC0(T1, CP0_STATUS)                                                ;/*  保存 STATUS 寄存器          */
    SW      T1 , STK_OFFSET_SR(A0)

    SW      RA , STK_OFFSET_EPC(A0)                                     ;/*  RA 代替 EPC 保存            */

    MFC0(T1, CP0_BADVADDR)                                              ;/*  保存 BADVADDR 寄存器        */
    SW      T1 , STK_OFFSET_BADVADDR(A0)

    MFC0(T1, CP0_CAUSE)                                                 ;/*  保存 CAUSE 寄存器           */
    SW      T1 , STK_OFFSET_CAUSE(A0)

    MFLO    T1                                                          ;/*  保存 LO 寄存器              */
    SW      T1 , STK_OFFSET_LO(A0)

    MFHI    T1                                                          ;/*  保存 HI 寄存器              */
    SW      T1 , STK_OFFSET_HI(A0)

    .set    pop
    MACRO_END()

;/*********************************************************************************************************
;  恢复寄存器宏
;*********************************************************************************************************/

MACRO_DEF(RESTORE_NON_VOLATILE_REG)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile

    LW      ZERO , 0(A0)
    LW      ZERO , STK_OFFSET_LAST(A0)
    SYNC
    ;/*
    ; * 后面不会再发生 TLB 重填异常
    ; */

    ;/*
    ; * $0 固定为 0
    ; */
    LW      $1  , 1  * 4(A0)
    ;/*
    ; * $2 是 V0(返回值寄存器)
    ; */
    LW      $3  , 3  * 4(A0)
    ;/*
    ; * $4 $5 是 A0 A1(两个参数寄存器)
    ; */
    LW      $6  , 6  * 4(A0)
    LW      $7  , 7  * 4(A0)
    LW      $8  , 8  * 4(A0)
    ;/*
    ; * $9 是 T1(后面会恢复)
    ; */
    LW      $10 , 10 * 4(A0)
    LW      $11 , 11 * 4(A0)
    LW      $12 , 12 * 4(A0)
    LW      $13 , 13 * 4(A0)
    LW      $14 , 14 * 4(A0)
    LW      $15 , 15 * 4(A0)
    LW      $16 , 16 * 4(A0)
    LW      $17 , 17 * 4(A0)
    LW      $18 , 18 * 4(A0)
    LW      $19 , 19 * 4(A0)
    LW      $20 , 20 * 4(A0)
    LW      $21 , 21 * 4(A0)
    LW      $22 , 22 * 4(A0)
    LW      $23 , 23 * 4(A0)
    LW      $24 , 24 * 4(A0)
    LW      $25 , 25 * 4(A0)
    ;/*
    ; * $26 $27 是 K0 K1
    ; */
    LW      $28 , 28 * 4(A0)
    LW      $29 , 29 * 4(A0)
    LW      $30 , 30 * 4(A0)
    LW      $31 , 31 * 4(A0)

    LW      T1 , STK_OFFSET_CAUSE(A0)                                   ;/*  恢复 CAUSE 寄存器           */
    MTC0(T1, CP0_CAUSE)

    LW      T1 , STK_OFFSET_BADVADDR(A0)                                ;/*  恢复 BADVADDR 寄存器        */
    MTC0(T1, CP0_BADVADDR)

    LW      T1 , STK_OFFSET_EPC(A0)                                     ;/*  恢复 EPC 寄存器             */
    MTC0(T1, CP0_EPC)

    LW      T1 , STK_OFFSET_LO(A0)                                      ;/*  恢复 LO 寄存器              */
    MTLO    T1

    LW      T1 , STK_OFFSET_HI(A0)                                      ;/*  恢复 HI 寄存器              */
    MTHI    T1

    LW      T1 , STK_OFFSET_SR(A0)                                      ;/*  恢复 SR  寄存器             */
    MTC0(T1, CP0_STATUS)

    LW      T1 , REG_T1 * 4(A0)                                         ;/*  恢复 T1 寄存器              */

    .set    pop
    MACRO_END()

;/*********************************************************************************************************
;  sigsetjmp (参数为 jmp_buf, mask_saved)
;*********************************************************************************************************/

FUNC_DEF(sigsetjmp)
    CALL_SETUP      __sigsetjmpSetup

    SAVE_NON_VOLATILE_REG

    JR      RA
    MOV     V0 , ZERO
    FUNC_END(sigsetjmp)

;/*********************************************************************************************************
;  setjmp (参数为 jmp_buf)
;*********************************************************************************************************/

FUNC_DEF(setjmp)
    CALL_SETUP      __setjmpSetup

    SAVE_NON_VOLATILE_REG

    JR      RA
    MOV     V0 , ZERO
    FUNC_END(setjmp)

;/*********************************************************************************************************
;  siglongjmp (参数为 jmp_buf, retval)
;*********************************************************************************************************/

FUNC_DEF(siglongjmp)
    CALL_SETUP      __siglongjmpSetup

    RESTORE_NON_VOLATILE_REG

    MOV     V0 , A1
    BNE     V0 , ZERO , siglongjmpnotzero
    NOP
    LI      V0 , 1
LINE_LABEL(siglongjmpnotzero)
    JR      RA
    NOP
    FUNC_END(siglongjmp)

;/*********************************************************************************************************
;  longjmp (参数为 jmp_buf, retval)
;*********************************************************************************************************/

FUNC_DEF(longjmp)
    CALL_SETUP      __longjmpSetup

    RESTORE_NON_VOLATILE_REG

    MOV     V0 , A1
    BNE     V0 , ZERO , longjmpnotzero
    NOP
    LI      V0 , 1
LINE_LABEL(longjmpnotzero)
    JR      RA
    NOP
    FUNC_END(longjmp)

;/*********************************************************************************************************
;  系统重启
;*********************************************************************************************************/

FUNC_DEF(archReboot)
    J       bspReboot                                                   ;/*  由BSP进行复位操作, 不再返回 */
    NOP
    FUNC_END(archReboot)

;/*********************************************************************************************************
;  Bogo 循环
;*********************************************************************************************************/

FUNC_DEF(archBogoMipsLoop)
1:
    BNE     A0 , ZERO , 1b
    SUBU    A0 , 1
    JR      RA
    NOP
    FUNC_END(archBogoMipsLoop)

    FILE_END()

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
