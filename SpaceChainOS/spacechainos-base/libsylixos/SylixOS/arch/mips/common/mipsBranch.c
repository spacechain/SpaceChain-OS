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
** 文   件   名: mipsBranch.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2015 年 12 月 25 日
**
** 描        述: MIPS 分支预测.
*********************************************************************************************************/
/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1996, 97, 2000, 2001 by Ralf Baechle
 * Copyright (C) 2001 MIPS Technologies, Inc.
 */

#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "mipsInst.h"

#define CONFIG_CPU_CAVIUM_OCTEON

/*
 * Compute the return address and do emulate branch simulation, if required.
 */
int __compute_return_epc (ARCH_REG_CTX *regs)
{
    unsigned int *addr;
#ifndef SYLIXOS
    unsigned int bit, fcr31, dspcontrol;
#else
    unsigned int bit, fcr31;
#endif
    long epc;
    union mips_instruction insn;

    epc = regs->REG_uiCP0EPC;
    if (epc & 3)
        goto unaligned;

    /*
     * Read the instruction
     */
    addr = (unsigned int *)epc;
    insn.word = *addr;

    switch (insn.i_format.opcode) {
    /*
     * jr and jalr are in r_format format.
     */
    case spec_op:
        switch (insn.r_format.func) {
        case jalr_op:
            regs->REG_uiReg[insn.r_format.rd] = epc + 8;
            /* Fall through */
        case jr_op:
            regs->REG_uiCP0EPC = regs->REG_uiReg[insn.r_format.rs];
            break;
        }
        break;

    /*
     * This group contains:
     * bltz_op, bgez_op, bltzl_op, bgezl_op,
     * bltzal_op, bgezal_op, bltzall_op, bgezall_op.
     */
    case bcond_op:
        switch (insn.i_format.rt) {
        case bltz_op:
        case bltzl_op:
            if ((long)regs->REG_uiReg[insn.i_format.rs] < 0)
                epc = epc + 4 + (insn.i_format.simmediate << 2);
            else
                epc += 8;
            regs->REG_uiCP0EPC = epc;
            break;

        case bgez_op:
        case bgezl_op:
            if ((long)regs->REG_uiReg[insn.i_format.rs] >= 0)
                epc = epc + 4 + (insn.i_format.simmediate << 2);
            else
                epc += 8;
            regs->REG_uiCP0EPC = epc;
            break;

        case bltzal_op:
        case bltzall_op:
            regs->REG_uiReg[31] = epc + 8;
            if ((long)regs->REG_uiReg[insn.i_format.rs] < 0)
                epc = epc + 4 + (insn.i_format.simmediate << 2);
            else
                epc += 8;
            regs->REG_uiCP0EPC = epc;
            break;

        case bgezal_op:
        case bgezall_op:
            regs->REG_uiReg[31] = epc + 8;
            if ((long)regs->REG_uiReg[insn.i_format.rs] >= 0)
                epc = epc + 4 + (insn.i_format.simmediate << 2);
            else
                epc += 8;
            regs->REG_uiCP0EPC = epc;
            break;
        case bposge32_op:
#ifndef SYLIXOS /* TODO: 暂不支持 DSP */
            if (!cpu_has_dsp)
                goto sigill;

            dspcontrol = rddsp(0x01);

            if (dspcontrol >= 32) {
                epc = epc + 4 + (insn.i_format.simmediate << 2);
            } else
                epc += 8;
            regs->REG_uiCP0EPC = epc;
#else
            goto sigill;
#endif
            break;
        }
        break;

    /*
     * These are unconditional and in j_format.
     */
    case jal_op:
        regs->REG_uiReg[31] = regs->REG_uiCP0EPC + 8;
    case j_op:
        epc += 4;
        epc >>= 28;
        epc <<= 28;
        epc |= (insn.j_format.target << 2);
        regs->REG_uiCP0EPC = epc;
        break;

    /*
     * These are conditional and in i_format.
     */
    case beq_op:
    case beql_op:
        if (regs->REG_uiReg[insn.i_format.rs] ==
            regs->REG_uiReg[insn.i_format.rt])
            epc = epc + 4 + (insn.i_format.simmediate << 2);
        else
            epc += 8;
        regs->REG_uiCP0EPC = epc;
        break;

    case bne_op:
    case bnel_op:
        if (regs->REG_uiReg[insn.i_format.rs] !=
            regs->REG_uiReg[insn.i_format.rt])
            epc = epc + 4 + (insn.i_format.simmediate << 2);
        else
            epc += 8;
        regs->REG_uiCP0EPC = epc;
        break;

    case blez_op: /* not really i_format */
    case blezl_op:
        /* rt field assumed to be zero */
        if ((long)regs->REG_uiReg[insn.i_format.rs] <= 0)
            epc = epc + 4 + (insn.i_format.simmediate << 2);
        else
            epc += 8;
        regs->REG_uiCP0EPC = epc;
        break;

    case bgtz_op:
    case bgtzl_op:
        /* rt field assumed to be zero */
        if ((long)regs->REG_uiReg[insn.i_format.rs] > 0)
            epc = epc + 4 + (insn.i_format.simmediate << 2);
        else
            epc += 8;
        regs->REG_uiCP0EPC = epc;
        break;

    /*
     * And now the FPA/cp1 branch instructions.
     */
    case cop1_op:
#ifndef SYLIXOS /* TODO: 直接读 FCR31 */
        preempt_disable();
        if (is_fpu_owner())
            asm volatile("cfc1\t%0,$31" : "=r" (fcr31));
        else
            fcr31 = current->thread.fpu.fcr31;
        preempt_enable();
#else
        asm volatile("cfc1\t%0,$31" : "=r" (fcr31));
#endif

        bit = (insn.i_format.rt >> 2);
        bit += (bit != 0);
        bit += 23;
        switch (insn.i_format.rt & 3) {
        case 0: /* bc1f */
        case 2: /* bc1fl */
            if (~fcr31 & (1 << bit))
                epc = epc + 4 + (insn.i_format.simmediate << 2);
            else
                epc += 8;
            regs->REG_uiCP0EPC = epc;
            break;

        case 1: /* bc1t */
        case 3: /* bc1tl */
            if (fcr31 & (1 << bit))
                epc = epc + 4 + (insn.i_format.simmediate << 2);
            else
                epc += 8;
            regs->REG_uiCP0EPC = epc;
            break;
        }
        break;
#ifdef CONFIG_CPU_CAVIUM_OCTEON
    case lwc2_op: /* This is bbit0 on Octeon */
        if ((regs->REG_uiReg[insn.i_format.rs] & (1ull<<insn.i_format.rt))
             == 0)
            epc = epc + 4 + (insn.i_format.simmediate << 2);
        else
            epc += 8;
        regs->REG_uiCP0EPC = epc;
        break;
    case ldc2_op: /* This is bbit032 on Octeon */
        if ((regs->REG_uiReg[insn.i_format.rs] &
            (1ull<<(insn.i_format.rt+32))) == 0)
            epc = epc + 4 + (insn.i_format.simmediate << 2);
        else
            epc += 8;
        regs->REG_uiCP0EPC = epc;
        break;
    case swc2_op: /* This is bbit1 on Octeon */
        if (regs->REG_uiReg[insn.i_format.rs] & (1ull<<insn.i_format.rt))
            epc = epc + 4 + (insn.i_format.simmediate << 2);
        else
            epc += 8;
        regs->REG_uiCP0EPC = epc;
        break;
    case sdc2_op: /* This is bbit132 on Octeon */
        if (regs->REG_uiReg[insn.i_format.rs] &
            (1ull<<(insn.i_format.rt+32)))
            epc = epc + 4 + (insn.i_format.simmediate << 2);
        else
            epc += 8;
        regs->REG_uiCP0EPC = epc;
        break;
#endif
    }

    return 0;

unaligned:
    return -EFAULT;

sigill:
    return -EFAULT;
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
