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
;** 文   件   名: c6xContextAsm.inc
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2017 年 03 月 17 日
;**
;** 描        述: c6x 体系构架上下文切换.
;*********************************************************************************************************/

;/*********************************************************************************************************
;
;  寄存器信息:
;
;  Non-scratch (被调用函数保护)
;       A10-A15, B10-B14 (B15)
;
;  Scratch (调用者函数保护)
;       A0-A9, A16-A31, B0-B9, B16-B31
;
;  Other (c64x)
;       AMR     Addressing mode                         Task
;         (A4-A7,B4-B7)
;       CSR     Control status                          Task        (*1)
;       GFPGFR  Galois field multiply control           Task
;       ICR     Interrupt clear                         Global
;       IER     Interrupt enable                        Global
;       IFR     Interrupt flag                          Global
;       IRP     Interrupt return pointer                Task
;       ISR     Interrupt set                           Global
;       ISTP    Interrupt service table pointer         Global
;       NRP     Nonmaskable interrupt return pointer    Global      (*2)
;       PCE1    Program counter, E1 phase               Global
;
;  Other (c64x+)
;       DIER    Debug interrupt enable                  Global
;       DNUM    DSP core number                         Global
;       ECR     Exception clear                         Global
;       EFR     Exception flag                          Global
;       GPLYA   GMPY A-side polynomial                  Task
;       GPLYB   GMPY B-side polynomial                  Task
;       IERR    Internal exception report               Global
;       ILC     Inner loop count                        Task
;       ITSR    Interrupt task state                    Task
;               (TSR is copied to ITSR on interrupt)
;       NTSR    NMI/Exception task state                Global      (*2)
;               (TSR is copied to NTSR on exception)
;       REP     Restricted entry point                  Global
;       RILC    Reload inner loop count                 Task
;       SSR     Saturation status                       Task        (*1)
;       TSCH    Time-stamp counter (high 32)            Global
;       TSCL    Time-stamp counter (low 32)             Global
;       TSR     Task state                              Global
;
;  Other (c66x)
;       FADCR   Floating point adder configuration      Task
;       FAUCR   Floating point auxiliary configuration  Task
;       FMCR    Floating point multiplier configuration Task
;
;  (*1) See comment about SAT bit in CSR in sprugh7.
;  (*2) Need to handle these for exception interrupt handling.
;
;*********************************************************************************************************/

;/*********************************************************************************************************
;
;  EABI:
;
;  可用于条件的寄存器有 A0、A1、A2、B0、B1、B2
;
;  前 10 个入口参数使用寄存器 A4、B4、A6、B6、A8、B8、A10、B10、A12、B12
;
;  返回值使用寄存器 A4
;
;*********************************************************************************************************/

;/*********************************************************************************************************
;
;  汇编预定义宏:
;
;  .TMS320C6X          Always set to 1
;  .TMS320C6200        Set to 1 if target is C6200, otherwise 0
;  .TMS320C6400        Set to 1 if target is C6400, C6400+, C6740, or C6600; otherwise 0
;  .TMS320C6400_PLUS   Set to 1 if target is C6400+, C6740, or C6600; otherwise 0
;  .TMS320C6600        Set to 1 if target is C6600, otherwise 0
;  .TMS320C6700        Set to 1 if target is C6700, C6700+, C6740, or C6600; otherwise 0
;  .TMS320C6700_PLUS   Set to 1 if target is C6700+, C6740, or C6600; otherwise 0
;  .TMS320C6740        Set to 1 if target is C6740 or C6600, otherwise 0
;
;*********************************************************************************************************/

;/*********************************************************************************************************
;  保存特殊功能寄存器
;  使用寄存器: B16-B24, B3, B7-B9
;*********************************************************************************************************/

PUSH_SPECIAL_REG .macro
    MVC     ILC  , B16
    STW     B16  , *B15--               ;/*  ILC                                                         */
 || MVC     GPLYA, B17
    STW     B17  , *B15--               ;/*  GPLYA                                                       */
 || MVC     GPLYB, B18
    STW     B18  , *B15--               ;/*  GPLYB                                                       */
 || MVC     ITSR , B19
    STW     B19  , *B15--               ;/*  ITSR                                                        */
 || MVC     RILC , B20
    STW     B20  , *B15--               ;/*  RILC                                                        */
 || MVC     SSR  , B21
    .if (.TMS320C6740)
    STW     B21  , *B15--               ;/*  SSR                                                         */
 || MVC     FADCR, B22
    STW     B22  , *B15--               ;/*  FADCR                                                       */
 || MVC     FAUCR, B23
    STW     B23  , *B15--               ;/*  FAUCR                                                       */
 || MVC     FMCR , B24
    .else
    STW     B21  , *B15--               ;/*  SSR                                                         */
    STW     B22  , *B15--               ;/*  FADCR                                                       */
    STW     B23  , *B15--               ;/*  FAUCR                                                       */
    .endif
    STW     B24  , *B15--               ;/*  FMCR                                                        */
    ADDK    -4   , B15                  ;/*  Align 64bit                                                 */
    MVC     GFPGFR, B7
    STW     B3   , *B15--               ;/*  B3                                                          */
 || MVC     AMR  , B8
    STW     B7   , *B15--               ;/*  GFPGFR                                                      */
 || MVC     CSR  , B9
    STW     B8   , *B15--               ;/*  AMR                                                         */
    STW     B9   , *B15--               ;/*  CSR                                                         */
    .endm

;/*********************************************************************************************************
;  恢复特殊功能寄存器
;  使用寄存器: B3, B7-B9, B16-B25
;*********************************************************************************************************/

POP_SPECIAL_REG .macro
    LDW     *++B15 , B9                 ;/*  CSR                                                         */
    LDW     *++B15 , B8                 ;/*  AMR                                                         */
    LDW     *++B15 , B7                 ;/*  GFPGFR                                                      */
    LDW     *++B15 , B3                 ;/*  B3                                                          */
    ADDK    +4     , B15                ;/*  Align 64bit                                                 */
    LDW     *++B15 , B24                ;/*  FMCR                                                        */
    LDW     *++B15 , B23                ;/*  FAUCR                                                       */
    LDW     *++B15 , B22                ;/*  FADCR                                                       */
    LDW     *++B15 , B21                ;/*  SSR                                                         */
    LDW     *++B15 , B20                ;/*  RILC                                                        */
 || EXTU    B9  , 22 , 31 , B25         ;/*  Extract SAT bit                                             */
    LDW     *++B15 , B19                ;/*  ITSR                                                        */
 || CLR     B9  , 9  , 9  , B9          ;/*  Clear SAT bit                                               */
    LDW     *++B15 , B18                ;/*  GPLYB                                                       */
 || MVC     B9  , CSR                   ;/*  Restore CSR                                                 */
    LDW     *++B15 , B17                ;/*  GPLYA                                                       */
 || SSHL    B25 , 31 , B25              ;/*  Restore SAT bit.                                            */
    LDW     *++B15 , B16                ;/*  ILC                                                         */
 || MVC     B8  , AMR
    MVC     B7  , GFPGFR
    .if (.TMS320C6740)
    MVC     B24 , FMCR
    MVC     B23 , FAUCR
    MVC     B22 , FADCR
    .endif
    MVC     B21 , SSR
    MVC     B20 , RILC
    MVC     B19 , ITSR
    MVC     B18 , GPLYB
    MVC     B17 , GPLYA
    MVC     B16 , ILC
    .endm

;/*********************************************************************************************************
;  保存 non scratch 寄存器
;  使用寄存器: A8
;*********************************************************************************************************/

PUSH_NON_SCR_REG .macro
    ADD     -8  , B15 , A8              ;/*  A8 should push a DW after B15                               */
    STW     B14 , *B15--(16)            ;/*  Put single word store first                                 */
 || STDW    A15:A14 , *A8--[2]
    STDW    B11:B10 , *B15--[2]
 || STDW    A11:A10 , *A8--[2]
    STDW    B13:B12 , *B15--[2]
 || STDW    A13:A12 , *A8
    .endm

;/*********************************************************************************************************
;  恢复 non scratch 寄存器
;  使用寄存器: A8,A10-A15,B10-B14
;*********************************************************************************************************/

POP_NON_SCR_REG .macro
    MV      B15 , A8
    LDDW    *++A8[1]  , A13:A12
 || LDDW    *++B15[2] , B13:B12
    LDDW    *++A8[2]  , A11:A10
 || LDDW    *++B15[2] , B11:B10
    LDDW    *++A8[2]  , A15:A14
 || LDW     *++B15(16), B14             ;/*  Put single word load last                                   */
    .endm

;/*********************************************************************************************************
;  保存 scratch 寄存器
;  使用寄存器: A10
;*********************************************************************************************************/

PUSH_SCR_REG .macro
    ADD     -8 , B15, A10               ;/*  A10 should push a DW after B15                              */
    STDW    B1:B0   , *B15--[2]
 || STDW    A1:A0   , *A10--[2]
    STDW    B3:B2   , *B15--[2]
 || STDW    A3:A2   , *A10--[2]
    STDW    B5:B4   , *B15--[2]
 || STDW    A5:A4   , *A10--[2]
    STDW    B7:B6   , *B15--[2]
 || STDW    A7:A6   , *A10--[2]
    STDW    B9:B8   , *B15--[2]
 || STDW    A9:A8   , *A10--[2]
    STDW    B17:B16 , *B15--[2]
 || STDW    A17:A16 , *A10--[2]
    STDW    B19:B18 , *B15--[2]
 || STDW    A19:A18 , *A10--[2]
    STDW    B21:B20 , *B15--[2]
 || STDW    A21:A20 , *A10--[2]
    STDW    B23:B22 , *B15--[2]
 || STDW    A23:A22 , *A10--[2]
    STDW    B25:B24 , *B15--[2]
 || STDW    A25:A24 , *A10--[2]
    STDW    B27:B26 , *B15--[2]
 || STDW    A27:A26 , *A10--[2]
    STDW    B29:B28 , *B15--[2]
 || STDW    A29:A28 , *A10--[2]
    STDW    B31:B30 , *B15--[2]
 || STDW    A31:A30 , *A10
    .endm

;/*********************************************************************************************************
;  恢复 scratch 寄存器
;  使用寄存器: A0-A9,A10,A16-A31,B0-B9,B16-B31
;*********************************************************************************************************/

POP_SCR_REG .macro
    ;/*
    ; * 避免并行操作, 因为 DMA/L1D 的硬件 BUG
    ; */
    MV      B15 , A10                   ;/*  A10 should pop a DW before B15                              */
    LDDW    *++A10[1] , A31:A30
    LDDW    *++B15[2] , B31:B30
    LDDW    *++A10[2] , A29:A28
    LDDW    *++B15[2] , B29:B28
    LDDW    *++A10[2] , A27:A26
    LDDW    *++B15[2] , B27:B26
    LDDW    *++A10[2] , A25:A24
    LDDW    *++B15[2] , B25:B24
    LDDW    *++A10[2] , A23:A22
    LDDW    *++B15[2] , B23:B22
    LDDW    *++A10[2] , A21:A20
    LDDW    *++B15[2] , B21:B20
    LDDW    *++A10[2] , A19:A18
    LDDW    *++B15[2] , B19:B18
    LDDW    *++A10[2] , A17:A16
    LDDW    *++B15[2] , B17:B16
    LDDW    *++A10[2] , A9:A8
    LDDW    *++B15[2] , B9:B8
    LDDW    *++A10[2] , A7:A6
    LDDW    *++B15[2] , B7:B6
    LDDW    *++A10[2] , A5:A4
    LDDW    *++B15[2] , B5:B4
    LDDW    *++A10[2] , A3:A2
    LDDW    *++B15[2] , B3:B2
    LDDW    *++A10[2] , A1:A0
    LDDW    *++B15[2] , B1:B0
    .endm

;/*********************************************************************************************************
;  恢复一个大的上下文
;*********************************************************************************************************/

RESTORE_BIG_CTX     .macro
    POP_NON_SCR_REG

    POP_SPECIAL_REG
    MVC     B3 , IRP

    POP_SCR_REG

    ADDK    +4 , B15                    ;/*  Align stack                                                 */
    LDW     *++B15 , A10                ;/*  Pop A10                                                     */
    B       IRP                         ;/*  Return from interrupt                                       */
 || LDW     *++B15 , A11                ;/*  Pop A11                                                     */
    LDW     *++B15 , B11                ;/*  Pop B11                                                     */
    NOP     4                           ;/*  LDs needs to be done before branch                          */
    .endm

;/*********************************************************************************************************
;  保存一个大的上下文
;*********************************************************************************************************/

SAVE_BIG_CTX_IRQ   .macro
    ;/*
    ; * Note that SP set to 64 bit alignement since uneven
    ; * number of registers (A10,A11,B11) have been pushed to stack in _vectorXX
    ; */
    ADDK    -4 , B15

    PUSH_SCR_REG

    MVC     IRP , B3
    PUSH_SPECIAL_REG

    PUSH_NON_SCR_REG
    .endm

;/*********************************************************************************************************
;  异常保存一个大的上下文
;*********************************************************************************************************/

SAVE_BIG_CTX_EXC   .macro
    ;/*
    ; * Note that SP set to 64 bit alignement since uneven
    ; * number of registers (A10,A11,B11) have been pushed to stack in _vectorXX
    ; */
    ADDK    -4 , B15

    PUSH_SCR_REG

    MVC     NRP , B3
    PUSH_SPECIAL_REG

    PUSH_NON_SCR_REG
    .endm

;/*********************************************************************************************************
;  恢复一个小上下文
;*********************************************************************************************************/

RESTORE_SMALL_CTX .macro
    LDW     *++B15 , B9                 ;/*  CSR                                                         */
    LDW     *++B15 , B8                 ;/*  AMR                                                         */
    LDW     *++B15 , B7                 ;/*  GFPGFR                                                      */
    LDW     *++B15 , B3                 ;/*  B3                                                          */
    ADDK    +4     , B15                ;/*  Reserved1                                                   */
    LDW     *++B15 , B24                ;/*  FMCR                                                        */
    LDW     *++B15 , B23                ;/*  FAUCR                                                       */
    LDW     *++B15 , B22                ;/*  FADCR                                                       */
    LDW     *++B15 , B21                ;/*  SSR                                                         */
    LDW     *++B15 , B20                ;/*  RILC                                                        */
 || EXTU    B9  , 22 , 31 , B25         ;/*  Extract SAT bit                                             */
    LDW     *++B15 , B19                ;/*  ITSR                                                        */
 || CLR     B9  , 9  , 9  , B9          ;/*  Clear SAT bit                                               */
    LDW     *++B15 , B18                ;/*  GPLYB                                                       */
 || MVC     B9  , CSR                   ;/*  Restore CSR                                                 */
    LDW     *++B15 , B17                ;/*  GPLYA                                                       */
 || SSHL    B25 , 31 , B25              ;/*  Restore SAT bit.                                            */
    LDW     *++B15 , B16                ;/*  ILC                                                         */
 || MVC     B8  , AMR
    MVC     B7  , GFPGFR
    .if (.TMS320C6740)
    MVC     B24 , FMCR
    MVC     B23 , FAUCR
    MVC     B22 , FADCR
    .endif
    MVC     B21 , SSR
    MVC     B20 , RILC
    MVC     B19 , ITSR
    MVC     B18 , GPLYB
    MVC     B17 , GPLYA
    MVC     B16 , ILC
    MV      B15 , A8
    LDDW    *++A8[1]  , A13:A12
 || LDDW    *++B15[2] , B13:B12
    LDDW    *++A8[2]  , A11:A10
 || LDDW    *++B15[2] , B11:B10

    LDDW    *++A8[2]  , A15:A14
    LDW     *++B15(16), B14             ;/*  Get single word load last                                   */
    B       B3
    LDW     *++B15 , A4                 ;/*  A4                                                          */
    NOP     4                           ;/*  LDs needs to be done before branch                          */
    .endm

;/*********************************************************************************************************
;  保存一个小上下文
;*********************************************************************************************************/

SAVE_SMALL_CTX  .macro
    ADDK    -4   , B15
    ADD     -8   , B15 , A8             ;/*  A8 should push a DW after B15                               */
    STW     B14  , *B15--(16)           ;/*  Put single word store first                                 */
 || STDW    A15:A14 ,  *A8--[2]
    STDW    B11:B10 , *B15--[2]
 || STDW    A11:A10 ,  *A8--[2]
    STDW    B13:B12 , *B15--[2]
 || STDW    A13:A12 ,  *A8
 || MVC     ILC  , B16
    STW     B16  , *B15--               ;/*  ILC                                                         */
 || MVC     GPLYA, B17
    STW     B17  , *B15--               ;/*  GPLYA                                                       */
 || MVC     GPLYB, B18
    STW     B18  , *B15--               ;/*  GPLYB                                                       */
 || MVC     ITSR , B19
    STW     B19  , *B15--               ;/*  ITSR                                                        */
 || MVC     RILC , B20
    STW     B20  , *B15--               ;/*  RILC                                                        */
 || MVC     SSR  , B21
    .if (.TMS320C6740)
    STW     B21  , *B15--               ;/*  SSR                                                         */
 || MVC     FADCR, B22
    STW     B22  , *B15--               ;/*  FADCR                                                       */
 || MVC     FAUCR, B23
    STW     B23  , *B15--               ;/*  FAUCR                                                       */
 || MVC     FMCR , B24
    .else
    STW     B21  , *B15--               ;/*  SSR                                                         */
    STW     B22  , *B15--               ;/*  FADCR                                                       */
    STW     B23  , *B15--               ;/*  FAUCR                                                       */
    .endif
    STW     B24  , *B15--               ;/*  FMCR                                                        */
    ADDK    -4   , B15                  ;/*  Reserved1                                                   */
    MVC     GFPGFR, B7
    STW     B3   , *B15--               ;/*  B3                                                          */
 || MVC     AMR  , B8
    STW     B7   , *B15--               ;/*  GFPGFR                                                      */
 || MVC     CSR  , B9
    STW     B8   , *B15--               ;/*  AMR                                                         */
    STW     B9   , *B15--               ;/*  CSR                                                         */
    .endm
    
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
