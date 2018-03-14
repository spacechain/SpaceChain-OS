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
;** 文   件   名: x86CacheAsm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2016 年 07 月 18 日
;**
;** 描        述: x86 体系构架 CACHE 驱动.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/kernel/cache_cfg.h>

#if LW_CFG_CACHE_EN > 0

    FILE_BEGIN()
    
    EXPORT_LABEL(x86CacheResetHw)
    EXPORT_LABEL(x86CacheEnableHw)
    EXPORT_LABEL(x86CacheDisableHw)

    EXPORT_LABEL(x86CacheFlushX86Hw)
    EXPORT_LABEL(x86CacheClearX86Hw)

    EXPORT_LABEL(x86CacheFlushPen4Hw)
    EXPORT_LABEL(x86CacheClearPen4Hw)

    IMPORT_LABEL(_G_stX86CacheFlushBytes)

;/*********************************************************************************************************
;  复位 CACHE
;*********************************************************************************************************/

FUNC_DEF(x86CacheResetHw)
    WBINVD                                                              ;/*  回写并无效                  */
    RET
    FUNC_END(x86CacheResetHw)

;/*********************************************************************************************************
;  使能 CACHE
;*********************************************************************************************************/

FUNC_DEF(x86CacheEnableHw)
    MOVL    %CR0 , %EAX
    ANDL    $X86_CR0_CD_NOT , %EAX
    ANDL    $X86_CR0_NW_NOT , %EAX
    MOVL    %EAX , %CR0
    RET
    FUNC_END(x86CacheEnableHw)

;/*********************************************************************************************************
;  禁能 CACHE
;*********************************************************************************************************/

FUNC_DEF(x86CacheDisableHw)
    MOVL    %CR0 , %EAX
    ORL     $X86_CR0_CD , %EAX
    ANDL    $X86_CR0_NW_NOT , %EAX
    MOVL    %EAX , %CR0
    WBINVD                                                              ;/*  回写并无效                  */
    RET
    FUNC_END(x86CacheDisableHw)

;/*********************************************************************************************************
;  X86 回写无效 CACHE
;*********************************************************************************************************/

FUNC_DEF(x86CacheFlushX86Hw)
FUNC_DEF(x86CacheClearX86Hw)
    WBINVD                                                              ;/*  回写并无效                  */
    RET
    FUNC_END(x86CacheFlushX86Hw)

;/*********************************************************************************************************
;  Pentium4 回写无效指定范围的 CACHE
;
;  VOID  x86CacheFlushPen4Hw(PVOID  pvAdrs, size_t  stBytes);
;  VOID  x86CacheClearPen4Hw(PVOID  pvAdrs, size_t  stBytes);
;*********************************************************************************************************/

FUNC_DEF(x86CacheFlushPen4Hw)
FUNC_DEF(x86CacheClearPen4Hw)
    MOVL    X86_SP_ARG2(%ESP) , %EDX                                    ;/*  Get stBytes in EDX          */
    TESTL   %EDX , %EDX                                                 ;/*  Return if (stBytes == 0)    */
    JZ      __cachePen4Ret

    ANDL    $(~X86_CLFLUSH_MAX_BYTES) , %EDX                            ;/*  WBINVD if (stBytes > MAX)   */
    JNZ     x86CacheClearX86Hw

    MOVL    X86_SP_ARG1(%ESP) , %EAX                                    ;/*  Get pvAdrs  in EAX          */
    MOVL    X86_SP_ARG2(%ESP) , %EDX                                    ;/*  Get stBytes in EDX          */

    PUSHL   %EDI                                                        ;/*  Save EDI                    */
    MOVL    %EDX , %EDI                                                 ;/*  Get stBytes in EDI          */

    ;/*
     ;* stBytes += ((addr_t)pvAdrs % _G_stX86CacheFlushBytes);
     ;*/
    MOVL    _G_stX86CacheFlushBytes , %EDX                              ;/*  Get stFlushBytes in EDX     */
    SUBL    $1 , %EDX                                                   ;/*  Create lowerbit mask        */
    ANDL    %EDX , %EAX                                                 ;/*  Get the lowerbit stRem      */
    ADDL    %EAX , %EDI                                                 ;/*  Add the stRem to stBytes    */

    ;/*
     ;* if ((stRem = stBytes % _G_stX86CacheFlushBytes) != 0)
     ;*     stBytes += (_G_stX86CacheFlushBytes - stRem);
     ;*/
    MOVL    %EDI , %ECX                                                 ;/*  Get stBytes in ECX          */
    ANDL    %EDX , %ECX                                                 ;/*  Get the lowerbit stRem      */
    JZ      __cachePen4Flush0                                           ;/*  Skip if (stRem == 0)        */
    SUBL    %ECX , %EDI                                                 ;/*  Sub the stRem from stBytes  */
    ADDL    _G_stX86CacheFlushBytes , %EDI                              ;/*  Add the flushBytes          */

LINE_LABEL(__cachePen4Flush0)
    ;/*
     ;* ulLoopCount = stBytes / _G_stX86CacheFlushBytes;
     ;*/
    BSFL    _G_stX86CacheFlushBytes , %ECX                              ;/*  Find the LSB, ECX=[0-31]    */
    SHRL    %CL , %EDI                                                  ;/*  Shift right ECX bit         */

    ;/*
     ;* pvAdrs -= ((addr_t)pvAdrs % _G_stX86CacheFlushBytes);
     ;*/
    MOVL    (X86_SP_ARG1 + 4)(%ESP) , %EAX                              ;/*  Get pvAdrs in EAX           */
    XORL    $0xffffffff , %EDX                                          ;/*  Create the upperbit mask    */
    ANDL    %EDX , %EAX                                                 ;/*  Get the upperbit            */

    ;/*
     ;* do {
     ;*   CLFLUSH (pvAdrs);
     ;*   (addr_t)pvAdrs += _G_stX86CacheFlushBytes;
     ;* } while (ulLoopCount--);
     ;*/
    MOVL    %EDI , %ECX                                                 ;/*  Set the ulLoopCount         */

LINE_LABEL(__cachePen4FlushLoop)
    CLFLUSH (%EAX)                                                      ;/*  Flush the line              */
    ADDL    _G_stX86CacheFlushBytes , %EAX                              ;/*  (addr_t)pvAdrs += flushBytes*/
    LOOP    __cachePen4FlushLoop                                        ;/*  Loop if (--ECX != 0)        */

    POPL    %EDI                                                        ;/*  Restore EDI                 */

LINE_LABEL(__cachePen4Ret)
    RET
    FUNC_END(x86CacheFlushPen4Hw)

    FILE_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
