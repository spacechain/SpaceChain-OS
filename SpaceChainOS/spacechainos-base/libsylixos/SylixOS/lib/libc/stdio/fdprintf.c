/*
 * sylixos hanhui add.
 * 2009.12.18
 */
#include "stdio.h"

#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)

#include "local.h"

int
#if __STDC__
fdprintf(int fd, char const *fmt, ...)
#else
fdprintf(fd, fmt, va_alist)
	int fd;
	char *fmt;
	va_dcl
#endif
{
	int ret;
	va_list ap;
	FILE f;

	f._flags = __SWR | __SNBF;
	f._bf._base = f._p = NULL;
	f._bf._size = f._w = 0;
	f._cookie = (void *)&f;
	f._file = (short)fd;
	f._write = __swrite;
#if __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	ret = vfprintf(&f, fmt, ap);
	va_end(ap);
	*f._p = 0;
	return (ret);
}
#endif  /*  (LW_CFG_DEVICE_EN > 0)      */
