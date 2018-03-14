#include "sys/cdefs.h"
#include "sys/types.h"

#if LW_CFG_SHELL_PASS_CRYPT_EN > 0

#include "crypt_internal.h"

static const unsigned char itoa64[] =		/* 0 ... 63 => ascii - 64 */
	"./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

void
__crypt_to64(char *s, u_int32_t v, int n)
{

	while (--n >= 0) {
		*s++ = itoa64[v & 0x3f];
		v >>= 6;
	}
}

#endif /* LW_CFG_SHELL_PASS_CRYPT_EN > 0 */
