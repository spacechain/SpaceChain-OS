#ifndef __LINUX_ERR_H
#define __LINUX_ERR_H

#include "compat.h"
#include "errno.h"

/*
 * Kernel pointers have redundant information, so we can use a
 * scheme where we can return either an error code or a dentry
 * pointer with the same return value.
 *
 * This should be a per-architecture thing, to allow different
 * error and pointer decisions.
 */
#define MAX_ERRNO	ERRMAX

#ifndef __ASSEMBLY__

#define IS_ERR_VALUE(x) unlikely((x) >= (unsigned long)-MAX_ERRNO)

static LW_INLINE void *ERR_PTR(long error)
{
	return (void *) error;
}

static LW_INLINE long PTR_ERR(const void *ptr)
{
	return (long) ptr;
}

static LW_INLINE long IS_ERR(const void *ptr)
{
	return IS_ERR_VALUE((unsigned long)ptr);
}

#endif

#endif /* __LINUX_ERR_H */
