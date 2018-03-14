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
** 文   件   名: mipsUnaligned.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2015 年 12 月 25 日
**
** 描        述: MIPS 非对齐处理.
*********************************************************************************************************/
/*
 * Handle unaligned accesses by emulation.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1996, 1998, 1999, 2002 by Ralf Baechle
 * Copyright (C) 1999 Silicon Graphics, Inc.
 * Copyright (C) 2014 Imagination Technologies Ltd.
 *
 * This file contains exception handler for address error exception with the
 * special capability to execute faulting instructions in software.  The
 * handler does not try to handle the case when the program counter points
 * to an address not aligned to a word boundary.
 *
 * Putting data to unaligned addresses is a bad practice even on Intel where
 * only the performance is affected.  Much worse is that such code is non-
 * portable.  Due to several programs that die on MIPS due to alignment
 * problems I decided to implement this handler anyway though I originally
 * didn't intend to do this at all for user code.
 *
 * For now I enable fixing of address errors by default to make life easier.
 * I however intend to disable this somewhen in the future when the alignment
 * problems with user programs have been fixed.	 For programmers this is the
 * right way to go.
 *
 * Fixing address errors is a per process option.  The option is inherited
 * across fork(2) and execve(2) calls.	If you really want to use the
 * option in your user programs - I discourage the use of the software
 * emulation strongly - use the following code in your userland stuff:
 *
 * #include <sys/sysmips.h>
 *
 * ...
 * sysmips(MIPS_FIXADE, x);
 * ...
 *
 * The argument x is 0 for disabling software emulation, enabled otherwise.
 *
 * Below a little program to play around with this feature.
 *
 * #include <stdio.h>
 * #include <sys/sysmips.h>
 *
 * struct foo {
 *	   unsigned char bar[8];
 * };
 *
 * main(int argc, char *argv[])
 * {
 *	   struct foo x = {0, 1, 2, 3, 4, 5, 6, 7};
 *	   unsigned int *p = (unsigned int *) (x.bar + 3);
 *	   int i;
 *
 *	   if (argc > 1)
 *		   sysmips(MIPS_FIXADE, atoi(argv[1]));
 *
 *	   printf("*p = %08lx\n", *p);
 *
 *	   *p = 0xdeadface;
 *
 *	   for(i = 0; i <= 7; i++)
 *	   printf("%02x ", x.bar[i]);
 *	   printf("\n");
 * }
 *
 * Coprocessor loads are not supported; I think this case is unimportant
 * in the practice.
 *
 * TODO: Handle ndc (attempted store to doubleword in uncached memory)
 *	 exception for the R6000.
 *	 A store crossing a page boundary might be executed only partially.
 *	 Undo the partial store in this case.
 */

#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include <linux/compat.h>
#include "./config.h"
#include "./porting.h"
#include "asm-eva.h"
#include "arch/mips/common/mipsInst.h"
#include "arch/mips/common/mipsBranch.h"
#include "arch/mips/param/mipsParam.h"

#define STR(x)	__STR(x)
#define __STR(x)  #x

#if BYTE_ORDER == BIG_ENDIAN
#define     _LoadHW(addr, value, res, type)  \
do {                                                        \
		__asm__ __volatile__ (".set\tnoat\n"        \
			"1:\t"type##_lb("%0", "0(%2)")"\n"  \
			"2:\t"type##_lbu("$1", "1(%2)")"\n\t"\
			"sll\t%0, 0x8\n\t"                  \
			"or\t%0, $1\n\t"                    \
			"li\t%1, 0\n"                       \
			"3:\t.set\tat\n\t"                  \
			".insn\n\t"                         \
			".section\t.fixup,\"ax\"\n\t"       \
			"4:\tli\t%1, %3\n\t"                \
			"j\t3b\n\t"                         \
			".previous\n\t"                     \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 4b\n\t"              \
			STR(PTR)"\t2b, 4b\n\t"              \
			".previous"                         \
			: "=&r" (value), "=r" (res)         \
			: "r" (addr), "i" (-EFAULT));       \
} while(0)

#ifndef CONFIG_CPU_MIPSR6
#define     _LoadW(addr, value, res, type)   \
do {                                                        \
		__asm__ __volatile__ (                      \
			"1:\t"type##_lwl("%0", "(%2)")"\n"   \
			"2:\t"type##_lwr("%0", "3(%2)")"\n\t"\
			"li\t%1, 0\n"                       \
			"3:\n\t"                            \
			".insn\n\t"                         \
			".section\t.fixup,\"ax\"\n\t"       \
			"4:\tli\t%1, %3\n\t"                \
			"j\t3b\n\t"                         \
			".previous\n\t"                     \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 4b\n\t"              \
			STR(PTR)"\t2b, 4b\n\t"              \
			".previous"                         \
			: "=&r" (value), "=r" (res)         \
			: "r" (addr), "i" (-EFAULT));       \
} while(0)

#else
/* MIPSR6 has no lwl instruction */
#define     _LoadW(addr, value, res, type) \
do {                                                        \
		__asm__ __volatile__ (			    \
			".set\tpush\n"			    \
			".set\tnoat\n\t"		    \
			"1:"type##_lb("%0", "0(%2)")"\n\t"  \
			"2:"type##_lbu("$1", "1(%2)")"\n\t" \
			"sll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"3:"type##_lbu("$1", "2(%2)")"\n\t" \
			"sll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"4:"type##_lbu("$1", "3(%2)")"\n\t" \
			"sll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"li\t%1, 0\n"			    \
			".set\tpop\n"			    \
			"10:\n\t"			    \
			".insn\n\t"			    \
			".section\t.fixup,\"ax\"\n\t"	    \
			"11:\tli\t%1, %3\n\t"		    \
			"j\t10b\n\t"			    \
			".previous\n\t"			    \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 11b\n\t"		    \
			STR(PTR)"\t2b, 11b\n\t"		    \
			STR(PTR)"\t3b, 11b\n\t"		    \
			STR(PTR)"\t4b, 11b\n\t"		    \
			".previous"			    \
			: "=&r" (value), "=r" (res)	    \
			: "r" (addr), "i" (-EFAULT));       \
} while(0)

#endif /* CONFIG_CPU_MIPSR6 */

#define     _LoadHWU(addr, value, res, type) \
do {                                                        \
		__asm__ __volatile__ (                      \
			".set\tnoat\n"                      \
			"1:\t"type##_lbu("%0", "0(%2)")"\n" \
			"2:\t"type##_lbu("$1", "1(%2)")"\n\t"\
			"sll\t%0, 0x8\n\t"                  \
			"or\t%0, $1\n\t"                    \
			"li\t%1, 0\n"                       \
			"3:\n\t"                            \
			".insn\n\t"                         \
			".set\tat\n\t"                      \
			".section\t.fixup,\"ax\"\n\t"       \
			"4:\tli\t%1, %3\n\t"                \
			"j\t3b\n\t"                         \
			".previous\n\t"                     \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 4b\n\t"              \
			STR(PTR)"\t2b, 4b\n\t"              \
			".previous"                         \
			: "=&r" (value), "=r" (res)         \
			: "r" (addr), "i" (-EFAULT));       \
} while(0)

#ifndef CONFIG_CPU_MIPSR6
#define     _LoadWU(addr, value, res, type)  \
do {                                                        \
		__asm__ __volatile__ (                      \
			"1:\t"type##_lwl("%0", "(%2)")"\n"  \
			"2:\t"type##_lwr("%0", "3(%2)")"\n\t"\
			"dsll\t%0, %0, 32\n\t"              \
			"dsrl\t%0, %0, 32\n\t"              \
			"li\t%1, 0\n"                       \
			"3:\n\t"                            \
			".insn\n\t"                         \
			"\t.section\t.fixup,\"ax\"\n\t"     \
			"4:\tli\t%1, %3\n\t"                \
			"j\t3b\n\t"                         \
			".previous\n\t"                     \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 4b\n\t"              \
			STR(PTR)"\t2b, 4b\n\t"              \
			".previous"                         \
			: "=&r" (value), "=r" (res)         \
			: "r" (addr), "i" (-EFAULT));       \
} while(0)

#define     _LoadDW(addr, value, res)  \
do {                                                        \
		__asm__ __volatile__ (                      \
			"1:\tldl\t%0, (%2)\n"               \
			"2:\tldr\t%0, 7(%2)\n\t"            \
			"li\t%1, 0\n"                       \
			"3:\n\t"                            \
			".insn\n\t"                         \
			"\t.section\t.fixup,\"ax\"\n\t"     \
			"4:\tli\t%1, %3\n\t"                \
			"j\t3b\n\t"                         \
			".previous\n\t"                     \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 4b\n\t"              \
			STR(PTR)"\t2b, 4b\n\t"              \
			".previous"                         \
			: "=&r" (value), "=r" (res)         \
			: "r" (addr), "i" (-EFAULT));       \
} while(0)

#else
/* MIPSR6 has not lwl and ldl instructions */
#define	    _LoadWU(addr, value, res, type) \
do {                                                        \
		__asm__ __volatile__ (			    \
			".set\tpush\n\t"		    \
			".set\tnoat\n\t"		    \
			"1:"type##_lbu("%0", "0(%2)")"\n\t" \
			"2:"type##_lbu("$1", "1(%2)")"\n\t" \
			"sll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"3:"type##_lbu("$1", "2(%2)")"\n\t" \
			"sll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"4:"type##_lbu("$1", "3(%2)")"\n\t" \
			"sll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"li\t%1, 0\n"			    \
			".set\tpop\n"			    \
			"10:\n\t"			    \
			".insn\n\t"			    \
			".section\t.fixup,\"ax\"\n\t"	    \
			"11:\tli\t%1, %3\n\t"		    \
			"j\t10b\n\t"			    \
			".previous\n\t"			    \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 11b\n\t"		    \
			STR(PTR)"\t2b, 11b\n\t"		    \
			STR(PTR)"\t3b, 11b\n\t"		    \
			STR(PTR)"\t4b, 11b\n\t"		    \
			".previous"			    \
			: "=&r" (value), "=r" (res)	    \
			: "r" (addr), "i" (-EFAULT));       \
} while(0)

#define     _LoadDW(addr, value, res)  \
do {                                                        \
		__asm__ __volatile__ (			    \
			".set\tpush\n\t"		    \
			".set\tnoat\n\t"		    \
			"1:lb\t%0, 0(%2)\n\t"    	    \
			"2:lbu\t $1, 1(%2)\n\t"   	    \
			"dsll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"3:lbu\t$1, 2(%2)\n\t"   	    \
			"dsll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"4:lbu\t$1, 3(%2)\n\t"   	    \
			"dsll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"5:lbu\t$1, 4(%2)\n\t"   	    \
			"dsll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"6:lbu\t$1, 5(%2)\n\t"   	    \
			"dsll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"7:lbu\t$1, 6(%2)\n\t"   	    \
			"dsll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"8:lbu\t$1, 7(%2)\n\t"   	    \
			"dsll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"li\t%1, 0\n"			    \
			".set\tpop\n\t"			    \
			"10:\n\t"			    \
			".insn\n\t"			    \
			".section\t.fixup,\"ax\"\n\t"	    \
			"11:\tli\t%1, %3\n\t"		    \
			"j\t10b\n\t"			    \
			".previous\n\t"			    \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 11b\n\t"		    \
			STR(PTR)"\t2b, 11b\n\t"		    \
			STR(PTR)"\t3b, 11b\n\t"		    \
			STR(PTR)"\t4b, 11b\n\t"		    \
			STR(PTR)"\t5b, 11b\n\t"		    \
			STR(PTR)"\t6b, 11b\n\t"		    \
			STR(PTR)"\t7b, 11b\n\t"		    \
			STR(PTR)"\t8b, 11b\n\t"		    \
			".previous"			    \
			: "=&r" (value), "=r" (res)	    \
			: "r" (addr), "i" (-EFAULT));       \
} while(0)

#endif /* CONFIG_CPU_MIPSR6 */


#define     _StoreHW(addr, value, res, type) \
do {                                                        \
		__asm__ __volatile__ (                      \
			".set\tnoat\n"                      \
			"1:\t"type##_sb("%1", "1(%2)")"\n"  \
			"srl\t$1, %1, 0x8\n"                \
			"2:\t"type##_sb("$1", "0(%2)")"\n"  \
			".set\tat\n\t"                      \
			"li\t%0, 0\n"                       \
			"3:\n\t"                            \
			".insn\n\t"                         \
			".section\t.fixup,\"ax\"\n\t"       \
			"4:\tli\t%0, %3\n\t"                \
			"j\t3b\n\t"                         \
			".previous\n\t"                     \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 4b\n\t"              \
			STR(PTR)"\t2b, 4b\n\t"              \
			".previous"                         \
			: "=r" (res)                        \
			: "r" (value), "r" (addr), "i" (-EFAULT));\
} while(0)

#ifndef CONFIG_CPU_MIPSR6
#define     _StoreW(addr, value, res, type)  \
do {                                                        \
		__asm__ __volatile__ (                      \
			"1:\t"type##_swl("%1", "(%2)")"\n"  \
			"2:\t"type##_swr("%1", "3(%2)")"\n\t"\
			"li\t%0, 0\n"                       \
			"3:\n\t"                            \
			".insn\n\t"                         \
			".section\t.fixup,\"ax\"\n\t"       \
			"4:\tli\t%0, %3\n\t"                \
			"j\t3b\n\t"                         \
			".previous\n\t"                     \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 4b\n\t"              \
			STR(PTR)"\t2b, 4b\n\t"              \
			".previous"                         \
		: "=r" (res)                                \
		: "r" (value), "r" (addr), "i" (-EFAULT));  \
} while(0)

#define     _StoreDW(addr, value, res) \
do {                                                        \
		__asm__ __volatile__ (                      \
			"1:\tsdl\t%1,(%2)\n"                \
			"2:\tsdr\t%1, 7(%2)\n\t"            \
			"li\t%0, 0\n"                       \
			"3:\n\t"                            \
			".insn\n\t"                         \
			".section\t.fixup,\"ax\"\n\t"       \
			"4:\tli\t%0, %3\n\t"                \
			"j\t3b\n\t"                         \
			".previous\n\t"                     \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 4b\n\t"              \
			STR(PTR)"\t2b, 4b\n\t"              \
			".previous"                         \
		: "=r" (res)                                \
		: "r" (value), "r" (addr), "i" (-EFAULT));  \
} while(0)

#else
/* MIPSR6 has no swl and sdl instructions */
#define     _StoreW(addr, value, res, type)  \
do {                                                        \
		__asm__ __volatile__ (                      \
			".set\tpush\n\t"		    \
			".set\tnoat\n\t"		    \
			"1:"type##_sb("%1", "3(%2)")"\n\t"  \
			"srl\t$1, %1, 0x8\n\t"		    \
			"2:"type##_sb("$1", "2(%2)")"\n\t"  \
			"srl\t$1, $1,  0x8\n\t"		    \
			"3:"type##_sb("$1", "1(%2)")"\n\t"  \
			"srl\t$1, $1, 0x8\n\t"		    \
			"4:"type##_sb("$1", "0(%2)")"\n\t"  \
			".set\tpop\n\t"			    \
			"li\t%0, 0\n"			    \
			"10:\n\t"			    \
			".insn\n\t"			    \
			".section\t.fixup,\"ax\"\n\t"	    \
			"11:\tli\t%0, %3\n\t"		    \
			"j\t10b\n\t"			    \
			".previous\n\t"			    \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 11b\n\t"		    \
			STR(PTR)"\t2b, 11b\n\t"		    \
			STR(PTR)"\t3b, 11b\n\t"		    \
			STR(PTR)"\t4b, 11b\n\t"		    \
			".previous"			    \
		: "=&r" (res)			    	    \
		: "r" (value), "r" (addr), "i" (-EFAULT)    \
		: "memory");                                \
} while(0)

#define     _StoreDW(addr, value, res) \
do {                                                        \
		__asm__ __volatile__ (                      \
			".set\tpush\n\t"		    \
			".set\tnoat\n\t"		    \
			"1:sb\t%1, 7(%2)\n\t"    	    \
			"dsrl\t$1, %1, 0x8\n\t"		    \
			"2:sb\t$1, 6(%2)\n\t"    	    \
			"dsrl\t$1, $1, 0x8\n\t"		    \
			"3:sb\t$1, 5(%2)\n\t"    	    \
			"dsrl\t$1, $1, 0x8\n\t"		    \
			"4:sb\t$1, 4(%2)\n\t"    	    \
			"dsrl\t$1, $1, 0x8\n\t"		    \
			"5:sb\t$1, 3(%2)\n\t"    	    \
			"dsrl\t$1, $1, 0x8\n\t"		    \
			"6:sb\t$1, 2(%2)\n\t"    	    \
			"dsrl\t$1, $1, 0x8\n\t"		    \
			"7:sb\t$1, 1(%2)\n\t"    	    \
			"dsrl\t$1, $1, 0x8\n\t"		    \
			"8:sb\t$1, 0(%2)\n\t"    	    \
			"dsrl\t$1, $1, 0x8\n\t"		    \
			".set\tpop\n\t"			    \
			"li\t%0, 0\n"			    \
			"10:\n\t"			    \
			".insn\n\t"			    \
			".section\t.fixup,\"ax\"\n\t"	    \
			"11:\tli\t%0, %3\n\t"		    \
			"j\t10b\n\t"			    \
			".previous\n\t"			    \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 11b\n\t"		    \
			STR(PTR)"\t2b, 11b\n\t"		    \
			STR(PTR)"\t3b, 11b\n\t"		    \
			STR(PTR)"\t4b, 11b\n\t"		    \
			STR(PTR)"\t5b, 11b\n\t"		    \
			STR(PTR)"\t6b, 11b\n\t"		    \
			STR(PTR)"\t7b, 11b\n\t"		    \
			STR(PTR)"\t8b, 11b\n\t"		    \
			".previous"			    \
		: "=&r" (res)			    	    \
		: "r" (value), "r" (addr), "i" (-EFAULT)    \
		: "memory");                                \
} while(0)

#endif /* CONFIG_CPU_MIPSR6 */

#else /* __BIG_ENDIAN */

#define     _LoadHW(addr, value, res, type)  \
do {                                                        \
		__asm__ __volatile__ (".set\tnoat\n"        \
			"1:\t"type##_lb("%0", "1(%2)")"\n"  \
			"2:\t"type##_lbu("$1", "0(%2)")"\n\t"\
			"sll\t%0, 0x8\n\t"                  \
			"or\t%0, $1\n\t"                    \
			"li\t%1, 0\n"                       \
			"3:\t.set\tat\n\t"                  \
			".insn\n\t"                         \
			".section\t.fixup,\"ax\"\n\t"       \
			"4:\tli\t%1, %3\n\t"                \
			"j\t3b\n\t"                         \
			".previous\n\t"                     \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 4b\n\t"              \
			STR(PTR)"\t2b, 4b\n\t"              \
			".previous"                         \
			: "=&r" (value), "=r" (res)         \
			: "r" (addr), "i" (-EFAULT));       \
} while(0)

#ifndef CONFIG_CPU_MIPSR6
#define     _LoadW(addr, value, res, type)   \
do {                                                        \
		__asm__ __volatile__ (                      \
			"1:\t"type##_lwl("%0", "3(%2)")"\n" \
			"2:\t"type##_lwr("%0", "(%2)")"\n\t"\
			"li\t%1, 0\n"                       \
			"3:\n\t"                            \
			".insn\n\t"                         \
			".section\t.fixup,\"ax\"\n\t"       \
			"4:\tli\t%1, %3\n\t"                \
			"j\t3b\n\t"                         \
			".previous\n\t"                     \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 4b\n\t"              \
			STR(PTR)"\t2b, 4b\n\t"              \
			".previous"                         \
			: "=&r" (value), "=r" (res)         \
			: "r" (addr), "i" (-EFAULT));       \
} while(0)

#else
/* MIPSR6 has no lwl instruction */
#define     _LoadW(addr, value, res, type) \
do {                                                        \
		__asm__ __volatile__ (			    \
			".set\tpush\n"			    \
			".set\tnoat\n\t"		    \
			"1:"type##_lb("%0", "3(%2)")"\n\t"  \
			"2:"type##_lbu("$1", "2(%2)")"\n\t" \
			"sll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"3:"type##_lbu("$1", "1(%2)")"\n\t" \
			"sll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"4:"type##_lbu("$1", "0(%2)")"\n\t" \
			"sll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"li\t%1, 0\n"			    \
			".set\tpop\n"			    \
			"10:\n\t"			    \
			".insn\n\t"			    \
			".section\t.fixup,\"ax\"\n\t"	    \
			"11:\tli\t%1, %3\n\t"		    \
			"j\t10b\n\t"			    \
			".previous\n\t"			    \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 11b\n\t"		    \
			STR(PTR)"\t2b, 11b\n\t"		    \
			STR(PTR)"\t3b, 11b\n\t"		    \
			STR(PTR)"\t4b, 11b\n\t"		    \
			".previous"			    \
			: "=&r" (value), "=r" (res)	    \
			: "r" (addr), "i" (-EFAULT));       \
} while(0)

#endif /* CONFIG_CPU_MIPSR6 */


#define     _LoadHWU(addr, value, res, type) \
do {                                                        \
		__asm__ __volatile__ (                      \
			".set\tnoat\n"                      \
			"1:\t"type##_lbu("%0", "1(%2)")"\n" \
			"2:\t"type##_lbu("$1", "0(%2)")"\n\t"\
			"sll\t%0, 0x8\n\t"                  \
			"or\t%0, $1\n\t"                    \
			"li\t%1, 0\n"                       \
			"3:\n\t"                            \
			".insn\n\t"                         \
			".set\tat\n\t"                      \
			".section\t.fixup,\"ax\"\n\t"       \
			"4:\tli\t%1, %3\n\t"                \
			"j\t3b\n\t"                         \
			".previous\n\t"                     \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 4b\n\t"              \
			STR(PTR)"\t2b, 4b\n\t"              \
			".previous"                         \
			: "=&r" (value), "=r" (res)         \
			: "r" (addr), "i" (-EFAULT));       \
} while(0)

#ifndef CONFIG_CPU_MIPSR6
#define     _LoadWU(addr, value, res, type)  \
do {                                                        \
		__asm__ __volatile__ (                      \
			"1:\t"type##_lwl("%0", "3(%2)")"\n" \
			"2:\t"type##_lwr("%0", "(%2)")"\n\t"\
			"dsll\t%0, %0, 32\n\t"              \
			"dsrl\t%0, %0, 32\n\t"              \
			"li\t%1, 0\n"                       \
			"3:\n\t"                            \
			".insn\n\t"                         \
			"\t.section\t.fixup,\"ax\"\n\t"     \
			"4:\tli\t%1, %3\n\t"                \
			"j\t3b\n\t"                         \
			".previous\n\t"                     \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 4b\n\t"              \
			STR(PTR)"\t2b, 4b\n\t"              \
			".previous"                         \
			: "=&r" (value), "=r" (res)         \
			: "r" (addr), "i" (-EFAULT));       \
} while(0)

#define     _LoadDW(addr, value, res)  \
do {                                                        \
		__asm__ __volatile__ (                      \
			"1:\tldl\t%0, 7(%2)\n"              \
			"2:\tldr\t%0, (%2)\n\t"             \
			"li\t%1, 0\n"                       \
			"3:\n\t"                            \
			".insn\n\t"                         \
			"\t.section\t.fixup,\"ax\"\n\t"     \
			"4:\tli\t%1, %3\n\t"                \
			"j\t3b\n\t"                         \
			".previous\n\t"                     \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 4b\n\t"              \
			STR(PTR)"\t2b, 4b\n\t"              \
			".previous"                         \
			: "=&r" (value), "=r" (res)         \
			: "r" (addr), "i" (-EFAULT));       \
} while(0)

#else
/* MIPSR6 has not lwl and ldl instructions */
#define	    _LoadWU(addr, value, res, type) \
do {                                                        \
		__asm__ __volatile__ (			    \
			".set\tpush\n\t"		    \
			".set\tnoat\n\t"		    \
			"1:"type##_lbu("%0", "3(%2)")"\n\t" \
			"2:"type##_lbu("$1", "2(%2)")"\n\t" \
			"sll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"3:"type##_lbu("$1", "1(%2)")"\n\t" \
			"sll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"4:"type##_lbu("$1", "0(%2)")"\n\t" \
			"sll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"li\t%1, 0\n"			    \
			".set\tpop\n"			    \
			"10:\n\t"			    \
			".insn\n\t"			    \
			".section\t.fixup,\"ax\"\n\t"	    \
			"11:\tli\t%1, %3\n\t"		    \
			"j\t10b\n\t"			    \
			".previous\n\t"			    \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 11b\n\t"		    \
			STR(PTR)"\t2b, 11b\n\t"		    \
			STR(PTR)"\t3b, 11b\n\t"		    \
			STR(PTR)"\t4b, 11b\n\t"		    \
			".previous"			    \
			: "=&r" (value), "=r" (res)	    \
			: "r" (addr), "i" (-EFAULT));       \
} while(0)

#define     _LoadDW(addr, value, res)  \
do {                                                        \
		__asm__ __volatile__ (			    \
			".set\tpush\n\t"		    \
			".set\tnoat\n\t"		    \
			"1:lb\t%0, 7(%2)\n\t"    	    \
			"2:lbu\t$1, 6(%2)\n\t"   	    \
			"dsll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"3:lbu\t$1, 5(%2)\n\t"   	    \
			"dsll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"4:lbu\t$1, 4(%2)\n\t"   	    \
			"dsll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"5:lbu\t$1, 3(%2)\n\t"   	    \
			"dsll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"6:lbu\t$1, 2(%2)\n\t"   	    \
			"dsll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"7:lbu\t$1, 1(%2)\n\t"   	    \
			"dsll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"8:lbu\t$1, 0(%2)\n\t"   	    \
			"dsll\t%0, 0x8\n\t"		    \
			"or\t%0, $1\n\t"		    \
			"li\t%1, 0\n"			    \
			".set\tpop\n\t"			    \
			"10:\n\t"			    \
			".insn\n\t"			    \
			".section\t.fixup,\"ax\"\n\t"	    \
			"11:\tli\t%1, %3\n\t"		    \
			"j\t10b\n\t"			    \
			".previous\n\t"			    \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 11b\n\t"		    \
			STR(PTR)"\t2b, 11b\n\t"		    \
			STR(PTR)"\t3b, 11b\n\t"		    \
			STR(PTR)"\t4b, 11b\n\t"		    \
			STR(PTR)"\t5b, 11b\n\t"		    \
			STR(PTR)"\t6b, 11b\n\t"		    \
			STR(PTR)"\t7b, 11b\n\t"		    \
			STR(PTR)"\t8b, 11b\n\t"		    \
			".previous"			    \
			: "=&r" (value), "=r" (res)	    \
			: "r" (addr), "i" (-EFAULT));       \
} while(0)
#endif /* CONFIG_CPU_MIPSR6 */

#define     _StoreHW(addr, value, res, type) \
do {                                                        \
		__asm__ __volatile__ (                      \
			".set\tnoat\n"                      \
			"1:\t"type##_sb("%1", "0(%2)")"\n"  \
			"srl\t$1,%1, 0x8\n"                 \
			"2:\t"type##_sb("$1", "1(%2)")"\n"  \
			".set\tat\n\t"                      \
			"li\t%0, 0\n"                       \
			"3:\n\t"                            \
			".insn\n\t"                         \
			".section\t.fixup,\"ax\"\n\t"       \
			"4:\tli\t%0, %3\n\t"                \
			"j\t3b\n\t"                         \
			".previous\n\t"                     \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 4b\n\t"              \
			STR(PTR)"\t2b, 4b\n\t"              \
			".previous"                         \
			: "=r" (res)                        \
			: "r" (value), "r" (addr), "i" (-EFAULT));\
} while(0)

#ifndef CONFIG_CPU_MIPSR6
#define     _StoreW(addr, value, res, type)  \
do {                                                        \
		__asm__ __volatile__ (                      \
			"1:\t"type##_swl("%1", "3(%2)")"\n" \
			"2:\t"type##_swr("%1", "(%2)")"\n\t"\
			"li\t%0, 0\n"                       \
			"3:\n\t"                            \
			".insn\n\t"                         \
			".section\t.fixup,\"ax\"\n\t"       \
			"4:\tli\t%0, %3\n\t"                \
			"j\t3b\n\t"                         \
			".previous\n\t"                     \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 4b\n\t"              \
			STR(PTR)"\t2b, 4b\n\t"              \
			".previous"                         \
		: "=r" (res)                                \
		: "r" (value), "r" (addr), "i" (-EFAULT));  \
} while(0)

#define     _StoreDW(addr, value, res) \
do {                                                        \
		__asm__ __volatile__ (                      \
			"1:\tsdl\t%1, 7(%2)\n"              \
			"2:\tsdr\t%1, (%2)\n\t"             \
			"li\t%0, 0\n"                       \
			"3:\n\t"                            \
			".insn\n\t"                         \
			".section\t.fixup,\"ax\"\n\t"       \
			"4:\tli\t%0, %3\n\t"                \
			"j\t3b\n\t"                         \
			".previous\n\t"                     \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 4b\n\t"              \
			STR(PTR)"\t2b, 4b\n\t"              \
			".previous"                         \
		: "=r" (res)                                \
		: "r" (value), "r" (addr), "i" (-EFAULT));  \
} while(0)

#else
/* MIPSR6 has no swl and sdl instructions */
#define     _StoreW(addr, value, res, type)  \
do {                                                        \
		__asm__ __volatile__ (                      \
			".set\tpush\n\t"		    \
			".set\tnoat\n\t"		    \
			"1:"type##_sb("%1", "0(%2)")"\n\t"  \
			"srl\t$1, %1, 0x8\n\t"		    \
			"2:"type##_sb("$1", "1(%2)")"\n\t"  \
			"srl\t$1, $1,  0x8\n\t"		    \
			"3:"type##_sb("$1", "2(%2)")"\n\t"  \
			"srl\t$1, $1, 0x8\n\t"		    \
			"4:"type##_sb("$1", "3(%2)")"\n\t"  \
			".set\tpop\n\t"			    \
			"li\t%0, 0\n"			    \
			"10:\n\t"			    \
			".insn\n\t"			    \
			".section\t.fixup,\"ax\"\n\t"	    \
			"11:\tli\t%0, %3\n\t"		    \
			"j\t10b\n\t"			    \
			".previous\n\t"			    \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 11b\n\t"		    \
			STR(PTR)"\t2b, 11b\n\t"		    \
			STR(PTR)"\t3b, 11b\n\t"		    \
			STR(PTR)"\t4b, 11b\n\t"		    \
			".previous"			    \
		: "=&r" (res)			    	    \
		: "r" (value), "r" (addr), "i" (-EFAULT)    \
		: "memory");                                \
} while(0)

#define     _StoreDW(addr, value, res) \
do {                                                        \
		__asm__ __volatile__ (                      \
			".set\tpush\n\t"		    \
			".set\tnoat\n\t"		    \
			"1:sb\t%1, 0(%2)\n\t"    	    \
			"dsrl\t$1, %1, 0x8\n\t"		    \
			"2:sb\t$1, 1(%2)\n\t"    	    \
			"dsrl\t$1, $1, 0x8\n\t"		    \
			"3:sb\t$1, 2(%2)\n\t"    	    \
			"dsrl\t$1, $1, 0x8\n\t"		    \
			"4:sb\t$1, 3(%2)\n\t"    	    \
			"dsrl\t$1, $1, 0x8\n\t"		    \
			"5:sb\t$1, 4(%2)\n\t"    	    \
			"dsrl\t$1, $1, 0x8\n\t"		    \
			"6:sb\t$1, 5(%2)\n\t"    	    \
			"dsrl\t$1, $1, 0x8\n\t"		    \
			"7:sb\t$1, 6(%2)\n\t"    	    \
			"dsrl\t$1, $1, 0x8\n\t"		    \
			"8:sb\t$1, 7(%2)\n\t"    	    \
			"dsrl\t$1, $1, 0x8\n\t"		    \
			".set\tpop\n\t"			    \
			"li\t%0, 0\n"			    \
			"10:\n\t"			    \
			".insn\n\t"			    \
			".section\t.fixup,\"ax\"\n\t"	    \
			"11:\tli\t%0, %3\n\t"		    \
			"j\t10b\n\t"			    \
			".previous\n\t"			    \
			".section\t__ex_table,\"a\"\n\t"    \
			STR(PTR)"\t1b, 11b\n\t"		    \
			STR(PTR)"\t2b, 11b\n\t"		    \
			STR(PTR)"\t3b, 11b\n\t"		    \
			STR(PTR)"\t4b, 11b\n\t"		    \
			STR(PTR)"\t5b, 11b\n\t"		    \
			STR(PTR)"\t6b, 11b\n\t"		    \
			STR(PTR)"\t7b, 11b\n\t"		    \
			STR(PTR)"\t8b, 11b\n\t"		    \
			".previous"			    \
		: "=&r" (res)			    	    \
		: "r" (value), "r" (addr), "i" (-EFAULT)    \
		: "memory");                                \
} while(0)

#endif /* CONFIG_CPU_MIPSR6 */
#endif

#define LoadHWU(addr, value, res)	_LoadHWU(addr, value, res, kernel)
#define LoadHWUE(addr, value, res)	_LoadHWU(addr, value, res, user)
#define LoadWU(addr, value, res)	_LoadWU(addr, value, res, kernel)
#define LoadWUE(addr, value, res)	_LoadWU(addr, value, res, user)
#define LoadHW(addr, value, res)	_LoadHW(addr, value, res, kernel)
#define LoadHWE(addr, value, res)	_LoadHW(addr, value, res, user)
#define LoadW(addr, value, res)		_LoadW(addr, value, res, kernel)
#define LoadWE(addr, value, res)	_LoadW(addr, value, res, user)
#define LoadDW(addr, value, res)	_LoadDW(addr, value, res)

#define StoreHW(addr, value, res)	_StoreHW(addr, value, res, kernel)
#define StoreHWE(addr, value, res)	_StoreHW(addr, value, res, user)
#define StoreW(addr, value, res)	_StoreW(addr, value, res, kernel)
#define StoreWE(addr, value, res)	_StoreW(addr, value, res, user)
#define StoreDW(addr, value, res)	_StoreDW(addr, value, res)

static int emulate_load_store_insn(ARCH_REG_CTX *regs,
	void *addr, unsigned int *pc)
{
	union mips_instruction insn;
	unsigned long value;
    unsigned int res;
	unsigned long origpc;
	unsigned long orig31;
	void *fault_addr = NULL;
#ifdef	CONFIG_EVA
	mm_segment_t seg;
#endif
	origpc = (unsigned long)pc;
	orig31 = regs->REG_uiReg[31];

	/*
	 * This load never faults.
	 */
    insn.word = *pc;

	switch (insn.i_format.opcode) {
		/*
		 * These are instructions that a compiler doesn't generate.  We
		 * can assume therefore that the code is MIPS-aware and
		 * really buggy.  Emulating these instructions would break the
		 * semantics anyway.
		 */
	case ll_op:
	case lld_op:
	case sc_op:
	case scd_op:

		/*
		 * For these instructions the only way to create an address
		 * error is an attempted access to kernel/supervisor address
		 * space.
		 */
	case ldl_op:
	case ldr_op:
	case lwl_op:
	case lwr_op:
	case sdl_op:
	case sdr_op:
	case swl_op:
	case swr_op:
	case lb_op:
	case lbu_op:
	case sb_op:
		goto sigbus;

		/*
		 * The remaining opcodes are the ones that are really of
		 * interest.
		 */
#ifdef CONFIG_EVA
	case spec3_op:
		/*
		 * we can land here only from kernel accessing user memory,
		 * so we need to "switch" the address limit to user space, so
		 * address check can work properly.
		 */
		seg = get_fs();
		set_fs(USER_DS);
		switch (insn.spec3_format.func) {
		case lhe_op:
			if (!access_ok(VERIFY_READ, addr, 2)) {
				set_fs(seg);
				goto sigbus;
			}
			LoadHWE(addr, value, res);
			if (res) {
				set_fs(seg);
				goto fault;
			}
			compute_return_epc(regs);
			regs->REG_uiReg[insn.spec3_format.rt] = value;
			break;
		case lwe_op:
			if (!access_ok(VERIFY_READ, addr, 4)) {
				set_fs(seg);
				goto sigbus;
			}
				LoadWE(addr, value, res);
			if (res) {
				set_fs(seg);
				goto fault;
			}
			compute_return_epc(regs);
			regs->REG_uiReg[insn.spec3_format.rt] = value;
			break;
		case lhue_op:
			if (!access_ok(VERIFY_READ, addr, 2)) {
				set_fs(seg);
				goto sigbus;
			}
			LoadHWUE(addr, value, res);
			if (res) {
				set_fs(seg);
				goto fault;
			}
			compute_return_epc(regs);
			regs->REG_uiReg[insn.spec3_format.rt] = value;
			break;
		case she_op:
			if (!access_ok(VERIFY_WRITE, addr, 2)) {
				set_fs(seg);
				goto sigbus;
			}
			compute_return_epc(regs);
			value = regs->REG_uiReg[insn.spec3_format.rt];
			StoreHWE(addr, value, res);
			if (res) {
				set_fs(seg);
				goto fault;
			}
			break;
		case swe_op:
			if (!access_ok(VERIFY_WRITE, addr, 4)) {
				set_fs(seg);
				goto sigbus;
			}
			compute_return_epc(regs);
			value = regs->REG_uiReg[insn.spec3_format.rt];
			StoreWE(addr, value, res);
			if (res) {
				set_fs(seg);
				goto fault;
			}
			break;
		default:
			set_fs(seg);
			goto sigill;
		}
		set_fs(seg);
		break;
#endif
	case lh_op:
		if (!access_ok(VERIFY_READ, addr, 2))
			goto sigbus;

		if (IS_ENABLED(CONFIG_EVA)) {
			if (segment_eq(get_fs(), get_ds()))
				LoadHW(addr, value, res);
			else
				LoadHWE(addr, value, res);
		} else {
			LoadHW(addr, value, res);
		}

		if (res)
			goto fault;
		compute_return_epc(regs);
		regs->REG_uiReg[insn.i_format.rt] = value;
		break;

	case lw_op:
		if (!access_ok(VERIFY_READ, addr, 4))
			goto sigbus;

		if (IS_ENABLED(CONFIG_EVA)) {
			if (segment_eq(get_fs(), get_ds()))
				LoadW(addr, value, res);
			else
				LoadWE(addr, value, res);
		} else {
			LoadW(addr, value, res);
		}

		if (res)
			goto fault;
		compute_return_epc(regs);
		regs->REG_uiReg[insn.i_format.rt] = value;
		break;

	case lhu_op:
		if (!access_ok(VERIFY_READ, addr, 2))
			goto sigbus;

		if (IS_ENABLED(CONFIG_EVA)) {
			if (segment_eq(get_fs(), get_ds()))
				LoadHWU(addr, value, res);
			else
				LoadHWUE(addr, value, res);
		} else {
			LoadHWU(addr, value, res);
		}

		if (res)
			goto fault;
		compute_return_epc(regs);
		regs->REG_uiReg[insn.i_format.rt] = value;
		break;

	case lwu_op:
#ifdef CONFIG_64BIT
		/*
		 * A 32-bit kernel might be running on a 64-bit processor.  But
		 * if we're on a 32-bit processor and an i-cache incoherency
		 * or race makes us see a 64-bit instruction here the sdl/sdr
		 * would blow up, so for now we don't handle unaligned 64-bit
		 * instructions on 32-bit kernels.
		 */
		if (!access_ok(VERIFY_READ, addr, 4))
			goto sigbus;

		LoadWU(addr, value, res);
		if (res)
			goto fault;
		compute_return_epc(regs);
		regs->REG_uiReg[insn.i_format.rt] = value;
		break;
#endif /* CONFIG_64BIT */

		/* Cannot handle 64-bit instructions in 32-bit kernel */
		goto sigill;

	case ld_op:
#ifdef CONFIG_64BIT
		/*
		 * A 32-bit kernel might be running on a 64-bit processor.  But
		 * if we're on a 32-bit processor and an i-cache incoherency
		 * or race makes us see a 64-bit instruction here the sdl/sdr
		 * would blow up, so for now we don't handle unaligned 64-bit
		 * instructions on 32-bit kernels.
		 */
		if (!access_ok(VERIFY_READ, addr, 8))
			goto sigbus;

		LoadDW(addr, value, res);
		if (res)
			goto fault;
		compute_return_epc(regs);
		regs->REG_uiReg[insn.i_format.rt] = value;
		break;
#endif /* CONFIG_64BIT */

		/* Cannot handle 64-bit instructions in 32-bit kernel */
		goto sigill;

	case sh_op:
		if (!access_ok(VERIFY_WRITE, addr, 2))
			goto sigbus;

		compute_return_epc(regs);
		value = regs->REG_uiReg[insn.i_format.rt];

		if (IS_ENABLED(CONFIG_EVA)) {
			if (segment_eq(get_fs(), get_ds()))
				StoreHW(addr, value, res);
			else
				StoreHWE(addr, value, res);
		} else {
			StoreHW(addr, value, res);
		}

		if (res)
			goto fault;
		break;

	case sw_op:
		if (!access_ok(VERIFY_WRITE, addr, 4))
			goto sigbus;

		compute_return_epc(regs);
		value = regs->REG_uiReg[insn.i_format.rt];

		if (IS_ENABLED(CONFIG_EVA)) {
			if (segment_eq(get_fs(), get_ds()))
				StoreW(addr, value, res);
			else
				StoreWE(addr, value, res);
		} else {
			StoreW(addr, value, res);
		}

		if (res)
			goto fault;
		break;

	case sd_op:
#ifdef CONFIG_64BIT
		/*
		 * A 32-bit kernel might be running on a 64-bit processor.  But
		 * if we're on a 32-bit processor and an i-cache incoherency
		 * or race makes us see a 64-bit instruction here the sdl/sdr
		 * would blow up, so for now we don't handle unaligned 64-bit
		 * instructions on 32-bit kernels.
		 */
		if (!access_ok(VERIFY_WRITE, addr, 8))
			goto sigbus;

		compute_return_epc(regs);
		value = regs->REG_uiReg[insn.i_format.rt];
		StoreDW(addr, value, res);
		if (res)
			goto fault;
		break;
#endif /* CONFIG_64BIT */

		/* Cannot handle 64-bit instructions in 32-bit kernel */
		goto sigill;

	case lwc1_op:
	case ldc1_op:
	case swc1_op:
	case sdc1_op:
	case cop1x_op:
	{
        ARCH_FPU_CTX   *pFpuCtx;
        LW_VMM_ABORT    abtInfo;

        pFpuCtx = &_G_mipsFpuCtx[LW_CPU_GET_CUR_ID()];
        __ARCH_FPU_SAVE(pFpuCtx);                                       /*  保存当前 FPU CTX            */

		res = fpu_emulator_cop1Handler(regs, pFpuCtx, 1, &fault_addr);
        switch (res) {

        case 0:                                                         /*  成功模拟                    */
            abtInfo.VMABT_uiType = 0;
            __ARCH_FPU_RESTORE(pFpuCtx);                                /*  恢复当前 FPU CTX            */
            break;

        case SIGILL:                                                    /*  未定义指令                  */
            abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
            abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_UNDEF;
            break;

        case SIGBUS:                                                    /*  总线错误                    */
            abtInfo.VMABT_uiMethod = BUS_ADRERR;
            abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_BUS;
            break;

        case SIGSEGV:                                                   /*  地址不合法                  */
            abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_WRITE;
            abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_TERMINAL;
            addr = fault_addr;
            break;

        case SIGFPE:                                                    /*  FPU 错误                    */
        default:
            abtInfo.VMABT_uiMethod = 0;
            abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_FPE;
            break;
        }

        if (abtInfo.VMABT_uiType) {
            PLW_CLASS_TCB   ptcbCur;
            LW_TCB_GET_CUR(ptcbCur);
            API_VmmAbortIsr((addr_t)pc, (ULONG)addr, &abtInfo, ptcbCur);
        }
	}
	/*
	 * 上面已经调用 API_VmmAbortIsr, 返回 0
	 */
        return  (0);

	case msa_op:
		/*
		 * TODO MSA(MIPS SIMD架构)暂不支持模拟
		 */
        goto sigill;
		break;

#ifndef CONFIG_CPU_MIPSR6
    case lwc2_op:
    case ldc2_op:
    case swc2_op:
    case sdc2_op:
        /*
         * TODO 64 位协处理器 2 指令暂不支持模拟
         */
#endif
	default:
		/*
		 * Pheeee...  We encountered an yet unknown instruction or
		 * cache coherence problem.  Die sucker, die ...
		 */
		goto sigill;
	}

    return  (0);

fault:
	/* roll back jump/branch */
	regs->REG_uiCP0EPC  = origpc;
	regs->REG_uiReg[31] = orig31;
    return  (LW_VMM_ABORT_TYPE_TERMINAL);

sigbus:
    return  (LW_VMM_ABORT_TYPE_BUS);

sigill:
    return  (LW_VMM_ABORT_TYPE_UNDEF);
}
/*********************************************************************************************************
** 函数名称: mipsUnalignedHandle
** 功能描述: MIPS 非对齐处理
** 输　入  : pregctx           寄存器上下文
**           ulAbortAddr       终止地址
** 输　出  : 终止类型
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ULONG  mipsUnalignedHandle (ARCH_REG_CTX  *pregctx, addr_t  ulAbortAddr)
{
    MIPS_PARAM  *param = archKernelParamGet();
    UINT        *pc;

    /*
     * Did we catch a fault trying to load an instruction?
     * Or are we running in MIPS16 mode?
     */
    if ((ulAbortAddr == pregctx->REG_uiCP0EPC) || (pregctx->REG_uiCP0EPC & 0x1)) {
        goto    sigbus;
    }

    /*
     * unsupport unalign access
     */
    if (param->MP_bUnalign == LW_FALSE) {
        goto    sigbus;
    }

    pc = (UINT *)exception_epc(pregctx);

    /*
     * Do branch emulation only if we didn't forward the exception.
     * This is all so but ugly ...
     */
    return  (emulate_load_store_insn(pregctx, (VOID *)ulAbortAddr, pc));

sigbus:
    return  (LW_VMM_ABORT_TYPE_BUS);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
