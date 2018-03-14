/**
 * @file
 * SylixOS 2012.09.18 (create)
 * 
 */
/*
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the sendmail distribution.
 *
 *	$Id: cdefs.h,v 1.2 2003/10/20 15:03:16 chrisgreen Exp $
 *	@(#)cdefs.h	8.8 (Berkeley) 1/9/95
 */

#ifndef _SYS_CDEFS_H
#define _SYS_CDEFS_H

# if defined(__cplusplus)
#  define	__BEGIN_DECLS	extern "C" {
#  define	__END_DECLS	};
# else /* defined(__cplusplus) */
#  define	__BEGIN_DECLS
#  define	__END_DECLS
# endif /* defined(__cplusplus) */

# if defined(__cplusplus) && (defined(__CPP_USE_NAMESPACES) || defined(_GLIBCPP_USE_NAMESPACES))
#   define __BEGIN_NAMESPACE_STD        namespace std {
#   define __END_NAMESPACE_STD          }
#   define __USING_NAMESPACE_STD(name)  using std::name;
#   define __BEGIN_NAMESPACE_C99        namespace __c99 {
#   define __END_NAMESPACE_C99          }
#   define __USING_NAMESPACE_C99(name)  using __c99::name;
# else
#   define __BEGIN_NAMESPACE_STD
#   define __END_NAMESPACE_STD
#   define __USING_NAMESPACE_STD(name)
#   define __BEGIN_NAMESPACE_C99
#   define __END_NAMESPACE_C99
#   define __USING_NAMESPACE_C99(name)
# endif /* __cplusplus && __CPP_USE_NAMESPACES */

/*
 * The __CONCAT macro is used to concatenate parts of symbol names, e.g.
 * with "#define OLD(foo) __CONCAT(old,foo)", OLD(foo) produces oldfoo.
 * The __CONCAT macro is a bit tricky -- make sure you don't put spaces
 * in between its arguments.  __CONCAT can also concatenate double-quoted
 * strings produced by the __STRING macro, but this only works with ANSI C.
 */
# if defined(__STDC__) || defined(__cplusplus)
#  define	__P(protos)	protos		/* full-blown ANSI C */
#  ifndef __CONCAT
#   define	__CONCAT(x,y)	x ## y
#  endif /* ! __CONCAT */
#  define	__STRING(x)	#x

#  ifndef __const
#   define	__const		const		/* define reserved names to standard */
#  endif /* ! __const */
#  define	__signed	signed
#  define	__volatile	volatile
#  if defined(__cplusplus)
#   define	__inline	inline		/* convert to C++ keyword */
#  else /* defined(__cplusplus) */
#   ifndef __GNUC__
#    define	__inline			/* delete GCC keyword */
#   endif /* ! __GNUC__ */
#  endif /* defined(__cplusplus) */

# else /* defined(__STDC__) || defined(__cplusplus) */
#  define	__P(protos)	()		/* traditional C preprocessor */
#  ifndef __CONCAT
#   define	__CONCAT(x,y)	x/**/y
#  endif /* ! __CONCAT */
#  define	__STRING(x)	"x"

#  ifndef __GNUC__
#   define	__const				/* delete pseudo-ANSI C keywords */
#   define	__inline
#   define	__signed
#   define	__volatile

/*
 * In non-ANSI C environments, new programs will want ANSI-only C keywords
 * deleted from the program and old programs will want them left alone.
 * When using a compiler other than gcc, programs using the ANSI C keywords
 * const, inline etc. as normal identifiers should define -DNO_ANSI_KEYWORDS.
 * When using "gcc -traditional", we assume that this is the intent; if
 * __GNUC__ is defined but __STDC__ is not, we leave the new keywords alone.
 */
#   ifndef NO_ANSI_KEYWORDS
#    define	const				/* delete ANSI C keywords */
#    define	inline
#    define	signed
#    define	volatile
#   endif /* ! NO_ANSI_KEYWORDS */
#  endif /* ! __GNUC__ */
# endif /* defined(__STDC__) || defined(__cplusplus) */

/*
 * GCC1 and some versions of GCC2 declare dead (non-returning) and
 * pure (no side effects) functions using "volatile" and "const";
 * unfortunately, these then cause warnings under "-ansi -pedantic".
 * GCC2 uses a new, peculiar __attribute__((attrs)) style.  All of
 * these work for GNU C++ (modulo a slight glitch in the C++ grammar
 * in the distribution version of 2.5.5).
 */
# if !defined(__GNUC__) || __GNUC__ < 2 || \
	(__GNUC__ == 2 && __GNUC_MINOR__ < 5)
#  define	__attribute__(x)	/* delete __attribute__ if non-gcc or gcc1 */
#  if defined(__GNUC__) && !defined(__STRICT_ANSI__)
#   define	__dead		__volatile
#   define	__pure		__const
#  endif /* defined(__GNUC__) && !defined(__STRICT_ANSI__) */
# endif /* !defined(__GNUC__) || __GNUC__ < 2 || \ */

/* Delete pseudo-keywords wherever they are not available or needed. */
# ifndef __dead
#  define	__dead
#  define	__pure
# endif /* ! __dead */

#define __PMT(args)	args
#define __const		const
#define __signed	signed
#define __volatile	volatile
#define __DOTS    	, ...
#define __THROW

#define __ptr_t void *
#define __long_double_t  long double

#define __attribute_malloc__
#define __attribute_pure__
#define __attribute_format_strfmon__(a,b)

#ifdef __GNUC__
# define __flexarr      [0]
#elif defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L
# define __flexarr      []
#else
/* Some other non-C99 compiler.  Approximate with [1].  */
# define __flexarr      [1]
#endif /* __GNUC__ */

#ifndef __BOUNDED_POINTERS__
# define __bounded      /* nothing */
# define __unbounded    /* nothing */
# define __ptrvalue     /* nothing */
#endif

#ifndef __UNCONST
# define __UNCONST(a)    ((void *)(unsigned long)(const void *)(a))
#endif

/*
 *  GCC __attribute__
 */
#ifdef __GNUC__
# ifndef __dead
# define __dead volatile
# endif
# if __GNUC__ < 2  || (__GNUC__ == 2 && __GNUC_MINOR__ < 5)
#  ifndef __attribute__
#   define __attribute__(args)
#  endif
# endif
#else
# ifndef __dead
#  define __dead
# endif
# ifndef __attribute__
#  define __attribute__(args)
# endif
#endif

/*
 *  GCC __restrict
 */
#ifndef __GNUC__
# ifndef __restrict
#  define __restrict
# endif
#endif

/*
 *  GCC compiler keywords & attribute
 */
#include "compiler.h"

/*
 *  GCC cdefs_elf.h (SylixOS mode no cdefs_aout.h)
 */
#ifdef __ELF__
# include "cdefs_elf.h"
#else
# include "cdefs_elf.h" /* sylixos only support ELF format (NOT aout) */
#endif

/*
 *  Old ARM CC fixed
 */
#if defined(__CC_ARM) && __CC_ARM == 1
# undef __inline
#endif

/*
 *  GCC elf symbol
 */
#if defined(__TMS320C6X__)
#define __strong_reference(sym,aliassym)
#define __weak_reference(sym,alias)

#else
#ifdef __GNUC__
# define __strong_reference(sym,aliassym)	\
	extern __typeof (sym) aliassym __attribute__ ((__alias__ (#sym)));
# ifdef __ELF__
#  ifdef __STDC__
#   define __weak_reference(sym,alias)	\
	       __asm__(".weak " #alias);	\
	       __asm__(".equ "  #alias ", " #sym)
	       
#  else
#   define __weak_reference(sym,alias)	\
           __asm__(".weak alias");		\
           __asm__(".equ alias, sym")
           
#  endif	/* __STDC__ */
# else	/* !__ELF__ */
#  ifdef __STDC__
#   define __weak_reference(sym,alias)	\
           __asm__(".stabs \"_" #alias "\",11,0,0,0");	\
           __asm__(".stabs \"_" #sym "\",1,0,0,0")

#  else
#   define __weak_reference(sym,alias)	\
           __asm__(".stabs \"_/**/alias\",11,0,0,0");	\
           __asm__(".stabs \"_/**/sym\",1,0,0,0")

#  endif /* __STDC__ */
# endif	/* __ELF__ */
#endif /* __GNUC__ */
#endif /* __TMS320C6X__ */
/*
 * Macros for manipulating "link sets".  Link sets are arrays of pointers
 * to objects, which are gathered up by the linker.
 *
 * Object format-specific code has provided us with the following macros:
 *
 *	__link_set_add_text(set, sym)
 *		Add a reference to the .text symbol `sym' to `set'.
 *
 *	__link_set_add_rodata(set, sym)
 *		Add a reference to the .rodata symbol `sym' to `set'.
 *
 *	__link_set_add_data(set, sym)
 *		Add a reference to the .data symbol `sym' to `set'.
 *
 *	__link_set_add_bss(set, sym)
 *		Add a reference to the .bss symbol `sym' to `set'.
 *
 *	__link_set_decl(set, ptype)
 *		Provide an extern declaration of the set `set', which
 *		contains an array of the pointer type `ptype'.  This
 *		macro must be used by any code which wishes to reference
 *		the elements of a link set.
 *
 *	__link_set_start(set)
 *		This points to the first slot in the link set.
 *
 *	__link_set_end(set)
 *		This points to the (non-existent) slot after the last
 *		entry in the link set.
 *
 *	__link_set_count(set)
 *		Count the number of entries in link set `set'.
 *
 * In addition, we provide the following macros for accessing link sets:
 *
 *	__link_set_foreach(pvar, set)
 *		Iterate over the link set `set'.  Because a link set is
 *		an array of pointers, pvar must be declared as "type **pvar",
 *		and the actual entry accessed as "*pvar".
 *
 *	__link_set_entry(set, idx)
 *		Access the link set entry at index `idx' from set `set'.
 */
#define	__link_set_foreach(pvar, set)					\
	for (pvar = __link_set_start(set); pvar < __link_set_end(set); pvar++)

#define	__link_set_entry(set, idx)	(__link_set_begin(set)[idx])

#endif /* _SYS_CDEFS_H */
