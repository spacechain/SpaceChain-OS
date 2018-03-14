/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: endian.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2010 年 05 月 22 日
**
** 描        述: 系统大小端.
*********************************************************************************************************/

#ifndef __ENDIAN_H
#define __ENDIAN_H

#include "sys/cdefs.h"
#include "sys/endian.h"
#include "sys/types.h"

__BEGIN_DECLS
uint32_t htonl(uint32_t) __attribute__((__const__));
uint16_t htons(uint16_t) __attribute__((__const__));
uint32_t ntohl(uint32_t) __attribute__((__const__));
uint16_t ntohs(uint16_t) __attribute__((__const__));
__END_DECLS

static __inline uint16_t
bswap16(uint16_t x)
{
    return  (uint16_t)((((x) & 0x00ff) << 8) | 
                      (((x) & 0xff00) >> 8));
}

static __inline uint32_t
bswap32(uint32_t x)
{
    return  (uint32_t)((((x) & 0x000000ff) << 24) | 
                       (((x) & 0x0000ff00) <<  8) | 
                       (((x) & 0x00ff0000) >>  8) | 
                       (((x) & 0xff000000) >> 24));
}

static __inline uint64_t
bswap64(uint64_t x)
{
#if LW_CFG_CPU_WORD_LENGHT == 64
    return  (uint64_t)((((x) & 0x00000000000000ffUL) << 56) |
                       (((x) & 0x000000000000ff00UL) << 40) |
                       (((x) & 0x0000000000ff0000UL) << 24) |
                       (((x) & 0x00000000ff000000UL) <<  8) |
                       (((x) & 0x000000ff00000000UL) >>  8) | 
                       (((x) & 0x0000ff0000000000UL) >> 24) | 
                       (((x) & 0x00ff000000000000UL) >> 40) | 
                       (((x) & 0xff00000000000000UL) >> 56));
#else
    uint32_t tl, th;
    th = bswap32((uint32_t)(x & 0x00000000ffffffffULL));
	tl = bswap32((uint32_t)((x >> 32) & 0x00000000ffffffffULL));
    return ((uint64_t)th << 32) | tl;
#endif /* LW_CFG_CPU_WORD_LENGHT == 64 */
}

#if __BYTE_ORDER == __BIG_ENDIAN

#define	NTOHL(x)	(void) (x)
#define	NTOHS(x)	(void) (x)
#define	HTONL(x)	(void) (x)
#define	HTONS(x)	(void) (x)

#define htobe16(x)	(x)
#define htobe32(x)	(x)
#define htobe64(x)	(x)
#define htole16(x)	bswap16((uint16_t)(x))
#define htole32(x)	bswap32((uint32_t)(x))
#define htole64(x)	bswap64((uint64_t)(x))

#define HTOBE16(x)	(void) (x)
#define HTOBE32(x)	(void) (x)
#define HTOBE64(x)	(void) (x)
#define HTOLE16(x)	(x) = bswap16((uint16_t)(x))
#define HTOLE32(x)	(x) = bswap32((uint32_t)(x))
#define HTOLE64(x)	(x) = bswap64((uint64_t)(x))

#else /* little-endian */

#define	NTOHL(x)	(x) = ntohl((uint32_t)(x))
#define	NTOHS(x)	(x) = ntohs((uint16_t)(x))
#define	HTONL(x)	(x) = htonl((uint32_t)(x))
#define	HTONS(x)	(x) = htons((uint16_t)(x))

#define htobe16(x)	bswap16((uint16_t)(x))
#define htobe32(x)	bswap32((uint32_t)(x))
#define htobe64(x)	bswap64((uint64_t)(x))
#define htole16(x)	(x)
#define htole32(x)	(x)
#define htole64(x)	(x)

#define HTOBE16(x)	(x) = bswap16((uint16_t)(x))
#define HTOBE32(x)	(x) = bswap32((uint32_t)(x))
#define HTOBE64(x)	(x) = bswap64((uint64_t)(x))
#define HTOLE16(x)	(void) (x)
#define HTOLE32(x)	(void) (x)
#define HTOLE64(x)	(void) (x)

#endif /* big/little-endian */

#define be16toh(x)	htobe16(x)
#define be32toh(x)	htobe32(x)
#define be64toh(x)	htobe64(x)
#define le16toh(x)	htole16(x)
#define le32toh(x)	htole32(x)
#define le64toh(x)	htole64(x)

#define BE16TOH(x)	HTOBE16(x)
#define BE32TOH(x)	HTOBE32(x)
#define BE64TOH(x)	HTOBE64(x)
#define LE16TOH(x)	HTOLE16(x)
#define LE32TOH(x)	HTOLE32(x)
#define LE64TOH(x)	HTOLE64(x)

#ifdef __GNUC__

#define __GEN_ENDIAN_ENC(bits, endian) \
static __inline __unused void \
endian ## bits ## enc(void *dst, uint ## bits ## _t u) \
{ \
	u = hto ## endian ## bits (u); \
	__builtin_memcpy(dst, &u, sizeof(u)); \
}

__GEN_ENDIAN_ENC(16, be)
__GEN_ENDIAN_ENC(32, be)
__GEN_ENDIAN_ENC(64, be)
__GEN_ENDIAN_ENC(16, le)
__GEN_ENDIAN_ENC(32, le)
__GEN_ENDIAN_ENC(64, le)
#undef __GEN_ENDIAN_ENC

#define __GEN_ENDIAN_DEC(bits, endian) \
static __inline __unused uint ## bits ## _t \
endian ## bits ## dec(const void *buf) \
{ \
	uint ## bits ## _t u; \
	__builtin_memcpy(&u, buf, sizeof(u)); \
	return endian ## bits ## toh (u); \
}

__GEN_ENDIAN_DEC(16, be)
__GEN_ENDIAN_DEC(32, be)
__GEN_ENDIAN_DEC(64, be)
__GEN_ENDIAN_DEC(16, le)
__GEN_ENDIAN_DEC(32, le)
__GEN_ENDIAN_DEC(64, le)
#undef __GEN_ENDIAN_DEC

#else

static __inline void __unused
be16enc(void *buf, uint16_t u)
{
	uint8_t *p = (uint8_t *)buf;

	p[0] = (uint8_t)(((unsigned)u >> 8) & 0xff);
	p[1] = (uint8_t)(u & 0xff);
}

static __inline void __unused
le16enc(void *buf, uint16_t u)
{
	uint8_t *p = (uint8_t *)buf;

	p[0] = (uint8_t)(u & 0xff);
	p[1] = (uint8_t)(((unsigned)u >> 8) & 0xff);
}

static __inline uint16_t __unused
be16dec(const void *buf)
{
	const uint8_t *p = (const uint8_t *)buf;

	return (uint16_t)((p[0] << 8) | p[1]);
}

static __inline uint16_t __unused
le16dec(const void *buf)
{
	const uint8_t *p = (const uint8_t *)buf;

	return (uint16_t)((p[1] << 8) | p[0]);
}

static __inline void __unused
be32enc(void *buf, uint32_t u)
{
	uint8_t *p = (uint8_t *)buf;

	p[0] = (uint8_t)((u >> 24) & 0xff);
	p[1] = (uint8_t)((u >> 16) & 0xff);
	p[2] = (uint8_t)((u >> 8) & 0xff);
	p[3] = (uint8_t)(u & 0xff);
}

static __inline void __unused
le32enc(void *buf, uint32_t u)
{
	uint8_t *p = (uint8_t *)buf;

	p[0] = (uint8_t)(u & 0xff);
	p[1] = (uint8_t)((u >> 8) & 0xff);
	p[2] = (uint8_t)((u >> 16) & 0xff);
	p[3] = (uint8_t)((u >> 24) & 0xff);
}

static __inline uint32_t __unused
be32dec(const void *buf)
{
	const uint8_t *p = (const uint8_t *)buf;

	return ((p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]);
}

static __inline uint32_t __unused
le32dec(const void *buf)
{
	const uint8_t *p = (const uint8_t *)buf;

	return ((p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0]);
}

static __inline void __unused
be64enc(void *buf, uint64_t u)
{
	uint8_t *p = (uint8_t *)buf;

	be32enc(p, (uint32_t)(u >> 32));
	be32enc(p + 4, (uint32_t)(u & 0xffffffffULL));
}

static __inline void __unused
le64enc(void *buf, uint64_t u)
{
	uint8_t *p = (uint8_t *)buf;

	le32enc(p, (uint32_t)(u & 0xffffffffULL));
	le32enc(p + 4, (uint32_t)(u >> 32));
}

static __inline uint64_t __unused
be64dec(const void *buf)
{
	const uint8_t *p = (const uint8_t *)buf;

	return (((uint64_t)be32dec(p) << 32) | be32dec(p + 4));
}

static __inline uint64_t __unused
le64dec(const void *buf)
{
	const uint8_t *p = (const uint8_t *)buf;

	return (le32dec(p) | ((uint64_t)le32dec(p + 4) << 32));
}

#endif /* __GNUC__ */

#endif                                                                  /*  __ENDIAN_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
