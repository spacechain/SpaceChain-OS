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
;** 文   件   名: sparcWindowAsm.S
;**
;** 创   建   人: Xu.Guizhou (徐贵洲)
;**
;** 文件创建日期: 2017 年 05 月 15 日
;**
;** 描        述: SPARC 体系构架寄存器窗口处理.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/kernel/kernel_cfg.h>
#include <config/mp/mp_cfg.h>

    FILE_BEGIN()

    EXPORT_LABEL(archWinOverFlowEntry)
    EXPORT_LABEL(archWinUnderFlowEntry)
    EXPORT_LABEL(archWinFlushTrapEntry)

;/*********************************************************************************************************
;  Window overflow trap handler.
;
;  On entry:
;
;    l0 = psr (from trap table)
;    l1 = pc
;    l2 = npc
;*********************************************************************************************************/

FUNC_DEF(archWinOverFlowEntry)
    ;/*
    ; *  Calculate new WIM by "rotating" the valid bits in the WIM right
    ; *  by one position.  The following shows how the bits move for a SPARC
    ; *  cpu implementation where LW_CFG_SPARC_REG_WIN_NR is 8.
    ; *
    ; *    OLD WIM = 76543210
    ; *    NEW WIM = 07654321
    ; *
    ; *  NOTE: New WIM must be stored in a global register since the
    ; *        "save" instruction just prior to the load of the wim
    ; *        register will result in the local register set changing.
    ; */
    MOV     %wim , %l3                                  ;/*  l3 = WIM                                    */
    MOV     %g1  , %l7                                  ;/*  save g1                                     */
    SRL     %l3  , 1 , %g1                              ;/*  g1 = WIM >> 1                               */
    SLL     %l3  , LW_CFG_SPARC_REG_WIN_NR - 1 , %l4
                                                        ;/*  l4 = WIM << (Number Windows - 1)            */
    OR      %l4  , %g1 , %g1                            ;/*  g1 = (WIM >> 1) |                           */
                                                        ;/*       (WIM << (Number Windows - 1))          */

    SAVE                                                ;/*  Get into window to be saved.                */
    MOV     %g1  , %wim                                 ;/*  load new WIM                                */
    NOP
    NOP
    NOP
    STD     %l0  , [%sp + SF_LOCAL(0)]
    SPARC_B2BST_NOP
    STD     %l2  , [%sp + SF_LOCAL(2)]
    SPARC_B2BST_NOP
    STD     %l4  , [%sp + SF_LOCAL(4)]
    SPARC_B2BST_NOP
    STD     %l6  , [%sp + SF_LOCAL(6)]
    SPARC_B2BST_NOP

    STD     %i0  , [%sp + SF_INPUT(0)]
    SPARC_B2BST_NOP
    STD     %i2  , [%sp + SF_INPUT(2)]
    SPARC_B2BST_NOP
    STD     %i4  , [%sp + SF_INPUT(4)]
    SPARC_B2BST_NOP
    STD     %i6  , [%sp + SF_INPUT(6)]
    RESTORE                                             ;/*  Go back to trap window.                     */
    MOV     %l7  , %g1                                  ;/*  restore g1                                  */
    JMP     %l1                                         ;/*  Re-execute save.                            */
    RETT    %l2
    FUNC_END(archWinOverFlowEntry)

;/*********************************************************************************************************
;  Window underflow trap handler.
;
;  On entry:
;
;    l0 = psr (from trap table)
;    l1 = pc
;    l2 = npc
;*********************************************************************************************************/

FUNC_DEF(archWinUnderFlowEntry)
    ;/*
    ; *  Calculate new WIM by "rotating" the valid bits in the WIM left
    ; *  by one position.  The following shows how the bits move for a SPARC
    ; *  cpu implementation where LW_CFG_SPARC_REG_WIN_NR is 8.
    ; *
    ; *    OLD WIM = 76543210
    ; *    NEW WIM = 07654321
    ; *
    ; *  NOTE: New WIM must be stored in a global register since the
    ; *        "save" instruction just prior to the load of the wim
    ; *        register will result in the local register set changing.
    ; */
    MOV     %wim , %l3                                  ;/*  Calculate new WIM                           */
    SLL     %l3  , 1 , %l4                              ;/*  l4 = WIM << 1                               */
    SRL     %l3  , LW_CFG_SPARC_REG_WIN_NR - 1 , %l5
                                                        ;/*  l5 = WIM >> (Number Windows-1)              */
    OR      %l5  , %l4 , %l5                            ;/*  l5 = (WIM << 1) |                           */
                                                        ;/*       (WIM >> (Number Windows-1))            */
    MOV     %l5  , %wim                                 ;/*  load the new WIM                            */
    NOP
    NOP
    NOP
    RESTORE                                             ;/*  Two restores to get into the                */
    RESTORE                                             ;/*  window to restore                           */
    LDD     [%sp + SF_LOCAL(0)] , %l0                   ;/*  First the local register set                */
    LDD     [%sp + SF_LOCAL(2)] , %l2
    LDD     [%sp + SF_LOCAL(4)] , %l4
    LDD     [%sp + SF_LOCAL(6)] , %l6
    LDD     [%sp + SF_INPUT(0)] , %i0                   ;/*  Then the input registers                    */
    LDD     [%sp + SF_INPUT(2)] , %i2
    LDD     [%sp + SF_INPUT(4)] , %i4
    LDD     [%sp + SF_INPUT(6)] , %i6
    SAVE                                                ;/*  Get back to the trap window.                */
    SAVE
    JMP     %l1                                         ;/*  Re-execute restore.                         */
    RETT    %l2
    FUNC_END(archWinUnderFlowEntry)

;/*********************************************************************************************************
;  Flush All Windows trap handler.
;
;  Flush all windows with valid contents except the current one
;  and the one we will be returning to.
;
;  In examining the set register windows, one may logically divide
;  the windows into sets (some of which may be empty) based on their
;  current status:
;
;    + current (i.e. in use),
;    + used (i.e. a restore would not trap)
;    + invalid (i.e. 1 in corresponding bit in WIM)
;    + unused
;
;  Either the used or unused set of windows may be empty.
;
;  NOTE: We assume only one bit is set in the WIM at a time.
;
;  Given a CWP of 5 and a WIM of 0x1, the registers are divided
;  into sets as follows:
;
;    + 0   - invalid
;    + 1-4 - unused
;    + 5   - current
;    + 6-7 - used
;
;  In this case, we only would save the used windows which we
;  will not be returning to -- 6.
;
;    Register Usage while saving the windows:
;      g1 = current PSR
;      g2 = current wim
;      g3 = CWP
;      g4 = wim scratch
;      g5 = scratch
;
;  On entry:
;
;    l0 = psr (from trap table)
;    l1 = pc
;    l2 = npc
;*********************************************************************************************************/

FUNC_DEF(archWinFlushTrapEntry)
    ;/*
    ; *  Save the global registers we will be using
    ; */
    MOV     %g1  , %l3
    MOV     %g2  , %l4
    MOV     %g3  , %l5
    MOV     %g4  , %l6
    MOV     %g5  , %l7

    MOV     %l0  , %g1                                  ;/*  g1 = psr                                    */
    MOV     %wim , %g2                                  ;/*  g2 = wim                                    */
    AND     %l0  , SPARC_PSR_CWP_MASK , %g3             ;/*  g3 = CWP                                    */

    ADD     %g3  , 1 , %g5                              ;/*  g5 = CWP + 1                                */
    AND     %g5  , LW_CFG_SPARC_REG_WIN_NR - 1 , %g5

    MOV     1    , %g4
    SLL     %g4  , %g5 , %g4                            ;/*  g4 = WIM mask for CWP+1 invalid             */

    RESTORE                                             ;/*  go back one register window                 */

LINE_LABEL(save_frame_loop)
    SLL     %g4  , 1 , %g5                              ;/*  rotate the "wim" left 1                     */
    SRL     %g4  , LW_CFG_SPARC_REG_WIN_NR - 1 , %g4
    OR      %g4  , %g5 , %g4                            ;/*  g4 = wim if we do one restore               */

    ;/*
    ; *  If a restore would not underflow, then continue.
    ; */
    ANDCC   %g4 , %g2 , %g0                             ;/*  Any windows to flush?                       */
    BNZ     done_flushing                               ;/*  No, then continue                           */
    NOP

    RESTORE                                             ;/*  back one window                             */

    ;/*
    ; *  Now save the window just as if we overflowed to it.
    ; */
    STD     %l0 , [%sp + SF_LOCAL(0)]
    SPARC_B2BST_NOP
    STD     %l2 , [%sp + SF_LOCAL(2)]
    SPARC_B2BST_NOP
    STD     %l4 , [%sp + SF_LOCAL(4)]
    SPARC_B2BST_NOP
    STD     %l6 , [%sp + SF_LOCAL(6)]
    SPARC_B2BST_NOP

    STD     %i0 , [%sp + SF_INPUT(0)]
    SPARC_B2BST_NOP
    STD     %i2 , [%sp + SF_INPUT(2)]
    SPARC_B2BST_NOP
    STD     %i4 , [%sp + SF_INPUT(4)]
    SPARC_B2BST_NOP
    STD     %i6 , [%sp + SF_INPUT(6)]

    BA      save_frame_loop
    NOP

LINE_LABEL(done_flushing)
    ADD     %g3 , 2 , %g3                               ;/*  calculate desired WIM                       */
    AND     %g3 , LW_CFG_SPARC_REG_WIN_NR - 1 , %g3
    MOV     1   , %g4
    SLL     %g4 , %g3 , %g4                             ;/*  g4 = new WIM                                */
    MOV     %g4 , %wim

    MOV     %g1 , %psr                                  ;/*  restore PSR                                 */
    NOP
    NOP
    NOP

    ;/*
    ; *  Restore the global registers we used
    ; */
    MOV     %l3 , %g1
    MOV     %l4 , %g2
    MOV     %l5 , %g3
    MOV     %l6 , %g4
    MOV     %l7 , %g5

    JMPL    %l2 , %g0
    RETT    %l2 + 4
    FUNC_END(archWinFlushTrapEntry)

    FILE_END()

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
