/**
 * @file
 * netdev crc tool for multicast filter hash calculating
 * Verification using sylixos(tm) real-time operating system
 */

/*
 * Copyright (c) 2006-2017 SylixOS Group.
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

#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include <linux/types.h>
#include <linux/compat.h>
#include <linux/crc32.h>

#if BYTE_ORDER == LITTLE_ENDIAN
#define tole(x)     (x)
#else
#define tole(x)     ((((x) & (u32)0x000000ffUL) << 24) | \
                     (((x) & (u32)0x0000ff00UL) <<  8) | \
                     (((x) & (u32)0x00ff0000UL) >>  8) | \
                     (((x) & (u32)0xff000000UL) >> 24))
#endif /* BYTE_ORDER == LITTLE_ENDIAN */

#define CRCPOLY_LE            0xedb88320
#define CRCPOLY_BE            0x04c11db7
#define CRC32C_POLY_LE        0x82F63B78

#define ____cacheline_aligned __attribute__((aligned(64)))

#include "ncrc_crc32table.h"

#if BYTE_ORDER == LITTLE_ENDIAN
#define DO_CRC(x)             crc = t0[(crc ^ (x)) & 255] ^ (crc >> 8)
#define DO_CRC4               (t3[(q) & 255] ^ t2[(q >> 8) & 255] ^ t1[(q >> 16) & 255] ^ t0[(q >> 24) & 255])
#define DO_CRC8               (t7[(q) & 255] ^ t6[(q >> 8) & 255] ^ t5[(q >> 16) & 255] ^ t4[(q >> 24) & 255])
#else
#define DO_CRC(x)             crc = t0[((crc >> 24) ^ (x)) & 255] ^ (crc << 8)
#define DO_CRC4               (t0[(q) & 255] ^ t1[(q >> 8) & 255] ^ t2[(q >> 16) & 255] ^ t3[(q >> 24) & 255])
#define DO_CRC8               (t4[(q) & 255] ^ t5[(q >> 8) & 255] ^ t6[(q >> 16) & 255] ^ t7[(q >> 24) & 255])
#endif /* BYTE_ORDER == LITTLE_ENDIAN */

/* implements slicing-by-4 or slicing-by-8 algorithm */
static inline u32 crc32_body (u32 crc, unsigned char const *buf, size_t len, const u32 (*tab)[256])
{
  const u32 *b;
  size_t  rem_len;
  const u32 *t0 = tab[0], *t1 = tab[1], *t2 = tab[2], *t3 = tab[3];
  u32 q;

  /* Align it */
  if (unlikely((long)buf & 3 && len)) {
	do {
	  DO_CRC(*buf++);
	} while ((--len) && ((long)buf) & 3);
  }

  rem_len = len & 3;
  len = len >> 2;

  b = (const u32 *)buf;

  for (--b; len; --len) {
    q = crc ^ *++b; /* use pre increment for speed */
	crc = DO_CRC4;
  }
  len = rem_len;

  /* And the last few bytes */
  if (len) {
    u8 *p = (u8 *)(b + 1) - 1;

    do {
	  DO_CRC(*++p); /* use pre increment for speed */
	} while (--len);
  }

  return (crc);
}
#undef DO_CRC
#undef DO_CRC4
#undef DO_CRC8

/* crc32_le_generic() - Calculate bitwise little-endian Ethernet AUTODIN II CRC32/CRC32C */
static inline u32 crc32_le_generic (u32 crc, unsigned char const *p, size_t len, const u32 (*tab)[256], u32 polynomial)
{
  crc = cpu_to_le32(crc);
  crc = crc32_body(crc, p, len, tab);
  crc = le32_to_cpu(( __le32)crc);

  return (crc);
}

u32 crc32_le (u32 crc, unsigned char const *p, size_t len)
{
  return crc32_le_generic(crc, p, len, (const u32 (*)[256])crc32table_le, CRCPOLY_LE);
}
/*
 * end
 */
