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
;** 文   件   名: mipsExcAsm.S
;**
;** 创   建   人: Ryan.Xin (信金龙)
;**
;** 文件创建日期: 2015 年 09 月 01 日
;**
;** 描        述: MIPS 体系构架异常处理.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include "mipsContextAsm.h"

    FILE_BEGIN()

    IMPORT_LABEL(API_ThreadTcbInter)
    IMPORT_LABEL(API_InterStackBaseGet)
    IMPORT_LABEL(API_InterEnter)
    IMPORT_LABEL(API_InterExit)

    IMPORT_LABEL(bspIntHandle)
    IMPORT_LABEL(archCacheErrorHandle)
    IMPORT_LABEL(archExceptionHandle)

    EXPORT_LABEL(archInterruptEntry)
    EXPORT_LABEL(archCacheErrorEntry)
    EXPORT_LABEL(archExceptionEntry)

;/*********************************************************************************************************
;  用于临时保存异常上下文
;*********************************************************************************************************/

#define TEMP_CTX_OFFSET_LOCK        0
#define TEMP_CTX_OFFSET_SR          4
#define TEMP_CTX_OFFSET_EPC         8
#define TEMP_CTX_OFFSET_CAUSE       12
#define TEMP_CTX_OFFSET_BADVADDR    16

    SECTION(.bss)
    .align (4)
LINE_LABEL(archExcTempCtx)
    .space    32

;/*********************************************************************************************************
;  保存异常上下文
;*********************************************************************************************************/

MACRO_DEF(EXC_SAVE_REGS)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile

    ;/*
    ; * 因为这些都是内核代码，不经 TLB 转换，所以执行时不会发生 TLB 重填异常
    ; */
;/*********************************************************************************************************
;  1. 对 archExcTempCtx 进行加锁
;*********************************************************************************************************/
    ;/*
    ; * archExcTempCtx 在内核内存，不经 TLB 转换，所以访问时不会发生 TLB 重填异常
    ; * 用自旋锁的方式访问 archExcTempCtx
    ; */
1:
    LA      K0 , archExcTempCtx
    LL      K1 , TEMP_CTX_OFFSET_LOCK(K0)
    BNEZ    K1 , 1b
    NOP

    LI      K1 , 1
    SC      K1 , TEMP_CTX_OFFSET_LOCK(K0)
    BEQZ    K1 , 1b
    SYNC

;/*********************************************************************************************************
;  2. 保存异常时的 EPC CAUSE BADVADDR 寄存器和异常前的 STATUS 寄存器到 archExcTempCtx
;*********************************************************************************************************/

    MFC0(K1, CP0_EPC)                                                   ;/*  临时保存异常时的 EPC 寄存器 */
    SW      K1 , TEMP_CTX_OFFSET_EPC(K0)

    MFC0(K1, CP0_CAUSE)                                                 ;/*  临时保存异常时 CAUSE 寄存器 */
    SW      K1 , TEMP_CTX_OFFSET_CAUSE(K0)

    MFC0(K1, CP0_BADVADDR)                                              ;/*  临时保存异常时的 BADVADDR   */
    SW      K1 , TEMP_CTX_OFFSET_BADVADDR(K0)

    MFC0(K1, CP0_STATUS)
    LI      K0 , ~(M_StatusEXL | M_StatusERL)
    AND     K1 , K1, K0                                                 ;/*  K1 等于异常前的 SR 寄存器   */
    LA      K0 , archExcTempCtx
    SW      K1 , TEMP_CTX_OFFSET_SR(K0)                                 ;/*  临时保存异常前的 SR 寄存器  */

;/*********************************************************************************************************
;  3. 回到内核模式并关中断
;*********************************************************************************************************/

    LI      K0 , ~(M_StatusIE | M_StatusKSU)
    AND     K1 , K1, K0                                                 ;/*  回到内核模式并关中断        */
    MTC0(K1, CP0_STATUS)
    ;/*
    ; * 已经清除 EXL 位，TLB 重填会在它自己的异常向量入口
    ; */

;/*********************************************************************************************************
;  4. 预访问，提前触发 TLB 重填异常
;*********************************************************************************************************/
    ;/*
    ; * 现在可能发生 TLB 重填，会破坏 CAUSE STATUS K0 K1 EPC BADVADDR 寄存器，不过没关系，
    ; * 因为我们已经把它们都保存好了
    ; */
    SUBU    SP , STK_CTX_SIZE                                           ;/*  修改 SP                     */

    SW      ZERO , 0(SP)
    SW      ZERO , STK_OFFSET_LAST(SP)
    SYNC
    ;/*
    ; * 后面不会再发生 TLB 重填异常
    ; */

;/*********************************************************************************************************
;  5. 保存上下文
;*********************************************************************************************************/

    SAVE_GREGS                                                          ;/*  所有寄存器入栈              */

    LA      T0 , archExcTempCtx

    LW      T1 , TEMP_CTX_OFFSET_SR(T0)
    SW      T1 , STK_OFFSET_SR(SP)                                      ;/*  真正保存异常前的 SR 寄存器  */

    LW      S0 , TEMP_CTX_OFFSET_EPC(T0)                                ;/*  S0 = EPC                    */
    SW      S0 , STK_OFFSET_EPC(SP)                                     ;/*  真正保存异常时的 EPC 寄存器 */

    LW      S1 , TEMP_CTX_OFFSET_CAUSE(T0)                              ;/*  S1 = CAUSE                  */
    SW      S1 , STK_OFFSET_CAUSE(SP)                                   ;/*  真正保存异常时 CAUSE 寄存器 */

    LW      S2 , TEMP_CTX_OFFSET_BADVADDR(T0)                           ;/*  S2 = BADVADDR               */
    SW      S2 , STK_OFFSET_BADVADDR(SP)                                ;/*  真正保存异常时 BADVADDR     */

    MOV     S3 , SP                                                     ;/*  S3 = SP                     */

    MFLO    T1
    EHB
    SW      T1 , STK_OFFSET_LO(SP)                                      ;/*  保存 LO 寄存器              */

    MFHI    T1
    EHB
    SW      T1 , STK_OFFSET_HI(SP)                                      ;/*  保存 HI 寄存器              */

    LA      GP , _gp                                                    ;/*  重新设置 GP 寄存器          */

;/*********************************************************************************************************
;  6. 解锁 archExcTempCtx
;*********************************************************************************************************/

    SYNC
    SW      ZERO , TEMP_CTX_OFFSET_LOCK(T0)                             ;/*  解锁 archExcTempCtx         */

    .set    pop
    MACRO_END()

;/*********************************************************************************************************
;  异常入口宏
;*********************************************************************************************************/

MACRO_DEF(EXCEPTION_ENTRY handle)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile

    EXC_SAVE_REGS                                                       ;/*  所有寄存器入栈              */

    ;/*
    ; * 注意以下的子函数调用不会破坏 S0 S1 S2 S3 寄存器
    ; */

    ;/*
    ; *  if (API_InterEnter() == 1) {
    ; *      current stack = SP;
    ; *      SP = interrupt stack;
    ; *  }
    ; */
    SUBU    SP , 32
    JAL     API_InterEnter                                              ;/*  获取中断层数放到返回值 V0   */
    NOP
    ADDU    SP , 32

    LI      V1 , 1
    BNE     V0 , V1 , 1f
    NOP

    SUBU    SP , 32
    JAL     API_ThreadTcbInter                                          ;/*  get current tcb             */
    NOP
    ADDU    SP , 32
    SW      SP , 0(V0)                                                  ;/*  current stack = SP          */

    SUBU    SP , 32
    JAL     API_InterStackBaseGet                                       ;/*  获得当前 CPU 中断堆栈栈顶   */
    NOP
    MOV     SP , V0

1:
    MOV     A0 , S0                                                     ;/*  ARG0: EPC                   */
    MOV     A1 , S1                                                     ;/*  ARG1: CAUSE                 */
    MOV     A2 , S2                                                     ;/*  ARG2: BADVADDR              */
    MOV     A3 , S3                                                     ;/*  ARG3: SP                    */
    SUBU    SP , 32
    JAL     \handle
    NOP

    JAL     API_InterExit
    NOP
    ADDU    SP , 32

    RESTORE_REGS                                                        ;/*  所有寄存器出栈              */

    .set    pop
    MACRO_END()

;/*********************************************************************************************************
;  中断入口
;*********************************************************************************************************/

FUNC_DEF(archInterruptEntry)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile

    EXCEPTION_ENTRY     bspIntHandle

    .set    pop
    FUNC_END(archInterruptEntry)

;/*********************************************************************************************************
;  Cache 错误异常入口
;*********************************************************************************************************/

FUNC_DEF(archCacheErrorEntry)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile

    EXCEPTION_ENTRY     archCacheErrorHandle

    .set    pop
    FUNC_END(archCacheErrorEntry)

;/*********************************************************************************************************
;  通用异常入口
;*********************************************************************************************************/

FUNC_DEF(archExceptionEntry)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile

    EXCEPTION_ENTRY     archExceptionHandle

    .set    pop
    FUNC_END(archExceptionEntry)

    FILE_END()

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
