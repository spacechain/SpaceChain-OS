/*
 * SylixOS(TM)  LW : long wing
 * Copyright All Rights Reserved
 *
 * MODULE PRIVATE DYNAMIC MEMORY IN VPROCESS PATCH
 * this file is support current module malloc/free use his own heap in a vprocess.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

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

#include <SylixOS.h>

#ifndef _DIAGASSERT
#define _DIAGASSERT(c)  assert(c)
#endif /* _DIAGASSERT */

#include <sys/cdefs.h>

#ifndef __UNCONST
#define __UNCONST(a)    ((void *)(unsigned long)(const void *)(a))
#endif

#ifndef _RUNETYPE_LOCAL_H_
typedef uint32_t        __nbrune_t;
typedef uint64_t        __runepad_t;
#endif

#endif /* __WCFIX_H */
