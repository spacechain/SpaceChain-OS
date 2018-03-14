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
;** 文   件   名: x64IoAsm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2017 年 06 月 06 日
;**
;** 描        述: x86-64 体系构架 IO 相关函数.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/kernel/kernel_cfg.h>

    FILE_BEGIN()

;/*********************************************************************************************************
;  UINT8  in8( addr_t  ulAddr);
;  UINT16 in16(addr_t  ulAddr);
;  UINT32 in32(addr_t  ulAddr);
;
;  VOID out8( UINT8   ucData,  addr_t  ulAddr);
;  VOID out16(UINT16  usData,  addr_t  ulAddr);
;  VOID out32(UINT32  uiData,  addr_t  ulAddr);
;
;  VOID ins8( addr_t  ulAddr, PVOID  pvBuffer, size_t  stCount);
;  VOID ins16(addr_t  ulAddr, PVOID  pvBuffer, size_t  stCount);
;  VOID ins32(addr_t  ulAddr, PVOID  pvBuffer, size_t  stCount);
;
;  VOID outs8( addr_t  ulAddr, PVOID  pvBuffer, size_t  stCount);
;  VOID outs16(addr_t  ulAddr, PVOID  pvBuffer, size_t  stCount);
;  VOID outs32(addr_t  ulAddr, PVOID  pvBuffer, size_t  stCount);
;*********************************************************************************************************/

    EXPORT_LABEL(in8)
    EXPORT_LABEL(in16)
    EXPORT_LABEL(in32)

    EXPORT_LABEL(out8)
    EXPORT_LABEL(out16)
    EXPORT_LABEL(out32)

    EXPORT_LABEL(ins8)
    EXPORT_LABEL(ins16)
    EXPORT_LABEL(ins32)

    EXPORT_LABEL(outs8)
    EXPORT_LABEL(outs16)
    EXPORT_LABEL(outs32)

;/*********************************************************************************************************
;  UINT8  in8(addr_t  ulAddr);
;*********************************************************************************************************/

FUNC_DEF(in8)
    MOV     X86_64_ARG0 , %RDX
    MOV     $0 , %RAX
    INB     %DX , %AL
    RET
    FUNC_END(in8)

;/*********************************************************************************************************
;  UINT16 in16(addr_t  ulAddr);
;*********************************************************************************************************/

FUNC_DEF(in16)
    MOV     X86_64_ARG0 , %RDX
    MOV     $0 , %RAX
    INW     %DX , %AX
    RET
    FUNC_END(in16)

;/*********************************************************************************************************
;  UINT32 in32(addr_t  ulAddr);
;*********************************************************************************************************/

FUNC_DEF(in32)
    MOV     X86_64_ARG0 , %RDX
    MOV     $0 , %RAX
    INL     %DX , %EAX
    RET
    FUNC_END(in32)

;/*********************************************************************************************************
;  VOID out8(UINT8   ucData,  addr_t  ulAddr);
;*********************************************************************************************************/

FUNC_DEF(out8)
    MOV     X86_64_ARG0 , %RAX
    MOV     X86_64_ARG1 , %RDX
    OUTB    %AL , %DX
    RET
    FUNC_END(out8)

;/*********************************************************************************************************
;  VOID out16(UINT16   usData,  addr_t  ulAddr);
;*********************************************************************************************************/

FUNC_DEF(out16)
    MOV     X86_64_ARG0 , %RAX
    MOV     X86_64_ARG1 , %RDX
    OUTW    %AX , %DX
    RET
    FUNC_END(out16)

;/*********************************************************************************************************
;  VOID out32(UINT32   uiData,  addr_t  ulAddr);
;*********************************************************************************************************/

FUNC_DEF(out32)
    MOV     X86_64_ARG0 , %RAX
    MOV     X86_64_ARG1 , %RDX
    OUTL    %EAX , %DX
    RET
    FUNC_END(out32)

;/*********************************************************************************************************
;  VOID ins8(addr_t  ulAddr, PVOID  pvBuffer, size_t  stCount);
;*********************************************************************************************************/

FUNC_DEF(ins8)
    ;/*
    ; * 不要调整取参顺序! 下同
    ; */
    MOV     X86_64_ARG2 , %RCX                                          ;/*  %RCX = stCount              */
    MOV     X86_64_ARG0 , %RDX                                          ;/*  %RDX = ulAddr               */
    MOV     X86_64_ARG1 , %RDI                                          ;/*  %RDI = pvBuffer             */
    CLD
    REP
    INSB    %DX  , (%RDI)
    RET
    FUNC_END(ins8)

;/*********************************************************************************************************
;  VOID ins16(addr_t  ulAddr, PVOID  pvBuffer, size_t  stCount);
;*********************************************************************************************************/

FUNC_DEF(ins16)
    MOV     X86_64_ARG2 , %RCX                                          ;/*  %RCX = stCount              */
    MOV     X86_64_ARG0 , %RDX                                          ;/*  %RDX = ulAddr               */
    MOV     X86_64_ARG1 , %RDI                                          ;/*  %RDI = pvBuffer             */
    CLD
    REP
    INSW    %DX  , (%RDI)
    RET
    FUNC_END(ins16)

;/*********************************************************************************************************
;  VOID ins32(addr_t  ulAddr, PVOID  pvBuffer, size_t  stCount);
;*********************************************************************************************************/

FUNC_DEF(ins32)
    MOV     X86_64_ARG2 , %RCX                                          ;/*  %RCX = stCount              */
    MOV     X86_64_ARG0 , %RDX                                          ;/*  %RDX = ulAddr               */
    MOV     X86_64_ARG1 , %RDI                                          ;/*  %RDI = pvBuffer             */
    CLD
    REP
    INSL    %DX  , (%RDI)
    RET
    FUNC_END(ins32)

;/*********************************************************************************************************
;  VOID outs8(addr_t  ulAddr, PVOID  pvBuffer, size_t  stCount);
;*********************************************************************************************************/

FUNC_DEF(outs8)
    MOV     X86_64_ARG2 , %RCX                                          ;/*  %RCX = stCount              */
    MOV     X86_64_ARG0 , %RDX                                          ;/*  %RDX = ulAddr               */
                                                                        ;/*  %RSI = pvBuffer             */
    CLD
    REP
    OUTSB   (%RSI) , %DX
    RET
    FUNC_END(outs8)

;/*********************************************************************************************************
;  VOID outs16(addr_t  ulAddr, PVOID  pvBuffer, size_t  stCount);
;*********************************************************************************************************/

FUNC_DEF(outs16)
    MOV     X86_64_ARG2 , %RCX                                          ;/*  %RCX = stCount              */
    MOV     X86_64_ARG0 , %RDX                                          ;/*  %RDX = ulAddr               */
                                                                        ;/*  %RSI = pvBuffer             */
    CLD
    REP
    OUTSW   (%RSI) , %DX
    RET
    FUNC_END(outs16)

;/*********************************************************************************************************
;  VOID outs32(addr_t  ulAddr, PVOID  pvBuffer, size_t  stCount);
;*********************************************************************************************************/

FUNC_DEF(outs32)
    MOV     X86_64_ARG2 , %RCX                                          ;/*  %RCX = stCount              */
    MOV     X86_64_ARG0 , %RDX                                          ;/*  %RDX = ulAddr               */
                                                                        ;/*  %RSI = pvBuffer             */
    CLD
    REP
    OUTSL   (%RSI) , %DX
    RET
    FUNC_END(outs32)

    FILE_END()

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
