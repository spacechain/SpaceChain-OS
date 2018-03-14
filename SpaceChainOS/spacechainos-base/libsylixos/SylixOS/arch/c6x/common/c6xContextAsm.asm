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
;** 文   件   名: c6xContextAsm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2017 年 03 月 17 日
;**
;** 描        述: c6x 体系构架上下文切换.
;*********************************************************************************************************/

    .include "c6xContextAsmInc.asm"

    .ref    _SchedSwp
    .ref    _SchedCrSwp

    .global archTaskCtxInit
    .global archTaskCtxStart
    .global archTaskCtxSwitch
    .global archCrtCtxSwitch
    .global archIntCtxLoad
    .global archSigCtxLoad

    .sect .text

;/*********************************************************************************************************
;  初始化任务上下文中的特殊功能寄存器部分
;*********************************************************************************************************/

archTaskCtxInit: .asmfunc
    MVC     ILC  , B16
    STW     B16  , *A4--                                                ;/*  ILC                         */
 || MVC     GPLYA, B17

    STW     B17  , *A4--                                                ;/*  GPLYA                       */
 || MVC     GPLYB, B18

    STW     B18  , *A4--                                                ;/*  GPLYB                       */
 || MVC     ITSR , B19

    STW     B19  , *A4--                                                ;/*  ITSR                        */
 || MVC     RILC , B20

    STW     B20  , *A4--                                                ;/*  RILC                        */
 || MVC     SSR  , B21

    .if (.TMS320C6740)
    STW     B21  , *A4--                                                ;/*  SSR                         */
 || MVC     FADCR, B22

    STW     B22  , *A4--                                                ;/*  FADCR                       */
 || MVC     FAUCR, B23

    STW     B23  , *A4--                                                ;/*  FAUCR                       */
 || MVC     FMCR , B24
    .else
    STW     B21  , *A4--                                                ;/*  SSR                         */
    STW     B22  , *A4--                                                ;/*  FADCR                       */
    STW     B23  , *A4--                                                ;/*  FAUCR                       */
    .endif

    STW     B24  , *A4--                                                ;/*  FMCR                        */

    ADDK    -4   , A4                                                   ;/*  Align 64bit                 */

    MVC     GFPGFR, B7

    STW     B3   , *A4--                                                ;/*  B3                          */
 || MVC     AMR  , B8

    STW     B7   , *A4--                                                ;/*  GFPGFR                      */
 || MVC     CSR  , B9

    STW     B8   , *A4--                                                ;/*  AMR                         */

    B       B3
 || STW     B9   , *A4--                                                ;/*  CSR                         */

    NOP     5
    .endasmfunc

;/*********************************************************************************************************
;  当前 CPU 开始运行多任务执行
;  参数(A4 寄存器)为当前 CPU 控制块
;*********************************************************************************************************/

archTaskCtxStart: .asmfunc
    LDW     *+A4(0) , A3                                                ;/*  Load new tcb                */
    NOP     4
    LDW     *+A3(0) , B15                                               ;/*  Load new context SP         */
    NOP     4
    RESTORE_SMALL_CTX                                                   ;/*  Restore small context       */
    .endasmfunc

;/*********************************************************************************************************
;  线程切换 
;  参数(A4 寄存器)为当前 CPU 控制块
;*********************************************************************************************************/

archTaskCtxSwitch: .asmfunc
    SAVE_SMALL_CTX                                                      ;/*  Save small context          */

    LDW     *+A4(0) , A3                                                ;/*  Load current tcb            */
    NOP     4

    STW     B15 , *+A3(0)                                               ;/*  Save current context SP     */
 || MVK     0   , A2
    STW     A2  , *+A3(4)                                               ;/*  Set small context flag      */
 || MV      A4  , A10                                                   ;/*  Save A4 to A10              */
    B       _SchedSwp                                                   ;/*  CALL _SchedSwp()            */
 || AND     ~7  , B15 , B15
    ADDKPC  _SchedSwpRet , B3 , 4
_SchedSwpRet:

    LDW     *+A10(0) , A3                                               ;/*  Load new tcb                */
    NOP     4

    LDW     *+A3(4) , A2                                                ;/*  Load context flag           */
    LDW     *+A3(0) , B15                                               ;/*  Load new context SP         */
    NOP     3
    [A2]    BNOP  _RestoreBigCtx , 5

    RESTORE_SMALL_CTX                                                   ;/*  Restore small context       */

_RestoreBigCtx:
    RESTORE_BIG_CTX                                                     ;/*  Restore big context         */
    .endasmfunc

;/*********************************************************************************************************
;  协程切换 
;  参数(A4 寄存器)为当前 CPU 控制块
;*********************************************************************************************************/

archCrtCtxSwitch: .asmfunc
    SAVE_SMALL_CTX                                                      ;/*  Save big context            */

    LDW     *+A4(8) , A3                                                ;/*  Load current crcb           */
    NOP     4

    STW     B15 , *+A3(0)                                               ;/*  Save current context SP     */
 || MV      A4  , A10                                                   ;/*  Save A4 to A10              */
    B       _SchedCrSwp                                                 ;/*  CALL _SchedCrSwp()          */
 || AND     ~7  , B15 , B15
    ADDKPC  _SchedCrSwpRet , B3 , 4
_SchedCrSwpRet:

    LDW     *+A10(8) , A3                                               ;/*  Load new crcb               */
    NOP     4

    LDW     *+A3(0) , B15                                               ;/*  Load new context SP         */
    NOP     4

    RESTORE_SMALL_CTX                                                   ;/*  Restore big context         */
    .endasmfunc

;/*********************************************************************************************************
;  中断返回时, 线程装载
;  参数(A4 寄存器)为当前 CPU 控制块
;*********************************************************************************************************/

archIntCtxLoad: .asmfunc
    LDW     *+A4(0) , A3                                                ;/*  Load new tcb                */
    NOP     4

    LDW     *+A3(4) , A2                                                ;/*  Load context flag           */
    LDW     *+A3(0) , B15                                               ;/*  Load new context SP         */
    NOP     3
    [A2]    BNOP  _RestoreBigCtx2 , 5

    RESTORE_SMALL_CTX                                                   ;/*  Restore small context       */

_RestoreBigCtx2:
    RESTORE_BIG_CTX                                                     ;/*  Restore big context         */
    .endasmfunc

;/*********************************************************************************************************
;  信号上下文返回
;  参数1(A4 寄存器)为需要返回的堆栈栈顶位置
;  参数2(B4 寄存器)为上下文类型
;*********************************************************************************************************/

archSigCtxLoad: .asmfunc
    MV      A4 , B15                                                    ;/*  Load new context SP         */
    MV      B4 , A2
    [A2]    BNOP  _RestoreBigCtx3 , 5

    RESTORE_SMALL_CTX                                                   ;/*  Restore small context       */

_RestoreBigCtx3:
    RESTORE_BIG_CTX                                                     ;/*  Restore big context         */
    .endasmfunc

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
