/*
 * sylixos hanhui add.
 * 2009.12.18
 */
#include "stdio.h"

#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)

#include "local.h"

int
#if __STDC__
fdscanf(int fd, char const *fmt, ...)
#else
fdscanf(fd, fmt, va_alist)
	int fd;
	char *fmt;
	va_dcl
#endif
{
	int ret;
	va_list ap;
	FILE f;
	
	f._flags = __SRD | __SNBF;
	f._bf._base = f._p = NULL;
	f._bf._size = f._w = 0;
	f._cookie = (void *)&f;
	f._file = (short)fd;
	f._read = __sread;
	f._ub._base = NULL;
	f._lb._base = NULL;
#if __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	ret = __svfscanf(&f, fmt, ap);
	va_end(ap);
	return (ret);
}
#endif  /*  (LW_CFG_DEVICE_EN > 0)      */
