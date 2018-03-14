/**
 * @file
 * netdev crc tool for multicast filter hash calculating
 * following handle bit reverse
 * Verification using sylixos(tm) real-time operating system
 */

/*
 * Copyright (c) 2006-2018 SylixOS Group.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 4. This code has been or is applying for intellectual property protection
 *    and can only be used with acoinfo software products.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * Author: Han.hui <hanhui@acoinfo.com>
 *
 */

#ifndef __LINUX_BITREV_H
#define __LINUX_BITREV_H

#include <linux/types.h>

extern u8 const ncrc_rev_table[256];

static inline u8 __bitrev8 (u8 byte)
{
    return (ncrc_rev_table[byte]);
}

static inline u16 __bitrev16 (u16 x)
{
    return (__bitrev8(x & 0xff) << 8) | __bitrev8(x >> 8);
}

static inline u32 __bitrev32 (u32 x)
{
    return (__bitrev16(x & 0xffff) << 16) | __bitrev16(x >> 16);
}

#define __constant_bitrev32(x) \
({ \
    u32 __x = x; \
    __x = (__x >> 16) | (__x << 16); \
    __x = ((__x & (u32)0xFF00FF00UL) >> 8) | ((__x & (u32)0x00FF00FFUL) << 8); \
    __x = ((__x & (u32)0xF0F0F0F0UL) >> 4) | ((__x & (u32)0x0F0F0F0FUL) << 4); \
    __x = ((__x & (u32)0xCCCCCCCCUL) >> 2) | ((__x & (u32)0x33333333UL) << 2); \
    __x = ((__x & (u32)0xAAAAAAAAUL) >> 1) | ((__x & (u32)0x55555555UL) << 1); \
    __x; \
})

#define __constant_bitrev16(x) \
({ \
    u16 __x = x; \
    __x = (__x >> 8) | (__x << 8); \
    __x = ((__x & (u16)0xF0F0U) >> 4) | ((__x & (u16)0x0F0FU) << 4); \
    __x = ((__x & (u16)0xCCCCU) >> 2) | ((__x & (u16)0x3333U) << 2); \
    __x = ((__x & (u16)0xAAAAU) >> 1) | ((__x & (u16)0x5555U) << 1); \
    __x; \
})

#define __constant_bitrev8(x) \
({ \
    u8 __x = x; \
    __x = (__x >> 4) | (__x << 4); \
    __x = ((__x & (u8)0xCCU) >> 2) | ((__x & (u8)0x33U) << 2); \
    __x = ((__x & (u8)0xAAU) >> 1) | ((__x & (u8)0x55U) << 1); \
    __x; \
})

#define bitrev32(x) \
({ \
    u32 __x = x; \
    __builtin_constant_p(__x) ? \
    __constant_bitrev32(__x) : \
    __bitrev32(__x); \
})

#define bitrev16(x) \
({ \
    u16 __x = x; \
    __builtin_constant_p(__x) ? \
    __constant_bitrev16(__x) : \
    __bitrev16(__x); \
})

#define bitrev8(x) \
({ \
    u8 __x = x; \
    __builtin_constant_p(__x) ? \
    __constant_bitrev8(__x) : \
    __bitrev8(__x); \
})

#endif /* __LINUX_BITREV_H */
