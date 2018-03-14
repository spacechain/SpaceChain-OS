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
;** 文   件   名: c6xExcAsm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2017 年 03 月 17 日
;**
;** 描        述: c6x 体系构架异常/中断处理.
;*********************************************************************************************************/

    .include "c6xContextAsmInc.asm"

    .ref    bspIntHandle
    .ref    archExcHandle
    .ref    API_InterEnter
    .ref    API_InterExit
    .ref    API_InterStackBaseGet
    .ref    API_ThreadTcbInter

    .global archIntEntry1
    .global archIntEntry2
    .global archIntEntry3
    .global archIntEntry4
    .global archIntEntry5
    .global archIntEntry6
    .global archIntEntry7
    .global archIntEntry8
    .global archIntEntry9
    .global archIntEntry10
    .global archIntEntry11
    .global archIntEntry12
    .global archIntEntry13
    .global archIntEntry14
    .global archIntEntry15

    .sect .text

;/*********************************************************************************************************
;  中断进入点
;  注意: 在跳转到 ARCH_INT_ENTRY 前必须完成以下操作:
;   STW     B11 , *B15--
;   STW     A11 , *B15--
;   STW     A10 , *B15--
;*********************************************************************************************************/

ARCH_INT_ENTRY  .macro  vector
    SAVE_BIG_CTX_IRQ

    NOP     4                                                   ;/*  确保上下文保存完成                  */
    MV      B15 , A10                                           ;/*  A10 = B15                           */
    AND     ~7  , B15 , B15                                     ;/*  SP 向下 16 字节对齐                 */

    ;/*
    ; *  if (API_InterEnter() == 1) {
    ; *      current stack = SP;
    ; *      SP = interrupt stack;
    ; *  }
    ; */
    B       API_InterEnter                                      ;/*  CALL A4 = API_InterEnter();         */
    ADDKPC  $1  , B3 , 4
$1:
    CMPGT   A4  , 1 , A1                                        ;/*  A1 = A4 > 1 ? 1 : 0;                */
    [A1]    BNOP $4 , 5                                         ;/*  if (A1) { goto $4; }                */

    B       API_ThreadTcbInter                                  ;/*  CALL A4 = API_ThreadTcbInter();     */
    ADDKPC  $2  , B3 , 4
$2:
    STW     A10 , *+A4(0)                                       ;/*  Save current context sp             */
 || MVK     1   , A2
    STW     A2  , *+A4(4)                                       ;/*  Set big context flag                */
 || B       API_InterStackBaseGet                               ;/*  CALL A4 = API_InterStackBaseGet();  */
    ADDKPC  $3  , B3 , 4
$3:
    MV      A4  , B15                                           ;/*  B15 = A4;                           */

$4:
    B       bspIntHandle                                        ;/*  CALL bspIntHandle(vector);          */
 || MVK     vector , A4                                         ;/*  参数为 vector                       */
    ADDKPC  $5  , B3 , 4
$5:

    B       API_InterExit                                       ;/*  CALL API_InterExit();               */
    ADDKPC  $6  , B3 , 4
$6:

    MV      A10 , B15                                           ;/*  B15 = A10                           */
    RESTORE_BIG_CTX                                             ;/*  Restore big context                 */
    .endm

;/*********************************************************************************************************
;  异常进入点
;  注意: 在跳转到 ARCH_EXC_ENTRY 前必须完成以下操作:
;   STW     B11 , *B15--
;   STW     A11 , *B15--
;   STW     A10 , *B15--
;*********************************************************************************************************/

ARCH_EXC_ENTRY  .macro  vector
    SAVE_BIG_CTX_EXC

    NOP     4                                                   ;/*  确保上下文保存完成                  */
    MV      B15 , A10                                           ;/*  A10 = B15                           */
    AND     ~7  , B15 , B15                                     ;/*  SP 向下 16 字节对齐                 */

    ;/*
    ; *  if (API_InterEnter() == 1) {
    ; *      current stack = SP;
    ; *      SP = interrupt stack;
    ; *  }
    ; */
    B       API_InterEnter                                      ;/*  CALL A4 = API_InterEnter();         */
    ADDKPC  $1  , B3 , 4
$1:
    CMPGT   A4  , 1 , A1                                        ;/*  A1 = A4 > 1 ? 1 : 0;                */
    [A1]    BNOP $4 , 5                                         ;/*  if (A1) { goto $4; }                */

    B       API_ThreadTcbInter                                  ;/*  CALL A4 = API_ThreadTcbInter();     */
    ADDKPC  $2  , B3 , 4
$2:
    STW     A10 , *+A4(0)                                       ;/*  Save current context sp             */
 || MVK     1   , A2
    STW     A2  , *+A4(4)                                       ;/*  Set big context flag                */
 || B       API_InterStackBaseGet                               ;/*  CALL A4 = API_InterStackBaseGet();  */
    ADDKPC  $3  , B3 , 4
$3:
    MV      A4  , B15                                           ;/*  B15 = A4;                           */

$4:
    B       archExcHandle                                       ;/*  CALL archExcHandle();               */
    ADDKPC  $5  , B3 , 4
$5:

    B       API_InterExit                                       ;/*  CALL API_InterExit();               */
    ADDKPC  $6  , B3 , 4
$6:

    MV      A10 , B15                                           ;/*  B15 = A10                           */
    RESTORE_BIG_CTX                                             ;/*  Restore big context                 */
    .endm

;/*********************************************************************************************************
;  中断进入点
;*********************************************************************************************************/

archIntEntry1: .asmfunc
    ARCH_EXC_ENTRY
    .endasmfunc

archIntEntry2: .asmfunc
    ARCH_INT_ENTRY 2
    .endasmfunc

archIntEntry3: .asmfunc
    ARCH_INT_ENTRY 3
    .endasmfunc

archIntEntry4: .asmfunc
    ARCH_INT_ENTRY 4
    .endasmfunc

archIntEntry5: .asmfunc
    ARCH_INT_ENTRY 5
    .endasmfunc

archIntEntry6: .asmfunc
    ARCH_INT_ENTRY 6
    .endasmfunc

archIntEntry7: .asmfunc
    ARCH_INT_ENTRY 7
    .endasmfunc

archIntEntry8: .asmfunc
    ARCH_INT_ENTRY 8
    .endasmfunc

archIntEntry9: .asmfunc
    ARCH_INT_ENTRY 9
    .endasmfunc

archIntEntry10: .asmfunc
    ARCH_INT_ENTRY 10
    .endasmfunc

archIntEntry11: .asmfunc
    ARCH_INT_ENTRY 11
    .endasmfunc

archIntEntry12: .asmfunc
    ARCH_INT_ENTRY 12
    .endasmfunc

archIntEntry13: .asmfunc
    ARCH_INT_ENTRY 13
    .endasmfunc

archIntEntry14: .asmfunc
    ARCH_INT_ENTRY 14
    .endasmfunc

archIntEntry15: .asmfunc
    ARCH_INT_ENTRY 15
    .endasmfunc

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
