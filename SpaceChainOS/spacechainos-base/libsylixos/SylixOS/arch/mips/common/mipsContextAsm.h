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
;** 文   件   名: mipsContextAsm.h
;**
;** 创   建   人: Ryan.Xin (信金龙)
;**
;** 文件创建日期: 2015 年 09 月 01 日
;**
;** 描        述: MIPS 体系构架上下文处理.
;*********************************************************************************************************/

#ifndef __MIPSCONTEXTASM_H
#define __MIPSCONTEXTASM_H

#include "arch/mips/arch_regs.h"

;/*********************************************************************************************************
;  保存 $0 - $31 寄存器
;*********************************************************************************************************/

MACRO_DEF(SAVE_GREGS)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile

    SW      $0  , 0  * 4(SP)
    SW      $1  , 1  * 4(SP)
    SW      $2  , 2  * 4(SP)
    SW      $3  , 3  * 4(SP)
    SW      $4  , 4  * 4(SP)
    SW      $5  , 5  * 4(SP)
    SW      $6  , 6  * 4(SP)
    SW      $7  , 7  * 4(SP)
    SW      $8  , 8  * 4(SP)
    SW      $9  , 9  * 4(SP)
    SW      $10 , 10 * 4(SP)
    SW      $11 , 11 * 4(SP)
    SW      $12 , 12 * 4(SP)
    SW      $13 , 13 * 4(SP)
    SW      $14 , 14 * 4(SP)
    SW      $15 , 15 * 4(SP)
    SW      $16 , 16 * 4(SP)
    SW      $17 , 17 * 4(SP)
    SW      $18 , 18 * 4(SP)
    SW      $19 , 19 * 4(SP)
    SW      $20 , 20 * 4(SP)
    SW      $21 , 21 * 4(SP)
    SW      $22 , 22 * 4(SP)
    SW      $23 , 23 * 4(SP)
    SW      $24 , 24 * 4(SP)
    SW      $25 , 25 * 4(SP)
    ;/*
    ; * $26 $27 是 K0 K1
    ; */
    SW      $28 , 28 * 4(SP)
    SW      $29 , 29 * 4(SP)
    SW      $30 , 30 * 4(SP)
    SW      $31 , 31 * 4(SP)

    .set    pop
    MACRO_END()

;/*********************************************************************************************************
;  恢复 $0 - $31 寄存器
;*********************************************************************************************************/

MACRO_DEF(RESTORE_GREGS)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile

    ;/*
    ; * $0 固定为 0
    ; */
    LW      $1  , 1  * 4(SP)
    LW      $2  , 2  * 4(SP)
    LW      $3  , 3  * 4(SP)
    LW      $4  , 4  * 4(SP)
    LW      $5  , 5  * 4(SP)
    LW      $6  , 6  * 4(SP)
    LW      $7  , 7  * 4(SP)
    LW      $8  , 8  * 4(SP)
    ;/*
    ; * $9 是 T1(后面会恢复)
    ; */
    LW      $10 , 10 * 4(SP)
    LW      $11 , 11 * 4(SP)
    LW      $12 , 12 * 4(SP)
    LW      $13 , 13 * 4(SP)
    LW      $14 , 14 * 4(SP)
    LW      $15 , 15 * 4(SP)
    LW      $16 , 16 * 4(SP)
    LW      $17 , 17 * 4(SP)
    LW      $18 , 18 * 4(SP)
    LW      $19 , 19 * 4(SP)
    LW      $20 , 20 * 4(SP)
    LW      $21 , 21 * 4(SP)
    LW      $22 , 22 * 4(SP)
    LW      $23 , 23 * 4(SP)
    LW      $24 , 24 * 4(SP)
    LW      $25 , 25 * 4(SP)
    ;/*
    ; * $26 $27 是 K0 K1
    ; */
    LW      $28 , 28 * 4(SP)
    ;/*
    ; * $29 是 SP
    ; */
    LW      $30 , 30 * 4(SP)
    LW      $31 , 31 * 4(SP)

    .set    pop
    MACRO_END()

;/*********************************************************************************************************
;  保存上下文
;*********************************************************************************************************/

MACRO_DEF(SAVE_REGS)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile

    SUBU    SP , STK_CTX_SIZE                                           ;/*  修改 SP                     */

    SW      ZERO , 0(SP)
    SW      ZERO , STK_OFFSET_LAST(SP)
    SYNC
    ;/*
    ; * 后面不会再发生 TLB 重填异常
    ; */

    SAVE_GREGS                                                          ;/*  保存 $0 - $31 寄存器        */

    SW      RA , STK_OFFSET_EPC(SP)                                     ;/*  RA 代替 EPC 保存            */

    MFC0(T1, CP0_STATUS)                                                ;/*  保存 STATUS 寄存器          */
    SW      T1 , STK_OFFSET_SR(SP)

    MFC0(T1, CP0_BADVADDR)                                              ;/*  保存 BADVADDR 寄存器        */
    SW      T1 , STK_OFFSET_BADVADDR(SP)

    MFC0(T1, CP0_CAUSE)                                                 ;/*  保存 CAUSE 寄存器           */
    SW      T1 , STK_OFFSET_CAUSE(SP)

    MFLO    T1                                                          ;/*  保存 LO 寄存器              */
    SW      T1 , STK_OFFSET_LO(SP)

    MFHI    T1                                                          ;/*  保存 HI 寄存器              */
    SW      T1 , STK_OFFSET_HI(SP)

    .set    pop
    MACRO_END()

;/*********************************************************************************************************
;  恢复上下文
;*********************************************************************************************************/

MACRO_DEF(RESTORE_REGS)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile

    LW      ZERO , 0(SP)
    LW      ZERO , STK_OFFSET_LAST(SP)
    SYNC
    ;/*
    ; * 后面不会再发生 TLB 重填异常
    ; */

    RESTORE_GREGS                                                       ;/*  恢复 $0 - $31 寄存器        */

    LW      T1 , STK_OFFSET_LO(SP)                                      ;/*  恢复 LO 寄存器              */
    MTLO    T1

    LW      T1 , STK_OFFSET_HI(SP)                                      ;/*  恢复 HI 寄存器              */
    MTHI    T1

    LW      T1 , STK_OFFSET_CAUSE(SP)                                   ;/*  恢复 CAUSE 寄存器           */
    MTC0(T1, CP0_CAUSE)

    LW      T1 , STK_OFFSET_BADVADDR(SP)                                ;/*  恢复 BADVADDR 寄存器        */
    MTC0(T1, CP0_BADVADDR)

    LW      T1 , REG_T1 * 4(SP)                                         ;/*  恢复 T1 寄存器              */

    LW      K0 , STK_OFFSET_SR(SP)                                      ;/*  恢复 SR  寄存器             */
    LW      K1 , STK_OFFSET_EPC(SP)                                     ;/*  恢复 EPC 寄存器             */

    ADDU    SP , STK_CTX_SIZE                                           ;/*  修改 SP                     */

    ORI     K0 , K0 , M_StatusEXL                                       ;/*  通过设置 EXL 位             */

    MTC0    K1 , CP0_EPC                                                ;/*  恢复 EPC 寄存器             */
    MTC0    K0 , CP0_STATUS                                             ;/*  进入内核模式，并关中断      */
    EHB
    ERET                                                                ;/*  清除 EXL 位并返回           */
    NOP

    .set    pop
    MACRO_END()

#endif                                                                  /*  __MIPSCONTEXTASM_H          */
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
