
#define __SYLIXOS_KERNEL
#include "SylixOS.h"
#include <linux/compat.h>

#include "./config.h"
#include "./porting.h"
#include "arch/mips/common/mipsInst.h"
#include "arch/mips/common/mipsBranch.h"

/**
 * struct emuframe - The 'emulation' frame structure
 * @emul:   The instruction to 'emulate'.
 * @badinst:    A break instruction to cause a return to the kernel.
 *
 * This structure defines the frames placed within the delay slot emulation
 * page in response to a call to mips_dsemul(). Each thread may be allocated
 * only one frame at any given time. The kernel stores within it the
 * instruction to be 'emulated' followed by a break instruction, then
 * executes the frame in user mode. The break causes a trap to the kernel
 * which leads to do_dsemulret() being called unless the instruction in
 * @emul causes a trap itself, is a branch, or a signal is delivered to
 * the thread. In these cases the allocated frame will either be reused by
 * a subsequent delay slot 'emulation', or be freed during signal delivery or
 * upon thread exit.
 *
 * This approach is used because:
 *
 * - Actually emulating all instructions isn't feasible. We would need to
 *   be able to handle instructions from all revisions of the MIPS ISA,
 *   all ASEs & all vendor instruction set extensions. This would be a
 *   whole lot of work & continual maintenance burden as new instructions
 *   are introduced, and in the case of some vendor extensions may not
 *   even be possible. Thus we need to take the approach of actually
 *   executing the instruction.
 *
 * - We must execute the instruction within user context. If we were to
 *   execute the instruction in kernel mode then it would have access to
 *   kernel resources without very careful checks, leaving us with a
 *   high potential for security or stability issues to arise.
 *
 * - We used to place the frame on the users stack, but this requires
 *   that the stack be executable. This is bad for security so the
 *   per-process page is now used instead.
 *
 * - The instruction in @emul may be something entirely invalid for a
 *   delay slot. The user may (intentionally or otherwise) place a branch
 *   in a delay slot, or a kernel mode instruction, or something else
 *   which generates an exception. Thus we can't rely upon the break in
 *   @badinst always being hit. For this reason we track the index of the
 *   frame allocated to each thread, allowing us to clean it up at later
 *   points such as signal delivery or thread exit.
 *
 * - The user may generate a fake struct emuframe if they wish, invoking
 *   the BRK_MEMU break instruction themselves. We must therefore not
 *   trust that BRK_MEMU means there's actually a valid frame allocated
 *   to the thread, and must not allow the user to do anything they
 *   couldn't already.
 */
/*
 * SylixOS FPU emulator frame
 */
extern MIPS_FPU_EMU_FRAME  *archFpuEmuFrameGet(PLW_CLASS_TCB  ptcbCur);

int mips_dsemul(ARCH_REG_CTX *regs, mips_instruction ir,
		        unsigned long branch_pc, unsigned long cont_pc)
{
	int isa16 = get_isa16_mode(regs->cp0_epc);
	mips_instruction break_math;
	MIPS_FPU_EMU_FRAME *fr;
    PLW_CLASS_TCB ptcbCur;
	int err;

	/* NOP is easy */
	if (ir == 0)
		return -1;

	/* microMIPS instructions */
	if (isa16) {
		union mips_instruction insn = { .word = ir };

		/* NOP16 aka MOVE16 $0, $0 */
		if ((ir >> 16) == MM_NOP16)
			return -1;

		/* ADDIUPC */
		if (insn.mm_a_format.opcode == mm_addiupc_op) {
			unsigned int rs;
			s32 v;

			rs = (((insn.mm_a_format.rs + 0xe) & 0xf) + 2);
			v = regs->cp0_epc & ~3;
			v += insn.mm_a_format.simmediate << 2;
			regs->regs[rs] = (long)v;
			return -1;
		}
	}

	pr_debug("dsemul 0x%08lx cont at 0x%08lx\n", regs->cp0_epc, cont_pc);

	/* Allocate a frame if we don't already have one */
    LW_TCB_GET_CUR(ptcbCur);
	fr = archFpuEmuFrameGet(ptcbCur);

	/* Retrieve the appropriately encoded break instruction */
	break_math = BREAK_MATH(isa16);

	/* Write the instructions to the frame */
	if (isa16) {
		err = __put_user(ir >> 16,
				 (u16 __user *)(&fr->MIPSFPUE_emul));
		err |= __put_user(ir & 0xffff,
				  (u16 __user *)((long)(&fr->MIPSFPUE_emul) + 2));
		err |= __put_user(break_math >> 16,
				  (u16 __user *)(&fr->MIPSFPUE_badinst));
		err |= __put_user(break_math & 0xffff,
				  (u16 __user *)((long)(&fr->MIPSFPUE_badinst) + 2));
	} else {
		err = __put_user(ir, &fr->MIPSFPUE_emul);
		err |= __put_user(break_math, &fr->MIPSFPUE_badinst);
	}

	if (unlikely(err)) {
		MIPS_FPU_EMU_INC_STATS(errors);
		return SIGBUS;
	}

	/* Record the PC of the branch, PC to continue from & frame index */
    ptcbCur->TCB_ulBdEmuBranchPC = branch_pc;
    ptcbCur->TCB_ulBdEmuContPC   = cont_pc;

	/* Change user register context to execute the frame */
	regs->cp0_epc = (unsigned long)&fr->MIPSFPUE_emul | isa16;

#if LW_CFG_CACHE_EN > 0
	/* Ensure the icache observes our newly written frame */
	API_CacheTextUpdate((PVOID)fr, sizeof(MIPS_FPU_EMU_FRAME));
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

	return 0;
}

int do_dsemulret(PLW_CLASS_TCB ptcbCur, ARCH_REG_CTX *xcp)
{
	/* Set EPC to return to post-branch instruction */
	xcp->cp0_epc = ptcbCur->TCB_ulBdEmuContPC;
	pr_debug("dsemulret to 0x%08lx\n", xcp->cp0_epc);

    return 0;
}
