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
;** 文   件   名: x64CacheAsm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2017 年 06 月 02 日
;**
;** 描        述: x86-64 体系构架 CACHE 驱动.
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
    MOV     %CR0 , %RAX
    MOV     $X86_CR0_CD_NOT , %RDX
    AND     %RDX , %RAX
    MOV     $X86_CR0_NW_NOT , %RDX
    AND     %RDX , %RAX
    MOV     %RAX , %CR0
    RET
    FUNC_END(x86CacheEnableHw)

;/*********************************************************************************************************
;  禁能 CACHE
;*********************************************************************************************************/

FUNC_DEF(x86CacheDisableHw)
    MOV     %CR0 , %RAX
    OR      $X86_CR0_CD , %RAX
    MOV     $X86_CR0_NW_NOT , %RDX
    AND     %RDX , %RAX
    MOV     %RAX , %CR0
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
    TEST    X86_64_ARG1 , X86_64_ARG1                                   ;/*  Return if (stBytes == 0)    */
    JZ      __cachePen4Ret

    MOV     X86_64_ARG1 , %RCX                                          ;/*  Use %RCX for scrach         */
    AND     $~X86_CLFLUSH_MAX_BYTES , %RCX                              ;/*  WBINVD if (stBytes > MAX)   */
    JNZ     x86CacheClearX86Hw

    MOV     X86_64_ARG0 , %R8
    MOV     X86_64_ARG1 , %RDI
    MOV     %R8 , %RSI                                                  ;/*  Keep a copy of first arg    */

    ;/*
     ;* stBytes += ((addr_t)pvAdrs % _G_stX86CacheFlushBytes);
     ;*/
    MOV     _G_stX86CacheFlushBytes , %RDX                              ;/*  Get stFlushBytes in RDX     */
    SUB     $1 , %RDX                                                   ;/*  Create lowerbit mask        */
    AND     %RDX , %R8                                                  ;/*  Get the lowerbit stRem      */
    ADD     %R8  , %RDI                                                 ;/*  Add the stRem to stBytes    */

    ;/*
     ;* if ((stRem = stBytes % _G_stX86CacheFlushBytes) != 0)
     ;*     stBytes += (_G_stX86CacheFlushBytes - stRem);
     ;*/
    MOV     %RDI , %RCX                                                 ;/*  Get stBytes in RCX          */
    AND     %RDX , %RCX                                                 ;/*  Get the lowerbit stRem      */
    JZ      __cachePen4Flush0                                           ;/*  Skip if (stRem == 0)        */
    SUB     %RCX , %RDI                                                 ;/*  Sub the stRem from stBytes  */
    ADD     _G_stX86CacheFlushBytes , %RDI                              ;/*  Add the flushBytes          */

LINE_LABEL(__cachePen4Flush0)
    ;/*
     ;* ulLoopCount = stBytes / _G_stX86CacheFlushBytes;
     ;*/
    BSF     _G_stX86CacheFlushBytes , %RCX                              ;/*  Find the LSB, RCX=[0-63]    */
    SHR     %CL , %RDI                                                  ;/*  Shift right RCX bit         */

    ;/*
     ;* pvAdrs -= ((addr_t)pvAdrs % _G_stX86CacheFlushBytes);
     ;*/
    MOV     %RSI , %R8                                                  ;/*  Get pvAdrs in R8            */
    XOR     $0xffffffffffffffff , %RDX                                  ;/*  Create the upperbit mask    */
    AND     %RDX , %R8                                                  ;/*  Get the upperbit            */

    ;/*
     ;* do {
     ;*   CLFLUSH (pvAdrs);
     ;*   (addr_t)pvAdrs += _G_stX86CacheFlushBytes;
     ;* } while (ulLoopCount--);
     ;*/
    MOV     %RDI , %RCX                                                 ;/*  Set the ulLoopCount         */

LINE_LABEL(__cachePen4FlushLoop)
    CLFLUSH (%R8)                                                       ;/*  Flush the line              */
    ADD     _G_stX86CacheFlushBytes , %R8                               ;/*  (addr_t)pvAdrs += flushBytes*/
    LOOP    __cachePen4FlushLoop                                        ;/*  Loop if (--RCX != 0)        */

LINE_LABEL(__cachePen4Ret)
    RET
    FUNC_END(x86CacheFlushPen4Hw)

    FILE_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
