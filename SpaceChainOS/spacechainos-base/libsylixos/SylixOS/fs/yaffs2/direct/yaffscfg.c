/*
 * YAFFS: Yet Another Flash File System. A NAND-flash specific file system.
 *
 * Copyright (C) 2002-2010 Aleph One Ltd.
 *   for Toby Churchill Ltd and Brightstar Engineering
 *
 * Created by Charles Manning <charles@aleph1.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * yaffscfg.c  The configuration for the "direct" use of yaffs.
 *
 * This file is intended to be modified to your requirements.
 * There is no need to redistribute this file.
 */

#define  __SYLIXOS_KERNEL
#include "yaffscfg.h"
#include "yaffsfs.h"
#include "yaffs_list.h"
#include "../yaffs_guts.h"
#include "../yaffs_sylixos.h"
#include "../yaffs_trace.h"
#include "string.h"

#if LW_CFG_YAFFS_EN > 0

unsigned int yaffs_trace_mask = YAFFS_TRACE_BAD_BLOCKS | YAFFS_TRACE_ALWAYS; /*  only printf bad blocks msg  */

struct yaffs_obj *yaffsfs_FindSymlink(const YCHAR *name, YCHAR **tail, YCHAR **symfile)
{
    YCHAR namebuf[PATH_MAX + 1];
    YCHAR *name_start;
    YCHAR *ptr = namebuf;
    
    struct yaffs_obj *obj = LW_NULL;
    struct yaffs_dev *dev;
    
    if (!name || !tail || !symfile) {
        return  (LW_NULL);
    }
    
    strlcpy(namebuf, name, PATH_MAX + 1);
    ptr++; /* pass first divider */
    
    name_start = ptr;
    while (*ptr && *ptr != YAFFS_PATH_DIVIDERS[0]) {
        ptr++;
    }
    if (*ptr != 0) {
        *ptr++ = 0;
    }
    
    dev = (struct yaffs_dev *)yaffs_getdev(name_start);
    if (dev) {
        obj = yaffs_root(dev);
    } else {
        return  (LW_NULL);
    }
    
    while (*ptr) {
    
        name_start = ptr;
        while (*ptr && *ptr != YAFFS_PATH_DIVIDERS[0]) {
            ptr++;
        }
        if (*ptr != 0) {
            *ptr++ = 0;
        }
        
        obj = yaffs_find_by_name(obj, name_start);
        obj = yaffs_get_equivalent_obj(obj);
        
        if (!obj) {
            return  (LW_NULL);
            
        } else if (obj->variant_type == YAFFS_OBJECT_TYPE_SYMLINK) {
            *tail = (char *)(name + (ptr - namebuf));
            *symfile = (char *)(name + (name_start - namebuf));   /* point to symlink file name */
            return  (obj);
            
        } else if (obj->variant_type != YAFFS_OBJECT_TYPE_DIRECTORY) {
            return  (LW_NULL);
        }
    }
    
    return  (LW_NULL);
}

int yaffsfs_PathBuildLink(YCHAR *buf, YCHAR *dev, YCHAR *prefix, struct yaffs_obj *obj, YCHAR *tail)
{
    if ((obj->variant_type == YAFFS_OBJECT_TYPE_SYMLINK) &&
        (obj->variant.symlink_variant.alias)) {
        
        return  (_PathBuildLink(buf, PATH_MAX + 1, dev, prefix, 
                                obj->variant.symlink_variant.alias, tail));
    }
    
    return  (PX_ERROR);
}

void yaffsfs_SetError(int err)
{
	errno = (err > 0) ? (err) : (-err);     /*  errno is positive number  */
}

int yaffsfs_GetLastError()
{
    return  (errno);
}

void yaffsfs_Lock(void) /* sylixos will lock fs */
{
}

void yaffsfs_Unlock(void)
{
}

u32 yaffsfs_CurrentTime(void)
{
    return (u32)lib_time(LW_NULL);  /*  use UTC time */
}

int yaffsfs_CheckMemRegion (const void *addr, size_t size, int write_request)
{
    return  (addr != LW_NULL) ? 0 : PX_ERROR;
}

void yaffsfs_OSInitialisation (void)
{
}

void *yaffsfs_malloc(size_t size)
{
    return  sys_malloc(size);
}

void yaffsfs_free(void *ptr)
{
	sys_free(ptr);
}

int yaffs_start_up (void)
{
	__yaffsOsInit();
	
	return 0;
}

extern struct list_head yaffsfs_deviceList;

char *yaffs_getdevname (int  index, int  *pindex)
{
    int               i;
    struct list_head *cfg;
    struct yaffs_dev *dev;
    
    if (pindex == LW_NULL) {
        return  (LW_NULL);
    }
    
    if (index < 0) {
        *pindex = -1;
        return  (LW_NULL);
    }
    
    i = -1;     /* first operator is ++! */
    list_for_each(cfg, &yaffsfs_deviceList) {
        i++;
        if (i >= index) {
            break;
        }
    }
    
    if (i >= index) {
        *pindex = i + 1;
        dev = list_entry(cfg, struct yaffs_dev, dev_list);
        return  ((char *)dev->param.name);
    }
    
    *pindex = -1;
    return  (LW_NULL);
}

#ifndef __GNUC__
#include "stdarg.h"
void yaffsfs_trace (unsigned int msk, const char *fmt, ...)
{
    va_list     valist;

    if (yaffs_trace_mask & msk) {
#if __STDC__
        va_start(valist, fmt);
#else
        va_start(valist);
#endif
        vprintf(fmt, valist);
        
        va_end(valist);
        
        puts("\n");
    }
}
#endif  /*  __GNUC__  */

#endif  /*  LW_CFG_YAFFS_EN > 0  */
