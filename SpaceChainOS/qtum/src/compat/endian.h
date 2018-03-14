// Copyright (c) 2014-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COMPAT_ENDIAN_H
#define BITCOIN_COMPAT_ENDIAN_H

#if defined(SYLIXOS)
//#include <endian.h>

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


#else

#if defined(HAVE_CONFIG_H)
#include "config/bitcoin-config.h"
#endif

#include <stdint.h>

#include "compat/byteswap.h"

#if defined(HAVE_ENDIAN_H)
#include <endian.h>
#elif defined(HAVE_SYS_ENDIAN_H)
#include <sys/endian.h>
#endif

#if defined(WORDS_BIGENDIAN)

#if HAVE_DECL_HTOBE16 == 0
inline uint16_t htobe16(uint16_t host_16bits)
{
    return host_16bits;
}
#endif // HAVE_DECL_HTOBE16

#if HAVE_DECL_HTOLE16 == 0
inline uint16_t htole16(uint16_t host_16bits)
{
    return bswap_16(host_16bits);
}
#endif // HAVE_DECL_HTOLE16

#if HAVE_DECL_BE16TOH == 0
inline uint16_t be16toh(uint16_t big_endian_16bits)
{
    return big_endian_16bits;
}
#endif // HAVE_DECL_BE16TOH

#if HAVE_DECL_LE16TOH == 0
inline uint16_t le16toh(uint16_t little_endian_16bits)
{
    return bswap_16(little_endian_16bits);
}
#endif // HAVE_DECL_LE16TOH

#if HAVE_DECL_HTOBE32 == 0
inline uint32_t htobe32(uint32_t host_32bits)
{
    return host_32bits;
}
#endif // HAVE_DECL_HTOBE32

#if HAVE_DECL_HTOLE32 == 0
inline uint32_t htole32(uint32_t host_32bits)
{
    return bswap_32(host_32bits);
}
#endif // HAVE_DECL_HTOLE32

#if HAVE_DECL_BE32TOH == 0
inline uint32_t be32toh(uint32_t big_endian_32bits)
{
    return big_endian_32bits;
}
#endif // HAVE_DECL_BE32TOH

#if HAVE_DECL_LE32TOH == 0
inline uint32_t le32toh(uint32_t little_endian_32bits)
{
    return bswap_32(little_endian_32bits);
}
#endif // HAVE_DECL_LE32TOH

#if HAVE_DECL_HTOBE64 == 0
inline uint64_t htobe64(uint64_t host_64bits)
{
    return host_64bits;
}
#endif // HAVE_DECL_HTOBE64

#if HAVE_DECL_HTOLE64 == 0
inline uint64_t htole64(uint64_t host_64bits)
{
    return bswap_64(host_64bits);
}
#endif // HAVE_DECL_HTOLE64

#if HAVE_DECL_BE64TOH == 0
inline uint64_t be64toh(uint64_t big_endian_64bits)
{
    return big_endian_64bits;
}
#endif // HAVE_DECL_BE64TOH

#if HAVE_DECL_LE64TOH == 0
inline uint64_t le64toh(uint64_t little_endian_64bits)
{
    return bswap_64(little_endian_64bits);
}
#endif // HAVE_DECL_LE64TOH

#else // WORDS_BIGENDIAN

#if HAVE_DECL_HTOBE16 == 0
inline uint16_t htobe16(uint16_t host_16bits)
{
    return bswap_16(host_16bits);
}
#endif // HAVE_DECL_HTOBE16

#if HAVE_DECL_HTOLE16 == 0
inline uint16_t htole16(uint16_t host_16bits)
{
    return host_16bits;
}
#endif // HAVE_DECL_HTOLE16

#if HAVE_DECL_BE16TOH == 0
inline uint16_t be16toh(uint16_t big_endian_16bits)
{
    return bswap_16(big_endian_16bits);
}
#endif // HAVE_DECL_BE16TOH

#if HAVE_DECL_LE16TOH == 0
inline uint16_t le16toh(uint16_t little_endian_16bits)
{
    return little_endian_16bits;
}
#endif // HAVE_DECL_LE16TOH

#if HAVE_DECL_HTOBE32 == 0
inline uint32_t htobe32(uint32_t host_32bits)
{
    return bswap_32(host_32bits);
}
#endif // HAVE_DECL_HTOBE32

#if HAVE_DECL_HTOLE32 == 0
inline uint32_t htole32(uint32_t host_32bits)
{
    return host_32bits;
}
#endif // HAVE_DECL_HTOLE32

#if HAVE_DECL_BE32TOH == 0
inline uint32_t be32toh(uint32_t big_endian_32bits)
{
    return bswap_32(big_endian_32bits);
}
#endif // HAVE_DECL_BE32TOH

#if HAVE_DECL_LE32TOH == 0
inline uint32_t le32toh(uint32_t little_endian_32bits)
{
    return little_endian_32bits;
}
#endif // HAVE_DECL_LE32TOH

#if HAVE_DECL_HTOBE64 == 0
inline uint64_t htobe64(uint64_t host_64bits)
{
    return bswap_64(host_64bits);
}
#endif // HAVE_DECL_HTOBE64

#if HAVE_DECL_HTOLE64 == 0
inline uint64_t htole64(uint64_t host_64bits)
{
    return host_64bits;
}
#endif // HAVE_DECL_HTOLE64

#if HAVE_DECL_BE64TOH == 0
inline uint64_t be64toh(uint64_t big_endian_64bits)
{
    return bswap_64(big_endian_64bits);
}
#endif // HAVE_DECL_BE64TOH

#if HAVE_DECL_LE64TOH == 0
inline uint64_t le64toh(uint64_t little_endian_64bits)
{
    return little_endian_64bits;
}
#endif // HAVE_DECL_LE64TOH

#endif // WORDS_BIGENDIAN
#endif // SYLIXOS
#endif // BITCOIN_COMPAT_ENDIAN_H
