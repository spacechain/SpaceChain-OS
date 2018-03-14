/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: arch_leon.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2017 年 10 月 10 日
**
** 描        述: SPARC LEON 变种相关定义.
*********************************************************************************************************/

#ifndef __SPARC_ARCH_LEON_H
#define __SPARC_ARCH_LEON_H

/*********************************************************************************************************
  Copyright (C) 2004 Konrad Eisele (eiselekd@web.de,konrad@gaisler.com) Gaisler Research
  Copyright (C) 2004 Stefan Holst (mail@s-holst.de) Uni-Stuttgart
  Copyright (C) 2009 Daniel Hellstrom (daniel@gaisler.com) Aeroflex Gaisler AB
  Copyright (C) 2009 Konrad Eisele (konrad@gaisler.com) Aeroflex Gaisler AB
*********************************************************************************************************/
#if defined(__SYLIXOS_KERNEL) || defined(__ASSEMBLY__) || defined(ASSEMBLY)
/*********************************************************************************************************
  Read CPU ID(LEON3)
*********************************************************************************************************/
#define LEON3_READ_CPUID(reg)   \
    RD      %asr17 , reg;       \
    SRL     reg    , 28  , reg;
/*********************************************************************************************************
  See GRLIB-TN-0009: "LEON3FT Stale Cache Entry After Store with Data Tag Parity Error"
*********************************************************************************************************/
#if LW_CFG_SPARC_LEON_B2BST_NOP > 0
#define LEON3FT_B2BST_NOP \
    NOP;
#else
#define LEON3FT_B2BST_NOP
#endif
/*********************************************************************************************************
  MMU register access, ASI_LEON_MMUREGS
*********************************************************************************************************/
#define LEON_CNR_CTRL                   0x000
#define LEON_CNR_CTXP                   0x100
#define LEON_CNR_CTX                    0x200
#define LEON_CNR_F                      0x300
#define LEON_CNR_FADDR                  0x400
#define LEON_CNR_CTX_NCTX               256                             /*  number of MMU ctx           */
#define LEON_CNR_CTRL_TLBDIS            0x80000000
/*********************************************************************************************************
  MMU TBL SIZE
*********************************************************************************************************/
#define LEON_MMUTLB_ENT_MAX             64
/*********************************************************************************************************
  Diagnostic access from mmutlb.vhd:
  0: pte address
  4: pte
  8: additional flags
*********************************************************************************************************/
#define LEON_DIAGF_LVL                  0x3
#define LEON_DIAGF_WR                   0x8
#define LEON_DIAGF_WR_SHIFT             3
#define LEON_DIAGF_HIT                  0x10
#define LEON_DIAGF_HIT_SHIFT            4
#define LEON_DIAGF_CTX                  0x1fe0
#define LEON_DIAGF_CTX_SHIFT            5
#define LEON_DIAGF_VALID                0x2000
#define LEON_DIAGF_VALID_SHIFT          13
/*********************************************************************************************************
  IRQ masks
*********************************************************************************************************/
#define LEON_HARD_INT(x)                (1 << (x))                      /*  irq 0-15                    */
#define LEON_IRQMASK_R                  0x0000fffe                      /*  bit 15- 1 of lregs.irqmask  */
#define LEON_IRQPRIO_R                  0xfffe0000                      /*  bit 31-17 of lregs.irqmask  */
/*********************************************************************************************************
  MCFG2 SRAM SDRAM
*********************************************************************************************************/
#define LEON_MCFG2_SRAMDIS              0x00002000
#define LEON_MCFG2_SDRAMEN              0x00004000
#define LEON_MCFG2_SRAMBANKSZ           0x00001e00                      /*  [12-9]                      */
#define LEON_MCFG2_SRAMBANKSZ_SHIFT     9
#define LEON_MCFG2_SDRAMBANKSZ          0x03800000                      /*  [25-23]                     */
#define LEON_MCFG2_SDRAMBANKSZ_SHIFT    23
/*********************************************************************************************************
  TCNT0 masks
*********************************************************************************************************/
#define LEON_TCNT0_MASK                 0x7fffff
/*********************************************************************************************************
  LEON3 ASI
*********************************************************************************************************/
#define ASI_LEON3_SYSCTRL               0x02
#define ASI_LEON3_SYSCTRL_ICFG          0x08
#define ASI_LEON3_SYSCTRL_DCFG          0x0c
#define ASI_LEON3_SYSCTRL_CFG_SNOOPING  (1 << 27)
#define ASI_LEON3_SYSCTRL_CFG_SSIZE(c)  (1 << ((c >> 20) & 0xf))
/*********************************************************************************************************
  CCR
*********************************************************************************************************/
#define LEON3_XCCR_SETS_MASK            0x07000000UL
#define LEON3_XCCR_SSIZE_MASK           0x00f00000UL

#define LEON2_CCR_DSETS_MASK            0x03000000UL
#define LEON2_CFG_SSIZE_MASK            0x00007000UL
/*********************************************************************************************************
  Do a physical address bypass write, i.e. for 0x80000000
*********************************************************************************************************/
#if !defined(__ASSEMBLY__) && !defined(ASSEMBLY)

static LW_INLINE VOID  leonStoreReg (ULONG  paddr, ULONG  value)
{
    __asm__ __volatile__ ("sta  %0, [%1] %2\n\t" : : "r"(value), "r"(paddr),
                          "i"(ASI_LEON_BYPASS) : "memory");
}
/*********************************************************************************************************
  Do a physical address bypass load, i.e. for 0x80000000
*********************************************************************************************************/
static LW_INLINE ULONG  leonLoadReg (ULONG  paddr)
{
    ULONG  retval;

    __asm__ __volatile__ ("lda  [%1] %2, %0\n\t" :
                          "=r"(retval) : "r"(paddr), "i"(ASI_LEON_BYPASS));
    return retval;
}
/*********************************************************************************************************
  Macro access for leonLoadReg() and leonStoreReg()
*********************************************************************************************************/
#define LEON3_BYPASS_LOAD_PA(x)     (leonLoadReg((ULONG)(x)))
#define LEON3_BYPASS_STORE_PA(x, v) (leonStoreReg((ULONG)(x), (ULONG)(v)))
#define LEON_BYPASS_LOAD_PA(x)      leonLoadReg((ULONG)(x))
#define LEON_BYPASS_STORE_PA(x, v)  leonStoreReg((ULONG)(x), (ULONG)(v))

#endif
#endif
#endif                                                                  /*  __SPARC_ARCH_LEON_H         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
