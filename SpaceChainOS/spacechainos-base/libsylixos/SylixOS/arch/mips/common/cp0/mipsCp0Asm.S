;/*********************************************************************************************************
;**
;**                                    中国软件开源组织
;**
;**                                   嵌入式实时操作系统
;**
;**                                SylixOS(TM)  LW : long wing
;**
;**                               Copyright All Rights Reserved
;**
;**--------------文件信息--------------------------------------------------------------------------------
;**
;** 文   件   名: mipsCp0Asm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2016 年 06 月 08 日
;**
;** 描        述: MIPS CP0 函数库.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/kernel/kernel_cfg.h>
#include "arch/mips/arch_def.h"

    FILE_BEGIN()

    EXPORT_LABEL(mipsCp0StatusRead)
    EXPORT_LABEL(mipsCp0StatusWrite)
    EXPORT_LABEL(mipsCp0CauseRead)
    EXPORT_LABEL(mipsCp0CauseWrite)
    EXPORT_LABEL(mipsCp0EPCRead)
    EXPORT_LABEL(mipsCp0EPCWrite)
    EXPORT_LABEL(mipsCp0BadVAddrRead)
    EXPORT_LABEL(mipsCp0BadVAddrWrite)
    EXPORT_LABEL(mipsCp0CountRead)
    EXPORT_LABEL(mipsCp0CountWrite)
    EXPORT_LABEL(mipsCp0CompareRead)
    EXPORT_LABEL(mipsCp0CompareWrite)
    EXPORT_LABEL(mipsCp0PRIdRead)
    EXPORT_LABEL(mipsCp0PRIdWrite)
    EXPORT_LABEL(mipsCp0ConfigRead)
    EXPORT_LABEL(mipsCp0ConfigWrite)
    EXPORT_LABEL(mipsCp0Config1Read)
    EXPORT_LABEL(mipsCp0Config1Write)
    EXPORT_LABEL(mipsCp0Config2Read)
    EXPORT_LABEL(mipsCp0Config2Write)
    EXPORT_LABEL(mipsCp0Config3Read)
    EXPORT_LABEL(mipsCp0Config3Write)
    EXPORT_LABEL(mipsCp0EBaseRead)
    EXPORT_LABEL(mipsCp0EBaseWrite)
    EXPORT_LABEL(mipsCp0IntCtlRead)
    EXPORT_LABEL(mipsCp0IntCtlWrite)
    EXPORT_LABEL(mipsCp0LLAddrRead)
    EXPORT_LABEL(mipsCp0LLAddrWrite)
    EXPORT_LABEL(mipsCp0ECCRead)
    EXPORT_LABEL(mipsCp0ECCWrite)
    EXPORT_LABEL(mipsCp0CacheErrRead)
    EXPORT_LABEL(mipsCp0CacheErrWrite)
    EXPORT_LABEL(mipsCp0ERRPCRead)
    EXPORT_LABEL(mipsCp0ERRPCWrite)
    EXPORT_LABEL(mipsCp0IndexRead)
    EXPORT_LABEL(mipsCp0IndexWrite)
    EXPORT_LABEL(mipsCp0WiredRead)
    EXPORT_LABEL(mipsCp0WiredWrite)
    EXPORT_LABEL(mipsCp0RandomRead)
    EXPORT_LABEL(mipsCp0RandomWrite)
    EXPORT_LABEL(mipsCp0EntryLo0Read)
    EXPORT_LABEL(mipsCp0EntryLo0Write)
    EXPORT_LABEL(mipsCp0EntryLo1Read)
    EXPORT_LABEL(mipsCp0EntryLo1Write)
    EXPORT_LABEL(mipsCp0ContextRead)
    EXPORT_LABEL(mipsCp0ContextWrite)
    EXPORT_LABEL(mipsCp0PageMaskRead)
    EXPORT_LABEL(mipsCp0PageMaskWrite)
    EXPORT_LABEL(mipsCp0EntryHiRead)
    EXPORT_LABEL(mipsCp0EntryHiWrite)
    EXPORT_LABEL(mipsCp0TagLoRead)
    EXPORT_LABEL(mipsCp0TagLoWrite)
    EXPORT_LABEL(mipsCp0TagHiRead)
    EXPORT_LABEL(mipsCp0TagHiWrite)
    EXPORT_LABEL(mipsCp0DiagRead)
    EXPORT_LABEL(mipsCp0DiagWrite)
    EXPORT_LABEL(mipsCp0PageGrainRead)
    EXPORT_LABEL(mipsCp0PageGrainWrite)
    EXPORT_LABEL(mipsCp0GSConfigRead)
    EXPORT_LABEL(mipsCp0GSConfigWrite)

;/*********************************************************************************************************
;  CP0 register read
;*********************************************************************************************************/

MACRO_DEF(mipsCp0RegRead reg)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile

    MFC0    V0 , \reg
    EHB
    JR      RA
    NOP

    .set    pop
    MACRO_END()

;/*********************************************************************************************************
;  CP0 register write
;*********************************************************************************************************/

MACRO_DEF(mipsCp0RegWrite reg)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile

    MTC0    A0 , \reg
    EHB
    JR      RA
    NOP

    .set    pop
    MACRO_END()

;/*********************************************************************************************************
;  CP0_STATUS
;*********************************************************************************************************/

FUNC_DEF(mipsCp0StatusRead)
    mipsCp0RegRead     CP0_STATUS
    FUNC_END(mipsCp0StatusRead)

FUNC_DEF(mipsCp0StatusWrite)
    mipsCp0RegWrite    CP0_STATUS
    FUNC_END(mipsCp0StatusWrite)

;/*********************************************************************************************************
;  CP0_CAUSE
;*********************************************************************************************************/

FUNC_DEF(mipsCp0CauseRead)
    mipsCp0RegRead     CP0_CAUSE
    FUNC_END(mipsCp0CauseRead)

FUNC_DEF(mipsCp0CauseWrite)
    mipsCp0RegWrite    CP0_CAUSE
    FUNC_END(mipsCp0CauseWrite)

;/*********************************************************************************************************
;  CP0_ERRPC
;*********************************************************************************************************/

FUNC_DEF(mipsCp0EPCRead)
    mipsCp0RegRead     CP0_ERRPC
    FUNC_END(mipsCp0EPCRead)

FUNC_DEF(mipsCp0EPCWrite)
    mipsCp0RegWrite    CP0_ERRPC
    FUNC_END(mipsCp0EPCWrite)

;/*********************************************************************************************************
;  CP0_BADVADDR
;*********************************************************************************************************/

FUNC_DEF(mipsCp0BadVAddrRead)
    mipsCp0RegRead     CP0_BADVADDR
    FUNC_END(mipsCp0BadVAddrRead)

FUNC_DEF(mipsCp0BadVAddrWrite)
    mipsCp0RegWrite    CP0_BADVADDR
    FUNC_END(mipsCp0BadVAddrWrite)

;/*********************************************************************************************************
;  CP0_COUNT
;*********************************************************************************************************/

FUNC_DEF(mipsCp0CountRead)
    mipsCp0RegRead     CP0_COUNT
    FUNC_END(mipsCp0CountRead)

FUNC_DEF(mipsCp0CountWrite)
    mipsCp0RegWrite    CP0_COUNT
    FUNC_END(mipsCp0CountWrite)

;/*********************************************************************************************************
;  CP0_COMPARE
;*********************************************************************************************************/

FUNC_DEF(mipsCp0CompareRead)
    mipsCp0RegRead     CP0_COMPARE
    FUNC_END(mipsCp0CompareRead)

FUNC_DEF(mipsCp0CompareWrite)
    mipsCp0RegWrite    CP0_COMPARE
    FUNC_END(mipsCp0CompareWrite)

;/*********************************************************************************************************
;  CP0_PRID
;*********************************************************************************************************/

FUNC_DEF(mipsCp0PRIdRead)
    mipsCp0RegRead     CP0_PRID
    FUNC_END(mipsCp0PRIdRead)

FUNC_DEF(mipsCp0PRIdWrite)
    mipsCp0RegWrite    CP0_PRID
    FUNC_END(mipsCp0PRIdWrite)

;/*********************************************************************************************************
;  CP0_CONFIG
;*********************************************************************************************************/

FUNC_DEF(mipsCp0ConfigRead)
    mipsCp0RegRead     CP0_CONFIG
    FUNC_END(mipsCp0ConfigRead)

FUNC_DEF(mipsCp0ConfigWrite)
    mipsCp0RegWrite    CP0_CONFIG
    FUNC_END(mipsCp0ConfigWrite)

;/*********************************************************************************************************
;  CP0_CONFIG1
;*********************************************************************************************************/

FUNC_DEF(mipsCp0Config1Read)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile

    MFC0    V0 , CP0_CONFIG1
    EHB
    JR      RA
    NOP

    .set    pop
    FUNC_END(mipsCp0Config1Read)

FUNC_DEF(mipsCp0Config1Write)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile

    MTC0    A0 , CP0_CONFIG1
    EHB
    JR      RA
    NOP

    .set    pop
    FUNC_END(mipsCp0Config1Write)

;/*********************************************************************************************************
;  CP0_CONFIG2
;*********************************************************************************************************/

FUNC_DEF(mipsCp0Config2Read)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile

    MFC0    V0 , CP0_CONFIG2
    EHB
    JR      RA
    NOP

    .set    pop
    FUNC_END(mipsCp0Config2Read)

FUNC_DEF(mipsCp0Config2Write)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile

    MTC0    A0 , CP0_CONFIG2
    EHB
    JR      RA
    NOP

    .set    pop
    FUNC_END(mipsCp0Config2Write)

;/*********************************************************************************************************
;  CP0_CONFIG3
;*********************************************************************************************************/

FUNC_DEF(mipsCp0Config3Read)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile

    MFC0    V0 , CP0_CONFIG3
    EHB
    JR      RA
    NOP

    .set    pop
    FUNC_END(mipsCp0Config3Read)

FUNC_DEF(mipsCp0Config3Write)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile

    MTC0    A0 , CP0_CONFIG3
    EHB
    JR      RA
    NOP

    .set    pop
    FUNC_END(mipsCp0Config3Write)

;/*********************************************************************************************************
;  CP0_EBASE
;*********************************************************************************************************/

FUNC_DEF(mipsCp0EBaseRead)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile

    MFC0    V0 , CP0_EBASE
    EHB
    JR      RA
    NOP

    .set    pop
    FUNC_END(mipsCp0EBaseRead)

FUNC_DEF(mipsCp0EBaseWrite)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile

    MTC0    A0 , CP0_EBASE
    EHB
    JR      RA
    NOP

    .set    pop
    FUNC_END(mipsCp0EBaseWrite)

;/*********************************************************************************************************
;  CP0_INTCTL
;*********************************************************************************************************/

FUNC_DEF(mipsCp0IntCtlRead)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile

    MFC0    V0 , CP0_INTCTL
    EHB
    JR      RA
    NOP

    .set    pop
    FUNC_END(mipsCp0IntCtlRead)

FUNC_DEF(mipsCp0IntCtlWrite)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile

    MTC0    A0 , CP0_INTCTL
    EHB
    JR      RA
    NOP

    .set    pop
    FUNC_END(mipsCp0IntCtlWrite)

;/*********************************************************************************************************
;  CP0_LLADDR
;*********************************************************************************************************/

FUNC_DEF(mipsCp0LLAddrRead)
    mipsCp0RegRead     CP0_LLADDR
    FUNC_END(mipsCp0LLAddrRead)

FUNC_DEF(mipsCp0LLAddrWrite)
    mipsCp0RegWrite    CP0_LLADDR
    FUNC_END(mipsCp0LLAddrWrite)

;/*********************************************************************************************************
;  CP0_ECC
;*********************************************************************************************************/

FUNC_DEF(mipsCp0ECCRead)
    mipsCp0RegRead     CP0_ECC
    FUNC_END(mipsCp0ECCRead)

FUNC_DEF(mipsCp0ECCWrite)
    mipsCp0RegWrite    CP0_ECC
    FUNC_END(mipsCp0ECCWrite)

;/*********************************************************************************************************
;  CP0_CACHEERR
;*********************************************************************************************************/

FUNC_DEF(mipsCp0CacheErrRead)
    mipsCp0RegRead     CP0_CACHEERR
    FUNC_END(mipsCp0CacheErrRead)

FUNC_DEF(mipsCp0CacheErrWrite)
    mipsCp0RegWrite    CP0_CACHEERR
    FUNC_END(mipsCp0CacheErrWrite)

;/*********************************************************************************************************
;  CP0_ERRPC
;*********************************************************************************************************/

FUNC_DEF(mipsCp0ERRPCRead)
    mipsCp0RegRead     CP0_ERRPC
    FUNC_END(mipsCp0ERRPCRead)

FUNC_DEF(mipsCp0ERRPCWrite)
    mipsCp0RegWrite    CP0_ERRPC
    FUNC_END(mipsCp0ERRPCWrite)

;/*********************************************************************************************************
;  CP0_INX
;*********************************************************************************************************/

FUNC_DEF(mipsCp0IndexRead)
    mipsCp0RegRead     CP0_INX
    FUNC_END(mipsCp0IndexRead)

FUNC_DEF(mipsCp0IndexWrite)
    mipsCp0RegWrite    CP0_INX
    FUNC_END(mipsCp0IndexWrite)

;/*********************************************************************************************************
;  CP0_WIRED
;*********************************************************************************************************/

FUNC_DEF(mipsCp0WiredRead)
    mipsCp0RegRead     CP0_WIRED
    FUNC_END(mipsCp0WiredRead)

FUNC_DEF(mipsCp0WiredWrite)
    mipsCp0RegWrite    CP0_WIRED
    FUNC_END(mipsCp0WiredWrite)

;/*********************************************************************************************************
;  CP0_RAND
;*********************************************************************************************************/

FUNC_DEF(mipsCp0RandomRead)
    mipsCp0RegRead     CP0_RAND
    FUNC_END(mipsCp0RandomRead)

FUNC_DEF(mipsCp0RandomWrite)
    mipsCp0RegWrite    CP0_RAND
    FUNC_END(mipsCp0RandomWrite)

;/*********************************************************************************************************
;  CP0_TLBLO0
;*********************************************************************************************************/

FUNC_DEF(mipsCp0EntryLo0Read)
    mipsCp0RegRead     CP0_TLBLO0
    FUNC_END(mipsCp0EntryLo0Read)

FUNC_DEF(mipsCp0EntryLo0Write)
    mipsCp0RegWrite    CP0_TLBLO0
    FUNC_END(mipsCp0EntryLo0Write)

;/*********************************************************************************************************
;  CP0_TLBLO1
;*********************************************************************************************************/

FUNC_DEF(mipsCp0EntryLo1Read)
    mipsCp0RegRead     CP0_TLBLO1
    FUNC_END(mipsCp0EntryLo1Read)

FUNC_DEF(mipsCp0EntryLo1Write)
    mipsCp0RegWrite    CP0_TLBLO1
    FUNC_END(mipsCp0EntryLo1Write)

;/*********************************************************************************************************
;  CP0_CTXT
;*********************************************************************************************************/

FUNC_DEF(mipsCp0ContextRead)
    mipsCp0RegRead     CP0_CTXT
    FUNC_END(mipsCp0ContextRead)

FUNC_DEF(mipsCp0ContextWrite)
    mipsCp0RegWrite    CP0_CTXT
    FUNC_END(mipsCp0ContextWrite)

;/*********************************************************************************************************
;  CP0_PAGEMASK
;*********************************************************************************************************/

FUNC_DEF(mipsCp0PageMaskRead)
    mipsCp0RegRead     CP0_PAGEMASK
    FUNC_END(mipsCp0PageMaskRead)

FUNC_DEF(mipsCp0PageMaskWrite)
    mipsCp0RegWrite    CP0_PAGEMASK
    FUNC_END(mipsCp0PageMaskWrite)

;/*********************************************************************************************************
;  CP0_TLBHI
;*********************************************************************************************************/

FUNC_DEF(mipsCp0EntryHiRead)
    mipsCp0RegRead     CP0_TLBHI
    FUNC_END(mipsCp0EntryHiRead)

FUNC_DEF(mipsCp0EntryHiWrite)
    mipsCp0RegWrite    CP0_TLBHI
    FUNC_END(mipsCp0EntryHiWrite)

;/*********************************************************************************************************
;  CP0_TAGLO
;*********************************************************************************************************/

FUNC_DEF(mipsCp0TagLoRead)
    mipsCp0RegRead     CP0_TAGLO
    FUNC_END(mipsCp0TagLoRead)

FUNC_DEF(mipsCp0TagLoWrite)
    mipsCp0RegWrite    CP0_TAGLO
    FUNC_END(mipsCp0TagLoWrite)

;/*********************************************************************************************************
;  CP0_TAGHI
;*********************************************************************************************************/

FUNC_DEF(mipsCp0TagHiRead)
    mipsCp0RegRead     CP0_TAGHI
    FUNC_END(mipsCp0TagHiRead)

FUNC_DEF(mipsCp0TagHiWrite)
    mipsCp0RegWrite    CP0_TAGHI
    FUNC_END(mipsCp0TagHiWrite)

;/*********************************************************************************************************
;  CP0_DIAG
;*********************************************************************************************************/

FUNC_DEF(mipsCp0DiagRead)
    mipsCp0RegRead     CP0_DIAG
    FUNC_END(mipsCp0DiagRead)

FUNC_DEF(mipsCp0DiagWrite)
    mipsCp0RegWrite    CP0_DIAG
    FUNC_END(mipsCp0DiagWrite)

;/*********************************************************************************************************
;  CP0_PAGEGRAIN
;*********************************************************************************************************/

FUNC_DEF(mipsCp0PageGrainRead)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile

    MFC0    V0 , CP0_PAGEGRAIN
    EHB
    JR      RA
    NOP

    .set    pop
    FUNC_END(mipsCp0PageGrainRead)

FUNC_DEF(mipsCp0PageGrainWrite)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile

    MTC0    A0 , CP0_PAGEGRAIN
    EHB
    JR      RA
    NOP

    .set    pop
    FUNC_END(mipsCp0PageGrainWrite)

;/*********************************************************************************************************
;  CP0_GSCONFIG
;*********************************************************************************************************/

FUNC_DEF(mipsCp0GSConfigRead)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile

    MFC0    V0 , CP0_GSCONFIG
    EHB
    JR      RA
    NOP

    .set    pop
    FUNC_END(mipsCp0GSConfigRead)

FUNC_DEF(mipsCp0GSConfigWrite)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile

    MTC0    A0 , CP0_GSCONFIG
    EHB
    JR      RA
    NOP

    .set    pop
    FUNC_END(mipsCp0GSConfigWrite)

    FILE_END()

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
