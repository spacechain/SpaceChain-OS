/* $NetBSD: hmac_sha1.c,v 1.1 2006/10/27 18:22:56 drochner Exp $ */

/*
 * hmac_sha1 - using HMAC from RFC 2104
 */
#include "unistd.h"

#if LW_CFG_SHELL_PASS_CRYPT_EN > 0

#include "mbedtls/sha1.h"
#include "crypt_internal.h"

#define HMAC_HASH sha1
#define HMAC_FUNC __hmac_sha1
#define HMAC_KAT  hmac_kat_sha1

#define HASH_LENGTH 20
#define HASH_CTX mbedtls_sha1_context

/*
 *  polarssl sha1_finish(ctx, output) not SHA1Final(output, ctx)!!!
 */
#define HASH_Init(x)            mbedtls_sha1_starts((x))
#define HASH_Update(x, b, l)    mbedtls_sha1_update((x), (b), (l))
#define HASH_Final(v, x)        mbedtls_sha1_finish((x), (v))

#include "hmac.h"

#endif /* LW_CFG_SHELL_PASS_CRYPT_EN > 0 */
