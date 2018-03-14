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
;** 文   件   名: mips32MmuAsm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2016 年 06 月 15 日
;**
;** 描        述: MIPS32 体系构架 MMU TLB 重填处理.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/kernel/vmm_cfg.h>
#include "config/kernel/memory_cfg.h"

#if LW_CFG_VMM_EN > 0

#include "arch/mips/arch_mmu.h"

;/*********************************************************************************************************
;  PTE BASE 相关定义
;*********************************************************************************************************/

#define PTE_BASE_OFFSET                 23
#define PTE_BASE_SIZE                   9

;/*********************************************************************************************************
;  BADVPN2 相关定义
;*********************************************************************************************************/

#if   LW_CFG_VMM_PAGE_SIZE == (4  * LW_CFG_KB_SIZE)
#define MIPS32_BADVPN2_SHIFT            0
#elif LW_CFG_VMM_PAGE_SIZE == (16 * LW_CFG_KB_SIZE)
#define MIPS32_BADVPN2_SHIFT            2
#elif LW_CFG_VMM_PAGE_SIZE == (64 * LW_CFG_KB_SIZE)
#define MIPS32_BADVPN2_SHIFT            4
#else
#error  LW_CFG_VMM_PAGE_SIZE must be (4K, 16K, 64K)!
#endif                                                                   /*  LW_CFG_VMM_PAGE_SIZE        */

    EXPORT_LABEL(mips32TlbRefillEntry)

    FILE_BEGIN()

;/*********************************************************************************************************
;  TLB 重填
;*********************************************************************************************************/

FUNC_DEF(mips32TlbRefillEntry)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile

    ;/*
    ; * K1 = CP0_CTXT
    ; * K0 = K1
    ; */
    MFC0    K1 , CP0_CTXT                                               ;/*  K1 等于 Context 寄存器      */
    EHB
    MOVE    K0 , K1                                                     ;/*  K0 等于 Context 寄存器      */

    ;/*
    ; * K1 <<= PTE_BASE_SIZE
    ; * K1 >>= PTE_BASE_SIZE
    ; * K1 >>= 4
    ; * K1 >>= MIPS32_BADVPN2_SHIFT
    ; * K1 <<= 3
    ; */
    SLL     K1 , PTE_BASE_SIZE
    SRL     K1 , (PTE_BASE_SIZE + 4 + MIPS32_BADVPN2_SHIFT)             ;/*  K1 为 BAD VPN2              */
    SLL     K1 , (4 - 1)

    ;/*
    ; * K0 >>= PTE_BASE_OFFSET
    ; * K0 <<= PTE_BASE_OFFSET
    ; */
    SRL     K0 , PTE_BASE_OFFSET
    SLL     K0 , PTE_BASE_OFFSET                                        ;/*  K0 为 PTE BASE              */

    ;/*
    ; * K1 = K1 | K0
    ; */
    OR      K1 , K1 , K0                                                ;/*  合成                        */

    ;/*
    ; * K0 = *K1
    ; * K1 = *(K1 + 4)
    ; */
    LW      K0 , 0(K1)
    LW      K1 , 4(K1)

    ;/*
    ; * CP0_TLBLO0 = K0
    ; * CP0_TLBLO1 = K1
    ; */
    MTC0    K0 , CP0_TLBLO0                                             ;/*  EntryLo0                    */
    MTC0    K1 , CP0_TLBLO1                                             ;/*  EntryLo1                    */
    EHB

    TLBWR                                                               ;/*  TLB 随机替换                */

    ERET                                                                ;/*  异常返回                    */
    NOP

    .set    pop
    FUNC_END(mips32TlbRefillEntry)

    FILE_END()

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
