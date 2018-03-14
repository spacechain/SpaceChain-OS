/*	$NetBSD: wcscasecmp.c,v 1.2 2006/08/26 22:45:52 christos Exp $	*/

/*
 * Copyright (C) 2006 Aleksey Cheusov
 *
 * This material is provided "as is", with absolutely no warranty expressed
 * or implied. Any use is at your own risk.
 *
 * Permission to use or copy this software for any purpose is hereby granted 
 * without fee. Permission to modify the code and to distribute modified
 * code is also granted without any restrictions.
 */

#ifndef __WCFIX_H
#define __WCFIX_H

#include "SylixOS.h"

#ifndef _DIAGASSERT
#define _DIAGASSERT(c)  assert(c)
#endif /* _DIAGASSERT */

#ifdef __GNUC__
#include "sys/cdefs.h"
#endif

#ifndef __weak_reference
#define __weak_reference(a, b)
#endif

#ifndef __UNCONST
#define __UNCONST(a)    ((void *)(unsigned long)(const void *)(a))
#endif

#ifndef _RUNETYPE_LOCAL_H_
typedef uint32_t        __nbrune_t;
typedef uint64_t        __runepad_t;
#endif

#endif /* __WCFIX_H */
