/**
 * @file
 * SylixOS 2012.09.18 (create)
 * 
 */
/* $NetBSD: cdefs_elf.h,v 1.29.28.1 2008/10/19 22:18:09 haad Exp $	*/

/*
 * Copyright (c) 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */
 
#ifndef _SYS_CDEFS_ELF_H
#define _SYS_CDEFS_ELF_H

#ifdef __LEADING_UNDERSCORE
# define _C_LABEL(x)        __CONCAT(_,x)
# define _C_LABEL_STRING(x) "_"x
#else
# define _C_LABEL(x)        x
# define _C_LABEL_STRING(x) x
#endif

#if __STDC__
# define ___RENAME(x)       __asm__(___STRING(_C_LABEL(x)))
#else
# ifdef __LEADING_UNDERSCORE
#  define ___RENAME(x)      ____RENAME(_/**/x)
#  define ____RENAME(x)     __asm__(___STRING(x))
# else
#  define ___RENAME(x)      __asm__(___STRING(x))
# endif
#endif

#define	__indr_reference(sym,alias)	/* nada, since we do weak refs */

#if __STDC__
# define __strong_alias(alias,sym)	       				\
         __asm__(".global " _C_LABEL_STRING(#alias) " ; "			\
         _C_LABEL_STRING(#alias) " = " _C_LABEL_STRING(#sym));

# define __weak_alias(alias,sym)						\
         __asm__(".weak " _C_LABEL_STRING(#alias) " ; "			\
         _C_LABEL_STRING(#alias) " = " _C_LABEL_STRING(#sym));

# define __weak_extern(sym)						\
         __asm__(".weak " _C_LABEL_STRING(#sym));

# define __warn_references(sym,msg)					\
         __asm__(".section .gnu.warning." #sym " ; .ascii \"" msg "\" ; .text");

#else /* !__STDC__ */

# ifdef __LEADING_UNDERSCORE

#  define __weak_alias(alias,sym) ___weak_alias(_/**/alias,_/**/sym)

#  define ___weak_alias(alias,sym)					\
          __asm__(".weak alias ; alias = sym");
# else

#  define __weak_alias(alias,sym)						\
          __asm__(".weak alias ; alias = sym");
# endif

# ifdef __LEADING_UNDERSCORE

#  define __weak_extern(sym) ___weak_extern(_/**/sym)

#  define ___weak_extern(sym)						\
          __asm__(".weak sym");
# else

#  define __weak_extern(sym)						\
          __asm__(".weak sym");
# endif

# define __warn_references(sym,msg)					\
         __asm__(".section .gnu.warning.sym ; .ascii msg ; .text");

#endif /* !__STDC__ */

#if __STDC__
# define __SECTIONSTRING(_sec, _str)					\
         __asm__(".section " #_sec " ; .asciz \"" _str "\" ; .text")

#else
# define __SECTIONSTRING(_sec, _str)					\
         __asm__(".section _sec ; .asciz _str ; .text")
#endif

/*
 *  FreeBSD compatibility Note: the goal here is not compatibility to K&R C. Since we know that we
 *  have GCC which understands ANSI C perfectly well, we make use of this.
 */
#ifndef __FBSDID
# define __FBSDID(x) /* nothing */
#endif
#ifndef __RCSID
# define __RCSID(s)	/* nothing */
#endif
#ifndef __SCCSID
# define __SCCSID(s) /* nothing */
#endif
#ifndef __COPYRIGHT
# define __COPYRIGHT(s)
#endif

#ifndef __COPYRIGHT
# define __KERNEL_RCSID(_n, _s)      __RCSID(_s)
#endif
#ifndef __COPYRIGHT
# define __KERNEL_SCCSID(_n, _s)
#endif
#ifndef __COPYRIGHT
# define __KERNEL_COPYRIGHT(_n, _s)  __COPYRIGHT(_s)
#endif


#ifndef __lint__
# define __link_set_make_entry(set, sym)					\
         static void const * const __link_set_##set##_sym_##sym		\
         __section("link_set_" #set) __used = &sym
# define __link_set_make_entry2(set, sym, n)				\
         static void const * const __link_set_##set##_sym_##sym##_##n	\
         __section("link_set_" #set) __used = &sym[n]
#else
# define __link_set_make_entry(set, sym)					\
         extern void const * const __link_set_##set##_sym_##sym
# define __link_set_make_entry2(set, sym, n)				\
         extern void const * const __link_set_##set##_sym_##sym##_##n
#endif /* __lint__ */

#define	__link_set_add_text(set, sym)       __link_set_make_entry(set, sym)
#define	__link_set_add_rodata(set, sym)     __link_set_make_entry(set, sym)
#define	__link_set_add_data(set, sym)       __link_set_make_entry(set, sym)
#define	__link_set_add_bss(set, sym)        __link_set_make_entry(set, sym)
#define	__link_set_add_text2(set, sym, n)   __link_set_make_entry2(set, sym, n)
#define	__link_set_add_rodata2(set, sym, n) __link_set_make_entry2(set, sym, n)
#define	__link_set_add_data2(set, sym, n)   __link_set_make_entry2(set, sym, n)
#define	__link_set_add_bss2(set, sym, n)    __link_set_make_entry2(set, sym, n)

#define	__link_set_decl(set, ptype)					\
        extern ptype * const __start_link_set_##set[];			\
        extern ptype * const __stop_link_set_##set[]			\

#define	__link_set_start(set)   (__start_link_set_##set)
#define	__link_set_end(set)     (__stop_link_set_##set)

#define	__link_set_count(set)						\
        (__link_set_end(set) - __link_set_start(set))
	
#endif /* _SYS_CDEFS_ELF_H */

