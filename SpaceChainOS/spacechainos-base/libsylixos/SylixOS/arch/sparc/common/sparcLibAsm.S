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
;** 文   件   名: sparcLibAsm.S
;**
;** 创   建   人: Xu.Guizhou (徐贵洲)
;**
;** 文件创建日期: 2017 年 05 月 15 日
;**
;** 描        述: SPARC 体系构架内部库.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>

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
    EXPORT_LABEL(archReboot)
    EXPORT_LABEL(archBogoMipsLoop)
    EXPORT_LABEL(archPsrGet)

    WEAK(archIntDisable)
    WEAK(archIntEnable)
    WEAK(archIntEnableForce)

;/*********************************************************************************************************
;  SPARC 获得 PSR
;*********************************************************************************************************/

FUNC_DEF(archPsrGet)
    RETL
    RD      %psr , %o0
    FUNC_END(archPsrGet)

;/*********************************************************************************************************
;  SPARC 开关闭总中断
;*********************************************************************************************************/

FUNC_DEF(archIntDisable)
    RD      %psr , %o0
    OR      %o0  , PSR_PIL , %o1                                        ;/*  加入 PSR_PIL 去关中断       */
    WR      %o1  , %psr
    NOP
    NOP
    NOP
    RETL
    NOP
    FUNC_END(archIntDisable)

FUNC_DEF(archIntEnable)
    ANDCC   %o0  , PSR_PIL , %g0
    BZ      archIntEnableForce
    NOP
    RETL
    NOP
    FUNC_END(archIntEnable)

FUNC_DEF(archIntEnableForce)
    RD      %psr , %o0
    ANDN    %o0  , PSR_PIL , %o0                                        ;/*  清除 PSR_PIL 去开中断       */
    WR      %o0  , %psr
    NOP
    NOP
    NOP
    RETL
    NOP
    FUNC_END(archIntEnableForce)
    
;/*********************************************************************************************************
;  注意: setjmp 与 longjmp 上下文结构与线程上下文结构不同
;  来源于 glibc
;*********************************************************************************************************/

#define JB_SP               0
#define JB_FP               1
#define JB_PC               2

#define ENV(base, reg)      [%base + (reg * 4)]

#define LOC(label)          label

;/*********************************************************************************************************
;  __sigjmp_save
;*********************************************************************************************************/

FUNC_DEF(__sigjmp_save)
    MOV     %g0 , %o0
    RETL
    NOP
FUNC_END(__sigjmp_save)

;/*********************************************************************************************************
;  sigsetjmp (参数为 jmp_buf, mask_saved)
;*********************************************************************************************************/

FUNC_DEF(sigsetjmp)
    ;/*
    ; * Save our PC, SP and FP.  Save the signal mask if requested with
    ; * a tail-call for simplicity; it always returns zero.
    ; */
    SPARC_FLUSH_REG_WINDOWS()

#ifdef PTR_MANGLE
    PTR_MANGLE(%g1  , %o7 , %g4)
    PTR_MANGLE2(%g2 , %sp , %g4)
    PTR_MANGLE2(%g3 , %fp , %g4)
    ST      %g1 , [%o0 + (JB_PC * 4)]
    ST      %g2 , [%o0 + (JB_SP * 4)]
    ST      %g3 , [%o0 + (JB_FP * 4)]
#else
    ST      %o7 , [%o0 + (JB_PC * 4)]
    ST      %sp , [%o0 + (JB_SP * 4)]
    ST      %fp , [%o0 + (JB_FP * 4)]
#endif

    MOV     %o7 , %g1
    CALL    __sigjmp_save
    MOV     %g1 , %o7
FUNC_END(sigsetjmp)

;/*********************************************************************************************************
;  setjmp (参数为 jmp_buf)
;*********************************************************************************************************/

FUNC_DEF(setjmp)
    B       sigsetjmp
    MOV     %g0 , %o1
FUNC_END(setjmp)
    
;/*********************************************************************************************************
;  siglongjmp (参数为 jmp_buf, retval)
;*********************************************************************************************************/

FUNC_DEF(siglongjmp)
    B       longjmp
    NOP
FUNC_END(setjmp)

;/*********************************************************************************************************
;  longjmp (参数为 jmp_buf, retval)
;*********************************************************************************************************/

FUNC_DEF(longjmp)
    ;/*
    ; * Store our arguments in global registers so we can still
    ; * use them while unwinding frames and their register windows.
    ; */

    LD      ENV(o0 , JB_FP) , %g3   ;/*  Cache target FP in register %g3.                                */
#ifdef PTR_DEMANGLE
    PTR_DEMANGLE(%g3 , %g3 , %g4)
#endif
    MOV     %o0 , %g1               ;/*  ENV in %g1                                                      */
    ORCC    %o1 , %g0 , %g2         ;/*  VAL in %g2                                                      */
    BE,a    0f                      ;/*  Branch if zero; else skip delay slot.                           */
    MOV     1 , %g2                 ;/*  Delay slot only hit if zero: VAL = 1.                           */
0:
    XOR     %fp , %g3  , %o0
    ADD     %fp , 512  , %o1
    ANDNCC  %o0 , 4095 , %o0
    BNE     LOC(thread)
    CMP     %o1 , %g3
    BL      LOC(thread)

    ;/*
    ; * Now we will loop, unwinding the register windows up the stack
    ; * until the restored %fp value matches the target value in %g3.
    ; */

LOC(loop):
    CMP     %fp , %g3               ;/*  Have we reached the target frame?                               */
    BL,a    LOC(loop)               ;/*  Loop while current fp is below target.                          */
    RESTORE                         ;/*  Unwind register window in delay slot.                           */
    BE,a    LOC(found)              ;/*  Better have hit it exactly.                                     */
    LD      ENV(g1 , JB_SP) , %o0   ;/*  Delay slot: extract target SP.                                  */

LOC(thread):
    SAVE    %sp , -96 , %sp
    ;/*
    ; * Do a "flush register windows trap".  The trap handler in the
    ; * kernel writes all the register windows to their stack slots, and
    ; * marks them all as invalid (needing to be sucked up from the
    ; * stack when used).  This ensures that all information needed to
    ; * unwind to these callers is in memory, not in the register
    ; * windows.
    ; */
    SPARC_FLUSH_REG_WINDOWS()
#ifdef PTR_DEMANGLE
    LD      ENV(g1 , JB_PC) , %g5   ;/*  Set return PC.                                                  */
    LD      ENV(g1 , JB_SP) , %g1   ;/*  Set saved SP on restore below.                                  */
    PTR_DEMANGLE2(%i7 , %g5 , %g4)
    PTR_DEMANGLE2(%fp , %g1 , %g4)
#else
    LD      ENV(g1 , JB_PC) , %i7   ;/*  Set return PC.                                                  */
    LD      ENV(g1 , JB_SP) , %fp   ;/*  Set saved SP on restore below.                                  */
#endif
    JMP     %i7 + 8
    RESTORE %g2 , 0 , %o0           ;/*  Restore values from above register frame.                       */

LOC(found):
    ;/*
    ; * We have unwound register windows so %fp matches the target.
    ; */
#ifdef PTR_DEMANGLE
    PTR_DEMANGLE2(%sp , %o0 , %g4)
#else
    MOV     %o0 , %sp               ;/*  OK, install new SP.                                             */
#endif

LOC(sp_ok):
    LD      ENV(g1 , JB_PC) , %o0   ;/*  Extract target return PC.                                       */
#ifdef PTR_DEMANGLE
    PTR_DEMANGLE2(%o0 , %o0 , %g4)
#endif
    JMP     %o0 + 8                 ;/*  Return there.                                                   */
    MOV     %g2 , %o0               ;/*  Delay slot: set return value.                                   */
FUNC_END(longjmp)
    
;/*********************************************************************************************************
;  系统重启
;*********************************************************************************************************/

FUNC_DEF(archReboot)
    BA      bspReboot
    NOP
FUNC_END(archReboot)
    
;/*********************************************************************************************************
;  Bogo 循环(三条指令)
;*********************************************************************************************************/

FUNC_DEF(archBogoMipsLoop)
LINE_LABEL(bogo_loop)
    SUBCC   %o0 , 0x1 , %o0
    BNE     bogo_loop
    NOP
    RETL
    NOP
    FUNC_END(archBogoMipsLoop)

    FILE_END()

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
