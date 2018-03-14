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
;** 文   件   名: x86IoAsm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2016 年 07 月 04 日
;**
;** 描        述: x86 体系构架 IO 相关函数.
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
    MOVL    X86_SP_ARG1(%ESP) , %EDX
    MOVL    $0 , %EAX
    INB     %DX , %AL
    RET
    FUNC_END(in8)

;/*********************************************************************************************************
;  UINT16 in16(addr_t  ulAddr);
;*********************************************************************************************************/

FUNC_DEF(in16)
    MOVL    X86_SP_ARG1(%ESP) , %EDX
    MOVL    $0 , %EAX
    INW     %DX , %AX
    RET
    FUNC_END(in16)

;/*********************************************************************************************************
;  UINT32 in32(addr_t  ulAddr);
;*********************************************************************************************************/

FUNC_DEF(in32)
    MOVL    X86_SP_ARG1(%ESP) , %EDX
    INL     %DX , %EAX
    RET
    FUNC_END(in32)

;/*********************************************************************************************************
;  VOID out8(UINT8   ucData,  addr_t  ulAddr);
;*********************************************************************************************************/

FUNC_DEF(out8)
    MOVL    X86_SP_ARG1(%ESP) , %EAX
    MOVL    X86_SP_ARG2(%ESP) , %EDX
    OUTB    %AL , %DX
    RET
    FUNC_END(out8)

;/*********************************************************************************************************
;  VOID out16(UINT16   usData,  addr_t  ulAddr);
;*********************************************************************************************************/

FUNC_DEF(out16)
    MOVL    X86_SP_ARG1(%ESP) , %EAX
    MOVL    X86_SP_ARG2(%ESP) , %EDX
    OUTW    %AX , %DX
    RET
    FUNC_END(out16)

;/*********************************************************************************************************
;  VOID out32(UINT32   uiData,  addr_t  ulAddr);
;*********************************************************************************************************/

FUNC_DEF(out32)
    MOVL    X86_SP_ARG1(%ESP) , %EAX
    MOVL    X86_SP_ARG2(%ESP) , %EDX
    OUTL    %EAX , %DX
    RET
    FUNC_END(out32)

;/*********************************************************************************************************
;  VOID ins8(addr_t  ulAddr, PVOID  pvBuffer, size_t  stCount);
;*********************************************************************************************************/

FUNC_DEF(ins8)
    PUSHL   %EDI
    MOVL    (X86_SP_ARG1 + 4)(%ESP) , %EDX
    MOVL    (X86_SP_ARG2 + 4)(%ESP) , %EDI
    MOVL    (X86_SP_ARG3 + 4)(%ESP) , %ECX
    CLD
    REP
    INSB    %DX , (%EDI)
    POPL    %EDI
    RET
    FUNC_END(ins8)

;/*********************************************************************************************************
;  VOID ins16(addr_t  ulAddr, PVOID  pvBuffer, size_t  stCount);
;*********************************************************************************************************/

FUNC_DEF(ins16)
    PUSHL   %EDI
    MOVL    (X86_SP_ARG1 + 4)(%ESP) , %EDX
    MOVL    (X86_SP_ARG2 + 4)(%ESP) , %EDI
    MOVL    (X86_SP_ARG3 + 4)(%ESP) , %ECX
    CLD
    REP
    INSW    %DX , (%EDI)
    POPL    %EDI
    RET
    FUNC_END(ins16)

;/*********************************************************************************************************
;  VOID ins32(addr_t  ulAddr, PVOID  pvBuffer, size_t  stCount);
;*********************************************************************************************************/

FUNC_DEF(ins32)
    PUSHL   %EDI
    MOVL    (X86_SP_ARG1 + 4)(%ESP) , %EDX
    MOVL    (X86_SP_ARG2 + 4)(%ESP) , %EDI
    MOVL    (X86_SP_ARG3 + 4)(%ESP) , %ECX
    CLD
    REP
    INSL    %DX , (%EDI)
    POPL    %EDI
    RET
    FUNC_END(ins32)

;/*********************************************************************************************************
;  VOID outs8(addr_t  ulAddr, PVOID  pvBuffer, size_t  stCount);
;*********************************************************************************************************/

FUNC_DEF(outs8)
    PUSHL   %ESI
    MOVL    (X86_SP_ARG1 + 4)(%ESP) , %EDX
    MOVL    (X86_SP_ARG2 + 4)(%ESP) , %ESI
    MOVL    (X86_SP_ARG3 + 4)(%ESP) , %ECX
    CLD
    REP
    OUTSB   (%ESI) , %DX
    POPL    %ESI
    RET
    FUNC_END(outs8)

;/*********************************************************************************************************
;  VOID outs16(addr_t  ulAddr, PVOID  pvBuffer, size_t  stCount);
;*********************************************************************************************************/

FUNC_DEF(outs16)
    PUSHL   %ESI
    MOVL    (X86_SP_ARG1 + 4)(%ESP) , %EDX
    MOVL    (X86_SP_ARG2 + 4)(%ESP) , %ESI
    MOVL    (X86_SP_ARG3 + 4)(%ESP) , %ECX
    CLD
    REP
    OUTSW   (%ESI) , %DX
    POPL    %ESI
    RET
    FUNC_END(outs16)

;/*********************************************************************************************************
;  VOID outs32(addr_t  ulAddr, PVOID  pvBuffer, size_t  stCount);
;*********************************************************************************************************/

FUNC_DEF(outs32)
    PUSHL   %ESI
    MOVL    (X86_SP_ARG1 + 4)(%ESP) , %EDX
    MOVL    (X86_SP_ARG2 + 4)(%ESP) , %ESI
    MOVL    (X86_SP_ARG3 + 4)(%ESP) , %ECX
    CLD
    REP
    OUTSL   (%ESI) , %DX
    POPL    %ESI
    RET
    FUNC_END(outs32)

    FILE_END()

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
