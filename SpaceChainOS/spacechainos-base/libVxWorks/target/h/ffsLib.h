/**
 * @file
 * errno library.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#ifndef __VXWORKS_FFSLIB_H
#define __VXWORKS_FFSLIB_H

#include "vxWorksCommon.h"

#define ffsLsb      archFindLsb
#define ffsMsb      archFindMsb
#define ffs32Lsb    archFindLsb
#define ffs32Msb    archFindMsb

#define ffsLsb_autosize(i)  ((sizeof(i) == 8) ? ffs64Lsb(i) : ffs32Lsb(i))
#define ffsMsb_autosize(i)  ((sizeof(i) == 8) ? ffs64Msb(i) : ffs32Msb(i))

#ifdef __cplusplus
extern "C" {
#endif

extern int  archFindLsb(UINT32 ui32);
extern int  archFindMsb(UINT32 ui32);

static inline int ffs64Msb (UINT64 i)
{
    union {
        UINT64 qword;
        UINT32 dwords[2];
    } i_u;

    i_u.qword = i;

#if _BYTE_ORDER == _BIG_ENDIAN
    if (i_u.dwords[0]) {
        return ffsMsb(i_u.dwords[0]) + 32;
    } else {
        return ffsMsb(i_u.dwords[1]);
    }
#else
    if (i_u.dwords[1]) {
        return ffsMsb(i_u.dwords[1]) + 32;
    } else {
        return ffsMsb(i_u.dwords[0]);
    }
#endif /* _BYTE_ORDER */
}

static inline int ffs64Lsb (UINT64 i)
{
    union {
        UINT64 qword;
        UINT32 dwords[2];
    } i_u;

    i_u.qword = i;

#if _BYTE_ORDER == _BIG_ENDIAN
    if (i_u.dwords[1]) {
        return ffsLsb(i_u.dwords[1]);
    } else {
        return ffsLsb(i_u.dwords[0]) + (i ? 32 : 0);
    }
#else
    if (i_u.dwords[0]) {
        return ffsLsb(i_u.dwords[0]);
    } else {
        return ffsLsb(i_u.dwords[1]) + (i ? 32 : 0);
    }
#endif /* _BYTE_ORDER */
}

#ifdef __cplusplus
}
#endif

#endif /* __VXWORKS_FFSLIB_H */
