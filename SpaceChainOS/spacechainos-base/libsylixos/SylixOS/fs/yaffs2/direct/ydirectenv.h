/*
 * YAFFS: Yet another Flash File System . A NAND-flash specific file system.
 *
 * Copyright (C) 2002-2011 Aleph One Ltd.
 *   for Toby Churchill Ltd and Brightstar Engineering
 *
 * Created by Charles Manning <charles@aleph1.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2.1 as
 * published by the Free Software Foundation.
 *
 * Note: Only YAFFS headers are LGPL, YAFFS C code is covered by GPL.
 */

/*
 * ydirectenv.h: Environment wrappers for YAFFS direct.
 */

#ifndef __YDIRECTENV_H__
#define __YDIRECTENV_H__

#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "yaffs_osglue.h"
#include "yaffs_hweight.h"

void yaffs_bug_fn(const char *file_name, int line_no);

#define YCHAR char
#define YUCHAR unsigned char
#define _Y(x) x

#ifndef Y_LOFF_T
#define Y_LOFF_T loff_t
#endif

#define yaffs_strcat(a, b)      lib_strcat(a, b)
#define yaffs_strcpy(a, b)	    lib_strcpy(a, b)
#define yaffs_strncpy(a, b, c)	lib_strncpy(a, b, c)
#define yaffs_strnlen(s, m)	    lib_strnlen(s, m)
#ifdef CONFIG_YAFFS_CASE_INSENSITIVE
#define yaffs_strcmp(a, b)	    lib_strcasecmp(a, b)
#define yaffs_strncmp(a, b, c)	lib_strncasecmp(a, b, c)
#else
#define yaffs_strcmp(a, b)	    lib_strcmp(a, b)
#define yaffs_strncmp(a, b, c)	lib_strncmp(a, b, c)
#endif

/* you yaffs own hweight function */
#undef hweight8
#undef hweight32

#define hweight8(x)	            yaffs_hweight8(x)
#define hweight32(x)	        yaffs_hweight32(x)

void yaffs_qsort(void *aa, size_t n, size_t es,
		int (*cmp)(const void *, const void *));

#define sort(base, n, sz, cmp_fn, swp) yaffs_qsort(base, n, sz, cmp_fn)

#define YAFFS_PATH_DIVIDERS  "/"

#ifdef NO_inline
#define inline
#else
#define inline __inline__
#endif

#define cond_resched()  do {} while (0)

#ifdef  __GNUC__
#define yaffs_trace(msk, fmt, ...) do { \
	if(yaffs_trace_mask & (msk)) \
		printf("yaffs: " fmt "\n", ##__VA_ARGS__); \
} while(0)
#else
void yaffsfs_trace(unsigned int msk, const char *fmt, ...);
#define yaffs_trace                 yaffsfs_trace
#endif  /* __GNUC__ */

#define YAFFS_LOSTNFOUND_NAME		"lost+found"
#define YAFFS_LOSTNFOUND_PREFIX		"obj"

#include "yaffscfg.h"

#define Y_CURRENT_TIME yaffsfs_CurrentTime()
#define Y_TIME_CONVERT(x) x

#define YAFFS_ROOT_MODE				0666
#define YAFFS_LOSTNFOUND_MODE		0666

#include "yaffs_list.h"

#include "yaffsfs.h"

#endif


